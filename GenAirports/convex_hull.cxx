// convex_hull.cxx -- calculate the convex hull of a set of points
//
// Written by Curtis Olson, started September 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)
//


#include <math.h>
#include <stdio.h>

#include <list>
#include <map>

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif

#include <Include/fg_constants.h>

#include "convex_hull.hxx"
#include "point2d.hxx"


// stl map typedefs
typedef map < double, double, less<double> > map_container;
typedef map_container::iterator map_iterator;


// Calculate theta of angle (a, b, c)
double calc_angle(point2d a, point2d b, point2d c) {
    point2d u, v;
    double udist, vdist, uv_dot, tmp;

    // u . v = ||u|| * ||v|| * cos(theta)

    u.x = b.x - a.x;
    u.y = b.y - a.y;
    udist = sqrt( u.x * u.x + u.y * u.y );
    // printf("udist = %.6f\n", udist);

    v.x = b.x - c.x;
    v.y = b.y - c.y;
    vdist = sqrt( v.x * v.x + v.y * v.y );
    // printf("vdist = %.6f\n", vdist);

    uv_dot = u.x * v.x + u.y * v.y;
    // printf("uv_dot = %.6f\n", uv_dot);

    tmp = uv_dot / (udist * vdist);
    // printf("tmp = %.6f\n", tmp);

    return acos(tmp);
}


// Test to see if angle(Pa, Pb, Pc) < 180 degrees
bool test_point(point2d Pa, point2d Pb, point2d Pc) {
    point2d origin, a, b, c;
    double a1, a2;

    origin.x = origin.y = 0.0;

    a.x = cos(Pa.theta) * Pa.dist;
    a.y = sin(Pa.theta) * Pa.dist;

    b.x = cos(Pb.theta) * Pb.dist;
    b.y = sin(Pb.theta) * Pb.dist;

    c.x = cos(Pc.theta) * Pc.dist;
    c.y = sin(Pc.theta) * Pc.dist;

    // printf("a is %.6f %.6f\n", a.x, a.y);
    // printf("b is %.6f %.6f\n", b.x, b.y);
    // printf("c is %.6f %.6f\n", c.x, c.y);

    a1 = calc_angle(a, b, origin);
    a2 = calc_angle(origin, b, c);

    // printf("a1 = %.2f  a2 = %.2f\n", a1 * RAD_TO_DEG, a2 * RAD_TO_DEG);

    return ( (a1 + a2) < FG_PI );
}


// calculate the convex hull of a set of points, return as a list of
// point2d.  The algorithm description can be found at:
// http://riot.ieor.berkeley.edu/riot/Applications/ConvexHull/CHDetails.html
list_container convex_hull( list_container input_list )
{
    list_iterator current, last;
    map_iterator map_current, map_next, map_next_next, map_last;

    // list of translated points
    list_container trans_list;

    // points sorted by radian degrees
    map_container radians_map;

    // will contain the convex hull
    list_container con_hull;

    point2d p, average, Pa, Pb, Pc, result;
    double sum_x, sum_y;
    int in_count, last_size;

    // STEP ONE:  Find an average midpoint of the input set of points
    current = input_list.begin();
    last = input_list.end();
    in_count = input_list.size();
    sum_x = sum_y = 0.0;

    for ( ; current != last ; ++current ) {
	sum_x += (*current).x;
	sum_y += (*current).y;
    }

    average.x = sum_x / in_count;
    average.y = sum_y / in_count;

    // printf("Average center point is %.4f %.4f\n", average.x, average.y);

    // STEP TWO:  Translate input points so average is at origin
    current = input_list.begin();
    last = input_list.end();
    trans_list.erase( trans_list.begin(), trans_list.end() );

    for ( ; current != last ; ++current ) {
	p.x = (*current).x - average.x;
	p.y = (*current).y - average.y;
	// printf("%.6f %.6f\n", p.x, p.y);
	trans_list.push_back(p);
    }

    // STEP THREE:  convert to radians and sort by theta
    current = trans_list.begin();
    last = trans_list.end();
    radians_map.erase( radians_map.begin(), radians_map.end() );

    for ( ; current != last ; ++current) {
	p = cart_to_polar_2d(*current);
	if ( p.dist > radians_map[p.theta] ) {
	    radians_map[p.theta] = p.dist;
	}
    }

    // printf("Sorted list\n");
    map_current = radians_map.begin();
    map_last = radians_map.end();
    for ( ; map_current != map_last ; ++map_current ) {
	p.x = (*map_current).first;
	p.y = (*map_current).second;

	// printf("p is %.6f %.6f\n", p.x, p.y);
    }

    // STEP FOUR: traverse the sorted list and eliminate everything
    // not on the perimeter.
    // printf("Traversing list\n");

    // double check list size ... this should never fail because a
    // single runway will always generate four points.
    if ( radians_map.size() < 3 ) {
	// printf("convex hull not possible with < 3 points\n");
	exit(0);
    }

    // ensure that we run the while loop at least once
    last_size = radians_map.size() + 1;

    while ( last_size > radians_map.size() ) {
	// printf("Running an iteration of the graham scan algorithm\n"); 
	last_size = radians_map.size();

	map_current = radians_map.begin();
	while ( map_current != radians_map.end() ) {
	    // get first element
	    Pa.theta = (*map_current).first;
	    Pa.dist = (*map_current).second;

	    // get second element
	    map_next = map_current;
	    ++map_next;
	    if ( map_next == radians_map.end() ) {
		map_next = radians_map.begin();
	    }
	    Pb.theta = (*map_next).first;
	    Pb.dist = (*map_next).second;

	    // get third element
	    map_next_next = map_next;
	    ++map_next_next;
	    if ( map_next_next == radians_map.end() ) {
		map_next_next = radians_map.begin();
	    }
	    Pc.theta = (*map_next_next).first;
	    Pc.dist = (*map_next_next).second;

	    // printf("Pa is %.6f %.6f\n", Pa.theta, Pa.dist);
	    // printf("Pb is %.6f %.6f\n", Pb.theta, Pb.dist);
	    // printf("Pc is %.6f %.6f\n", Pc.theta, Pc.dist);

	    if ( test_point(Pa, Pb, Pc) ) {
		// printf("Accepted a point\n");
		// accept point, advance Pa, Pb, and Pc.
		++map_current;
	    } else {
		// printf("REJECTED A POINT\n");
		// reject point, delete it and advance only Pb and Pc
		map_next = map_current;
		++map_next;
		if ( map_next == radians_map.end() ) {
		    map_next = radians_map.begin();
		}
		radians_map.erase( map_next );
	    }
	}
    }

    // translate back to correct lon/lat
    printf("Final sorted convex hull\n");
    con_hull.erase( con_hull.begin(), con_hull.end() );
    map_current = radians_map.begin();
    map_last = radians_map.end();
    for ( ; map_current != map_last ; ++map_current ) {
	p.theta = (*map_current).first;
	p.dist = (*map_current).second;

	result.x = cos(p.theta) * p.dist + average.x;
	result.y = sin(p.theta) * p.dist + average.y;

	printf("%.6f %.6f\n", result.x, result.y);

	con_hull.push_back(result);
    }

    return con_hull;
}


// $Log$
// Revision 1.3  1998/09/09 20:59:55  curt
// Loop construct tweaks for STL usage.
// Output airport file to be used to generate airport scenery on the fly
//   by the run time sim.
//
// Revision 1.2  1998/09/09 16:26:32  curt
// Continued progress in implementing the convex hull algorithm.
//
// Revision 1.1  1998/09/04 23:04:51  curt
// Beginning of convex hull genereration routine.
//
//
