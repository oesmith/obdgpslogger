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

#include <stdio.h>
#include <stdlib.h>
#include "simport.h"

OBDSimPort::OBDSimPort() {
	mUsable = false;
	mEcho = true;
	mLogFile = NULL;
}

OBDSimPort::~OBDSimPort() {
}

void OBDSimPort::setEcho(int yes) {
	mEcho = yes;
}

int OBDSimPort::getEcho() {
	return mEcho;
}

int OBDSimPort::startLog(const char *filename) {
	mLogFile = fopen(filename, "w");
	if(NULL == mLogFile) {
		perror("Couldn't open logfile\n");
		return 1;
	}
	return 0;
}

void OBDSimPort::endLog() {
	if(NULL != mLogFile) {
		fclose(mLogFile);
		mLogFile = NULL;
	}
}

void OBDSimPort::writeLog(const char *data) {
	if(NULL != mLogFile) {
		fputs(data, mLogFile);
		fflush(mLogFile);
	}
}

int OBDSimPort::isUsable() {
	return mUsable;
}

void OBDSimPort::setUsable(int yes) {
	mUsable = yes;
}


