// stars.hxx -- data structures and routines for managing and rendering stars.
//
// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@me.umn.edu
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


#ifndef _STARS_HXX
#define _STARS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#include <Time/fg_time.hxx>

#define FG_STAR_LEVELS 8         // how many star transitions

// Initialize the Star Management Subsystem
int fgStarsInit( void );

// Draw the Stars
void fgStarsRender( void );

extern struct OrbElements pltOrbElements[9];
extern fgTIME cur_time_params;


#endif // _STARS_HXX


// $Log$
// Revision 1.7  1998/09/24 15:36:20  curt
// Converted to c++ style comments.
//
// Revision 1.6  1998/09/24 15:25:26  curt
// Miscellaneous tweaks.
//
//
// Revision 1.5  1998/09/17 18:25:13  curt
// Fixed output message.
//
// Revision 1.4  1998/09/15 04:26:23  curt
// New textured moon and rewritten/restructured Astro code contributed by Durk
// Talsma.
//
// Revision 1.3  1998/08/06 12:45:20  curt
// Modified to bring in stars in 8 increments based on magnitude, not number
// of stars.
//
// Revision 1.2  1998/04/28 01:19:03  curt
// Type-ified fgTIME and fgVIEW
//
// Revision 1.1  1998/04/22 13:21:35  curt
// C++ - ifing the code a bit.
//
// Revision 1.5  1998/04/21 17:02:33  curt
// Prepairing for C++ integration.
//
// Revision 1.4  1998/02/12 21:59:39  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.3  1998/01/22 02:59:28  curt
// Changed #ifdef FILE_H to #ifdef _FILE_H
//
// Revision 1.2  1998/01/19 18:40:18  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.1  1998/01/07 03:16:20  curt
// Moved from .../Src/Scenery/ to .../Src/Astro/
//
// Revision 1.6  1997/10/25 03:18:29  curt
// Incorporated sun, moon, and planet position and rendering code contributed
// by Durk Talsma.
//
// Revision 1.5  1997/09/18 16:20:09  curt
// At dusk/dawn add/remove stars in stages.
//
// Revision 1.4  1997/09/05 01:36:00  curt
// Working on getting stars right.
//
// Revision 1.3  1997/08/29 17:55:28  curt
// Worked on properly aligning the stars.
//
// Revision 1.2  1997/08/27 21:32:30  curt
// Restructured view calculation code.  Added stars.
//
// Revision 1.1  1997/08/27 03:34:50  curt
// Initial revision.
//

