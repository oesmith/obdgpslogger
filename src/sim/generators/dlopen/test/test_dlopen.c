/*
 In the intention of being able to work with closed-source plugins,
 this file is explicitly NOT under the GPL. I [Gary "ChunkyKs" Briggs]
 disclaim copyright on this source code, and in the spirit of SQLite
 instead place a blessing here:
 
     May you do good and not evil.
     May you find forgiveness for yourself and forgive others.
     May you share freely, never taking more than you give.
 */

/** \file
 \brief Test file for dlopen generator
*/

/* This just implements the functions declared in simdl_datasource.h
 */

#include <stdio.h>

#include "simdl_datasource.h"

const char *simdl_name() {
	return "obdsim dlopen test";
}

int simdl_create(void **gen, const char *seed) {
	printf("In simdl_create, seed=\"%s\"\n", NULL==seed?"NULL":seed);
	return 0;
}

void simdl_destroy(void *gen) {
	printf("In simdl_destroy\n");
}

int simdl_getvalue(void *gen, unsigned int mode, unsigned int PID,
		unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {

	printf("In simdl_getvalue. Requested %02X %02X\n", mode, PID);
	if(0x00 == PID) {
		*A = 0xFF;
		*B = 0xFF;
		*C = 0xFF;
		*D = 0xFE;
	} else {
		*A = 0x11;
		*B = 0x22;
		*C = 0x33;
		*D = 0x44;
	}
	return 4;

}


