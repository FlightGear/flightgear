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
#include <simgear/sg_inlines.h>
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
#include <Time/light.hxx>
#include <Scenery/tileentry.hxx>

#include "newmat.hxx"
#include "matlib.hxx"
#include "obj.hxx"

SG_USING_STD(string);
SG_USING_STD(vector);


typedef vector < int > int_list;
typedef int_list::iterator int_list_iterator;
typedef int_list::const_iterator int_point_list_iterator;


static double normals[FG_MAX_NODES][3];
static double tex_coords[FG_MAX_NODES*3][3];

static int
runway_lights_predraw (ssgEntity * e)
{
				// Turn on lights only at night
    float sun_angle = cur_light_params.sun_angle * SGD_RADIANS_TO_DEGREES;
    return int(sun_angle > 90.0);
}


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


/**
 * Create a rotation matrix to align an object for the current lat/lon.
 *
 * By default, objects are aligned for the north pole.  This code
 * calculates a matrix to rotate them for the surface of the earth in
 * the current location.
 *
 * TODO: there should be a single version of this method somewhere
 * for all of SimGear.
 *
 * @param ROT The resulting rotation matrix.
 * @param hdg_deg The object heading in degrees.
 * @param lon_deg The longitude in degrees.
 * @param lat_deg The latitude in degrees.
 */
static void
makeWorldUpRotationMatrix (sgMat4 ROT, double hdg_deg,
			   double lon_deg, double lat_deg)
{
	SGfloat sin_lat = sin( lat_deg * SGD_DEGREES_TO_RADIANS );
	SGfloat cos_lat = cos( lat_deg * SGD_DEGREES_TO_RADIANS );
	SGfloat sin_lon = sin( lon_deg * SGD_DEGREES_TO_RADIANS );
	SGfloat cos_lon = cos( lon_deg * SGD_DEGREES_TO_RADIANS );
	SGfloat sin_hdg = sin( hdg_deg * SGD_DEGREES_TO_RADIANS ) ;
	SGfloat cos_hdg = cos( hdg_deg * SGD_DEGREES_TO_RADIANS ) ;

	ROT[0][0] =  cos_hdg * sin_lat * cos_lon - sin_hdg * sin_lon;
	ROT[0][1] =  cos_hdg * sin_lat * sin_lon + sin_hdg * cos_lon;
	ROT[0][2] = -cos_hdg * cos_lat;
	ROT[0][3] =  SG_ZERO;

	ROT[1][0] = -sin_hdg * sin_lat * cos_lon - cos_hdg * sin_lon;
	ROT[1][1] = -sin_hdg * sin_lat * sin_lon + cos_hdg * cos_lon;
	ROT[1][2] =  sin_hdg * cos_lat;
	ROT[1][3] =  SG_ZERO;

	ROT[2][0] = cos_lat * cos_lon;
	ROT[2][1] = cos_lat * sin_lon;
	ROT[2][2] = sin_lat;
	ROT[2][3] = SG_ZERO;

	ROT[3][0] = SG_ZERO;
	ROT[3][1] = SG_ZERO;
	ROT[3][2] = SG_ZERO;
	ROT[3][3] = SG_ONE ;
}


/**
 * Add an object to a random location inside a triangle.
 *
 * @param p1 The first vertex of the triangle.
 * @param p2 The second vertex of the triangle.
 * @param p3 The third vertex of the triangle.
 * @param center The center of the triangle.
 * @param lon_deg The longitude of the surface center, in degrees.
 * @param lat_deg The latitude of the surface center, in degrees.
 * @param object The randomly-placed object.
 * @param branch The branch where the object should be added to the
 *        scene graph.
 */
