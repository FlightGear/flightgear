/**************************************************************************
 * moon.h
 *
 * Written 1997 by Durk Talsma, started October, 1997.
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


#ifndef _MOON_H
#define _MOON_H


#include <Astro/orbits.h>

#include <Time/fg_time.h>
#include <math.h>

/* #define X .525731112119133606 */
/* #define Z .850650808352039932 */


 /* Initialize the Moon Display management Subsystem */
void fgMoonInit( void );

/* Draw the Moon */
void fgMoonRender( void );

struct CelestialCoord fgCalculateMoon(struct OrbElements Params,
                                      struct OrbElements sunParams,
                                      struct fgTIME t);

extern struct OrbElements pltOrbElements[9];

#endif /* _MOON_H */


/* $Log$
/* Revision 1.5  1998/02/02 20:53:21  curt
/* To version 0.29
/*
 * Revision 1.4  1998/01/22 02:59:27  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.3  1998/01/19 19:26:58  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.2  1998/01/19 18:40:17  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.1  1998/01/07 03:16:16  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.4  1997/12/11 04:43:56  curt
 * Fixed sun vector and lighting problems.  I thing the moon is now lit
 * correctly.
 *
 * Revision 1.3  1997/11/25 19:25:35  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.2  1997/10/25 03:24:23  curt
 * Incorporated sun, moon, and star positioning code contributed by Durk Talsma.
 *
 * Revision 1.1  1997/10/25 03:16:09  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */
