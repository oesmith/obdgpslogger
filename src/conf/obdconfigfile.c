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
 \brief Tool-wide configuration
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "obdconfig.h"
#include "obdservicecommands.h"
#include "obdconfigfile.h"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif //MAX_PATH

/// \name configuration lines
///@{
#define OBDCONF_OBDDEVICE "obddevice"
#define OBDCONF_GPSDEVICE "gpsdevice"
#define OBDCONF_OPTIMISATIONS "optimisations"
#define OBDCONF_LOGCOLUMNS "log_columns"
#define OBDCONF_LOGFILE "log_file"
#define OBDCONF_SAMPLERATE "samplerate"
#define OBDCONF_BAUDRATE "baudrate"
#define OBDCONF_BAUDRATEUPGRADE "baudrate_upgrade"
///@}

/// Get "a" valid home dir in which to store a dotfile
static const char *getPlatformHomeDir() {
	static char homedir[MAX_PATH] = "\0";

	char *envhome;
	if(0 != strlen(homedir)) {
		return homedir;
	} else {
		// On OSX, might want to suffix "Application Data" ?
		envhome = getenv("HOME");
		if(NULL != envhome && *envhome) {
			snprintf(homedir,sizeof(homedir), "%s/", envhome);
			return homedir;
		}

		// Smells like windows
		envhome = getenv("APPDATA");
		if(NULL != envhome && *envhome) {
			snprintf(homedir,sizeof(homedir), "%s", envhome);
			return homedir;
		}

		snprintf(homedir,sizeof(homedir), ".");
	}
	
	return homedir;
}

static int obd_parseConfig(FILE *f, struct OBDGPSConfig *c, int verbose) {
	char line[1024];
	while(NULL != fgets(line,sizeof(line),f)) {
		// If you change the 1024 here, be damn sure to change it in the sscanf too
		char singleval_s[1024];
		int singleval_i;
		long singleval_l;

		char *firstnonspace = line;
		while(*firstnonspace != '\0' &&
			(' ' == *firstnonspace || '\t' == *firstnonspace)) firstnonspace++;
		if('#' == *firstnonspace) {
			if(verbose) printf("Conf found comment: %s\n", line);
			continue;
		}

		if(1 == sscanf(line, OBDCONF_OBDDEVICE "=%1023s", singleval_s)) {
			if(NULL != c->obd_device) {
				free((void *)c->obd_device);
			}
			c->obd_device = strdup(singleval_s);
			if(verbose) printf("Conf Found OBD Device: %s\n", singleval_s);
		}
		if(1 == sscanf(line, OBDCONF_GPSDEVICE "=%1023s", singleval_s)) {
			if(NULL != c->gps_device) {
				free((void *)c->gps_device);
			}
			c->gps_device = strdup(singleval_s);
			if(verbose) printf("Conf Found GPS Device: %s\n", singleval_s);
		}
		if(1 == sscanf(line, OBDCONF_LOGFILE "=%1023s", singleval_s)) {
			if(NULL != c->log_file) {
				free((void *)c->log_file);
			}
			c->log_file = strdup(singleval_s);
			if(verbose) printf("Conf Found log_file: %s\n", singleval_s);
		}
		if(1 == sscanf(line, OBDCONF_LOGCOLUMNS "=%1023s", singleval_s)) {
			if(NULL != c->log_columns) {
				free((void *)c->log_columns);
			}
			c->log_columns = strdup(singleval_s);
			if(verbose) printf("Conf Found log_columns: %s\n", singleval_s);
		}
		if(1 == sscanf(line, OBDCONF_BAUDRATE "=%li", &singleval_l)) {
			c->baudrate = singleval_l;
			if(verbose) printf("Conf Found baudrate: %li\n", singleval_l);
		}
		if(1 == sscanf(line, OBDCONF_BAUDRATEUPGRADE "=%li", &singleval_l)) {
			c->baudrate_upgrade = singleval_l;
			if(verbose) printf("Conf Found baudrate upgrade: %li\n", singleval_l);
		}
		if(1 == sscanf(line, OBDCONF_SAMPLERATE "=%i", &singleval_i)) {
			c->samplerate = singleval_i;
			if(verbose) printf("Conf Found samplerate: %i\n", singleval_i);
		}
		if(1 == sscanf(line, OBDCONF_OPTIMISATIONS "=%i", &singleval_i)) {
			c->optimisations = singleval_i;
			if(verbose) printf("Conf Found optimisations: %i\n", singleval_i);
		}
	}
	return 0;
}