static void
add_object_to_triangle (sgVec3 p1, sgVec3 p2, sgVec3 p3, sgVec3 center,
			double lon_deg, double lat_deg,
			FGNewMat::Object * object, ssgBranch * branch)
{
				// Set up the random heading if required.
    double hdg_deg = 0;
    if (object->get_heading_type() == FGNewMat::Object::HEADING_RANDOM)
      hdg_deg = sg_random() * 360;

    sgVec3 result;

    sgMat4 ROT;
    makeWorldUpRotationMatrix(ROT, hdg_deg, lon_deg, lat_deg);

    random_pt_inside_tri(result, p1, p2, p3);
    sgSubVec3(result, center);
    sgMat4 OBJ_pos, OBJ;
    sgMakeTransMat4(OBJ_pos, result);
    sgCopyMat4(OBJ, ROT);
    sgPostMultMat4(OBJ, OBJ_pos);
    ssgTransform * pos = new ssgTransform;
    pos->setTransform(OBJ);
    pos->addKid(object->get_random_model());
    branch->addKid(pos);
}


/**
 * User data for populating triangles when they come in range.
 */
class TriUserData : public ssgBase
{
public:
  bool is_filled_in;
  float * p1;
  float * p2;
  float * p3;
  FGNewMat::ObjectGroup * object_group;
  ssgBranch * branch;
  double lon_deg;
  double lat_deg;
  unsigned int seed;
};


/**
 * Fill in a triangle with randomly-placed objects.
 *
 * This method is invoked by a callback when the triangle is in range
 * but not yet populated.
 *
 * @param p1 The first vertex of the triangle.
 * @param p2 The second vertex of the triangle.
 * @param p3 The third vertex of the triangle.
 * @param mat The triangle's material.
 * @param object_index The index of the random object in the triangle.
 * @param branch The branch where the objects should be added.
 * @param lon_deg The longitude of the surface center, in degrees.
 * @param lat_deg The latitude of the surface center, in degrees.
 */
static void
fill_in_triangle (float * p1, float * p2, float * p3,
		  FGNewMat::ObjectGroup * object_group, ssgBranch * branch,
		  double lon_deg, double lat_deg, unsigned int seed)
{
				// generate a repeatable random seed
    sg_srandom(seed);

    int nObjects = object_group->get_object_count();
    for (int i = 0; i < nObjects; i++) {
      FGNewMat::Object * object = object_group->get_object(i);
      sgVec3 center;
      sgSetVec3(center,
		(p1[0] + p2[0] + p3[0]) / 3.0,
		(p1[1] + p2[1] + p3[1]) / 3.0,
		(p1[2] + p2[2] + p3[2]) / 3.0);
      double area = sgTriArea(p1, p2, p3);
      double num = area / object->get_coverage_m2();

      // place an object each unit of area
      while ( num > 1.0 ) {
	add_object_to_triangle(p1, p2, p3, center, lon_deg, lat_deg,
			       object, branch);
	num -= 1.0;
      }
      // for partial units of area, use a zombie door method to
      // create the proper random chance of an object being created
      // for this triangle
      if ( num > 0.0 ) {
	if ( sg_random() <= num ) {
	  // a zombie made it through our door
	  add_object_to_triangle(p1, p2, p3, center, lon_deg, lat_deg,
				 object, branch);
	}
      }
    }
}

/**
 * SSG callback for an in-range triangle of randomly-placed objects.
 *
 * This pretraversal callback is attached to a branch that is traversed
 * only when a triangle is in range.  If the triangle is not currently
 * populated with randomly-placed objects, this callback will populate
 * it.
 *
 * @param entity The entity to which the callback is attached (not used).
 * @param mask The entity's traversal mask (not used).
 * @return Always 1, to allow traversal and culling to continue.
 */
static int
tri_in_range_callback (ssgEntity * entity, int mask)
{
  TriUserData * data = (TriUserData *)entity->getUserData();
  if (!data->is_filled_in) {
    fill_in_triangle(data->p1, data->p2, data->p3, data->object_group,
		     data->branch, data->lon_deg, data->lat_deg,
		     data->seed);
    data->is_filled_in = true;
  }
  return 1;
}


/**
 * SSG callback for an out-of-range triangle of randomly-placed objects.
 *
 * This pretraversal callback is attached to a branch that is traversed
 * only when a triangle is out of range.  If the triangle is currently
 * populated with randomly-placed objects, the objects will be removed.
 *
 *
 * @param entity The entity to which the callback is attached (not used).
 * @param mask The entity's traversal mask (not used).
 * @return Always 0, to prevent any further traversal or culling.
 */
