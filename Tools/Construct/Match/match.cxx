// array.cxx -- Array management class
//
// Written by Curtis Olson, started March 1998.
//
// Copyright (C) 1998 - 1999  Curtis L. Olson  - curt@flightgear.org
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Misc/fgstream.hxx>

#include "match.hxx"



FGMatch::FGMatch( void ) {
}


FGMatch::~FGMatch( void ) {
}


// scan the specified share file for the specified information
void FGMatch::scan_share_file( const string& dir, const FGBucket& b, 
			     neighbor_type n )
{
    string file = dir + "/" + b.gen_index_str();
    cout << "reading shared data from " << file << endl;

    fg_gzifstream in( file );
    if ( !in.is_open() ) {
        cout << "Cannot open file: " << file << endl;
	return;
    }

    string target;
    if ( n == SW_Corner ) {
	target = "sw_node";
    } else if ( n == SE_Corner ) {
	target = "se_node";
    } else if ( n == NE_Corner ) {
	target = "ne_node";
    } else if ( n == NW_Corner ) {
	target = "nw_node";
    }

    string key;
    Point3D node, normal;
    while ( in ) {
	in >> key;
	in >> node;
	if ( key == target ) {
	    cout << key << " " << node << endl;
	    in >> key;
	    in >> normal;

	    if ( n == SW_Corner ) {
		sw_node = node;
		sw_normal = normal;
		sw_flag = true;
	    } else if ( n == SE_Corner ) {
		se_node = node;
		se_normal = normal;
		se_flag = true;
	    } else if ( n == NE_Corner ) {
		ne_node = node;
		ne_normal = normal;
		ne_flag = true;
	    } else if ( n == NW_Corner ) {
		nw_node = node;
		nw_normal = normal;
		nw_flag = true;
	    } else if ( n == NORTH ) {
		north_nodes.push_back(node);
		north_normals.push_back(normal);
		north_flat = true;
	    } else if ( n == SOUTH ) {
		south_nodes.push_back(node);
		south_normals.push_back(normal);
		south_flat = true;
	    } else if ( n == EAST ) {
		east_nodes.push_back(node);
		east_normals.push_back(normal);
		east_flat = true;
	    } else if ( n == WEST ) {
		west_nodes.push_back(node);
		west_normals.push_back(normal);
		west_flat = true;
	    }
	}
    }
}


// try to find info for the specified shared component
void FGMatch::load_shared( const FGConstruct& c, neighbor_type n ) {
    FGBucket b = c.get_bucket();

    double clon = b.get_center_lon();
    double clat = b.get_center_lat();

    string base = c.get_work_base() + ".shared/Scenery/" + b.gen_base_path();

    FGBucket cb;

    // test
    scan_share_file( base, b, SE_Corner );

    if ( n == SW_Corner ) {
	cb = fgBucketOffset(clon, clat, -1, 0);
	scan_share_file( base, cb, SE_Corner );
	cb = fgBucketOffset(clon, clat, -1, -1);
	scan_share_file( base, cb, NE_Corner );
	cb = fgBucketOffset(clon, clat, 0, -1);
	scan_share_file( base, cb, NW_Corner );
    } else if ( n == SE_Corner ) {
	cb = fgBucketOffset(clon, clat, 0, -1);
	scan_share_file( base, cb, NE_Corner );
	cb = fgBucketOffset(clon, clat, 1, -1);
	scan_share_file( base, cb, NW_Corner );
	cb = fgBucketOffset(clon, clat, 1, 0);
	scan_share_file( base, cb, SW_Corner );
    } else if ( n == NE_Corner ) {
	cb = fgBucketOffset(clon, clat, 1, 0);
	scan_share_file( base, cb, NW_Corner );
	cb = fgBucketOffset(clon, clat, 1, 1);
	scan_share_file( base, cb, SW_Corner );
	cb = fgBucketOffset(clon, clat, 0, 1);
	scan_share_file( base, cb, SE_Corner );
    } else if ( n == NW_Corner ) {
	cb = fgBucketOffset(clon, clat, 0, 1);
	scan_share_file( base, cb, SW_Corner );
	cb = fgBucketOffset(clon, clat, -1, 1);
	scan_share_file( base, cb, SE_Corner );
	cb = fgBucketOffset(clon, clat, -1, 0);
	scan_share_file( base, cb, NE_Corner );
    } else if ( n == NORTH ) {
 	cb = fgBucketOffset(clon, clat, 0, 1);
	scan_share_file( base, cb, SOUTH );
    } else if ( n == SOUTH ) {
 	cb = fgBucketOffset(clon, clat, 0, 1);
	scan_share_file( base, cb, NORTH );
    } else if ( n == EAST ) {
 	cb = fgBucketOffset(clon, clat, 0, 1);
	scan_share_file( base, cb, WEST );
    } else if ( n == WEST ) {
 	cb = fgBucketOffset(clon, clat, 0, 1);
	scan_share_file( base, cb, EAST );
    }
}


