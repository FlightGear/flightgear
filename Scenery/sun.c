/**************************************************************************
 * sun.c
 *
 * Written 1997 by Durk Talsma, started October, 1997.  For the flight gear
 * project.
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


#include "../Time/fg_time.h"
#include "orbits.h"

struct SunPos fgCalcSunPos(struct OrbElements params)
{
    double
    	EccAnom, lonSun,
        xv, yv, v, r;
    struct SunPos
    	solarPosition;

    /* calculate the eccentric anomaly */
    EccAnom = fgCalcEccAnom(params.M, params.e);

    /* calculate the Suns distance (r) and its true anomaly (v) */
	xv = cos(EccAnom) - params.e;
    yv = sqrt(1.0 - params.e*params.e) * sin(EccAnom);
    v = atan2(yv, xv);
    r = sqrt(xv*xv + yv*yv);

    /* calculate the the Suns true longitude (lonsun) */
    lonSun = v + params.w;

	/* convert true longitude and distance to ecliptic rectangular geocentric
       coordinates (xs, ys) */
    solarPosition.xs = r * cos(lonSun);
    solarPosition.ys = r * sin(lonSun);
    return solarPosition;
}


/* $Log$
/* Revision 1.1  1997/10/25 03:16:11  curt
/* Initial revision of code contributed by Durk Talsma.
/*
 */
