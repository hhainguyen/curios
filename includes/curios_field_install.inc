<?php
/*
 * curios_field_install.inc
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

/**
 * Batch API start handler.
 *
 * Removes old fields and resources using the previously loaded configuration (if any), and then installs new ones
 * using the current configuration. The batch is triggered from the submit handler for the admin settings page.
 */
function  _curios_update_entity_batch_start(&$config_old, &$config_new, $remove_only = FALSE, $quick_mode = FALSE) {
  $install_fields = $config_new->getFields();
  $install_resources = $config_new->getResources(TRUE);

  $remove_fields = array();
  $remove_resources = array();

  if ($config_old != NULL) {
    if ($quick_mode) { // Calculate shallow diff.
      list($remove_fields, $install_fields, $remove_resources, $install_resources) = _curios_config_diff($config_old, $config_new);
    } else { // Full reinstall.
      $remove_fields = $config_old->getFields();
      $remove_resources = $config_old->getResources(TRUE);
    }
  }
  
  $operations = array();
  $operations[] = array('_curios_remove_rdf_mappings', array(_curios_rdf_mappings($config_old)));
  $operations[] = array('_curios_remove_field_instances', array(_curios_installed_instances($remove_resources, $config_old)));
  $operations[] = array('_curios_remove_fields', array(array_values($remove_fields)));
  $operations[] = array('_curios_remove_resource_types', array(array_values($remove_resources)));
  if (!$remove_only) {
    $operations[] = array('_curios_create_resource_types', array(_curios_installed_resource_types($install_resources, $config_new)));
    $operations[] = array('_curios_create_fields', array(_curios_installed_fields($install_fields, $config_new)));
    $operations[] = array('_curios_create_field_instances', array(_curios_installed_instances($install_resources, $config_new)));
    $operations[] = array('_curios_create_rdf_mappings', array(_curios_rdf_mappings($config_new)));
    $operations[] = array('_curios_finalise_install', array());
  }
  
  // Disable views, pages, etc.
  variable_set('ldcms_installed', FALSE);

  batch_set(array(
    'title' => t('Updating LDCMS Entities'),
    'operations' => $operations,
    'finished' => '_curios_install_config_batch_finished',
    )
  );
  batch_process();
}

/**
 * Batch fininshed handler.
 */
function _curios_install_config_batch_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message('Entity update successful: ' . number_format(count($results)) . ' entities updated.');
    cache_clear_all('*', 'cache_views', TRUE);
    menu_rebuild();
    cache_clear_all(NULL, 'cache_block');
  } else {
    drupal_set_message('Entity installation failed.', 'error');
  }
}

function _curios_finalise_install() {
  variable_set('ldcms_installed', TRUE);
}

/**
 * Creates SPARQL Views resource types.
 */
function _curios_create_resource_types($resource_types, &$context) {
  $limit = 5;

  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($resource_types);
    $context['sandbox']['payload'] = $resource_types;
  }

  $context['message'] = 'Creating new resource types.';

  $count = 0;
  foreach ($context['sandbox']['payload'] as $resource_type) {
    if ($count == $limit) {
      break;
    }

    //$context['message'] = 'Creating resource type: '. $resource_type['name'];

    sparql_views_resource_type_save((object)$resource_type);
    unset($context['sandbox']['payload'][$context['sandbox']['progress']]);

    $context['results'][] = $resource_type['name'];
    $context['sandbox']['progress'] += 1;
    $count += 1;
  }

  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }

}

/**
 * Removes SPARQL Views resource types.
 */
function _curios_remove_resource_types($resource_types, &$context) {
  $limit = 5;

  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($resource_types);
    $context['sandbox']['payload'] = $resource_types;
  }

  $context['message'] = 'Removing unused resource types.';

  $count = 0;
  foreach ($context['sandbox']['payload'] as $resource_type) {
    if ($count == $limit) {
      break;
    }

    sparql_views_resource_type_delete($resource_type->getId());
    unset($context['sandbox']['payload'][$context['sandbox']['progress']]);
    
    $context['results'][] = $resource_type->getId();
    $context['sandbox']['progress'] += 1;
    $count += 1;
  }

  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }

}

/**
 * Creates fields via the Field API.
 */
