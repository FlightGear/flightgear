// tilemgr.cxx -- routines to handle dynamic management of scenery tiles
//
// Written by Curtis Olson, started January 1998.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Aircraft/aircraft.hxx>

#include <Debug/logstream.hxx>
// #include <Bucket/bucketutils.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Math/fg_geodesy.hxx>
#include <Math/mat3.h>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Math/vector.hxx>
#include <Objects/materialmgr.hxx>
#include <Objects/obj.hxx>
#include <Weather/weather.hxx>

#include "scenery.hxx"
#include "tilecache.hxx"
#include "tileentry.hxx"
#include "tilemgr.hxx"


// to test clipping speedup in fgTileMgrRender()
#if defined ( USE_FAST_FOV_CLIP )
// #define TEST_FOV_CLIP
// #define TEST_ELEV
#endif


// the tile manager
FGTileMgr global_tile_mgr;


// Constructor
FGTileMgr::FGTileMgr ( void ):
    state( Start )
{
}


// Destructor
FGTileMgr::~FGTileMgr ( void ) {
}


// Initialize the Tile Manager subsystem
int FGTileMgr::init( void ) {
    FG_LOG( FG_TERRAIN, FG_INFO, "Initializing Tile Manager subsystem." );

    // load default material library
    if ( ! material_mgr.loaded() ) {
	material_mgr.load_lib();
    }

    state = Inited;

    return 1;
}


// schedule a tile for loading
void FGTileMgr::sched_tile( const FGBucket& b, int *index ) {
    // see if tile already exists in the cache
    *index = global_tile_cache.exists( b );
    if ( *index < 0 ) {
	// find the next availabel cache entry and mark it as scheduled
	*index = global_tile_cache.next_avail();
	FGTileEntry *t = global_tile_cache.get_tile( *index );
	t->mark_scheduled();

	// register a load request
	FGLoadRec request;
	request.b = b;
	request.index = *index;
	load_queue.push_back( request );
    }
}


// load a tile
void FGTileMgr::load_tile( const FGBucket& b, int cache_index) {

    FG_LOG( FG_TERRAIN, FG_DEBUG, "Loading tile " << b );
    
    global_tile_cache.fill_in(cache_index, b);

    FG_LOG( FG_TERRAIN, FG_DEBUG, "Loaded for cache index: " << cache_index );
}


// Calculate shortest distance from point to line
static double point_line_dist_squared( const Point3D& tc, const Point3D& vp, 
				       MAT3vec d )
{
    MAT3vec p, p0;

    p[0] = tc.x(); p[1] = tc.y(); p[2] = tc.z();
    p0[0] = vp.x(); p0[1] = vp.y(); p0[2] = vp.z();

    return fgPointLineSquared(p, p0, d);
}


