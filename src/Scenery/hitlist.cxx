#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <float.h>
#include <math.h>

#include <GL/glut.h>
#include <GL/gl.h>

#include <simgear/constants.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/vector.hxx>

#include <Main/globals.hxx>
#include <Main/viewer.hxx>

#include "hitlist.hxx"


extern ssgBranch *terrain;


// check to see if the intersection point is
// actually inside this face
static bool sgdPointInTriangle( sgdVec3 point, sgdVec3 tri[3] )
{
    double xmin, xmax, ymin, ymax, zmin, zmax;
	
    // punt if outside bouding cube
    if ( point[0] < (xmin = SG_MIN3 (tri[0][0], tri[1][0], tri[2][0])) ) {
        return false;
    } else if ( point[0] > (xmax = SG_MAX3 (tri[0][0], tri[1][0], tri[2][0])) ) {
        return false;
    } else if ( point[1] < (ymin = SG_MIN3 (tri[0][1], tri[1][1], tri[2][1])) ) {
        return false;
    } else if ( point[1] > (ymax = SG_MAX3 (tri[0][1], tri[1][1], tri[2][1])) ) {
        return false;
    } else if ( point[2] < (zmin = SG_MIN3 (tri[0][2], tri[1][2], tri[2][2])) ) {
        return false;
    } else if ( point[2] > (zmax = SG_MAX3 (tri[0][2], tri[1][2], tri[2][2])) ) {
        return false;
    }

    // (finally) check to see if the intersection point is
    // actually inside this face

    //first, drop the smallest dimension so we only have to work
    //in 2d.
    double dx = xmax - xmin;
    double dy = ymax - ymin;
    double dz = zmax - zmin;
    double min_dim = SG_MIN3 (dx, dy, dz);

    //first, drop the smallest dimension so we only have to work
    //in 2d.
    double x1, y1, x2, y2, x3, y3, rx, ry;
    if ( fabs(min_dim-dx) <= SG_EPSILON ) {
        // x is the smallest dimension
        x1 = point[1];
        y1 = point[2];
        x2 = tri[0][1];
        y2 = tri[0][2];
        x3 = tri[1][1];
        y3 = tri[1][2];
        rx = tri[2][1];
        ry = tri[2][2];
    } else if ( fabs(min_dim-dy) <= SG_EPSILON ) {
        // y is the smallest dimension
        x1 = point[0];
        y1 = point[2];
        x2 = tri[0][0];
        y2 = tri[0][2];
        x3 = tri[1][0];
        y3 = tri[1][2];
        rx = tri[2][0];
        ry = tri[2][2];
    } else if ( fabs(min_dim-dz) <= SG_EPSILON ) {
        // z is the smallest dimension
        x1 = point[0];
        y1 = point[1];
        x2 = tri[0][0];
        y2 = tri[0][1];
        x3 = tri[1][0];
        y3 = tri[1][1];
        rx = tri[2][0];
        ry = tri[2][1];
    } else {
        // all dimensions are really small so lets call it close
        // enough and return a successful match
        return true;
    }
    
    // check if intersection point is on the same side of p1 <-> p2 as p3
    double tmp = (y2 - y3) / (x2 - x3);
    int side1 = SG_SIGN (tmp * (rx - x3) + y3 - ry);
    int side2 = SG_SIGN (tmp * (x1 - x3) + y3 - y1);
    if ( side1 != side2 ) {
        // printf("failed side 1 check\n");
        return false;
    }

    // check if intersection point is on correct side of p2 <-> p3 as p1
    tmp = (y3 - ry) / (x3 - rx);
    side1 = SG_SIGN (tmp * (x2 - rx) + ry - y2);
    side2 = SG_SIGN (tmp * (x1 - rx) + ry - y1);
    if ( side1 != side2 ) {
        // printf("failed side 2 check\n");
        return false;
    }

    // check if intersection point is on correct side of p1 <-> p3 as p2
    tmp = (y2 - ry) / (x2 - rx);
    side1 = SG_SIGN (tmp * (x3 - rx) + ry - y3);
    side2 = SG_SIGN (tmp * (x1 - rx) + ry - y1);
    if ( side1 != side2 ) {
        // printf("failed side 3  check\n");
        return false;
    }

    return true;
}

static int sgdIsectInfLinePlane( sgdVec3 dst, const sgdVec3 l_org,
                                 const sgdVec3 l_vec, const sgdVec4 plane )
{
    SGDfloat tmp = sgdScalarProductVec3 ( l_vec, plane ) ;

  /* Is line parallel to plane? */

    if ( fabs ( tmp ) < FLT_EPSILON )
        return false ;

    sgdScaleVec3 ( dst, l_vec, -( sgdScalarProductVec3 ( l_org, plane )
                                 + plane[3] ) / tmp ) ;
    sgdAddVec3  ( dst, l_org ) ;

    return true ;
}


