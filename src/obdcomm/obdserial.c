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
 \brief serial stuff
 */

// Code and ideas borrowed in places from
// http://easysw.com/~mike/serial/serial.html

#include "obdserial.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <termios.h>

/// What to use as the obd newline char in commands
#define OBDCMD_NEWLINE "\r"

/// Whether serial data is into pc, or out from pc
#define SERIAL_IN 0
#define SERIAL_OUT 1

/// Handle to the serial log
static FILE *seriallog = NULL;

/// Guess the baudrate
/** return -1 on error, or baudrate on success */
static long guessbaudrate(int fd);

/// Upgrade the baudrate
/** \param baudrate_target 0 to guess, -1 to disable, specified number for >0
  \return -1 on error, or baudrate on success */
static long upgradebaudrate(int fd, long baudrate_target, long current_baudrate);

/// Write to the log
static void appendseriallog(const char *line, int out) {
	if(NULL != seriallog) {
		char timestr[200];
		time_t t;
		struct tm *tmp;

		t = time(NULL);
		tmp = localtime(&t);
		if (tmp == NULL) {
			snprintf(timestr, sizeof(timestr), "Unknown time");
		}

		if (strftime(timestr, sizeof(timestr), "%H:%M:%S", tmp) == 0) {
			snprintf(timestr, sizeof(timestr), "Unknown time");
		}

		fprintf(seriallog, "%s(%s): '%s'\n", timestr, out==SERIAL_OUT?"out":"in", line);
		fflush(seriallog);
	}
}

/// Collect data up to the next prompt
/** Reads up to the next '>'
   \param buf buffer to fill
   \param n size of buf
   \return number of bytes put in buf, or -1 on error
*/
int readserialdata(int fd, char *buf, int n) {
	char *bufptr = buf; // current position in buf

	struct timeval start,curr; // For timing out
	if(0 != gettimeofday(&start, NULL)) {
		perror("Couldn't gettimeofday");
		return -1;
	}
	memset((void *)buf, '\0', n);
	int retval = 0; // Value to return
	int nbytes; // Number of bytes read
	do {
		nbytes = read(fd, bufptr, buf+n-bufptr-1);
		if(-1 == nbytes && EAGAIN != errno) {
			perror("Error in readserialdata");
		}
		if(-1 != nbytes) {
			// printf("Read bytes '%s'\n", bufptr);
			retval += nbytes; // Increment bytecount
			bufptr += nbytes; // Move pointer forward
		}
		if(0 != gettimeofday(&curr, NULL)) {
			perror("Couldn't gettimeofday");
			return -1;
		}
		if(OBDCOMM_TIMEOUT < 1000000l*(curr.tv_sec - start.tv_sec) +
			(curr.tv_usec - start.tv_usec)) {
			printf("Timeout!\n");
			return -1;
		}
	} while (retval == 0 || bufptr[-1] != '>');

	appendseriallog(buf, SERIAL_IN);
	return retval;
}

/// Throw away all data until the next prompt
void readtonextprompt(int fd) {
	char retbuf[4096]; // Buffer to store returned stuff
	readserialdata(fd, retbuf, sizeof(retbuf));
}

// Blindly send a command and throw away all data to next prompt
/**
 \param cmd command to send
 \param no_response if we don't read to next prompt [ie, on exit]
 \param fd file descriptor
 */
void blindcmd(int fd, const char *cmd, int no_response) {
	char outstr[1024];
	snprintf(outstr, sizeof(outstr), "%s%s\0", cmd, OBDCMD_NEWLINE);
	appendseriallog(outstr, SERIAL_OUT);
	write(fd,outstr, strlen(outstr));
	if(0 != no_response) {
		sleep(1);
		readtonextprompt(fd);
	}
}