// Determine scenery altitude.  Normally this just happens when we
// render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  abs_view_pos in meters.
// Returns result in meters.
double
FGTileMgr::current_elev_new( const FGBucket& p ) {
    FGTileEntry *t;
    fgFRAGMENT *frag_ptr;
    Point3D abs_view_pos = current_view.get_abs_view_pos();
    Point3D earth_center(0.0);
    Point3D result;
    MAT3vec local_up;
    double dist, lat_geod, alt, sea_level_r;
    int index;

    local_up[0] = abs_view_pos.x();
    local_up[1] = abs_view_pos.y();
    local_up[2] = abs_view_pos.z();

    // Find current translation offset
    // fgBucketFind(lon * RAD_TO_DEG, lat * RAD_TO_DEG, &p);
    index = global_tile_cache.exists(p);
    if ( index < 0 ) {
	FG_LOG( FG_TERRAIN, FG_WARN, "Tile not found" );
	return 0.0;
    }

    t = global_tile_cache.get_tile(index);

    scenery.next_center = t->center;
    
    FG_LOG( FG_TERRAIN, FG_DEBUG, 
	    "Current bucket = " << p << "  Index = " << p.gen_index_str() );
    FG_LOG( FG_TERRAIN, FG_DEBUG,
	    "abs_view_pos = " << abs_view_pos );

    // calculate tile offset
    // x = (t->offset.x = t->center.x - scenery.center.x);
    // y = (t->offset.y = t->center.y - scenery.center.y);
    // z = (t->offset.z = t->center.z - scenery.center.z);
    
    // calc current terrain elevation calculate distance from
    // vertical tangent line at current position to center of
    // tile.
	
    /* printf("distance squared = %.2f, bounding radius = %.2f\n", 
       point_line_dist_squared(&(t->offset), &(v->view_pos), 
       v->local_up), t->bounding_radius); */

    dist = point_line_dist_squared( t->center, abs_view_pos, local_up );
    if ( dist < FG_SQUARE(t->bounding_radius) ) {

	// traverse fragment list for tile
        FGTileEntry::FragmentIterator current = t->begin();
        FGTileEntry::FragmentIterator last = t->end();

	for ( ; current != last; ++current ) {
	    frag_ptr = &(*current);
	    /* printf("distance squared = %.2f, bounding radius = %.2f\n", 
	       point_line_dist_squared( &(frag_ptr->center), 
	       &abs_view_pos), local_up),
	       frag_ptr->bounding_radius); */

	    dist = point_line_dist_squared( frag_ptr->center,
					    abs_view_pos,
					    local_up);
	    if ( dist <= FG_SQUARE(frag_ptr->bounding_radius) ) {
		if ( frag_ptr->intersect( abs_view_pos, 
					  earth_center, 0, result ) ) {
		    FG_LOG( FG_TERRAIN, FG_DEBUG, "intersection point " <<
			    result );
		    // compute geocentric coordinates of tile center
		    Point3D pp = fgCartToPolar3d(result);
		    FG_LOG( FG_TERRAIN, FG_DEBUG, "  polar form = " << pp );
		    // convert to geodetic coordinates
		    fgGeocToGeod(pp.lat(), pp.radius(), &lat_geod, 
				 &alt, &sea_level_r);

		    // printf("alt = %.2f\n", alt);
		    // exit since we found an intersection
		    if ( alt > -9999.0 ) {
			// printf("returning alt\n");
			return alt;
		    } else {
			// printf("returning 0\n");
			return 0.0;
		    }
		}
	    }
	}
    }

    FG_LOG( FG_TERRAIN, FG_INFO, "(new) no terrain intersection found" );

    return 0.0;
}


