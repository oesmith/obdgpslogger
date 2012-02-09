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
 \brief Test the obdconfig stuffs
 */
#include <stdio.h>
#include <stdlib.h>

#include "obdconfigfile.h"

int main() {
	struct OBDGPSConfig *c;
	if(NULL == (c = obd_loadConfig(1))) {
		printf("Error in loadConfig\n");
		exit(1);
	} else {
		printf("Successfully loaded config\n");
	}

	struct obdservicecmd **cmds;
	int cmds_found = obd_configCmds(c->log_columns, &cmds);
	printf("Found %i cmds:\n", cmds_found);
	int i;
	for(i=0;NULL != cmds[i];i++) {
		printf("   [%02X] %s\n", cmds[i]->cmdid, cmds[i]->human_name);
	}

	obd_freeConfigCmds(cmds);
	printf("Freed commands\n");

	obd_freeConfig(c);
	printf("Freed config\n");
	return 0;
}

