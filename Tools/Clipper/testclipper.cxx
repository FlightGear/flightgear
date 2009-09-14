// main.cxx -- sample use of the clipper lib
//
// Written by Curtis Olson, started February 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)
 


#include <Debug/logstream.hxx>
#include <Bucket/newbucket.hxx>

#include "clipper.hxx"


int main( int argc, char **argv ) {
    point2d global_min, global_max;

    fglog().setLogLevels( FG_ALL, FG_DEBUG );

    global_min.x = global_min.y = 200;
    global_max.y = global_max.x = -200;

    FGClipper clipper;
    clipper.init();

    if ( argc < 2 ) {
	FG_LOG( FG_CLIPPER, FG_ALERT, "Usage: " << argv[0] 
		<< " file1 file2 ..." );
	exit(-1);
    }

    // process all specified polygon files
    for ( int i = 1; i < argc; i++ ) {
	string full_path = argv[i];

	// determine bucket for this polygon
	int pos = full_path.rfind("/");
	string file_name = full_path.substr(pos + 1);
	cout << "file name = " << file_name << endl;

	pos = file_name.find(".");
	string base_name = file_name.substr(0, pos);
	cout << "base_name = " << base_name << endl;

	long int index;
	sscanf( base_name.c_str(), "%ld", &index);
	FGBucket b(index);
	cout << "bucket = " << b << endl;

	// calculate bucket dimensions
	point2d c, min, max;

	c.x = b.get_center_lon();
	c.y = b.get_center_lat();
	double span = bucket_span(c.y);

	if ( (c.y >= -89.0) && (c.y < 89.0) ) {
	    min.x = c.x - span / 2.0;
	    max.x = c.x + span / 2.0;
	    min.y = c.y - FG_HALF_BUCKET_SPAN;
	    max.y = c.y + FG_HALF_BUCKET_SPAN;
	} else if ( c.y < -89.0) {
	    min.x = -90.0;
	    max.x = -89.0;
	    min.y = -180.0;
	    max.y = 180.0;
	} else if ( c.y >= 89.0) {
	    min.x = 89.0;
	    max.x = 90.0;
	    min.y = -180.0;
	    max.y = 180.0;
	} else {
	    FG_LOG ( FG_GENERAL, FG_ALERT, 
		     "Out of range latitude in clip_and_write_poly() = " 
		     << c.y );
	}

	if ( min.x < global_min.x ) global_min.x = min.x;
	if ( min.y < global_min.y ) global_min.y = min.y;
	if ( max.x > global_max.x ) global_max.x = max.x;
	if ( max.y > global_max.y ) global_max.y = max.y;

	// finally, load the polygon(s) from this file
	clipper.load_polys( full_path );
    }

    // do the clipping
    clipper.clip_all(global_min, global_max);

    FG_LOG( FG_CLIPPER, FG_INFO, "finished main" );

    return 0;
}

// $Log$
// Revision 1.1  1999/03/13 23:51:36  curt
// Renamed main.cxx to testclipper.cxx
// Converted clipper routines to a class FGClipper.
//
// Revision 1.1  1999/03/01 15:39:39  curt
// Initial revision.
//
