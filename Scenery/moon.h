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


#ifndef _MOON_H_
#define _MOON_H_


#include "orbits.h"

#include "../Time/fg_time.h"


 /* Initialize the Moon Display management Subsystem */
void fgMoonInit();

/* Draw the Stars */
void fgMoonRender();

struct CelestialCoord fgCalculateMoon(struct OrbElements Params,
                                      struct OrbElements sunParams,
                                      struct fgTIME t);

extern struct OrbElements pltOrbElements[9];


#endif /* _MOON_H_ */


/* $Log$
/* Revision 1.2  1997/10/25 03:24:23  curt
/* Incorporated sun, moon, and star positioning code contributed by Durk Talsma.
/*
 * Revision 1.1  1997/10/25 03:16:09  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */
