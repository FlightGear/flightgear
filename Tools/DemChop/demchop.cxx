// demchop.cxx -- chop up a dem file into it's corresponding pieces and stuff
//                them into the workspace directory
//
// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@flightgear.org
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


#include <Include/compiler.h>

#include STL_STRING

#include <Debug/logstream.hxx>
#include <Bucket/newbucket.hxx>
#include <DEM/dem.hxx>

#include "point2d.hxx"

FG_USING_STD(string);


int main(int argc, char **argv) {
    /*
    fgDEM dem;
    FGBucket p;
    string fg_root;
    string filename;
    double error;
    int i, j;
    */

    fglog().setLogLevels( FG_ALL, FG_DEBUG );

    if ( argc != 3 ) {
	FG_LOG( FG_GENERAL, FG_ALERT, 
		"Usage " << argv[0] << " <dem_file> <work_dir>" );
	exit(-1);
    }

    string dem_name = argv[1];
    string work_dir = argv[2];
    string command = "mkdir -p " + work_dir;
    system( command.c_str() );

    FGDem dem(dem_name);
    dem.parse();
    dem.close();

    point2d min, max;
    min.x = dem.get_originx() / 3600.0 + FG_HALF_BUCKET_SPAN;
    min.y = dem.get_originy() / 3600.0 + FG_HALF_BUCKET_SPAN;
    FGBucket b_min( min.x, min.y );

    max.x = (dem.get_originx() + dem.get_cols() * dem.get_col_step()) / 3600.0 
	- FG_HALF_BUCKET_SPAN;
    max.y = (dem.get_originy() + dem.get_rows() * dem.get_row_step()) / 3600.0 
	- FG_HALF_BUCKET_SPAN;
    FGBucket b_max( max.x, max.y );

    if ( b_min == b_max ) {
	dem.write_area( work_dir, b_min, true );
    } else {
	FGBucket b_cur;
	int dx, dy, i, j;

	fgBucketDiff(b_min, b_max, &dx, &dy);
	cout << "DEM file spans tile boundaries" << endl;
	cout << "  dx = " << dx << "  dy = " << dy << endl;

	if ( (dx > 20) || (dy > 20) ) {
	    cout << "somethings really wrong!!!!" << endl;
	    exit(-1);
	}

	for ( j = 0; j <= dy; j++ ) {
	    for ( i = 0; i <= dx; i++ ) {
		b_cur = fgBucketOffset(min.x, min.y, i, j);
		dem.write_area( work_dir, b_cur, true );
	    }
	}
    }

    return 0;
}


// $Log$
// Revision 1.3  1999/03/12 22:53:46  curt
// First working version!
//
// Revision 1.2  1999/03/10 16:09:44  curt
// Hacking towards the first working version.
//
// Revision 1.1  1999/03/10 01:02:54  curt
// Initial revision.
//
