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

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include <stdio.h>
#include <string.h>

// #if defined ( __sun__ )
// extern "C" void *memmove(void *, const void *, size_t);
// extern "C" void *memset(void *, int, size_t);
// #endif

#include <Include/compiler.h>

#include STL_STRING
#include <map>			// STL
#include <vector>		// STL
#include <ctype.h>		// isdigit()

#include <Debug/logstream.hxx>
#include <Misc/fgstream.hxx>
#include <Include/fg_constants.h>
#include <Main/options.hxx>
#include <Math/mat3.h>
#include <Math/fg_geodesy.hxx>
#include <Math/fg_random.h>
#include <Math/point3d.hxx>
#include <Math/polar3d.hxx>
#include <Misc/stopwatch.hxx>
#include <Scenery/tileentry.hxx>

#include "materialmgr.hxx"
#include "obj.hxx"

FG_USING_STD(string);
FG_USING_STD(vector);


typedef vector < int > int_list;
typedef int_list::iterator int_list_iterator;
typedef int_list::const_iterator int_point_list_iterator;


static double normals[FG_MAX_NODES][3];
static double tex_coords[FG_MAX_NODES*3][3];


// given three points defining a triangle, calculate the normal
static void calc_normal(Point3D p1, Point3D p2, 
			Point3D p3, double normal[3])
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
static Point3D calc_tex_coords(const Point3D& node, const Point3D& ref) {
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


// Generate a generic ocean tile on the fly
ssgBranch *fgGenTile( const string& path, FGTileEntry *t) {
    fgFRAGMENT fragment;
    fragment.init();
    fragment.tile_ptr = t;

    ssgSimpleState *state = NULL;

    ssgBranch *tile = new ssgBranch () ;
    tile -> setName ( (char *)path.c_str() ) ;

    // find Ocean material in the properties list
    if ( ! material_mgr.find( "Ocean", fragment.material_ptr )) {
	FG_LOG( FG_TERRAIN, FG_ALERT, 
		"Ack! unknown usemtl name = " << "Ocean" 
		<< " in " << path );
    }

    // set the texture width and height values for this
    // material
    FGMaterial m = fragment.material_ptr->get_m();
    // double tex_width = m.get_xsize();
    // double tex_height = m.get_ysize();

    // set ssgState
    state = fragment.material_ptr->get_state();

    // Calculate center point
    FGBucket b = t->tile_bucket;
    double clon = b.get_center_lon();
    double clat = b.get_center_lat();
    double height = b.get_height();
    double width = b.get_width();

    Point3D center = fgGeodToCart(Point3D(clon*DEG_TO_RAD,clat*DEG_TO_RAD,0.0));
    t->center = center;
    fragment.center = center;
    // cout << "center = " << center << endl;;
    
    // Caculate corner vertices
    Point3D geod[4];
    geod[0] = Point3D( clon - width/2.0, clat - height/2.0, 0.0 );
    geod[1] = Point3D( clon + width/2.0, clat - height/2.0, 0.0 );
    geod[2] = Point3D( clon + width/2.0, clat + height/2.0, 0.0 );
    geod[3] = Point3D( clon - width/2.0, clat + height/2.0, 0.0 );

    Point3D rad[4];
    int i;
    for ( i = 0; i < 4; ++i ) {
	rad[i] = Point3D( geod[i].x() * DEG_TO_RAD, geod[i].y() * DEG_TO_RAD,
			  geod[i].z() );
    }

    Point3D cart[4], rel[4];
    t->nodes.clear();
    for ( i = 0; i < 4; ++i ) {
	cart[i] = fgGeodToCart(rad[i]);
	rel[i] = cart[i] - center;
	t->nodes.push_back( rel[i] );
	// cout << "corner " << i << " = " << cart[i] << endl;
    }

    t->ncount = 4;

    // Calculate bounding radius
    t->bounding_radius = center.distance3D( cart[0] );
    fragment.bounding_radius = t->bounding_radius;
    // cout << "bounding radius = " << t->bounding_radius << endl;

    // Calculate normals
    Point3D normals[4];
    for ( i = 0; i < 4; ++i ) {
	normals[i] = cart[i];
	double length = normals[i].distance3D( Point3D(0.0) );
	normals[i] /= length;
	// cout << "normal = " << normals[i] << endl;
    }

    // Calculate texture coordinates
    Point3D texs[4];
    for ( i = 0; i < 4; ++i ) {
	texs[i] = calc_tex_coords( rel[i], center );
	// cout << "texture coordinate = " << texs[i] << endl;
    }

    // Build flight gear structure
    fragment.add_face(0, 1, 2);
    fragment.add_face(0, 2, 3);
    t->fragment_list.push_back(fragment);

    // Build ssg structure
    t->vtlist = new sgVec3 [ 4 ];
    t->vnlist = new sgVec3 [ 4 ];
    t->tclist = new sgVec2 [ 4 ];

    for ( i = 0; i < 4; ++i ) {
	sgSetVec3( t->vtlist[i], 
		   rel[i].x(), rel[i].y(), rel[i].z() );
	sgSetVec3( t->vnlist[i], 
		   normals[i].x(), normals[i].y(), normals[i].z() );
	sgSetVec2( t->tclist[i], texs[i].x(), texs[i].y() );
    }
    
    unsigned short *vindex = new unsigned short [ 4 ];
    unsigned short *tindex = new unsigned short [ 4 ];
    for ( i = 0; i < 4; ++i ) {
	vindex[i] = i;
	tindex[i] = i;
    }

    ssgLeaf *leaf = 
	new ssgVTable ( GL_TRIANGLE_FAN,
			4, vindex, t->vtlist,
			4, vindex, t->vnlist,
			4, tindex, t->tclist,
			0, NULL, NULL ) ;
    leaf->setState( state );

    tile->addKid( leaf );

    return tile;
}


// Load a .obj file and build the fragment list
ssgBranch *fgObjLoad( const string& path, FGTileEntry *t) {
    fgFRAGMENT fragment;
    Point3D pp;
    double approx_normal[3] /*, normal[3], scale = 0.0 */;
    // double x, y, z, xmax, xmin, ymax, ymin, zmax, zmin;
    // GLfloat sgenparams[] = { 1.0, 0.0, 0.0, 0.0 };
    // GLint display_list = 0;
    int shading;
    bool in_fragment = false, in_faces = false;
    int vncount, vtcount;
    int n1 = 0, n2 = 0, n3 = 0, n4 = 0;
    int tex;
    int last1 = 0, last2 = 0;
    bool odd = false;
    point_list nodes;
    Point3D node;
    Point3D center;
    double scenery_version = 0.0;
    double tex_width = 1000.0, tex_height = 1000.0;
    bool shared_done = false;
    int_list fan_vertices;
    int_list fan_tex_coords;
    int i;
    ssgSimpleState *state = NULL;

    ssgBranch *tile = new ssgBranch () ;
    tile -> setName ( (char *)path.c_str() ) ;

    // Attempt to open "path.gz" or "path"
    fg_gzifstream in( path );
    if ( ! in.is_open() ) {
	FG_LOG( FG_TERRAIN, FG_ALERT, "Cannot open file: " << path );
	FG_LOG( FG_TERRAIN, FG_ALERT, "default to ocean tile: " << path );

	return fgGenTile( path, t );
    }

    shading = current_options.get_shading();

    in_fragment = false;
    t->ncount = 0;
    vncount = 0;
    vtcount = 0;
    t->bounding_radius = 0.0;
    center = t->center;

    StopWatch stopwatch;
    stopwatch.start();

    // ignore initial comments and blank lines. (priming the pump)
    // in >> skipcomment;
    // string line;

    string token;
    char c;

#ifdef __MWERKS__
    while ( in.get(c) && c  != '\0' ) {
	in.putback(c);
#else
    while ( ! in.eof() ) {
#endif

#if defined( MACOS )
	in >> ::skipws;
#else
	in >> skipws;
#endif

	if ( in.get( c ) && c == '#' ) {
	    // process a comment line

	    // getline( in, line );
	    // cout << "comment = " << line << endl;

	    in >> token;

	    if ( token == "Version" ) {
		// read scenery versions number
		in >> scenery_version;
		// cout << "scenery_version = " << scenery_version << endl;
	    } else if ( token == "gbs" ) {
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

		// if first usemtl with shared_done = false, then set
		// shared_done true and build the ssg shared lists
		if ( ! shared_done ) {
		    // sanity check
		    if ( (int)nodes.size() != vncount ) {
			FG_LOG( FG_TERRAIN, FG_ALERT, 
				"Tile has mismatched nodes and normals: " 
				<< path );
			// exit(-1);
		    }
		    shared_done = true;

		    t->vtlist = new sgVec3 [ nodes.size() ];
		    t->vnlist = new sgVec3 [ vncount ];
		    t->tclist = new sgVec2 [ vtcount ];

		    for ( i = 0; i < (int)nodes.size(); ++i ) {
			sgSetVec3( t->vtlist[i], 
				   nodes[i][0], nodes[i][1], nodes[i][2] );
		    }
		    for ( i = 0; i < vncount; ++i ) {
			sgSetVec3( t->vnlist[i], 
				   normals[i][0], 
				   normals[i][1],
				   normals[i][2] );
		    }
		    for ( i = 0; i < vtcount; ++i ) {
			sgSetVec2( t->tclist[i],
				   tex_coords[i][0],
				   tex_coords[i][1] );
		    }
		}

		// series of individual triangles
		// if ( in_faces ) {
		//     xglEnd();
	        // }

		// this also signals the start of a new fragment
		if ( in_fragment ) {
		    // close out the previous structure and start the next
		    // xglEndList();
		    // printf("xglEnd(); xglEndList();\n");

		    // update fragment
		    // fragment.display_list = display_list;

		    // push this fragment onto the tile's object list
		    t->fragment_list.push_back(fragment);
		} else {
		    in_fragment = true;
		}

		// printf("start of fragment (usemtl)\n");

		// display_list = xglGenLists(1);
		// xglNewList(display_list, GL_COMPILE);
		// printf("xglGenLists(); xglNewList();\n");
		in_faces = false;

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

		// set the texture width and height values for this
		// material
		FGMaterial m = fragment.material_ptr->get_m();
		tex_width = m.get_xsize();
		tex_height = m.get_ysize();
		state = fragment.material_ptr->get_state();
		// cout << "(w) = " << tex_width << " (h) = " 
		//      << tex_width << endl;

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
		if ( vncount < FG_MAX_NODES ) {
		    in >> normals[vncount][0]
		       >> normals[vncount][1]
		       >> normals[vncount][2];
		    vncount++;
		} else {
		    FG_LOG( FG_TERRAIN, FG_ALERT, 
			    "Read too many vertex normals in " << path 
			    << " ... dying :-(" );
		    exit(-1);
		}
	    } else if ( token == "vt" ) {
		// vertex texture coordinate
		if ( vtcount < FG_MAX_NODES*3 ) {
		    in >> tex_coords[vtcount][0]
		       >> tex_coords[vtcount][1];
		    vtcount++;
		} else {
		    FG_LOG( FG_TERRAIN, FG_ALERT, 
			    "Read too many vertex texture coords in " << path
			    << " ... dying :-("
			    );
		    exit(-1);
		}
	    } else if ( token == "v" ) {
		// node (vertex)
		if ( t->ncount < FG_MAX_NODES ) {
		    /* in >> nodes[t->ncount][0]
		       >> nodes[t->ncount][1]
		       >> nodes[t->ncount][2]; */
		    in >> node;
		    nodes.push_back(node);
		    t->ncount++;
		} else {
		    FG_LOG( FG_TERRAIN, FG_ALERT, 
			    "Read too many nodes in " << path 
			    << " ... dying :-(");
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

		// xglBegin(GL_TRIANGLE_STRIP);
		// printf("xglBegin(tristrip) %d %d %d\n", n1, n2, n3);

		odd = true; 
		// scale = 1.0;

		if ( shading ) {
		    // Shading model is "GL_SMOOTH" so use precalculated
		    // (averaged) normals
		    // MAT3_SCALE_VEC(normal, normals[n1], scale);
		    // xglNormal3dv(normal);
		    pp = calc_tex_coords(nodes[n1], center);
		    // xglTexCoord2f(pp.lon(), pp.lat());
		    // xglVertex3dv(nodes[n1].get_n());		

		    // MAT3_SCALE_VEC(normal, normals[n2], scale);
		    // xglNormal3dv(normal);
		    pp = calc_tex_coords(nodes[n2], center);
		    // xglTexCoord2f(pp.lon(), pp.lat());
		    // xglVertex3dv(nodes[n2].get_n());				

		    // MAT3_SCALE_VEC(normal, normals[n3], scale);
		    // xglNormal3dv(normal);
		    pp = calc_tex_coords(nodes[n3], center);
		    // xglTexCoord2f(pp.lon(), pp.lat());
		    // xglVertex3dv(nodes[n3].get_n());
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
		    // xglNormal3dv(normal);

		    pp = calc_tex_coords(nodes[n1], center);
		    // xglTexCoord2f(pp.lon(), pp.lat());
		    // xglVertex3dv(nodes[n1].get_n());		

		    pp = calc_tex_coords(nodes[n2], center);
		    // xglTexCoord2f(pp.lon(), pp.lat());
		    // xglVertex3dv(nodes[n2].get_n());		
		    
		    pp = calc_tex_coords(nodes[n3], center);
		    // xglTexCoord2f(pp.lon(), pp.lat());
		    // xglVertex3dv(nodes[n3].get_n());		
		}
		// printf("some normals, texcoords, and vertices\n");

		odd = !odd;
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
		    // xglNormal3dv(normal);
		    pp = calc_tex_coords(nodes[n4], center);
		    // xglTexCoord2f(pp.lon(), pp.lat());
		    // xglVertex3dv(nodes[n4].get_n());		
		    
		    odd = !odd;
		    last1 = n3;
		    last2 = n4;
		    // printf("a normal, texcoord, and vertex (4th)\n");
		}
	    } else if ( (token == "tf") || (token == "ts") ) {
		// triangle fan
		// fgPrintf( FG_TERRAIN, FG_DEBUG, "new fan");

		fan_vertices.clear();
		fan_tex_coords.clear();
		odd = true;

		// xglBegin(GL_TRIANGLE_FAN);

		in >> n1;
		fan_vertices.push_back( n1 );
		// xglNormal3dv(normals[n1]);
		if ( in.get( c ) && c == '/' ) {
		    in >> tex;
		    fan_tex_coords.push_back( tex );
		    if ( scenery_version >= 0.4 ) {
			if ( tex_width > 0 ) {
			    t->tclist[tex][0] *= (1000.0 / tex_width);
			}
			if ( tex_height > 0 ) {
			    t->tclist[tex][1] *= (1000.0 / tex_height);
			}
		    }
		    pp.setx( tex_coords[tex][0] * (1000.0 / tex_width) );
		    pp.sety( tex_coords[tex][1] * (1000.0 / tex_height) );
		} else {
		    in.putback( c );
		    pp = calc_tex_coords(nodes[n1], center);
		}
		// xglTexCoord2f(pp.x(), pp.y());
		// xglVertex3dv(nodes[n1].get_n());

		in >> n2;
		fan_vertices.push_back( n2 );
		// xglNormal3dv(normals[n2]);
		if ( in.get( c ) && c == '/' ) {
		    in >> tex;
		    fan_tex_coords.push_back( tex );
		    if ( scenery_version >= 0.4 ) {
			if ( tex_width > 0 ) {
			    t->tclist[tex][0] *= (1000.0 / tex_width);
			}
			if ( tex_height > 0 ) {
			    t->tclist[tex][1] *= (1000.0 / tex_height);
			}
		    }
		    pp.setx( tex_coords[tex][0] * (1000.0 / tex_width) );
		    pp.sety( tex_coords[tex][1] * (1000.0 / tex_height) );
		} else {
		    in.putback( c );
		    pp = calc_tex_coords(nodes[n2], center);
		}
		// xglTexCoord2f(pp.x(), pp.y());
		// xglVertex3dv(nodes[n2].get_n());
		
		// read all subsequent numbers until next thing isn't a number
		while ( true ) {
#if defined( MACOS )
		    in >> ::skipws;
#else
		    in >> skipws;
#endif

		    char c;
		    in.get(c);
		    in.putback(c);
		    if ( ! isdigit(c) || in.eof() ) {
			break;
		    }

		    in >> n3;
		    fan_vertices.push_back( n3 );
		    // cout << "  triangle = " 
		    //      << n1 << "," << n2 << "," << n3 
		    //      << endl;
		    // xglNormal3dv(normals[n3]);
		    if ( in.get( c ) && c == '/' ) {
			in >> tex;
			fan_tex_coords.push_back( tex );
			if ( scenery_version >= 0.4 ) {
			    if ( tex_width > 0 ) {
				t->tclist[tex][0] *= (1000.0 / tex_width);
			    }
			    if ( tex_height > 0 ) {
				t->tclist[tex][1] *= (1000.0 / tex_height);
			    }
			}
			pp.setx( tex_coords[tex][0] * (1000.0 / tex_width) );
			pp.sety( tex_coords[tex][1] * (1000.0 / tex_height) );
		    } else {
			in.putback( c );
			pp = calc_tex_coords(nodes[n3], center);
		    }
		    // xglTexCoord2f(pp.x(), pp.y());
		    // xglVertex3dv(nodes[n3].get_n());

		    if ( token == "tf" ) {
			// triangle fan
			fragment.add_face(n1, n2, n3);
			n2 = n3;
		    } else {
			// triangle strip
			if ( odd ) {
			    fragment.add_face(n1, n2, n3);
			} else {
			    fragment.add_face(n2, n1, n3);
			}
			odd = !odd;
			n1 = n2;
			n2 = n3;
		    }
		}

		// xglEnd();

		// build the ssg entity
		unsigned short *vindex = 
		    new unsigned short [ fan_vertices.size() ];
		unsigned short *tindex = 
		    new unsigned short [ fan_tex_coords.size() ];
		for ( i = 0; i < (int)fan_vertices.size(); ++i ) {
		    vindex[i] = fan_vertices[i];
		}
		for ( i = 0; i < (int)fan_tex_coords.size(); ++i ) {
		    tindex[i] = fan_tex_coords[i];
		}
		ssgLeaf *leaf;
		if ( token == "tf" ) {
		    // triangle fan
		    leaf = 
			new ssgVTable ( GL_TRIANGLE_FAN,
					fan_vertices.size(), vindex, t->vtlist,
					fan_vertices.size(), vindex, t->vnlist,
					fan_tex_coords.size(), tindex,t->tclist,
					0, NULL, NULL ) ;
		} else {
		    // triangle strip
		    leaf = 
			new ssgVTable ( GL_TRIANGLE_STRIP,
					fan_vertices.size(), vindex, t->vtlist,
					fan_vertices.size(), vindex, t->vnlist,
					fan_tex_coords.size(), tindex,t->tclist,
					0, NULL, NULL ) ;
		}
		leaf->setState( state );

		tile->addKid( leaf );

	    } else if ( token == "f" ) {
		// unoptimized face

		if ( !in_faces ) {
		    // xglBegin(GL_TRIANGLES);
		    // printf("xglBegin(triangles)\n");
		    in_faces = true;
		}

		// fgPrintf( FG_TERRAIN, FG_DEBUG, "new triangle = %s", line);*/
		in >> n1 >> n2 >> n3;
		fragment.add_face(n1, n2, n3);

		// xglNormal3d(normals[n1][0], normals[n1][1], normals[n1][2]);
		// xglNormal3dv(normals[n1]);
		pp = calc_tex_coords(nodes[n1], center);
		// xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3dv(nodes[n1].get_n());

		// xglNormal3dv(normals[n2]);
		pp = calc_tex_coords(nodes[n2], center);
		// xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3dv(nodes[n2].get_n());
		
		// xglNormal3dv(normals[n3]);
		pp = calc_tex_coords(nodes[n3], center);
		// xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3dv(nodes[n3].get_n());
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
			calc_normal(nodes[last1], nodes[last2], 
				    nodes[n1], approx_normal);
		    } else {
			calc_normal(nodes[last2], nodes[last1], 
				    nodes[n1], approx_normal);
		    }
		    // MAT3_SCALE_VEC(normal, approx_normal, scale);
		}
		// xglNormal3dv(normal);

		pp = calc_tex_coords(nodes[n1], center);
		// xglTexCoord2f(pp.lon(), pp.lat());
		// xglVertex3dv(nodes[n1].get_n());
		// printf("a normal, texcoord, and vertex (4th)\n");
   
		odd = !odd;
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
		    // xglNormal3dv(normal);
		
		    pp = calc_tex_coords(nodes[n2], center);
		    // xglTexCoord2f(pp.lon(), pp.lat());
		    // xglVertex3dv(nodes[n2].get_n());		
		    // printf("a normal, texcoord, and vertex (4th)\n");

		    odd = !odd;
		    last1 = last2;
		    last2 = n2;
		}
	    } else {
		FG_LOG( FG_TERRAIN, FG_WARN, "Unknown token in " 
			<< path << " = " << token );
	    }

	    // eat white space before start of while loop so if we are
	    // done with useful input it is noticed before hand.
#if defined( MACOS )
	    in >> ::skipws;
#else
	    in >> skipws;
#endif
	}
    }

    if ( in_fragment ) {
	// close out the previous structure and start the next
	// xglEnd();
	// xglEndList();
	// printf("xglEnd(); xglEndList();\n");
	
	// update fragment
	// fragment.display_list = display_list;
	
	// push this fragment onto the tile's object list
	t->fragment_list.push_back(fragment);
    }

#if 0
    // Draw normal vectors (for visually verifying normals)
    xglBegin(GL_LINES);
    xglColor3f(0.0, 0.0, 0.0);
    for ( i = 0; i < t->ncount; i++ ) {
	xglVertex3d(nodes[i][0],
		    nodes[i][1] ,
 		    nodes[i][2]);
	xglVertex3d(nodes[i][0] + 500*normals[i][0],
 		    nodes[i][1] + 500*normals[i][1],
 		    nodes[i][2] + 500*normals[i][2]);
    } 
    xglEnd();
#endif

    t->nodes = nodes;

    stopwatch.stop();
    FG_LOG( FG_TERRAIN, FG_DEBUG, 
	    "Loaded " << path << " in " 
	    << stopwatch.elapsedSeconds() << " seconds" );
    
    return tile;
}


