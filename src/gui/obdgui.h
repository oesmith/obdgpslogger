/* Copyright 2009 Gary Briggs

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

/** \file
  \brief Main obd gui entrypoint
*/

#ifndef __OBDGUI_H
#define __OBDGUI_H


/// UI State
enum ui_state {
	UI_STOPPED, ///< OBD GPS Logger not running
	UI_STARTING, ///< OBD GPS Logger starting
	UI_STARTED ///< OBD GPS Logger started
};

/// UI Trip State
enum trip_state {
	TRIP_NONE, ///< All trip buttons disabled
	TRIP_STARTED, ///< On a trip
	TRIP_STOPPED ///< Not on a trip
};

/// What we want to convert to
enum obd_convert_type {
	CONVERT_KML, ///< Output a google earth file
	CONVERT_CSV ///< Output a comma-separated values file
};

#endif // __OBDGUI_H