static void sgdXformPnt3 ( sgdVec3 dst, const sgVec3 src, const sgdMat4 mat )
{
    SGDfloat t0 = src[ 0 ] ;
    SGDfloat t1 = src[ 1 ] ;
    SGDfloat t2 = src[ 2 ] ;

    dst[0] = ( t0 * mat[ 0 ][ 0 ] +
               t1 * mat[ 1 ][ 0 ] +
               t2 * mat[ 2 ][ 0 ] +
               mat[ 3 ][ 0 ] ) ;

    dst[1] = ( t0 * mat[ 0 ][ 1 ] +
               t1 * mat[ 1 ][ 1 ] +
               t2 * mat[ 2 ][ 1 ] +
               mat[ 3 ][ 1 ] ) ;

    dst[2] = ( t0 * mat[ 0 ][ 2 ] +
               t1 * mat[ 1 ][ 2 ] +
               t2 * mat[ 2 ][ 2 ] +
               mat[ 3 ][ 2 ] ) ;
}

/*
   Find the intersection of an infinite line with a leaf
   the line being defined by a point and direction.

   Variables
    In:
     ssgLeaf pointer  -- leaf
     qualified matrix -- m
     line origin      -- orig
     line direction   -- dir
    Out:
     result  -- intersection point
     normal  -- intersected tri's normal

   Returns:
    true if intersection found
    false otherwise
*/
int FGHitList::IntersectLeaf( ssgLeaf *leaf, sgdMat4 m,
							  sgdVec3 orig, sgdVec3 dir )
{
    int num_hits = 0;
    for ( int i = 0; i < leaf->getNumTriangles(); ++i ) {
	short i1, i2, i3;
	leaf->getTriangle( i, &i1, &i2, &i3 );

	sgdVec3 tri[3];
	sgdXformPnt3( tri[0], leaf->getVertex( i1 ), m );
	sgdXformPnt3( tri[1], leaf->getVertex( i2 ), m );
	sgdXformPnt3( tri[2], leaf->getVertex( i3 ), m );

        //avoid division by zero when two points are the same
        if ( sgdEqualVec3(tri[0], tri[1]) ||
             sgdEqualVec3(tri[1], tri[2]) ||
             sgdEqualVec3(tri[2], tri[0]) ) {
            continue;
        }

	sgdVec4 plane;
	sgdMakePlane( plane, tri[0], tri[1], tri[2] );

	sgdVec3 point;
	if( sgdIsectInfLinePlane( point, orig, dir, plane ) ) {
	    if( sgdPointInTriangle( point, tri ) ) {
		add(leaf,i,point,plane);
		num_hits++;
	    }
	}
    }
    return num_hits;
}

void FGHitList::IntersectBranch( ssgBranch *branch, sgdMat4 m, 
				 sgdVec3 orig, sgdVec3 dir )
{
    sgSphere *bsphere;
    for ( ssgEntity *kid = branch->getKid( 0 );
	  kid != NULL; 
	  kid = branch->getNextKid() )
    {
	if ( kid->getTraversalMask() & SSGTRAV_HOT ) {
            bsphere = kid->getBSphere();
	    sgVec3 fcenter;
	    sgCopyVec3( fcenter, bsphere->getCenter() );
	    sgdVec3 center;
	    center[0] = fcenter[0]; 
	    center[1] = fcenter[1];
	    center[2] = fcenter[2];
	    sgdXformPnt3( center, m ) ;
            // watch out for overflow
	    if ( sgdClosestPointToLineDistSquared( center, orig, dir ) <
		 double(bsphere->getRadius() * bsphere->getRadius()) )
	    {
		// possible intersections
		if ( kid->isAKindOf ( ssgTypeBranch() ) ) {
		    sgdMat4 m_new;
		    sgdCopyMat4(m_new, m);
		    if ( kid->isA( ssgTypeTransform() ) ) {
                        sgMat4 fxform;
                        ((ssgTransform *)kid)->getTransform( fxform );
                        sgdMat4 xform;
                        sgdSetMat4( xform, fxform );
                        sgdPreMultMat4( m_new, xform );
		    }
		    IntersectBranch( (ssgBranch *)kid, m_new, orig, dir );
		} else if ( kid->isAKindOf ( ssgTypeLeaf() ) ) {
		    IntersectLeaf( (ssgLeaf *)kid, m, orig, dir );
		}
	    } else {
		// end of the line for this branch
	    }
	} else {
	    // branch requested not to be traversed
	}
    }
}


