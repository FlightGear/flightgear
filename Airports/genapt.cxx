//
// getapt.cxx -- generate airport scenery from the given definition file
//
// Written by Curtis Olson, started September 1998.
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


#include <string>        // Standard C++ string library
#include <vector>
#include "Include/fg_stl_config.h"

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif

#include <Debug/fg_debug.h>
#include <Include/fg_types.h>
#include <Math/fg_geodesy.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Misc/fgstream.hxx>
#include <Objects/material.hxx>

// #include <gpc/gpc.h>

#include "genapt.hxx"


typedef vector < fgPoint3d > container;
typedef container::iterator iterator;
typedef container::const_iterator const_iterator;


// Calculate distance between to fgPoint3d's
static double calc_dist(const fgPoint3d& p1, const fgPoint3d& p2) {
    double x, y, z;
    x = p1.x - p2.x;
    y = p1.y - p2.y;
    z = p1.z - p2.z;
    return sqrt(x*x + y*y + z*z);
}


// convert a geodetic point lon(degrees), lat(degrees), elev(meter) to a
// cartesian point
static fgPoint3d geod_to_cart(fgPoint3d geod) {
    fgPoint3d cart;
    fgPoint3d geoc;
    double sl_radius;

    // printf("A geodetic point is (%.2f, %.2f, %.2f)\n", geod[0],
    //        geod[1], geod[2]);

    geoc.lon = geod.lon*DEG_TO_RAD;
    fgGeodToGeoc(geod.lat*DEG_TO_RAD, geod.radius, &sl_radius, &geoc.lat);

    // printf("A geocentric point is (%.2f, %.2f, %.2f)\n", gc_lon, 
    //        gc_lat, sl_radius+geod[2]); */

    geoc.radius = sl_radius + geod.radius;
    cart = fgPolarToCart3d(geoc);
    
    // printf("A cart point is (%.8f, %.8f, %.8f)\n", cp.x, cp.y, cp.z);

    return cart;
}


#define FG_APT_BASE_TEX_CONSTANT 2000.0

// Calculate texture coordinates for a given point.
static fgPoint3d
calc_tex_coords(const fgPoint3d& p) {
    fgPoint3d tex;

    cout << "Texture coordinates = " << 
	FG_APT_BASE_TEX_CONSTANT * p.lon << "  " << 
	FG_APT_BASE_TEX_CONSTANT * p.lat << "\n";

    tex.x = fmod(FG_APT_BASE_TEX_CONSTANT * p.lon, 10.0);
    tex.y = fmod(FG_APT_BASE_TEX_CONSTANT * p.lat, 10.0);

    if ( tex.x < 0.0 ) {
	tex.x += 10.0;
    }

    if ( tex.y < 0.0 ) {
	tex.y += 10.0;
    }

    cout << "Texture coordinates = " << tex.x << "  " << tex.y << "\n";

    return tex;
}


