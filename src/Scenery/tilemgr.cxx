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

#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "scenery.hxx"
#include "tilecache.hxx"
#include "tileentry.hxx"
#include "tilemgr.hxx"


// to test clipping speedup in fgTileMgrRender()
#if defined ( USE_FAST_FOV_CLIP )
// #define TEST_FOV_CLIP
// #define TEST_ELEV
#endif


extern ssgRoot *scene;


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

    global_tile_cache.init();

    state = Inited;

    return 1;
}


// schedule a tile for loading
static void disable_tile( int cache_index ) {
    // see if tile already exists in the cache
    // cout << "DISABLING CACHE ENTRY = " << cache_index << endl;
    FGTileEntry *t = global_tile_cache.get_tile( cache_index );
    t->ssg_disable();
}


// schedule a tile for loading
int FGTileMgr::sched_tile( const FGBucket& b ) {
    // see if tile already exists in the cache
    int cache_index = global_tile_cache.exists( b );

    if ( cache_index >= 0 ) {
	// tile exists in cache, reenable it.
	// cout << "REENABLING DISABLED TILE" << endl;
	FGTileEntry *t = global_tile_cache.get_tile( cache_index );
	t->select_ptr->select( 1 );
	t->mark_loaded();
    } else {
	// find the next available cache entry and mark it as
	// scheduled
	cache_index = global_tile_cache.next_avail();
	FGTileEntry *t = global_tile_cache.get_tile( cache_index );
	t->mark_scheduled_for_use();

	// register a load request
	FGLoadRec request;
	request.b = b;
	request.cache_index = cache_index;
	load_queue.push_back( request );
    }

    return cache_index;
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


inline int fg_sign( const double x ) {
    return x < 0 ? -1 : 1;
}

inline double fg_min( const double a, const double b ) {
    return b < a ? b : a;
}

inline double fg_max( const double a, const double b ) {
    return a < b ? b : a;
}

// return the minimum of the three values
inline double fg_min3( const double a, const double b, const double c ) {
    return a > b ? fg_min(b, c) : fg_min(a, c);
}

// return the maximum of the three values
inline double fg_max3 (const double a, const double b, const double c ) {
    return a < b ? fg_max(b, c) : fg_max(a, c);
}


// check for an instersection with the individual triangles of a leaf
static bool my_ssg_instersect_leaf( string s, ssgLeaf *leaf, sgMat4 m,
				    const sgVec3 p, const sgVec3 dir,
				    sgVec3 result )
{
    sgVec3 v1, v2, n;
    sgVec3 p1, p2, p3;
    double x, y, z;  // temporary holding spot for result
    double a, b, c, d;
    double x0, y0, z0, x1, y1, z1, a1, b1, c1;
    double t1, t2, t3;
    double xmin, xmax, ymin, ymax, zmin, zmax;
    double dx, dy, dz, min_dim, x2, y2, x3, y3, rx, ry;
    float *tmp;
    int side1, side2;
    short i1, i2, i3;

    // cout << s << "Intersecting" << endl;

    // traverse the triangle list for this leaf
    for ( int i = 0; i < leaf->getNumTriangles(); ++i ) {
	// cout << s << "testing triangle = " << i << endl;

	leaf->getTriangle( i, &i1, &i2, &i3 );

	// get triangle vertex coordinates

	tmp = leaf->getVertex( i1 );
	// cout << s << "orig point 1 = " << tmp[0] << " " << tmp[1] 
	//      << " " << tmp[2] << endl;
	sgXformPnt3( p1, tmp, m ) ;

	tmp = leaf->getVertex( i2 );
	// cout << s << "orig point 2 = " << tmp[0] << " " << tmp[1] 
	//      << " " << tmp[2] << endl;
	sgXformPnt3( p2, tmp, m ) ;

	tmp = leaf->getVertex( i3 );
	// cout << s << "orig point 3 = " << tmp[0] << " " << tmp[1] 
	//      << " " << tmp[2] << endl;
	sgXformPnt3( p3, tmp, m ) ;

	// cout << s << "point 1 = " << p1[0] << " " << p1[1] << " " << p1[2]
	//      << endl;
	// cout << s << "point 2 = " << p2[0] << " " << p2[1] << " " << p2[2]
	//      << endl;
	// cout << s << "point 3 = " << p3[0] << " " << p3[1] << " " << p3[2]
	//      << endl;

	// calculate two edge vectors, and the face normal
	sgSubVec3(v1, p2, p1);
	sgSubVec3(v2, p3, p1);
	sgVectorProductVec3(n, v1, v2);

	// calculate the plane coefficients for the plane defined by
	// this face.  If n is the normal vector, n = (a, b, c) and p1
	// is a point on the plane, p1 = (x0, y0, z0), then the
	// equation of the line is a(x-x0) + b(y-y0) + c(z-z0) = 0
	a = n[0];
	b = n[1];
	c = n[2];
	d = a * p1[0] + b * p1[1] + c * p1[2];
	// printf("a, b, c, d = %.2f %.2f %.2f %.2f\n", a, b, c, d);

	// printf("p1(d) = %.2f\n", a * p1[0] + b * p1[1] + c * p1[2]);
	// printf("p2(d) = %.2f\n", a * p2[0] + b * p2[1] + c * p2[2]);
	// printf("p3(d) = %.2f\n", a * p3[0] + b * p3[1] + c * p3[2]);

	// calculate the line coefficients for the specified line
	x0 = p[0];  x1 = p[0] + dir[0];
	y0 = p[1];  y1 = p[1] + dir[1];
	z0 = p[2];  z1 = p[2] + dir[2];

	if ( fabs(x1 - x0) > FG_EPSILON ) {
	    a1 = 1.0 / (x1 - x0);
	} else {
	    // we got a big divide by zero problem here
	    a1 = 0.0;
	}
	b1 = y1 - y0;
	c1 = z1 - z0;

	// intersect the specified line with this plane
	t1 = b * b1 * a1;
	t2 = c * c1 * a1;

	// printf("a = %.2f  t1 = %.2f  t2 = %.2f\n", a, t1, t2);

	if ( fabs(a + t1 + t2) > FG_EPSILON ) {
	    x = (t1*x0 - b*y0 + t2*x0 - c*z0 + d) / (a + t1 + t2);
	    t3 = a1 * (x - x0);
	    y = b1 * t3 + y0;
	    z = c1 * t3 + z0;	    
	    // printf("result(d) = %.2f\n", a * x + b * y + c * z);
	} else {
	    // no intersection point
	    continue;
	}

#if 0
	if ( side_flag ) {
	    // check to see if end0 and end1 are on opposite sides of
	    // plane
	    if ( (x - x0) > FG_EPSILON ) {
		t1 = x;
		t2 = x0;
		t3 = x1;
	    } else if ( (y - y0) > FG_EPSILON ) {
		t1 = y;
		t2 = y0;
		t3 = y1;
	    } else if ( (z - z0) > FG_EPSILON ) {
		t1 = z;
		t2 = z0;
		t3 = z1;
	    } else {
		// everything is too close together to tell the difference
		// so the current intersection point should work as good
		// as any
		sgSetVec3( result, x, y, z );
		return true;
	    }
	    side1 = fg_sign (t1 - t2);
	    side2 = fg_sign (t1 - t3);
	    if ( side1 == side2 ) {
		// same side, punt
		continue;
	    }
	}
#endif

	// check to see if intersection point is in the bounding
	// cube of the face
#ifdef XTRA_DEBUG_STUFF
	xmin = fg_min3 (p1[0], p2[0], p3[0]);
	xmax = fg_max3 (p1[0], p2[0], p3[0]);
	ymin = fg_min3 (p1[1], p2[1], p3[1]);
	ymax = fg_max3 (p1[1], p2[1], p3[1]);
	zmin = fg_min3 (p1[2], p2[2], p3[2]);
	zmax = fg_max3 (p1[2], p2[2], p3[2]);
	printf("bounding cube = %.2f,%.2f,%.2f  %.2f,%.2f,%.2f\n",
	       xmin, ymin, zmin, xmax, ymax, zmax);
#endif
	// punt if outside bouding cube
	if ( x < (xmin = fg_min3 (p1[0], p2[0], p3[0])) ) {
	    continue;
	} else if ( x > (xmax = fg_max3 (p1[0], p2[0], p3[0])) ) {
	    continue;
	} else if ( y < (ymin = fg_min3 (p1[1], p2[1], p3[1])) ) {
	    continue;
	} else if ( y > (ymax = fg_max3 (p1[1], p2[1], p3[1])) ) {
	    continue;
	} else if ( z < (zmin = fg_min3 (p1[2], p2[2], p3[2])) ) {
	    continue;
	} else if ( z > (zmax = fg_max3 (p1[2], p2[2], p3[2])) ) {
	    continue;
	}

	// (finally) check to see if the intersection point is
	// actually inside this face

	//first, drop the smallest dimension so we only have to work
	//in 2d.
	dx = xmax - xmin;
	dy = ymax - ymin;
	dz = zmax - zmin;
	min_dim = fg_min3 (dx, dy, dz);
	if ( fabs(min_dim - dx) <= FG_EPSILON ) {
	    // x is the smallest dimension
	    x1 = p1[1];
	    y1 = p1[2];
	    x2 = p2[1];
	    y2 = p2[2];
	    x3 = p3[1];
	    y3 = p3[2];
	    rx = y;
	    ry = z;
	} else if ( fabs(min_dim - dy) <= FG_EPSILON ) {
	    // y is the smallest dimension
	    x1 = p1[0];
	    y1 = p1[2];
	    x2 = p2[0];
	    y2 = p2[2];
	    x3 = p3[0];
	    y3 = p3[2];
	    rx = x;
	    ry = z;
	} else if ( fabs(min_dim - dz) <= FG_EPSILON ) {
	    // z is the smallest dimension
	    x1 = p1[0];
	    y1 = p1[1];
	    x2 = p2[0];
	    y2 = p2[1];
	    x3 = p3[0];
	    y3 = p3[1];
	    rx = x;
	    ry = y;
	} else {
	    // all dimensions are really small so lets call it close
	    // enough and return a successful match
	    sgSetVec3( result, x, y, z );
	    return true;
	}

	// check if intersection point is on the same side of p1 <-> p2 as p3
	t1 = (y1 - y2) / (x1 - x2);
	side1 = fg_sign (t1 * ((x3) - x2) + y2 - (y3));
	side2 = fg_sign (t1 * ((rx) - x2) + y2 - (ry));
	if ( side1 != side2 ) {
	    // printf("failed side 1 check\n");
	    continue;
	}

	// check if intersection point is on correct side of p2 <-> p3 as p1
	t1 = (y2 - y3) / (x2 - x3);
	side1 = fg_sign (t1 * ((x1) - x3) + y3 - (y1));
	side2 = fg_sign (t1 * ((rx) - x3) + y3 - (ry));
	if ( side1 != side2 ) {
	    // printf("failed side 2 check\n");
	    continue;
	}

	// check if intersection point is on correct side of p1 <-> p3 as p2
	t1 = (y1 - y3) / (x1 - x3);
	side1 = fg_sign (t1 * ((x2) - x3) + y3 - (y2));
	side2 = fg_sign (t1 * ((rx) - x3) + y3 - (ry));
	if ( side1 != side2 ) {
	    // printf("failed side 3  check\n");
	    continue;
	}

	// printf( "intersection point = %.2f %.2f %.2f\n", x, y, z);
	sgSetVec3( result, x, y, z );
	return true;
    }

    // printf("\n");

    return false;
}


void FGTileMgr::my_ssg_los( string s, ssgBranch *branch, sgMat4 m, 
			    const sgVec3 p, const sgVec3 dir )
{
    sgSphere *bsphere;
    for ( ssgEntity *kid = branch->getKid( 0 );
	  kid != NULL; 
	  kid = branch->getNextKid() )
    {
	if ( kid->getTraversalMask() & SSGTRAV_HOT ) {
	    bsphere = kid->getBSphere();
	    sgVec3 center;
	    sgCopyVec3( center, bsphere->getCenter() );
	    sgXformPnt3( center, m ) ;
	    // cout << s << "entity bounding sphere:" << endl;
	    // cout << s << "center = " << center[0] << " "
	    //      << center[1] << " " << center[2] << endl;
    	    // cout << s << "radius = " << bsphere->getRadius() << endl;
	    double radius_sqd = bsphere->getRadius() * bsphere->getRadius();
	    if ( sgPointLineDistSquared( center, p, dir ) < radius_sqd ) {
		// possible intersections
		if ( kid->isAKindOf ( ssgTypeBranch() ) ) {
		    sgMat4 m_new;
		    sgCopyMat4(m_new, m);
		    if ( kid->isA( ssgTypeTransform() ) ) {
			sgMat4 xform;
			((ssgTransform *)kid)->getTransform( xform );
			sgPreMultMat4( m_new, xform );
		    }
		    my_ssg_los( s + " ", (ssgBranch *)kid, m_new, p, dir );
		} else if ( kid->isAKindOf ( ssgTypeLeaf() ) ) {
		    sgVec3 result;
		    if ( my_ssg_instersect_leaf( s, (ssgLeaf *)kid, m, p, dir, 
						 result ) )
		    {
			cout << "sgLOS hit: " << result[0] << "," 
			     << result[1] << "," << result[2] << endl;
			hit_pts[hitcount] = result;
			hitcount++;
		    }
		}
	    } else {
		// end of the line for this branch
	    }
	} else {
	    // branch requested not to be traversed
	}
    }
}


// Determine scenery altitude via ssg.  Normally this just happens
// when we render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  view_pos in current world
// coordinate translated near (0,0,0) (in meters.)  Returns result in
// meters.
double
FGTileMgr::current_elev_ssg( const Point3D& abs_view_pos, 
			     const Point3D& view_pos )
{
    hitcount = 0;

    sgMat4 m;
    sgMakeIdentMat4 ( m ) ;

    sgVec3 sgavp, sgvp;
    sgSetVec3(sgavp, abs_view_pos.x(), abs_view_pos.y(), abs_view_pos.z() );
    sgSetVec3(sgvp, view_pos.x(), view_pos.y(), view_pos.z() );

    cout << "starting ssg_los, abs view pos = " << abs_view_pos[0] << " "
         << abs_view_pos[1] << " " << abs_view_pos[2] << endl;
    cout << "starting ssg_los, view pos = " << view_pos[0] << " "
         << view_pos[1] << " " << view_pos[2] << endl;
    my_ssg_los( "", scene, m, sgvp, sgavp );

    double result = -9999;

    for ( int i = 0; i < hitcount; ++i ) {
	Point3D rel_cart( hit_pts[i][0], hit_pts[i][1], hit_pts[i][2] );
	Point3D abs_cart = rel_cart + scenery.center;
	Point3D pp = fgCartToPolar3d( abs_cart );
	FG_LOG( FG_TERRAIN, FG_INFO, "  polar form = " << pp );
	// convert to geodetic coordinates
	double lat_geod, alt, sea_level_r;
	fgGeocToGeod(pp.lat(), pp.radius(), &lat_geod, 
		     &alt, &sea_level_r);

	// printf("alt = %.2f\n", alt);
	// exit since we found an intersection
	if ( alt > result && alt < 10000 ) {
	    // printf("returning alt\n");
	    result = alt;
	}
    }

    if ( result > -9000 ) {
	return result;
    } else {
	FG_LOG( FG_TERRAIN, FG_INFO, "no terrain intersection" );
	return 0.0;
    }
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

	// First time through or we have teleported, initialize the
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
	sched_tile( p2 );

	// prime scenery center calculations
	Point3D geod_center( p2.get_center_lon() * RAD_TO_DEG,
			     p2.get_center_lat() * RAD_TO_DEG, 0.0 );
	scenery.center = scenery.next_center = fgGeodToCart( geod_center );

	Point3D geod_view_center( p2.get_center_lon() * RAD_TO_DEG, 
				  p2.get_center_lat() * RAD_TO_DEG, 
				  cur_fdm_state->get_Altitude()*FEET_TO_METER +
				  3 );
	current_view.abs_view_pos = fgGeodToCart( geod_view_center );
	current_view.view_pos = current_view.abs_view_pos - scenery.center;

	for ( i = 3; i <= tile_diameter; i = i + 2 ) {
	    int span = i / 2;

	    // bottom row
	    for ( j = -span; j <= span; ++j ) {
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     j, -span );
		sched_tile( p2 );
	    }

	    // top row
	    for ( j = -span; j <= span; ++j ) {
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     j, span );
		sched_tile( p2 );
	    }

	    // middle rows
	    for ( j = -span + 1; j <= span - 1; ++j ) {
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     -span, j );
		sched_tile( p2 );
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     span, j );
		sched_tile( p2 );
	    }

	}

	/* for ( j = 0; j < tile_diameter; j++ ) {
	    for ( i = 0; i < tile_diameter; i++ ) {
		// fgBucketOffset(&p1, &p2, i - dw, j - dh);
		p2 = fgBucketOffset( f->get_Longitude() * RAD_TO_DEG,
				     f->get_Latitude() * RAD_TO_DEG,
				     i - dw, j -dh );
		sched_tile( p2 );
	    }
	} */

	// Now force a load of the center tile and inner ring so we
	// have something to see in our first frame.
	for ( i = 0; i < 9; ++i ) {
	    if ( load_queue.size() ) {
		FG_LOG( FG_TERRAIN, FG_DEBUG, 
			"Load queue not empty, loading a tile" );
	    
		FGLoadRec pending = load_queue.front();
		load_queue.pop_front();
		load_tile( pending.b, pending.cache_index );
	    }
	}

    } else {
	// We've moved to a new bucket, we need to scroll our
        // structures, and load in the new tiles

#if 0 
	// make sure load queue is flushed before doing shift
	while ( load_queue.size() ) {
	    FG_LOG( FG_TERRAIN, FG_DEBUG, 
		    "Load queue not empty, flushing queue before tile shift." );
	    
	    FGLoadRec pending = load_queue.front();
	    load_queue.pop_front();
	    load_tile( pending.b, pending.index );
	}
#endif

	// CURRENTLY THIS ASSUMES WE CAN ONLY MOVE TO ADJACENT TILES.
	// AT ULTRA HIGH SPEEDS THIS ASSUMPTION MAY NOT BE VALID IF
	// THE AIRCRAFT CAN SKIP A TILE IN A SINGLE ITERATION.

	FG_LOG( FG_TERRAIN, FG_INFO, "Updating Tile list for " << p1 );

	if ( (p1.get_lon() > p_last.get_lon()) ||
	     ( (p1.get_lon() == p_last.get_lon()) && 
	       (p1.get_x() > p_last.get_x()) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  (East) Loading " << tile_diameter << " tiles" );
	    for ( j = 0; j < tile_diameter; j++ ) {
		// scrolling East
		// schedule new column
		p2 = fgBucketOffset( last_lon, last_lat, dw + 1, j - dh );
		sched_tile( p2 );
	    }
	} else if ( (p1.get_lon() < p_last.get_lon()) ||
		    ( (p1.get_lon() == p_last.get_lon()) && 
		      (p1.get_x() < p_last.get_x()) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  (West) Loading " << tile_diameter << " tiles" );
	    for ( j = 0; j < tile_diameter; j++ ) {
		// scrolling West
		// schedule new column
		p2 = fgBucketOffset( last_lon, last_lat, -dw - 1, j - dh );
		sched_tile( p2 );
	    }
	}

	if ( (p1.get_lat() > p_last.get_lat()) ||
	     ( (p1.get_lat() == p_last.get_lat()) && 
	       (p1.get_y() > p_last.get_y()) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  (North) Loading " << tile_diameter << " tiles" );
	    for ( i = 0; i < tile_diameter; i++ ) {
		// scrolling North
		// schedule new row
		p2 = fgBucketOffset( last_lon, last_lat, i - dw, dh + 1);
		sched_tile( p2 );
	    }
	} else if ( (p1.get_lat() < p_last.get_lat()) ||
		    ( (p1.get_lat() == p_last.get_lat()) && 
		      (p1.get_y() < p_last.get_y()) ) ) {
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "  (South) Loading " << tile_diameter << " tiles" );
	    for ( i = 0; i < tile_diameter; i++ ) {
		// scrolling South
		// schedule new row
		p2 = fgBucketOffset( last_lon, last_lat, i - dw, -dh - 1);
		sched_tile( p2 );
	    }
	}
    }

    if ( load_queue.size() ) {
	FG_LOG( FG_TERRAIN, FG_DEBUG, "Load queue not empty, loading a tile" );

	FGLoadRec pending = load_queue.front();
	load_queue.pop_front();
	load_tile( pending.b, pending.cache_index );
    }

    // find our current elevation (feed in the current bucket to save work)
    Point3D geod_pos = Point3D( f->get_Longitude(), f->get_Latitude(), 0.0);
    Point3D tmp_abs_view_pos = fgGeodToCart(geod_pos);

    cout << "current elevation (old) == " 
	 << current_elev( f->get_Longitude(), f->get_Latitude(), 
			  tmp_abs_view_pos ) 
	 << endl;
    scenery.cur_elev = current_elev_ssg( current_view.abs_view_pos,
					 current_view.view_pos );
    cout << "current elevation (ssg) == " << scenery.cur_elev << endl;
	
    p_last = p1;
    last_lon = f->get_Longitude() * RAD_TO_DEG;
    last_lat = f->get_Latitude() * RAD_TO_DEG;

    return 1;
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


// Prepare the ssg nodes ... for each tile, set it's proper
// transform and update it's range selector based on current
// visibilty
void FGTileMgr::prep_ssg_nodes( void ) {
    FGTileEntry *t;

    float ranges[2];
    ranges[0] = 0.0f;

    // traverse the potentially viewable tile list and update range
    // selector and transform
    for ( int i = 0; i < (int)global_tile_cache.get_size(); i++ ) {
	t = global_tile_cache.get_tile( i );

	if ( t->is_loaded() ) {
	    // set range selector (LOD trick) to be distance to center
	    // of tile + bounding radius
#ifndef FG_OLD_WEATHER
	    ranges[1] = WeatherDatabase->getWeatherVisibility()
		+ t->bounding_radius;
#else
	    ranges[1] = current_weather.get_visibility()+t->bounding_radius;
#endif
	    t->range_ptr->setRanges( ranges, 2 );

	    // calculate tile offset
	    t->SetOffset( scenery.center );

	    // calculate ssg transform
	    sgCoord sgcoord;
	    sgSetCoord( &sgcoord,
			t->offset.x(), t->offset.y(), t->offset.z(),
			0.0, 0.0, 0.0 );
	    t->transform_ptr->setTransform( &sgcoord );
	}
    }
}
