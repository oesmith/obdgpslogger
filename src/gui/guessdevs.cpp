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
 \brief Guess likely serial device names
 */

#include <stdio.h>
#include <string.h>
#include <glob.h>
#include <FL/Fl_Input_Choice.H>

#include "guessdevs.h"

int populateSerialDevs(Fl_Input_Choice *addto, const char *def) {
	glob_t names;

	addto->clear();
	if(NULL != def && 0 < strlen(def)) {
		addto->add(def);
		addto->value(0);
	}

	// Just take a bunch of semi-educated guesses:

	// This is the OSX standard way of doing things
	glob("/dev/cu.*", 0, NULL, &names);
	// Linux bluetooth
	glob("/dev/rfcomm*", GLOB_APPEND, NULL, &names);
	// Linux USB
	glob("/dev/ttyUSB*", GLOB_APPEND, NULL, &names);
	// Good ol' fashioned serial port
	glob("/dev/ttyS*", GLOB_APPEND, NULL, &names);

	size_t i;
	for(i=0; i<names.gl_pathc; i++) {
		addto->add(names.gl_pathv[i]);
	}
	
	globfree(&names);

	return 0;
}

