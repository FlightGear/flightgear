// fragment.cxx -- routines to handle "atomic" display objects
//
// Written by Curtis Olson, started August 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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


#include <Include/fg_constants.h>
#include <Math/mat3.h>
#include <Math/point3d.hxx>
#include <Scenery/tile.hxx>

#include "fragment.hxx"


template <class T>
inline const int FG_SIGN(const T& x) {
    return x < T(0) ? -1 : 1;
}

template <class T>
inline const T& FG_MIN(const T& a, const T& b) {
    return b < a ? b : a;
}

template <class T>
inline const T& FG_MAX(const T& a, const T& b) {
    return  a < b ? b : a;
}

// return the minimum of the three values
template <class T>
inline const T& fg_min3( const T& a, const T& b, const T& c)
{
    return (a > b ? FG_MIN (b, c) : FG_MIN (a, c));
}


// return the maximum of the three values
template <class T>
inline const T& fg_max3 (const T& a, const T& b, const T& c)
{
    return (a < b ? FG_MAX (b, c) : FG_MAX (a, c));
}

// Add a face to the face list
// Copy constructor
fgFRAGMENT::fgFRAGMENT ( const fgFRAGMENT & rhs ) :
    center         ( rhs.center          ),
    bounding_radius( rhs.bounding_radius ),
    material_ptr   ( rhs.material_ptr    ),
    tile_ptr       ( rhs.tile_ptr        ),
    display_list   ( rhs.display_list    ),
    faces          ( rhs.faces           )
{
}

fgFRAGMENT & fgFRAGMENT::operator = ( const fgFRAGMENT & rhs )
{
    if(!(this == &rhs )) {
	center          = rhs.center;
	bounding_radius = rhs.bounding_radius;
	material_ptr    = rhs.material_ptr;
	tile_ptr        = rhs.tile_ptr;
	// display_list    = rhs.display_list;
	faces           = rhs.faces;
    }
    return *this;
}


// test if line intesects with this fragment.  p0 and p1 are the two
// line end points of the line.  If side_flag is true, check to see
// that end points are on opposite sides of face.  Returns 1 if it
// intersection found, 0 otherwise.  If it intesects, result is the
// point of intersection

int fgFRAGMENT::intersect( const Point3D& end0,
			   const Point3D& end1,
			   int side_flag,
			   Point3D& result) const
{
    fgTILE *t;
    MAT3vec v1, v2, n, center;
    double p1[3], p2[3], p3[3];
    double x, y, z;  // temporary holding spot for result
    double a, b, c, d;
    double x0, y0, z0, x1, y1, z1, a1, b1, c1;
    double t1, t2, t3;
    double xmin, xmax, ymin, ymax, zmin, zmax;
    double dx, dy, dz, min_dim, x2, y2, x3, y3, rx, ry;
    int side1, side2;

    // find the associated tile
    t = tile_ptr;

    // printf("Intersecting\n");

    // traverse the face list for this fragment
    const_iterator last = faces.end();
    for ( const_iterator current = faces.begin(); current != last; ++current )
    {
	// printf(".");

	// get face vertex coordinates
	center[0] = t->center.x();
	center[1] = t->center.y();
	center[2] = t->center.z();

	MAT3_ADD_VEC(p1, t->nodes[(*current).n1], center);
	MAT3_ADD_VEC(p2, t->nodes[(*current).n2], center);
	MAT3_ADD_VEC(p3, t->nodes[(*current).n3], center);

	// printf("point 1 = %.2f %.2f %.2f\n", p1[0], p1[1], p1[2]);
	// printf("point 2 = %.2f %.2f %.2f\n", p2[0], p2[1], p2[2]);
	// printf("point 3 = %.2f %.2f %.2f\n", p3[0], p3[1], p3[2]);

	// calculate two edge vectors, and the face normal
	MAT3_SUB_VEC(v1, p2, p1);
	MAT3_SUB_VEC(v2, p3, p1);
	MAT3cross_product(n, v1, v2);

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
	x0 = end0.x();  x1 = end1.x();
	y0 = end0.y();  y1 = end1.y();
	z0 = end0.z();  z1 = end1.z();

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
		result = Point3D(x, y, z);
		return(1);
	    }
	    side1 = FG_SIGN (t1 - t2);
	    side2 = FG_SIGN (t1 - t3);
	    if ( side1 == side2 ) {
		// same side, punt
		continue;
	    }
	}

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
	    result = Point3D(x, y, z);
	    return(1);
	}

	// check if intersection point is on the same side of p1 <-> p2 as p3
	t1 = (y1 - y2) / (x1 - x2);
	side1 = FG_SIGN (t1 * ((x3) - x2) + y2 - (y3));
	side2 = FG_SIGN (t1 * ((rx) - x2) + y2 - (ry));
	if ( side1 != side2 ) {
	    // printf("failed side 1 check\n");
	    continue;
	}

	// check if intersection point is on correct side of p2 <-> p3 as p1
	t1 = (y2 - y3) / (x2 - x3);
	side1 = FG_SIGN (t1 * ((x1) - x3) + y3 - (y1));
	side2 = FG_SIGN (t1 * ((rx) - x3) + y3 - (ry));
	if ( side1 != side2 ) {
	    // printf("failed side 2 check\n");
	    continue;
	}

	// check if intersection point is on correct side of p1 <-> p3 as p2
	t1 = (y1 - y3) / (x1 - x3);
	side1 = FG_SIGN (t1 * ((x2) - x3) + y3 - (y2));
	side2 = FG_SIGN (t1 * ((rx) - x3) + y3 - (ry));
	if ( side1 != side2 ) {
	    // printf("failed side 3  check\n");
	    continue;
	}

	// printf( "intersection point = %.2f %.2f %.2f\n", x, y, z);
	result = Point3D(x, y, z);
	return(1);
    }

    // printf("\n");

    return(0);
}