int openserial(const char *portfilename, long baudrate, long baudrate_target) {
	struct termios options;
	int fd;

	fprintf(stderr,"Opening serial port %s, this can take a while\n", portfilename);
	fd = open(portfilename, O_RDWR | O_NOCTTY | O_NDELAY);
	// fd = open(portfilename, O_RDWR | O_NOCTTY);

	if(fd == -1) {
		perror(portfilename);
	} else {
		fcntl(fd, F_SETFL, 0);

		// Get the current options for the port
		tcgetattr(fd, &options);

		options.c_cflag |= (CLOCAL | CREAD);
		options.c_lflag &= !(ICANON | ECHO | ECHOE | ISIG);
		options.c_oflag &= !(OPOST);
		options.c_cc[VMIN] = 0;
		options.c_cc[VTIME] = 100;

		tcsetattr(fd, TCSANOW, &options);

		long current_baud = 9600;
		if(0 != modifybaud(fd, baudrate)) {
			fprintf(stderr, "Error modifying baudrate. Continuing, but may suffer issues\n");
		} else {
			if(baudrate != 0)
				current_baud = baudrate;
		}

		// Reset the device. Some software changes settings and then leaves it
		blindcmd(fd,"ATZ",1);

		// printf("Baudrate upgrader disabled\n");
		if(0 > upgradebaudrate(fd, baudrate_target, current_baud)) {
			fprintf(stderr, "Error upgrading baudrate. Continuing, but may suffer issues\n");
		}

		// Now some churn to get everything up and running.
		// Do a general cmd that all obd-devices support
		// Do this once in case we have a partially-written command somehow
		blindcmd(fd,"0100",1);
		// Disable command echo [elm327]
		blindcmd(fd,"ATE0",1);
		// Disable linefeeds [an extra byte of speed can't hurt]
		blindcmd(fd,"ATL0",1);
		// Don't insert spaces [readability is for ugly bags of mostly water]
		blindcmd(fd,"ATS0",1);
		// Then do it again to make sure the command really worked
		blindcmd(fd,"0100",1);

	}
	return fd;
}

void closeserial(int fd) {
	blindcmd(fd,"ATZ",0);
	close(fd);
}

static long attempt_upgradebaudrate(int fd, long rate, long previousrate) {
	char brd_cmd[64];

	int timeout = 500; // this many ms
	int brt_val = timeout / 5; // ATBRT wants it in increments of five ms
	char brt_cmd[64];
	snprintf(brt_cmd, sizeof(brt_cmd), "ATBRT%02X", brt_val);

	blindcmd(fd, brt_cmd, 1);
	// printf("%s\n", brt_cmd);
	
	int brd_val = 4000000l/rate;
	snprintf(brd_cmd, sizeof(brd_cmd), "ATBRD%02X" OBDCMD_NEWLINE, brd_val);
	printf("%li [%02X]:", rate, brd_val);

	int nbytes = write(fd, brd_cmd, strlen(brd_cmd));
	if(-1 == nbytes) {
		printf("\n");
		perror("Error writing to serial port upgrading baudrate");
		return -1;
	}
	
	char elm_response[64];
	memset(elm_response, '\0', sizeof(elm_response));
	int readcount = 0;
	while(NULL == strstr(elm_response, "?") && NULL == strstr(elm_response, "OK")) {
		nbytes = read(fd, elm_response+readcount, sizeof(elm_response)-readcount);
		// printf("Read \"%s\", %i\n", elm_response, readcount);
		if(-1 == nbytes && errno != EAGAIN) {
			printf("\n");
			perror("Error reading[1] from serial port upgrading baudrate");
			return -1;
		}
		if(-1 != nbytes) {
			readcount += nbytes;
		}
	}

	// printf("\nreadcount: %i: \"%s\"\n", readcount, elm_response);
	if(NULL != strstr(elm_response, "OK")) {
		// printf("got OK");
		if(-1 == modifybaud(fd,rate)) {
			printf("Error modifying baudrate to %li\n", rate);
			return -1;
		}
		
		char elm_response2[256];
		memset(elm_response2, '\0', sizeof(elm_response2));

//		usleep(timeout * 1000); // Give it a chance to upgrade

		struct timeval start,end;
		gettimeofday(&start, NULL);

		int readcount2 = 0;
		while(readcount2 < 5 && NULL == strstr(elm_response2,">")) {
			nbytes = read(fd, elm_response2 + readcount2, sizeof(elm_response2)-readcount2);

			if(-1 == nbytes && EAGAIN != errno) {
				perror("Error reading[2] from serial port upgrading baudrate");
				modifybaud(fd,previousrate);
				return -1;
			}

			gettimeofday(&end, NULL);

			long timediff = (end.tv_sec-start.tv_sec)*1000000l + (end.tv_usec-start.tv_usec);
			if(2000l*timeout < timediff) {
				printf("timed out\n");
				break;
			}
			if(-1 != nbytes) {
				readcount2 += nbytes;
			}
		}
		// printf("\n\"%s\"\n", elm_response2);
		if(NULL != strstr(elm_response2, "ELM")) {
			write(fd, OBDCMD_NEWLINE, strlen(OBDCMD_NEWLINE));
			printf("success, ");
			return 0;
		} else {
			printf("fail [no ELM], ");
			modifybaud(fd,previousrate);
			return -1;
		}
	} else {
		printf("fail [no OK], ");
		modifybaud(fd,previousrate);
		return -1;
	}
}

