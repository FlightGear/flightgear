// main.cxx -- top level construction routines
//
// Written by Curtis Olson, started March 1999.
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
// (Log is kept at end of this file)


#include <sys/types.h>  // for directory reading
#include <dirent.h>     // for directory reading

#include <Bucket/newbucket.hxx>

#include <Array/array.hxx>
#include <Clipper/clipper.hxx>


// load regular grid of elevation data (dem based)
int load_dem(const string& work_base, FGBucket& b, FGArray& array) {
    char tile_name[256];
    string base = b.gen_base_path();
    long int b_index = b.gen_index();
    sprintf(tile_name, "%ld", b_index);

    string dem_path = work_base + ".dem" + "/Scenery/" + base 
	+ "/" + tile_name + ".dem";
    cout << "dem_path = " << dem_path << endl;

    if ( ! array.open(dem_path) ) {
	return 0;
    }
    array.parse();
    array.fit( 100 );

    return 1;
}


// load all 2d polygons matching the specified base path and clip
// against each other to resolve any overlaps
int load_polys( const string& work_base, FGBucket& b, FGClipper& clipper) {
    char tile_char[256];
    string base = b.gen_base_path();
    long int b_index = b.gen_index();
    sprintf(tile_char, "%ld", b_index);
    string tile_str = tile_char;

    string poly_path = work_base + ".shapes" + "/Scenery/" + base;
    cout << "poly_path = " << poly_path << endl;

    DIR *d;
    struct dirent *de;

    if ( (d = opendir( poly_path.c_str() )) == NULL ) {
        cout << "cannot open directory " << poly_path << "\n";
	return 0;
    }

    // initialize clipper
    clipper.init();

    // load all matching polygon files
    string file, f_index, full_path;
    int pos;
    while ( (de = readdir(d)) != NULL ) {
	file = de->d_name;
	pos = file.find(".");
	f_index = file.substr(0, pos);

	if ( tile_str == f_index ) {
	    cout << file << "  " << f_index << endl;
	    full_path = poly_path + "/" + file;
	    clipper.load_polys( full_path );
	}
    }

    point2d min, max;
    min.x = b.get_center_lon() - 0.5 * b.get_width();
    min.y = b.get_center_lat() - 0.5 * b.get_height();
    max.x = b.get_center_lon() + 0.5 * b.get_width();
    max.y = b.get_center_lat() + 0.5 * b.get_height();

    // do clipping
    clipper.clip_all(min, max);

    return 1;
}


main(int argc, char **argv) {
    double lon, lat;

    if ( argc != 2 ) {
	cout << "Usage: " << argv[0] << " work_base" << endl;
	exit(-1);
    }

    string work_base = argv[1];
   
    lon = -146.248360; lat = 61.133950;  // PAVD (Valdez, AK)
    // lon = -110.664244; lat = 33.352890;  // P13
    FGBucket b( lon, lat );

    // load and fit grid of elevation data
    FGArray array;
    load_dem( work_base, b, array );

    // load and clip 2d polygon data
    FGClipper clipper;
    load_polys( work_base, b, clipper );
}


// $Log$
// Revision 1.1  1999/03/14 00:03:24  curt
// Initial revision.
//


