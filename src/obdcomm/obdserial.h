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

#ifndef __OBDSERIAL_H
#define __OBDSERIAL_H

#include "obdconvertfunctions.h"

/// This is returned from getobdvalue
enum obd_serial_status {
	OBD_SUCCESS, ///< Successfully found value
	OBD_NO_DATA, ///< Got a NO DATA message
	OBD_UNPARSABLE, ///< Couldn't parse OBD return message
	OBD_INVALID_RESPONSE, ///< Invalid response
	OBD_INVALID_MODE, ///< Invalid mode
	OBD_UNABLE_TO_CONNECT, ///< Device reported UNABLE TO CONNECT
	OBD_ERROR ///< Some other error
};

/// The timeout for serial reads in general, measured in usec
#define OBDCOMM_TIMEOUT 10000000l

/// Open the serial port and set appropriate options
/**
 \param portfilename path and filename of the serial port
 \param baudrate -1 for "don't touch", 0 for "guess", >0 for the passed number
 \param baudrate_target -1 for "don't touch", 0 for "guess", >0 for the passed number
 \return fd on success or -1 on error
 */
int openserial(const char *portfilename, long baudrate, long baudrate_target);

/// Close the serialport
void closeserial(int fd);

/// Modify the baudrate of the passed serial port
/**
 \param baurdate attempt to set to this value. -1 makes no changes
 \param fd the opened serial port to try it on
 \return 0 on success, or -1 on failure
 */
int modifybaud(int fd, long baudrate);

/// Write to this log
int startseriallog(const char *logname);

/// Close the log
void closeseriallog();

/// Get the currently set error codes.
enum obd_serial_status getobderrorcodes(int fd,
        unsigned int *retvals, unsigned int retvals_size, int *numbytes_returned);

/// Get an OBD value
/** This sends "01 cmd-in-hex" to the OBD device.
 It then grabs all the data returned and does its best to put it into the return value
 \param fd the serial port opened with openserial
 \param cmd the obd service command
 \param ret the return value
 \param numbytes the number of bytes we expect in the response [optimisation]. Set to zero for safe-nothing-can-go-wrong
 \param conv the convert function to get from A,B,C,D to a float
 \return something from the obd_serial_status enum 
 */
enum obd_serial_status getobdvalue(int fd, unsigned int cmd, float *ret, int numbytes, OBDConvFunc conv);

/// Get the raw bits returned from an OBD command
/** This returns some unsigned integers. Each contains eight bits
	in its low byte and zeros in the rest
 \param fd the serial port opened with openserial
 \param cmd the obd service command
 \param numbytes_expected the number of bytes we expect in the response [optimisation]. Set to zero for safe-nothing-can-go-wrong
 \param retvals_size number of retvals allocated in the retvals array
 \param retvals array of retvals_size items to stuff with retvalues
 \param numbytes_returned tells you how many of retvals were filled.
 \return something from the obd_serial_status enum
*/
enum obd_serial_status getobdbytes(int fd, unsigned int mode, unsigned int cmd, int numbytes_expected,
        unsigned int *retvals, unsigned int retvals_size, int *numbytes_returned, int quiet);

/// Get the number of errors codes the car claims to currently have
int getnumobderrors(int fd);

#endif // __OBDSERIAL_H