// load any previously existing shared data from all neighbors
void FGMatch::load_neighbor_shared( FGConstruct& c ) {
    cout << "Loading existing shared data from neighbor tiles" << endl;

    sw_flag = se_flag = ne_flag = nw_flag = false;
    north_flag = south_flag = east_flag = west_flag = false;

    load_shared( c, SW_Corner );
    load_shared( c, SE_Corner );
    load_shared( c, NE_Corner );
    load_shared( c, NW_Corner );

    load_shared( c, NORTH );
    load_shared( c, SOUTH );
    load_shared( c, EAST );
    load_shared( c, WEST );
}


// split up the tile between the shared edge points, normals, and
// segments and the body.  This must be done after calling
// load_neighbor_data() and will ignore any shared data from the
// current tile that already exists from a neighbor.
void FGMatch::split_tile( FGConstruct& c ) {

    cout << "Extracting (shared) edge nodes and normals" << endl;

    // calculate tile boundaries
    point2d min, max;
    FGBucket b = c.get_bucket();
    min.x = b.get_center_lon() - 0.5 * b.get_width();
    min.y = b.get_center_lat() - 0.5 * b.get_height();
    max.x = b.get_center_lon() + 0.5 * b.get_width();
    max.y = b.get_center_lat() + 0.5 * b.get_height();

    // separate nodes and normals into components

    north_nodes.clear();
    south_nodes.clear();
    east_nodes.clear();
    west_nodes.clear();
    body_nodes.clear();

    point_list nodes = c.get_geod_nodes();
    point_list point_normals = c.get_point_normals();

    for ( int i = 0; i < (int)nodes.size(); ++i ) {
	Point3D node = nodes[i];
	Point3D normal = point_normals[i];

	if ( (fabs(node.y() - min.y) < FG_EPSILON) && 
	     (fabs(node.x() - min.x) < FG_EPSILON) ) {
	    if ( ! sw_flag ) {
		sw_node = node;
		sw_normal = normal;
	    }
	} else if ( (fabs(node.y() - min.y) < FG_EPSILON) &&
		    (fabs(node.x() - max.x) < FG_EPSILON) ) {
	    if ( ! se_flag ) {
		se_node = node;
		se_normal = normal;
	    }
	} else if ( (fabs(node.y() - max.y) < FG_EPSILON) &&
		    (fabs(node.x() - max.x) < FG_EPSILON)) {
	    if ( ! ne_flag ) {
		ne_node = node;
		ne_normal = normal;
	    }
	} else if ( (fabs(node.y() - max.y) < FG_EPSILON) &&
		    (fabs(node.x() - min.x) < FG_EPSILON) ) {
	    if ( ! nw_flag ) {
		nw_node = node;
		nw_normal = normal;
	    }
	} else if ( fabs(node.x() - min.x) < FG_EPSILON ) {
	    west_nodes.push_back( node );
	    west_normals.push_back( normal );
	} else if ( fabs(node.x() - max.x) < FG_EPSILON ) {
	    east_nodes.push_back( node );
	    east_normals.push_back( normal );
	} else if ( fabs(node.y() - min.y) < FG_EPSILON ) {
	    south_nodes.push_back( node );
	    south_normals.push_back( normal );
	} else if ( fabs(node.y() - max.y) < FG_EPSILON ) {
	    north_nodes.push_back( node );
	    north_normals.push_back( normal );
	} else {
	    body_nodes.push_back( node );
	    body_normals.push_back( normal );
	}
    }

    // separate area edge segment into components
    cout << "Extracting (shared) area edge segments" << endl;

    FGTriSeg seg;
    Point3D p1, p2;
    triseg_list seg_list = c.get_tri_segs().get_seg_list();
    triseg_list_iterator current = seg_list.begin();
    triseg_list_iterator last = seg_list.end();

    for ( ; current != last; ++current ) {
	seg = *current;
	p1 = nodes[ seg.get_n1() ];
	p2 = nodes[ seg.get_n2() ];

	if ( fabs(p1.y() - p2.y()) < FG_EPSILON ) {
	    // check if horizontal
	    if ( fabs(p1.y() - max.y) < FG_EPSILON ) {
		north_segs.push_back( seg );
	    } else if ( fabs(p1.y() - min.y) < FG_EPSILON ) {
		south_segs.push_back( seg );
	    } else {
		body_segs.push_back( seg );
	    }
	} else if ( fabs(p1.x() - p2.x()) < FG_EPSILON ) {
	    // check if vertical
	    if ( fabs(p1.x() - max.x) < FG_EPSILON ) {
		east_segs.push_back( seg );
	    } else if ( fabs(p1.x() - min.x) < FG_EPSILON ) {
		west_segs.push_back( seg );
	    } else {
		body_segs.push_back( seg );
	    }
	} else {
	    body_segs.push_back( seg );
	}
    }
}


