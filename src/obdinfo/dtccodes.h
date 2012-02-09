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

#ifndef __DTCCODES_H
#define __DTCCODES_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// Check whether this string is a valid error code
/** \return 1 if this is a valid error code, 0 otherwise */
int dtc_isvalid(const char *test);

/// Convert a human-friendly DTC to the byte representation
/** \param human the string code
	\param A,B put converted data into these
	\return 0 on success, -1 on error
*/
int dtc_humantobytes(const char *human, unsigned int *A, unsigned int *B);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif //__DTCCODES_H

