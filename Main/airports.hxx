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


#include <string>        // Standard C++ string library
#include <map>           // STL associative "array"

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif


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
// Revision 1.7  1998/07/24 21:39:09  curt
// Debugging output tweaks.
// Cast glGetString to (char *) to avoid compiler errors.
// Optimizations to fgGluLookAt() by Norman Vine.
//
// Revision 1.6  1998/07/06 21:34:19  curt
// Added an enable/disable splash screen option.
// Added an enable/disable intro music option.
// Added an enable/disable instrument panel option.
// Added an enable/disable mouse pointer option.
// Added using namespace std for compilers that support this.
//
// Revision 1.5  1998/06/17 21:35:11  curt
// Refined conditional audio support compilation.
// Moved texture parameter setup calls to ../Scenery/materials.cxx
// #include <string.h> before various STL includes.
// Make HUD default state be enabled.
//
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
