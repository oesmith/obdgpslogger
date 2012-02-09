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
 \brief Data generators need to implement the functions defined herein
*/

#ifndef __DATASOURCE_H
#define __DATASOURCE_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Declare a generator by building one of these structs
struct obdsim_generator {
	/// Get a human-friendly name for this generator
	const char *(*name)();

	/// Get a long description [typically including seed info] for this generator
	const char *(*longdesc)();

	/// Initialise the data generator
	/** \return 0 for success, 1 for failure
    	  \param gen opaque data generator
    	  \param seed intialisation seed. implementation specific
	*/
	int (*create)(void **gen, const char *seed);

	/// Shut down the data generator
	void (*destroy)(void *gen);

	/// Get a value for the specified PID
	/** \return number of values created, or -1 for "must exit"
    	  \param mode the mode of the request
    	  \param PID the PID this is for
    	  \param gen opaque data generator
    	  \param A,B,C,D four values to fill
	*/
	int (*getvalue)(void *gen, unsigned int mode, unsigned int PID,
		unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D);

	/// Called whenever the simulator is idle
	/** \param idlems Take no longer than this many milliseconds
	 \return anything other than zero is considered a condition which means we must exit
	 */
	int (*idle)(void *gen, int idlems);

	/// Called to find out if there are any error conditions set, and what they are
	/** NOTE! errorcodes is an array of bytes. There are twice as many spaces in errorcodes as num_codes
	 \param errorcodes populate this with the error codes currently set
	 \param num_codes number of errorcodes available to be populated
	 \param mil whether or not MIL is set
	 \return -1 on error. >=0 num error codes. num_codes+1 to indicate "more than there was space for"
	*/
	int (*geterrorcodes)(void *gen, unsigned int *errorcodes, int num_codes, int *mil);

	/// Called to signify that error codes should be cleared
	/** \return -1 on error, 0 on success */
	int (*clearerrorcodes)(void *gen);
};

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__DATASOURCE_H

