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
 \brief For sending OBD data via dbus
 */

#ifndef __OBDDBUS_H
#define __OBDDBUS_H

#ifdef HAVE_DBUS

#include <dbus/dbus.h>

/// Interface name for obdgpslogger dbus calls
#define OBDDBUS_INTERFACENAME "org.icculus.obdgpslogger"

/// We've been sent a message from another app
enum obd_dbus_message {
	OBD_DBUS_NOMESSAGE, ///< No Message waiting at this time
	OBD_DBUS_STARTTRIP ///< Start a trip
};

/// Handle any dbus messages we've been sent
enum obd_dbus_message obdhandledbusmessages();

/// Initialise dbus
int obdinitialisedbus();

/// Signal that we have found a value for this cmd
void obddbussignalpid(struct obdservicecmd *cmd, float value);


#endif //HAVE_DBUS

#endif // __OBDDBUS_H

