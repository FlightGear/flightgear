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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include STL_FUNCTIONAL
#include STL_ALGORITHM
#include STL_STRING

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sgstream.hxx>

#include <Aircraft/aircraft.hxx>
#include <Include/general.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Time/light.hxx>
#include <Objects/matlib.hxx>
#include <Objects/newmat.hxx>
#include <Objects/obj.hxx>

#include "tileentry.hxx"

SG_USING_STD(for_each);
SG_USING_STD(mem_fun_ref);
SG_USING_STD(string);


// Constructor
FGTileEntry::FGTileEntry ( const SGBucket& b )
    : ncount( 0 ),
      center( Point3D( 0.0 ) ),
      tile_bucket( b ),
      terra_transform( new ssgTransform ),
      terra_range( new ssgRangeSelector ),
      loaded(false)
{
    nodes.clear();

    // update the contents
    // if ( vec3_ptrs.size() || vec2_ptrs.size() || index_ptrs.size() ) {
    //     SG_LOG( SG_TERRAIN, SG_ALERT, 
    //             "Attempting to overwrite existing or"
    //             << " not properly freed leaf data." );
    //     exit(-1);
    // }
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
    SG_LOG( SG_TERRAIN, SG_INFO,
	    "FREEING TILE = (" << tile_bucket << ")" );

    SG_LOG( SG_TERRAIN, SG_DEBUG,
	    "  deleting " << nodes.size() << " nodes" );
    nodes.clear();

    // delete the ssg structures
    SG_LOG( SG_TERRAIN, SG_DEBUG,
	    "  deleting (leaf data) vertex, normal, and "
	    << " texture coordinate arrays" );

    for ( i = 0; i < (int)vec3_ptrs.size(); ++i ) {
	delete [] vec3_ptrs[i];
    }
    vec3_ptrs.clear();

    for ( i = 0; i < (int)vec2_ptrs.size(); ++i ) {
	delete [] vec2_ptrs[i];
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
    if ( !loaded ) return;

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


ssgLeaf* FGTileEntry::gen_lights( ssgVertexArray *lights, int inc, float bright ) {
    // generate a repeatable random seed
    float *p1 = lights->get( 0 );
    unsigned int *seed = (unsigned int *)p1;
    sg_srandom( *seed );

    int size = lights->getNum() / inc;

    // Allocate ssg structure
    ssgVertexArray   *vl = new ssgVertexArray( size + 1 );
    ssgNormalArray   *nl = NULL;
    ssgTexCoordArray *tl = NULL;
    ssgColourArray   *cl = new ssgColourArray( size + 1 );

    sgVec4 color;
    for ( int i = 0; i < lights->getNum(); ++i ) {
	// this loop is slightly less efficient than it otherwise
	// could be, but we want a red light to always be red, and a
	// yellow light to always be yellow, etc. so we are trying to
	// preserve the random sequence.
	float zombie = sg_random();
	if ( i % inc == 0 ) {
	    vl->add( lights->get(i) );

	    // factor = sg_random() ^ 2, range = 0 .. 1 concentrated towards 0
	    float factor = sg_random();
	    factor *= factor;

	    if ( zombie > 0.5 ) {
		// 50% chance of yellowish
		sgSetVec4( color, 0.9, 0.9, 0.3, bright - factor * 0.2 );
	    } else if ( zombie > 0.15 ) {
		// 35% chance of whitish
		sgSetVec4( color, 0.9, 0.9, 0.8, bright - factor * 0.2 );
	    } else if ( zombie > 0.05 ) {
		// 10% chance of orangish
		sgSetVec4( color, 0.9, 0.6, 0.2, bright - factor * 0.2 );
	    } else {
		// 5% chance of redish
		sgSetVec4( color, 0.9, 0.2, 0.2, bright - factor * 0.2 );
	    }
	    cl->add( color );
	}
    }

    // create ssg leaf
    ssgLeaf *leaf = 
	new ssgVtxTable ( GL_POINTS, vl, nl, tl, cl );

    // assign state
    FGNewMat *newmat = material_lib.find( "LIGHTS" );
    leaf->setState( newmat->get_state() );

    return leaf;
}


ssgBranch*
FGTileEntry::obj_load( const std::string& path,
		       ssgVertexArray* lights, bool is_base )
{
    ssgBranch* result = 0;

    // try loading binary format
    result = fgBinObjLoad( path, this, lights, is_base );
    if ( result == NULL ) {
	// next try the older ascii format
	result = fgAsciiObjLoad( path, this, lights, is_base );
	if ( result == NULL ) {
	    // default to an ocean tile
	    result = fgGenTile( path, this );
	}
    }

    return result;
}


void
FGTileEntry::load( const SGPath& base, bool is_base )
{
    string index_str = tile_bucket.gen_index_str();

    SGPath tile_path = base;
    // Generate name of file to load.
    tile_path.append( tile_bucket.gen_base_path() );
    SGPath basename = tile_path;
    basename.append( index_str );
    string path = basename.str();

    SG_LOG( SG_TERRAIN, SG_INFO, "Loading tile " << path );

    // fgObjLoad will generate ground lighting for us ...
    ssgVertexArray *light_pts = new ssgVertexArray( 100 );

    ssgBranch* new_tile = obj_load( path, light_pts, is_base );
    if ( new_tile != NULL ) {
	terra_range->addKid( new_tile );
    }

    // load custom objects
    SG_LOG( SG_TERRAIN, SG_DEBUG, "CUSTOM OBJECTS" );

    SGPath index_path = tile_path;
    index_path.append( index_str );
    index_path.concat( ".ind" );

    SG_LOG( SG_TERRAIN, SG_DEBUG, "Looking in " << index_path.str() );

    sg_gzifstream in( index_path.str() );

    if ( in.is_open() ) {
	string token, name;

	while ( ! in.eof() ) {
	    in >> token;

	    if ( token == "OBJECT" ) {
		in >> name >> ::skipws;
		SG_LOG( SG_TERRAIN, SG_DEBUG, "token = " << token
			<< " name = " << name );

		SGPath custom_path = tile_path;
		custom_path.append( name );
		ssgBranch *custom_obj
		    = obj_load( custom_path.str(), NULL, false );
		if ( (new_tile != NULL) && (custom_obj != NULL) ) {
		    new_tile -> addKid( custom_obj );
		}
	    } else if ( token == "OBJECT_STATIC" ) {
		// load object info
		double lon, lat, elev, hdg;
		in >> name >> lon >> lat >> elev >> hdg >> ::skipws;
		SG_LOG( SG_TERRAIN, SG_INFO, "token = " << token
			<< " name = " << name 
			<< " pos = " << lon << ", " << lat
			<< " elevation = " << elev
			<< " heading = " << hdg );

		// load the object itself
		SGPath custom_path = tile_path;
		custom_path.append( name );
		ssgEntity *obj_model = ssgLoad( (char *)custom_path.c_str() );

		// setup transforms
		Point3D geod( lon * SGD_DEGREES_TO_RADIANS,
			      lat *  SGD_DEGREES_TO_RADIANS,
			      elev );
		Point3D world_pos = sgGeodToCart( geod );
		Point3D offset = world_pos - center;
		sgMat4 POS;
		sgMakeTransMat4( POS, offset.x(), offset.y(), offset.z() );

		sgVec3 obj_rt, obj_up;
		sgSetVec3( obj_rt, 0.0, 1.0, 0.0); // Y axis
		sgSetVec3( obj_up, 0.0, 0.0, 1.0); // Z axis

		sgMat4 ROT_lon, ROT_lat, ROT_hdg;
		sgMakeRotMat4( ROT_lon, lon, obj_up );
		sgMakeRotMat4( ROT_lat, 90 - lat, obj_rt );
		sgMakeRotMat4( ROT_hdg, hdg, obj_up );
	
		sgMat4 TUX;
		sgCopyMat4( TUX, ROT_hdg );
		sgPostMultMat4( TUX, ROT_lat );
		sgPostMultMat4( TUX, ROT_lon );
		sgPostMultMat4( TUX, POS );

		sgCoord obj_pos;
		sgSetCoord( &obj_pos, TUX );
		ssgTransform *obj_trans = new ssgTransform;
		obj_trans->setTransform( &obj_pos );

		// wire the scene graph together
		obj_trans->addKid( obj_model );
		new_tile->addKid( obj_trans );
	    } else {
		SG_LOG( SG_TERRAIN, SG_ALERT,
			"Unknown token " << token << " in "
			<< index_path.str() );
		in >> ::skipws;
	    }
	}
    }

    terra_transform->addKid( terra_range );

    // calculate initial tile offset
    SetOffset( scenery.center );
    sgCoord sgcoord;
    sgSetCoord( &sgcoord,
		offset.x(), offset.y(), offset.z(),
		0.0, 0.0, 0.0 );
    terra_transform->setTransform( &sgcoord );
    // terrain->addKid( terra_transform );

    lights_transform = NULL;
    lights_range = NULL;
    /* uncomment this section for testing ground lights */
    if ( light_pts->getNum() ) {
	SG_LOG( SG_TERRAIN, SG_DEBUG, "generating lights" );
	lights_transform = new ssgTransform;
	lights_range = new ssgRangeSelector;
	lights_brightness = new ssgSelector;
	ssgLeaf *lights;

	lights = gen_lights( light_pts, 4, 0.7 );
	lights_brightness->addKid( lights );

	lights = gen_lights( light_pts, 2, 0.85 );
	lights_brightness->addKid( lights );

	lights = gen_lights( light_pts, 1, 1.0 );
	lights_brightness->addKid( lights );

	lights_range->addKid( lights_brightness );
	lights_transform->addKid( lights_range );
	lights_transform->setTransform( &sgcoord );
        // ground->addKid( lights_transform );
    }
    /* end of ground light section */
}


void
FGTileEntry::add_ssg_nodes( ssgBranch* terrain, ssgBranch* ground )
{
    terrain->addKid( terra_transform );
    if (lights_transform != 0)
	ground->addKid( lights_transform );

    loaded = true;
}
