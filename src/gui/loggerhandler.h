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
   \brief class to launch and handle obdgpslogger
*/

#ifndef __LOGGERHANDLER_H
#define __LOGGERHANDLER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

class OBDUI;

/// Class to launch obdgpslogger and use the data from it
/** Drinks excessively from the Stevens kool-aid */
class loggerhandler {
public:
	/// Constructor
	/** This directly queries the UI window for parameters it needs
	   \param mainui the mainUI window already launched
	*/
	loggerhandler(OBDUI *mainui);

	/// Destructor
	~loggerhandler();

	/// Find out if this is usable
	bool isUsable() { return mUsable; }

	/// Find out if this is actually running
	bool isStarted() { return mStarted; }

	/// Check if the child is still running
	/** \param block if true, use a wait() that blocks
	 */
	void checkRunning(bool block);

	/// Check for new input. Do something useful with it
	void pulse();

	/// Tell obdgpslogger to start a trip
	void starttrip();

	/// Tell obdgpslogger to end a trip
	void endtrip();

protected:
	/// Check the line, update the UI if we find something good
	void updateUI(const char *line);

	/// Handle to the main ui window
	OBDUI *mMainui;

	/// Set if this object is usable
	bool mUsable;

	/// Set after everything is actually started
	bool mStarted;

	/// The pipe into which obdgpslogger writes stuff [stdout]
	int mStdOutPipe[2];

	/// The pipe into which obdgpslogger writes stuff [stderr]
	int mStdErrPipe[2];

	/// The PID of the obdgpslogger process
	pid_t mChildPID;

	/// We line-buffer the reads from the child
	char mLinebuf[4096];

	/// The current position in the line buffer
	char *mCurrentBufpos;
};


#endif // __LOGGERHANDLER_H

