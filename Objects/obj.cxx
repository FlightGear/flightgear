// obj.cxx -- routines to handle "sorta" WaveFront .obj format files.
//
// Written by Curtis Olson, started October 1997.
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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <stdio.h>
#include <string.h>
#include <GL/glut.h>
#include <XGL/xgl.h>

#if defined ( __sun__ )
extern "C" void *memmove(void *, const void *, size_t);
extern "C" void *memset(void *, int, size_t);
#endif

#include <string>       // Standard C++ library
#include <map>          // STL
#include <ctype.h>      // isdigit()

#ifdef NEEDNAMESPACESTD
using namespace std;
#endif

#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Include/fg_zlib.h>
#include <Main/options.hxx>
#include <Math/mat3.h>
#include <Math/fg_random.h>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Misc/stopwatch.hxx>
#include <Misc/fgstream.hxx>
#include <Scenery/tile.hxx>

#include "material.hxx"
#include "obj.hxx"


static double normals[MAX_NODES][3];


// given three points defining a triangle, calculate the normal
static void calc_normal(double p1[3], double p2[3], 
			double p3[3], double normal[3])
{
    double v1[3], v2[3];
    double temp;

    v1[0] = p2[0] - p1[0]; v1[1] = p2[1] - p1[1]; v1[2] = p2[2] - p1[2];
    v2[0] = p3[0] - p1[0]; v2[1] = p3[1] - p1[1]; v2[2] = p3[2] - p1[2];

    MAT3cross_product(normal, v1, v2);
    MAT3_NORMALIZE_VEC(normal,temp);

    // fgPrintf( FG_TERRAIN, FG_DEBUG, "  Normal = %.2f %.2f %.2f\n", 
    //           normal[0], normal[1], normal[2]);
}


#define FG_TEX_CONSTANT 69.0


// Calculate texture coordinates for a given point.
static Point3D calc_tex_coords(double *node, const Point3D& ref) {
    Point3D cp;
    Point3D pp;
    // double tmplon, tmplat;

    // cout << "-> " << node[0] << " " << node[1] << " " << node[2] << endl;
    // cout << "-> " << ref.x() << " " << ref.y() << " " << ref.z() << endl;

    cp = Point3D( node[0] + ref.x(),
		  node[1] + ref.y(),
		  node[2] + ref.z() );

    pp = fgCartToPolar3d(cp);

    // tmplon = pp.lon() * RAD_TO_DEG;
    // tmplat = pp.lat() * RAD_TO_DEG;
    // cout << tmplon << " " << tmplat << endl;

    pp.setx( fmod(RAD_TO_DEG * FG_TEX_CONSTANT * pp.x(), 11.0) );
    pp.sety( fmod(RAD_TO_DEG * FG_TEX_CONSTANT * pp.y(), 11.0) );

    if ( pp.x() < 0.0 ) {
	pp.setx( pp.x() + 11.0 );
    }

    if ( pp.y() < 0.0 ) {
	pp.sety( pp.y() + 11.0 );
    }

    // cout << pp << endl;

    return(pp);
}


