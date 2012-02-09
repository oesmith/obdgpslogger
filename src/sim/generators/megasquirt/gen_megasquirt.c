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
 \brief Megasquirt proxy
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "datasource.h"
#include "obdservicecommands.h"

// http://www.diyautotune.com/megamanual/xFAQ/manuals/COM_RS232.htm

/// The void * generator for megasquirt
struct megasquirt_gen {
	int fd; //< Pointer to the open serial port
};

const char *megasquirt_simgen_name() {
	return "Megasquirt";
}

const char *megasquirt_simgen_longdesc() {
	return "Talk to a megasquirt device\n"
		"Seed: <path-to-config-file>";
}

int megasquirt_simgen_create(void **gen, const char *seed) {
	if(NULL == seed || '\0' == *seed) {
		fprintf(stderr, "Megasquirt generator needs the config file as a seed\n");
		return 1;
	}

	struct megasquirt_gen *g = (struct megasquirt_gen *)malloc(sizeof(struct megasquirt_gen));
	if(NULL == g) {
		fprintf(stderr, "Couldn't allocate memory for megasquirt generator\n");
		return 1;
	}

	return 0;
}

void megasquirt_simgen_destroy(void *gen) {
	free(gen);
}

int megasquirt_simgen_getvalue(void *gen, unsigned int mode, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {

	struct megasquirt_gen *g = (struct megasquirt_gen *)gen;

	return -1; // Shouldn't be able to get here
}

int megasquirt_simgen_idle(void *gen, int idlems) {
	return 0;
}

// Declare our obdsim_generator. This is pulled in as an extern in obdsim.c
struct obdsim_generator obdsimgen_megasquirt = {
	megasquirt_simgen_name,
	megasquirt_simgen_longdesc,
	megasquirt_simgen_create,
	megasquirt_simgen_destroy,
	megasquirt_simgen_getvalue,
	megasquirt_simgen_idle,
	NULL,
	NULL
};

