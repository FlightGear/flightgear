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

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include <stdio.h>
#include <string.h>

#include <simgear/compiler.h>
#include <simgear/io/sg_binobj.hxx>

#include STL_STRING
#include <map>			// STL
#include <vector>		// STL
#include <ctype.h>		// isdigit()

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/stopwatch.hxx>
#include <simgear/misc/texcoord.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/tileentry.hxx>

#include "matlib.hxx"
#include "obj.hxx"

SG_USING_STD(string);
SG_USING_STD(vector);


typedef vector < int > int_list;
typedef int_list::iterator int_list_iterator;
typedef int_list::const_iterator int_point_list_iterator;


static double normals[FG_MAX_NODES][3];
static double tex_coords[FG_MAX_NODES*3][3];


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

    pp = sgCartToPolar3d(cp);

    // tmplon = pp.lon() * SGD_RADIANS_TO_DEGREES;
    // tmplat = pp.lat() * SGD_RADIANS_TO_DEGREES;
    // cout << tmplon << " " << tmplat << endl;

    pp.setx( fmod(SGD_RADIANS_TO_DEGREES * FG_TEX_CONSTANT * pp.x(), 11.0) );
    pp.sety( fmod(SGD_RADIANS_TO_DEGREES * FG_TEX_CONSTANT * pp.y(), 11.0) );

    if ( pp.x() < 0.0 ) {
	pp.setx( pp.x() + 11.0 );
    }

    if ( pp.y() < 0.0 ) {
	pp.sety( pp.y() + 11.0 );
    }

    // cout << pp << endl;

    return(pp);
}


// Generate an ocean tile
bool fgGenTile( const string& path, SGBucket b,
		      Point3D *center,
		      double *bounding_radius,
		      ssgBranch* geometry )
{
    FGNewMat *newmat;

    ssgSimpleState *state = NULL;

    geometry -> setName ( (char *)path.c_str() ) ;

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
	SG_LOG( SG_TERRAIN, SG_ALERT, 
		"Ack! unknown usemtl name = " << "Ocean" 
		<< " in " << path );
    }

    // Calculate center point
    double clon = b.get_center_lon();
    double clat = b.get_center_lat();
    double height = b.get_height();
    double width = b.get_width();

    *center = sgGeodToCart( Point3D(clon*SGD_DEGREES_TO_RADIANS,
				    clat*SGD_DEGREES_TO_RADIANS,
				    0.0) );
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
        rad[i] = Point3D( geod[i].x() * SGD_DEGREES_TO_RADIANS,
                          geod[i].y() * SGD_DEGREES_TO_RADIANS,
                          geod[i].z() );
    }

    Point3D cart[4], rel[4];
    for ( i = 0; i < 4; ++i ) {
	cart[i] = sgGeodToCart(rad[i]);
	rel[i] = cart[i] - *center;
	// cout << "corner " << i << " = " << cart[i] << endl;
    }

    // Calculate bounding radius
    *bounding_radius = center->distance3D( cart[0] );
    // cout << "bounding radius = " << t->bounding_radius << endl;

    // Calculate normals
    Point3D normals[4];
    for ( i = 0; i < 4; ++i ) {
	double length = cart[i].distance3D( Point3D(0.0) );
	normals[i] = cart[i] / length;
	// cout << "normal = " << normals[i] << endl;
    }

    // Calculate texture coordinates
    point_list geod_nodes;
    geod_nodes.clear();
    int_list rectangle;
    rectangle.clear();
    for ( i = 0; i < 4; ++i ) {
	geod_nodes.push_back( geod[i] );
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

    geometry->addKid( leaf );

    return true;
}


static void random_pt_inside_tri( float *res,
				  float *n1, float *n2, float *n3 )
{
    sgVec3 p1, p2, p3;

    double a = sg_random();
    double b = sg_random();
    if ( a + b > 1.0 ) {
	a = 1.0 - a;
	b = 1.0 - b;
    }
    double c = 1 - a - b;

    sgScaleVec3( p1, n1, a );
    sgScaleVec3( p2, n2, b );
    sgScaleVec3( p3, n3, c );

    sgAddVec3( res, p1, p2 );
    sgAddVec3( res, p3 );
}


