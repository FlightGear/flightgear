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


#include <sys/types.h>  // for directory reading
#include <dirent.h>     // for directory reading

#include <Bucket/newbucket.hxx>
#include <Include/fg_constants.h>

#include <Debug/logstream.hxx>
#include <Array/array.hxx>
#include <Clipper/clipper.hxx>
#include <GenOutput/genobj.hxx>
#include <Triangulate/triangle.hxx>


// load regular grid of elevation data (dem based), return list of
// fitted nodes
int load_dem(const string& work_base, FGBucket& b, FGArray& array) {
    point_list result;
    string base = b.gen_base_path();

    string dem_path = work_base + ".dem" + "/Scenery/" + base 
	+ "/" + b.gen_index_str() + ".dem";
    cout << "dem_path = " << dem_path << endl;

    if ( ! array.open(dem_path) ) {
	cout << "ERROR: cannot open " << dem_path << endl;
    }

    array.parse( b );

    return 1;
}

// fit dem nodes, return number of fitted nodes
int fit_dem(FGArray& array, int error) {
    return array.fit( error );
}


// do actual scan of directory and loading of files
int actual_load_polys( const string& dir, FGBucket& b, FGClipper& clipper ) {
    int counter = 0;
    string base = b.gen_base_path();
    string tile_str = b.gen_index_str();
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

    point_list corner_list = array.get_corner_node_list();
    point_list fit_list = array.get_fit_node_list();
    FGgpcPolyList gpc_polys = clipper.get_polys_clipped();

    cout << "ready to build node list and polygons" << endl;
    t.build( corner_list, fit_list, gpc_polys );
    cout << "done building node list and polygons" << endl;

    cout << "ready to do triangulation" << endl;
    t.run_triangulate();
    cout << "finished triangulation" << endl;
}


// generate the flight gear scenery file
void do_output( const string& base, const FGBucket &b, const FGTriangle& t, 
		const FGArray& array, FGGenOutput& output ) {
    output.build( array, t );
    output.write( base, b );
}


void construct_tile( const string& work_base, const string& output_base,
		     FGBucket& b )
{
    cout << "Construct tile, bucket = " << b << endl;

    // fit with ever increasing error tolerance until we produce <=
    // 80% of max nodes.  We should really have the sim end handle
    // arbitrarily complex tiles.

    const int min_nodes = 50;
    const int max_nodes = (int)(MAX_NODES * 0.8);

    bool acceptable = false;
    double error = 200.0;
    int count = 0;

    // load and clip 2d polygon data
    FGClipper clipper;
    load_polys( work_base, b, clipper );

    // load grid of elevation data (dem)
    FGArray array;
    load_dem( work_base, b, array );

    FGTriangle t;

    while ( ! acceptable ) {
	// do a least squares fit of the (dem) data with the given
	// error tolerance
	array.fit( error );

	// triangulate the data for each polygon
	do_triangulate( array, clipper, t );

	acceptable = true;

	count = t.get_out_nodes_size();

	if ( (count < min_nodes) && (error >= 25.0) ) {
	    // reduce error tolerance until number of points exceeds the
	    // minimum threshold
	    cout << "produced too few nodes ..." << endl;

	    acceptable = false;

	    error /= 1.5;
	    cout << "Setting error to " << error << " and retrying fit." 
		 << endl;
	}

	if ( (count > max_nodes) && (error <= 1000.0) ) {
	    // increase error tolerance until number of points drops below
	    // the maximum threshold
	    cout << "produced too many nodes ..." << endl;
	    
	    acceptable = false;

	    error *= 1.5;
	    cout << "Setting error to " << error << " and retrying fit." 
		 << endl;
	}
    }

    cout << "finished fit with error = " << error << " node count = " 
	 << count << endl;

    // generate the output
    FGGenOutput output;
    do_output( output_base, b, t, array, output );
}


main(int argc, char **argv) {
    double lon, lat;

    fglog().setLogLevels( FG_ALL, FG_DEBUG );

    if ( argc != 3 ) {
	cout << "Usage: " << argv[0] << " <work_base> <output_base>" << endl;
	exit(-1);
    }

    string work_base = argv[1];
    string output_base = argv[2];
   
    // lon = -146.248360; lat = 61.133950;     // PAVD (Valdez, AK)
    // lon = -110.664244; lat = 33.352890;     // P13
    // lon = -93.211389; lat = 45.145000;      // KANE
    // lon = -92.486188; lat = 44.590190;      // KRGK
    // lon = -89.744682312011719; lat= 29.314495086669922;
    // lon = -122.488090; lat = 42.743183;     // 64S
    // lon = -114.861097; lat = 35.947480;     // 61B
    // lon = -112.012175; lat = 41.195944;     // KOGD
    // lon = -90.757128; lat = 46.790212;      // WI32
    // lon = -122.220717; lat = 37.721291;     // KOAK
    // lon = -111.721477; lat = 40.215641;     // KPVU
    // lon = -122.309313; lat = 47.448982;     // KSEA
    // lon = -148.798131; lat = 63.645099;     // AK06 (Danali, AK)
    // lon = -92.5; lat = 47.5;                // Marsh test (northern MN)
    // lon = -111.977773; lat = 40.788388;     // KSLC
    // lon = -121.914; lat = 42.5655;          // TEST (Oregon SW of Crater)
    lon = -76.201239; lat = 36.894606;      // KORF (Norfolk, Virginia)

    double min_x = lon - 3;
    double min_y = lat - 1;
    FGBucket b_min( min_x, min_y );
    FGBucket b_max( lon + 3, lat + 1 );

    FGBucket b_start(1662962L);
    bool do_tile = false;

    // FGBucket b_omit(-1L);
    // FGBucket b(1122504L);
    FGBucket b(-146.248360, 61.133950);
    construct_tile( work_base, output_base, b );
    exit(0);

    if ( b_min == b_max ) {
	construct_tile( work_base, output_base, b_min );
    } else {
	FGBucket b_cur;
	int dx, dy, i, j;
	    
	fgBucketDiff(b_min, b_max, &dx, &dy);
	cout << "  construction area spans tile boundaries" << endl;
	cout << "  dx = " << dx << "  dy = " << dy << endl;

	for ( j = 0; j <= dy; j++ ) {
	    for ( i = 0; i <= dx; i++ ) {
		b_cur = fgBucketOffset(min_x, min_y, i, j);

		if ( b_cur == b_start ) {
		   do_tile = true;
		}

		if ( do_tile ) {
		    construct_tile( work_base, output_base, b_cur );
		} else {
		    cout << "skipping " << b_cur << endl;
		}
	    }
	}
	// string answer; cin >> answer;
    }
}


