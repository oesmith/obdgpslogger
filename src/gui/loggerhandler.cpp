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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "maindisplay.h"
#include "loggerhandler.h"

loggerhandler::loggerhandler(OBDUI *mainui) {
	mMainui = mainui;
	mUsable = false;
	mStarted = false;

	if(NULL == mMainui) return;

	if(pipe(mStdOutPipe) < 0) {
		perror("stdout pipe error");
		return;
	}

	if(pipe(mStdErrPipe) < 0) {
		perror("sterr pipe error");
		return;
	}

	if(0 > (mChildPID = fork())) {
		perror("fork error");
		// Close the pipe since we can't use it
		close(mStdOutPipe[0]);
		close(mStdOutPipe[1]);
		close(mStdErrPipe[0]);
		close(mStdErrPipe[1]);
		return;
	}

	if(0 == mChildPID) { // In child
		close(mStdOutPipe[0]); // Close "read" end of pipe
		close(mStdErrPipe[0]); // Close "read" end of pipe
		// child can now write to mStd{Out,Err}Pipe[1] to send stuff to parent

		dup2(mStdOutPipe[1], STDOUT_FILENO); // hook stdout to the pipe
		dup2(mStdErrPipe[1], STDERR_FILENO); // hook stderr to the pipe
		close(mStdOutPipe[1]); // Close the dup'd fd
		close(mStdErrPipe[1]); // Close the dup'd fd
		
		const char *serialfilename = mMainui->getSerialfilename();
		const char *logfilename = mMainui->getLogfilename();

		int ret = execlp("obdgpslogger",
			"obdgpslogger",
			"--spam-stdout", // Spam all values to stdout

			"--db", // write to...
			logfilename, // this logfile

			"--serial", // connect to...
			serialfilename, // this serial port

			"--samplerate", // Sample...
			"10",               // 10 times a second

			NULL // Sentinel
			);

		// Removed --no-autotrip
		//
		// "--no-autotrip", // Don't start and stop trips automatically

		perror("execlp failed");
		exit(1);


	} else { // In parent
		close(mStdOutPipe[1]); // Close "write" end of pipe
		close(mStdErrPipe[1]); // Close "write" end of pipe
		// parent can now read from mStd{Out,Err}Pipe[0]
		//   to get std{out,err} from child

		memset(mLinebuf, '\0', sizeof(mLinebuf));
		mCurrentBufpos = mLinebuf;
	}

	mUsable = true;

}

loggerhandler::~loggerhandler() {
	if(!mUsable) return;

	// Only the parent will do this stuff
	close(mStdOutPipe[0]);
	close(mStdErrPipe[0]);

	if(0 > kill(mChildPID, SIGINT)) {
		perror("Couldn't KILL -INT child");
		return;
	}

	if(mChildPID != waitpid(mChildPID, NULL, 0)) {
		perror("waitpid unexpected value");
		return;
	}
}

void loggerhandler::checkRunning(bool block) {
	if(0 < waitpid(mChildPID, NULL, block?0:WNOHANG)) {
		close(mStdOutPipe[0]);
		mUsable = false;
	}
}

void loggerhandler::updateUI(const char *line) {
	float val_1f; // Value for single floats
	float val_3f[3]; // Value for three floats

	if(0 < sscanf(line, "vss=%f", &val_1f)) {
		mMainui->setvss(val_1f);
		mStarted = true;
	}

	if(0 < sscanf(line, "rpm=%f", &val_1f)) {
		mMainui->setrpm((float)val_1f);
		mStarted = true;
	}

	if(0 < sscanf(line, "maf=%f", &val_1f)) {
		mMainui->setmaf(val_1f);
		mStarted = true;
	}

	if(0 < sscanf(line, "throttlepos=%f", &val_1f)) {
		mMainui->setthrottlepos(val_1f);
		mStarted = true;
	}

	if(0 < sscanf(line, "temp=%f", &val_1f)) {
		mMainui->settemp(val_1f);
		mStarted = true;
	}

	if(2 < sscanf(line, "gpspos=%f,%f,%f", &val_3f[0], &val_3f[1], &val_3f[2])) {
		mMainui->setgps(val_3f[0], val_3f[1], val_3f[2]);
		mStarted = true;
	}
}

void loggerhandler::pulse() {
	if(!mUsable) return;

	fd_set mask;
	timeval timeout;

	// Check stderr
	FD_ZERO( &mask );
	FD_SET( mStdErrPipe[0], &mask );
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	if(select( mStdErrPipe[0]+1, &mask, NULL, NULL, &timeout ) > 0) {
		char errbuf[1024];
		size_t readlen = read(mStdErrPipe[0], errbuf, sizeof(errbuf));

		if(0 < readlen) {
			errbuf[readlen] = '\0';
			// printf("%s", errbuf);
			mMainui->append_stderr_log(errbuf);
		}
	}

	// Check stdout
	FD_ZERO( &mask );
	FD_SET( mStdOutPipe[0], &mask );
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	if(select( mStdOutPipe[0]+1, &mask, NULL, NULL, &timeout ) > 0) {
		size_t readlen = read(mStdOutPipe[0], mCurrentBufpos, sizeof(mLinebuf) - (mCurrentBufpos - mLinebuf));

		if(0 < readlen) {
			mCurrentBufpos[readlen] = '\0';
			mMainui->append_stdout_log(mCurrentBufpos);
			mCurrentBufpos += readlen;
		}
	}

	bool done = false;
	while(!done) {
		char line[sizeof(mLinebuf)];
		size_t linelen = strcspn(mLinebuf, "\r\n\0"); // Look for a newline

		if(0 < linelen && mLinebuf[linelen] != '\0') {
			// This one's the one we parse later
			strncpy(line, mLinebuf, linelen);
			line[linelen] = '\0';


			// Copy the rest of the string back to the start of the buffer
			char tmp[sizeof(mLinebuf)];
			strncpy(tmp, mLinebuf+linelen+1, sizeof(mLinebuf)-linelen);
		
			memset(mLinebuf, '\0', sizeof(mLinebuf));
			strncpy(mLinebuf, tmp, sizeof(mLinebuf));
			mCurrentBufpos = mLinebuf + strlen(mLinebuf);

			// printf("Got a line: %s\n", line);
			// printf("New Buffer: %s\n", mLinebuf);
			//
			checkRunning(false);
			if(!mUsable) return;

			updateUI(line);

		} else {
			done = true;
		}

	}
}

void loggerhandler::starttrip() {
	if(!mUsable) return;

	if(0 > kill(mChildPID, SIGUSR1)) {
		perror("Couldn't send signal USR1 to child");
	}
}

void loggerhandler::endtrip() {
	if(!mUsable) return;

	if(0 > kill(mChildPID, SIGUSR2)) {
		perror("Couldn't send signal USR2 to child");
	}
}

