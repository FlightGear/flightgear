/**************************************************************************
 * stars.h -- data structures and routines for managing and rendering stars.
 *
 * Written by Curtis Olson, started August 1997.
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


#ifndef _STARS_H
#define _STARS_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


#define FG_MAX_STARS 500
#define FG_STAR_LEVELS 4         /* how many star transitions */
#define FG_MIN_STAR_MAG 0.738750 /* magnitude of weakest star we'll display */

/* Initialize the Star Management Subsystem */
int fgStarsInit( void );

/* Draw the Stars */
void fgStarsRender( void );
extern struct OrbElements pltOrbElements[9];
extern struct fgTIME cur_time_params;


#ifdef __cplusplus
}
#endif


#endif /* _STARS_H */


/* $Log$
/* Revision 1.5  1998/04/21 17:02:33  curt
/* Prepairing for C++ integration.
/*
 * Revision 1.4  1998/02/12 21:59:39  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.3  1998/01/22 02:59:28  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.2  1998/01/19 18:40:18  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.1  1998/01/07 03:16:20  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.6  1997/10/25 03:18:29  curt
 * Incorporated sun, moon, and planet position and rendering code contributed
 * by Durk Talsma.
 *
 * Revision 1.5  1997/09/18 16:20:09  curt
 * At dusk/dawn add/remove stars in stages.
 *
 * Revision 1.4  1997/09/05 01:36:00  curt
 * Working on getting stars right.
 *
 * Revision 1.3  1997/08/29 17:55:28  curt
 * Worked on properly aligning the stars.
 *
 * Revision 1.2  1997/08/27 21:32:30  curt
 * Restructured view calculation code.  Added stars.
 *
 * Revision 1.1  1997/08/27 03:34:50  curt
 * Initial revision.
 *
 */
