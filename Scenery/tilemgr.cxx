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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <Aircraft/aircraft.hxx>

#include <Bucket/bucketutils.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Math/fg_geodesy.hxx>
#include <Math/mat3.h>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Math/vector.hxx>
#include <Objects/material.hxx>
#include <Objects/obj.hxx>
#include <Weather/weather.hxx>

#include "scenery.hxx"
#include "tile.hxx"
#include "tilecache.hxx"
#include "tilemgr.hxx"


// to test clipping speedup in fgTileMgrRender()
#if defined ( USE_FAST_FOV_CLIP )
  // #define TEST_FOV_CLIP
  // #define TEST_ELEV
#endif


#define FG_LOCAL_X_Y         81  // max(o->tile_diameter) ** 2

#define FG_SQUARE( X ) ( (X) * (X) )

#if defined(USE_MEM) || defined(WIN32)
#  define FG_MEM_COPY(to,from,n)        memcpy(to, from, n)
#else
#  define FG_MEM_COPY(to,from,n)        bcopy(from, to, n)
#endif

// closest (potentially viewable) tiles, centered on current tile.
// This is an array of pointers to cache indexes.
int tiles[FG_LOCAL_X_Y];


// Initialize the Tile Manager subsystem
int fgTileMgrInit( void ) {
    FG_LOG( FG_TERRAIN, FG_INFO, "Initializing Tile Manager subsystem." );

    // load default material library
    material_mgr.load_lib();

    return 1;
}


// load a tile
void fgTileMgrLoadTile( const fgBUCKET& p, int *index) {
    fgTILECACHE *c;

    c = &global_tile_cache;

    FG_LOG( FG_TERRAIN, FG_DEBUG, "Updating for bucket " << p );
    
    // if not in cache, load tile into the next available slot
    *index = c->exists(p);
    if ( *index < 0 ) {
	*index = c->next_avail();
	c->fill_in(*index, p);
    }

    FG_LOG( FG_TERRAIN, FG_DEBUG, "Selected cache index: " << *index );
}


// Calculate shortest distance from point to line
static double point_line_dist_squared( const Point3D& tc, const Point3D& vp, 
				       MAT3vec d )
{
    MAT3vec p, p0;
    double dist;

    p[0] = tc.x(); p[1] = tc.y(); p[2] = tc.z();
    p0[0] = vp.x(); p0[1] = vp.y(); p0[2] = vp.z();

    dist = fgPointLineSquared(p, p0, d);

    // cout << "dist = " << dist << endl;

    return(dist);
}


// Determine scenery altitude.  Normally this just happens when we
// render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  abs_view_pos in meters.
// Returns result in meters.
double
fgTileMgrCurElev( const fgBUCKET& p ) {
    fgTILE *t;
    fgFRAGMENT *frag_ptr;
    Point3D abs_view_pos = current_view.abs_view_pos;
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
    
    // earth_center = Point3D(0.0, 0.0, 0.0);

    FG_LOG( FG_TERRAIN, FG_DEBUG, 
	    "Current bucket = " << p << "  Index = " << fgBucketGenIndex(&p) );

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
        fgTILE::FragmentIterator current = t->begin();
        fgTILE::FragmentIterator last = t->end();

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

    cout << "no terrain intersection found\n";
    return 0.0;
}


// Determine scenery altitude.  Normally this just happens when we
// render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  abs_view_pos in meters.
// Returns result in meters.
double
fgTileMgrCurElevOLD( double lon, double lat, const Point3D& abs_view_pos ) {
    fgTILECACHE *c;
    fgTILE *t;
    // fgVIEW *v;
    fgFRAGMENT *frag_ptr;
    fgBUCKET p;
    Point3D earth_center(0.0);
    Point3D result;
    MAT3vec local_up;
    double dist, lat_geod, alt, sea_level_r;
    // double x, y, z;
    int index;

    c = &global_tile_cache;
    // v = &current_view;

    local_up[0] = abs_view_pos.x();
    local_up[1] = abs_view_pos.y();
    local_up[2] = abs_view_pos.z();

    // Find current translation offset
    fgBucketFind(lon * RAD_TO_DEG, lat * RAD_TO_DEG, &p);
    index = c->exists(p);
    if ( index < 0 ) {
	FG_LOG( FG_TERRAIN, FG_WARN, "Tile not found" );
	return 0.0;
    }

    t = c->get_tile(index);

    scenery.next_center = t->center;
    
    // earth_center = Point3D(0.0, 0.0, 0.0);

    FG_LOG( FG_TERRAIN, FG_DEBUG, 
	    "Pos = (" << lon * RAD_TO_DEG << ", " << lat * RAD_TO_DEG
	    << ")  Current bucket = " << p 
	    << "  Index = " << fgBucketGenIndex(&p) );

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
        fgTILE::FragmentIterator current = t->begin();
        fgTILE::FragmentIterator last = t->end();

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

    cout << "no terrain intersection found\n";
    return 0.0;
}


