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
 \brief DBus data generator
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dbus/dbus.h>

#include "obdservicecommands.h"

#include "datasource.h"

/* Linked lists woo. O(n) happy fun time. On the other hand, we're not
     gonna be seeing more than, like, 20 values... so who cares? */

/// Mappings and most-recently-seen values are stored in this list
/** DBus messages are expected to come as two numbers; an integer then
   an integer or floating point number. The first is mapped onto a PID
   using this table. The second is the actual value to be passed.
*/
struct dbus_simvals {
	int map_from; //< The first number in the message
	unsigned int pid; //< Is mapped to this OBDII PID
	struct obdservicecmd *pid_cmd; // Cache the cmd for this PID
	float most_recent; //< The most recent value we saw for this PID
	struct dbus_simvals *next; //< Yay linked lists
};

/// This is the void * generator
struct dbus_gen {
        DBusConnection *dbusconn; //< The dbus connection
	struct dbus_simvals *simval_list; //< Head of the linked list
	unsigned long supportedpids_00; // Supported pids according to 0100
	unsigned long supportedpids_20; // Supported pids according to 0120
	unsigned long supportedpids_40; // Supported pids according to 0140
	unsigned long supportedpids_60; // Supported pids according to 0160
};

/// This is what parses messages
static DBusHandlerResult dbus_simgen_msgfilter
      (DBusConnection *connection, DBusMessage *message, void *gen);
 
/// Find the dbus_simval for this map_from value
struct dbus_simvals *dbus_simgen_findsimval_from(void *gen, int from);

/// Find the dbus_simval for this map_to (PID) value
struct dbus_simvals *dbus_simgen_findsimval_to(void *gen, int to);

/// Flush the queue of dbus messages waiting for us
static void dbus_simgen_flushqueue(struct dbus_gen *gen, int timems);

const char *dbus_simgen_name() {
	return "DBus";
}

const char *dbus_simgen_longdesc() {
	return "Accept signals from dbus\n"
		"Seed: <config file>";
}

