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

#ifndef __SIMPORT_H
#define __SIMPORT_H

/// Base class for virtual ports
class OBDSimPort {
public:
	/// Destructor
	virtual ~OBDSimPort();

	/// Get a string representing the port as it's exposed
	/** Take a copy if you care - the memory won't stay valid */
	virtual char *getPort() = 0;

	/// Enable or disable echo
	void setEcho(int yes);

	/// Find out if echo is set
	int getEcho();

	/// Enable or disable logging
	virtual int startLog(const char *filename);

	/// Enable or disable logging
	virtual void endLog();

	/// Write some data to the port
	/** If log is set to zero, the data isn't copied to log */
	virtual void writeData(const char *data, int log=1) = 0;

	/// Write some data to the logfile
	virtual void writeLog(const char *data);

	/// Find out if this initialised correctly
	int isUsable();

	/// Read a line from the virtual port
	/** Take a copy if you care - the memory won't stay valid */
	virtual char *readLine() = 0;

protected:
	/// Set usable
	void setUsable(int yes);

	/// Constructor. Can't construct these [base class]
	OBDSimPort();

	/// Whether or not to echo stuff
	int mEcho;

	/// log file
	FILE *mLogFile;
	
	/// Set when this is usable
	int mUsable;
	
};

#endif //__SIMPORT_H

