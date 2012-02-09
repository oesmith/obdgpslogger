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

# After this function is called, the image has been sent to the client. Do not print anything after.
function renderGauge($datacolumn, $dataname, $datamin, $datamax, $renderval, $extra) {
	# Number of numbers to draw on the gauge
	$divisions = 7;

	# Border around the dial on the image [the divisions go here]
	$imageborder = 40;

	# Radius of dial
	$gaugeradius = 40;

	$gaugewidth = 2 * $gaugeradius;
	$gaugeheight = $gaugeradius;
	$imagewidth = $gaugewidth + 2 * $imageborder;
	$imageheight = $gaugeheight + 2 * $imageborder;
	$gaugecenterx = $imagewidth / 2;
	$gaugecentery = $imageheight - $imageborder;
	$textradius = $gaugeheight + 20;
	$dialradius = $gaugeheight - 20;

	$font = 1; # Passed to imagestring()

	$im = imagecreate($imagewidth,$imageheight);
	$black = imagecolorallocate ($im,0x00,0x00,0x00);
	$white = imagecolorallocate ($im,0xff,0xff,0xff);
	imagefilledrectangle($im,0,0,$imagewidth,$imageheight,$white);

	imagearc($im, $gaugecenterx, $gaugecentery, $gaugewidth, $gaugeheight*2, 180, 0, $black);

	for($i=0;$i<=$divisions;$i++) {
		$angle = (M_PI/180) * (180 + 180 * $i / $divisions);
		$showval = $datamin + $i * ($datamax - $datamin) / $divisions;
		$xpos = $gaugecenterx + $textradius * cos($angle);
		$ypos = $gaugecentery + $textradius * sin($angle);
		imagestring($im, $font, $xpos, $ypos, round($showval, 0), $black);
	}

	$centerstr = "$dataname : $renderval";

	imagestring($im, $font, $imageborder, $gaugecentery + 10, $centerstr, $black);
	if(!empty($extra) && strlen($extra)) {
		imagestring($im, $font, 2*$imageborder, 2*$imageborder, $extra, $black);
	}

	$angle = (M_PI/180) * (180 + (180 * ($renderval - $datamin) / ($datamax - $datamin)));
	# imagestring($im, $font, $width/2, 30, $angle, $black);

	$endx = $gaugecenterx + $dialradius * cos($angle);
	$endy = $gaugecentery + $dialradius * sin($angle);

	imageline($im, $endx, $endy, $gaugecenterx, $gaugecentery, $black);


	header("Content-type: image/png");
	imagepng($im);
}

?>
