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

#include <FL/Fl.H>
#include "fftwindow.h"

int main(int argc, char **argv) {
	FFTOBD mainwindow;

	if(argc > 1) {
		if(NULL != strstr(argv[1], ".csv") || NULL != strstr(argv[1], ".txt")) {
			mainwindow.opencsv(argv[1]);
		} else if(NULL != strstr(argv[1], ".db")) {
			mainwindow.opendb(argv[1]);
		} else {
			fprintf(stderr, "First argument to %s must be csv,txt or db\n", argv[0]);
		}
	}

	// mainwindow.show(argc,argv);
	mainwindow.show();

	Fl::run();

	return 0;
}

