/**************************************************************************
 * weather.h -- routines to model weather
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


#ifndef WEATHER_H
#define WEATHER_H


/* holds the current weather values */
struct WEATHER {
    float visibility;
};

extern struct WEATHER current_weather;


/* Initialize the weather modeling subsystem */
void fgWeatherInit(void);

/* Update the weather parameters for the current position */
void fgWeatherUpdate(double lon, double lat, double alt);


#endif /* WEATHER_H */


/* $Log$
/* Revision 1.4  1997/08/27 03:30:39  curt
/* Changed naming scheme of basic shared structures.
/*
 * Revision 1.3  1997/08/22 21:34:43  curt
 * Doing a bit of reorganizing and house cleaning.
 *
 * Revision 1.2  1997/07/23 21:52:30  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.1  1997/07/19 23:03:58  curt
 * Initial revision.
 *
 */
