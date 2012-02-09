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

#ifndef __BLUETOOTHSIMPORT_H
#define __BLUETOOTHSIMPORT_H

#ifdef HAVE_BLUETOOTH

#include <sys/socket.h>
#include <termios.h>
#include <sys/types.h>
#include <fcntl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "simport.h"

/// Base class for virtual ports
class BluetoothSimPort : public OBDSimPort {
public:
	/// Constructor
	BluetoothSimPort();

	/// Destructor
	virtual ~BluetoothSimPort();

	/// Get a string representing the port as it's exposed
	/** Take a copy if you care - the memory won't stay valid */
	virtual char *getPort();

	/// Read a line from the virtual port
	/** Take a copy if you care - the memory won't stay valid */
	virtual char *readLine();

	/// Write some data to the virtual port
	virtual void writeData(const char *data, int log=1);

private:
	/// Wait for a bluetooth connection
	int waitConnection();

	/// Two locations
	struct sockaddr_rc loc_addr, rem_addr;

	/// The actual socket
	int s;

	/// The connected client
	int fd;

	/// Set when we're actually connected
	int connected;

	/// Last line read [returned by readLine]
	char lastread[4096];

	/// Current char buf [while reading]
	char readbuf[4096];

	/// String returned by getPort
	char portname[4096];

	/// Current position in the read buffer
	int readbuf_pos;
};

#endif //  HAVE_BLUETOOTH

#endif //__BLUETOOTHSIMPORT_H

