//
// airports.hxx -- a really simplistic class to manage airport ID,
//                 lat, lon of the center of one of it's runways, and 
//                 elevation in feet.
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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


#ifndef _AIRPORTS_HXX
#define _AIRPORTS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#define MAX_AIRPORTS 10000


typedef struct {
    char id[5];
    double longitude, latitude, elevation;
} fgAIRPORT;


class fgAIRPORTS {
    fgAIRPORT airports[MAX_AIRPORTS];
    int size;

public:

    // Constructor
    fgAIRPORTS( void );

    // load the data
    int load( char *file );

    // search for the specified id
    fgAIRPORT search( char *id );

    // Destructor
    ~fgAIRPORTS( void );

};


#endif /* _AIRPORTS_HXX */


// $Log$
// Revision 1.1  1998/04/25 15:11:11  curt
// Added an command line option to set starting position based on airport ID.
//
//