long upgradebaudrate(int fd, long baudrate_target, long current_baudrate) {
// AT BRD is discussed on pages 9-10,48-49 of the ELM327 datasheet

	// Temporarily make sure this is nonblocking
	int old_flags = fcntl(fd, F_GETFL);
	if(-1 == fcntl(fd,F_SETFL,O_NONBLOCK)) {
		perror("fcntl");
	}

	// Try these speeds
	long speeds[] = { 38400, 57600, 115200, 230400, 460800, 500000, 576000 };

	int i;

	if(-1 == baudrate_target) {
		return 0;
	}

	long current_best = -1;

	printf("Baudrate upgrading: ");
	if(0 < baudrate_target) {
		int	retval = (0==attempt_upgradebaudrate(fd, baudrate_target,current_baudrate)?baudrate_target:-1);
		fcntl(fd,F_SETFL,old_flags);
		printf("\n");
		return retval;
	}

	for(i=0; i<sizeof(speeds)/sizeof(speeds[0]); i++) {
		if(0 == attempt_upgradebaudrate(fd, speeds[i], current_best)) {
			current_best = speeds[i];
		}
	}
	
	printf("\n");
	fcntl(fd,F_SETFL,old_flags);
	return current_best;
}

long guessbaudrate(int fd) {
	const char testcmd[] = "0100\r\n";
	long guesses[] = { 9600, 38400, 115200, 57600, 2400, 1200 };
	int i;

	printf("Baudrate guessing: ");

	for(i=0; i<sizeof(guesses)/sizeof(guesses[0]); i++) {
		long guess = guesses[i];

		printf("%li, ", guess);
		if(-1 == modifybaud(fd, guess)) {
			fprintf(stderr, "Error setting baudrate %li\n", guess);
			return -1;
		}

		char retbuf[1024];
		memset(retbuf, '\0', sizeof(retbuf));

		int nbytes = write(fd, testcmd, sizeof(testcmd));
		if(-1 == nbytes) {
			perror("Error writing to serial port guessing baudrate");
			return -1;
		}

		sleep(1); // CHEESY
		nbytes = read(fd, retbuf, sizeof(retbuf));
		if(-1 == nbytes) {
			perror("Error reading from serial port guessing baudrate");
			return -1;
		}
		if(NULL != strstr(retbuf, ">")) {
			printf("success at %li\n", guess);
			return guess;
		}
	}
	fprintf(stderr, "Couldn't guess baudrate\n");
	return -1;
}

