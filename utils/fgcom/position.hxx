/*
 * fgcom - VoIP-Client for the FlightGear-Radio-Infrastructure
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

#ifndef __POSITION_H__
#define __POSITION_H__

/* Compute distance between two points. */
double distance (double lat1, double lon1, double lat2, double lon2);

/* Build the phone number based on the ICAO code and airport frequency. */
void icao2number (char *icao, float frequency, char *buf);
void icao2atisnumber (char *icao, float frequency, char *buf);

/* Search for the closest airport on selected frequency. */
const char *
icaobypos (struct airport *airports, double frequency, double plane_lat,
	   double plane_lon, double range);

#endif