// Load a .obj file and build the GL fragment list
int fgObjLoad( const string& path, fgTILE *t) {
    fgFRAGMENT fragment;
    Point3D pp;
    double approx_normal[3], normal[3], scale;
    // double x, y, z, xmax, xmin, ymax, ymin, zmax, zmin;
    // GLfloat sgenparams[] = { 1.0, 0.0, 0.0, 0.0 };
    GLint display_list;
    int shading;
    int in_fragment, in_faces, vncount, n1, n2, n3, n4;
    int last1, last2, odd;
    double (*nodes)[3];
    Point3D center;

    // printf("loading %s\n", path.c_str() );

    // Attempt to open "path.gz" or "path"
    fg_gzifstream in( path );
    if ( ! in )
    {
	// Attempt to open "path.obj" or "path.obj.gz"
	in.open( path + ".obj" );
	if ( ! in )
	{
	    fgPrintf( FG_TERRAIN, FG_ALERT, 
		      "Cannot open file: %s\n", path.c_str() );
	    return 0;
	}
    }

    shading = current_options.get_shading();

    in_fragment = 0;
    t->ncount = 1;
    vncount = 1;
    t->bounding_radius = 0.0;
    nodes = t->nodes;
    center = t->center;

    StopWatch stopwatch;
    stopwatch.start();

    // ignore initial comments and blank lines. (priming the pump)
    in.eat_comments();

    while ( ! in.eof() )
    {

	string token;
	in.stream() >> token;

	// printf("token = %s\n", token.c_str() );

	if ( token == "gbs" )
	{
	    // reference point (center offset)
	    in.stream() >> t->center >> t->bounding_radius;
	    center = t->center;
	}
	else if ( token == "bs" )
	{
	    // reference point (center offset)
	    in.stream() >> fragment.center;
	    in.stream() >> fragment.bounding_radius;
	}
	else if ( token == "vn" )
	{
	    // vertex normal
	    if ( vncount < MAX_NODES ) {
		in.stream() >> normals[vncount][0]
			    >> normals[vncount][1]
			    >> normals[vncount][2];
		vncount++;
	    } else {
		fgPrintf( FG_TERRAIN, FG_EXIT, 
			  "Read too many vertex normals ... dying :-(\n");
	    }
	}
	else if ( token[0] == 'v' )
	{
	    // node (vertex)
	    if ( t->ncount < MAX_NODES ) {
		in.stream() >> t->nodes[t->ncount][0]
			    >> t->nodes[t->ncount][1]
			    >> t->nodes[t->ncount][2];
		t->ncount++;
	    } else {
		fgPrintf( FG_TERRAIN, FG_EXIT, 
			  "Read too many nodes ... dying :-(\n");
	    }
	}
	else if ( token == "usemtl" )
	{
	    // material property specification

	    // this also signals the start of a new fragment
	    if ( in_fragment ) {
		// close out the previous structure and start the next
		xglEnd();
		xglEndList();
		// printf("xglEnd(); xglEndList();\n");

		// update fragment
		fragment.display_list = display_list;

		// push this fragment onto the tile's object list
		t->fragment_list.push_back(fragment);
	    } else {
		in_fragment = 1;
	    }

	    // printf("start of fragment (usemtl)\n");

	    display_list = xglGenLists(1);
	    xglNewList(display_list, GL_COMPILE);
	    // printf("xglGenLists(); xglNewList();\n");
	    in_faces = 0;

	    // reset the existing face list
	    // printf("cleaning a fragment with %d faces\n", 
	    //        fragment.faces.size());
	    fragment.init();

	    // scan the material line
	    string material;
	    in.stream() >> material;
	    fragment.tile_ptr = t;

	    // find this material in the properties list
	    if ( ! material_mgr.find( material, fragment.material_ptr )) {
		fgPrintf( FG_TERRAIN, FG_ALERT, 
			  "Ack! unknown usemtl name = %s in %s\n",
			  material.c_str(), path.c_str() );
	    }

	    // initialize the fragment transformation matrix
	    /*
	    for ( i = 0; i < 16; i++ ) {
		fragment.matrix[i] = 0.0;
	    }
	    fragment.matrix[0] = fragment.matrix[5] =
		fragment.matrix[10] = fragment.matrix[15] = 1.0;
	    */
	} else if ( token[0] == 't' ) {
	    // start a new triangle strip

	    n1 = n2 = n3 = n4 = 0;

	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "    new tri strip = %s", line);
	    in.stream() >> n1 >> n2 >> n3;
	    fragment.add_face(n1, n2, n3);

	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "(t) = ");

	    xglBegin(GL_TRIANGLE_STRIP);
	    // printf("xglBegin(tristrip) %d %d %d\n", n1, n2, n3);

	    odd = 1; 
	    scale = 1.0;

	    if ( shading ) {
		// Shading model is "GL_SMOOTH" so use precalculated
		// (averaged) normals
		MAT3_SCALE_VEC(normal, normals[n1], scale);
		xglNormal3dv(normal);
		pp = calc_tex_coords(nodes[n1], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3d(t->nodes[n1][0],t->nodes[n1][1],t->nodes[n1][2]);
		xglVertex3dv(nodes[n1]);		

		MAT3_SCALE_VEC(normal, normals[n2], scale);
		xglNormal3dv(normal);
		pp = calc_tex_coords(nodes[n2], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		//xglVertex3d(t->nodes[n2][0],t->nodes[n2][1],t->nodes[n2][2]);
		xglVertex3dv(nodes[n2]);				

		MAT3_SCALE_VEC(normal, normals[n3], scale);
		xglNormal3dv(normal);
                pp = calc_tex_coords(nodes[n3], center);
                xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3d(t->nodes[n3][0],t->nodes[n3][1],t->nodes[n3][2]);
		xglVertex3dv(nodes[n3]);
	    } else {
		// Shading model is "GL_FLAT" so calculate per face
		// normals on the fly.
		if ( odd ) {
		    calc_normal(nodes[n1], nodes[n2], 
				nodes[n3], approx_normal);
		} else {
		    calc_normal(nodes[n2], nodes[n1], 
				nodes[n3], approx_normal);
		}
		MAT3_SCALE_VEC(normal, approx_normal, scale);
		xglNormal3dv(normal);

		pp = calc_tex_coords(nodes[n1], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3d(t->nodes[n1][0],t->nodes[n1][1],t->nodes[n1][2]);
		xglVertex3dv(nodes[n1]);		

		pp = calc_tex_coords(nodes[n2], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3d(t->nodes[n2][0],t->nodes[n2][1],t->nodes[n2][2]);
		xglVertex3dv(nodes[n2]);		

		pp = calc_tex_coords(nodes[n3], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3d(t->nodes[n3][0],t->nodes[n3][1],t->nodes[n3][2]);
		xglVertex3dv(nodes[n3]);		
	    }
	    // printf("some normals, texcoords, and vertices\n");

	    odd = 1 - odd;
	    last1 = n2;
	    last2 = n3;

	    // There can be three or four values 
	    char c;
	    while ( in.get(c) )
	    {
		if ( c == '\n' )
		    break; // only the one

		if ( isdigit(c) )
		{
		    in.putback(c);
		    in.stream() >> n4;
		    break;
		}
	    }

	    if ( n4 > 0 ) {
		fragment.add_face(n3, n2, n4);

		if ( shading ) {
		    // Shading model is "GL_SMOOTH"
		    MAT3_SCALE_VEC(normal, normals[n4], scale);
		} else {
		    // Shading model is "GL_FLAT"
		    calc_normal(nodes[n3], nodes[n2], nodes[n4], 
				approx_normal);
		    MAT3_SCALE_VEC(normal, approx_normal, scale);
		}
		xglNormal3dv(normal);
		pp = calc_tex_coords(nodes[n4], center);
                xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3d(t->nodes[n4][0],t->nodes[n4][1],t->nodes[n4][2]);
		xglVertex3dv(nodes[n4]);		

		odd = 1 - odd;
		last1 = n3;
		last2 = n4;
		// printf("a normal, texcoord, and vertex (4th)\n");
	    }
	} else if ( token[0] == 'f' ) {
	    // unoptimized face

	    if ( !in_faces ) {
		xglBegin(GL_TRIANGLES);
		// printf("xglBegin(triangles)\n");
		in_faces = 1;
	    }

	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "new triangle = %s", line);*/
	    in.stream() >> n1 >> n2 >> n3;
	    fragment.add_face(n1, n2, n3);

            // xglNormal3d(normals[n1][0], normals[n1][1], normals[n1][2]);
	    xglNormal3dv(normals[n1]);
	    pp = calc_tex_coords(nodes[n1], center);
	    xglTexCoord2f(pp.lon(), pp.lat());
 	    // xglVertex3d(t->nodes[n1][0], t->nodes[n1][1], t->nodes[n1][2]);
 	    xglVertex3dv(nodes[n1]);

            // xglNormal3d(normals[n2][0], normals[n2][1], normals[n2][2]);
	    xglNormal3dv(normals[n2]);
            pp = calc_tex_coords(nodes[n2], center);
            xglTexCoord2f(pp.lon(), pp.lat());
	    // xglVertex3d(t->nodes[n2][0], t->nodes[n2][1], t->nodes[n2][2]);
	    xglVertex3dv(nodes[n2]);
		
            // xglNormal3d(normals[n3][0], normals[n3][1], normals[n3][2]);
	    xglNormal3dv(normals[n3]);
            pp = calc_tex_coords(nodes[n3], center);
            xglTexCoord2f(pp.lon(), pp.lat());
	    // xglVertex3d(t->nodes[n3][0], t->nodes[n3][1], t->nodes[n3][2]);
	    xglVertex3dv(nodes[n3]);
	    // printf("some normals, texcoords, and vertices (tris)\n");
	} else if ( token[0] == 'q' ) {
	    // continue a triangle strip
	    n1 = n2 = 0;

	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "continued tri strip = %s ", 
	    //           line);
	    in.stream() >> n1;

	    // There can be one or two values 
	    char c;
	    while ( in.get(c) )
	    {
		if ( c == '\n' )
		    break; // only the one

		if ( isdigit(c) )
		{
		    in.putback(c);
		    in.stream() >> n2;
		    break;
		}
	    }
	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "read %d %d\n", n1, n2);

	    if ( odd ) {
		fragment.add_face(last1, last2, n1);
	    } else {
		fragment.add_face(last2, last1, n1);
	    }

	    if ( shading ) {
		// Shading model is "GL_SMOOTH"
		MAT3_SCALE_VEC(normal, normals[n1], scale);
	    } else {
		// Shading model is "GL_FLAT"
		if ( odd ) {
		    calc_normal(nodes[last1], nodes[last2], nodes[n1], 
				approx_normal);
		} else {
		    calc_normal(nodes[last2], nodes[last1], nodes[n1], 
				approx_normal);
		}
		MAT3_SCALE_VEC(normal, approx_normal, scale);
	    }
	    xglNormal3dv(normal);

            pp = calc_tex_coords(nodes[n1], center);
            xglTexCoord2f(pp.lon(), pp.lat());
	    // xglVertex3d(t->nodes[n1][0], t->nodes[n1][1], t->nodes[n1][2]);
	    xglVertex3dv(nodes[n1]);
	    // printf("a normal, texcoord, and vertex (4th)\n");
   
	    odd = 1 - odd;
	    last1 = last2;
	    last2 = n1;

	    if ( n2 > 0 ) {
		// fgPrintf( FG_TERRAIN, FG_DEBUG, " (cont)\n");

		if ( odd ) {
		    fragment.add_face(last1, last2, n2);
		} else {
		    fragment.add_face(last2, last1, n2);
		}

		if ( shading ) {
		    // Shading model is "GL_SMOOTH"
		    MAT3_SCALE_VEC(normal, normals[n2], scale);
		} else {
		    // Shading model is "GL_FLAT"
		    if ( odd ) {
			calc_normal(nodes[last1], nodes[last2], 
				    nodes[n2], approx_normal);
		    } else {
			calc_normal(nodes[last2], nodes[last1], 
				    nodes[n2], approx_normal);
		    }
		    MAT3_SCALE_VEC(normal, approx_normal, scale);
		}
		xglNormal3dv(normal);
		
		pp = calc_tex_coords(nodes[n2], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3d(t->nodes[n2][0],t->nodes[n2][1],t->nodes[n2][2]);
		xglVertex3dv(nodes[n2]);		
		// printf("a normal, texcoord, and vertex (4th)\n");

		odd = 1 -odd;
		last1 = last2;
		last2 = n2;
	    }
	} else {
	    fgPrintf( FG_TERRAIN, FG_WARN, "Unknown token in %s = %s\n", 
		      path.c_str(), token.c_str() );
	}

	// eat comments and blank lines before start of while loop so
	// if we are done with useful input it is noticed before hand.
	in.eat_comments();
    }

    if ( in_fragment ) {
	// close out the previous structure and start the next
	xglEnd();
	xglEndList();
	// printf("xglEnd(); xglEndList();\n");

	// update fragment
	fragment.display_list = display_list;
	
	// push this fragment onto the tile's object list
	t->fragment_list.push_back(fragment);
    }

    // Draw normal vectors (for visually verifying normals)
    /*
    xglBegin(GL_LINES);
    xglColor3f(0.0, 0.0, 0.0);
    for ( i = 0; i < t->ncount; i++ ) {
        xglVertex3d(t->nodes[i][0],
 		    t->nodes[i][1] ,
 		    t->nodes[i][2]);
 	xglVertex3d(t->nodes[i][0] + 500*normals[i][0],
 		    t->nodes[i][1] + 500*normals[i][1],
 		    t->nodes[i][2] + 500*normals[i][2]);
    } 
    xglEnd();
    */   

    stopwatch.stop();
    fgPrintf( FG_TERRAIN, FG_INFO, "Loaded %s in %f seconds\n",
	      path.c_str(), stopwatch.elapsedSeconds() );

    // printf("end of tile\n");

    return(1);
}


// $Log$
// Revision 1.8  1998/10/20 18:33:55  curt
// Tweaked texture coordinates, but we still have some problems. :-(
//
// Revision 1.7  1998/10/20 15:48:44  curt
// Removed an extraneous output message.
//
// Revision 1.6  1998/10/18 01:17:21  curt
// Point3D tweaks.
//
// Revision 1.5  1998/10/16 00:54:39  curt
// Converted to Point3D class.
//
// Revision 1.4  1998/09/15 01:35:07  curt
// cleaned up my fragment.num_faces hack :-) to use the STL (no need in
// duplicating work.)
// Tweaked fgTileMgrRender() do not calc tile matrix unless necessary.
// removed some unneeded stuff from fgTileMgrCurElev()
//
// Revision 1.3  1998/09/03 21:27:03  curt
// Fixed a serious bug caused by not-quite-correct comment/white space eating
// which resulted in mismatched glBegin() glEnd() pairs, incorrect display lists,
// and ugly display artifacts.
//
// Revision 1.2  1998/09/01 19:03:09  curt
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
// Revision 1.1  1998/08/25 16:51:25  curt
// Moved from ../Scenery
//
// Revision 1.23  1998/08/20 15:16:43  curt
// obj.cxx: use more explicit parenthases.
// texload.[ch]: use const in function definitions where appropriate.
//
// Revision 1.22  1998/08/20 15:12:03  curt
// Used a forward declaration of classes fgTILE and fgMATERIAL to eliminate
// the need for "void" pointers and casts.
// Quick hack to count the number of scenery polygons that are being drawn.
//
// Revision 1.21  1998/08/12 21:13:04  curt
// material.cxx: don't load textures if they are disabled
// obj.cxx: optimizations from Norman Vine
// tile.cxx: minor tweaks
// tile.hxx: addition of num_faces
// tilemgr.cxx: minor tweaks
//
// Revision 1.20  1998/07/24 21:42:07  curt
// material.cxx: whups, double method declaration with no definition.
// obj.cxx: tweaks to avoid errors in SGI's CC.
// tile.cxx: optimizations by Norman Vine.
// tilemgr.cxx: optimizations by Norman Vine.
//
// Revision 1.19  1998/07/13 21:01:58  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.18  1998/07/12 03:18:27  curt
// Added ground collision detection.  This involved:
// - saving the entire vertex list for each tile with the tile records.
// - saving the face list for each fragment with the fragment records.
// - code to intersect the current vertical line with the proper face in
//   an efficient manner as possible.
// Fixed a bug where the tiles weren't being shifted to "near" (0,0,0)
//
// Revision 1.17  1998/07/08 14:47:21  curt
// Fix GL_MODULATE vs. GL_DECAL problem introduced by splash screen.
// polare3d.h renamed to polar3d.hxx
// fg{Cartesian,Polar}Point3d consolodated.
// Added some initial support for calculating local current ground elevation.
//
// Revision 1.16  1998/07/06 21:34:33  curt
// Added using namespace std for compilers that support this.
//
// Revision 1.15  1998/07/04 00:54:28  curt
// Added automatic mipmap generation.
//
// When rendering fragments, use saved model view matrix from associated tile
// rather than recalculating it with push() translate() pop().
//
// Revision 1.14  1998/06/17 21:36:40  curt
// Load and manage multiple textures defined in the Materials library.
// Boost max material fagments for each material property to 800.
// Multiple texture support when rendering.
//
// Revision 1.13  1998/06/12 00:58:05  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
//
// Revision 1.12  1998/06/08 17:57:54  curt
// Working first pass at material proporty sorting.
//
// Revision 1.11  1998/06/06 01:09:31  curt
// I goofed on the log message in the last commit ... now fixed.
//
// Revision 1.10  1998/06/06 01:07:17  curt
// Increased per material fragment list size from 100 to 400.
// Now correctly draw viewable fragments in per material order.
//
// Revision 1.9  1998/06/05 22:39:54  curt
// Working on sorting by, and rendering by material properties.
//
// Revision 1.8  1998/06/05 18:19:18  curt
// Recognize file, file.gz, and file.obj as scenery object files.
//
// Revision 1.7  1998/05/24 02:49:09  curt
// Implimented fragment level view frustum culling.
//
// Revision 1.6  1998/05/23 14:09:20  curt
// Added tile.cxx and tile.hxx.
// Working on rewriting the tile management system so a tile is just a list
// fragments, and the fragment record contains the display list for that 
// fragment.
//
// Revision 1.5  1998/05/20 20:53:53  curt
// Moved global ref point and radius (bounding sphere info, and offset) to
// data file rather than calculating it on the fly.
// Fixed polygon winding problem in scenery generation stage rather than
// compensating for it on the fly.
// Made a fgTILECACHE class.
//
// Revision 1.4  1998/05/16 13:09:57  curt
// Beginning to add support for view frustum culling.
// Added some temporary code to calculate bouding radius, until the
//   scenery generation tools and scenery can be updated.
//
// Revision 1.3  1998/05/03 00:48:01  curt
// Updated texture coordinate fmod() parameter.
//
// Revision 1.2  1998/05/02 01:52:14  curt
// Playing around with texture coordinates.
//
// Revision 1.1  1998/04/30 12:35:28  curt
// Added a command line rendering option specify smooth/flat shading.
//
// Revision 1.35  1998/04/28 21:43:26  curt
// Wrapped zlib calls up so we can conditionally comment out zlib support.
//
// Revision 1.34  1998/04/28 01:21:42  curt
// Tweaked texture parameter calculations to keep the number smaller.  This
// avoids the "swimming" problem.
// Type-ified fgTIME and fgVIEW.
//
// Revision 1.33  1998/04/27 15:58:15  curt
// Screwing around with texture coordinate generation ... still needs work.
//
// Revision 1.32  1998/04/27 03:30:13  curt
// Minor transformation adjustments to try to keep scenery tiles closer to
// (0, 0, 0)  GLfloats run out of precision at the distances we need to model
// the earth, but we can do a bunch of pre-transformations using double math
// and then cast to GLfloat once everything is close in where we have less
// precision problems.
//
// Revision 1.31  1998/04/25 15:09:57  curt
// Changed "r" to "rb" in gzopen() options.  This fixes bad behavior in win32.
//
// Revision 1.30  1998/04/24 14:21:08  curt
// Added "file.obj.gz" support.
//
// Revision 1.29  1998/04/24 00:51:07  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Tweaked the scenery file extentions to be "file.obj" (uncompressed)
// or "file.obz" (compressed.)
//
// Revision 1.28  1998/04/22 13:22:44  curt
// C++ - ifing the code a bit.
//
// Revision 1.27  1998/04/18 04:13:17  curt
// Added zlib on the fly decompression support for loading scenery objects.
//
// Revision 1.26  1998/04/03 22:11:36  curt
// Converting to Gnu autoconf system.
//
// Revision 1.25  1998/03/14 00:30:50  curt
// Beginning initial terrain texturing experiments.
//
// Revision 1.24  1998/02/09 21:30:18  curt
// Fixed a nagging problem with terrain tiles not "quite" matching up perfectly.
//
// Revision 1.23  1998/02/09 15:07:52  curt
// Minor tweaks.
//
// Revision 1.22  1998/02/01 03:39:54  curt
// Minor tweaks.
//
// Revision 1.21  1998/01/31 00:43:25  curt
// Added MetroWorks patches from Carmen Volpe.
//
// Revision 1.20  1998/01/29 00:51:39  curt
// First pass at tile cache, dynamic tile loading and tile unloading now works.
//
// Revision 1.19  1998/01/27 03:26:42  curt
// Playing with new fgPrintf command.
//
// Revision 1.18  1998/01/19 19:27:16  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.17  1998/01/13 00:23:10  curt
// Initial changes to support loading and management of scenery tiles.  Note,
// there's still a fair amount of work left to be done.
//
// Revision 1.16  1997/12/30 23:09:40  curt
// Worked on winding problem without luck, so back to calling glFrontFace()
// 3 times for each scenery area.
//
// Revision 1.15  1997/12/30 20:47:51  curt
// Integrated new event manager with subsystem initializations.
//
// Revision 1.14  1997/12/30 01:38:46  curt
// Switched back to per vertex normals and smooth shading for terrain.
//
// Revision 1.13  1997/12/18 23:32:36  curt
// First stab at sky dome actually starting to look reasonable. :-)
//
// Revision 1.12  1997/12/17 23:13:47  curt
// Began working on rendering the sky.
//
// Revision 1.11  1997/12/15 23:55:01  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.10  1997/12/12 21:41:28  curt
// More light/material property tweaking ... still a ways off.
//
// Revision 1.9  1997/12/12 19:52:57  curt
// Working on lightling and material properties.
//
// Revision 1.8  1997/12/10 01:19:51  curt
// Tweaks for verion 0.15 release.
//
// Revision 1.7  1997/12/08 22:51:17  curt
// Enhanced to handle ccw and cw tri-stripe winding.  This is a temporary
// admission of defeat.  I will eventually go back and get all the stripes
// wound the same way (ccw).
//
// Revision 1.6  1997/11/25 19:25:35  curt
// Changes to integrate Durk's moon/sun code updates + clean up.
//
// Revision 1.5  1997/11/15 18:16:39  curt
// minor tweaks.
//
// Revision 1.4  1997/11/14 00:26:49  curt
// Transform scenery coordinates earlier in pipeline when scenery is being
// created, not when it is being loaded.  Precalculate normals for each node
// as average of the normals of each containing polygon so Garoude shading is
// now supportable.
//
// Revision 1.3  1997/10/31 04:49:12  curt
// Tweaking vertex orders.
//
// Revision 1.2  1997/10/30 12:38:45  curt
// Working on new scenery subsystem.
//
// Revision 1.1  1997/10/28 21:14:54  curt
// Initial revision.