// given the current lon/lat, fill in the array of local chunks.  If
// the chunk isn't already in the cache, then read it from disk.
int fgTileMgrUpdate( void ) {
    fgTILECACHE *c;
    fgFLIGHT *f;
    fgBUCKET p1, p2;
    static fgBUCKET p_last = {-1000, 0, 0, 0};
    int tile_diameter;
    int i, j, dw, dh;

    c = &global_tile_cache;
    f = current_aircraft.flight;

    tile_diameter = current_options.get_tile_diameter();

    fgBucketFind( f->get_Longitude() * RAD_TO_DEG,
		  f->get_Latitude() * RAD_TO_DEG, &p1);
    dw = tile_diameter / 2;
    dh = tile_diameter / 2;

    if ( p1 == p_last ) {
	// same bucket as last time
	FG_LOG( FG_TERRAIN, FG_DEBUG, "Same bucket as last time" );
    } else if ( p_last.lon == -1000 ) {
	// First time through, initialize the system and load all
	// relavant tiles

	FG_LOG( FG_TERRAIN, FG_INFO, "  First time through ... " );
	FG_LOG( FG_TERRAIN, FG_INFO, "  Updating Tile list for " << p1 );
	FG_LOG( FG_TERRAIN, FG_INFO, "  Loading " 
		<< tile_diameter * tile_diameter << " tiles" );

	// wipe/initialize tile cache
	c->init();

	// build the local area list and update cache
	for ( j = 0; j < tile_diameter; j++ ) {
	    for ( i = 0; i < tile_diameter; i++ ) {
		fgBucketOffset(&p1, &p2, i - dw, j - dh);
		fgTileMgrLoadTile( p2, &tiles[(j*tile_diameter) + i]);
	    }
	}
    } else {
	// We've moved to a new bucket, we need to scroll our
        // structures, and load in the new tiles

	// CURRENTLY THIS ASSUMES WE CAN ONLY MOVE TO ADJACENT TILES.
	// AT ULTRA HIGH SPEEDS THIS ASSUMPTION MAY NOT BE VALID IF
	// THE AIRCRAFT CAN SKIP A TILE IN A SINGLE ITERATION.

	FG_LOG( FG_TERRAIN, FG_INFO, "Updating Tile list for " << p1 );

	if ( (p1.lon > p_last.lon) ||
	     ( (p1.lon == p_last.lon) && (p1.x > p_last.x) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  Loading " << tile_diameter << "tiles" );
	    for ( j = 0; j < tile_diameter; j++ ) {
		// scrolling East
		for ( i = 0; i < tile_diameter - 1; i++ ) {
		    tiles[(j*tile_diameter) + i] = 
			tiles[(j*tile_diameter) + i + 1];
		}
		// load in new column
		fgBucketOffset(&p_last, &p2, dw + 1, j - dh);
		fgTileMgrLoadTile( p2, &tiles[(j*tile_diameter) + 
					     tile_diameter - 1]);
	    }
	} else if ( (p1.lon < p_last.lon) ||
		    ( (p1.lon == p_last.lon) && (p1.x < p_last.x) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  Loading " << tile_diameter << "tiles" );
	    for ( j = 0; j < tile_diameter; j++ ) {
		// scrolling West
		for ( i = tile_diameter - 1; i > 0; i-- ) {
		    tiles[(j*tile_diameter) + i] = 
			tiles[(j*tile_diameter) + i - 1];
		}
		// load in new column
		fgBucketOffset(&p_last, &p2, -dw - 1, j - dh);
		fgTileMgrLoadTile( p2, &tiles[(j*tile_diameter) + 0]);
	    }
	}

	if ( (p1.lat > p_last.lat) ||
	     ( (p1.lat == p_last.lat) && (p1.y > p_last.y) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  Loading " << tile_diameter << "tiles" );
	    for ( i = 0; i < tile_diameter; i++ ) {
		// scrolling North
		for ( j = 0; j < tile_diameter - 1; j++ ) {
		    tiles[(j * tile_diameter) + i] =
			tiles[((j+1) * tile_diameter) + i];
		}
		// load in new column
		fgBucketOffset(&p_last, &p2, i - dw, dh + 1);
		fgTileMgrLoadTile( p2, &tiles[((tile_diameter-1) * 
					       tile_diameter) + i]);
	    }
	} else if ( (p1.lat < p_last.lat) ||
		    ( (p1.lat == p_last.lat) && (p1.y < p_last.y) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  Loading " << tile_diameter << "tiles" );
	    for ( i = 0; i < tile_diameter; i++ ) {
		// scrolling South
		for ( j = tile_diameter - 1; j > 0; j-- ) {
		    tiles[(j * tile_diameter) + i] = 
			tiles[((j-1) * tile_diameter) + i];
		}
		// load in new column
		fgBucketOffset(&p_last, &p2, i - dw, -dh - 1);
		fgTileMgrLoadTile( p2, &tiles[0 + i]);
	    }
	}
    }

    // find our current elevation (feed in the current bucket to save work)
    scenery.cur_elev = fgTileMgrCurElev( p1 );

    p_last.lon = p1.lon;
    p_last.lat = p1.lat;
    p_last.x = p1.x;
    p_last.y = p1.y;

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
	
    mat = (double *)(current_view.WORLD_TO_EYE);
	
    eye[2] =  x*mat[2] + y*mat[6] + z*mat[10] + mat[14];
	
    // Check near and far clip plane
    if( ( eye[2] > radius ) ||
	( eye[2] + radius + current_weather.visibility < 0) )
    {
	return(0);
    }
	
    eye[0] = (x*mat[0] + y*mat[4] + z*mat[8] + mat[12]) * current_view.slope_x;

    // check right and left clip plane (from eye perspective)
    x1 = radius * current_view.fov_x_clip;
    if( (eye[2] > -(eye[0]+x1)) || (eye[2] > (eye[0]-x1)) )
    {
	return(0);
    }
	
    eye[1] = (x*mat[1] + y*mat[5] + z*mat[9] + mat[13]) * current_view.slope_y;

    // check bottom and top clip plane (from eye perspective)
    y1 = radius * current_view.fov_y_clip;
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
    if ( eye[2] + radius < -current_weather.visibility ) {
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
update_tile_geometry( fgTILE *t, GLdouble *MODEL_VIEW)
{
    GLdouble *m;
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
void fgTileMgrRender( void ) {
    fgFLIGHT *f;
    fgTILECACHE *c;
    fgTILE *t;
    fgVIEW *v;
    Point3D frag_offset;
    fgFRAGMENT *frag_ptr;
    fgMATERIAL *mtl_ptr;
    int i;
    int tile_diameter;
    int index;
    int culled = 0;
    int drawn = 0;

    c = &global_tile_cache;
    f = current_aircraft.flight;
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

	// calculate tile offset
	t->SetOffset( scenery.center );

	// Course (tile based) culling
	if ( viewable(t->offset, t->bounding_radius) ) {
	    // at least a portion of this tile could be viewable
	    
	    // Calculate the model_view transformation matrix for this tile
	    // This is equivalent to doing a glTranslatef(x, y, z);
	    t->UpdateViewMatrix( v->MODEL_VIEW );

	    // xglPushMatrix();
	    // xglTranslatef(t->offset.x, t->offset.y, t->offset.z);

	    // traverse fragment list for tile
            fgTILE::FragmentIterator current = t->begin();
            fgTILE::FragmentIterator last = t->end();

	    for ( ; current != last; ++current ) {
		frag_ptr = &(*current);
		
		if ( frag_ptr->display_list >= 0 ) {
		    // Fine (fragment based) culling
		    frag_offset = frag_ptr->center - scenery.center;

		    if ( viewable(frag_offset, frag_ptr->bounding_radius*2) ) {
			// add to transient per-material property fragment list
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
			//        frag_ptr->center.z, frag_ptr->bounding_radius);
			culled++;
		    }
		}
	    }

	    // xglPopMatrix();
	} else {
	    culled += t->fragment_list.size();
	}
    }

    if ( (drawn + culled) > 0 ) {
	v->vfc_ratio = (double)culled / (double)(drawn + culled);
    } else {
	v->vfc_ratio = 0.0;
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


// $Log$
// Revision 1.47  1998/12/05 14:11:19  curt
// Sun portability tweak.
//
// Revision 1.46  1998/12/03 14:15:24  curt
// Actually set the current scenery elevation based on scenery intersection point
// rather than calculating the intesection point and throwing it away.
//
// Revision 1.45  1998/12/03 01:18:18  curt
// Converted fgFLIGHT to a class.
// Tweaks for Sun Portability.
// Tweaked current terrain elevation code as per NHV.
//
// Revision 1.44  1998/11/23 21:49:48  curt
// minor tweaks.
//
// Revision 1.43  1998/11/09 23:40:52  curt
// Bernie Bright <bbright@c031.aone.net.au> writes:
// I've made some changes to the Scenery handling.  Basically just tidy ups.
// The main difference is in tile.[ch]xx where I've changed list<fgFRAGMENT> to
// vector<fgFRAGMENT>.  Studying our usage patterns this seems reasonable.
// Lists are good if you need to insert/delete elements randomly but we
// don't do that.  All access seems to be sequential.  Two additional
// benefits are smaller memory usage - each list element requires pointers
// to the next and previous elements, and faster access - vector iterators
// are smaller and faster than list iterators.  This should also help
// Charlie Hotchkiss' problem when compiling with Borland and STLport.
//
// ./Lib/Bucket/bucketutils.hxx
//   Convenience functions for fgBUCKET.
//
// ./Simulator/Scenery/tile.cxx
// ./Simulator/Scenery/tile.hxx
//   Changed fragment list to a vector.
//   Added some convenience member functions.
//
// ./Simulator/Scenery/tilecache.cxx
// ./Simulator/Scenery/tilecache.hxx
//   use const fgBUCKET& instead of fgBUCKET* where appropriate.
//
// ./Simulator/Scenery/tilemgr.cxx
// ./Simulator/Scenery/tilemgr.hxx
//   uses all the new convenience functions.
//
// Revision 1.42  1998/11/06 21:18:23  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.41  1998/10/18 01:17:23  curt
// Point3D tweaks.
//
// Revision 1.40  1998/10/17 01:34:28  curt
// C++ ifying ...
//
// Revision 1.39  1998/10/16 00:55:50  curt
// Converted to Point3D class.
//
// Revision 1.38  1998/09/17 18:36:18  curt
// Tweaks and optimizations by Norman Vine.
//
// Revision 1.37  1998/09/15 01:36:45  curt
// cleaned up my fragment.num_faces hack :-) to use the STL (no need in
// duplicating work.)
// Tweaked fgTileMgrRender() do not calc tile matrix unless necessary.
// removed some unneeded stuff from fgTileMgrCurElev()
//
// Revision 1.36  1998/09/14 12:45:26  curt
// minor tweaks.
//
// Revision 1.35  1998/09/10 19:07:16  curt
// /Simulator/Objects/fragment.hxx
//   Nested fgFACE inside fgFRAGMENT since its not used anywhere else.
//
// ./Simulator/Objects/material.cxx
// ./Simulator/Objects/material.hxx
//   Made fgMATERIAL and fgMATERIAL_MGR bona fide classes with private
//   data members - that should keep the rabble happy :)
//
// ./Simulator/Scenery/tilemgr.cxx
//   In viewable() delay evaluation of eye[0] and eye[1] in until they're
//   actually needed.
//   Change to fgTileMgrRender() to call fgMATERIAL_MGR::render_fragments()
//   method.
//
// ./Include/fg_stl_config.h
// ./Include/auto_ptr.hxx
//   Added support for g++ 2.7.
//   Further changes to other files are forthcoming.
//
// Brief summary of changes required for g++ 2.7.
//   operator->() not supported by iterators: use (*i).x instead of i->x
//   default template arguments not supported,
//   <functional> doesn't have mem_fun_ref() needed by callbacks.
//   some std include files have different names.
//   template member functions not supported.
//
// Revision 1.34  1998/09/09 20:58:09  curt
// Tweaks to loop constructs with STL usage.
//
// Revision 1.33  1998/09/08 15:05:10  curt
// Optimization by Norman Vine.
//
// Revision 1.32  1998/08/25 16:52:44  curt
// material.cxx material.hxx obj.cxx obj.hxx texload.c texload.h moved to
//   ../Objects
//
// Revision 1.31  1998/08/24 20:11:40  curt
// Tweaks ...
//
// Revision 1.30  1998/08/22  14:49:59  curt
// Attempting to iron out seg faults and crashes.
// Did some shuffling to fix a initialization order problem between view
// position, scenery elevation.
//
// Revision 1.29  1998/08/20 15:12:06  curt
// Used a forward declaration of classes fgTILE and fgMATERIAL to eliminate
// the need for "void" pointers and casts.
// Quick hack to count the number of scenery polygons that are being drawn.
//
// Revision 1.28  1998/08/12 21:13:06  curt
// material.cxx: don't load textures if they are disabled
// obj.cxx: optimizations from Norman Vine
// tile.cxx: minor tweaks
// tile.hxx: addition of num_faces
// tilemgr.cxx: minor tweaks
//
// Revision 1.27  1998/07/24 21:42:09  curt
// material.cxx: whups, double method declaration with no definition.
// obj.cxx: tweaks to avoid errors in SGI's CC.
// tile.cxx: optimizations by Norman Vine.
// tilemgr.cxx: optimizations by Norman Vine.
//
// Revision 1.26  1998/07/20 12:51:26  curt
// Added far clip plane to fragment clipping calculations and tie this to
// weather->visibility.  This way you can increase framerates by increasing
// for and lowering visibility.
//
// Revision 1.25  1998/07/13 21:02:01  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.24  1998/07/12 03:18:29  curt
// Added ground collision detection.  This involved:
// - saving the entire vertex list for each tile with the tile records.
// - saving the face list for each fragment with the fragment records.
// - code to intersect the current vertical line with the proper face in
//   an efficient manner as possible.
// Fixed a bug where the tiles weren't being shifted to "near" (0,0,0)
//
// Revision 1.23  1998/07/08 14:47:23  curt
// Fix GL_MODULATE vs. GL_DECAL problem introduced by splash screen.
// polare3d.h renamed to polar3d.hxx
// fg{Cartesian,Polar}Point3d consolodated.
// Added some initial support for calculating local current ground elevation.
//
// Revision 1.22  1998/07/04 00:54:31  curt
// Added automatic mipmap generation.
//
// When rendering fragments, use saved model view matrix from associated tile
// rather than recalculating it with push() translate() pop().
//
// Revision 1.21  1998/06/27 16:54:59  curt
// Check for GL_VERSION_1_1 or GL_EXT_texture_object to decide whether to use
//   "EXT" versions of texture management routines.
//
// Revision 1.20  1998/06/17 21:36:42  curt
// Load and manage multiple textures defined in the Materials library.
// Boost max material fagments for each material property to 800.
// Multiple texture support when rendering.
//
// Revision 1.19  1998/06/08 17:57:54  curt
// Working first pass at material proporty sorting.
//
// Revision 1.18  1998/06/06 01:09:32  curt
// I goofed on the log message in the last commit ... now fixed.
//
// Revision 1.17  1998/06/06 01:07:18  curt
// Increased per material fragment list size from 100 to 400.
// Now correctly draw viewable fragments in per material order.
//
// Revision 1.16  1998/06/05 22:39:55  curt
// Working on sorting by, and rendering by material properties.
//
// Revision 1.15  1998/06/03 00:47:51  curt
// No .h for STL includes.
// Minor view culling optimizations.
//
// Revision 1.14  1998/06/01 17:56:20  curt
// Incremental additions to material.cxx (not fully functional)
// Tweaked vfc_ratio math to avoid divide by zero.
//
// Revision 1.13  1998/05/24 02:49:10  curt
// Implimented fragment level view frustum culling.
//
// Revision 1.12  1998/05/23 14:09:23  curt
// Added tile.cxx and tile.hxx.
// Working on rewriting the tile management system so a tile is just a list
// fragments, and the fragment record contains the display list for that fragment.
//
// Revision 1.11  1998/05/20 20:53:55  curt
// Moved global ref point and radius (bounding sphere info, and offset) to
// data file rather than calculating it on the fly.
// Fixed polygon winding problem in scenery generation stage rather than
// compensating for it on the fly.
// Made a fgTILECACHE class.
//
// Revision 1.10  1998/05/17 16:59:34  curt
// Frist pass at view frustum culling now operational.
//
// Revision 1.9  1998/05/16 13:09:58  curt
// Beginning to add support for view frustum culling.
// Added some temporary code to calculate bouding radius, until the
//   scenery generation tools and scenery can be updated.
//
// Revision 1.8  1998/05/07 23:15:21  curt
// Fixed a glTexImage2D() usage bug where width and height were mis-swapped.
// Added support for --tile-radius=n option.
//
// Revision 1.7  1998/05/06 03:16:42  curt
// Added an option to control square tile radius.
//
// Revision 1.6  1998/05/02 01:52:18  curt
// Playing around with texture coordinates.
//
// Revision 1.5  1998/04/30 12:35:32  curt
// Added a command line rendering option specify smooth/flat shading.
//
// Revision 1.4  1998/04/27 03:30:14  curt
// Minor transformation adjustments to try to keep scenery tiles closer to
// (0, 0, 0)  GLfloats run out of precision at the distances we need to model
// the earth, but we can do a bunch of pre-transformations using double math
// and then cast to GLfloat once everything is close in where we have less
// precision problems.
//
// Revision 1.3  1998/04/25 22:06:32  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.2  1998/04/24 00:51:09  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Tweaked the scenery file extentions to be "file.obj" (uncompressed)
// or "file.obz" (compressed.)
//
// Revision 1.1  1998/04/22 13:22:48  curt
// C++ - ifing the code a bit.
//
// Revision 1.25  1998/04/18 04:14:07  curt
// Moved fg_debug.c to it's own library.
//
// Revision 1.24  1998/04/14 02:23:18  curt
// Code reorganizations.  Added a Lib/ directory for more general libraries.
//
// Revision 1.23  1998/04/08 23:30:08  curt
// Adopted Gnu automake/autoconf system.
//
// Revision 1.22  1998/04/03 22:11:38  curt
// Converting to Gnu autoconf system.
//
// Revision 1.21  1998/03/23 21:23:05  curt
// Debugging output tweaks.
//
// Revision 1.20  1998/03/14 00:30:51  curt
// Beginning initial terrain texturing experiments.
//
// Revision 1.19  1998/02/20 00:16:25  curt
// Thursday's tweaks.
//
// Revision 1.18  1998/02/19 13:05:54  curt
// Incorporated some HUD tweaks from Michelle America.
// Tweaked the sky's sunset/rise colors.
// Other misc. tweaks.
//
// Revision 1.17  1998/02/16 13:39:46  curt
// Miscellaneous weekend tweaks.  Fixed? a cache problem that caused whole
// tiles to occasionally be missing.
//
// Revision 1.16  1998/02/12 21:59:53  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.14  1998/02/09 21:30:19  curt
// Fixed a nagging problem with terrain tiles not "quite" matching up perfectly.
//
// Revision 1.13  1998/02/07 15:29:46  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.12  1998/02/01 03:39:55  curt
// Minor tweaks.
//
// Revision 1.11  1998/01/31 00:43:27  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.10  1998/01/29 00:51:40  curt
// First pass at tile cache, dynamic tile loading and tile unloading now works.
//
// Revision 1.9  1998/01/27 03:26:44  curt
// Playing with new fgPrintf command.
//
// Revision 1.8  1998/01/27 00:48:04  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.7  1998/01/26 15:55:25  curt
// Progressing on building dynamic scenery system.
//
// Revision 1.6  1998/01/24 00:03:30  curt
// Initial revision.
//
// Revision 1.5  1998/01/19 19:27:18  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.4  1998/01/19 18:40:38  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.3  1998/01/13 00:23:11  curt
// Initial changes to support loading and management of scenery tiles.  Note,
// there's still a fair amount of work left to be done.
//
// Revision 1.2  1998/01/08 02:22:27  curt
// Continue working on basic features.
//
// Revision 1.1  1998/01/07 23:50:51  curt
// "area" renamed to "tile"
//
// Revision 1.2  1998/01/07 03:29:29  curt
// Given an arbitrary lat/lon, we can now:
//   generate a unique index for the chunk containing the lat/lon
//   generate a path name to the chunk file
//   build a list of the indexes of all the nearby areas.
//
// Revision 1.1  1998/01/07 02:05:48  curt
// Initial revision.
