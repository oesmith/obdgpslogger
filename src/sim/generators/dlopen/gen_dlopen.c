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
 \brief Generate dlopen data
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "simdl_datasource.h"
#include "datasource.h"

/// The void * generator for dlopen
struct dlopen_gen {
	void *handle; //< Returned by dlopen
	void *gen_gen; //< the void * from the dlopen'd generator

// The generator functions
 	const char *(*simdl_name)();
	int (*simdl_create)(void **gen, const char *seed); //< Create the generator
	void (*simdl_destroy)(void *gen); //< Destroy the generator
	int (*simdl_getvalue)(void *gen, unsigned int mode, unsigned int PID,
	        unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D);
	int (*simdl_idle)(void *gen, int timems);
	int (*simdl_geterrorcodes)(void *gen, unsigned int *errorcodes, int num_codes, int *mil);
	int (*simdl_clearerrorcodes)(void *gen);
};

const char *dlopen_simgen_name() {
	return "dlopen";
}

const char *dlopen_simgen_longdesc() {
	return "Dynamically open a shared library plugin\n"
		"Seed: <dynamic library filename[,seed for dlopen plugin]>";
}

int dlopen_simgen_create(void **gen, const char *seed) {
	if(NULL == seed || '\0' == *seed) {
		fprintf(stderr, "Must pass name of library as seed to dlopen generator\n");
		return 1;
	}

	char *seedcpy = strdup(seed);

	char *libname = seedcpy;

	char *subseed = libname;
	for(;'\0' != *subseed && ',' != *subseed; subseed++);
	if(',' == *subseed) {
		*subseed = '\0';
		subseed++;
	};

	void *handle = dlopen(libname, RTLD_NOW);
	if(NULL == handle) {
		fprintf(stderr, "Error dlopening %s: %s\n", libname, dlerror());
		return 1;
	}

	struct dlopen_gen *g = (struct dlopen_gen *)malloc(sizeof(struct dlopen_gen));
	if(NULL == g) {
		fprintf(stderr, "Couldn't allocate memory for dlopen'd generator\n");
		dlclose(handle);
		return 1;
	}

	g->handle = handle;
	dlerror(); // Clear any errors

	char *error;
	// Read the dlopen manpage if you wish to know why
	*(void **)(&g->simdl_name) = dlsym(handle, "simdl_name");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_name in library: %s\n", error);
		dlclose(handle);
		free(g);
		return 1;
	}

	*(void **)(&g->simdl_create) = dlsym(handle, "simdl_create");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_create in library: %s\n", error);
		dlclose(handle);
		free(g);
		return 1;
	}

	*(void **)(&g->simdl_destroy) = dlsym(handle, "simdl_destroy");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_destroy in library: %s\n", error);
		dlclose(handle);
		free(g);
		return 1;
	}

	*(void **)(&g->simdl_getvalue) = dlsym(handle, "simdl_getvalue");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_getvalue in library: %s\n", error);
		dlclose(handle);
		free(g);
		return 1;
	}

	*(void **)(&g->simdl_idle) = dlsym(handle, "simdl_idle");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_idle in library [Not Fatal]: %s\n", error);
	}

	*(void **)(&g->simdl_geterrorcodes) = dlsym(handle, "simdl_geterrorcodes");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_geterrorcodes in library [Not Fatal]: %s\n", error);
	}

	*(void **)(&g->simdl_clearerrorcodes) = dlsym(handle, "simdl_clearerrorcodes");
	if(NULL != (error = dlerror())) {
		fprintf(stderr, "Couldn't find symbol simdl_clearerrorcodes in library [Not Fatal]: %s\n", error);
	}

	printf("Successfully dlopen'd sim generator %s, %s\n", libname, g->simdl_name());

	if(1 == g->simdl_create(&(g->gen_gen), subseed)) {
		fprintf(stderr, "dlopene'd library reported error on creation\n");
		dlclose(handle);
		free(g);
		return 1;
	}

	free(seedcpy);

	*gen = g;
	return 0;
}

void dlopen_simgen_destroy(void *gen) {
	struct dlopen_gen *g = (struct dlopen_gen *)gen;
	g->simdl_destroy(g->gen_gen);
	dlclose(g->handle);
	free(g);
}

int dlopen_simgen_getvalue(void *gen, unsigned int mode, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {
	struct dlopen_gen *g = (struct dlopen_gen *)gen;
	return g->simdl_getvalue(g->gen_gen, mode, PID, A, B, C, D);
}

int dlopen_simgen_idle(void *gen, int idlems) {
	struct dlopen_gen *g = (struct dlopen_gen *)gen;
	if(NULL != g->simdl_idle) {
		return g->simdl_idle(g->gen_gen, idlems);
	}
	return 0;
}

int dlopen_simgen_geterrorcodes(void *gen, unsigned int *errorcodes, int num_codes, int *mil) {
	struct dlopen_gen *g = (struct dlopen_gen *)gen;
	if(NULL != g->simdl_geterrorcodes) {
		return g->simdl_geterrorcodes(g->gen_gen, errorcodes, num_codes, mil);
	}
	return 0;
}

int dlopen_simgen_clearerrorcodes(void *gen) {
	struct dlopen_gen *g = (struct dlopen_gen *)gen;
	if(NULL != g->simdl_clearerrorcodes) {
		return g->simdl_clearerrorcodes(g->gen_gen);
	}
	return 0;
}

// Declare our obdsim_generator. This is pulled in as an extern in obdsim.c
struct obdsim_generator obdsimgen_dlopen = {
	dlopen_simgen_name,
	dlopen_simgen_longdesc,
	dlopen_simgen_create,
	dlopen_simgen_destroy,
	dlopen_simgen_getvalue,
	dlopen_simgen_idle,
	dlopen_simgen_geterrorcodes,
	dlopen_simgen_clearerrorcodes
};

