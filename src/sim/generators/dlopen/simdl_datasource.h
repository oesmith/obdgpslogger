/*
 In the intention of being able to work with closed-source plugins,
 this file is explicitly NOT under the GPL. I [Gary "ChunkyKs" Briggs]
 disclaim copyright on this source code, and in the spirit of SQLite
 instead place a blessing here:
 
     May you do good and not evil.
     May you find forgiveness for yourself and forgive others.
     May you share freely, never taking more than you give.
 */

/** \file
 \brief Dlopen()'d data generators need to implement the functions defined herein
*/

#ifndef __SIMDL_DATASOURCE_H
#define __SIMDL_DATASOURCE_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/// Get a human-friendly name for this generator
const char *simdl_name();

/// Initialise the data generator
/** \return 0 for success, 1 for failure
   	  \param gen opaque data generator
   	  \param seed intialisation seed. implementation specific
*/
int simdl_create(void **gen, const char *seed);

/// Shut down the data generator
void simdl_destroy(void *gen);

/// Get a value for the specified PID
/** \return number of values created
   	  \param mode the mode of this request
   	  \param PID the PID this is for
   	  \param gen opaque data generator
   	  \param A,B,C,D four values to fill
*/
int simdl_getvalue(void *gen, unsigned int mode, unsigned int PID,
		unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D);

/// Called whenever the sim is idle. Do not block more than timems milliseconds
/** \return anything other than zero is considered a condition which means we must exit
 */
int simdl_idle(void *gen, int timems);

/// Called to find out if there are any error conditions set, and what they are
/** \param errorcodes populate this with the error codes currently set
	\param num_codes number of errorcodes available to be populated
	\param mil set to indicate whether MIL is on
	\return -1 on error. >=0 num error codes. num_codes+1 to indicate "more than there was space for"
*/
int simdl_geterrorcodes(void *gen, unsigned int *errorcodes, int num_codes, int *mil);

/// Called to signify that error codes should be cleared
/** \return -1 on error, 0 on success */
int simdl_clearerrorcodes(void *gen);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__SIMDL_DATASOURCE_H

