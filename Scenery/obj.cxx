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

#include <map>     // STL
#include <string>  // Standard C++ library

#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Include/fg_zlib.h>
#include <Main/options.hxx>
#include <Math/mat3.h>
#include <Math/fg_random.h>
#include <Math/polar3d.h>

#include "material.hxx"
#include "obj.hxx"
#include "scenery.hxx"
#include "tile.hxx"


#define MAXNODES 100000

static double nodes[MAXNODES][3];
static double normals[MAXNODES][3];


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


#define FG_TEX_CONSTANT 128.0


// Calculate texture coordinates for a given point.
fgPolarPoint3d calc_tex_coords(double *node, fgCartesianPoint3d *ref) {
    fgCartesianPoint3d cp;
    fgPolarPoint3d pp;

    cp.x = node[0] + ref->x; 
    cp.y = node[1] + ref->y;
    cp.z = node[2] + ref->z;

    pp = fgCartToPolar3d(cp);

    pp.lon = fmod(RAD_TO_DEG * FG_TEX_CONSTANT * pp.lon, 25.0);
    pp.lat = fmod(RAD_TO_DEG * FG_TEX_CONSTANT * pp.lat, 25.0);

    return(pp);
}


// Load a .obj file and build the GL fragment list
int fgObjLoad(char *path, fgTILE *tile) {
    fgOPTIONS *o;
    fgFRAGMENT fragment;
    fgPolarPoint3d pp;
    char fgpath[256], line[256], material[256];
    double approx_normal[3], normal[3], scale;
    // double x, y, z, xmax, xmin, ymax, ymin, zmax, zmin;
    // GLfloat sgenparams[] = { 1.0, 0.0, 0.0, 0.0 };
    GLint display_list;
    fgFile f;
    int in_fragment, in_faces, ncount, vncount, n1, n2, n3, n4;
    int last1, last2, odd;
    int i;

    o = &current_options;

    // First try "path.gz" (compressed format)
    strcpy(fgpath, path);
    strcat(fgpath, ".gz");
    if ( (f = fgopen(fgpath, "rb")) == NULL ) {
	// Next try "path" (uncompressed format)
	strcpy(fgpath, path);
	if ( (f = fgopen(fgpath, "rb")) == NULL ) {
	    // Next try "path.obj" (uncompressed format)
	    strcat(fgpath, ".gz");
	    if ( (f = fgopen(fgpath, "rb")) == NULL ) {
		strcpy(fgpath, path);
		fgPrintf( FG_TERRAIN, FG_ALERT, 
			  "Cannot open file: %s\n", fgpath );
		return(0);
	    }
	}
    }

    in_fragment = 0;
    ncount = 1;
    vncount = 1;
    tile->bounding_radius = 0.0;

    while ( fggets(f, line, 250) != NULL ) {
	if ( line[0] == '#' ) {
	    // comment -- ignore
	} else if ( line[0] == '\n' ) {
	    // empty line -- ignore
	} else if ( strncmp(line, "gbs ", 4) == 0 ) {
	    // reference point (center offset)
	    sscanf(line, "gbs %lf %lf %lf %lf\n", 
		   &tile->center.x, &tile->center.y, &tile->center.z, 
		   &tile->bounding_radius);
	} else if ( strncmp(line, "bs ", 3) == 0 ) {
	    // reference point (center offset)
	    sscanf(line, "bs %lf %lf %lf %lf\n", 
		   &fragment.center.x, &fragment.center.y, &fragment.center.z, 
		   &fragment.bounding_radius);
	} else if ( strncmp(line, "v ", 2) == 0 ) {
	    // node (vertex)
	    if ( ncount < MAXNODES ) {
		// fgPrintf( FG_TERRAIN, FG_DEBUG, "vertex = %s", line);
		sscanf(line, "v %lf %lf %lf\n", 
		       &nodes[ncount][0], &nodes[ncount][1], 
		       &nodes[ncount][2]);

		ncount++;
	    } else {
		fgPrintf( FG_TERRAIN, FG_EXIT, 
			  "Read too many nodes ... dying :-(\n");
	    }
	} else if ( strncmp(line, "vn ", 3) == 0 ) {
	    // vertex normal
	    if ( vncount < MAXNODES ) {
		// fgPrintf( FG_TERRAIN, FG_DEBUG, "vertex normal = %s", line);
		sscanf(line, "vn %lf %lf %lf\n", 
		       &normals[vncount][0], &normals[vncount][1], 
		       &normals[vncount][2]);
		vncount++;
	    } else {
		fgPrintf( FG_TERRAIN, FG_EXIT, 
			  "Read too many vertex normals ... dying :-(\n");
	    }
	} else if ( strncmp(line, "usemtl ", 7) == 0 ) {
	    // material property specification

	    // this also signals the start of a new fragment
	    if ( in_fragment ) {
		// close out the previous structure and start the next
		xglEnd();
		xglEndList();

		// update fragment
		fragment.display_list = display_list;

		// push this fragment onto the tile's object list
		tile->fragment_list.push_back(fragment);
	    } else {
		in_fragment = 1;
	    }

	    display_list = xglGenLists(1);
	    xglNewList(display_list, GL_COMPILE);
	    in_faces = 0;

	    // scan the material line
	    sscanf(line, "usemtl %s\n", material);

	    // find this material in the properties list
	    map < string, fgMATERIAL, less<string> > :: iterator myfind = 
		material_mgr.material_map.find(material);
	    if ( myfind == material_mgr.material_map.end() ) {
		fgPrintf( FG_TERRAIN, FG_ALERT, 
			  "Ack! unknown usemtl name = %s in %s\n",
			  material, path);
	    } else {
		(fgMATERIAL *)fragment.material_ptr = &(*myfind).second;
	    }

	    // initialize the fragment transformation matrix
	    /*
	    for ( i = 0; i < 16; i++ ) {
		fragment.matrix[i] = 0.0;
	    }
	    fragment.matrix[0] = fragment.matrix[5] =
		fragment.matrix[10] = fragment.matrix[15] = 1.0;
	    */
	} else if ( line[0] == 't' ) {
	    // start a new triangle strip

	    n1 = n2 = n3 = n4 = 0;

	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "    new tri strip = %s", line);
	    sscanf(line, "t %d %d %d %d\n", &n1, &n2, &n3, &n4);

	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "(t) = ");

	    xglBegin(GL_TRIANGLE_STRIP);

	    odd = 1; 
	    scale = 1.0;

	    if ( o->shading ) {
		// Shading model is "GL_SMOOTH" so use precalculated
		// (averaged) normals
		MAT3_SCALE_VEC(normal, normals[n1], scale);
		xglNormal3dv(normal);
		pp = calc_tex_coords(nodes[n1], &tile->center);
		xglTexCoord2f(pp.lon, pp.lat);
		xglVertex3d(nodes[n1][0], nodes[n1][1], nodes[n1][2]);

		MAT3_SCALE_VEC(normal, normals[n2], scale);
		xglNormal3dv(normal);
                pp = calc_tex_coords(nodes[n2], &tile->center);
                xglTexCoord2f(pp.lon, pp.lat);
		xglVertex3d(nodes[n2][0], nodes[n2][1], nodes[n2][2]);

		MAT3_SCALE_VEC(normal, normals[n3], scale);
		xglNormal3dv(normal);
                pp = calc_tex_coords(nodes[n3], &tile->center);
                xglTexCoord2f(pp.lon, pp.lat);
		xglVertex3d(nodes[n3][0], nodes[n3][1], nodes[n3][2]);
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

                pp = calc_tex_coords(nodes[n1], &tile->center);
                xglTexCoord2f(pp.lon, pp.lat);
		xglVertex3d(nodes[n1][0], nodes[n1][1], nodes[n1][2]);

                pp = calc_tex_coords(nodes[n2], &tile->center);
                xglTexCoord2f(pp.lon, pp.lat);
		xglVertex3d(nodes[n2][0], nodes[n2][1], nodes[n2][2]);

                pp = calc_tex_coords(nodes[n3], &tile->center);
                xglTexCoord2f(pp.lon, pp.lat);
		xglVertex3d(nodes[n3][0], nodes[n3][1], nodes[n3][2]);
	    }

	    odd = 1 - odd;
	    last1 = n2;
	    last2 = n3;

	    if ( n4 > 0 ) {
		if ( o->shading ) {
		    // Shading model is "GL_SMOOTH"
		    MAT3_SCALE_VEC(normal, normals[n4], scale);
		} else {
		    // Shading model is "GL_FLAT"
		    calc_normal(nodes[n3], nodes[n2], nodes[n4], approx_normal);
		    MAT3_SCALE_VEC(normal, approx_normal, scale);
		}
		xglNormal3dv(normal);
		pp = calc_tex_coords(nodes[n4], &tile->center);
                xglTexCoord2f(pp.lon, pp.lat);
		xglVertex3d(nodes[n4][0], nodes[n4][1], nodes[n4][2]);

		odd = 1 - odd;
		last1 = n3;
		last2 = n4;
	    }
	} else if ( line[0] == 'f' ) {
	    // unoptimized face

	    if ( !in_faces ) {
		xglBegin(GL_TRIANGLES);
		in_faces = 1;
	    }

	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "new triangle = %s", line);*/
	    sscanf(line, "f %d %d %d\n", &n1, &n2, &n3);

            xglNormal3d(normals[n1][0], normals[n1][1], normals[n1][2]);
	    pp = calc_tex_coords(nodes[n1], &tile->center);
	    xglTexCoord2f(pp.lon, pp.lat);
 	    xglVertex3d(nodes[n1][0], nodes[n1][1], nodes[n1][2]);

            xglNormal3d(normals[n2][0], normals[n2][1], normals[n2][2]);
            pp = calc_tex_coords(nodes[n2], &tile->center);
            xglTexCoord2f(pp.lon, pp.lat);
	    xglVertex3d(nodes[n2][0], nodes[n2][1], nodes[n2][2]);
		
            xglNormal3d(normals[n3][0], normals[n3][1], normals[n3][2]);
            pp = calc_tex_coords(nodes[n3], &tile->center);
            xglTexCoord2f(pp.lon, pp.lat);
	    xglVertex3d(nodes[n3][0], nodes[n3][1], nodes[n3][2]);
	} else if ( line[0] == 'q' ) {
	    // continue a triangle strip
	    n1 = n2 = 0;

	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "continued tri strip = %s ", 
	    //           line);
	    sscanf(line, "q %d %d\n", &n1, &n2);
	    // fgPrintf( FG_TERRAIN, FG_DEBUG, "read %d %d\n", n1, n2);

	    if ( o->shading ) {
		// Shading model is "GL_SMOOTH"
		MAT3_SCALE_VEC(normal, normals[n1], scale);
		xglNormal3dv(normal);
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
		xglNormal3dv(normal);
	    }

            pp = calc_tex_coords(nodes[n1], &tile->center);
            xglTexCoord2f(pp.lon, pp.lat);
	    xglVertex3d(nodes[n1][0], nodes[n1][1], nodes[n1][2]);
    
	    odd = 1 - odd;
	    last1 = last2;
	    last2 = n1;

	    if ( n2 > 0 ) {
		// fgPrintf( FG_TERRAIN, FG_DEBUG, " (cont)\n");

		if ( o->shading ) {
		    // Shading model is "GL_SMOOTH"
		    MAT3_SCALE_VEC(normal, normals[n2], scale);
		    xglNormal3dv(normal);
		} else {
		    // Shading model is "GL_FLAT"
		    if ( odd ) {
			calc_normal(nodes[last1], nodes[last2], nodes[n2], 
				    approx_normal);
		    } else {
			calc_normal(nodes[last2], nodes[last1], nodes[n2], 
				    approx_normal);
		    }
		    MAT3_SCALE_VEC(normal, approx_normal, scale);
		    xglNormal3dv(normal);
		}

		pp = calc_tex_coords(nodes[n2], &tile->center);
		xglTexCoord2f(pp.lon, pp.lat);
		xglVertex3d(nodes[n2][0], nodes[n2][1], nodes[n2][2]);

		odd = 1 -odd;
		last1 = last2;
		last2 = n2;
	    }
	} else {
	    fgPrintf( FG_TERRAIN, FG_WARN, "Unknown line in %s = %s\n", 
		      path, line);
	}
    }

    if ( in_fragment ) {
	// close out the previous structure and start the next
	xglEnd();
	xglEndList();

	// update fragment
	fragment.display_list = display_list;
	
	// push this fragment onto the tile's object list
	tile->fragment_list.push_back(fragment);
    }

    // Draw normal vectors (for visually verifying normals)
    /*
    xglBegin(GL_LINES);
    xglColor3f(0.0, 0.0, 0.0);
    for ( i = 0; i < ncount; i++ ) {
        xglVertex3d(nodes[i][0],
 		    nodes[i][1] ,
 		    nodes[i][2]);
 	xglVertex3d(nodes[i][0] + 500*normals[i][0],
 		    nodes[i][1] + 500*normals[i][1],
 		    nodes[i][2] + 500*normals[i][2]);
    } 
    xglEnd();
    */   

    fgclose(f);

    return(1);
}


// $Log$
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


