/**************************************************************************
 * aircraft.h -- define shared aircraft parameters
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


#ifndef _AIRCRAFT_H
#define _AIRCRAFT_H


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Flight/flight.hxx>
#include <Controls/controls.h>


/* Define a structure containing all the parameters for an aircraft */
typedef struct{
    fgFLIGHT   *flight;
    fgCONTROLS *controls;
} fgAIRCRAFT ;


/* current_aircraft contains all the parameters of the aircraft
   currently being operated. */
extern fgAIRCRAFT current_aircraft;


/* Initialize an Aircraft structure */
void fgAircraftInit( void );


/* Display various parameters to stdout */
void fgAircraftOutputCurrent(fgAIRCRAFT *a);


#endif /* _AIRCRAFT_H */


/* $Log$
/* Revision 1.1  1998/10/16 23:26:49  curt
/* C++-ifying.
/*
 * Revision 1.12  1998/04/22 13:26:15  curt
 * C++ - ifing the code a bit.
 *
 * Revision 1.11  1998/04/21 17:02:27  curt
 * Prepairing for C++ integration.
 *
 * Revision 1.10  1998/02/07 15:29:32  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.9  1998/01/22 02:59:23  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.8  1998/01/19 19:26:57  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.7  1997/12/10 22:37:38  curt
 * Prepended "fg"AIRCRAFT_H */


/* $Log$
/* Revision 1.1  1998/10/16 23:26:49  curt
/* C++-ifying.
/*
 * Revision 1.12  1998/04/22 13:26:15  curt
 * C++ - ifing the code a bit.
 *
 * Revision 1.11  1998/04/21 17:02:27  curt
 * Prepairing for C++ integration.
 *
 * Revision 1.10  1998/02/07 15:29:32  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.9  1998/01/22 02:59:23  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.8  1998/01/19 19:26:57  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.7  1997/12/10 22:37:38  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.6  1997/09/13 02:00:06  curt
 * Mostly working on stars and generating sidereal time for accurate star
 * placement.
 *
 * Revision 1.5  1997/08/27 03:29:58  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.4  1997/07/23 21:52:17  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.3  1997/06/21 17:12:42  curt
 * Capitalized subdirectory names.
 *
 * Revision 1.2  1997/05/23 15:40:30  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 15:58:25  curt
 * Initial revision.
 *
 */
