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


#include <map>             // STL associative "array"
#include <string>          // Standard C++ string library


typedef struct {
    // char id[5];
    double longitude;
    double latitude;
    double elevation;
} fgAIRPORT;


class fgAIRPORTS {
    map < string, fgAIRPORT, less<string> > airports;

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
// Revision 1.4  1998/06/03 00:47:14  curt
// Updated to compile in audio support if OSS available.
// Updated for new version of Steve's audio library.
// STL includes don't use .h
// Small view optimizations.
//
// Revision 1.3  1998/06/01 17:54:42  curt
// Added Linux audio support.
// avoid glClear( COLOR_BUFFER_BIT ) when not using it to set the sky color.
// map stl tweaks.
//
// Revision 1.2  1998/05/29 20:37:22  curt
// Tweaked material properties & lighting a bit in GLUTmain.cxx.
// Read airport list into a "map" STL for dynamic list sizing and fast tree
// based lookups.
//
// Revision 1.1  1998/04/25 15:11:11  curt
// Added an command line option to set starting position based on airport ID.
//
//
