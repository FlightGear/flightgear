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

#include <simgear/compiler.h>

#include STL_STRING
#include <map>			// STL
#include <vector>		// STL
#include <ctype.h>		// isdigit()

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/fg_geodesy.hxx>
#include <simgear/math/fg_random.h>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/misc/fgstream.hxx>
#include <simgear/misc/stopwatch.hxx>
#include <simgear/misc/texcoord.hxx>

#include <Main/options.hxx>
#include <Scenery/tileentry.hxx>

#include "matlib.hxx"
#include "obj.hxx"

FG_USING_STD(string);
FG_USING_STD(vector);


typedef vector < int > int_list;
typedef int_list::iterator int_list_iterator;
typedef int_list::const_iterator int_point_list_iterator;


static double normals[FG_MAX_NODES][3];
static double tex_coords[FG_MAX_NODES*3][3];


#if 0
// given three points defining a triangle, calculate the normal
static void calc_normal(Point3D p1, Point3D p2, 
			Point3D p3, sgVec3 normal)
{
    sgVec3 v1, v2;

    v1[0] = p2[0] - p1[0]; v1[1] = p2[1] - p1[1]; v1[2] = p2[2] - p1[2];
    v2[0] = p3[0] - p1[0]; v2[1] = p3[1] - p1[1]; v2[2] = p3[2] - p1[2];

    sgVectorProductVec3( normal, v1, v2 );
    sgNormalizeVec3( normal );

    // fgPrintf( FG_TERRAIN, FG_DEBUG, "  Normal = %.2f %.2f %.2f\n", 
    //           normal[0], normal[1], normal[2]);
}
#endif


#define FG_TEX_CONSTANT 69.0

// Calculate texture coordinates for a given point.
static Point3D local_calc_tex_coords(const Point3D& node, const Point3D& ref) {
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
    FGNewMat *newmat;

    ssgSimpleState *state = NULL;

    ssgBranch *tile = new ssgBranch () ;
    tile -> setName ( (char *)path.c_str() ) ;

    double tex_width = 1000.0;
    // double tex_height;

    // find Ocean material in the properties list
    newmat = material_lib.find( "Ocean" );
    if ( newmat != NULL ) {
	// set the texture width and height values for this
	// material
	tex_width = newmat->get_xsize();
	// tex_height = newmat->get_ysize();
	
	// set ssgState
	state = newmat->get_state();
    } else {
	FG_LOG( FG_TERRAIN, FG_ALERT, 
		"Ack! unknown usemtl name = " << "Ocean" 
		<< " in " << path );
    }

    // Calculate center point
    FGBucket b = t->tile_bucket;
    double clon = b.get_center_lon();
    double clat = b.get_center_lat();
    double height = b.get_height();
    double width = b.get_width();

    Point3D center = fgGeodToCart(Point3D(clon*DEG_TO_RAD,clat*DEG_TO_RAD,0.0));
    t->center = center;
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
    point_list geod_nodes;
    geod_nodes.clear();
    for ( i = 0; i < 4; ++i ) {
	geod_nodes.push_back( geod[i] );
    }
    int_list rectangle;
    rectangle.clear();
    for ( i = 0; i < 4; ++i ) {
	rectangle.push_back( i );
    }
    point_list texs = calc_tex_coords( b, geod_nodes, rectangle, 
				       1000.0 / tex_width );

    // Allocate ssg structure
    ssgVertexArray   *vl = new ssgVertexArray( 4 );
    ssgNormalArray   *nl = new ssgNormalArray( 4 );
    ssgTexCoordArray *tl = new ssgTexCoordArray( 4 );
    ssgColourArray   *cl = new ssgColourArray( 1 );

    sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
    cl->add( color );

    // sgVec3 *vtlist = new sgVec3 [ 4 ];
    // t->vec3_ptrs.push_back( vtlist );
    // sgVec3 *vnlist = new sgVec3 [ 4 ];
    // t->vec3_ptrs.push_back( vnlist );
    // sgVec2 *tclist = new sgVec2 [ 4 ];
    // t->vec2_ptrs.push_back( tclist );

    sgVec2 tmp2;
    sgVec3 tmp3;
    for ( i = 0; i < 4; ++i ) {
	sgSetVec3( tmp3, 
		   rel[i].x(), rel[i].y(), rel[i].z() );
	vl->add( tmp3 );

	sgSetVec3( tmp3, 
		   normals[i].x(), normals[i].y(), normals[i].z() );
	nl->add( tmp3 );

	sgSetVec2( tmp2, texs[i].x(), texs[i].y());
	tl->add( tmp2 );
    }
    
    ssgLeaf *leaf = 
	new ssgVtxTable ( GL_TRIANGLE_FAN, vl, nl, tl, cl );

    leaf->setState( state );

    tile->addKid( leaf );
    // if ( current_options.get_clouds() ) {
    //    fgGenCloudTile(path, t, tile);
    // }

    return tile;
}


