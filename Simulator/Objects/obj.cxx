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

// #if defined ( __sun__ )
// extern "C" void *memmove(void *, const void *, size_t);
// extern "C" void *memset(void *, int, size_t);
// #endif

#include <Include/compiler.h>

#include STL_STRING
#include <map>          // STL
#include <ctype.h>      // isdigit()

#include <Debug/logstream.hxx>
#include <Misc/fgstream.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Math/mat3.h>
#include <Math/fg_random.h>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Misc/stopwatch.hxx>
#include <Scenery/tile.hxx>

#include "material.hxx"
#include "obj.hxx"

FG_USING_STD(string);


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
    double approx_normal[3], normal[3] /*, scale = 0.0 */;
    // double x, y, z, xmax, xmin, ymax, ymin, zmax, zmin;
    // GLfloat sgenparams[] = { 1.0, 0.0, 0.0, 0.0 };
    GLint display_list = 0;
    int shading;
    int in_fragment = 0, in_faces = 0, vncount;
    int n1 = 0, n2 = 0, n3 = 0, n4 = 0;
    int last1 = 0, last2 = 0, odd = 0;
    double (*nodes)[3];
    Point3D center;

    // printf("loading %s\n", path.c_str() );

    // Attempt to open "path.gz" or "path"
    fg_gzifstream in( path );
    if ( ! in ) {
	FG_LOG( FG_TERRAIN, FG_ALERT, "Cannot open file: " << path );
	return 0;
    }

    shading = current_options.get_shading();

    in_fragment = 0;
    t->ncount = 0;
    vncount = 0;
    t->bounding_radius = 0.0;
    nodes = t->nodes;
    center = t->center;

    StopWatch stopwatch;
    stopwatch.start();

    // ignore initial comments and blank lines. (priming the pump)
    // in >> skipcomment;
    string line;

    while ( ! in.eof() ) {
	string token;
	char c;

	in >> skipws;

	if ( in.get( c ) && c == '#' ) {
	    // process a comment line

	    // getline( in, line );
	    // cout << "comment = " << line << endl;

	    in >> token;

	    if ( token == "gbs" ) {
		// reference point (center offset)
		in >> t->center >> t->bounding_radius;
		center = t->center;
		// cout << "center = " << center 
		//      << " radius = " << t->bounding_radius << endl;
	    } else if ( token == "bs" )	{
		// reference point (center offset)
		in >> fragment.center;
		in >> fragment.bounding_radius;

		// cout << "center = " << fragment.center 
		//      << " radius = " << fragment.bounding_radius << endl;
	    } else if ( token == "usemtl" ) {
		// material property specification

		// series of individual triangles
		if ( in_faces ) {
		    xglEnd();
		}

		// this also signals the start of a new fragment
		if ( in_fragment ) {
		    // close out the previous structure and start the next
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
		in >> material;
		fragment.tile_ptr = t;
		
		// find this material in the properties list
		if ( ! material_mgr.find( material, fragment.material_ptr )) {
		    FG_LOG( FG_TERRAIN, FG_ALERT, 
			    "Ack! unknown usemtl name = " << material 
			    << " in " << path );
		}
		
		// initialize the fragment transformation matrix
		/*
		 for ( i = 0; i < 16; i++ ) {
	           fragment.matrix[i] = 0.0;
	         }
	         fragment.matrix[0] = fragment.matrix[5] =
		 fragment.matrix[10] = fragment.matrix[15] = 1.0;
	        */
	    } else {
		// unknown comment, just gobble the input untill the
		// end of line

		in >> skipeol;
	    }
	} else {
	    in.putback( c );
	
	    in >> token;

	    // cout << "token = " << token << endl;

	    if ( token == "vn" ) {
		// vertex normal
		if ( vncount < MAX_NODES ) {
		    in >> normals[vncount][0]
		       >> normals[vncount][1]
		       >> normals[vncount][2];
		    vncount++;
		} else {
		    FG_LOG( FG_TERRAIN, FG_ALERT, 
			    "Read too many vertex normals ... dying :-(" );
		    exit(-1);
		}
	    } else if ( token == "v" ) {
		// node (vertex)
		if ( t->ncount < MAX_NODES ) {
		    in >> t->nodes[t->ncount][0]
		       >> t->nodes[t->ncount][1]
		       >> t->nodes[t->ncount][2];
		    t->ncount++;
		} else {
		    FG_LOG( FG_TERRAIN, FG_ALERT, 
			    "Read too many nodes ... dying :-(");
		    exit(-1);
		}
	    } else if ( token == "t" ) {
		// start a new triangle strip

		n1 = n2 = n3 = n4 = 0;

		// fgPrintf( FG_TERRAIN, FG_DEBUG, 
		//           "    new tri strip = %s", line);
		in >> n1 >> n2 >> n3;
		fragment.add_face(n1, n2, n3);

		// fgPrintf( FG_TERRAIN, FG_DEBUG, "(t) = ");

		xglBegin(GL_TRIANGLE_STRIP);
		// printf("xglBegin(tristrip) %d %d %d\n", n1, n2, n3);

		odd = 1; 
		// scale = 1.0;

		if ( shading ) {
		    // Shading model is "GL_SMOOTH" so use precalculated
		    // (averaged) normals
		    // MAT3_SCALE_VEC(normal, normals[n1], scale);
		    xglNormal3dv(normal);
		    pp = calc_tex_coords(nodes[n1], center);
		    xglTexCoord2f(pp.lon(), pp.lat());
		    xglVertex3dv(nodes[n1]);		

		    // MAT3_SCALE_VEC(normal, normals[n2], scale);
		    xglNormal3dv(normal);
		    pp = calc_tex_coords(nodes[n2], center);
		    xglTexCoord2f(pp.lon(), pp.lat());
		    xglVertex3dv(nodes[n2]);				

		    // MAT3_SCALE_VEC(normal, normals[n3], scale);
		    xglNormal3dv(normal);
		    pp = calc_tex_coords(nodes[n3], center);
		    xglTexCoord2f(pp.lon(), pp.lat());
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
		    // MAT3_SCALE_VEC(normal, approx_normal, scale);
		    xglNormal3dv(normal);

		    pp = calc_tex_coords(nodes[n1], center);
		    xglTexCoord2f(pp.lon(), pp.lat());
		    xglVertex3dv(nodes[n1]);		

		    pp = calc_tex_coords(nodes[n2], center);
		    xglTexCoord2f(pp.lon(), pp.lat());
		    xglVertex3dv(nodes[n2]);		
		    
		    pp = calc_tex_coords(nodes[n3], center);
		    xglTexCoord2f(pp.lon(), pp.lat());
		    xglVertex3dv(nodes[n3]);		
		}
		// printf("some normals, texcoords, and vertices\n");

		odd = 1 - odd;
		last1 = n2;
		last2 = n3;

		// There can be three or four values 
		char c;
		while ( in.get(c) ) {
		    if ( c == '\n' ) {
			break; // only the one
		    }
		    if ( isdigit(c) ){
			in.putback(c);
			in >> n4;
			break;
		    }
		}

		if ( n4 > 0 ) {
		    fragment.add_face(n3, n2, n4);

		    if ( shading ) {
			// Shading model is "GL_SMOOTH"
			// MAT3_SCALE_VEC(normal, normals[n4], scale);
		    } else {
			// Shading model is "GL_FLAT"
			calc_normal(nodes[n3], nodes[n2], nodes[n4], 
				    approx_normal);
			// MAT3_SCALE_VEC(normal, approx_normal, scale);
		    }
		    xglNormal3dv(normal);
		    pp = calc_tex_coords(nodes[n4], center);
		    xglTexCoord2f(pp.lon(), pp.lat());
		    xglVertex3dv(nodes[n4]);		
		    
		    odd = 1 - odd;
		    last1 = n3;
		    last2 = n4;
		    // printf("a normal, texcoord, and vertex (4th)\n");
		}
	    } else if ( token == "tf" ) {
		// triangle fan
		// fgPrintf( FG_TERRAIN, FG_DEBUG, "new fan");

		xglBegin(GL_TRIANGLE_FAN);

		in >> n1;
		xglNormal3dv(normals[n1]);
		pp = calc_tex_coords(nodes[n1], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		xglVertex3dv(nodes[n1]);

		in >> n2;
		xglNormal3dv(normals[n2]);
		pp = calc_tex_coords(nodes[n2], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		xglVertex3dv(nodes[n2]);
		
		// read all subsequent numbers until next thing isn't a number
		while ( true ) {
		    in >> skipws;

		    char c;
		    in.get(c);
		    in.putback(c);
		    if ( ! isdigit(c) || in.eof() ) {
			break;
		    }

		    in >> n3;
		    // cout << "  triangle = " 
		    //      << n1 << "," << n2 << "," << n3 
		    //      << endl;
		    xglNormal3dv(normals[n3]);
		    pp = calc_tex_coords(nodes[n3], center);
		    xglTexCoord2f(pp.lon(), pp.lat());
		    xglVertex3dv(nodes[n3]);

		    fragment.add_face(n1, n2, n3);
		    n2 = n3;
		}

		xglEnd();
	    } else if ( token == "f" ) {
		// unoptimized face

		if ( !in_faces ) {
		    xglBegin(GL_TRIANGLES);
		    // printf("xglBegin(triangles)\n");
		    in_faces = 1;
		}

		// fgPrintf( FG_TERRAIN, FG_DEBUG, "new triangle = %s", line);*/
		in >> n1 >> n2 >> n3;
		fragment.add_face(n1, n2, n3);

		// xglNormal3d(normals[n1][0], normals[n1][1], normals[n1][2]);
		xglNormal3dv(normals[n1]);
		pp = calc_tex_coords(nodes[n1], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		xglVertex3dv(nodes[n1]);

		xglNormal3dv(normals[n2]);
		pp = calc_tex_coords(nodes[n2], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		xglVertex3dv(nodes[n2]);
		
		xglNormal3dv(normals[n3]);
		pp = calc_tex_coords(nodes[n3], center);
		xglTexCoord2f(pp.lon(), pp.lat());
		xglVertex3dv(nodes[n3]);
		// printf("some normals, texcoords, and vertices (tris)\n");
	    } else if ( token == "q" ) {
		// continue a triangle strip
		n1 = n2 = 0;

		// fgPrintf( FG_TERRAIN, FG_DEBUG, "continued tri strip = %s ", 
		//           line);
		in >> n1;

		// There can be one or two values 
		char c;
		while ( in.get(c) ) {
		    if ( c == '\n' ) {
			break; // only the one
		    }

		    if ( isdigit(c) ) {
			in.putback(c);
			in >> n2;
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
		    // MAT3_SCALE_VEC(normal, normals[n1], scale);
		} else {
		    // Shading model is "GL_FLAT"
		    if ( odd ) {
			calc_normal(nodes[last1], nodes[last2], nodes[n1], 
				    approx_normal);
		    } else {
			calc_normal(nodes[last2], nodes[last1], nodes[n1], 
				    approx_normal);
		    }
		    // MAT3_SCALE_VEC(normal, approx_normal, scale);
		}
		xglNormal3dv(normal);

		pp = calc_tex_coords(nodes[n1], center);
		xglTexCoord2f(pp.lon(), pp.lat());
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
			// MAT3_SCALE_VEC(normal, normals[n2], scale);
		    } else {
			// Shading model is "GL_FLAT"
			if ( odd ) {
			    calc_normal(nodes[last1], nodes[last2], 
					nodes[n2], approx_normal);
			} else {
			    calc_normal(nodes[last2], nodes[last1], 
					nodes[n2], approx_normal);
			}
			// MAT3_SCALE_VEC(normal, approx_normal, scale);
		    }
		    xglNormal3dv(normal);
		
		    pp = calc_tex_coords(nodes[n2], center);
		    xglTexCoord2f(pp.lon(), pp.lat());
		    xglVertex3dv(nodes[n2]);		
		    // printf("a normal, texcoord, and vertex (4th)\n");

		    odd = 1 -odd;
		    last1 = last2;
		    last2 = n2;
		}
	    } else {
		FG_LOG( FG_TERRAIN, FG_WARN, "Unknown token in " 
			<< path << " = " << token );
	    }

	    // eat white space before start of while loop so if we are
	    // done with useful input it is noticed before hand.
	    in >> skipws;
	}
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

#if 0
    // Draw normal vectors (for visually verifying normals)
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
#endif

    stopwatch.stop();
    FG_LOG( FG_TERRAIN, FG_INFO, 
	    "Loaded " << path << " in " 
	    << stopwatch.elapsedSeconds() << " seconds" );
    
    return 1;
}


