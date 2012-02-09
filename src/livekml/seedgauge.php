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

function seedGaugeKML($dom, $gaugesnode, $endtime, $debug, $startdelta, $dbfilename, $column, $name, $min, $max, $ox, $oy, $sx, $sy) {
        $overlayNode = $dom->createElement('ScreenOverlay');
        $nameNode = $dom->createElement('name', $name);
        $overlayNode->appendChild($nameNode);
        
        $overlayXYNode = $dom->createElement('overlayXY');
        $overlayXYNode->setAttribute('x', $ox);
        $overlayXYNode->setAttribute('y', $oy);
        $overlayXYNode->setAttribute('xunits', 'fraction');
        $overlayXYNode->setAttribute('yunits', 'fraction');
        $overlayNode->appendChild($overlayXYNode);
        
        $screenXYNode = $dom->createElement('screenXY');
        $screenXYNode->setAttribute('x', $sx);
        $screenXYNode->setAttribute('y', $sy);
        $screenXYNode->setAttribute('xunits', 'fraction');
        $screenXYNode->setAttribute('yunits', 'fraction');
        $overlayNode->appendChild($screenXYNode);
        
        $sizeXYNode = $dom->createElement('size');
        $sizeXYNode->setAttribute('x', "0.15");
        $sizeXYNode->setAttribute('y', "0.15");
        $sizeXYNode->setAttribute('xunits', 'fraction');
        $sizeXYNode->setAttribute('yunits', 'fraction');
        $overlayNode->appendChild($sizeXYNode);
        
        
        $iconNode = $dom->createElement('Icon');
        $gaugedelta = time() - $endtime;
        $gaugeurl = "http://" . $_SERVER['SERVER_NAME'] .
                dirname(filter_var($_SERVER['PHP_SELF'], FILTER_SANITIZE_STRING)) .
                '/gauge.php' .
                "?startdelta=$startdelta" . "&" .
                "dbfilename=$dbfilename" . "&" .
                "datacolumn=$column" . "&" .
                "datamin=$min" . "&" .
                "datamax=$max" . "&" .
                "dataname=$name" . "&" .
                "debug=$debug";

        $hrefNode = $dom->createElement('href');
        $urlNode = $dom->createCDATASection($gaugeurl);
        $hrefNode->appendChild($urlNode);

        $iconNode->appendChild($hrefNode);
        $overlayNode->appendChild($iconNode);
        $gaugesnode->appendChild($overlayNode);

}

?>