// Determine scenery altitude.  Normally this just happens when we
// render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  abs_view_pos in meters.
// Returns result in meters.
double
FGTileMgr::current_elev( double lon, double lat, const Point3D& abs_view_pos ) {
    FGTileCache *c;
    FGTileEntry *t;
    fgFRAGMENT *frag_ptr;
    Point3D earth_center(0.0);
    Point3D result;
    MAT3vec local_up;
    double dist, lat_geod, alt, sea_level_r;
    int index;

    c = &global_tile_cache;

    local_up[0] = abs_view_pos.x();
    local_up[1] = abs_view_pos.y();
    local_up[2] = abs_view_pos.z();

    FG_LOG( FG_TERRAIN, FG_DEBUG, "Absolute view pos = " << abs_view_pos );

    // Find current translation offset
    FGBucket p( lon * RAD_TO_DEG, lat * RAD_TO_DEG );
    index = c->exists(p);
    if ( index < 0 ) {
	FG_LOG( FG_TERRAIN, FG_WARN, "Tile not found" );
	return 0.0;
    }

    t = c->get_tile(index);

    scenery.next_center = t->center;
    
    FG_LOG( FG_TERRAIN, FG_DEBUG, 
	    "Pos = (" << lon * RAD_TO_DEG << ", " << lat * RAD_TO_DEG
	    << ")  Current bucket = " << p 
	    << "  Index = " << p.gen_index_str() );

    FG_LOG( FG_TERRAIN, FG_DEBUG, "Tile center " << t->center 
	    << "  bounding radius = " << t->bounding_radius );

    // calculate tile offset
    // x = (t->offset.x = t->center.x - scenery.center.x);
    // y = (t->offset.y = t->center.y - scenery.center.y);
    // z = (t->offset.z = t->center.z - scenery.center.z);
    
    // calc current terrain elevation calculate distance from
    // vertical tangent line at current position to center of
    // tile.
	
    /* printf("distance squared = %.2f, bounding radius = %.2f\n", 
       point_line_dist_squared(&(t->offset), &(v->view_pos), 
       v->local_up), t->bounding_radius); */

    dist = point_line_dist_squared( t->center, abs_view_pos, local_up );
    FG_LOG( FG_TERRAIN, FG_DEBUG, "(gross check) dist squared = " << dist );

    if ( dist < FG_SQUARE(t->bounding_radius) ) {

	// traverse fragment list for tile
        FGTileEntry::FragmentIterator current = t->begin();
        FGTileEntry::FragmentIterator last = t->end();

	for ( ; current != last; ++current ) {
	    frag_ptr = &(*current);
	    /* printf("distance squared = %.2f, bounding radius = %.2f\n", 
	       point_line_dist_squared( &(frag_ptr->center), 
	       &abs_view_pos), local_up),
	       frag_ptr->bounding_radius); */

	    dist = point_line_dist_squared( frag_ptr->center,
					    abs_view_pos,
					    local_up);
	    if ( dist <= FG_SQUARE(frag_ptr->bounding_radius) ) {
		if ( frag_ptr->intersect( abs_view_pos, 
					  earth_center, 0, result ) ) {
		    FG_LOG( FG_TERRAIN, FG_DEBUG, "intersection point " <<
			    result );
		    // compute geocentric coordinates of tile center
		    Point3D pp = fgCartToPolar3d(result);
		    FG_LOG( FG_TERRAIN, FG_DEBUG, "  polar form = " << pp );
		    // convert to geodetic coordinates
		    fgGeocToGeod(pp.lat(), pp.radius(), &lat_geod, 
				 &alt, &sea_level_r);

		    // printf("alt = %.2f\n", alt);
		    // exit since we found an intersection
		    if ( alt > -9999.0 ) {
			// printf("returning alt\n");
			return alt;
		    } else {
			// printf("returning 0\n");
			return 0.0;
		    }
		}
	    }
	}
    }

    FG_LOG( FG_TERRAIN, FG_INFO, "(old) no terrain intersection found" );

    return 0.0;
}


