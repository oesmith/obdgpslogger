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
 \brief Generate error codes
*/

#include <stdlib.h>
#include <stdio.h>

#include "datasource.h"

const char *error_simgen_name() {
	return "Error";
}

const char *error_simgen_longdesc() {
	return "Generate MIL errors\n"
		"Seed: Ignored";
}

int error_simgen_create(void **gen, const char *seed) {
	return 0;
}

void error_simgen_destroy(void *gen) {
}

int error_simgen_getvalue(void *gen, unsigned int mode, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {
	if(0x00 == PID) {
		*A = 0x80; // 0101 => "Tell me about errors"
		*B = 0x00;
		*C = 0x00;
		*D = 0x00;
		return 4;
	}

	if(0x01 == PID) {
		*A = 0x83; // 0x80 [mil set] bitwise OR 0x03 [three errors]
		*B = 0x00;
		*C = 0x00;
		*D = 0x00;
		return 4;
	}
	return 0;
}

int error_simgen_geterrorcodes(void *gen, unsigned int *errorcodes, int num_codes, int *mil) {
	int ret_codes = 3;
	switch(num_codes) {
		default:
			if(num_codes >= 3) {
				errorcodes[4] = 0x41; // C0123
				errorcodes[5] = 0x23;
			}
		case 2:
			if(num_codes >= 2) {
				errorcodes[2] = 0x80; // B0003
				errorcodes[3] = 0x03;
			}
		case 1:
			if(num_codes >= 1) {
				errorcodes[0] = 0x01; // P0104
				errorcodes[1] = 0x04;
			}
	}
	*mil=1;
	return ret_codes;
}

int error_simgen_clearerrorcodes(void *gen) {
	return 0;
}

// Declare our obdsim_generator. This is pulled in as an extern in obdsim.c
struct obdsim_generator obdsimgen_error = {
	error_simgen_name,
	error_simgen_longdesc,
	error_simgen_create,
	error_simgen_destroy,
	error_simgen_getvalue,
	NULL,
	error_simgen_geterrorcodes,
	error_simgen_clearerrorcodes
};