int modifybaud(int fd, long baudrate) {
	// printf("Baudrate: %i\n", (int)baudrate);
	if(baudrate == -1) return 0;

	if(baudrate == 0) {
		return guessbaudrate(fd)>0?0:1;
	}

	struct termios options;
	if(0 != tcgetattr(fd, &options)) {
		perror("tcgetattr");
		return -1;
	}

	speed_t speedreq = B38400;

	switch(baudrate) {
#ifdef B4000000
		case 4000000:
			speedreq = B4000000;
			break;
#endif //B4000000
#ifdef B3500000
		case 3500000:
			speedreq = B3500000;
			break;
#endif //B3500000
#ifdef B3000000
		case 3000000:
			speedreq = B3000000;
			break;
#endif //B3000000
#ifdef B2500000
		case 2500000:
			speedreq = B2500000;
			break;
#endif //B2500000
#ifdef B2000000
		case 2000000:
			speedreq = B2000000;
			break;
#endif //B2000000
#ifdef B1500000
		case 1500000:
			speedreq = B1500000;
			break;
#endif //B1500000
#ifdef B1152000
		case 1152000:
			speedreq = B1152000;
			break;
#endif //B1152000
#ifdef B1000000
		case 1000000:
			speedreq = B1000000;
			break;
#endif //B1000000
#ifdef B9210600
		case 9210600:
			speedreq = B9210600;
			break;
#endif //B9210600
#ifdef B576000
		case 576000:
			speedreq = B576000;
			break;
#endif //B576000
#ifdef B500000
		case 500000:
			speedreq = B500000;
			break;
#endif //B500000
#ifdef B460800
		case 460800:
			speedreq = B460800;
			break;
#endif //B460800
#ifdef B230400
		case 230400:
			speedreq = B230400;
			break;
#endif //B230400
#ifdef B115200
		case 115200:
			speedreq = B115200;
			break;
#endif //B115200
#ifdef B76800
		case 76800:
			speedreq = B76800;
			break;
#endif //B76800
#ifdef B57600
		case 57600:
			speedreq = B57600;
			break;
#endif //B57600
		case 38400:
			speedreq = B38400;
			break;
#ifdef B28800
		case 28800:
			speedreq = B28800;
			break;
#endif //B28800
		case 19200:
			speedreq = B19200;
			break;
#ifdef B14400
		case 14400:
			speedreq = B14400;
			break;
#endif //B14400
		case 9600:
			speedreq = B9600;
			break;
#ifdef B7200
		case 7200:
			speedreq = B7200;
			break;
#endif //B7200
		case 4800:
			speedreq = B4800;
			break;
		case 2400:
			speedreq = B2400;
			break;
		case 1200:
			speedreq = B1200;
			break;
		case 600:
			speedreq = B600;
			break;
		case 300:
			speedreq = B300;
			break;
		case 150:
			speedreq = B150;
			break;
		case 134:
			speedreq = B134;
			break;
		case 110:
			speedreq = B110;
			break;
		case 75:
			speedreq = B75;
			break;
		case 50:
			speedreq = B50;
			break;
		case 0: // Don't look at me like *I* think it's a good idea
			speedreq = B0;
			break;
		default:
			fprintf(stderr,"Uknown baudrate: %li\n", baudrate);
			return -1;
			break;

	}

	if(0 != cfsetispeed(&options, speedreq)) {
		perror("cfsetispeed");
		return -1;
	}

	if(0 != cfsetospeed(&options, speedreq)) {
		perror("cfsetospeed");
		return -1;
	}

	if(0 != tcsetattr(fd, TCSANOW, &options)) {
		perror("tcsetattr");
		return -1;
	}

	return 0;
}

int startseriallog(const char *logname) {
	if(NULL == (seriallog = fopen(logname, "w"))) {
		perror("Couldn't open seriallog");
		return 1;
	}
	return 0;
}

void closeseriallog() {
	fflush(seriallog);
	fclose(seriallog);
	seriallog = NULL;
}

enum obd_serial_status getobderrorcodes(int fd,
	unsigned int *retvals, unsigned int retvals_size, int *numbytes_returned) {

	enum obd_serial_status ret;
	// First, find out how many codes are set
	unsigned int codecount[4];
	int c;
	if(OBD_SUCCESS != (ret = getobdbytes(fd, 0x01, 0x01, 0,
		codecount, sizeof(codecount)/sizeof(codecount[0]), &c, 0))) {

		return ret;
	}

	// This bit being set means the MIL is on, doesn't contribute to error count
	unsigned int mil_mask = 0x80;
	// Actual count of errors
	int numtroubles = codecount[0] & (~mil_mask);

	printf("%i trouble codes set [MIL is %s]\n", numtroubles,
		(codecount[0] & mil_mask)?"on":"off");

	if(0 == numtroubles) {
		*numbytes_returned = 0;
		return OBD_SUCCESS;
	}

	return getobdbytes(fd, 0x03, 0x00, 0, retvals, retvals_size, numbytes_returned, 0);
}


