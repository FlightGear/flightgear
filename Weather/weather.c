/**************************************************************************
 * weather.c -- routines to model weather
 *
 * Written by Curtis Olson, started July 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include "weather.h"
#include "../Aircraft/aircraft.h"
#include "../Math/fg_random.h"


/* Initialize the weather modeling subsystem */
void fgWeatherInit(void) {
    struct weather_params *w;

    w = &current_weather;

    /* Configure some wind */
    /* FG_V_north_airmass = 15; */ /* ft/s =~ 10mph */

    w->visibility = 60000.0;       /* meters = 60km */
}


/* Update the weather parameters for the current position */
void fgWeatherUpdate(double lon, double lat, double alt) {
    struct flight_params *f;
    struct weather_params *w;

    f = &current_aircraft.flight;
    w = &current_weather;

    /* Add some random turbulence */
    /* FG_U_gust = fg_random() * 1.0 - 0.5;
    FG_V_gust = fg_random() * 1.0 - 0.5;
    FG_W_gust = fg_random() * 1.0 - 0.5; */
}


/* $Log$
/* Revision 1.5  1997/08/22 21:34:42  curt
/* Doing a bit of reorganizing and house cleaning.
/*
 * Revision 1.4  1997/08/02 16:23:55  curt
 * Misc. tweaks.
 *
 * Revision 1.3  1997/07/31 22:52:41  curt
 * Working on redoing internal coordinate systems & scenery transformations.
 *
 * Revision 1.2  1997/07/30 16:12:44  curt
 * Moved fg_random routines from Util/ to Math/
 *
 * Revision 1.1  1997/07/19 23:03:57  curt
 * Initial revision.
 *
 */
