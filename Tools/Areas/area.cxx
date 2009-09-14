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


point2d cart_to_polar_2d(point2d in) {
    point2d result;
    result.dist = sqrt( in.x * in.x + in.y * in.y );
    result.theta = atan2(in.y, in.x);    

    return(result);
}


void batch_cart_to_polar_2d(point2d *in, point2d *out, int size) {
    int i;

    for ( i = 0; i < size; i++ ) {
	out[i] = cart_to_polar_2d( in[i] );
    }
}


// given a set of 2d coordinates relative to a center point, and the
// lon, lat of that center point, as well as a potential orientation
// angle, generate the corresponding lon and lat of the original 2d
// verticies.
void make_area(point2d orig, point2d *cart, point2d *result, 
	       int size, double angle ) {
    point2d rad[size];
    int i;

    // convert to polar coordinates
    batch_cart_to_polar_2d(cart, rad, size);
    for ( i = 0; i < size; i++ ) {
	printf("(%.2f, %.2f)\n", rad[i].dist, rad[i].theta);
    }
    printf("\n");

    // rotate by specified angle
    for ( i = 0; i < size; i++ ) {
	rad[i].theta += angle;
	while ( rad[i].theta > FG_2PI ) {
	    rad[i].theta -= FG_2PI;
	}
	printf("(%.2f, %.2f)\n", rad[i].dist, rad[i].theta);
    }
    printf("\n");

    for ( i = 0; i < size; i++ ) {
	result[i] = calc_lon_lat(orig, rad[i]);
	printf("(%.8f, %.8f)\n", result[i].lon, result[i].lat);
    }
    printf("\n");
}


// generate an area for a runway
void gen_runway_area( double lon, double lat, double heading, 
		      double length, double width,
		      point2d *result, int *count) 
{
    point2d cart[4];
    point2d orig;
    double l, w;
    int i;

    orig.lon = lon;
    orig.lat = lat;
    l = (length / 2.0) + (length * 0.1);
    w = (width / 2.0) + (width * 0.1);

    // generate untransformed runway area vertices
    cart[0].x =  l; cart[0].y =  w;
    cart[1].x =  l; cart[1].y = -w;
    cart[2].x = -l; cart[2].y = -w;
    cart[3].x = -l; cart[3].y =  w;
    for ( i = 0; i < 4; i++ ) {
	printf("(%.2f, %.2f)\n", cart[i].x, cart[i].y);
    }
    printf("\n");

    make_area(orig, cart, result, 4, heading);

    for ( i = 0; i < 4; i++ ) {
	printf("(%.8f, %.8f)\n", result[i].lon, result[i].lat);
    }
    printf("\n");

    *count = 4;
}


// $Log$
// Revision 1.1  1998/07/20 12:54:05  curt
// Initial revision.
//
//
