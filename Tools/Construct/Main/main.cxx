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
#include <Math/mat3.h>

#include <Debug/logstream.hxx>
#include <Array/array.hxx>
#include <Clipper/clipper.hxx>
#include <GenOutput/genobj.hxx>
#include <Match/match.hxx>
#include <Triangulate/triangle.hxx>

#include "construct.hxx"


// do actual scan of directory and loading of files
int actual_load_polys( const string& dir, FGConstruct& c, FGClipper& clipper ) {
    int counter = 0;
    string base = c.get_bucket().gen_base_path();
    string tile_str = c.get_bucket().gen_index_str();
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
int load_polys( FGConstruct& c ) {
    FGClipper clipper;

    string base = c.get_bucket().gen_base_path();
    int result;

    // initialize clipper
    clipper.init();

    // load airports
    string poly_path = c.get_work_base() + ".apt" + "/Scenery/" + base;
    cout << "poly_path = " << poly_path << endl;
    result = actual_load_polys( poly_path, c, clipper );
    cout << "  loaded " << result << " polys" << endl;

    // load hydro
    poly_path = c.get_work_base() + ".hydro" + "/Scenery/" + base;
    cout << "poly_path = " << poly_path << endl;
    result = actual_load_polys( poly_path, c, clipper );
    cout << "  loaded " << result << " polys" << endl;

    point2d min, max;
    min.x = c.get_bucket().get_center_lon() - 0.5 * c.get_bucket().get_width();
    min.y = c.get_bucket().get_center_lat() - 0.5 * c.get_bucket().get_height();
    max.x = c.get_bucket().get_center_lon() + 0.5 * c.get_bucket().get_width();
    max.y = c.get_bucket().get_center_lat() + 0.5 * c.get_bucket().get_height();

    // do clipping
    cout << "clipping polygons" << endl;
    clipper.clip_all(min, max);

    // update main data repository
    c.set_clipped_polys( clipper.get_polys_clipped() );
    return 1;
}


// load regular grid of elevation data (dem based), return list of
// fitted nodes
int load_dem( FGConstruct& c, FGArray& array) {
    point_list result;
    string base = c.get_bucket().gen_base_path();

    string dem_path = c.get_work_base() + ".dem" + "/Scenery/" + base 
	+ "/" + c.get_bucket().gen_index_str() + ".dem";
    cout << "dem_path = " << dem_path << endl;

    if ( ! array.open(dem_path) ) {
	cout << "ERROR: cannot open " << dem_path << endl;
    }

    FGBucket b = c.get_bucket();
    array.parse( b );

    return 1;
}


// fit dem nodes, return number of fitted nodes
int fit_dem(FGArray& array, int error) {
    return array.fit( error );
}


// triangulate the data for each polygon ( first time before splitting )
void first_triangulate( FGConstruct& c, const FGArray& array,
		     FGTriangle& t ) {
    // first we need to consolidate the points of the DEM fit list and
    // all the polygons into a more "Triangle" friendly format

    point_list corner_list = array.get_corner_node_list();
    point_list fit_list = array.get_fit_node_list();
    FGgpcPolyList gpc_polys = c.get_clipped_polys();

    cout << "ready to build node list and polygons" << endl;
    t.build( corner_list, fit_list, gpc_polys );
    cout << "done building node list and polygons" << endl;

    cout << "ready to do triangulation" << endl;
    t.run_triangulate( 1 );
    cout << "finished triangulation" << endl;
}


// triangulate the data for each polygon ( second time after splitting
// and reassembling )
void second_triangulate( FGConstruct& c, FGTriangle& t ) {
    t.rebuild( c );
    cout << "done re building node list and polygons" << endl;

    cout << "ready to do second triangulation" << endl;
    t.run_triangulate( 2 );
    cout << "finished second triangulation" << endl;
}


// build the wgs-84 point list (and fix the elevations of the geodetic
// nodes)
static void fix_point_heights( FGConstruct& c, const FGArray& array ) {
    point_list geod_nodes;
    point_list wgs84_nodes;

    cout << "fixing node heights and generating wgs84 list" << endl;
    Point3D geod, radians, cart;

    point_list raw_nodes = c.get_tri_nodes().get_node_list();
    point_list_iterator current = raw_nodes.begin();
    point_list_iterator last = raw_nodes.end();

    for ( ; current != last; ++current ) {
	geod = *current;

	geod.setz( array.interpolate_altitude( geod.x() * 3600.0, 
					       geod.y() * 3600.0 ) );

	// convert to radians
	radians = Point3D( geod.x() * DEG_TO_RAD,
			   geod.y() * DEG_TO_RAD,
			   geod.z() );

        cart = fgGeodToCart(radians);
	// cout << cart << endl;

	geod_nodes.push_back(geod);
        wgs84_nodes.push_back(cart);
    }

    c.set_geod_nodes( geod_nodes );
    c.set_wgs84_nodes( wgs84_nodes );
}


// build the node -> element (triangle) reverse lookup table.  there
// is an entry for each point containing a list of all the triangles
// that share that point.
static belongs_to_list gen_node_ele_lookup_table( FGConstruct& c ) {
    belongs_to_list reverse_ele_lookup;
    reverse_ele_lookup.clear();

    int_list ele_list;
    ele_list.clear();

    // initialize reverse_ele_lookup structure by creating an empty
    // list for each point
    point_list wgs84_nodes = c.get_wgs84_nodes();
    const_point_list_iterator w_current = wgs84_nodes.begin();
    const_point_list_iterator w_last = wgs84_nodes.end();
    for ( ; w_current != w_last; ++w_current ) {
	reverse_ele_lookup.push_back( ele_list );
    }

    // traverse triangle structure building reverse lookup table
    triele_list tri_elements = c.get_tri_elements();
    const_triele_list_iterator current = tri_elements.begin();
    const_triele_list_iterator last = tri_elements.end();
    int counter = 0;
    for ( ; current != last; ++current ) {
	reverse_ele_lookup[ current->get_n1() ].push_back( counter );
	reverse_ele_lookup[ current->get_n2() ].push_back( counter );
	reverse_ele_lookup[ current->get_n3() ].push_back( counter );
	++counter;
    }

    return reverse_ele_lookup;
}


// caclulate the normal for the specified triangle face
static Point3D calc_normal( FGConstruct& c, int i ) {
    double v1[3], v2[3], normal[3];
    double temp;

    point_list wgs84_nodes = c.get_wgs84_nodes();
    triele_list tri_elements = c.get_tri_elements();

    Point3D p1 = wgs84_nodes[ tri_elements[i].get_n1() ];
    Point3D p2 = wgs84_nodes[ tri_elements[i].get_n2() ];
    Point3D p3 = wgs84_nodes[ tri_elements[i].get_n3() ];

    v1[0] = p2.x() - p1.x(); v1[1] = p2.y() - p1.y(); v1[2] = p2.z() - p1.z();
    v2[0] = p3.x() - p1.x(); v2[1] = p3.y() - p1.y(); v2[2] = p3.z() - p1.z();

    MAT3cross_product(normal, v1, v2);
    MAT3_NORMALIZE_VEC(normal,temp);

    return Point3D( normal[0], normal[1], normal[2] );
}


// build the face normal list
static point_list gen_face_normals( FGConstruct& c ) {
    point_list face_normals;

    // traverse triangle structure building the face normal table

    cout << "calculating face normals" << endl;

    triele_list tri_elements = c.get_tri_elements();
    for ( int i = 0; i < (int)tri_elements.size(); i++ ) {
	// cout << calc_normal( i ) << endl;
	face_normals.push_back( calc_normal( c, i ) );
    }

    return face_normals;
}


// calculate the normals for each point in wgs84_nodes
static point_list gen_point_normals( FGConstruct& c ) {
    point_list point_normals;

    Point3D normal;
    cout << "caculating node normals" << endl;

    point_list wgs84_nodes = c.get_wgs84_nodes();
    belongs_to_list reverse_ele_lookup = c.get_reverse_ele_lookup();
    point_list face_normals = c.get_face_normals();

    // for each node
    for ( int i = 0; i < (int)wgs84_nodes.size(); ++i ) {
	int_list tri_list = reverse_ele_lookup[i];

	int_list_iterator current = tri_list.begin();
	int_list_iterator last = tri_list.end();

	Point3D average( 0.0 );

	// for each triangle that shares this node
	for ( ; current != last; ++current ) {
	    normal = face_normals[ *current ];
	    average += normal;
	    // cout << normal << endl;
	}

	average /= tri_list.size();
	// cout << "average = " << average << endl;

	point_normals.push_back( average );
    }

    return point_normals;
}


// generate the flight gear scenery file
void do_output( FGConstruct& c, FGGenOutput& output ) {
    output.build( c );
    output.write( c );
}


// master construction routine
void construct_tile( FGConstruct& c ) {
    cout << "Construct tile, bucket = " << c.get_bucket() << endl;

    // fit with ever increasing error tolerance until we produce <=
    // 80% of max nodes.  We should really have the sim end handle
    // arbitrarily complex tiles.

    bool acceptable = false;
    double error = 200.0;
    int count = 0;

    // load and clip 2d polygon data
    load_polys( c );

    // load grid of elevation data (dem)
    FGArray array;
    load_dem( c, array );

    FGTriangle t;

    while ( ! acceptable ) {
	// do a least squares fit of the (dem) data with the given
	// error tolerance
	array.fit( error );

	// triangulate the data for each polygon
	first_triangulate( c, array, t );

	acceptable = true;

	count = t.get_out_nodes_size();

	if ( (count < c.get_min_nodes()) && (error >= 25.0) ) {
	    // reduce error tolerance until number of points exceeds the
	    // minimum threshold
	    cout << "produced too few nodes ..." << endl;

	    acceptable = false;

	    error /= 1.5;
	    cout << "Setting error to " << error << " and retrying fit." 
		 << endl;
	}

	if ( (count > c.get_max_nodes()) && (error <= 1000.0) ) {
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

    // save the results of the triangulation
    c.set_tri_nodes( t.get_out_nodes() );
    c.set_tri_elements( t.get_elelist() );
    c.set_tri_segs( t.get_out_segs() );

    // calculate wgs84 (cartesian) form of node list
    fix_point_heights( c, array );
    
    // build the node -> element (triangle) reverse lookup table
    c.set_reverse_ele_lookup( gen_node_ele_lookup_table( c ) );

    // build the face normal list
    c.set_face_normals( gen_face_normals( c ) );

    // calculate the normals for each point in wgs84_nodes
    c.set_point_normals( gen_point_normals( c ) );

    // match tile edges with any neighbor tiles that have already been
    // generated
    FGMatch m;
    m.load_neighbor_shared( c );
    m.split_tile( c );
    m.write_shared( c );
    m.assemble_tile( c );

    // now we must retriangulate the pasted together tile points
    second_triangulate( c, t );

    // save the results of the triangulation
    c.set_tri_nodes( t.get_out_nodes() );
    c.set_tri_elements( t.get_elelist() );
    c.set_tri_segs( t.get_out_segs() );

    // calculate wgs84 (cartesian) form of node list
    fix_point_heights( c, array );

    // generate the output
    FGGenOutput output;
    do_output( c, output );
}


// display usage and exit
void usage( const string name ) {
    cout << "Usage: " << name
	 << " <work_base> <output_base> tile_id" << endl;
    cout << "Usage: " << name
	 << " <work_base> <output_base> center_lon center_lat xdist ydist"
	 << endl;
    exit(-1);
}


main(int argc, char **argv) {
    double lon, lat;

    fglog().setLogLevels( FG_ALL, FG_DEBUG );

    if ( argc < 3 ) {
	usage( argv[0] );
    }

    // main construction data management class
    FGConstruct c;

    c.set_work_base( argv[1] );
    c.set_output_base( argv[2] );

    c.set_min_nodes( 50 );
    c.set_max_nodes( (int)(FG_MAX_NODES * 0.8) );

    // lon = -146.248360; lat = 61.133950;     // PAVD (Valdez, AK)
    // lon = -110.664244; lat = 33.352890;     // P13
    // lon = -93.211389; lat = 45.145000;      // KANE
    // lon = -92.486188; lat = 44.590190;      // KRGK
    // lon = -89.7446823; lat= 29.314495;
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
    // lon = -76.201239; lat = 36.894606;      // KORF (Norfolk, Virginia)
    // lon = -147.166; lat = 60.9925;          // Hale-bop test

    if ( argc == 4 ) {
	// construct a specific tile and exit

	long index = atoi( argv[3] );
	FGBucket b( index );
	c.set_bucket( b );
	construct_tile( c );
    } else if ( argc == 7 ) {
	// build all the tiles in an area

	lon = atof( argv[3] );
	lat = atof( argv[4] );
	double xdist = atof( argv[5] );
	double ydist = atof( argv[6] );

	double min_x = lon - xdist;
	double min_y = lat - ydist;
	FGBucket b_min( min_x, min_y );
	FGBucket b_max( lon + xdist, lat + ydist );

	FGBucket b_start(550401L);
	bool do_tile = true;

	if ( b_min == b_max ) {
	    c.set_bucket( b_min );
	    construct_tile( c );
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
			c.set_bucket( b_cur );
			construct_tile( c );
		    } else {
			cout << "skipping " << b_cur << endl;
		    }
		}
	    }
	    // string answer; cin >> answer;
	}
    } else {
	usage( argv[0] );
    }

    cout << "[Finished successfully]" << endl;
}