static int
tri_out_of_range_callback (ssgEntity * entity, int mask)
{
  TriUserData * data = (TriUserData *)entity->getUserData();
  if (data->is_filled_in) {
    data->branch->removeAllKids();
    data->is_filled_in = false;
  }
  return 0;
}


/**
 * ssgEntity with a dummy bounding sphere, to fool culling.
 *
 * This forces the in-range and out-of-range branches to be visited
 * when appropriate, even if they have no children.  It's ugly, but
 * it works and seems fairly efficient (since branches can still
 * be culled when they're out of the view frustum).
 */
class DummyBSphereEntity : public ssgEntity
{
public:
  DummyBSphereEntity (float radius)
  {
    bsphere.setCenter(0, 0, 0);
    bsphere.setRadius(radius);
  }
  virtual ~DummyBSphereEntity () {}
  virtual void recalcBSphere () { bsphere_is_invalid = false; }
  virtual void cull (sgFrustum *f, sgMat4 m, int test_needed) {}
  virtual void isect (sgSphere *s, sgMat4 m, int test_needed) {}
  virtual void hot (sgVec3 s, sgMat4 m, int test_needed) {}
  virtual void los (sgVec3 s, sgMat4 m, int test_needed) {}
};


/**
 * Calculate the bounding radius of a triangle from its center.
 *
 * @param center The triangle center.
 * @param p1 The first point in the triangle.
 * @param p2 The second point in the triangle.
 * @param p3 The third point in the triangle.
 * @return The greatest distance any point lies from the center.
 */
static inline float
get_bounding_radius( sgVec3 center, float *p1, float *p2, float *p3)
{
   return sqrt( SG_MAX3( sgDistanceSquaredVec3(center, p1),
			 sgDistanceSquaredVec3(center, p2),
			 sgDistanceSquaredVec3(center, p3) ) );
}


/**
 * Set up a triangle for randomly-placed objects.
 *
 * No objects will be added unless the triangle comes into range.
 *
 * @param leaf The leaf containing the data for the terrain surface.
 * @param tri_index The index of the triangle in the leaf.
 * @param mat The material data for the triangle.
 * @param branch The branch to which the randomly-placed objects
 *        should be added.
 * @param lon_deg The longitude of the surface center, in degrees.
 * @param lat_deg The latitude of the surface center, in degrees.
 */
static void
setup_triangle (float * p1, float * p2, float * p3,
		FGNewMat * mat, ssgBranch * branch,
		double lon_deg, double lat_deg)
{
				// Set up a single center point for LOD
    sgVec3 center;
    sgSetVec3(center,
	      (p1[0] + p2[0] + p3[0]) / 3.0,
	      (p1[1] + p2[1] + p3[1]) / 3.0,
	      (p1[2] + p2[2] + p3[2]) / 3.0);
      
				// maximum radius of an object from center.
    double bounding_radius = get_bounding_radius(center, p1, p2, p3);

				// Set up a transformation to the center
				// point, so that everything else can
				// be specified relative to it.
    ssgTransform * location = new ssgTransform;
    sgMat4 TRANS;
    sgMakeTransMat4(TRANS, center);
    location->setTransform(TRANS);
    branch->addKid(location);

				// Iterate through all the object types.
    int num_groups = mat->get_object_group_count();
    for (int i = 0; i < num_groups; i++) {
				// Look up the random object.
        FGNewMat::ObjectGroup * group = mat->get_object_group(i);

				// Set up the range selector for the entire
				// triangle; note that we use the object
				// range plus the bounding radius here, to
				// allow for objects far from the center.
	float ranges[] = {0,
			  group->get_range_m() + bounding_radius,
                          SG_MAX};
	ssgRangeSelector * lod = new ssgRangeSelector;
	lod->setRanges(ranges, 3);
	location->addKid(lod);

				// Create the in-range and out-of-range
				// branches.
	ssgBranch * in_range = new ssgBranch;
	ssgBranch * out_of_range = new ssgBranch;

				// Set up the user data for if/when
				// the random objects in this triangle
				// are filled in.
	TriUserData * data = new TriUserData;
	data->is_filled_in = false;
	data->p1 = p1;
	data->p2 = p2;
	data->p3 = p3;
	data->object_group = group;
	data->branch = in_range;
	data->lon_deg = lon_deg;
	data->lat_deg = lat_deg;
	data->seed = (unsigned int)((p1[0] + lon_deg + lat_deg) * i * 128);

				// Set up the in-range node.
	in_range->setUserData(data);
	in_range->setTravCallback(SSG_CALLBACK_PRETRAV,
				 tri_in_range_callback);
	lod->addKid(in_range);

				// Set up the out-of-range node.
	out_of_range->setUserData(data);
	out_of_range->setTravCallback(SSG_CALLBACK_PRETRAV,
				      tri_out_of_range_callback);
	out_of_range->addKid(new DummyBSphereEntity(bounding_radius));
	lod->addKid(out_of_range);
    }
}