static void gen_random_surface_points( ssgLeaf *leaf, ssgVertexArray *lights,
				       double factor ) {
    int num = leaf->getNumTriangles();
    if ( num > 0 ) {
	short int n1, n2, n3;
	float *p1, *p2, *p3;
	sgVec3 result;

	// generate a repeatable random seed
	p1 = leaf->getVertex( 0 );
	unsigned int seed = (unsigned int)p1[0];
	sg_srandom( seed );

	for ( int i = 0; i < num; ++i ) {
	    leaf->getTriangle( i, &n1, &n2, &n3 );
	    p1 = leaf->getVertex(n1);
	    p2 = leaf->getVertex(n2);
	    p3 = leaf->getVertex(n3);
	    double area = sgTriArea( p1, p2, p3 );
	    double num = area / factor;

	    // generate a light point for each unit of area
	    while ( num > 1.0 ) {
		random_pt_inside_tri( result, p1, p2, p3 );
		lights->add( result );
		num -= 1.0;
	    }
	    // for partial units of area, use a zombie door method to
	    // create the proper random chance of a light being created
	    // for this triangle
	    if ( num > 0.0 ) {
		if ( sg_random() <= num ) {
		    // a zombie made it through our door
		    random_pt_inside_tri( result, p1, p2, p3 );
		    lights->add( result );
		}
	    }
	}
    }
}


