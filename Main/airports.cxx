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


#include <string.h>

#include <Debug/fg_debug.h>
#include <Include/general.h>
#include <Include/fg_zlib.h>

#include "airports.hxx"


// Constructor
fgAIRPORTS::fgAIRPORTS( void ) {
}


// load the data
int fgAIRPORTS::load( char *file ) {
    fgGENERAL *g;
    char path[256], fgpath[256], line[256];
    char id[5];
    double lon, lat, elev;
    fgFile f;

    g = &general;

    // build the path name to the ambient lookup table
    path[0] = '\0';
    strcat(path, g->root_dir);
    strcat(path, "/Scenery/");
    strcat(path, "Airports");
    strcpy(fgpath, path);
    strcat(fgpath, ".gz");

    // first try "path.gz"
    if ( (f = fgopen(fgpath, "rb")) == NULL ) {
	// next try "path"
        if ( (f = fgopen(path, "rb")) == NULL ) {
	    fgPrintf(FG_GENERAL, FG_EXIT, "Cannot open file: %s\n", path);
	}
    }

    size = 0;
    while ( fggets(f, line, 250) != NULL ) {
	// printf("%s", line);

	if ( size < MAX_AIRPORTS ) {
	    sscanf( line, "%s %lf %lf %lfl\n", id, &lon, &lat, &elev );
	    strcpy(airports[size].id, id);
	    airports[size].longitude = lon;
	    airports[size].latitude = lat;
	    airports[size].elevation = elev;
	} else {
	    fgPrintf( FG_GENERAL, FG_EXIT, 
		      "Overran size of airport list in fgAIRPORTS::load()\n");
	}

	size++;
    }

    fgclose(f);

    return(1);
}


// search for the specified id
fgAIRPORT fgAIRPORTS::search( char *id ) {
    fgAIRPORT a;
    int i;

    for ( i = 0; i < size; i++ ) {
	if ( strcmp(airports[i].id, id) == 0 ) {
	    return(airports[i]);
	}
    }

    strcpy(a.id, "none");
    return(a);
}


// Destructor
fgAIRPORTS::~fgAIRPORTS( void ) {
}


// $Log$
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