/**
 * User data for populating leaves when they come in range.
 */
class LeafUserData : public ssgBase
{
public:
  bool is_filled_in;
  ssgLeaf * leaf;
  FGNewMat * mat;
  ssgBranch * branch;
  double lon_deg;
  double lat_deg;
};


/**
 * SSG callback for an in-range leaf of randomly-placed objects.
 *
 * This pretraversal callback is attached to a branch that is
 * traversed only when a leaf is in range.  If the leaf is not
 * currently prepared to be populated with randomly-placed objects,
 * this callback will prepare it (actual population is handled by
 * the tri_in_range_callback for individual triangles).
 *
 * @param entity The entity to which the callback is attached (not used).
 * @param mask The entity's traversal mask (not used).
 * @return Always 1, to allow traversal and culling to continue.
 */
static int
leaf_in_range_callback (ssgEntity * entity, int mask)
{
  LeafUserData * data = (LeafUserData *)entity->getUserData();

  if (!data->is_filled_in) {
				// Iterate through all the triangles
				// and populate them.
    int num_tris = data->leaf->getNumTriangles();
    for ( int i = 0; i < num_tris; ++i ) {
      short n1, n2, n3;
      data->leaf->getTriangle(i, &n1, &n2, &n3);
      setup_triangle(data->leaf->getVertex(n1),
		     data->leaf->getVertex(n2),
		     data->leaf->getVertex(n3),
		     data->mat, data->branch, data->lon_deg, data->lat_deg);
    }
    data->is_filled_in = true;
  }
  return 1;
}


/**
 * SSG callback for an out-of-range leaf of randomly-placed objects.
 *
 * This pretraversal callback is attached to a branch that is
 * traversed only when a leaf is out of range.  If the leaf is
 * currently prepared to be populated with randomly-placed objects (or
 * is actually populated), the objects will be removed.
 *
 * @param entity The entity to which the callback is attached (not used).
 * @param mask The entity's traversal mask (not used).
 * @return Always 0, to prevent any further traversal or culling.
 */
static int
leaf_out_of_range_callback (ssgEntity * entity, int mask)
{
  LeafUserData * data = (LeafUserData *)entity->getUserData();
  if (data->is_filled_in) {
    data->branch->removeAllKids();
    data->is_filled_in = false;
  }
  return 0;
}


/**
 * Randomly place objects on a surface.
 *
 * The leaf node provides the geometry of the surface, while the
 * material provides the objects and placement density.  Latitude
 * and longitude are required so that the objects can be rotated
 * to the world-up vector.  This function does not actually add
 * any objects; instead, it attaches an ssgRangeSelector to the
 * branch with callbacks to generate the objects when needed.
 *
 * @param leaf The surface where the objects should be placed.
 * @param branch The branch that will hold the randomly-placed objects.
 * @param center The center of the leaf in FlightGear coordinates.
 * @param lon_deg The longitude of the surface center, in degrees.
 * @param lat_deg The latitude of the surface center, in degrees.
 * @param material_name The name of the surface's material.
 */