// Load a .obj file
ssgBranch *fgObjLoad( const string& path, FGTileEntry *t, const bool is_base) {
    FGNewMat *newmat;
    Point3D pp;
    // sgVec3 approx_normal;
    // double normal[3], scale = 0.0;
    // double x, y, z, xmax, xmin, ymax, ymin, zmax, zmin;
    // GLfloat sgenparams[] = { 1.0, 0.0, 0.0, 0.0 };
    // GLint display_list = 0;
    int shading;
    bool in_faces = false;
    int vncount, vtcount;
    int n1 = 0, n2 = 0, n3 = 0;
    int tex;
    // int last1 = 0, last2 = 0;
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
    sgVec3 *vtlist, *vnlist;
    sgVec2 *tclist;

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

    if ( is_base ) {
	t->ncount = 0;
    }
    vncount = 0;
    vtcount = 0;
    if ( is_base ) {
	t->bounding_radius = 0.0;
    }
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

#if defined( macintosh )
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
		if ( is_base ) {
		    in >> t->center >> t->bounding_radius;
		} else {
		    Point3D junk1;
		    double junk2;
		    in >> junk1 >> junk2;
		}
		center = t->center;
		// cout << "center = " << center 
		//      << " radius = " << t->bounding_radius << endl;
	    } else if ( token == "bs" )	{
		// reference point (center offset)
		// (skip past this)
		Point3D junk1;
		double junk2;
		in >> junk1 >> junk2;
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

		    vtlist = new sgVec3 [ nodes.size() ];
		    t->vec3_ptrs.push_back( vtlist );
		    vnlist = new sgVec3 [ vncount ];
		    t->vec3_ptrs.push_back( vnlist );
		    tclist = new sgVec2 [ vtcount ];
		    t->vec2_ptrs.push_back( tclist );

		    for ( i = 0; i < (int)nodes.size(); ++i ) {
			sgSetVec3( vtlist[i], 
				   nodes[i][0], nodes[i][1], nodes[i][2] );
		    }
		    for ( i = 0; i < vncount; ++i ) {
			sgSetVec3( vnlist[i], 
				   normals[i][0], 
				   normals[i][1],
				   normals[i][2] );
		    }
		    for ( i = 0; i < vtcount; ++i ) {
			sgSetVec2( tclist[i],
				   tex_coords[i][0],
				   tex_coords[i][1] );
		    }
		}

		// display_list = xglGenLists(1);
		// xglNewList(display_list, GL_COMPILE);
		// printf("xglGenLists(); xglNewList();\n");
		in_faces = false;

		// scan the material line
		string material;
		in >> material;
		
		// find this material in the properties list

		newmat = material_lib.find( material );
		if ( newmat == NULL ) {
		    // see if this is an on the fly texture
		    string file = path;
		    int pos = file.rfind( "/" );
		    file = file.substr( 0, pos );
		    cout << "current file = " << file << endl;
		    file += "/";
		    file += material;
		    cout << "current file = " << file << endl;
		    if ( ! material_lib.add_item( file ) ) {
			FG_LOG( FG_TERRAIN, FG_ALERT, 
				"Ack! unknown usemtl name = " << material 
				<< " in " << path );
		    } else {
			// locate our newly created material
			newmat = material_lib.find( material );
			if ( newmat == NULL ) {
			    FG_LOG( FG_TERRAIN, FG_ALERT, 
				    "Ack! bad on the fly materia create = "
				    << material << " in " << path );
			}
		    }
		}

		if ( newmat != NULL ) {
		    // set the texture width and height values for this
		    // material
		    tex_width = newmat->get_xsize();
		    tex_height = newmat->get_ysize();
		    state = newmat->get_state();
		    // cout << "(w) = " << tex_width << " (h) = " 
		    //      << tex_width << endl;
		}
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
		    if ( is_base ) {
			t->ncount++;
		    }
		} else {
		    FG_LOG( FG_TERRAIN, FG_ALERT, 
			    "Read too many nodes in " << path 
			    << " ... dying :-(");
		    exit(-1);
		}
	    } else if ( (token == "tf") || (token == "ts") || (token == "f") ) {
		// triangle fan, strip, or individual face
		// FG_LOG( FG_TERRAIN, FG_INFO, "new fan or strip");

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
			    tclist[tex][0] *= (1000.0 / tex_width);
			}
			if ( tex_height > 0 ) {
			    tclist[tex][1] *= (1000.0 / tex_height);
			}
		    }
		    pp.setx( tex_coords[tex][0] * (1000.0 / tex_width) );
		    pp.sety( tex_coords[tex][1] * (1000.0 / tex_height) );
		} else {
		    in.putback( c );
		    pp = local_calc_tex_coords(nodes[n1], center);
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
			    tclist[tex][0] *= (1000.0 / tex_width);
			}
			if ( tex_height > 0 ) {
			    tclist[tex][1] *= (1000.0 / tex_height);
			}
		    }
		    pp.setx( tex_coords[tex][0] * (1000.0 / tex_width) );
		    pp.sety( tex_coords[tex][1] * (1000.0 / tex_height) );
		} else {
		    in.putback( c );
		    pp = local_calc_tex_coords(nodes[n2], center);
		}
		// xglTexCoord2f(pp.x(), pp.y());
		// xglVertex3dv(nodes[n2].get_n());
		
		// read all subsequent numbers until next thing isn't a number
		while ( true ) {
#if defined( macintosh )
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
				tclist[tex][0] *= (1000.0 / tex_width);
			    }
			    if ( tex_height > 0 ) {
				tclist[tex][1] *= (1000.0 / tex_height);
			    }
			}
			pp.setx( tex_coords[tex][0] * (1000.0 / tex_width) );
			pp.sety( tex_coords[tex][1] * (1000.0 / tex_height) );
		    } else {
			in.putback( c );
			pp = local_calc_tex_coords(nodes[n3], center);
		    }
		    // xglTexCoord2f(pp.x(), pp.y());
		    // xglVertex3dv(nodes[n3].get_n());

		    if ( (token == "tf") || (token == "f") ) {
			// triangle fan
			n2 = n3;
		    } else {
			// triangle strip
			odd = !odd;
			n1 = n2;
			n2 = n3;
		    }
		}

		// xglEnd();

		// build the ssg entity
		int size = (int)fan_vertices.size();
		ssgVertexArray   *vl = new ssgVertexArray( size );
		ssgNormalArray   *nl = new ssgNormalArray( size );
		ssgTexCoordArray *tl = new ssgTexCoordArray( size );
		ssgColourArray   *cl = new ssgColourArray( 1 );

		sgVec4 color;
		sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
		cl->add( color );

		sgVec2 tmp2;
		sgVec3 tmp3;
		for ( i = 0; i < size; ++i ) {
		    sgCopyVec3( tmp3, vtlist[ fan_vertices[i] ] );
		    vl -> add( tmp3 );

		    sgCopyVec3( tmp3, vnlist[ fan_vertices[i] ] );
		    nl -> add( tmp3 );

		    sgCopyVec2( tmp2, tclist[ fan_tex_coords[i] ] );
		    tl -> add( tmp2 );
		}

		ssgLeaf *leaf = NULL;
		if ( token == "tf" ) {
		    // triangle fan
		    leaf = 
			new ssgVtxTable ( GL_TRIANGLE_FAN, vl, nl, tl, cl );
		} else if ( token == "ts" ) {
		    // triangle strip
		    leaf = 
			new ssgVtxTable ( GL_TRIANGLE_STRIP, vl, nl, tl, cl );
		} else if ( token == "f" ) {
		    // triangle
		    leaf = 
			new ssgVtxTable ( GL_TRIANGLES, vl, nl, tl, cl );
		}
		// leaf->makeDList();
		leaf->setState( state );

		tile->addKid( leaf );
	    } else {
		FG_LOG( FG_TERRAIN, FG_WARN, "Unknown token in " 
			<< path << " = " << token );
	    }

	    // eat white space before start of while loop so if we are
	    // done with useful input it is noticed before hand.
#if defined( macintosh )
	    in >> ::skipws;
#else
	    in >> skipws;
#endif
	}
    }

    if ( is_base ) {
	t->nodes = nodes;
    }

    stopwatch.stop();
    FG_LOG( FG_TERRAIN, FG_DEBUG, 
	    "Loaded " << path << " in " 
	    << stopwatch.elapsedSeconds() << " seconds" );

    // Generate a cloud layer above the tiles
    // if ( current_options.get_clouds() ) {
    //  	fgGenCloudTile(path, t, tile);
    // }
    return tile;
}


