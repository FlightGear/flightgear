/**************************************************************************
 * planets.h
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


#ifndef _PLANETS_H
#define _PLANETS_H


struct CelestialCoord fgCalculatePlanet(struct OrbElements planet,
                                         struct OrbElements theSun,
                                         struct fgTIME t, int idx);


void fgPlanetsInit();
void fgPlanetsRender();


#endif /* PLANETS_H */


/* $Log$
/* Revision 1.3  1998/02/02 20:53:23  curt
/* To version 0.29
/*
 * Revision 1.2  1998/01/22 02:59:28  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.1  1998/01/07 03:16:18  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.3  1997/12/30 16:36:53  curt
 * Merged in Durk's changes ...
 *
 * Revision 1.2  1997/12/12 21:41:30  curt
 * More light/material property tweaking ... still a ways off.
 *
 * Revision 1.1  1997/10/25 03:16:11  curt
 * Initial revision of code contributed by Durk Talsma.
 *
 */