struct OBDGPSConfig *obd_loadConfig(int verbose) {
	struct OBDGPSConfig *c = (struct OBDGPSConfig *)malloc(sizeof(struct OBDGPSConfig));
	if(NULL == c) return NULL;
	c->obd_device = strdup(OBD_DEFAULT_SERIALPORT);
	c->gps_device = strdup(OBD_DEFAULT_GPSPORT);
	c->log_columns = strdup(OBD_DEFAULT_COLUMNS);
	c->log_file = strdup(OBD_DEFAULT_DATABASE);
	c->samplerate = 1;
	c->optimisations = 0;
	c->baudrate = -1;
	c->baudrate_upgrade = -1;

	char fullfilename[MAX_PATH];

	FILE *f;

	// Each parse overwrites values set previously

	// Parse the system-wide config first

	if(verbose) {
		printf("Attempting to read /etc/obdgpslogger .. ");
	}
	f = fopen("/etc/obdgpslogger", "r");
	if(NULL != f) {
		if(verbose) {
			printf("Opened. Parsing\n");
		}
		obd_parseConfig(f,c,verbose);
		fclose(f);
	} else {
		if(verbose) {
			printf("Couldn't open\n");
		}
	}

	// Parse the specific user's config file second

	// For portableness back to older windows, '/' separator won't work
	snprintf(fullfilename, sizeof(fullfilename), "%s/%s",
			getPlatformHomeDir(), OBD_CONFIG_FILENAME
			);

	if(verbose) {
		printf("Attempting to read %s .. ", fullfilename);
	}
	f = fopen(fullfilename, "r");
	if(NULL != f) {
		if(verbose) {
			printf("Opened. Parsing\n");
		}
		obd_parseConfig(f,c,verbose);
		fclose(f);
	} else {
		if(verbose) {
			printf("Couldn't open\n");
		}
	}

	// Parse the obdftdipty entry

	if(verbose) {
		printf("Attempting to read %s .. ", OBD_FTDIPTY_DEVICE);
	}
	f = fopen(OBD_FTDIPTY_DEVICE, "r");
	if(NULL != f) {
		if(verbose) {
			printf("Opened. Parsing\n");
		}
		obd_parseConfig(f,c,verbose);
		fclose(f);
	} else {
		if(verbose) {
			printf("Couldn't open\n");
		}
	}

	// If the OBD_CONFIGFILE env var is set

	char *envfile = getenv("OBD_CONFIGFILE");
	if(verbose) {
		printf("OBD_CONFIGFILE env var %s: %s\n",
						NULL==envfile?"not found":"found",
						NULL==envfile?"":envfile);
	}
	if(NULL != envfile) {
		if(verbose) {
			printf("Attempting to read %s ..", envfile);
		}
		f = fopen(envfile, "r");
		if(NULL != f) {
			if(verbose) {
				printf("Opened. Parsing\n");
			}
			obd_parseConfig(f,c,verbose);
			fclose(f);
		} else {
			if(verbose) {
				printf("Couldn't open\n");
			}
		}
	}

	if(verbose) {
		printf("Full Config:\n"
					 "	" OBDCONF_OBDDEVICE ":%s\n"
					 "	" OBDCONF_GPSDEVICE ":%s\n"
					 "	" OBDCONF_LOGCOLUMNS ":%s\n"
					 "	" OBDCONF_OPTIMISATIONS ":%i\n"
					 "	" OBDCONF_SAMPLERATE ":%i\n"
					 "	" OBDCONF_BAUDRATE ":%li\n"
					 "	" OBDCONF_BAUDRATEUPGRADE ":%li\n"
					 "	" OBDCONF_LOGFILE ":%s\n",
					 	c->obd_device, c->gps_device, c->log_columns,
						c->optimisations, c->samplerate, c->baudrate,
						c->baudrate_upgrade, c->log_file);
	}
	return c;
}

