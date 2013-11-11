/*
 * curios-geo-handler.js
 *
 * Copyright (c) 2011-2013, University of Aberdeen. All rights reserved.
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
var osMap, dragControl, marker, map2Marker;

(function($) {
    Drupal.behaviors.field_geo = {
        attach: function(context, settings) {
            // switching between widgets
            $('input[name=hc_northing],input[name=hc_easting]').live("focus", function(event) {
		// if already loaded, don't reload;
		if ($('#map').attr("loaded")) return;

		$('#map').attr("loaded",true);
		$('#map').css("width", "100%");
		$('#map').css("height", "500px"); 
		var E = $("input[name=old_hc_easting]").val(), N = $("input[name=old_hc_northing]").val();
		if isNaN(E) E = 141314;
		if isNaN(N) N = 917410;
		var initialCenterPoint = new OpenSpace.MapPoint(E,N);
		createMap(initialCenterPoint);
                osMap.createMarker(initialCenterPoint);
		createDragControl();
                osMap.addControl(dragControl);
                activateDragControl();
		updateMarkerPositionToTextfield(initialCenterPoint);
            });
	   $('input[name=hc_northing],input[name=hc_easting]').live('blur', function(event) {
		var E = $("input[name=hc_easting]").val(), N = $("input[name=hc_northing]").val();
		if isNaN(E) E = 141314;
		if isNaN(N) N = 917410;
		var newCenterPoint = new OpenSpace.MapPoint(E,N);
		osMap.getMarkerLayer().clearMarkers();
		osMap.createMarker(newCenterPoint);
		osMap.setCenter(newCenterPoint);
	   });
        
            
        }
    }


})(jQuery);



// creates the hand map centred on the initialCenterPoint

function createMap(initialCenterPoint) {
  
	osMap = new OpenSpace.Map("map");
	osMap.setCenter(initialCenterPoint, 5);
}


// updates the display text to the marker location

function updateMarkerPositionToTextfield(lonlat) {
  
	document.getElementsByName('hc_easting')[0].value = Math.round(lonlat.lon);
	document.getElementsByName('hc_northing')[0].value = Math.round(lonlat.lat);
}

// creates the drag control for the map

function createDragControl() {
  

// Set dragging active on the default maker layer on map

	dragControl = new OpenSpace.Control.DragMarkers(osMap.getMarkerLayer(),{

// onDrag create a lonlat variable with the lonlat (easting/northing in this projection) of the marker

		onDrag: function( marker ){
			var lonlat = marker.lonlat;
			updateMarkerPositionToTextfield(lonlat);
		},

// onComplete centre boths maps on the marker lonlat

		onComplete: function( marker ){
			var lonlat = marker.lonlat;
                        osMap.setCenter(lonlat);
		}
	});
}

// Create the marker on the left hand map with the initial centre point


// Centre both maps on the lonlat passed through


function updateMapMarkerPosition(lonlat){
	osMap.getMarkerLayer().clearMarkers();
	map2Marker = osMap.createMarker(lonlat);
	osMap.setCenter(lonlat);
}
// Activate the dragcontrol

function activateDragControl() {
	dragControl.activate();  
	osMap.getMarkerLayer().setDragMode(true);
}
