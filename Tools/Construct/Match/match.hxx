// match.hxx -- Class to help match up tile edges
//
// Written by Curtis Olson, started April 1999.
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


#ifndef _MATCH_HXX
#define _MATCH_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include <Bucket/newbucket.hxx>

#include <Main/construct_types.hxx>
#include <Main/construct.hxx>


class FGMatch {

private:

    // nodes breakdown
    Point3D sw_node, se_node, ne_node, nw_node;
    point_list north_nodes, south_nodes, east_nodes, west_nodes;
    point_list body_nodes;

    // normals breakdown
    Point3D sw_normal, se_normal, ne_normal, nw_normal;
    point_list north_normals, south_normals, east_normals, west_normals;
    point_list body_normals;

    // flags
    bool sw_flag, se_flag, ne_flag, nw_flag;
    bool north_flag, south_flag, east_flag, west_flag;

    // segment breakdown
    triseg_list north_segs, south_segs, east_segs, west_segs;
    triseg_list body_segs;

public:

    enum neighbor_type {
	SW_Corner = 1,
	SE_Corner = 2,
	NE_Corner = 3,
	NW_Corner = 4,
	NORTH = 5,
	SOUTH = 6,
	EAST = 7,
	WEST = 8
    };

    // Constructor
    FGMatch( void );

    // Destructor
    ~FGMatch( void );

    // load any previously existing shared data from all neighbors
    void load_neighbor_shared( FGConstruct& c );

    // scan the specified share file for the specified information
    void scan_share_file( const string& dir, const FGBucket& b,
			  neighbor_type n );

    // try to find info for the specified shared component
    void load_shared( const FGConstruct& c, neighbor_type n );

    // split up the tile between the shared edge points, normals, and
    // segments and the body.  This must be done after calling
    // load_neighbor_data() and will ignore any shared data from the
    // current tile that already exists from a neighbor.
    void split_tile( FGConstruct& c );

    // write the shared edge points, normals, and segments for this tile
    void write_shared( FGConstruct& c );
};


#endif // _MATCH_HXX
