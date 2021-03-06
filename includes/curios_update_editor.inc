<?php
/*
 * curios_update_editor.inc
 * 
 * Copyright (c) 2011-2013, Stuart Taylor (staylor@abdn.ac.uk),
 * University of Aberdeen. All rights reserved.
 *
 * CURIOS: Linked Data CMS is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software. If not, see <http://www.gnu.org/licenses/>.
 */

require_once('scripts/php_fast_cache.php');

/**
 * Page callback.
 *
 * Displays the update form.
 */
function curios_update_form_page($resource_type, $subject_id = NULL) {
  $config = curios_configuration();

  // Do not show update form for default resource/class.
  if ($resource_type->name == $config->getDefaultResourceId()) {
    return curios_update_page();
  }

  $view_result = _curios_execute_properties_view($resource_type->name, $subject_id);

  if (curios_is_empty_view_result($view_result)) {
    drupal_set_message("Unknown record or unable to retrieve: $resource_type->label; ID: $subject_id.", 'error', FALSE);
    return curios_update_page();
  } else {
    $page = array();
    $page[] = array('#markup' => "<p>Edit the form below to update this record.</p>");
    $page[] = drupal_get_form('curios_update_form', $resource_type, $subject_id, $view_result);

    return $page;
  }
}

/**
 *
 * Checks for empty view results.
 *
 * The results are considered empty if only the URI field is returned.
 */
function curios_is_empty_view_result($view_result) {
 if (empty($view_result)) {
   return TRUE;
 }

 if (count((array)$view_result[0]) <= 1) { // only URI or empty.
   return TRUE;
 }

 return FALSE;
}

/**
 * Executes the properties view for a specified record, used to display the current values on the update form.
 */
function _curios_execute_properties_view($resource_name, $subject_id) {
  $config = curios_configuration();
  $resource_config = $config->getResource($resource_name);
  $view_config_id = $config->getViewIdForResource($resource_name);
  $view_config = $config->getViewSpec($view_config_id);

  return views_get_view_result($view_config->getMachineName(), 'properties', $subject_id, $resource_config->getRDFType());
}

/**
 * Executes a relationship view, used to display the current relationships in the delete relationships fieldset.
 */
function _curios_execute_relationship_view($resource_name, $relationship_name, $subject_id) {
  $config = curios_configuration();
  $resource_config = $config->getResource($resource_name);
  $view_config_id = $config->getViewIdForResource($resource_name);
  $view_config = $config->getViewSpec($view_config_id);

  return views_get_view_result($view_config->getMachineName(), 'object_'.$resource_name.'_'.$relationship_name,
      $subject_id, $resource_config->getRDFType());
}

/**
 * Executes the record meta view for a specified record.
 */
function _curios_execute_record_meta_view($resource_name, $subject_id) {
  $config = curios_configuration();
  $resource_config = $config->getResource($resource_name);
  $view_config_id = $config->getViewIdForResource($resource_name);
  $view_config = $config->getViewSpec($view_config_id);

  return views_get_view_result($view_config->getMachineName(), 'record-meta', $subject_id, $resource_config->getRDFType());
}

/**
 * Page callback.
 *
 * Get the resource type and id to be updated.
 */
function curios_update_page() {
  $page = array();
  $page[] = array('#markup' => "<p>Select the record type and enter the SubjectId below then click 'Next' to continue.</p>");
  $page[] = drupal_get_form('curios_manage_update_form');

  return $page;
}

/**
 * Form callback.
 *
 * The update form. This displays the existing values for a record by executing the appropriate views and using them
 * to render a form. Title, description and literal fields are single value. Relationships can be added or removed, these
 * are multivalued.
 */
