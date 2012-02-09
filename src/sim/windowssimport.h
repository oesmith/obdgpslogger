/* Copyright 2009 Gary Briggs, Michael Carpenter

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
  \brief Tools to open the sim port
*/

#ifndef __WINDOWSSIMPORT_H
#define __WINDOWSSIMPORT_H

#include "simport.h"
#include <windows.h>
#include <time.h>

/// Base class for virtual ports
class WindowsSimPort : public OBDSimPort {
public:
	/// Constructor
	WindowsSimPort(const char *portname);

	/// Destructor
	virtual ~WindowsSimPort();

	/// Get a string representing the port as it's exposed
	/** Take a copy if you care - the memory won't stay valid */
	virtual char *getPort();

	/// Read a line from the virtual port
	/** Take a copy if you care - the memory won't stay valid */
	virtual char *readLine();

	/// Write some data to the virtual port
	virtual void writeData(const char *data, int log=1);

private:
	/// Handle onto the windows virtual port
	HANDLE portHandle;

	/// The portname for getPort();
	char *portname;

	/// Last line read [returned by readLine]
	char lastread[4096];

	/// Current char buf [while reading]
	char readbuf[4096];

	/// Current position in the read buffer
	int readbuf_pos;
};

#ifndef HAVE_GETTIMEOFDAY
// Provide a windows implementation if one doesn't exist


#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone {
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

#endif //HAVE_GETTIMEOFDAY

#endif //__WINDOWSSIMPORT_H

