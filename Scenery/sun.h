/**************************************************************************
 * sun.h
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


#ifndef SUN_H
#define SUN_H


struct SunPos fgCalcSunPos(struct OrbElements sunParams);
extern struct OrbElements pltOrbElements[9];

/* Initialize the Sun */
void fgSunInit();

/* Draw the Sun */
void fgSunRender();


#endif /* SUN_H */


/* $Log$
/* Revision 1.3  1997/12/11 04:43:56  curt
/* Fixed sun vector and lighting problems.  I thing the moon is now lit
/* correctly.
/*
 * Revision 1.2  1997/11/25 19:25:39  curt
 * Changes to integrate Durk's moon/sun code updates + clean up.
 *
 * Revision 1.1  1997/10/25 03:16:12  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */
