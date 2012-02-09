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

include_once("rendergauge.php");

function errImg($err) {
	$w = 200;
	$h = 100;
	$im = imagecreate($w,$h);
	$black = imagecolorallocate ($im,0x00,0x00,0x00);
	$white = imagecolorallocate ($im,0xff,0xff,0xff);
	imagefilledrectangle($im,0,0,$w,$h,$white);

	imagestring($im, 2, 10, 10, $err, $black);

	header("Content-type: image/png");
	imagepng($im);
}

# Time to go back [from now]
$startdelta = time() - 1262889680;
if(!empty($_REQUEST['startdelta'])) {
	$startdelta = (int)$_REQUEST['startdelta'];
}

$dbfilename = "ces2010.db";
if(!empty($_REQUEST['dbfilename'])) {
	$dbfilename = $_REQUEST['dbfilename'];
}

# Actual column name in the obd table in the database
$datacolumn = "vss";
if(!empty($_REQUEST['datacolumn'])) {
	$datacolumn = $_REQUEST['datacolumn'];
}

# Human friendly name for column
$dataname = "Vehicle Speed";
if(!empty($_REQUEST['dataname'])) {
	$dataname = $_REQUEST['dataname'];
}

# Minimum reasonable value for data [ie, left of gauge]
$datamin = 0;
if(!empty($_REQUEST['datamin'])) {
	$datamin = $_REQUEST['datamin'];
}

# Maximum reasonable value for data [ie, right of gauge]
$datamax = 255;
if(!empty($_REQUEST['datamax'])) {
	$datamax = $_REQUEST['datamax'];
}


$db = new PDO("sqlite:$dbfilename");
if(!$db) {
	$err = "Error opening database\n$dbfilename\n";
	errImg($err);
	exit(1);
}

# Instead of trying to escape the column name, we check to see if we
#    can find the column they ask for
$sql = "PRAGMA table_info(obd)";

$stmt = $db->prepare($sql);
if(!$stmt) {
	$err = "Couldn't prepare statment\n$sql\n";
	errImg($err);
	exit(1);
}

$stmt->execute();
$found = 0;
while(false != ($row = $stmt->fetch(PDO::FETCH_ASSOC))) {
	if($row['name'] == $datacolumn) {
		$found = 1;
	}
}

if(0 == $found) {
	$err = "Couldn't find column\n$datacolumn\n";
	errImg($err);
	exit(1);
}


$starttime = time() - $startdelta;
$sql = "SELECT $datacolumn,time FROM obd WHERE time>=$starttime ORDER BY time ASC LIMIT 1";

$stmt = $db->prepare($sql);
if(!$stmt) {
	$err = "Couldn't prepare statement\n$sql";
	errImg($err);
	exit(1);
}

$stmt->execute();
$showval = "Unknown";
$showtime = "Unknown";
if(false != ($row = $stmt->fetch(PDO::FETCH_ASSOC))) {
	$showval = $row[$datacolumn];
	$showtime = $row["time"];
}

header("Cache-Control: no-cache, must-revalidate");
header("Expires: Sat, 26 Jul 1997 05:00:00 GMT");
renderGauge($datacolumn, $dataname, $datamin, $datamax, $showval, "");

?>