// write the shared edge points, normals, and segments for this tile
void FGMatch::write_shared( FGConstruct& c ) {
    string base = c.get_work_base();
    FGBucket b = c.get_bucket();

    string dir = base + ".shared/Scenery/" + b.gen_base_path();
    string command = "mkdir -p " + dir;
    string file = dir + "/" + b.gen_index_str();
    cout << "shared data will be written to " << file << endl;

    system(command.c_str());

    FILE *fp;
    if ( (fp = fopen( file.c_str(), "w" )) == NULL ) {
	cout << "ERROR: opening " << file << " for writing!" << endl;
	exit(-1);
    }

    fprintf( fp, "sw_node %.6f %.6f %.6f\n", 
	     sw_node.x(), sw_node.y(), sw_node.z() );
    fprintf( fp, "sw_normal %.6f %.6f %.6f\n", 
	     sw_normal.x(), sw_normal.y(), sw_normal.z() );

    fprintf( fp, "se_node %.6f %.6f %.6f\n", 
	     se_node.x(), se_node.y(), se_node.z() );
    fprintf( fp, "se_normal %.6f %.6f %.6f\n", 
	     se_normal.x(), se_normal.y(), se_normal.z() );

    fprintf( fp, "nw_node %.6f %.6f %.6f\n", 
	     nw_node.x(), nw_node.y(), nw_node.z() );
    fprintf( fp, "nw_normal %.6f %.6f %.6f\n", 
	     nw_normal.x(), nw_normal.y(), nw_normal.z() );

    fprintf( fp, "ne_node %.6f %.6f %.6f\n", 
	     ne_node.x(), ne_node.y(), ne_node.z() );
    fprintf( fp, "ne_normal %.6f %.6f %.6f\n", 
	     ne_normal.x(), ne_normal.y(), ne_normal.z() );

    for ( int i = 0; i < (int)north_nodes.size(); ++i ) {
	fprintf( fp, "n_node %.6f %.6f %.6f\n", 
		 north_nodes[i].x(), north_nodes[i].y(), north_nodes[i].z() );
	fprintf( fp, "n_normal %.6f %.6f %.6f\n", 
		 north_normals[i].x(), north_normals[i].y(),
		 north_normals[i].z() );
    }

    for ( int i = 0; i < (int)south_nodes.size(); ++i ) {
	fprintf( fp, "s_node %.6f %.6f %.6f\n", 
		 south_nodes[i].x(), south_nodes[i].y(), south_nodes[i].z() );
	fprintf( fp, "s_normal %.6f %.6f %.6f\n", 
		 south_normals[i].x(), south_normals[i].y(),
		 south_normals[i].z() );
    }

    for ( int i = 0; i < (int)east_nodes.size(); ++i ) {
	fprintf( fp, "e_node %.6f %.6f %.6f\n", 
		 east_nodes[i].x(), east_nodes[i].y(), east_nodes[i].z() );
	fprintf( fp, "e_normal %.6f %.6f %.6f\n", 
		 east_normals[i].x(), east_normals[i].y(),
		 east_normals[i].z() );
    }

    for ( int i = 0; i < (int)west_nodes.size(); ++i ) {
	fprintf( fp, "w_node %.6f %.6f %.6f\n", 
		 west_nodes[i].x(), west_nodes[i].y(), west_nodes[i].z() );
	fprintf( fp, "w_normal %.6f %.6f %.6f\n", 
		 west_normals[i].x(), west_normals[i].y(),
		 west_normals[i].z() );
    }

#if 0 // not needed
    point_list nodes = c.get_geod_nodes();
    Point3D p1, p2;

    for ( int i = 0; i < (int)north_segs.size(); ++i ) {
	p1 = nodes[ north_segs[i].get_n1() ];
	p2 = nodes[ north_segs[i].get_n2() ];
	fprintf( fp, "n_seg %.6f %.6f %.6f %.6f\n", 
		 p1.x(), p1.y(), p2.x(), p2.y() );
    }

    for ( int i = 0; i < (int)south_segs.size(); ++i ) {
	p1 = nodes[ south_segs[i].get_n1() ];
	p2 = nodes[ south_segs[i].get_n2() ];
	fprintf( fp, "s_seg %.6f %.6f %.6f %.6f\n", 
		 p1.x(), p1.y(), p2.x(), p2.y() );
    }

    for ( int i = 0; i < (int)east_segs.size(); ++i ) {
	p1 = nodes[ east_segs[i].get_n1() ];
	p2 = nodes[ east_segs[i].get_n2() ];
	fprintf( fp, "e_seg %.6f %.6f %.6f %.6f\n", 
		 p1.x(), p1.y(), p2.x(), p2.y() );
    }

    for ( int i = 0; i < (int)west_segs.size(); ++i ) {
	p1 = nodes[ west_segs[i].get_n1() ];
	p2 = nodes[ west_segs[i].get_n2() ];
	fprintf( fp, "w_seg %.6f %.6f %.6f %.6f\n", 
		 p1.x(), p1.y(), p2.x(), p2.y() );
    }
#endif

    fclose( fp );

    command = "gzip --force --best " + file;
    system(command.c_str());
}
