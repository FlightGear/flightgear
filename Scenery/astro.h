/**************************************************************************
 * astro.h
 *
 * Written by Durk Talsma. Started November 1997, for use with the flight
 * gear project.
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


#ifndef _ASTRO_H_
#define _ASTRO_H_

#include <GL/glut.h>
#include "stars.h"

extern struct CelestialCoord
    moonPos;

extern float xMoon, yMoon, zMoon, xSun, ySun, zSun;
/* extern GLint moon, sun; */
extern GLint stars[FG_STAR_LEVELS];


/* Initialize Astronomical Objects */
void fgAstroInit();

/* Render Astronomical objects */
void fgAstroRender();


#endif /* _ASTRO_H_ */


/* $Log$
/* Revision 1.3  1997/12/17 23:13:46  curt
/* Began working on rendering the sky.
/*
 * Revision 1.2  1997/12/11 04:43:56  curt
 * Fixed sun vector and lighting problems.  I thing the moon is now lit
 * correctly.
 *
 * Revision 1.1  1997/11/25 23:20:23  curt
 * Initial revision.
 *
 */