// $Log$
// Revision 1.1  1999/04/05 21:32:49  curt
// Initial revision
//
// Revision 1.7  1999/02/26 22:09:56  curt
// Added initial support for native SGI compilers.
//
// Revision 1.6  1998/10/18 01:17:20  curt
// Point3D tweaks.
//
// Revision 1.5  1998/10/16 00:54:37  curt
// Converted to Point3D class.
//
// Revision 1.4  1998/09/15 01:35:03  curt
// cleaned up my fragment.num_faces hack :-) to use the STL (no need in
// duplicating work.)
// Tweaked fgTileMgrRender() do not calc tile matrix unless necessary.
// removed some unneeded stuff from fgTileMgrCurElev()
//
// Revision 1.3  1998/09/08 21:40:42  curt
// Updates from Bernie Bright.
//
// Revision 1.2  1998/09/01 19:03:07  curt
// Changes contributed by Bernie Bright <bbright@c031.aone.net.au>
//  - The new classes in libmisc.tgz define a stream interface into zlib.
//    I've put these in a new directory, Lib/Misc.  Feel free to rename it
//    to something more appropriate.  However you'll have to change the
//    include directives in all the other files.  Additionally you'll have
//    add the library to Lib/Makefile.am and Simulator/Main/Makefile.am.
//
//    The StopWatch class in Lib/Misc requires a HAVE_GETRUSAGE autoconf
//    test so I've included the required changes in config.tgz.
//
//    There are a fair few changes to Simulator/Objects as I've moved
//    things around.  Loading tiles is quicker but thats not where the delay
//    is.  Tile loading takes a few tenths of a second per file on a P200
//    but it seems to be the post-processing that leads to a noticeable
//    blip in framerate.  I suppose its time to start profiling to see where
//    the delays are.
//
//    I've included a brief description of each archives contents.
//
// Lib/Misc/
//   zfstream.cxx
//   zfstream.hxx
//     C++ stream interface into zlib.
//     Taken from zlib-1.1.3/contrib/iostream/.
//     Minor mods for STL compatibility.
//     There's no copyright associated with these so I assume they're
//     covered by zlib's.
//
//   fgstream.cxx
//   fgstream.hxx
//     FlightGear input stream using gz_ifstream.  Tries to open the
//     given filename.  If that fails then filename is examined and a
//     ".gz" suffix is removed or appended and that file is opened.
//
//   stopwatch.hxx
//     A simple timer for benchmarking.  Not used in production code.
//     Taken from the Blitz++ project.  Covered by GPL.
//
//   strutils.cxx
//   strutils.hxx
//     Some simple string manipulation routines.
//
// Simulator/Airports/
//   Load airports database using fgstream.
//   Changed fgAIRPORTS to use set<> instead of map<>.
//   Added bool fgAIRPORTS::search() as a neater way doing the lookup.
//   Returns true if found.
//
// Simulator/Astro/
//   Modified fgStarsInit() to load stars database using fgstream.
//
// Simulator/Objects/
//   Modified fgObjLoad() to use fgstream.
//   Modified fgMATERIAL_MGR::load_lib() to use fgstream.
//   Many changes to fgMATERIAL.
//   Some changes to fgFRAGMENT but I forget what!
//
// Revision 1.1  1998/08/25 16:51:23  curt
// Moved from ../Scenery
//
//