/// Parse a line from obd
static enum obd_serial_status parseobdline(const char *line, unsigned int mode, unsigned int cmd,
	unsigned int *retvals, unsigned int retvals_size, unsigned int *vals_read, int quiet) {

	unsigned int response; // Response. Should always be 0x40 + mode
	unsigned int cmdret; // Mode returned [should be the same as cmd]

	int count; // number of retvals successfully sscanf'd

	unsigned int currbytes[20]; // Might be a long line if there's a bunch of errors

	int have_cmd; // Set if we expect a cmd thing returned
	if(0x03 == mode || 0x04 == mode) {
		// Don't look for the second "cmd" item in some modes.
		have_cmd = 0;
	} else {
		have_cmd = 1;
	}

	// Number of items to subtract from count to figure out
	//   how many items in curr_bytes we found
	int count_sub;
	if(have_cmd) {
		count_sub = 2;
		count = sscanf(line,
			"%2x %2x "
			"%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x "
			"%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
			&response, &cmdret,
			currbytes, currbytes+1, currbytes+2, currbytes+3, currbytes+4,
			currbytes+5, currbytes+6, currbytes+7, currbytes+8, currbytes+9,
			currbytes+10, currbytes+11, currbytes+12, currbytes+13, currbytes+14,
			currbytes+15, currbytes+16, currbytes+17, currbytes+18, currbytes+19
			);
	} else {
		count_sub = 1;
		count = sscanf(line,
			"%2x "
			"%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x "
			"%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x",
			&response,
			currbytes, currbytes+1, currbytes+2, currbytes+3, currbytes+4,
			currbytes+5, currbytes+6, currbytes+7, currbytes+8, currbytes+9,
			currbytes+10, currbytes+11, currbytes+12, currbytes+13, currbytes+14,
			currbytes+15, currbytes+16, currbytes+17, currbytes+18, currbytes+19
			);
	}

	if(count <= 2) {
		if(!quiet)
			fprintf(stderr, "Couldn't parse line for %02X %02X: %s\n", mode, cmd, line);
		return OBD_UNPARSABLE;
	}

	if(response != 0x40 + mode) {
		if(!quiet)
			fprintf(stderr, "Unsuccessful mode response for %02X %02X: %s\n", mode, cmd, line);
		return OBD_INVALID_RESPONSE;
	}

	if(have_cmd && cmdret != cmd) {
		if(!quiet)
			fprintf(stderr, "Unsuccessful cmd response for %02X %02X: %s\n", mode, cmd, line);
		return OBD_INVALID_MODE;
	}

	int i;
	for(i=0;i<count-count_sub && i<retvals_size;i++) {
		retvals[i] = currbytes[i];
	}

	*vals_read = count-count_sub;

	return OBD_SUCCESS;
}


