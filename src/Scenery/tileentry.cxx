// tile.cxx -- routines to handle a scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998, 1999  Curtis L. Olson  - curt@flightgear.org
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


#include <simgear/compiler.h>

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_FUNCTIONAL
#include STL_ALGORITHM

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>

#include <Aircraft/aircraft.hxx>
#include <Include/general.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Time/light.hxx>

#include "tileentry.hxx"

SG_USING_STD(for_each);
SG_USING_STD(mem_fun_ref);


// Constructor
FGTileEntry::FGTileEntry ()
    : ncount(0)
{
    nodes.clear();
}


// Destructor
FGTileEntry::~FGTileEntry () {
    // cout << "nodes = " << nodes.size() << endl;;
    // delete[] nodes;
}


// recurse an ssg tree and call removeKid() on every node from the
// bottom up.  Leaves the original branch in existance, but empty so
// it can be removed by the calling routine.
static void my_remove_branch( ssgBranch * branch ) {
    for ( ssgEntity *k = branch->getKid( 0 );
	  k != NULL; 
	  k = branch->getNextKid() )
    {
	if ( k -> isAKindOf ( ssgTypeBranch() ) ) {
	    my_remove_branch( (ssgBranch *)k );
	    branch -> removeKid ( k );
	} else if ( k -> isAKindOf ( ssgTypeLeaf() ) ) {
	    branch -> removeKid ( k ) ;
	}
    }
}


// Clean up the memory used by this tile and delete the arrays used by
// ssg as well as the whole ssg branch
void FGTileEntry::free_tile() {
    int i;
    SG_LOG( SG_TERRAIN, SG_DEBUG,
	    "FREEING TILE = (" << tile_bucket << ")" );

    SG_LOG( SG_TERRAIN, SG_DEBUG,
	    "  deleting " << nodes.size() << " nodes" );
    nodes.clear();

    // delete the ssg structures
    SG_LOG( SG_TERRAIN, SG_DEBUG,
	    "  deleting (leaf data) vertex, normal, and "
	    << " texture coordinate arrays" );

    for ( i = 0; i < (int)vec3_ptrs.size(); ++i ) {
#if defined(macintosh) || defined(_MSC_VER)
	delete [] vec3_ptrs[i];  //that's the correct version
#else
	delete vec3_ptrs[i];
#endif
    }
    vec3_ptrs.clear();

    for ( i = 0; i < (int)vec2_ptrs.size(); ++i ) {
#if defined(macintosh) || defined(_MSC_VER)
	delete [] vec2_ptrs[i];  //that's the correct version
#else
	delete vec2_ptrs[i];
#endif
    }
    vec2_ptrs.clear();

    for ( i = 0; i < (int)index_ptrs.size(); ++i ) {
	delete index_ptrs[i];
    }
    index_ptrs.clear();

    // delete the terrain branch
    int pcount = terra_transform->getNumParents();
    if ( pcount > 0 ) {
	// find the first parent (should only be one)
	ssgBranch *parent = terra_transform->getParent( 0 ) ;
	if( parent ) {
	    // my_remove_branch( select_ptr );
	    parent->removeKid( terra_transform );
	    terra_transform = NULL;
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT,
		    "parent pointer is NULL!  Dying" );
	    exit(-1);
	}
    } else {
	SG_LOG( SG_TERRAIN, SG_ALERT,
		"Parent count is zero for an ssg tile!  Dying" );
	exit(-1);
    }

    if ( lights_transform ) {
	// delete the terrain lighting branch
	pcount = lights_transform->getNumParents();
	if ( pcount > 0 ) {
	    // find the first parent (should only be one)
	    ssgBranch *parent = lights_transform->getParent( 0 ) ;
	    if( parent ) {
		parent->removeKid( lights_transform );
		lights_transform = NULL;
	    } else {
		SG_LOG( SG_TERRAIN, SG_ALERT,
			"parent pointer is NULL!  Dying" );
		exit(-1);
	    }
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT,
		    "Parent count is zero for an ssg light tile!  Dying" );
	    exit(-1);
	}
    }
}


// Update the ssg transform node for this tile so it can be
// properly drawn relative to our (0,0,0) point
void FGTileEntry::prep_ssg_node( const Point3D& p, float vis) {
    SetOffset( p );

// #define USE_UP_AND_COMING_PLIB_FEATURE
#ifdef USE_UP_AND_COMING_PLIB_FEATURE
    terra_range->setRange( 0, SG_ZERO );
    terra_range->setRange( 1, vis + bounding_radius );
    lights_range->setRange( 0, SG_ZERO );
    lights_range->setRange( 1, vis * 1.5 + bounding_radius );
#else
    float ranges[2];
    ranges[0] = SG_ZERO;
    ranges[1] = vis + bounding_radius;
    terra_range->setRanges( ranges, 2 );
    if ( lights_range ) {
	ranges[1] = vis * 1.5 + bounding_radius;
	lights_range->setRanges( ranges, 2 );
    }
#endif
    sgVec3 sgTrans;
    sgSetVec3( sgTrans, offset.x(), offset.y(), offset.z() );
    terra_transform->setTransform( sgTrans );

    if ( lights_transform ) {
	// we need to lift the lights above the terrain to avoid
	// z-buffer fighting.  We do this based on our altitude and
	// the distance this tile is away from scenery center.

	sgVec3 up;
	sgCopyVec3( up, globals->get_current_view()->get_world_up() );

	double agl;
	if ( current_aircraft.fdm_state ) {
	    agl = current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER
		- scenery.cur_elev;
	} else {
	    agl = 0.0;
	}

	// sgTrans just happens to be the
	// vector from scenery center to the center of this tile which
	// is what we want to calculate the distance of
	sgVec3 to;
	sgCopyVec3( to, sgTrans );
	double dist = sgLengthVec3( to );

	if ( general.get_glDepthBits() > 16 ) {
	    sgScaleVec3( up, 10.0 + agl / 100.0 + dist / 10000 );
	} else {
	    sgScaleVec3( up, 10.0 + agl / 20.0 + dist / 5000 );
	}
	sgAddVec3( sgTrans, up );
	lights_transform->setTransform( sgTrans );

	// select which set of lights based on sun angle
	float sun_angle = cur_light_params.sun_angle * SGD_RADIANS_TO_DEGREES;
	if ( sun_angle > 95 ) {
	    lights_brightness->select(0x04);
	} else if ( sun_angle > 92 ) {
	    lights_brightness->select(0x02);
	} else if ( sun_angle > 89 ) {
	    lights_brightness->select(0x01);
	} else {
	    lights_brightness->select(0x00);
	}
    }
}
