// area.c -- routines to assist with inserting "areas" into FG terrain
//
// Written by Curtis Olson, started March 1998.
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

#include <Include/fg_constants.h>

#include "area.hxx"
#include "point2d.hxx"


// calc new x, y for a rotation
double rot_x(double x, double y, double theta) {
    return ( x * cos(theta) + y * sin(theta) );
}


// calc new x, y for a rotation
double rot_y(double x, double y, double theta) {
    return ( -x * sin(theta) + y * cos(theta) );
}


// calc new lon/lat given starting lon/lat, and offset radial, and
// distance.  NOTE: distance is specified in meters (and converted
// internally to radians)
point2d calc_lon_lat( point2d orig, point2d offset ) {
    point2d result;

    // printf("calc_lon_lat()  offset.theta = %.2f offset.dist = %.2f\n",
    //        offset.theta, offset.dist);

    offset.dist *= METER_TO_NM * NM_TO_RAD;

    result.lat = asin( sin(orig.lat) * cos(offset.dist) + 
		       cos(orig.lat) * sin(offset.dist) * cos(offset.theta) );

    if ( cos(result.lat) < FG_EPSILON ) {
        result.lon = orig.lon;      // endpoint a pole
    } else {
        result.lon = 
	    fmod(orig.lon - asin( sin(offset.theta) * sin(offset.dist) / 
				  cos(result.lat) ) + FG_PI, FG_2PI) - FG_PI;
    }

    return(result);
}


list < point2d >
batch_cart_to_polar_2d( list < point2d > in_list)
{
    list < point2d > out_list;
    list < point2d > :: iterator current;                          
    list < point2d > :: iterator last;                    
    point2d p;

    current = in_list.begin();
    last = in_list.end();
    while ( current != last ) {
	p = cart_to_polar_2d( *current );
	out_list.push_back(p);
	++current;
    }

    return out_list;
}


// given a set of 2d coordinates relative to a center point, and the
// lon, lat of that center point (specified in degrees), as well as a
// potential orientation angle, generate the corresponding lon and lat
// of the original 2d verticies.
list < point2d >
gen_area(point2d origin, double angle, list < point2d > cart_list)
{
    list < point2d > rad_list;
    list < point2d > result_list;
    list < point2d > :: iterator current;                          
    list < point2d > :: iterator last;                    
    point2d origin_rad, p;

    origin_rad.lon = origin.lon * DEG_TO_RAD;
    origin_rad.lat = origin.lat * DEG_TO_RAD;
	
    // convert to polar coordinates
    rad_list = batch_cart_to_polar_2d(cart_list);

    /*
    // display points
    printf("converted to polar\n");
    current = rad_list.begin();
    last = rad_list.end();
    while ( current != last ) {
	printf("(%.2f, %.2f)\n", current->theta, current->dist);
	++current;
    }
    printf("\n");
    */

    // rotate by specified angle
    // printf("Rotating points by %.2f\n", angle);
    current = rad_list.begin();
    last = rad_list.end();
    while ( current != last ) {
	current->theta -= angle;
	while ( current->theta > FG_2PI ) {
	    current->theta -= FG_2PI;
	}
	// printf("(%.2f, %.2f)\n", current->theta, current->dist);
	++current;
    }
    // printf("\n");

    // find actual lon,lat of coordinates
    // printf("convert to lon, lat relative to %.2f %.2f\n", 
    //        origin.lon, origin.lat);
    current = rad_list.begin();
    last = rad_list.end();
    while ( current != last ) {
	p = calc_lon_lat(origin_rad, *current);
	// convert from radians to degress
	p.lon *= RAD_TO_DEG;
	p.lat *= RAD_TO_DEG;
	// printf("(%.8f, %.8f)\n", p.lon, p.lat);
	result_list.push_back(p);
	++current;
    }
    // printf("\n");

    return result_list;
}


// generate an area for a runway
list < point2d >
gen_runway_area( double lon, double lat, double heading, 
		      double length, double width) 
{
    list < point2d > result_list;
    list < point2d > tmp_list;
    list < point2d > :: iterator current;                          
    list < point2d > :: iterator last;                    

    point2d p;
    point2d origin;
    double l, w;
    int i;

    /*
    printf("runway: lon = %.2f lat = %.2f hdg = %.2f len = %.2f width = %.2f\n",
	   lon, lat, heading, length, width);
    */

    origin.lon = lon;
    origin.lat = lat;
    l = length / 2.0;
    w = width / 2.0;

    // generate untransformed runway area vertices
    p.x =  l; p.y =  w; tmp_list.push_back(p);
    p.x =  l; p.y = -w; tmp_list.push_back(p);
    p.x = -l; p.y = -w; tmp_list.push_back(p);
    p.x = -l; p.y =  w; tmp_list.push_back(p);

    /*
    // display points
    printf("Untransformed, unrotated runway\n");
    current = tmp_list.begin();
    last = tmp_list.end();
    while ( current != last ) {
	printf("(%.2f, %.2f)\n", current->x, current->y);
	++current;
    }
    printf("\n");
    */

    // rotate, transform, and convert points to lon, lat in degrees
    result_list = gen_area(origin, heading, tmp_list);

    /*
    // display points
    printf("Results in radians.\n");
    current = result_list.begin();
    last = result_list.end();
    while ( current != last ) {
	printf("(%.8f, %.8f)\n", current->lon, current->lat);
	++current;
    }
    printf("\n");
    */

    return result_list;
}


// $Log$
// Revision 1.3  1998/09/09 16:26:31  curt
// Continued progress in implementing the convex hull algorithm.
//
// Revision 1.2  1998/09/04 23:04:48  curt
// Beginning of convex hull genereration routine.
//
// Revision 1.1  1998/09/01 19:34:33  curt
// Initial revision.
//
// Revision 1.1  1998/07/20 12:54:05  curt
// Initial revision.
//
//
