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
#include <GenOutput/genobj.hxx>
#include <Triangulate/triangle.hxx>


// load regular grid of elevation data (dem based), return list of
// fitted nodes
int load_dem(const string& work_base, FGBucket& b, FGArray& array) {
    fitnode_list result;
    char tile_name[256];
    string base = b.gen_base_path();
    long int b_index = b.gen_index();
    sprintf(tile_name, "%ld", b_index);

    string dem_path = work_base + ".dem" + "/Scenery/" + base 
	+ "/" + tile_name + ".dem";
    cout << "dem_path = " << dem_path << endl;

    if ( ! array.open(dem_path) ) {
	cout << "ERROR: cannot open " << dem_path << endl;
	return 0;
    }

    array.parse();
    array.fit( 100 );

    return 1;
}


// do actual scan of directory and loading of files
int actual_load_polys( const string& dir, FGBucket& b, FGClipper& clipper ) {
    int counter = 0;
    char tile_char[256];
    string base = b.gen_base_path();
    long int b_index = b.gen_index();
    sprintf(tile_char, "%ld", b_index);
    string tile_str = tile_char;
    string ext;

    DIR *d;
    struct dirent *de;

    if ( (d = opendir( dir.c_str() )) == NULL ) {
        cout << "cannot open directory " << dir << "\n";
	return 0;
    }

    // load all matching polygon files
    string file, f_index, full_path;
    int pos;
    while ( (de = readdir(d)) != NULL ) {
	file = de->d_name;
	pos = file.find(".");
	f_index = file.substr(0, pos);

	if ( tile_str == f_index ) {
	    ext = file.substr(pos + 1);
	    cout << file << "  " << f_index << "  '" << ext << "'" << endl;
	    full_path = dir + "/" + file;
	    if ( (ext == "dem") || (ext == "dem.gz") ) {
		// skip
	    } else {
		cout << "ext = '" << ext << "'" << endl;
		clipper.load_polys( full_path );
		++counter;
	    }
	}
    }

    return counter;
}


// load all 2d polygons matching the specified base path and clip
// against each other to resolve any overlaps
int load_polys( const string& work_base, FGBucket& b, FGClipper& clipper) {
    string base = b.gen_base_path();
    int result;

    // initialize clipper
    clipper.init();

    // load airports
    string poly_path = work_base + ".apt" + "/Scenery/" + base;
    cout << "poly_path = " << poly_path << endl;
    result = actual_load_polys( poly_path, b, clipper );
    cout << "  loaded " << result << " polys" << endl;

    // load hydro
    poly_path = work_base + ".hydro" + "/Scenery/" + base;
    cout << "poly_path = " << poly_path << endl;
    result = actual_load_polys( poly_path, b, clipper );
    cout << "  loaded " << result << " polys" << endl;

    point2d min, max;
    min.x = b.get_center_lon() - 0.5 * b.get_width();
    min.y = b.get_center_lat() - 0.5 * b.get_height();
    max.x = b.get_center_lon() + 0.5 * b.get_width();
    max.y = b.get_center_lat() + 0.5 * b.get_height();

    // do clipping
    cout << "clipping polygons" << endl;
    clipper.clip_all(min, max);

    return 1;
}


// triangulate the data for each polygon
void do_triangulate( const FGArray& array, const FGClipper& clipper,
		  FGTriangle& t ) {
    // first we need to consolidate the points of the DEM fit list and
    // all the polygons into a more "Triangle" friendly format

    fitnode_list fit_list = array.get_fit_node_list();
    FGgpcPolyList gpc_polys = clipper.get_polys_clipped();

    cout << "ready to build node list and polygons" << endl;
    t.build( fit_list, gpc_polys );
    cout << "done building node list and polygons" << endl;

    cout << "ready to do triangulation" << endl;
    t.run_triangulate();
    cout << "finished triangulation" << endl;
}


// generate the flight gear scenery file
void do_output( const FGTriangle& t, FGGenOutput& output ) {
    output.build( t );
    output.write( "output" );
}


main(int argc, char **argv) {
    fitnode_list fit_list;
    double lon, lat;

    if ( argc != 2 ) {
	cout << "Usage: " << argv[0] << " work_base" << endl;
	exit(-1);
    }

    string work_base = argv[1];
   
    // lon = -146.248360; lat = 61.133950;     // PAVD (Valdez, AK)
    // lon = -110.664244; lat = 33.352890;     // P13
    // lon = -93.211389; lat = 45.145000;      // KANE
    // lon = -92.486188; lat = 44.590190;      // KRGK
    lon = -89.744682312011719; lat= 29.314495086669922;

    FGBucket b( lon, lat );

    // load and fit grid of elevation data
    FGArray array;
    load_dem( work_base, b, array );

    // load and clip 2d polygon data
    FGClipper clipper;
    load_polys( work_base, b, clipper );

    // triangulate the data for each polygon
    FGTriangle t;
    do_triangulate( array, clipper, t );

    // generate the output
    FGGenOutput output;
    do_output( t, output );
}


// $Log$
// Revision 1.8  1999/03/23 22:02:17  curt
// Worked on creating data to output ... normals, bounding spheres, etc.
//
// Revision 1.7  1999/03/22 23:48:29  curt
// Added GenOutput/
//
// Revision 1.6  1999/03/21 15:48:01  curt
// Removed Dem2node from the Tools fold.
// Tweaked the triangulator options to add quality mesh refinement.
//
// Revision 1.5  1999/03/21 14:02:05  curt
// Added a mechanism to dump out the triangle structures for viewing.
// Fixed a couple bugs in first pass at triangulation.
// - needed to explicitely initialize the polygon accumulator in triangle.cxx
//   before each polygon rather than depending on the default behavior.
// - Fixed a problem with region attribute propagation where I wasn't generating
//   the hole points correctly.
//
// Revision 1.4  1999/03/20 20:32:54  curt
// First mostly successful tile triangulation works.  There's plenty of tweaking
// to do, but we are marching in the right direction.
//
// Revision 1.3  1999/03/19 00:26:52  curt
// Minor tweaks ...
//
// Revision 1.2  1999/03/17 23:49:52  curt
// Started work on Triangulate/ section.
//
// Revision 1.1  1999/03/14 00:03:24  curt
// Initial revision.
//