// generate the actual base area for the airport
static void
gen_base( const fgPoint3d& average, const container& perimeter, fgTILE *t)
{
    GLint display_list;
    fgPoint3d ave_cart, cart, cart_trans, tex;
    MAT3vec normal;
    double dist, max_dist, temp;
    int center_num, i;

    fgFRAGMENT fragment;

    max_dist = 0.0;

    cout << "generating airport base for size = " << perimeter.size() << "\n";

    fragment.init();
    fragment.tile_ptr = t;

    // find airport base material in the properties list
    if ( ! material_mgr.find( APT_BASE_MATERIAL, fragment.material_ptr )) {
	fgPrintf( FG_TERRAIN, FG_ALERT, 
		  "Ack! unknown material name = %s in fgAptGenerat()\n",
		  APT_BASE_MATERIAL );
    }

    ave_cart = geod_to_cart( average );
    printf(" tile center = %.2f %.2f %.2f\n", 
	   t->center.x, t->center.y, t->center.z);
    printf(" airport center = %.2f %.2f %.2f\n", 
	   average.x, average.y, average.z);
    printf(" airport center = %.2f %.2f %.2f\n", 
	   ave_cart.x, ave_cart.y, ave_cart.z);
    fragment.center.x = ave_cart.x;
    fragment.center.y = ave_cart.y;
    fragment.center.z = ave_cart.z;

    normal[0] = ave_cart.x;
    normal[1] = ave_cart.y;
    normal[2] = ave_cart.z;
    MAT3_NORMALIZE_VEC(normal, temp);
    
    display_list = xglGenLists(1);
    xglNewList(display_list, GL_COMPILE);
    xglBegin(GL_TRIANGLE_FAN);

    // center of fan
    cart_trans.x = ave_cart.x - t->center.x;
    cart_trans.y = ave_cart.y - t->center.y;
    cart_trans.z = ave_cart.z - t->center.z;
    t->nodes[t->ncount][0] = cart_trans.x;
    t->nodes[t->ncount][1] = cart_trans.y;
    t->nodes[t->ncount][2] = cart_trans.z;
    t->ncount++;

    tex = calc_tex_coords( average );
    xglTexCoord2f(tex.x, tex.y);
    xglNormal3dv(normal);
    xglVertex3d(cart_trans.x, cart_trans.y, cart_trans.z);

    // first point (center of fan)
    iterator current = perimeter.begin();
    cart = geod_to_cart( *current );
    cart_trans.x = cart.x - t->center.x;
    cart_trans.y = cart.y - t->center.y;
    cart_trans.z = cart.z - t->center.z;
    center_num = t->ncount;
    t->nodes[t->ncount][0] = cart_trans.x;
    t->nodes[t->ncount][1] = cart_trans.y;
    t->nodes[t->ncount][2] = cart_trans.z;
    t->ncount++;

    tex = calc_tex_coords( *current );
    dist = calc_dist(ave_cart, cart);
    if ( dist > max_dist ) {
	max_dist = dist;
    }
    xglTexCoord2f(tex.x, tex.y);
    xglVertex3d(cart_trans.x, cart_trans.y, cart_trans.z);

    const_iterator last = perimeter.end();
    for ( ; current != last; ++current ) {
	cart = geod_to_cart( *current );
	cart_trans.x = cart.x - t->center.x;
	cart_trans.y = cart.y - t->center.y;
	cart_trans.z = cart.z - t->center.z;
	t->nodes[t->ncount][0] = cart_trans.x;
	t->nodes[t->ncount][1] = cart_trans.y;
	t->nodes[t->ncount][2] = cart_trans.z;
	t->ncount++;
	fragment.add_face(center_num, t->ncount - 2, t->ncount - 1);

	tex = calc_tex_coords( *current );
	dist = calc_dist(ave_cart, cart);
	if ( dist > max_dist ) {
	    max_dist = dist;
	}
	xglTexCoord2f(tex.x, tex.y);
	xglVertex3d(cart_trans.x, cart_trans.y, cart_trans.z);
    }

    // last point (first point in perimeter list)
    current = perimeter.begin();
    cart = geod_to_cart( *current );
    cart_trans.x = cart.x - t->center.x;
    cart_trans.y = cart.y - t->center.y;
    cart_trans.z = cart.z - t->center.z;
    fragment.add_face(center_num, t->ncount - 1, center_num + 1);

    tex = calc_tex_coords( *current );
    xglTexCoord2f(tex.x, tex.y);
    xglVertex3d(cart_trans.x, cart_trans.y, cart_trans.z);

    xglEnd();
    xglEndList();

    fragment.bounding_radius = max_dist;
    fragment.display_list = display_list;

    t->fragment_list.push_back(fragment);
}


// Load a .apt file and register the GL fragments with the
// corresponding tile
int
fgAptGenerate(const string& path, fgTILE *tile)
{
    string token;
    string apt_id, apt_name;
    char c;

    // face list (this indexes into the master tile vertex list)
    container perimeter;
    fgPoint3d p, average;
    int size;

    // gpc_vertex p_2d, list_2d[MAX_PERIMETER];
    // gpc_vertex_list perimeter_2d;

    fg_gzifstream in( path );
    if ( !in ) {
	// exit immediately assuming an airport file for this tile
	// doesn't exist.
	return 0;
    }

    apt_id = "";

    // read in each line of the file
    in.eat_comments();
    while ( ! in.eof() )
    {
	in.stream() >> token;

	if ( token == "a" ) {
	    // airport info record (start of airport)

	    if ( apt_id != "" ) {
		// we have just finished reading and airport record.
		// process the info
		gen_base(average, perimeter, tile);
	    }

	    cout << "Reading airport record\n";
	    in.stream() >> apt_id;
	    apt_name = "";
	    average.lon = average.lat = average.radius = 0.0;
	    perimeter.erase( perimeter.begin(), perimeter.end() );
	    // skip to end of line.
	    while ( in.get(c) && c != '\n' ) {
		apt_name += c;
	    }
	    cout << "\tID = " + apt_id + "  Name = " + apt_name + "\n";
	} else if ( token == "p" ) {
	    // airport area bounding polygon coordinate.  These
	    // specify a convex hull that should already have been cut
	    // out of the base terrain.  The points are given in
	    // counter clockwise order and are specified in lon/lat
	    // degrees.
	    in.stream() >> p.lon >> p.lat >> p.radius;
	    average.lon += p.lon;
	    average.lat += p.lat;
	    average.radius += p.radius;
	    perimeter.push_back(p);
	} else if ( token == "r" ) {
	    // runway record
	    // skip for now
	    while ( in.get(c) && c != '\n' );
	}

	// airports.insert(a);
	in.eat_comments();
    }

    if ( apt_id != "" ) {
	// we have just finished reading and airport record.
	// process the info
	size = perimeter.size();
	average.lon /= (double)size;
	average.lat /= (double)size;
	average.radius /= (double)size;

	gen_base(average, perimeter, tile);
    }

    return 1;
}


// $Log$
// Revision 1.1  1998/09/14 02:14:01  curt
// Initial revision of genapt.[ch]xx for generating airport scenery.
//
//

