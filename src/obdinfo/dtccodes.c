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

#include <string.h>
#include <stdio.h>
#include "dtccodes.h"

int dtc_isvalid(const char *test) {
	if(5 != strlen(test)) {
		return 0;
	}

	if(! ('P' == test[0] ||
		'C' == test[0] ||
		'B' == test[0] ||
		'U' == test[0]) ) {
	
		return 0;
	}

	int i;
	for(i=1;i<=4;i++) {
		if((test[i]<'0' || test[i]>'9') &&
			(test[i]<'A' || test[i]>'F')) {

			return 0;
		}
	}

	return 1;
}

int dtc_humantobytes(const char *human, unsigned int *A, unsigned int *B) {
	if(!dtc_isvalid(human)) {
		return -1;
	}

	int mod = human[1] - '0';
	if(mod > 3 || mod < 0) {
		return -1;
	}

	unsigned int val; // Will be shifted left for putting in A
	switch(human[0]) {
		case 'P':
			val = mod;
			break;
		case 'C':
			val = mod + 0x04;
			break;
		case 'B':
			val = mod + 0x08;
			break;
		case 'U':
			val = mod + 0x0C;
			break;
		default:
			return -1;
	}

	unsigned int one,two;
	if(2 != sscanf(human+2, "%1X%02X", &one, &two)) {
		return -1;
	}

	*A = one+(val<<4);
	*B = two;

	// printf("%s => %02X %02X\n", human, *A, *B);

	return 0;
}

