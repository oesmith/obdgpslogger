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
 \brief Generate data from FLTK GUI
*/

/* Do not bother trying to compile this. It's just going to be
    #included in another C file. It's here to work around a limitation
    in fluid. */

struct obdsim_generator obdsimgen_gui_fltk = {
	guifltk_simgen_name,
	guifltk_simgen_longdesc,
	guifltk_simgen_create,
	guifltk_simgen_destroy,
	guifltk_simgen_getvalue,
	guifltk_simgen_idle,
	guifltk_simgen_geterrorcodes,
	guifltk_simgen_clearerrorcodes
};