static void
gen_random_surface_objects (ssgLeaf *leaf,
			    ssgBranch *branch,
			    Point3D * center,
			    const string &material_name)
{
				// If the surface has no triangles, return
				// now.
    int num_tris = leaf->getNumTriangles();
    if (num_tris < 1)
      return;

				// Get the material for this surface.
    FGNewMat * mat = material_lib.find(material_name);
    if (mat == 0) {
      SG_LOG(SG_INPUT, SG_ALERT, "Unknown material " << material_name);
      return;
    }

				// If the material has no randomly-placed
				// objects, return now.
    if (mat->get_object_group_count() < 1)
      return;

				// Calculate the geodetic centre of
				// the tile, for aligning automatic
				// objects.
    double lon_deg, lat_rad, lat_deg, alt_m, sl_radius_m;
    Point3D geoc = sgCartToPolar3d(*center);
    lon_deg = geoc.lon() * SGD_RADIANS_TO_DEGREES;
    sgGeocToGeod(geoc.lat(), geoc.radius(),
		 &lat_rad, &alt_m, &sl_radius_m);
    lat_deg = lat_rad * SGD_RADIANS_TO_DEGREES;

				// LOD for the leaf
				// max random object range: 20000m
    float ranges[] = {0, 20000, 1000000};
    ssgRangeSelector * lod = new ssgRangeSelector;
    lod->setRanges(ranges, 3);
    branch->addKid(lod);

				// Create the in-range and out-of-range
				// branches.
    ssgBranch * in_range = new ssgBranch;
    ssgBranch * out_of_range = new ssgBranch;
    lod->addKid(in_range);
    lod->addKid(out_of_range);

    LeafUserData * data = new LeafUserData;
    data->is_filled_in = false;
    data->leaf = leaf;
    data->mat = mat;
    data->branch = in_range;
    data->lon_deg = lon_deg;
    data->lat_deg = lat_deg;

    in_range->setUserData(data);
    in_range->setTravCallback(SSG_CALLBACK_PRETRAV, leaf_in_range_callback);
    out_of_range->setUserData(data);
    out_of_range->setTravCallback(SSG_CALLBACK_PRETRAV,
				   leaf_out_of_range_callback);
    out_of_range
      ->addKid(new DummyBSphereEntity(leaf->getBSphere()->getRadius()));
}



