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

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "windowssimport.h"
#include "simport.h"

WindowsSimPort::WindowsSimPort(const char *port) {
	portname = NULL;

	if(NULL == port || 0 == strlen(port)) {
		return;
	}

	portname = strdup(port);

	char fullportname[512];
	snprintf(fullportname, sizeof(fullportname), "\\\\.\\%s", portname);
	portHandle=CreateFileA(fullportname, GENERIC_READ|GENERIC_WRITE,0, NULL, OPEN_EXISTING, 0, NULL);
	if (portHandle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Invalid handle returned by CreateFileA [port \"%s\"]\n", fullportname);
		return;
	}
	COMMCONFIG Win_CommConfig;
	COMMTIMEOUTS Win_CommTimeouts;
	unsigned long confSize = sizeof(COMMCONFIG);
	Win_CommConfig.dwSize = confSize;
	GetCommConfig(portHandle, &Win_CommConfig, &confSize);
	Win_CommConfig.dcb.Parity = 0;
	Win_CommConfig.dcb.fRtsControl = RTS_CONTROL_DISABLE;
	Win_CommConfig.dcb.fOutxCtsFlow = FALSE;
	Win_CommConfig.dcb.fOutxDsrFlow = FALSE;
	Win_CommConfig.dcb.fDtrControl = DTR_CONTROL_DISABLE;
	Win_CommConfig.dcb.fDsrSensitivity = FALSE;
	Win_CommConfig.dcb.fNull=FALSE;
	Win_CommConfig.dcb.fTXContinueOnXoff = FALSE;
	Win_CommConfig.dcb.fInX=FALSE;
	Win_CommConfig.dcb.fOutX=FALSE;
	Win_CommConfig.dcb.fBinary=TRUE;
	Win_CommConfig.dcb.DCBlength = sizeof(DCB);
	Win_CommConfig.dcb.BaudRate = 9600;
	Win_CommConfig.dcb.ByteSize = 8;
	Win_CommTimeouts.ReadIntervalTimeout = 50;
	Win_CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	Win_CommTimeouts.ReadTotalTimeoutConstant = 110;
	Win_CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	Win_CommTimeouts.WriteTotalTimeoutConstant = 110;
	SetCommConfig(portHandle, &Win_CommConfig, sizeof(COMMCONFIG));
	SetCommTimeouts(portHandle,&Win_CommTimeouts);


	setUsable(1);
}

WindowsSimPort::~WindowsSimPort() {
	delete portname;
	if(isUsable()) {
		CloseHandle(portHandle);
	}
}

char *WindowsSimPort::getPort() {
	return portname;
}

char *WindowsSimPort::readLine() {
	int nbytes; // Number of bytes read
	char *currpos = readbuf + readbuf_pos;

	ReadFile(portHandle, (void*)currpos, sizeof(readbuf) - readbuf_pos, (LPDWORD)&nbytes, NULL);

	if(0 < nbytes) {
		writeLog(currpos);

		if(getEcho()) {
			writeData(currpos,0);
		}

		// printf("Read %i bytes. strn is now '%s'\n", nbytes, readbuf);
		readbuf_pos += nbytes;
		char *lineend = strstr(readbuf, "\r");
		if(NULL == lineend) { // Just in case
			char *lineend = strstr(readbuf, "\n");
		}

		if(NULL != lineend) {
			int length = lineend - readbuf;
			strncpy(lastread, readbuf, length);
			lastread[length]='\0';

			while(*lineend == '\r' || *lineend == '\n') {
				lineend++;
			}
			memmove(readbuf, lineend, sizeof(readbuf) - (lineend - readbuf));
			readbuf_pos -= (lineend - readbuf);

			return lastread;
		}
	}
	return NULL;
}

void WindowsSimPort::writeData(const char *line, int log) {
	int len;
	if(log) writeLog(line);
	WriteFile(portHandle,line,(DWORD)strlen(line),(LPDWORD)&len,NULL);
}

#ifndef HAVE_GETTIMEOFDAY
int gettimeofday(struct timeval *tv, struct timezone *tz) {
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag = 0;

	if (NULL != tv) {
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		tmpres /= 10;  /*convert into microseconds*/
		/*converting file time to unix epoch*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS; 
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz) {
		if (!tzflag) {
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}
#endif //HAVE_GETTIMEOFDAY