function _curios_create_fields($fields, &$context) {
  $limit = 10;

  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($fields);
    $context['sandbox']['payload'] = $fields;
  }

  $context['message'] = 'Creating new fields.';

  $count = 0;
  foreach ($context['sandbox']['payload'] as $field) {
    if ($count == $limit) {
      break;
    }

    try {
      field_create_field($field);
    } catch (Exception $e) {
      dd($e->getMessage());
    }

    unset($context['sandbox']['payload'][$context['sandbox']['progress']]);

    $context['sandbox']['progress'] += 1;
    $context['results'][] = $field['field_name'];
    $count += 1;
  }

  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Removes fields via the Field API.
 */
function _curios_remove_fields($fields, &$context) {
  $limit = 10;

  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($fields);
    $context['sandbox']['payload'] = $fields;
  }

  $context['message'] = 'Removing unused fields.';

  $count = 0;
  foreach ($context['sandbox']['payload'] as $field) {
    if ($count == $limit) {
      break;
    }

    try {
      field_delete_field($field->getId());
    } catch (Exception $e) {
      dd($e->getMessage());
    }

    unset($context['sandbox']['payload'][$context['sandbox']['progress']]);

    $context['sandbox']['progress'] += 1;
    $context['results'][] = $field->getId();
    $count += 1;
  }

  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Creates field instances for each resource type.
 */
function _curios_create_field_instances($field_instances, &$context) {
  $limit = 10;

  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($field_instances);
    $context['sandbox']['payload'] = $field_instances;
  }

  $context['message'] = 'Creating field instances.';

  $count = 0;
  foreach ($context['sandbox']['payload'] as $instance) {
    if ($count == $limit) {
      break;
    }

    try {
      field_create_instance($instance);
    } catch (Exception $e) {
      dd($e->getMessage());
    }

    unset($context['sandbox']['payload'][$context['sandbox']['progress']]);

    $context['sandbox']['progress'] += 1;
    $context['results'][] = $instance['field_name'] . ' : ' . $instance['bundle'];
    $count += 1;
  }

  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Removes all field instances.
 */
function _curios_remove_field_instances($field_instances, &$context) {
  $limit = 10;

  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($field_instances);
    $context['sandbox']['payload'] = $field_instances;
  }

  $context['message'] = 'Removing unused field instances.';

  $count = 0;
  foreach ($context['sandbox']['payload'] as $instance) {
    if ($count == $limit) {
      break;
    }

    try {
      field_delete_instance($instance);
    } catch (Exception $e) {
      dd($e->getMessage());
    }

    unset($context['sandbox']['payload'][$context['sandbox']['progress']]);

    $context['sandbox']['progress'] += 1;
    $context['results'][] = $instance['field_name'] . ' : ' . $instance['bundle'];
    $count += 1;
  }

  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

function _curios_create_rdf_mappings($mappings, &$context) {
  $limit = 10;

  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['payload'] = $mappings;
    $context['sandbox']['max'] = count($mappings);
  }

  $context['message'] = 'Saving RDF mappings.';

  $count = 0;
  foreach ($context['sandbox']['payload'] as $mapping) {
    if ($count == $limit) {
      break;
    }

    try {
      rdf_mapping_save($mapping);
    } catch (Exception $e) {
      dd($e->getMessage());
    }

    unset($context['sandbox']['payload'][$context['sandbox']['progress']]);

    $context['sandbox']['progress'] += 1;
    $context['results'][] = $mapping['bundle'];
    $count += 1;
  }

  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

