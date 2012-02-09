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
 \brief OBD service commands
 */

#include "obdservicecommands.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct obdservicecmd *obdGetCmdForColumn(const char *db_column) {
	int i;
	int numrows = sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0]);
	for(i=0;i<numrows;i++) {
		struct obdservicecmd *o = &obdcmds_mode1[i];
		if(NULL == o->db_column) {
			continue;
		}

		if(0 == strcmp(db_column,o->db_column)) {
			return o;
		}
	}
	return NULL;
}

/// O(log(n)) PID finder
/** Profiling, it usually ends up slower than the linear one, which is
because we're normally in the lower end of the PID scale and the linear
one is much simpler */
static struct obdservicecmd *obdGetCmdForPID_recursive(const unsigned int pid,
	const int first_idx, const int last_idx) {

	// printf("%i %i\n", first_idx, last_idx);

	// const unsigned int first_pid = obdcmds_mode1[first_idx].cmdid;
	const unsigned int last_pid = obdcmds_mode1[last_idx].cmdid;

	// if(pid == first_pid) return &obdcmds_mode1[first_idx];
	if(pid == last_pid) return &obdcmds_mode1[last_idx];

	if(first_idx >= last_idx) {
		return NULL;
	}

	int middle_idx = (last_idx + first_idx)/2;
	const unsigned int middle_pid = obdcmds_mode1[middle_idx].cmdid;

	if(pid == middle_pid) return &obdcmds_mode1[middle_idx];

	if(middle_pid < pid) {
		return obdGetCmdForPID_recursive(pid, middle_idx+1, last_idx-1);
	} else {
		return obdGetCmdForPID_recursive(pid, first_idx, middle_idx-1);
	}
}

struct obdservicecmd *obdGetCmdForPID(const unsigned int pid) {
	int i;
	int numrows = sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0]);
	for(i=0;i<numrows;i++) {
		struct obdservicecmd *o = &obdcmds_mode1[i];

		if(pid == o->cmdid) {
			return o;
		}
	}
	return NULL;

	// return obdGetCmdForPID_recursive(pid, 0, sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0]));
}

int obderrconvert_r(char *buf, int n, unsigned int A, unsigned int B) {
	unsigned int partcode = (A>>4)&0x0F;
	unsigned int numbercode = 0;
	char strpartcode;
	switch(partcode) {
			// Powertrain codes
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
			strpartcode = 'P';
			numbercode = partcode;
			break;
			
			// Chassis codes
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			strpartcode = 'C';
			numbercode = partcode-4;
			break;
			
			// Body codes
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
			strpartcode = 'B';
			numbercode = partcode-8;
			break;
			
			// Network codes
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			strpartcode = 'U';
			numbercode = partcode-12;
			break;
			
		default:
			fprintf(stderr, "Could not decode error code.\n");
			return 0;
			break;
	}
	
	return snprintf(buf, n, "%c%u%01X%02X", strpartcode, numbercode, A&0x0F, B);
}

const char *obderrconvert(unsigned int A, unsigned int B) {
	static char strerr[6]; // Letter, four digits, \0
	strerr[0] = '\0';
	
	obderrconvert_r(strerr, sizeof(strerr), A, B);
	
	return strerr;
}


