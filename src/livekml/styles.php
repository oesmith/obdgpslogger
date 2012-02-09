<?php

/* Copyright 2010 Gary Briggs

This file is part of obdgpslogger.

obdgpslogger is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

obdgpslogger is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with obdgpslogger.  If not, see <http://www.gnu.org/licenses/>.
*/


# Print out a number of styles ranging from red to green
function planStyles($stylecount, $styleprefix, $dom, $parent) {
	/* for($i=0;$i<=$stylecount;$i++) {
		if($i == 0) {
			$color = "ff000000";
		} else {
			$color = sprintf("ff00%02x%02x", 
				(int)(($i-1)*0xFF/($stylecount-1)),
				(int)(0xFF-($i-1)*0xFF/($stylecount-1)));
		}


		$stylenode = $dom->createElement("Style");
		$stylenode->setAttribute('id', $styleprefix . $i);
		$parent->appendChild($stylenode);

		$polystylenode = $dom->createElement("PolyStyle");
		$stylenode->appendChild($polystylenode);

		$polycolornode = $dom->createElement("color", $color);
		$polystylenode->appendChild($polycolornode);
		
		$linestylenode = $dom->createElement("LineStyle");
		$stylenode->appendChild($linestylenode);

		$linecolornode = $dom->createElement("color", $color);
		$linestylenode->appendChild($linecolornode);
	} */

	$stylenode = $dom->createElement("Style");
	$stylenode->setAttribute('id', $styleprefix . "Green");
	$parent->appendChild($stylenode);

	$polystylenode = $dom->createElement("PolyStyle");
	$stylenode->appendChild($polystylenode);

	$polycolornode = $dom->createElement("color", "ff00ff00");
	$polystylenode->appendChild($polycolornode);
	
	$linestylenode = $dom->createElement("LineStyle");
	$stylenode->appendChild($linestylenode);

	$linecolornode = $dom->createElement("color", "ff00ff00");
	$linestylenode->appendChild($linecolornode);


	$stylenode = $dom->createElement("Style");
	$stylenode->setAttribute('id', $styleprefix . "Red");
	$parent->appendChild($stylenode);

	$polystylenode = $dom->createElement("PolyStyle");
	$stylenode->appendChild($polystylenode);

	$polycolornode = $dom->createElement("color", "ff0000ff");
	$polystylenode->appendChild($polycolornode);
	
	$linestylenode = $dom->createElement("LineStyle");
	$stylenode->appendChild($linestylenode);

	$linecolornode = $dom->createElement("color", "ff0000ff");
	$linestylenode->appendChild($linecolornode);
}


# Render a co-ordinate LineString
function renderLineString($dom, $parentnode, $coorStr, $style) {

	// Creates a Placemark and append it to the Document.
	$node = $dom->createElement('Placemark');
	$placeNode = $parentnode->appendChild($node);

	$styleNode = $dom->createElement('styleUrl', "#" . $style);
	$placeNode->appendChild($styleNode);

	$pointNode = $dom->createElement('LineString');
	$placeNode->appendChild($pointNode);

	$coorNode = $dom->createElement('coordinates', $coorStr);
	$pointNode->appendChild($coorNode);

	$extrudeNode = $dom->createElement('extrude', '1');
	$pointNode->appendChild($extrudeNode);

	$tessellateNode = $dom->createElement('tessellate', '1');
	$pointNode->appendChild($tessellateNode);

	$altitudeModeNode = $dom->createElement('altitudeMode', 'relativeToGround');
	$pointNode->appendChild($altitudeModeNode);
}


?>
