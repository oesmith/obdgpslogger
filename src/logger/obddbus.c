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
#ifdef HAVE_DBUS

#include <stdio.h>
#include <dbus/dbus.h>
#include "obdservicecommands.h"
#include "obddbus.h"

// Not for you
static DBusConnection* obddbusconn = NULL;

int obdinitialisedbus() {
	DBusError err;

	dbus_error_init(&err);
	obddbusconn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (obddbusconn == NULL) {
		fprintf(stderr, "Error getting dbus system bus: %s\n", err.message);
		return 1;
	}

	// request a name on the bus
	int ret = dbus_bus_request_name(obddbusconn, OBDDBUS_INTERFACENAME, 
			DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	if (dbus_error_is_set(&err)) { 
		// fprintf(stderr, "Error setting dbus name: %s\n", err.message); 
		dbus_error_free(&err); 
	}
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
		// Do something to do with this
	}
	return 0;
}

/// Called if someone tries to do the whole introspection thing
void obddbusintropsect(DBusMessage *msg) {
	fprintf(stderr,"Introspection requested. Need to implement!\n");
#if 0
	const char *intro_xml = "<xml><ishouldfillthisin/></xml>";

	// Flense the #if 0 guards to enable the response. Not working at time of writing.
	DBusMessage* reply;
	DBusMessageIter args;

	reply = dbus_message_new_method_return(msg);
	if(NULL == reply) {
		fprintf(stderr, "Failed to create introspection return msg\n");
		return;
	}

	dbus_message_iter_init_append(reply, &args);
	if (dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, intro_xml)) {
		if (dbus_connection_send(obddbusconn, reply, &serial)) {
			dbus_connection_flush(obddbusconn);
		} else {
			fprintf(stderr, "Failed to send dbus introspection message!\n"); 
		}
	} else {
		fprintf(stderr, "Failed to send introspection xml\n");
	}

	dbus_message_unref(reply);
#endif //0
}

enum obd_dbus_message obdhandledbusmessages() {
	DBusMessage *msg;
	dbus_connection_read_write(obddbusconn, 0);

	enum obd_dbus_message retvalue = OBD_DBUS_NOMESSAGE;
	if(NULL != (msg = dbus_connection_pop_message(obddbusconn))) {
		if(dbus_message_is_method_call(msg, OBDDBUS_INTERFACENAME, "startTrip")) {
			retvalue = OBD_DBUS_STARTTRIP;
		} else if(dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect")) {
			obddbusintropsect(msg);
		}

		dbus_message_unref(msg);
	}

	return retvalue;
}

void obddbussignalpid(struct obdservicecmd *cmd, float value) {
	DBusMessage *msg;
	dbus_uint32_t serial;
	
	if(NULL == obddbusconn) return;

	msg = dbus_message_new_signal("/obd", OBDDBUS_INTERFACENAME, "value");
	if(NULL == msg) return;

	double val = value; // DBus lacks a single float type

	// printf("Sending msg %u %f\n", cmd->cmdid, value);
	dbus_message_append_args (msg,
					DBUS_TYPE_UINT32, &(cmd->cmdid), // PID
					DBUS_TYPE_DOUBLE, &val, // Actual value
					DBUS_TYPE_STRING, &(cmd->db_column), // Short name
					DBUS_TYPE_STRING, &(cmd->human_name), // Long name
					DBUS_TYPE_INVALID // Sentinel
					);

	dbus_message_set_no_reply(msg, TRUE);
	dbus_connection_send(obddbusconn, msg, &serial);
	dbus_message_unref(msg);

}


#endif //HAVE_DBUS

