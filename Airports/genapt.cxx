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
// #include <Include/fg_types.h>
#include <Math/fg_geodesy.hxx>
#include <Math/mat3.h>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Misc/fgstream.hxx>
#include <Objects/material.hxx>

// #include <gpc/gpc.h>

#include "genapt.hxx"


typedef vector < Point3D > container;
typedef container::iterator iterator;
typedef container::const_iterator const_iterator;


/*
// Calculate distance between to Point3D's
static double calc_dist(const Point3D& p1, const Point3D& p2) {
    double x, y, z;
    x = p1.x() - p2.x();
    y = p1.y() - p2.y();
    z = p1.z() - p2.z();
    return sqrt(x*x + y*y + z*z);
}
*/


#define FG_APT_BASE_TEX_CONSTANT 2000.0

#ifdef OLD_TEX_COORDS
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
#endif


// Calculate texture coordinates for a given point.
static Point3D calc_tex_coords(double *node, const Point3D& ref) {
    Point3D cp;
    Point3D pp;

    cp.setvals( node[0] + ref.x(), node[1] + ref.y(), node[2] + ref.z() );

    pp = fgCartToPolar3d(cp);

    pp.setx( fmod(FG_APT_BASE_TEX_CONSTANT * pp.x(), 10.0) );
    pp.sety( fmod(FG_APT_BASE_TEX_CONSTANT * pp.y(), 10.0) );

    if ( pp.x() < 0.0 ) {
	pp.setx( pp.x() + 10.0 );
    }

    if ( pp.y() < 0.0 ) {
	pp.sety( pp.y() + 10.0 );
    }

    return(pp);
}


// generate the actual base area for the airport
static void
gen_base( const Point3D& average, const container& perimeter, fgTILE *t)
{
    GLint display_list;
    Point3D cart, cart_trans, tex;
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

    printf(" tile center = %.2f %.2f %.2f\n", 
	   t->center.x(), t->center.y(), t->center.z() );
    printf(" airport center = %.2f %.2f %.2f\n", 
	   average.x(), average.y(), average.z());
    fragment.center = average;

    normal[0] = average.x();
    normal[1] = average.y();
    normal[2] = average.z();
    MAT3_NORMALIZE_VEC(normal, temp);
    
    display_list = xglGenLists(1);
    xglNewList(display_list, GL_COMPILE);
    xglBegin(GL_TRIANGLE_FAN);

    // first point center of fan
    cart_trans = average - t->center;
    t->nodes[t->ncount][0] = cart_trans.x();
    t->nodes[t->ncount][1] = cart_trans.y();
    t->nodes[t->ncount][2] = cart_trans.z();
    center_num = t->ncount;
    t->ncount++;

    tex = calc_tex_coords( t->nodes[t->ncount-1], t->center );
    xglTexCoord2f(tex.x(), tex.y());
    xglNormal3dv(normal);
    xglVertex3dv(t->nodes[t->ncount-1]);

    // first point on perimeter
    const_iterator current = perimeter.begin();
    cart = fgGeodToCart( *current );
    cart_trans = cart - t->center;
    t->nodes[t->ncount][0] = cart_trans.x();
    t->nodes[t->ncount][1] = cart_trans.y();
    t->nodes[t->ncount][2] = cart_trans.z();
    t->ncount++;

    i = 1;
    tex = calc_tex_coords( t->nodes[i], t->center );
    dist = distance3D(average, cart);
    if ( dist > max_dist ) {
	max_dist = dist;
    }
    xglTexCoord2f(tex.x(), tex.y());
    xglVertex3dv(t->nodes[i]);
    ++current;
    ++i;

    const_iterator last = perimeter.end();
    for ( ; current != last; ++current ) {
	cart = fgGeodToCart( *current );
	cart_trans = cart - t->center;
	t->nodes[t->ncount][0] = cart_trans.x();
	t->nodes[t->ncount][1] = cart_trans.y();
	t->nodes[t->ncount][2] = cart_trans.z();
	t->ncount++;
	fragment.add_face(center_num, i - 1, i);

	tex = calc_tex_coords( t->nodes[i], t->center );
	dist = distance3D(average, cart);
	if ( dist > max_dist ) {
	    max_dist = dist;
	}
	xglTexCoord2f(tex.x(), tex.y());
	xglVertex3dv(t->nodes[i]);
	i++;
    }

    // last point (first point in perimeter list)
    current = perimeter.begin();
    cart = fgGeodToCart( *current );
    cart_trans = cart - t->center;
    fragment.add_face(center_num, i - 1, 1);

    tex = calc_tex_coords( t->nodes[1], t->center );
    xglTexCoord2f(tex.x(), tex.y());
    xglVertex3dv(t->nodes[1]);

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
    int i = 1;

    // face list (this indexes into the master tile vertex list)
    container perimeter;
    Point3D p, average;
    double avex = 0.0, avey = 0.0, avez = 0.0;
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
	    i = 1;
	    avex = avey = avez = 0.0;
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
	    in.stream() >> p;
	    avex += tile->nodes[i][0];
	    avey += tile->nodes[i][1];
	    avez += tile->nodes[i][2];
	    perimeter.push_back(p);
	    ++i;
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
	average.setvals( avex / (double)size + tile->center.x(),
			 avey / (double)size + tile->center.y(),
			 avez / (double)size + tile->center.z() );

	gen_base(average, perimeter, tile);
    }

    return 1;
}


// $Log$
// Revision 1.5  1998/10/16 23:27:14  curt
// C++-ifying.
//
// Revision 1.4  1998/10/16 00:51:46  curt
// Converted to Point3D class.
//
// Revision 1.3  1998/09/21 20:55:00  curt
// Used the cartesian form of the airport area coordinates to determine the
// center.
//
// Revision 1.2  1998/09/14 12:44:30  curt
// Don't recalculate perimeter points since it is not likely that they will match
// exactly with the previously calculated points, which will leave an ugly gap
// around the airport area.
//
// Revision 1.1  1998/09/14 02:14:01  curt
// Initial revision of genapt.[ch]xx for generating airport scenery.
//
//