function curios_update_form($form, $form_state, $resource_type, $subject_id, $view_result) {
  $config = curios_configuration();
  $browsing_class = $config->getResource($resource_type->name);
  $view_spec_id = $config->getViewIdForResource($browsing_class->getId());
  $view_spec = $config->getViewSpec($view_spec_id);
  $view_path = $view_spec->getPath() .'/'. $subject_id;

  $entity_info = entity_get_info('sparql_views_resource');
  $rdf_mapping = $entity_info['bundles'][$resource_type->name]['rdf_mapping'];
  $field_instances = field_read_instances(array(
    'entity_type' => 'sparql_views_resource',
    'bundle' => $resource_type->name));

    // Get title and description fields from view spec.
  $title_field = $config->getField($view_spec->getTitleFieldId());
  $description_field = $config->getField($view_spec->getDescriptionFieldId());

  $views_title_field = $resource_type->name.'_'.$title_field->getId();
  $title_value = $view_result[0]->{$views_title_field};
  $views_description_field = $resource_type->name.'_'.$description_field->getId();
  $description_value = empty($view_result[0]->{$views_description_field}) ? '' : $view_result[0]->{$views_description_field};

  $form = array();
  $form['status'] = array('#markup' => "<h3>Edit: <strong>".$resource_type->label."</strong>; ID: <strong>". $subject_id ."</strong>; Name: <strong>". $title_value ."</strong></h3>");
  $form['resource'] = array('#type' => 'hidden', '#value' => $resource_type->name);
  $form['identifier'] = array('#type' => 'hidden', '#value' => $subject_id);
  $form['view_path'] = array('#type' => 'hidden', '#value' => $view_path);

  $break = array('#markup' => '<br/>');

  $form[$title_field->getId()] = array(
    '#type' => 'textfield',
    '#title' => t($title_field->getLabel()),
    '#required' => TRUE,
    '#size' => '80',
    '#field_suffix' => '<span class="rdf-predicate">('. $rdf_mapping[$title_field->getId()]['predicates'][0] .')</span>',
    '#default_value' => $title_value,
  );
  $form['old_'.$title_field->getId()] = array(
    '#type' => 'hidden',
    '#value' => $title_value,
  );
  $form[$description_field->getId()] = array(
    '#type'=>'text_format',
    '#title' => t($description_field->getLabel()),
    '#required' => FALSE,
    '#cols' => '80',
    '#rows' => '4',
    '#format' => 'filtered_html',
    '#field_suffix' => '<span class="rdf-predicate">('. $rdf_mapping[$description_field->getId()]['predicates'][0] .')</span>',
    '#default_value' => $description_value,
  );
  $form['old_'.$description_field->getId()] = array(
    '#type' => 'hidden',
    '#value' => $description_value,
  );
  $form[] = $break;

  $form['optional_fields'] = array(
    '#type' => 'fieldset',
    '#title' => t('Fields / Attributes'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  // Populated in fieldset fields and used in relationship fieldsets.
  $relationship_options = array();

  // Include fieldsets.
  _curios_update_fieldset_fields($form, $config, $rdf_mapping, $view_result, $resource_type, $field_instances, $relationship_options);
  _curios_update_fieldset_record_meta($form, $config, $rdf_mapping, $resource_type, $subject_id);
  _curios_update_fieldset_existing_relationships($form, $form_state, $rdf_mapping, $resource_type, $subject_id, $relationship_options);
  _curios_update_fieldset_new_relationships($form, $form_state, $rdf_mapping, $relationship_options);
  _curios_update_fieldset_latest_records($form, $form_state);
  _curios_update_fieldset_delete($form, $config, $rdf_mapping);

  // Submit button.
  $form['submit_update'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes'),
    '#submit' => array('curios_update_form_submit'),
  );
  $form['submit_view'] = array(
    '#type' => 'submit',
    '#value' => t('Save & View record'),
    '#submit' => array('curios_update_form_submit_then_view'),
  );
  $form['submit_discard'] = array(
    '#type' => 'submit',
    '#value' => t('Discard Changes'),
    '#attributes' => array('onclick' => 'return confirm("Are you sure you want to discard your changes?");'),
    '#submit' => array('curios_update_form_submit_discard'),
  );

  return $form;
}

function _curios_update_fieldset_record_meta(&$form, $config, $rdf_mapping, $resource_type, $subject_id) {
  $view_result = _curios_execute_record_meta_view($resource_type->name, $subject_id);

  $owner_field = $config->getField($config->getOwnerFieldId());
  $views_owner_field = $resource_type->name.'_'.$owner_field->getId();
  $owner_value = isset($view_result[0]->$views_owner_field) ? $view_result[0]->$views_owner_field : '';

  $published_field = $config->getField($config->getPublishedFieldId());
  $views_published_field = $resource_type->name.'_'.$published_field->getId();
  $published_value = isset($view_result[0]->$views_published_field) ? $view_result[0]->$views_published_field : '';

  $form['record_meta'] = array(
    '#type' => 'fieldset',
    '#title' => t('Record Maintainance'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['record_meta'][$owner_field->getId()] = array(
    '#type' => 'textfield',
    '#title' => t($owner_field->getLabel()),
    '#required' => FALSE,
    '#size' => '80',
    '#field_suffix' => '<span class="rdf-predicate">('. $rdf_mapping[$owner_field->getId()]['predicates'][0] .')</span>',
    '#default_value' => $owner_value,
  );
  $form['old_'.$owner_field->getId()] = array(
    '#type' => 'hidden',
    '#value' => $owner_value,
  );
  $form['record_meta'][$published_field->getId()] = array(
    '#type' => 'textfield',
    '#title' => t($published_field->getLabel()),
    '#required' => FALSE,
    '#size' => '80',
    '#field_suffix' => '<span class="rdf-predicate">('. $rdf_mapping[$published_field->getId()]['predicates'][0] .')</span>',
    '#default_value' => $published_value,
  );
  $form['old_'.$published_field->getId()] = array(
    '#type' => 'hidden',
    '#value' => $published_value,
  );
}

function _curios_update_fieldset_fields(&$form, $config, $rdf_mapping, $view_result, $resource_type, $field_instances, &$relationship_options) {
  $date_range_position = 0;

  foreach ($field_instances as $instance) {
    if (__is_identifier_field($instance) ||
        __is_title_field($instance) ||
        __is_description_field($instance) ||
        __is_special_field($instance, $config)) {
      continue;
    }

    switch ($instance['ldcms_type']) {
      case 'relationship':
        // Store relationships for this record.
        $relationship_options[$instance['field_name']] = $instance['label'];
        break;
      case 'daterange':
        curios_update_form_add_daterange_field($form, $resource_type, $instance, $rdf_mapping,  $view_result, $date_range_position);
        $date_range_position += 1;
        break;
      case 'autocomplete':
        $autocomplete = TRUE;
	curios_update_form_add_text_field($form, $resource_type, $instance, $rdf_mapping,  $view_result, $autocomplete);
	break;
      case 'text':
      case 'integer':
      default:
        $autocomplete =  FALSE;
	curios_update_form_add_text_field($form, $resource_type, $instance, $rdf_mapping,  $view_result, $autocomplete);	
        break;
    }

    //  if (!empty($value)) {
    //    $form['optional_fields']['#collapsed'] = FALSE;
    //  }
    $form['optional_fields']['#collapsed'] = FALSE;
  }
}

function curios_update_form_add_text_field(&$form, $resource_type, $instance, $rdf_mapping, $view_result, $autocomplete = FALSE) {
  $results_field = $resource_type->name.'_'.$instance['field_name'];
  $value = empty($view_result[0]->$results_field) ? '' : $view_result[0]->$results_field;

  $form['optional_fields'][$instance['field_name']] = array(
    '#type' => 'textfield',
    '#default_value' => $value,
    '#title' => t($instance['label']),
    '#field_suffix' => '<span class="rdf-predicate">('.$rdf_mapping[$instance['field_name']]['predicates'][0].')</span>',
  );

  if ($autocomplete) {
    $form['optional_fields'][$instance['field_name']]['#autocomplete_path'] = 'curios/manage/autocomplete_fields/'. $resource_type->name .'/'. $instance['field_name'];
  }

  $form['old_'.$instance['field_name']] = array(
    '#type' => 'hidden',
    '#value' => $value,
  );
  if ($instance['field_name']=='hc_easting')
      {
	 drupal_add_js('http://openspace.ordnancesurvey.co.uk/osmapapi/openspace.js?key=EA4A2D87E8D473D6E0430B6CA40A9AB7');
	 drupal_add_js(drupal_get_path('module', 'curios') . '/js/curios-geo-handler.js');
	 $form['optional_fields']['hc_easting']['#field_suffix'] = '<span class="rdf-predicate">('.$rdf_mapping[$instance['field_name']]['predicates'][0].')</span><div id="markerPosition"></div><div id="map"></div>';
      }
}

function curios_update_form_add_daterange_field(&$form, $resource_type, $instance, $rdf_mapping, $view_result, $date_range_position) {
  $config = curios_configuration();

  drupal_add_js(drupal_get_path('module', 'curios') . '/js/curios-date-handler.js');
  drupal_add_js(array('curios' => array('path' => drupal_get_path('module', 'curios'), 'field_name' => $instance['field_name'])), 'setting');

  // Date range values.
  $field_suffix = ($date_range_position > 0) ? '_'.$date_range_position : '';
  $date_range_resource = $config->getDateRangeResource();
  $date_from_results_field = $date_range_resource->getId() .'_'. $config->getDateRangeFromId() . $field_suffix;
  $date_to_results_field = $date_range_resource->getId() .'_'. $config->getDateRangeToId() . $field_suffix;

  $date_from = ''; $date_to = ''; $display_value = '';
  if (isset($view_result[0]->{$date_from_results_field})) {
    $date_from = $view_result[0]->{$date_from_results_field};
    $date_to = $view_result[0]->{$date_to_results_field};
    $display_value = curios_format_daterange($date_from, $date_to);
  }

  $form['optional_fields'][$instance['field_name']] = array(
      '#type' => 'textfield',
      '#default_value' => $display_value,
      '#title' => t($instance['label']),
      '#field_suffix' =>
          '<span class="rdf-predicate">(' . $rdf_mapping[$instance['field_name']]['predicates'][0] . ')</span>' .
          '<a href="#" onclick="return false;" class="update-date-btn">[ Edit Date ]</a>' .
          '<div class="date-input-widget"></div>',
      '#attributes' => array(
          'class' => array('datetime-type'),
          'id' => array($instance['field_name']),
      ),
  );

  // add hidden fields for new and old startdate of the datefield.
  $form['optional_fields']['startdate-' . $instance['field_name']] = array(
      '#type' => 'hidden',
      '#default_value' => $date_from,
      '#attributes' => array(
          'name' => array('startdate-' . $instance['field_name']),
          'id' => array('startdate-' . $instance['field_name'])
      ),
  );
  $form['old_' . 'startdate-' . $instance['field_name']] = array(
      '#type' => 'hidden',
      '#value' => $date_from,
  );

  // add hidden fields for new and old enddate of the datefield.
  $form['optional_fields']['enddate-' . $instance['field_name']] = array(
      '#type' => 'hidden',
      '#default_value' => $date_to,
      '#attributes' => array(
          'name' => array('enddate-' . $instance['field_name']),
          'id' => array('enddate-' . $instance['field_name'])
      )
  );
  $form['old_' . 'enddate-' . $instance['field_name']] = array(
      '#type' => 'hidden',
      '#value' => $date_to,
  );
}

function _curios_update_fieldset_existing_relationships(&$form, $form_state, $rdf_mapping, $resource_type, $subject_id, $relationship_options) {
  // Existing relationships.
  $form['relationship_listing_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Existing Relationships'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE, // values stored in a tree for grouping relationship fields.
    '#prefix' => '<div id="relationships-remove-fieldset-wrapper">', // for AJAX callback
    '#suffix' => '</div>',
  );

  if (empty($form_state['storage']['relationship_listing_group_remove'])) {
    $form_state['storage']['relationship_listing_group_remove'] = array();
  }

  $group_count = 0;
  foreach ($relationship_options as $relationship_id=>$relationship_label) {
    $view_results = _curios_execute_relationship_view($resource_type->name, $relationship_id, $subject_id);

    if (!empty($view_results)) {
      $form['relationship_listing_fieldset'][] = array('#markup' => "<h3>$relationship_label:</h3>");
    }

    $previous = array();
    foreach ($view_results as $relationship_value) {
      $current_group_name = 'relationship-'.$group_count;

      // TODO process results properly.
      $relationship_value = array_values((array)$relationship_value);
      $object_value = "$relationship_value[1] ($relationship_value[0])";
      if (in_array($object_value, $previous)) {
        continue;
      }

      // Textbox with current value.
      $form['relationship_listing_fieldset'][$current_group_name]['description'] = array(
        '#type' => 'textfield',
        '#title' => '',
        '#required' => FALSE,
        '#disabled' => TRUE,
        '#size' => '40',
        '#default_value' => $object_value,
      );

      if (in_array($current_group_name, $form_state['storage']['relationship_listing_group_remove'])) {
        $form['relationship_listing_fieldset'][$current_group_name]['description']['#default_value'] = "Delete: [$object_value]";

        $form['relationship_listing_fieldset'][$current_group_name]['remove-relationship'] = array('#type' => 'hidden', '#value' => TRUE);
        $form['relationship_listing_fieldset'][$current_group_name]['field'] = array('#type' => 'hidden', '#value' => $relationship_id);
        $form['relationship_listing_fieldset'][$current_group_name]['value'] = array('#type' => 'hidden', '#value' => $relationship_value[0]);

        $form['relationship_listing_fieldset'][$current_group_name]['undo'] = array(
            '#type' => 'submit',
            '#name' => $current_group_name,
            '#value' => t('Undo'),
            '#submit' => array('curios_update_relationship_listing_undo'),
            '#ajax' => array(
              'callback' => 'curios_update_relationship_listing_callback',
              'wrapper' => 'relationships-remove-fieldset-wrapper',
            ),
            '#limit_validation_errors' => array(),
          );
      } else {
        $form['relationship_listing_fieldset'][$current_group_name]['delete'] = array(
            '#type' => 'submit',
            '#name' => $current_group_name,
            '#value' => t("Delete"),
            '#submit' => array('curios_update_relationship_listing_remove'),
            '#ajax' => array(
              'callback' => 'curios_update_relationship_listing_callback',
              'wrapper' => 'relationships-remove-fieldset-wrapper',
            ),
            '#limit_validation_errors' => array(),
          );
      }

      $form['relationship_listing_fieldset'][] = array('#markup' => '<div class="form-row"></div>');
      $previous[] = $object_value;
      $group_count += 1;
    }
  }

  if (!isset($form['relationship_listing_fieldset'][0])) {
    $form['relationship_listing_fieldset'][] = array('#markup' => "<p>No relationships found.</p>");
  } else {
    $form['relationship_listing_fieldset']['#collapsed'] = FALSE;
  }
}

function _curios_update_fieldset_new_relationships(&$form, &$form_state, $rdf_mapping, $relationship_options) {
  // New relationships.
  $form['relationship_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add Relationship'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE, //empty($form_state['storage']['relationship_group_count']), // collapsed if not used yet
    '#tree' => TRUE, // values stored in a tree for grouping relationship fields.
    '#prefix' => '<div id="relationships-create-fieldset-wrapper">', // for AJAX callback
    '#suffix' => '</div>',
  );
  $form['relationship_fieldset'][] = array('#markup' => '<p>To add a relationship, select the relationship type
      from the drop down menu and enter the SubjectId of the record in the textbox.</p>');

  if (!empty($relationship_options)) { // If this resource has relationship fields.
    if (empty($form_state['storage']['relationship_group_count'])) {
      $form_state['storage']['relationship_group_count'] = 1;
    }
    if (empty($form_state['storage']['relationship_group_remove'])) {
      $form_state['storage']['relationship_group_remove'] = array();
    }

    $relationship_group_count = $form_state['storage']['relationship_group_count'];
    for ($i = 0; $i < $relationship_group_count; $i++) {
      $current_group_name = 'relationship-'.$i;

      if (in_array($current_group_name, $form_state['storage']['relationship_group_remove'])) {
        continue; // Skip removed groups to retain numbering.
      }

      // Field select drop down.
      $form['relationship_fieldset'][$current_group_name]['field'] = array(
        '#type' => 'select',
        '#empty_option' => '- Select relationship -',
        '#required' => FALSE,
        '#options' => $relationship_options,
      );

      // Field value input.
      $form['relationship_fieldset'][$current_group_name]['value'] = array(
        '#type' => 'textfield',
        '#size' => '32',
        '#required' => FALSE,
      );

      // Add or remove button.
      if ($i < $relationship_group_count - 1) {
        $form['relationship_fieldset'][$current_group_name]['remove'] = array(
          '#type' => 'submit',
          '#name' => $current_group_name, // Allows drupal to distinguish multiple remove submit elements.
          '#value' => t('Remove'),
          '#submit' => array('curios_update_relationship_input_remove'),
          '#ajax' => array(
            'callback' => 'curios_update_relationship_input_callback',
            'wrapper' => 'relationships-create-fieldset-wrapper',
          ),
          '#limit_validation_errors' => array(),
        );
        $form['relationship_fieldset'][$current_group_name]['field']['#required'] = TRUE;
        $form['relationship_fieldset'][$current_group_name]['value']['#required'] = TRUE;
      } else {
        $form['relationship_fieldset'][$current_group_name]['add'] = array(
          '#type' => 'submit',
          '#value' => t('Add'),
          '#submit' => array('curios_update_relationship_input_add'),
          '#ajax' => array(
            'callback' => 'curios_update_relationship_input_callback',
            'wrapper' => 'relationships-create-fieldset-wrapper',
          ),
        );
      }

      $form['relationship_fieldset'][] = array('#markup' => '<div class="form-row"></div>');
    }
  } else {
    $form['relationship_fieldset'][] = array('#markup' => "<p>No relationship fields have been defined for this record type.</p>");
  }
}

function _curios_update_fieldset_latest_records(&$form, $form_state) {
  $form['latest_records_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Recently Created Records'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['latest_records_fieldset'][] = array('#markup' =>
    '<input type="button" onclick="jQuery(\'#edit-latest-records-fieldset .fieldset-wrapper\').load(\'/curios/latest-records/10\');" value="Load recently created records" />');
}

function _curios_update_fieldset_delete(&$form, $config, $rdf_mapping) {
  $form['delete_fieldset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Delete Record'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['delete_fieldset']['delete_text'] = array('#markup' => '<p>This will permanently delete the record.</p>');
  $form['delete_fieldset']['delete'] = array(
    '#type' => 'submit',
    '#value' => t('Delete Record'),
    '#attributes' => array('onclick' => 'return confirm("Are you sure you want to permanently delete this record?");'),
    '#submit' => array('curios_delete_form_submit'),
  );
}

/** Ajax callback. */
function curios_update_relationship_listing_callback($form, $form_state) {
  return $form['relationship_listing_fieldset'];
}
/** Ajax callback. */
function curios_update_relationship_listing_remove($form, &$form_state) {
  $form_state['storage']['relationship_listing_group_remove'][] = $form_state['triggering_element']['#name'];
  $form_state['rebuild'] = TRUE;
}
/** Ajax callback. */
function curios_update_relationship_listing_undo($form, &$form_state) {
  $remove = function($value) use (&$form_state) { return $value != $form_state['triggering_element']['#name']; };
  $form_state['storage']['relationship_listing_group_remove'] = array_filter($form_state['storage']['relationship_listing_group_remove'], $remove);
  $form_state['rebuild'] = TRUE;
}
/** Ajax callback. */
function curios_update_relationship_input_add($form, &$form_state) {
  $form_state['storage']['relationship_group_count'] += 1;
  $form_state['rebuild'] = TRUE;
}
/** Ajax callback. */
function curios_update_relationship_input_callback($form, $form_state) {
  return $form['relationship_fieldset'];
}
/** Ajax callback. */
function curios_update_relationship_input_remove($form, &$form_state) {
  $form_state['storage']['relationship_group_remove'][] = $form_state['triggering_element']['#name'];
  $form_state['rebuild'] = TRUE;
}


// VALIDATION HANDLERS

/**
 * Form validation handler.
 *
 * Validates the relationships in the update form. The relationships value is checked syntactically, and the relationship
 * is validated against the configuration.
 */
function curios_update_form_validate($form, &$form_state) {
  $resource_name = $form_state['values']['resource'];

  // Validate relationships.
  $relationships = $form_state['values']['relationship_fieldset'];
  array_pop($relationships);

  foreach ($relationships as $field_name=>$relationship) {
    $error_field_name = "relationship_fieldset][$field_name][value";

    if (!__curios_valid_id($relationship['value'])) {
      form_set_error($error_field_name, t('Relationship value is not a valid subject id.'));
    } else if (!_curios_is_valid_relationship_assertion($resource_name, $relationship['field'], $relationship['value'])) {
      form_set_error($error_field_name, 'Relationship type is not valid for subject '. $relationship['value']. '.');
    }
  }
}

/**
 * Uses the relationship targets defined in the configuration to check whether a relationship is valid.
 */
function _curios_is_valid_relationship_assertion($resource_name, $relationship_name, $subject_id) {
  $config = curios_configuration();
  $resource = $config->getResource($resource_name);
  $targets = $resource->getRelationshipTargets($relationship_name);

  $rdf_types = _curios_fetch_subject_types($subject_id);
  foreach ($rdf_types as $type) {
    $candidate_resource = __curios_resource_id_from_class_uri($type, $config);

    if ($candidate_resource && in_array($candidate_resource, $targets)) {
      return TRUE;
    }
  }

  return FALSE;
}

function curios_update_form_submit_then_view($form, &$form_state) {
  curios_update_form_submit($form, $form_state, 'view');
}

function curios_update_form_submit_discard($form, &$form_state) {
  drupal_set_message("No changes have been made.", 'status', FALSE);
  $path = "curios/manage/edit";
  $form_state['redirect'] = array($path, array(), 302);
}

/**
 * Submit handler.
 *
 * Processes the update form and generates the tripled to be added/removed.
 */
function curios_update_form_submit($form, &$form_state, $redirect = 'edit') {
  $config = curios_configuration();

  $resource_name = $form_state['values']['resource'];
  $subject_id = $form_state['values']['identifier'];
  $view_path = $form_state['values']['view_path'];

  $entity_info = entity_get_info('sparql_views_resource');
  $rdf_mapping = $entity_info['bundles'][$resource_name]['rdf_mapping'];
  $field_instances = field_read_instances(array(
    'entity_type' => 'sparql_views_resource',
    'bundle' => $resource_name,));

  $browsing_class = $config->getResource($resource_name);
  $view_spec_id = $config->getViewIdForResource($browsing_class->getId());
  $view_spec = $config->getViewSpec($view_spec_id);
  $title_field = $config->getField($view_spec->getTitleFieldId());
  $description_field = $config->getField($view_spec->getDescriptionFieldId());

  $triples_delete = array();
  $triples_insert = array();

  $triples_delete_append = array();
  $triples_where_append = array();

  $required_fields = array($title_field->getId(), $description_field->getId());

  // Add required fields.
  foreach ($required_fields as $field_name) {
    $form_value_new = $form_state['values'][$field_name];
    $form_value_old = $form_state['values']['old_'.$field_name];

    if (is_array($form_value_new)) { // if using a formatter
      $form_value_new = $form_state['values'][$field_name]['value'];
    }

    if ($form_value_new != $form_value_old) {
      $triples_delete[] = '      ' . $rdf_mapping[$field_name]['predicates'][0]  ." \"". __curios_escape_sparql($form_value_old) ."\"^^xsd:string;\n";
      $triples_insert[] = '      ' . $rdf_mapping[$field_name]['predicates'][0]  ." \"". __curios_escape_sparql($form_value_new) . "\"^^xsd:string;\n";
    }
  }

  // Calculate difference for remaining fields.
  foreach ($field_instances as $instance) {
    if (__is_identifier_field($instance) ||
        __is_title_field($instance) ||
        __is_description_field($instance) ||
        __is_relationship_field($instance)) {
      continue;
    }

    // Add/remove literal field values.
    if ($instance['ldcms_type'] == 'daterange') { // Handle date range values.
      curios_update_form_submit_date_range($form_state, $instance, $rdf_mapping, $triples_insert, $triples_delete, $triples_delete_append, $triples_where_append);
    } else { // All other field types.
      curios_update_form_submit_field($form_state, $instance, $rdf_mapping, $triples_insert, $triples_delete);
    }
  }

  // Add/remove relationships.
  curios_update_form_submit_relationships($form_state, $instance, $rdf_mapping, $triples_insert, $triples_delete);

  // Filter duplicates and build triples strings.
  $triples_insert = array_unique($triples_insert);
  $triples_insert = join("\n", $triples_insert);
  $triples_delete = array_unique($triples_delete);
  $triples_delete = join("\n", $triples_delete);
  $triples_delete_append = join("\n", $triples_delete_append);
  $triples_where_append = join("\n", $triples_where_append);

  // Submit form.
  if (!empty($triples_delete) || !empty($triples_insert)) {
    $success = _curios_subject_update($subject_id, $triples_insert, $triples_delete, $triples_delete_append, $triples_where_append);

    if ($success) {
      drupal_set_message("The record has been updated successfully (ID: $subject_id).", 'status', FALSE);

      if ($redirect == 'view') {
        $form_state['redirect'] = array($view_path, array(), 302);
      }
    } else {
      drupal_set_message('Failed to update record.', 'error', FALSE);
    }
  } else {
    drupal_set_message('There were no fields to update.', 'warning', FALSE);

    if ($redirect == 'view') {
      $form_state['redirect'] = array($view_path, array(), 302);
    }
  }
}

function curios_update_form_submit_field(&$form_state, $instance, $rdf_mapping, &$triples_insert, &$triples_delete) {
  $form_value_new = $form_state['values'][$instance['field_name']];
  $form_value_old = $form_state['values']['old_'.$instance['field_name']];

  if ($form_value_new != $form_value_old) {
    if (!empty($form_value_old)) {
      $triples_delete[] = '      ' . $rdf_mapping[$instance['field_name']]['predicates'][0]  ." \"". __curios_escape_sparql($form_value_old) ."\"^^xsd:string;";
    }

    if (!empty($form_value_new)) {
      $triples_insert[] = '      ' . $rdf_mapping[$instance['field_name']]['predicates'][0]  ." \"". __curios_escape_sparql($form_value_new) ."\"^^xsd:string;";
    }
  }
}

function curios_update_form_submit_date_range(&$form_state, $instance, $rdf_mapping, &$triples_insert, &$triples_delete, &$triples_delete_append, &$triples_where_append) {
  $config = curios_configuration();

  $date_from_new = $form_state['values']['startdate-'.$instance['field_name']];
  $date_from_old = $form_state['values']['old_'.'startdate-'.$instance['field_name']];
  $date_to_new = $form_state['values']['enddate-'.$instance['field_name']];
  $date_to_old = $form_state['values']['old_'.'enddate-'.$instance['field_name']];

  $date_range_class = $config->getDateRangeResource()->getBaseClass();
  $date_from_field = $config->getField($config->getDateRangeFromId());
  $date_from_property = $date_from_field->getRDFPredicate();
  $date_to_field = $config->getField($config->getDateRangeToId());
  $date_to_property = $date_to_field->getRDFPredicate();

  $date_property = $rdf_mapping[$instance['field_name']]['predicates'][0];
  $field_id = $instance['field_name'];

  if ($date_from_new != $date_from_old || $date_to_new != $date_to_old) {
    if (!empty($date_from_old)) {
      $date_time_triples = "a $date_range_class ; $date_from_property \"$date_from_old\"^^xsd:dateTime ; $date_to_property \"$date_to_old\"^^xsd:dateTime";

      // Delete relationship, DateTime instance -- variables must be bound in WHERE clause.
      $triples_delete[] = "      $date_property ?$field_id;";
      $triples_delete_append[] = "  ?$field_id $date_time_triples .";
      $triples_where_append[] = "  ?s $date_property ?$field_id .";
    }

    if (!empty($date_from_new)) {
      $date_time_triples = "a $date_range_class ; $date_from_property \"$date_from_new\"^^xsd:dateTime ; $date_to_property \"$date_to_new\"^^xsd:dateTime";
      $triples_insert[] = "      $date_property [ $date_time_triples ];";
    }
  }
}

function curios_update_form_submit_relationships(&$form_state, $instance, $rdf_mapping, &$triples_insert, &$triples_delete) {
  $config = curios_configuration();
  $default_namespace = $config->getDefaultNamespace();
  $prefix = $config->getPrefix($default_namespace) .':';

  // Add new relationships - last 'add' item is ignored.
  $relationships = $form_state['values']['relationship_fieldset'];
  array_pop($relationships);

  foreach ($relationships as $relationship) {
    $predicate = $rdf_mapping[$relationship['field']]['predicates'][0];
    $triples_insert[] = '      '. $predicate .' '. $prefix . $relationship['value'] ." ;";
  }

  // Remove existing relationships.
  $relationships = isset($form_state['values']['relationship_listing_fieldset']) ? $form_state['values']['relationship_listing_fieldset'] : array();

  foreach ($relationships as $relationship) {
    if (isset($relationship['remove-relationship'])) {
      $predicate = $rdf_mapping[$relationship['field']]['predicates'][0];
      $triples_delete[] = '      '. $predicate .' '. $prefix . $relationship['value'] ." ;";
    }
  }
}

/**
 * Ajax callback.
 *
 * Returns a list of matching values for a literal field input autocomplete.
 */
function curios_autocomplete_fields($resource_name, $field_name, $q = '') {
  $entity_info = entity_get_info('sparql_views_resource');
  $rdf_mapping = $entity_info['bundles'][$resource_name]['rdf_mapping'];

  $class_name = $rdf_mapping['rdftype'][0];
  $property_name = $rdf_mapping[$field_name]['predicates'][0];

  // Fetch property values.
  $key = __curios_hash($class_name, $property_name);
  $values = phpFastCache::get($key);

  if ($values == NULL) {
    $rs = _curios_fetch_property_values($class_name, $property_name);
    $values = array();

   	if ($rs != NULL) {
			foreach ($rs['result']['rows'] as $row) {
			  $values[$row['value']] = check_plain($row['value']);
			}
		}
  }
  phpFastCache::set($key, $values, 600);

  // Find matches.
  $match_function = function($value) use (&$q) { return startsWith_ncasecmp($q, $value); };
  $matches = array_filter($values, $match_function);

  drupal_json_output($matches);
}