enum obd_serial_status getobdbytes(int fd, unsigned int mode, unsigned int cmd, int numbytes_expected,
	unsigned int *retvals, unsigned int retvals_size, int *numbytes_returned, int quiet) {

	char sendbuf[20]; // Command to send
	int sendbuflen; // Number of bytes in the send buffer

	char retbuf[4096]; // Buffer to store returned stuff

	int nbytes; // Number of bytes read

	if(mode == 0x03 || mode == 0x04) {
		sendbuflen = snprintf(sendbuf,sizeof(sendbuf),"%02X" OBDCMD_NEWLINE, mode);
	} else {
		if(0 == numbytes_expected) {
			sendbuflen = snprintf(sendbuf,sizeof(sendbuf),"%02X%02X" OBDCMD_NEWLINE, mode, cmd);
		} else {
			sendbuflen = snprintf(sendbuf,sizeof(sendbuf),"%02X%02X%01X" OBDCMD_NEWLINE, mode, cmd, numbytes_expected);
		}
	}

	appendseriallog(sendbuf, SERIAL_OUT);
	if(write(fd,sendbuf,sendbuflen) < sendbuflen) {
		return OBD_ERROR;
	}

	nbytes = readserialdata(fd, retbuf, sizeof(retbuf));
	if(0 == nbytes) {
		if(!quiet)
			fprintf(stderr, "No data at all returned from serial port\n");
		return OBD_ERROR;
	} else if(-1 == nbytes) {
		if(!quiet)
			fprintf(stderr, "Error reading from serial port\n");
		return OBD_ERROR;
	}

	// First some sanity checks on the data

	if(NULL != strstr(retbuf, "NO DATA")) {
		if(!quiet)
			fprintf(stderr, "OBD reported NO DATA for %02X %02X: %s\n", mode, cmd, retbuf);
		return OBD_NO_DATA;
	}

	if(0 != strstr(retbuf, "?")) {
		if(!quiet)
			fprintf(stderr, "OBD reported ? for %02X %02X: %s\n", mode, cmd, retbuf);
		return OBD_NO_DATA;
	}

	if(NULL != strstr(retbuf, "UNABLE TO CONNECT")) {
		if(!quiet)
			fprintf(stderr, "OBD reported UNABLE TO CONNECT for %02X %02X: %s\n", mode, cmd, retbuf);
		return OBD_UNABLE_TO_CONNECT;
	}

	/* 
		Good grief, this is ugly.
		1) We look go through the output line by line [strtokking]
		2) For each line, if it's a regular line, parse it
		3) If it has a colon near the start, it means it's a multi-line response
		4) So go into crazy C string handling mode.
	*/

	char *line = strtok(retbuf, "\r\n>");

	int values_returned = 0;
	enum obd_serial_status ret = OBD_ERROR;
	while(NULL != line) {
		char *colon;
		int joined_lines = 0; // Set if we joined some lines together
		char longline[1024] = "\0"; // Catenate other lines into this

		char *parseline = line; // The line to actually parse.

		while(NULL != line && NULL != (colon = strstr(line, ":"))) {
			// printf("Colon line: %s\n", line);
			strncat(longline, colon+1, sizeof(longline)-strlen(longline)-1);
			parseline = longline;
			joined_lines = 1;
			line = strtok(NULL, "\r\n>");
		}
		// We gracefully handle these lines without
		//   needing to actually parse them
		if(3 >= strlen(parseline)) continue;

		// printf("parseline: %s\n", parseline);

		unsigned int local_rets[20];
		unsigned int vals_read;

		ret = parseobdline(parseline, mode, cmd,
			local_rets, sizeof(local_rets)/sizeof(local_rets[0]), &vals_read, quiet);

		if(OBD_SUCCESS == ret) {
			int i;
			for(i=0; i<vals_read; i++, values_returned++) {
				retvals[values_returned] = local_rets[i];
			}
			break;
		}

		if(0 == joined_lines) {
			// If we joined some lines together, this strtok was already done
			line = strtok(NULL, "\r\n>");
		}
	}
	*numbytes_returned = values_returned;
	if(0 == values_returned) return ret;
	return OBD_SUCCESS;
}

enum obd_serial_status getobdvalue(int fd, unsigned int cmd, float *ret, int numbytes, OBDConvFunc conv) {
	int numbytes_returned;
	unsigned int obdbytes[4];

	enum obd_serial_status ret_status = getobdbytes(fd, 0x01, cmd, numbytes,
		obdbytes, sizeof(obdbytes)/sizeof(obdbytes[0]), &numbytes_returned, 0);

	if(OBD_SUCCESS != ret_status) return ret_status;

	if(NULL == conv) {
		int i;
		*ret = 0;
		for(i=0;i<numbytes_returned;i++) {
			*ret = *ret * 256;
			*ret = *ret + obdbytes[i];
		}
	} else {
		*ret = conv(obdbytes[0], obdbytes[1], obdbytes[2], obdbytes[3]);
	}
	return OBD_SUCCESS;
}

int getnumobderrors(int fd) {
	int numbytes_returned;
	unsigned int obdbytes[4];
	
	enum obd_serial_status ret_status = getobdbytes(fd, 0x01, 0x01, 0,
		obdbytes, sizeof(obdbytes)/sizeof(obdbytes[0]), &numbytes_returned, 0);
	
	if(OBD_SUCCESS != ret_status || 0 == numbytes_returned) return 0;
	
	if(obdbytes[0] > 0) {
		return obdbytes[0] & 0x7F;
	}
	
	return 0;
}

