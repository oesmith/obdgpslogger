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

#include <unistd.h>
#include "maindisplay.h"
#include "loggerhandler.h"
#include "obdgui.h"

/// FLTK idle handler to pulse input
void obd_idle_cb(void *ui) {
	OBDUI *o = static_cast<OBDUI *>(ui);

	if(NULL != o) o->checkLogger();

	// Tiny sleep to avoid 100% cpu
	usleep(10000);
}

/// Main entrypoint
int main(int argc, char **argv) {
	OBDUI mainwindow;
	mainwindow.show(argc,argv);

	Fl::scheme("gtk+");

	Fl::add_idle(obd_idle_cb, static_cast<void *>(&mainwindow));

	Fl::run();

	return 0;
}

