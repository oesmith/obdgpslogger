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
 \brief Guess likely serial device names
 */
#ifndef __GUESSDEVS_H
#define __GUESSDEVS_H

#include <FL/Fl_Input_Choice.H>

/// Take some guesses at likely serial devices and populate the inputchoice
/** \param addto the input_choice to add all the likely candidates to
  \param def if provided, is added and made the default choice */
int populateSerialDevs(Fl_Input_Choice *addto, const char *def = NULL);

#endif //__GUESSDEVS_H