int dbus_simgen_create(void **gen, const char *seed) {
	// Read config file

	char dbusinterface[1024] = "\0"; //< The dbus interface name
	char dbuspath[1024] = "\0"; //< The dbus path
	char dbusbus[1024] = "\0"; //< The dbus bus [system or session]
	char dbusmember[1024] = "\0"; //< The dbus member

	struct dbus_simvals *simval_list = NULL;

	if(NULL == seed || '\0' == *seed) {
		fprintf(stderr, "Must pass a filename to a config file as the seed\n");
		return 1;
	}
	FILE *configfile = fopen(seed, "r");
	if(NULL == configfile) {
		fprintf(stderr,"Couldn't open dbus config file %s\n", seed);
		return 1;
	}

	char line[1024]; // Single line of config file

	int map_from; // Value to map PIDs from
	unsigned int map_to; // Value to map PIDs to

	while(NULL != (fgets(line, sizeof(line), configfile))) {
		if('#' == *line) {
			// Comment
			continue;
		}

		if(0 != sscanf(line, "interface=%1023s", dbusinterface)) {
			printf("DBus Config, interface=%s\n", dbusinterface);
		} else if(0 != sscanf(line, "path=%1023s", dbuspath)) {
			printf("DBus Config, path=%s\n", dbuspath);
		} else if(0 != sscanf(line, "bus=%1023s", dbusbus)) {
			printf("DBus Config, bus=%s\n", dbusbus);
		} else if(0 != sscanf(line, "member=%1023s", dbusmember)) {
			printf("DBus Config, member=%s\n", dbusmember);
		} else if(2 == sscanf(line, "map %i -> %x", &map_from, &map_to)) {
			struct obdservicecmd *cmd = obdGetCmdForPID(map_to);

			printf("DBus Config, map %i -> 0x%02X (%s)\n", map_from, map_to, cmd->human_name);

			if(NULL == cmd) {
				fprintf(stderr, "DBus Config, Cannot find obdservice command for PID %02X\n", map_to);
				continue;
			}

			struct dbus_simvals *v = (struct dbus_simvals *)malloc(sizeof(struct dbus_simvals));
			v->map_from = map_from;
			v->pid = map_to;
			v->pid_cmd = cmd;
			v->most_recent = 0;
			v->next = NULL;
			if(NULL == simval_list) {
				simval_list = v;
			} else {
				v->next = simval_list;
				simval_list = v;
			}

			
		}
	}

	fclose(configfile);
	// Done reading configfile

	if(NULL == simval_list) {
		fprintf(stderr,"Couldn't find any mappings in DBus config file, cannot continue\n");
		return 1;
	}
	if(0 == strlen(dbusinterface)) {
		fprintf(stderr,"DBus config file must contain interface\n");
		return 1;
	}
	if(0 == strlen(dbuspath)) {
		fprintf(stderr,"DBus config file must contain path\n");
		return 1;
	}
	if(0 == strlen(dbusbus)) {
		fprintf(stderr,"DBus config file must contain bus [system or session]\n");
		return 1;
	}
	if(0 == strlen(dbusmember)) {
		fprintf(stderr,"DBus config file must contain member\n");
		return 1;
	}

	char simgen_match[4096];
	snprintf(simgen_match, sizeof(simgen_match),
		"type='signal',interface='%s',path='%s',member='%s'",
		dbusinterface, dbuspath, dbusmember);


	// Create the void *gen
	struct dbus_gen *g = (struct dbus_gen *)malloc(sizeof(struct dbus_gen));
	if(NULL == g) {
		fprintf(stderr,"Couldn't allocate memory for dbus generator\n");
		return 1;
	}

	g->supportedpids_00 = 0x01;
	g->supportedpids_20 = 0x01;
	g->supportedpids_40 = 0x01;
	g->supportedpids_60 = 0x00;

	// Iterate across our PIDs, figuring out which ones we support
	struct dbus_simvals *pidtest = simval_list;
	
	for(; pidtest!=NULL; pidtest=pidtest->next) {
		if(pidtest->pid <= 0x20) {
			g->supportedpids_00 |= ((unsigned long)1<<(0x20 - pidtest->pid));
		} else if(pidtest->pid > 0x20 && pidtest->pid <= 0x40) {
			g->supportedpids_20 |= ((unsigned long)1<<(0x40 - pidtest->pid));
		} else if(pidtest->pid > 0x40 && pidtest->pid <= 0x60) {
			g->supportedpids_40 |= ((unsigned long)1<<(0x60 - pidtest->pid));
		} else if(pidtest->pid > 0x60 &&  pidtest->pid <= 0x80) {
			g->supportedpids_60 |= ((unsigned long)1<<(0x80 - pidtest->pid));
		} else {
			fprintf(stderr,"Don't support PIDs this high in sim yet: %i\n", pidtest->pid);
		}
	}



	// Set up dbus stuff
	DBusConnection *dc;
	DBusError err;

	dbus_error_init(&err);
	// At the moment, we assume that if they didn't ask for the
	//   system queue, the want the session one. DBus support multiple
	//   queues, so this isn't guaraunteed, but it does work in 99.9%
	//   of cases.
	dc = dbus_bus_get(0==strcmp(dbusbus, "system")?DBUS_BUS_SYSTEM:DBUS_BUS_SESSION, &err);
	if (NULL == dc) {
		fprintf(stderr, "Error getting dbus %s bus: %s\n", dbusbus, err.message);
		return 1;
	}

	dbus_bus_add_match (dc, simgen_match, &err);
	dbus_connection_add_filter (dc, dbus_simgen_msgfilter, g, NULL);

	dbus_connection_flush(dc);
	// Done setting up dbus


	g->dbusconn = dc;
	g->simval_list = simval_list;

	*gen = g;
	return 0;
}

void dbus_simgen_destroy(void *gen) {
	free(gen);
}

