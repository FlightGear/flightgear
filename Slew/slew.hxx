/**************************************************************************
 * slew.hxx -- the "slew" flight model
 *
 * Written by Curtis Olson, started May 1997.
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


#ifndef _SLEW_HXX
#define _SLEW_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


/* reset flight params to a specific position */ 
void fgSlewInit(double pos_x, double pos_y, double pos_z, double heading);

/* update position based on inputs, positions, velocities, etc. */
void fgSlewUpdate( void );


#endif /* _SLEW_HXX */


/* $Log$
/* Revision 1.1  1998/10/16 23:27:52  curt
/* C++-ifying.
/*
 * Revision 1.4  1998/01/22 02:59:34  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.3  1998/01/19 18:40:30  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.2  1997/07/23 21:52:20  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.1  1997/05/29 02:29:43  curt
 * Moved to their own directory.
 *
 * Revision 1.2  1997/05/23 15:40:38  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 16:04:46  curt
 * Initial revision.
 *
 */
