// master.cxx -- top level construction routines
//
// Written by Curtis Olson, started May 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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


#include <sys/types.h>  // for directory reading
#include <dirent.h>     // for directory reading

#include <string>

#include <Bucket/newbucket.hxx>

// #include <Include/fg_constants.h>
// #include <Math/mat3.h>

// #include <Debug/logstream.hxx>
// #include <Array/array.hxx>
// #include <Clipper/clipper.hxx>
// #include <GenOutput/genobj.hxx>
// #include <Match/match.hxx>
// #include <Triangulate/triangle.hxx>


// check if the specified tile has data defined for it
static bool has_data( const string& path, const FGBucket& b ) {
    
    return false;
}


// build the tile and check for success
bool build_tile( const string& work_base, const string& output_base, 
		 const FGBucket& b ) {
    return true;
}


// display usage and exit
void usage( const string name ) {
    cout << "Usage: " << name << " <work_base> <output_base>" << endl;
    exit(-1);
}


main(int argc, char **argv) {
    double lon, lat;

    if ( argc < 3 ) {
	usage( argv[0] );
    }

    string work_base = argv[1];
    string output_base = argv[2];

    FGBucket tmp1( 0.0, 0.0 );

    double dy = tmp1.get_height();
	
    lat = -90.0 + dy * 0.5;
    while ( lat <= 90.0 ) {
	FGBucket tmp2( 0.0, lat );
	double dx = tmp2.get_width();

	lon = -180 + dx * 0.5;
	while ( lon <= 180.0 ) {
	    FGBucket b( lon, lat );
	    cout << "Bucket = " << b << endl;

	    if ( has_data( work_base, b ) ) {
		build_tile( work_base, output_base, b );
	    }

	    lon += dx;
	}


	lat += dy;
    }
}