// Load an Ascii obj file
ssgBranch *fgAsciiObjLoad( const string& path, FGTileEntry *t,
			   ssgVertexArray *lights, const bool is_base)
{
    FGNewMat *newmat = NULL;
    string material;
    float coverage = -1;
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
    sg_gzifstream in( path );
    if ( ! in.is_open() ) {
	SG_LOG( SG_TERRAIN, SG_DEBUG, "Cannot open file: " << path );
	SG_LOG( SG_TERRAIN, SG_DEBUG, "default to ocean tile: " << path );

        delete tile;

	return NULL;
    }

    shading = fgGetBool("/sim/rendering/shading");

    if ( is_base ) {
	t->ncount = 0;
    }
    vncount = 0;
    vtcount = 0;
    if ( is_base ) {
	t->bounding_radius = 0.0;
    }
    center = t->center;

    // StopWatch stopwatch;
    // stopwatch.start();

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

	in >> ::skipws;

	if ( in.get( c ) && c == '#' ) {
	    // process a comment line

	    // getline( in, line );
	    // cout << "comment = " << line << endl;

	    in >> token;

	    if ( token == "Version" ) {
		// read scenery versions number
		in >> scenery_version;
		// cout << "scenery_version = " << scenery_version << endl;
		if ( scenery_version > 0.4 ) {
		    SG_LOG( SG_TERRAIN, SG_ALERT, 
			    "\nYou are attempting to load a tile format that\n"
			    << "is newer than this version of flightgear can\n"
			    << "handle.  You should upgrade your copy of\n"
			    << "FlightGear to the newest version.  For\n"
			    << "details, please see:\n"
			    << "\n    http://www.flightgear.org\n" );
		    exit(-1);
		}
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
			SG_LOG( SG_TERRAIN, SG_ALERT, 
				"Tile has mismatched nodes = " << nodes.size()
				<< " and normals = " << vncount << " : " 
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
			SG_LOG( SG_TERRAIN, SG_ALERT, 
				"Ack! unknown usemtl name = " << material 
				<< " in " << path );
		    } else {
			// locate our newly created material
			newmat = material_lib.find( material );
			if ( newmat == NULL ) {
			    SG_LOG( SG_TERRAIN, SG_ALERT, 
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
		    coverage = newmat->get_light_coverage();
		    // cout << "(w) = " << tex_width << " (h) = " 
		    //      << tex_width << endl;
		} else {
		    coverage = -1;
		}
	    } else {
		// unknown comment, just gobble the input until the
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
		    SG_LOG( SG_TERRAIN, SG_ALERT, 
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
		    SG_LOG( SG_TERRAIN, SG_ALERT, 
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
		    SG_LOG( SG_TERRAIN, SG_ALERT, 
			    "Read too many nodes in " << path 
			    << " ... dying :-(");
		    exit(-1);
		}
	    } else if ( (token == "tf") || (token == "ts") || (token == "f") ) {
		// triangle fan, strip, or individual face
		// SG_LOG( SG_TERRAIN, SG_INFO, "new fan or strip");

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
		    in >> ::skipws;

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

		if ( is_base ) {
		    if ( coverage > 0.0 ) {
			if ( coverage < 10000.0 ) {
			    SG_LOG(SG_INPUT, SG_ALERT, "Light coverage is "
				   << coverage << ", pushing up to 10000");
			    coverage = 10000;
			}
			gen_random_surface_points(leaf, lights, coverage);
		    }
		}
	    } else {
		SG_LOG( SG_TERRAIN, SG_WARN, "Unknown token in " 
			<< path << " = " << token );
	    }

	    // eat white space before start of while loop so if we are
	    // done with useful input it is noticed before hand.
	    in >> ::skipws;
	}
    }

    if ( is_base ) {
	t->nodes = nodes;
    }

    // stopwatch.stop();
    // SG_LOG( SG_TERRAIN, SG_DEBUG, 
    //     "Loaded " << path << " in " 
    //     << stopwatch.elapsedSeconds() << " seconds" );

    return tile;
}


ssgLeaf *gen_leaf( const string& path,
		   const GLenum ty, const string& material,
		   const point_list& nodes, const point_list& normals,
		   const point_list& texcoords,
		   const int_list node_index,
		   const int_list& tex_index,
		   const bool calc_lights, ssgVertexArray *lights )
{
    double tex_width = 1000.0, tex_height = 1000.0;
    ssgSimpleState *state = NULL;
    float coverage = -1;

    FGNewMat *newmat = material_lib.find( material );
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
	    SG_LOG( SG_TERRAIN, SG_ALERT, 
		    "Ack! unknown usemtl name = " << material 
		    << " in " << path );
	} else {
	    // locate our newly created material
	    newmat = material_lib.find( material );
	    if ( newmat == NULL ) {
		SG_LOG( SG_TERRAIN, SG_ALERT, 
			"Ack! bad on the fly material create = "
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
	coverage = newmat->get_light_coverage();
	// cout << "(w) = " << tex_width << " (h) = " 
	//      << tex_width << endl;
    } else {
	coverage = -1;
    }

    // cout << "before list allocs" << endl;

    // cout << "before vl, size = " << size << endl;
    // cout << "before nl" << endl;
    // cout << "before tl" << endl;
    // cout << "before cl" << endl;

    sgVec2 tmp2;
    sgVec3 tmp3;
    sgVec4 tmp4;
    int i;

    // vertices
    int size = node_index.size();
    if ( size < 1 ) {
	SG_LOG( SG_TERRAIN, SG_ALERT, "Woh! node list size < 1" );
	exit(-1);
    }
    ssgVertexArray *vl = new ssgVertexArray( size );
    Point3D node;
    for ( i = 0; i < size; ++i ) {
	node = nodes[ node_index[i] ];
	sgSetVec3( tmp3, node[0], node[1], node[2] );
	vl -> add( tmp3 );
    }

    // colors
    ssgColourArray *cl = new ssgColourArray( 1 );
    sgSetVec4( tmp4, 1.0, 1.0, 1.0, 1.0 );
    cl->add( tmp4 );

    // normals
    Point3D normal;
    ssgNormalArray *nl = new ssgNormalArray( size );
    if ( normals.size() == 1 ) {
        normal = normals[ 0 ];
        sgSetVec3( tmp3, normal[0], normal[1], normal[2] );
        nl -> add( tmp3 );
    } else if ( normals.size() > 1 ) {
        for ( i = 0; i < size; ++i ) {
            normal = normals[ node_index[i] ];
            sgSetVec3( tmp3, normal[0], normal[1], normal[2] );
            nl -> add( tmp3 );
        }
    }

    // texture coordinates
    size = tex_index.size();
    Point3D texcoord;
    ssgTexCoordArray *tl = new ssgTexCoordArray( size );
    if ( size == 1 ) {
        texcoord = texcoords[ tex_index[0] ];
	sgSetVec2( tmp2, texcoord[0], texcoord[1] );
	sgSetVec2( tmp2, texcoord[0], texcoord[1] );
	if ( tex_width > 0 ) {
	    tmp2[0] *= (1000.0 / tex_width);
	}
	if ( tex_height > 0 ) {
	    tmp2[1] *= (1000.0 / tex_height);
	}
	tl -> add( tmp2 );
    } else if ( size > 1 ) {
        for ( i = 0; i < size; ++i ) {
            texcoord = texcoords[ tex_index[i] ];
            sgSetVec2( tmp2, texcoord[0], texcoord[1] );
            if ( tex_width > 0 ) {
                tmp2[0] *= (1000.0 / tex_width);
            }
            if ( tex_height > 0 ) {
                tmp2[1] *= (1000.0 / tex_height);
            }
            tl -> add( tmp2 );
        }
    }

    // cout << "before leaf create" << endl;
    ssgLeaf *leaf = new ssgVtxTable ( ty, vl, nl, tl, cl );
    // cout << "after leaf create" << endl;

    // lookup the state record
    // cout << "looking up material = " << endl;
    // cout << material << endl;
    // cout << "'" << endl;

    leaf->setState( state );

    if ( calc_lights ) {
	if ( coverage > 0.0 ) {
	    if ( coverage < 10000.0 ) {
		SG_LOG(SG_INPUT, SG_ALERT, "Light coverage is "
		       << coverage << ", pushing up to 10000");
		coverage = 10000;
	    }
	    gen_random_surface_points(leaf, lights, coverage);
	}
    }

    return leaf;
}


// Load an Binary obj file
bool fgBinObjLoad( const string& path, const bool is_base,
		   Point3D *center,
		   double *bounding_radius,
		   ssgBranch* geometry,
		   ssgBranch* rwy_lights,
		   ssgVertexArray *ground_lights )
{
    SGBinObject obj;
    bool result = obj.read_bin( path );

    if ( !result ) {
	return false;
    }

    // cout << "fans size = " << obj.get_fans_v().size()
    //      << " fan_mats size = " << obj.get_fan_materials().size() << endl;

    geometry->setName( (char *)path.c_str() );
   
    if ( is_base ) {
	// reference point (center offset/bounding sphere)
	*center = obj.get_gbs_center();
	*bounding_radius = obj.get_gbs_radius();
    }

    point_list nodes = obj.get_wgs84_nodes();
    point_list colors = obj.get_colors();
    point_list normals = obj.get_normals();
    point_list texcoords = obj.get_texcoords();

    string material;
    int_list vertex_index;
    int_list tex_index;

    int i;

    // generate points
    string_list pt_materials = obj.get_pt_materials();
    group_list pts_v = obj.get_pts_v();
    for ( i = 0; i < (int)pts_v.size(); ++i ) {
        cout << "pts_v.size() = " << pts_v.size() << endl;
	material = pt_materials[i];
	vertex_index = pts_v[i];
	tex_index.clear();
	ssgLeaf *leaf = gen_leaf( path, GL_POINTS, material,
				  nodes, normals, texcoords,
				  vertex_index, tex_index,
				  false, ground_lights );

	geometry->addKid( leaf );
    }

    // generate triangles
    string_list tri_materials = obj.get_tri_materials();
    group_list tris_v = obj.get_tris_v();
    group_list tris_tc = obj.get_tris_tc();
    for ( i = 0; i < (int)tris_v.size(); ++i ) {
	material = tri_materials[i];
	vertex_index = tris_v[i];
	tex_index = tris_tc[i];
	ssgLeaf *leaf = gen_leaf( path, GL_TRIANGLES, material,
				  nodes, normals, texcoords,
				  vertex_index, tex_index,
				  is_base, ground_lights );

	geometry->addKid( leaf );
    }

    // generate strips
    string_list strip_materials = obj.get_strip_materials();
    group_list strips_v = obj.get_strips_v();
    group_list strips_tc = obj.get_strips_tc();
    for ( i = 0; i < (int)strips_v.size(); ++i ) {
	material = strip_materials[i];
	vertex_index = strips_v[i];
	tex_index = strips_tc[i];
	ssgLeaf *leaf = gen_leaf( path, GL_TRIANGLE_STRIP, material,
				  nodes, normals, texcoords,
				  vertex_index, tex_index,
				  is_base, ground_lights );

	geometry->addKid( leaf );
    }

    // generate fans
    string_list fan_materials = obj.get_fan_materials();
    group_list fans_v = obj.get_fans_v();
    group_list fans_tc = obj.get_fans_tc();
    for ( i = 0; i < (int)fans_v.size(); ++i ) {
	material = fan_materials[i];
	vertex_index = fans_v[i];
	tex_index = fans_tc[i];
	ssgLeaf *leaf = gen_leaf( path, GL_TRIANGLE_FAN, material,
				  nodes, normals, texcoords,
				  vertex_index, tex_index,
				  is_base, ground_lights );

	geometry->addKid( leaf );
    }

    return true;
}
