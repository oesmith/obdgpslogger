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
 \brief gps communications
 */
#ifdef HAVE_GPSD

#ifndef __GPSCOMM_H
#define __GPSCOMM_H

#include <gps.h>

/// Open the gps
/** \param server server running gpsd
 \param port port gpsd is listening on
 \return a gps_data_t, or NULL on failure
 */
struct gps_data_t *opengps(char *server, char *port);

/// Close the gps
void closegps(struct gps_data_t *g);

/// Get the current position
/** \param g the gps_data_t returned from opengps
 \param lat pointer to where you want the latitude stored
 \param lon pointer to where you want the longitude stored
 \param alt pointer to where you want the altitude stored
 \param course pointer to where you want the course stored
 \param speed pointer to where you want the speed stored
 \param gpstime pointer to where you want the gpstime stored
 \return -1 for no workable co-ordinates, 0 for lat,lon, and 1 for lat,lon,alt
 */
int getgpsposition(struct gps_data_t *g, double *lat, double *lon, double *alt, double *speed, double *course, double *gpstime);


#endif //__GPSCOMM_H

#endif //HAVE_GPSD