int obd_writeConfig(struct OBDGPSConfig *c) {
	char fullfilename[MAX_PATH];

	snprintf(fullfilename, sizeof(fullfilename), "%s/%s",
			getPlatformHomeDir(), OBD_CONFIG_FILENAME
			);

	FILE *f = fopen(fullfilename, "w");
	if(NULL == f) {
		perror("Couldn't open config file for writing");
		return 1;
	}

	fprintf(f, OBDCONF_OBDDEVICE "=%s\n", c->obd_device);
	fprintf(f, OBDCONF_GPSDEVICE "=%s\n", c->gps_device);
	fprintf(f, OBDCONF_LOGCOLUMNS "=%s\n", c->log_columns);
	fprintf(f, OBDCONF_LOGFILE "=%s\n", c->log_file);
	fprintf(f, OBDCONF_OPTIMISATIONS "=%i\n", c->optimisations);
	fprintf(f, OBDCONF_SAMPLERATE "=%i\n", c->samplerate);
	fprintf(f, OBDCONF_BAUDRATE "=%li\n", c->baudrate);
	fprintf(f, OBDCONF_BAUDRATEUPGRADE "=%li\n", c->baudrate_upgrade);

	fclose(f);

	return 0;
}

/// Free a config created by loadOBDGPSConfig
void obd_freeConfig(struct OBDGPSConfig *c) {
	if(NULL == c) return;
	if(NULL != c->obd_device) free((void *)c->obd_device);
	if(NULL != c->gps_device) free((void *)c->gps_device);
	if(NULL != c->log_columns) free((void *)c->log_columns);
	if(NULL != c->log_file) free((void *)c->log_file);
	free(c);
}

int obd_configCmds(const char *log_columns, struct obdservicecmd ***cmds) {
	int cols = 0;
	*cmds = NULL;
	const char *toklist=",: "; // Seriously, we only want comma-separated though

	// Goind to do this twice. First time, get a count to allocate.
	char *cmdlist = strdup(log_columns);
	if(NULL == cmdlist) return -1;

	char *currcmd = strtok(cmdlist, toklist);
	while(currcmd) {
		struct obdservicecmd *c;

		unsigned int cmdpid;
		if(NULL != (c = obdGetCmdForColumn(currcmd))) {
			cols++;
		} else if(1 == sscanf(currcmd, "%2X", &cmdpid)) {
			if(NULL != (c = obdGetCmdForPID(cmdpid))) {
				cols++;
			} else {
				printf("Warning: Couldn't find column for PID '%s'. Possible config file problem\n", currcmd);
			}
		} else {
			printf("Warning: Couldn't find column '%s'. Possible config file problem\n", currcmd);
		}
		currcmd = strtok(NULL, toklist);
	}
	free((void *)cmdlist);

	// Second time, do the allocation then the assignement
	*cmds = (struct obdservicecmd **)malloc((cols+1) * sizeof(struct obdservicecmd *)); // 1 == NULL sentinel

	int currcol = 0;
	cmdlist = strdup(log_columns);
	if(NULL == cmdlist) return -1;

	currcmd = strtok(cmdlist, toklist);
	while(currcmd) {
		struct obdservicecmd *c;

		unsigned int cmdpid;
		if(NULL != (c = obdGetCmdForColumn(currcmd))) {
			(*cmds)[currcol++] = c;
		} else if(1 == sscanf(currcmd, "%2X", &cmdpid)) {
			if(NULL != (c = obdGetCmdForPID(cmdpid))) {
				(*cmds)[currcol++] = c;
			}
		}
		currcmd = strtok(NULL, toklist);
	}
	free((void *)cmdlist);

	(*cmds)[currcol] = NULL;

	return cols;
}

void obd_freeConfigCmds(struct obdservicecmd **cmds) {
	free((void *)cmds);
}

