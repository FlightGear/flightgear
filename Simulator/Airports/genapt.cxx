//
// genapt.cxx -- generate airport scenery from the given definition file
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


#include <Include/compiler.h>

#include STL_STRING
#include <vector>

#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <math.h>

#ifdef FG_HAVE_NATIVE_SGI_COMPILERS
#  include <strings.h>
#endif

#include <Debug/logstream.hxx>
// #include <Include/fg_types.h>
#include <Math/fg_geodesy.hxx>
#include <Math/mat3.h>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Misc/fgstream.hxx>
#include <Objects/material.hxx>

// #include <gpc/gpc.h>

#include "genapt.hxx"

FG_USING_STD(string);
FG_USING_STD(vector);


typedef vector < Point3D > container;
typedef container::iterator iterator;
typedef container::const_iterator const_iterator;


#define FG_APT_BASE_TEX_CONSTANT 2000.0

// Calculate texture coordinates for a given point.
static Point3D calc_tex_coords(double *node, const Point3D& ref) {
    Point3D cp;
    Point3D pp;

    cp = Point3D( node[0] + ref.x(), node[1] + ref.y(), node[2] + ref.z() );

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

    FG_LOG( FG_TERRAIN, FG_INFO, 
	    "generating airport base for size = " << perimeter.size() );

    fragment.init();
    fragment.tile_ptr = t;

    // find airport base material in the properties list
    if ( ! material_mgr.find( APT_BASE_MATERIAL, fragment.material_ptr )) {
	FG_LOG( FG_TERRAIN, FG_ALERT, 
		"Ack! unknown material name = " << APT_BASE_MATERIAL 
		<< " in fgAptGenerat()" );
    }

    FG_LOG( FG_TERRAIN, FG_INFO, 
	    " tile center = " 
	    << t->center.x() << " " << t->center.y() << " " << t->center.z() );
    FG_LOG( FG_TERRAIN, FG_INFO, 
	    " airport center = "
	    << average.x() << " " << average.y() << " " << average.z() );
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
    dist = cart.distance3Dsquared(average);
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
	dist = cart.distance3Dsquared(average);
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

    fragment.bounding_radius = sqrt(max_dist);
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
    in >> skipcomment;
    while ( ! in.eof() )
    {
	in >> token;

	if ( token == "a" ) {
	    // airport info record (start of airport)

	    if ( apt_id.length() > 0 ) {
		// we have just finished reading and airport record.
		// process the info
		gen_base(average, perimeter, tile);
	    }

	    FG_LOG( FG_TERRAIN, FG_INFO, "Reading airport record" );
	    in >> apt_id;
	    apt_name = "";
	    i = 1;
	    avex = avey = avez = 0.0;
	    perimeter.erase( perimeter.begin(), perimeter.end() );
	    // skip to end of line.
	    while ( in.get(c) && c != '\n' ) {
		apt_name += c;
	    }
	    FG_LOG( FG_TERRAIN, FG_INFO, 
		    "\tID = " << apt_id << "  Name = " << apt_name );
	} else if ( token == "p" ) {
	    // airport area bounding polygon coordinate.  These
	    // specify a convex hull that should already have been cut
	    // out of the base terrain.  The points are given in
	    // counter clockwise order and are specified in lon/lat
	    // degrees.
	    in >> p;
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

	in >> skipcomment;
    }

    if ( apt_id.length() > 0 ) {
	// we have just finished reading and airport record.
	// process the info
	size = perimeter.size();
	average = Point3D( avex / (double)size + tile->center.x(),
			   avey / (double)size + tile->center.y(),
			   avez / (double)size + tile->center.z() );

	gen_base(average, perimeter, tile);
    }

    return 1;
}


// $Log$
// Revision 1.1  1999/04/05 21:32:47  curt
// Initial revision
//
// Revision 1.14  1999/03/02 01:02:31  curt
// Tweaks for building with native SGI compilers.
//
// Revision 1.13  1999/02/26 22:08:34  curt
// Added initial support for native SGI compilers.
//
// Revision 1.12  1999/02/01 21:08:33  curt
// Optimizations from Norman Vine.
//
// Revision 1.11  1998/11/23 21:48:09  curt
// Borland portability tweaks.
//
// Revision 1.10  1998/11/07 19:07:06  curt
// Enable release builds using the --without-logging option to the configure
// script.  Also a couple log message cleanups, plus some C to C++ comment
// conversion.
//
// Revision 1.9  1998/11/06 21:17:32  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.8  1998/11/06 14:46:59  curt
// Changes to track Bernie's updates to fgstream.
//
// Revision 1.7  1998/10/20 18:26:06  curt
// Updates to point3d.hxx
//
// Revision 1.6  1998/10/18 01:17:16  curt
// Point3D tweaks.
//
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
