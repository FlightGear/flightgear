/**************************************************************************
 * sky.h -- model sky with an upside down "bowl"
 *
 * Written by Curtis Olson, started December 1997.
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


#ifndef _SKY_H
#define _SKY_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


/* (Re)generate the display list */
void fgSkyInit( void );

/* (Re)calculate the sky colors at each vertex */
void fgSkyColorsInit( void );

/* Draw the Sky */
void fgSkyRender( void );


#ifdef __cplusplus
}
#endif


#endif /* _SKY_H */


/* $Log$
/* Revision 1.4  1998/04/21 17:02:32  curt
/* Prepairing for C++ integration.
/*
 * Revision 1.3  1998/01/22 02:59:28  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.2  1998/01/19 18:40:17  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.1  1998/01/07 03:16:19  curt
 * Moved from .../Src/Scenery/ to .../Src/Astro/
 *
 * Revision 1.2  1997/12/22 23:45:49  curt
 * First stab at sunset/sunrise sky glow effects.
 *
 * Revision 1.1  1997/12/17 23:14:31  curt
 * Initial revision.
 * Begin work on rendering the sky. (Rather than just using a clear screen.)
 *
 */