// This expects the inital m to the identity transform
void ssgGetEntityTransform(ssgEntity *branch, sgMat4 m )
{
    for ( ssgEntity *parent = branch->getParent(0);
          parent != NULL; 
          parent = parent->getNextParent() )
    {
        // recurse till we are at the scene root
        // then just unwinding the stack should 
        // give us our cumulative transform :-)  NHV
        ssgGetEntityTransform( parent, m );
        if ( parent->isA( ssgTypeTransform() ) ) {
            sgMat4 xform;
            ((ssgTransform *)parent)->getTransform( xform );
            sgPreMultMat4( m, xform );
        }
    }
}


// return the passed entitity's bsphere's center point radius and
// fully formed current model matrix for entity
void ssgGetCurrentBSphere( ssgEntity *entity, sgVec3 center,
						   float *radius, sgMat4 m )
{
    sgSphere *bsphere = entity->getBSphere();
    *radius = (double)bsphere->getRadius();
    sgCopyVec3( center, bsphere->getCenter() );
    sgMakeIdentMat4 ( m ) ;
    ssgGetEntityTransform( entity, m );
}


void FGHitList::IntersectCachedLeaf( sgdMat4 m,
				     sgdVec3 orig, sgdVec3 dir)
{
    if ( last_hit() ) {
	float radius;
	sgVec3 fcenter;
	sgMat4 fxform;
	// ssgEntity *ent = last_hit();
	ssgGetCurrentBSphere( last_hit(), fcenter, &radius, fxform );
	sgdMat4 m;
	sgdVec3 center;
	sgdSetMat4( m, fxform );
	sgdXformPnt3( center, m );

	if ( sgdClosestPointToLineDistSquared( center, orig, dir ) <
	     double(radius*radius) )
	{
	    IntersectLeaf( (ssgLeaf *)last_hit(), m, orig, dir );
	}
    }
}


static void CurrentNormalInLocalPlane(sgVec3 dst, sgVec3 src) {
    sgVec3 tmp;
    sgSetVec3(tmp, src[0], src[1], src[2] );
    sgMat4 TMP;
    sgTransposeNegateMat4 ( TMP, globals->get_current_view()->get_UP() ) ;
    sgXformVec3(tmp, tmp, TMP);
    sgSetVec3(dst, tmp[2], tmp[1], tmp[0] );
}


// a temporary hack until we get everything rewritten with sgdVec3
static inline Point3D operator + (const Point3D& a, const sgdVec3 b)
{
    return Point3D(a.x()+b[0], a.y()+b[1], a.z()+b[2]);
}


// Determine scenery altitude via ssg.  Normally this just happens
// when we render the scene, but we'd also like to be able to do this
// explicitely.  lat & lon are in radians.  view_pos in current world
// coordinate translated near (0,0,0) (in meters.)  Returns result in
// meters.
bool fgCurrentElev( sgdVec3 abs_view_pos, sgdVec3 scenery_center,
		    FGHitList *hit_list,
		    double *terrain_elev, double *radius, double *normal)
{
    sgdVec3 view_pos;
    sgdSubVec3( view_pos, abs_view_pos, scenery_center );

    sgdVec3 orig, dir;
    sgdCopyVec3(orig, view_pos );
    sgdCopyVec3(dir, abs_view_pos );

    hit_list->Intersect( terrain, orig, dir );

    int this_hit=0;
    Point3D geoc;
    double result = -9999;
    Point3D sc(scenery_center[0], scenery_center[1], scenery_center[2]) ;
    
    int hitcount = hit_list->num_hits();
    for ( int i = 0; i < hitcount; ++i ) {
        geoc = sgCartToPolar3d( sc + hit_list->get_point(i) );      
        double lat_geod, alt, sea_level_r;
        sgGeocToGeod(geoc.lat(), geoc.radius(), &lat_geod, 
                     &alt, &sea_level_r);
        if ( alt > result && alt < 10000 ) {
            result = alt;
            this_hit = i;
        }
    }

    if ( result > -9000 ) {
        *terrain_elev = result;
        *radius = geoc.radius();
        sgVec3 tmp;
        sgSetVec3(tmp, hit_list->get_normal(this_hit));
        // cout << "cur_normal: " << tmp[0] << " " << tmp[1] << " "
        //      << tmp[2] << endl;
        /* ssgState *IntersectedLeafState =
           ((ssgLeaf*)hit_list->get_entity(this_hit))->getState(); */
        CurrentNormalInLocalPlane(tmp, tmp);
        sgdSetVec3( normal, tmp );
        // cout << "NED: " << tmp[0] << " " << tmp[1] << " " << tmp[2] << endl;
        return true;
    } else {
        SG_LOG( SG_TERRAIN, SG_INFO, "no terrain intersection" );
        *terrain_elev = 0.0;
        float *up = globals->get_current_view()->get_world_up();
        sgdSetVec3(normal, up[0], up[1], up[2]);
        return false;
    }
}