function _curios_remove_rdf_mappings($mappings, &$context) {
  $limit = 10;

  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['payload'] = $mappings;
    $context['sandbox']['max'] = count($mappings);
  }

  $context['message'] = 'Removing RDF mappings.';

  $count = 0;
  foreach ($context['sandbox']['payload'] as $mapping) {
    if ($count == $limit) {
      break;
    }

    try {
      rdf_mapping_delete($mapping['type'], $mapping['bundle']);
    } catch (Exception $e) {
      dd($e->getMessage());
    }

    unset($context['sandbox']['payload'][$context['sandbox']['progress']]);

    $context['sandbox']['progress'] += 1;
    $context['results'][] = $mapping['bundle'];
    $count += 1;
  }

  if ($context['sandbox']['progress'] < $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Calculates a diff between the previous and current configurations to reduce the number of entities being
 * created/removed in the Drupal database.
 */
function _curios_config_diff($config_old, $config_new) {
  // TODO fix this
  
  $remove_fields = array_diff_key($config_old->getFields(), $config_new->getFields());
  $install_fields = array_diff_key($config_new->getFields(), $config_old->getFields());

//  $remove_resources = array_diff_key($config_old->getResources(), $config_new->getResources());
//  $install_resources = array_diff_key($config_new->getResources(), $config_old->getResources());
//  $retain_resources = array_diff_key($config_old->getResources(), $remove_resources);
//
//  $remove_views = array_diff_key($config_old->getViews(), $config_new->getViews());
//  $install_views = array_diff_key($config_new->getViews(), $config_old->getViews());
//  $retain_views = array_diff_key($config_old->getViews(), $remove_resources);
//
//  // Compute shallow diff between resource fields.
//  foreach ($retain_resources as $key=>$old_resource) {
//    $new_resource = $config_new->getResource($key);
//    $old_fields = $old_resource->getFields();
//    $new_fields = $new_resource->getFields();
//
//    $field_diff = array_diff_key($old_fields, $new_fields) + array_diff_key($new_fields, $old_fields);
//
//    if (!empty($field_diff)) {
//      $remove_resources[$key] = $old_resource;
//      $install_resources[$key] = $new_resource;
//    }
//  }

  $remove_resources = $config_old->getResources(TRUE);
  $install_resources = $config_new->getResources(TRUE);

  return array($remove_fields, $install_fields, $remove_resources, $install_resources);
}

/**
 * Creates to input array to install resource types using a configuration.
 */
function _curios_installed_resource_types($resources, &$config) {
  $config_endpoints = $config->getEndpoints();

  $resource_types = array();

  // Define the SPARQL Views resource type.
  foreach ($resources as $entity) {
    $resource_types[] = array(
      'name' => $entity->getMachineName(),
      'label' => t($entity->getLabel()),
      'base' => 'sparql_views_resource',
      'description' => t($entity->getLabel()),
      'endpoints' => array($config_endpoints['dataset']['query'] => TRUE),
    );
  }

  return $resource_types;
}

/**
 * Creates to input array to install fields using a configuration.
 */
function _curios_installed_fields($install_fields, &$config) {
  $fields = array();
  $description_settings = array('text_processing' => 1, 'default_formatter' => 'full_html', 'default_value' => ' ', 'format' => 'full_html', 'value' => ' ');

  foreach ($install_fields as $field) {
    // TODO integrate formatter into field object
    $settings = ($field->getMachineName() == $config->getDescriptionFieldId() ||$field->getMachineName() == 'hc_description') ?
      $description_settings : array('default_formatter' => 'filtered_html');

    $fields[] = array(
      'field_name' => $field->getMachineName(),
      'type' => $field->getDrupalFieldType(),
      'settings' => $settings,
    );
  }

  return $fields;
}

/**
 * Creates to input array to install field instances using a configuration.
 */
function _curios_installed_instances($resources, &$config) {
  $field_instances = array();
  $description_settings = array('text_processing' => 1, 'default_formatter' => 'full_html', 'default_value' => ' ', 'format' => 'full_html', 'value' => ' ');

  foreach ($resources as $entity) {
    foreach ($entity->getFields() as $field) {
      $settings = ($field->getMachineName() == $config->getDescriptionFieldId() ||$field->getMachineName() == 'hc_description') ?
        $description_settings : array('default_formatter' => 'filtered_html');

      $field_instances[] = array(
        'bundle' => $entity->getMachineName(),
        'field_name' => $field->getMachineName(),
        'label' => t($field->getLabel()),
        'entity_type' => 'sparql_views_resource',
        'ldcms_type' => $field->getType(),
        'settings' => $settings,
      );
    }
  }

  return $field_instances;
}

function _curios_rdf_mappings(&$config) {
  $mappings = array();

  foreach ($config->getResources(TRUE) as $entity) {
    $rdf_mapping = array();
    $rdf_mapping['rdftype'] = array($entity->getRDFType());

    foreach ($entity->getFields() as $field) {
      $rdf_mapping[$field->getMachineName()] = array('predicates' => array($field->getRDFPredicate()));
    }

    $mappings[] = array(
        'type' => 'sparql_views_resource',
        'bundle' => $entity->getMachineName(),
        'mapping' => $rdf_mapping);
  }

  return $mappings;
}