// given the current lon/lat, fill in the array of local chunks.  If
// the chunk isn't already in the cache, then read it from disk.
int FGTileMgr::update( void ) {
    FGTileCache *c;
    FGInterface *f;
    FGBucket p2;
    static FGBucket p_last(false);
    static double last_lon = -1000.0;  // in degrees
    static double last_lat = -1000.0;  // in degrees
    int tile_diameter;
    int i, j, dw, dh;

    c = &global_tile_cache;
    f = current_aircraft.fdm_state;

    tile_diameter = current_options.get_tile_diameter();

    FGBucket p1( f->get_Longitude() * RAD_TO_DEG,
		 f->get_Latitude() * RAD_TO_DEG );
    dw = tile_diameter / 2;
    dh = tile_diameter / 2;

    if ( (p1 == p_last) && (state == Running) ) {
	// same bucket as last time
	FG_LOG( FG_TERRAIN, FG_DEBUG, "Same bucket as last time" );
    } else if ( (state == Start) || (state == Inited) ) {
	state = Running;

	// First time through or we have teleporte, initialize the
	// system and load all relavant tiles

	FG_LOG( FG_TERRAIN, FG_INFO, "Updating Tile list for " << p1 );
	FG_LOG( FG_TERRAIN, FG_INFO, "  First time through ... " );
	FG_LOG( FG_TERRAIN, FG_INFO, "  Updating Tile list for " << p1 );
	FG_LOG( FG_TERRAIN, FG_INFO, "  Loading " 
		<< tile_diameter * tile_diameter << " tiles" );

	// wipe/initialize tile cache
	c->init();
	p_last.make_bad();

	// build the local area list and schedule tiles for loading

	// start with the center tile and work out in concentric
	// "rings"

	p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
			     f->get_Latitude() * RAD_TO_DEG,
			     0, 0 );
	sched_tile( p2, &tiles[(dh*tile_diameter) + dw]);

	for ( i = 3; i <= tile_diameter; i = i + 2 ) {
	    int span = i / 2;

	    // bottom row
	    for ( j = -span; j <= span; ++j ) {
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     j, -span );
		sched_tile( p2, &tiles[((dh-span)*tile_diameter) + dw+j]);
	    }

	    // top row
	    for ( j = -span; j <= span; ++j ) {
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     j, span );
		sched_tile( p2, &tiles[((dh+span)*tile_diameter) + dw+j]);
	    }

	    // middle rows
	    for ( j = -span + 1; j <= span - 1; ++j ) {
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     -span, j );
		sched_tile( p2, &tiles[((dh+j)*tile_diameter) + dw-span]);
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     span, j );
		sched_tile( p2, &tiles[((dh+j)*tile_diameter) + dw+span]);
	    }

	}

	/* for ( j = 0; j < tile_diameter; j++ ) {
	    for ( i = 0; i < tile_diameter; i++ ) {
		// fgBucketOffset(&p1, &p2, i - dw, j - dh);
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     i - dw, j -dh );
		sched_tile( p2, &tiles[(j*tile_diameter) + i]);
	    }
	} */

	// Now force a load of the center tile and inner ring so we
	// have something to see in our first frame.
	for ( i = 0; i < 9; ++i ) {
	    if ( load_queue.size() ) {
		FG_LOG( FG_TERRAIN, FG_INFO, 
			"Load queue not empty, loading a tile" );
	    
		FGLoadRec pending = load_queue.front();
		load_queue.pop_front();
		load_tile( pending.b, pending.index );
	    }
	}

    } else {
	// We've moved to a new bucket, we need to scroll our
        // structures, and load in the new tiles

	// CURRENTLY THIS ASSUMES WE CAN ONLY MOVE TO ADJACENT TILES.
	// AT ULTRA HIGH SPEEDS THIS ASSUMPTION MAY NOT BE VALID IF
	// THE AIRCRAFT CAN SKIP A TILE IN A SINGLE ITERATION.

	FG_LOG( FG_TERRAIN, FG_INFO, "Updating Tile list for " << p1 );

	if ( (p1.get_lon() > p_last.get_lon()) ||
	     ( (p1.get_lon() == p_last.get_lon()) && (p1.get_x() > p_last.get_x()) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  Loading " << tile_diameter << "tiles" );
	    for ( j = 0; j < tile_diameter; j++ ) {
		// scrolling East
		for ( i = 0; i < tile_diameter - 1; i++ ) {
		    tiles[(j*tile_diameter) + i] = 
			tiles[(j*tile_diameter) + i + 1];
		}
		// load in new column
		// fgBucketOffset(&p_last, &p2, dw + 1, j - dh);
		p2 = fgBucketOffset( last_lon, last_lat, dw + 1, j - dh );
		sched_tile( p2, &tiles[(j*tile_diameter) + 
					     tile_diameter - 1]);
	    }
	} else if ( (p1.get_lon() < p_last.get_lon()) ||
		    ( (p1.get_lon() == p_last.get_lon()) && (p1.get_x() < p_last.get_x()) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  Loading " << tile_diameter << "tiles" );
	    for ( j = 0; j < tile_diameter; j++ ) {
		// scrolling West
		for ( i = tile_diameter - 1; i > 0; i-- ) {
		    tiles[(j*tile_diameter) + i] = 
			tiles[(j*tile_diameter) + i - 1];
		}
		// load in new column
		// fgBucketOffset(&p_last, &p2, -dw - 1, j - dh);
		p2 = fgBucketOffset( last_lon, last_lat, -dw - 1, j - dh );
		sched_tile( p2, &tiles[(j*tile_diameter) + 0]);
	    }
	}

	if ( (p1.get_lat() > p_last.get_lat()) ||
	     ( (p1.get_lat() == p_last.get_lat()) && (p1.get_y() > p_last.get_y()) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  Loading " << tile_diameter << "tiles" );
	    for ( i = 0; i < tile_diameter; i++ ) {
		// scrolling North
		for ( j = 0; j < tile_diameter - 1; j++ ) {
		    tiles[(j * tile_diameter) + i] =
			tiles[((j+1) * tile_diameter) + i];
		}
		// load in new column
		// fgBucketOffset(&p_last, &p2, i - dw, dh + 1);
		p2 = fgBucketOffset( last_lon, last_lat, i - dw, dh + 1);
		sched_tile( p2, &tiles[((tile_diameter-1) * 
					       tile_diameter) + i]);
	    }
	} else if ( (p1.get_lat() < p_last.get_lat()) ||
		    ( (p1.get_lat() == p_last.get_lat()) && (p1.get_y() < p_last.get_y()) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  Loading " << tile_diameter << "tiles" );
	    for ( i = 0; i < tile_diameter; i++ ) {
		// scrolling South
		for ( j = tile_diameter - 1; j > 0; j-- ) {
		    tiles[(j * tile_diameter) + i] = 
			tiles[((j-1) * tile_diameter) + i];
		}
		// load in new column
		// fgBucketOffset(&p_last, &p2, i - dw, -dh - 1);
		p2 = fgBucketOffset( last_lon, last_lat, i - dw, -dh - 1);
		sched_tile( p2, &tiles[0 + i]);
	    }
	}
    }

    if ( load_queue.size() ) {
	FG_LOG( FG_TERRAIN, FG_INFO, "Load queue not empty, loading a tile" );

	FGLoadRec pending = load_queue.front();
	load_queue.pop_front();
	load_tile( pending.b, pending.index );
    }

    // find our current elevation (feed in the current bucket to save work)
    Point3D geod_pos = Point3D( f->get_Longitude(), f->get_Latitude(), 0.0);
    Point3D tmp_abs_view_pos = fgGeodToCart(geod_pos);

    scenery.cur_elev = 
	current_elev( f->get_Longitude(), f->get_Latitude(), tmp_abs_view_pos );

    p_last = p1;
    last_lon = f->get_Longitude() * RAD_TO_DEG;
    last_lat = f->get_Latitude() * RAD_TO_DEG;

    return 1;
}


// Calculate if point/radius is inside view frustum
static int viewable( const Point3D& cp, double radius ) {
    int viewable = 1; // start by assuming it's viewable
    double x1, y1;

    /********************************/
#if defined( USE_FAST_FOV_CLIP ) // views.hxx
    /********************************/
	
    MAT3vec eye;	
    double *mat;
    double x, y, z;

    x = cp.x();
    y = cp.y();
    z = cp.z();
	
    mat = (double *)(current_view.get_WORLD_TO_EYE());
	
    eye[2] =  x*mat[2] + y*mat[6] + z*mat[10] + mat[14];
	
    // Check near and far clip plane
    if( ( eye[2] > radius ) ||
	( eye[2] + radius + current_weather.get_visibility() < 0) )
	{
	    return(0);
	}
	
    eye[0] = (x*mat[0] + y*mat[4] + z*mat[8] + mat[12])
	* current_view.get_slope_x();

    // check right and left clip plane (from eye perspective)
    x1 = radius * current_view.get_fov_x_clip();
    if( (eye[2] > -(eye[0]+x1)) || (eye[2] > (eye[0]-x1)) )
	{
	    return(0);
	}
	
    eye[1] = (x*mat[1] + y*mat[5] + z*mat[9] + mat[13]) 
	* current_view.get_slope_y();

    // check bottom and top clip plane (from eye perspective)
    y1 = radius * current_view.get_fov_y_clip();
    if( (eye[2] > -(eye[1]+y1)) || (eye[2] > (eye[1]-y1)) )
	{
	    return(0);
	}

    /********************************/	
#else // DO NOT USE_FAST_FOV_CLIP
    /********************************/	

    fgVIEW *v;
    MAT3hvec world, eye;
    double x0, slope;

    v = &current_view;

    MAT3_SET_HVEC(world, cp->x, cp->y, cp->z, 1.0);
    // MAT3mult_vec(eye, world, v->WORLD_TO_EYE);
    // printf( "\nworld -> eye = %.2f %.2f %.2f  radius = %.2f\n", 
    //         eye[0], eye[1], eye[2], radius);

    // Use lazy evaluation for calculating eye hvec.
#define vec world
#define mat v->WORLD_TO_EYE
    eye[2] = vec[0]*mat[0][2]+vec[1]*mat[1][2]+vec[2]*mat[2][2]+mat[3][2];

    // Check near clip plane
    if ( eye[2] > radius ) {
	return(0);
    }

    // Check far clip plane
    if ( eye[2] + radius < -current_weather.get_visibility() ) {
	return(0);
    }

    // check right clip plane (from eye perspective)
    // y = m * (x - x0) = equation of a line intercepting X axis at x0
    x1 = v->cos_fov_x * radius;
    y1 = v->sin_fov_x * radius;
    slope = v->slope_x;
    eye[0] = vec[0]*mat[0][0]+vec[1]*mat[1][0]+vec[2]*mat[2][0]+mat[3][0];

    if ( eye[2] > ((slope * (eye[0] - x1)) + y1) ) {
	return( false );
    }

    // check left clip plane (from eye perspective)
    if ( eye[2] > -((slope * (eye[0] + x1)) - y1) ) {
	return( false );
    }

    // check bottom clip plane (from eye perspective)
    x1 = -(v->cos_fov_y) * radius;
    y1 = v->sin_fov_y * radius;
    slope = v->slope_y;
    eye[1] = vec[0]*mat[0][1]+vec[1]*mat[1][1]+vec[2]*mat[2][1]+mat[3][1];
#undef vec
#undef mat

    if ( eye[2] > ((slope * (eye[1] - x1)) + y1) ) {
	return( false );
    }

    // check top clip plane (from eye perspective)
    if ( eye[2] > -((slope * (eye[1] + x1)) - y1) ) {
	return( false );
    }

#endif // defined( USE_FAST_FOV_CLIP )
	
    return(viewable);
}


// NEW 

// inrange() IS THIS POINT WITHIN POSSIBLE VIEWING RANGE ?
//	calculate distance from vertical tangent line at
//	current position to center of object.
//	this is equivalent to
//	dist = point_line_dist_squared( &(t->center), &(v->abs_view_pos), 
//				        v->local_up );
//	if ( dist < FG_SQUARE(t->bounding_radius) ) {
//
// the compiler should inline this for us

static int
inrange( const double radius, const Point3D& center, const Point3D& vp,
	 const MAT3vec up)
{
    MAT3vec u, u1, v;
    //	double tmp;
	
    // u = p - p0
    u[0] = center.x() - vp.x();
    u[1] = center.y() - vp.y();
    u[2] = center.z() - vp.z();
	
    // calculate the projection, u1, of u along d.
    // u1 = ( dot_prod(u, d) / dot_prod(d, d) ) * d;
	
    MAT3_SCALE_VEC(u1, up,
		   (MAT3_DOT_PRODUCT(u, up) / MAT3_DOT_PRODUCT(up, up)) );
    
    // v = u - u1 = vector from closest point on line, p1, to the
    // original point, p.
    MAT3_SUB_VEC(v, u, u1);
	
    return( FG_SQUARE(radius) >= MAT3_DOT_PRODUCT(v, v));
}


// NEW for legibility

// update this tile's geometry for current view
// The Compiler should inline this
static void
update_tile_geometry( FGTileEntry *t, GLdouble *MODEL_VIEW)
{
    GLfloat *m;
    double x, y, z;
	
    // calculate tile offset
    t->offset = t->center - scenery.center;

    x = t->offset.x();
    y = t->offset.y();
    z = t->offset.z();
	
    m = t->model_view;
	
    // Calculate the model_view transformation matrix for this tile
    FG_MEM_COPY( m, MODEL_VIEW, 16*sizeof(GLdouble) );
    
    // This is equivalent to doing a glTranslatef(x, y, z);
    m[12] += (m[0]*x + m[4]*y + m[8] *z);
    m[13] += (m[1]*x + m[5]*y + m[9] *z);
    m[14] += (m[2]*x + m[6]*y + m[10]*z);
    // m[15] += (m[3]*x + m[7]*y + m[11]*z);
    // m[3] m7[] m[11] are 0.0 see LookAt() in views.cxx
    // so m[15] is unchanged
}


// Render the local tiles
void FGTileMgr::render( void ) {
    FGInterface *f;
    FGTileCache *c;
    FGTileEntry *t;
    FGView *v;
    Point3D frag_offset;
    fgFRAGMENT *frag_ptr;
    FGMaterialSlot *mtl_ptr;
    int i;
    int tile_diameter;
    int index;
    int culled = 0;
    int drawn = 0;

    c = &global_tile_cache;
    f = current_aircraft.fdm_state;
    v = &current_view;

    tile_diameter = current_options.get_tile_diameter();

    // moved to fgTileMgrUpdate, right after we check if we need to
    // load additional tiles:
    // scenery.cur_elev = fgTileMgrCurElev( FG_Longitude, FG_Latitude, 
    //                                      v->abs_view_pos );
 
    // initialize the transient per-material fragment lists
    material_mgr.init_transient_material_lists();
   
    // Pass 1
    // traverse the potentially viewable tile list
    for ( i = 0; i < (tile_diameter * tile_diameter); i++ ) {
	index = tiles[i];
	// fgPrintf( FG_TERRAIN, FG_DEBUG, "Index = %d\n", index);
	t = c->get_tile(index);

	if ( t->is_loaded() ) {

	    // calculate tile offset
	    t->SetOffset( scenery.center );

	    // Course (tile based) culling
	    if ( viewable(t->offset, t->bounding_radius) ) {
		// at least a portion of this tile could be viewable
	    
		// Calculate the model_view transformation matrix for this tile
		// This is equivalent to doing a glTranslatef(x, y, z);
		t->update_view_matrix( v->get_MODEL_VIEW() );

		// xglPushMatrix();
		// xglTranslatef(t->offset.x, t->offset.y, t->offset.z);

		// traverse fragment list for tile
		FGTileEntry::FragmentIterator current = t->begin();
		FGTileEntry::FragmentIterator last = t->end();

		for ( ; current != last; ++current ) {
		    frag_ptr = &(*current);
		
		    if ( frag_ptr->display_list >= 0 ) {
			// Fine (fragment based) culling
			frag_offset = frag_ptr->center - scenery.center;

			if ( viewable(frag_offset, 
				      frag_ptr->bounding_radius*2) )
			{
			    // add to transient per-material property
			    // fragment list

			    // frag_ptr->tile_offset.x = t->offset.x;
			    // frag_ptr->tile_offset.y = t->offset.y;
			    // frag_ptr->tile_offset.z = t->offset.z;

			    mtl_ptr = frag_ptr->material_ptr;
			    // printf(" lookup = %s\n", mtl_ptr->texture_name);
			    if ( ! mtl_ptr->append_sort_list( frag_ptr ) ) {
				FG_LOG( FG_TERRAIN, FG_ALERT,
					"Overran material sorting array" );
			    }

			    // xglCallList(frag_ptr->display_list);
			    drawn++;
			} else {
			    // printf("Culled a fragment %.2f %.2f %.2f %.2f\n",
			    //        frag_ptr->center.x, frag_ptr->center.y,
			    //        frag_ptr->center.z, 
			    //        frag_ptr->bounding_radius);
			    culled++;
			}
		    }
		}

		// xglPopMatrix();
	    } else {
		culled += t->fragment_list.size();
	    }
	} else {
	    FG_LOG( FG_TERRAIN, FG_DEBUG, "Skipping a not yet loaded tile" );
	}
    }

    if ( (drawn + culled) > 0 ) {
	v->set_vfc_ratio( (double)culled / (double)(drawn + culled) );
    } else {
	v->set_vfc_ratio( 0.0 );
    }
    // printf("drawn = %d  culled = %d  saved = %.2f\n", drawn, culled, 
    //        v->vfc_ratio);

    // Pass 2
    // traverse the transient per-material fragment lists and render
    // out all fragments for each material property.
    xglPushMatrix();
    material_mgr.render_fragments();
    xglPopMatrix();
}
