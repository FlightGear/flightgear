//
// airports.cxx -- a really simplistic class to manage airport ID,
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


#include <string>

#include <Debug/fg_debug.h>
#include <Include/fg_zlib.h>
#include <Main/options.hxx>

#include "simple.hxx"


// Constructor
fgAIRPORTS::fgAIRPORTS( void ) {
}


// load the data
int fgAIRPORTS::load( const string& file ) {
    fgAIRPORT a;
    string path, fgpath, id;
    char id_raw[256], line[256];
    fgFile f;

    // build the path name to the airport file
    path = current_options.get_fg_root() + "/Airports/" + file;
    fgpath = path + ".gz";

    // first try "path.gz"
    if ( (f = fgopen(fgpath.c_str(), "rb")) == NULL ) {
	// next try "path"
        if ( (f = fgopen(path.c_str(), "rb")) == NULL ) {
	    fgPrintf( FG_GENERAL, FG_EXIT, "Cannot open file: %s\n", 
		      path.c_str());
	}
    }

    while ( fggets(f, line, 250) != NULL ) {
	// printf("%s", line);

	sscanf( line, "%s %lf %lf %lfl\n", id_raw, &a.longitude, &a.latitude, 
		&a.elevation );
	id = id_raw;
	airports[id] = a;
    }

    fgclose(f);

    return(1);
}


// search for the specified id
fgAIRPORT fgAIRPORTS::search( char *id ) {
    map < string, fgAIRPORT, less<string> > :: iterator find;
    fgAIRPORT a;

    find = airports.find(id);
    if ( find == airports.end() ) {
	// not found
	a.longitude = a.latitude = a.elevation = 0;
    } else {
	a = (*find).second;
    }

    return(a);
}


// Destructor
fgAIRPORTS::~fgAIRPORTS( void ) {
}


// $Log$
// Revision 1.3  1998/08/27 17:01:55  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.2  1998/08/25 20:53:24  curt
// Shuffled $FG_ROOT file layout.
//
// Revision 1.1  1998/08/25 17:19:13  curt
// Moved from ../Main/
//
// Revision 1.8  1998/07/13 21:01:37  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.7  1998/06/03 22:01:07  curt
// Tweaking sound library usage.
//
// Revision 1.6  1998/06/03 00:47:13  curt
// Updated to compile in audio support if OSS available.
// Updated for new version of Steve's audio library.
// STL includes don't use .h
// Small view optimizations.
//
// Revision 1.5  1998/05/29 20:37:22  curt
// Tweaked material properties & lighting a bit in GLUTmain.cxx.
// Read airport list into a "map" STL for dynamic list sizing and fast tree
// based lookups.
//
// Revision 1.4  1998/05/13 18:26:25  curt
// Root path info moved to fgOPTIONS.
//
// Revision 1.3  1998/05/06 03:16:24  curt
// Added an averaged global frame rate counter.
// Added an option to control tile radius.
//
// Revision 1.2  1998/04/28 21:42:50  curt
// Wrapped zlib calls up so we can conditionally comment out zlib support.
//
// Revision 1.1  1998/04/25 15:11:11  curt
// Added an command line option to set starting position based on airport ID.
//
//