////////////////////////////////////////////////////////////////////////
// Scenery loaders.
////////////////////////////////////////////////////////////////////////


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
		    // cout << "current file = " << file << endl;
		    file += "/";
		    file += material;
		    // cout << "current file = " << file << endl;
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
		   const int_list normal_index,
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
	// cout << "current file = " << file << endl;
	file += "/";
	file += material;
	// cout << "current file = " << file << endl;
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

    // normals
    Point3D normal;
    ssgNormalArray *nl = new ssgNormalArray( size );
    if ( normal_index.size() ) {
	// object file specifies normal indices (i.e. normal indices
	// aren't 'implied'
        for ( i = 0; i < size; ++i ) {
            normal = normals[ normal_index[i] ];
            sgSetVec3( tmp3, normal[0], normal[1], normal[2] );
            nl -> add( tmp3 );
        }
    } else {
	// use implied normal indices.  normal index = vertex index.
	for ( i = 0; i < size; ++i ) {
	    normal = normals[ node_index[i] ];
	    sgSetVec3( tmp3, normal[0], normal[1], normal[2] );
	    nl -> add( tmp3 );
	}
    }

    // colors
    ssgColourArray *cl = new ssgColourArray( 1 );
    sgSetVec4( tmp4, 1.0, 1.0, 1.0, 1.0 );
    cl->add( tmp4 );

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

    ssgLeaf *leaf = new ssgVtxTable ( ty, vl, nl, tl, cl );

    // lookup the state record

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
    bool use_random_objects =
      fgGetBool("/sim/rendering/random-objects", true);

    if ( ! obj.read_bin( path ) ) {
	return false;
    }

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

    string material, tmp_mat;
    int_list vertex_index;
    int_list normal_index;
    int_list tex_index;

    int i;
    bool is_lighting = false;

    // generate points
    string_list pt_materials = obj.get_pt_materials();
    group_list pts_v = obj.get_pts_v();
    group_list pts_n = obj.get_pts_n();
    for ( i = 0; i < (int)pts_v.size(); ++i ) {
	// cout << "pts_v.size() = " << pts_v.size() << endl;
	tmp_mat = pt_materials[i];
	if ( tmp_mat.substr(0, 3) == "RWY" ) {
	    material = "LIGHTS";
	    is_lighting = true;
	} else {
	    material = tmp_mat;
	}
	vertex_index = pts_v[i];
	normal_index = pts_n[i];
	tex_index.clear();
	ssgLeaf *leaf = gen_leaf( path, GL_POINTS, material,
				  nodes, normals, texcoords,
				  vertex_index, normal_index, tex_index,
				  false, ground_lights );

	if ( is_lighting ) {
	    float ranges[] = { 0, 12000 };
	    leaf->setCallback(SSG_CALLBACK_PREDRAW, runway_lights_predraw);
	    ssgRangeSelector * lod = new ssgRangeSelector;
	    lod->setRanges(ranges, 2);
	    lod->addKid(leaf);
	    rwy_lights->addKid(lod);
	} else {
	    geometry->addKid( leaf );
	}
    }

    // Put all randomly-placed objects under a separate branch
    // (actually an ssgRangeSelector) named "random-models".
    ssgBranch * random_object_branch = 0;
    if (use_random_objects) {
      float ranges[] = {0, 20000}; // Maximum 20km range for random objects
      ssgRangeSelector * object_lod = new ssgRangeSelector;
      object_lod->setRanges(ranges, 2);
      object_lod->setName("random-models");
      geometry->addKid(object_lod);
      random_object_branch = new ssgBranch;
      object_lod->addKid(random_object_branch);
    }

    // generate triangles
    string_list tri_materials = obj.get_tri_materials();
    group_list tris_v = obj.get_tris_v();
    group_list tris_n = obj.get_tris_n();
    group_list tris_tc = obj.get_tris_tc();
    for ( i = 0; i < (int)tris_v.size(); ++i ) {
	material = tri_materials[i];
	vertex_index = tris_v[i];
	normal_index = tris_n[i];
	tex_index = tris_tc[i];
	ssgLeaf *leaf = gen_leaf( path, GL_TRIANGLES, material,
				  nodes, normals, texcoords,
				  vertex_index, normal_index, tex_index,
				  is_base, ground_lights );

	if (use_random_objects)
	  gen_random_surface_objects(leaf, random_object_branch,
				     center, material);
	geometry->addKid( leaf );
    }

    // generate strips
    string_list strip_materials = obj.get_strip_materials();
    group_list strips_v = obj.get_strips_v();
    group_list strips_n = obj.get_strips_n();
    group_list strips_tc = obj.get_strips_tc();
    for ( i = 0; i < (int)strips_v.size(); ++i ) {
	material = strip_materials[i];
	vertex_index = strips_v[i];
	normal_index = strips_n[i];
	tex_index = strips_tc[i];
	ssgLeaf *leaf = gen_leaf( path, GL_TRIANGLE_STRIP, material,
				  nodes, normals, texcoords,
				  vertex_index, normal_index, tex_index,
				  is_base, ground_lights );

	if (use_random_objects)
	  gen_random_surface_objects(leaf, random_object_branch,
				     center, material);
	geometry->addKid( leaf );
    }

    // generate fans
    string_list fan_materials = obj.get_fan_materials();
    group_list fans_v = obj.get_fans_v();
    group_list fans_n = obj.get_fans_n();
    group_list fans_tc = obj.get_fans_tc();
    for ( i = 0; i < (int)fans_v.size(); ++i ) {
	material = fan_materials[i];
	vertex_index = fans_v[i];
	normal_index = fans_n[i];
	tex_index = fans_tc[i];
	ssgLeaf *leaf = gen_leaf( path, GL_TRIANGLE_FAN, material,
				  nodes, normals, texcoords,
				  vertex_index, normal_index, tex_index,
				  is_base, ground_lights );
	if (use_random_objects)
	  gen_random_surface_objects(leaf, random_object_branch,
				     center, material);
	geometry->addKid( leaf );
    }

    return true;
}
