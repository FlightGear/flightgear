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


#include <stdio.h>

#include <list>
#include <map>

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif

#include "convex_hull.hxx"
#include "point2d.hxx"

// calculate the convex hull of a set of points, return as a list of
// point2d
list_container convex_hull( list_container input_list )
{
    list_iterator current, last;
    map_iterator map_current, map_last;

    // list of translated points
    list_container trans_list;

    // points sorted by radian degrees
    map_container radians_map;

    // will contain the convex hull
    list_container con_hull;

    point2d p, average;
    double sum_x, sum_y;
    int in_count;

    // STEP ONE:  Find an average midpoint of the input set of points
    current = input_list.begin();
    last = input_list.end();
    in_count = input_list.size();
    sum_x = sum_y = 0.0;

    while ( current != last ) {
	sum_x += (*current).x;
	sum_y += (*current).y;

	current++;
    }

    average.x = sum_x / in_count;
    average.y = sum_y / in_count;

    printf("Average center point is %.4f %.4f\n", average.x, average.y);

    // STEP TWO:  Translate input points so average is at origin
    current = input_list.begin();
    last = input_list.end();
    trans_list.erase( trans_list.begin(), trans_list.end() );

    while ( current != last ) {
	p.x = (*current).x - average.x;
	p.y = (*current).y - average.y;
	printf("p is %.6f %.6f\n", p.x, p.y);
	trans_list.push_back(p);
	current++;
    }

    // STEP THREE:  convert to radians and sort by theta
    current = trans_list.begin();
    last = trans_list.end();
    radians_map.erase( radians_map.begin(), radians_map.end() );

    while ( current != last ) {
	p = cart_to_polar_2d(*current);
	radians_map[p.theta] = p.dist;
	current++;
    }

    printf("Sorted list\n");
    map_current = radians_map.begin();
    map_last = radians_map.end();
    while ( map_current != map_last ) {
	p.x = (*map_current).first;
	p.y = (*map_current).second;

	printf("p is %.6f %.6f\n", p.x, p.y);

	map_current++;
    }

    return con_hull;
}


// $Log$
// Revision 1.1  1998/09/04 23:04:51  curt
// Beginning of convex hull genereration routine.
//
//