int dbus_simgen_getvalue(void *gen, unsigned int mode, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {

	struct dbus_gen *g = gen;

	dbus_simgen_flushqueue(g, 1);

	if(0x00 == PID || 0x20 == PID || 0x40 == PID || 0x60 == PID) {
		unsigned long bits;
		if(0x00 == PID) bits = g->supportedpids_00;
		else if(0x20 == PID) bits = g->supportedpids_20;
		else if(0x40 == PID) bits = g->supportedpids_40;
		else if(0x60 == PID) bits = g->supportedpids_60;
		else return 0;

		*D = bits & 0xFF;
		bits >>= 8;
		*C = bits & 0xFF;
		bits >>= 8;
		*B = bits & 0xFF;
		bits >>= 8;
		*A = bits & 0xFF;

		return 4;
	}

	struct dbus_simvals *v = dbus_simgen_findsimval_to(gen, PID);
	if(NULL == v) return 0;

	return v->pid_cmd->convrev(v->most_recent, A, B, C, D);
}

int dbus_simgen_idle(void *gen, int idlems) {
	struct dbus_gen *g = gen;

	dbus_simgen_flushqueue(g, idlems);
	return 0;
}

DBusHandlerResult dbus_simgen_msgfilter(DBusConnection *connection,
		DBusMessage *message, void *gen) {

	// printf("In Message Filter\n");

	DBusMessageIter args;
	dbus_message_iter_init_append(message, &args);
	
	if (!dbus_message_iter_init(message, &args))
		fprintf(stderr, "Message has no arguments!\n"); 

	int key;  // We accept message tuples, an integer key
	unsigned int ukey; // Or unsigned [which we convert]
	double value_f; // And a floating point number (ints are converted)

	
	switch(dbus_message_iter_get_arg_type(&args)) {
		case DBUS_TYPE_INT32:
			dbus_message_iter_get_basic(&args, &key);
			break;
		case DBUS_TYPE_UINT32:
			dbus_message_iter_get_basic(&args, &ukey);
			key = (int)ukey;
			break;
		default:
			fprintf(stderr,"First argument isn't of type int\n");
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	if(dbus_message_iter_next(&args)) {
		switch(dbus_message_iter_get_arg_type(&args)) {
			case DBUS_TYPE_INT32: {
				int value_i;
				dbus_message_iter_get_basic(&args, &value_i);
				value_f = value_i; // Set it in our floating point number
				break;
			}
			case DBUS_TYPE_UINT32: {
				unsigned int value_ui;
				dbus_message_iter_get_basic(&args, &value_ui);
				value_f = value_ui; // Set it in our floating point number
				break;
			}
			case DBUS_TYPE_DOUBLE:
				dbus_message_iter_get_basic(&args, &value_f);
				break;
			default:
				fprintf(stderr, "Second value in dbus message tuple must be number\n");
				return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
				break;
		}
	} else {
		fprintf(stderr, "Must have two values in dbus message\n");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	// printf("DBus message contained everything we need: %i -> %f\n", key, value_f);

	struct dbus_simvals *v = dbus_simgen_findsimval_from(gen, key);
	if(NULL == v) {
		fprintf(stderr, "DBus Message tuple with an unconfigured key (%i)\n", key);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	v->most_recent = value_f;

	return DBUS_HANDLER_RESULT_HANDLED;
}

struct dbus_simvals *dbus_simgen_findsimval_from(void *gen, int from) {
	struct dbus_gen *g = (struct dbus_gen *)gen;
	if(NULL == g) return NULL;

	struct dbus_simvals *s = g->simval_list;

	for(; s!=NULL; s=s->next) {
		if(s->map_from == from) {
			return s;
		}
	}
	return NULL;
}

struct dbus_simvals *dbus_simgen_findsimval_to(void *gen, int to) {
	struct dbus_gen *g = (struct dbus_gen *)gen;
	if(NULL == g) return NULL;

	struct dbus_simvals *s = g->simval_list;

	for(; s!=NULL; s=s->next) {
		if(s->pid == to) {
			return s;
		}
	}
	return NULL;
}

void dbus_simgen_flushqueue(struct dbus_gen *gen, int timems) {
	// dbus_connection_read_write(gen->dbusconn, 0);
	dbus_connection_read_write_dispatch (gen->dbusconn, timems);
}

// Declare our obdsim_generator. This is pulled in as an extern in obdsim.c
struct obdsim_generator obdsimgen_dbus = {
	dbus_simgen_name,
	dbus_simgen_longdesc,
	dbus_simgen_create,
	dbus_simgen_destroy,
	dbus_simgen_getvalue,
	dbus_simgen_idle,
	NULL,
	NULL
};

