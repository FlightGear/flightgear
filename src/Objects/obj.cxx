// obj.cxx -- routines to handle loading scenery and building the plib
//            scene graph.
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

#include STL_STRING
#include <map>                  // STL
#include <vector>               // STL
#include <ctype.h>              // isdigit()

#include <simgear/constants.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/sg_binobj.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/math/vector.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/stopwatch.hxx>
#include <simgear/misc/texcoord.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/tgdb/leaf.hxx>
#include <simgear/scene/tgdb/pt_lights.hxx>

#include <Main/globals.hxx>

#include "obj.hxx"

SG_USING_STD(string);
SG_USING_STD(vector);


typedef vector < int > int_list;
typedef int_list::iterator int_list_iterator;
typedef int_list::const_iterator int_point_list_iterator;


// Generate an ocean tile
bool fgGenTile( const string& path, SGBucket b,
                Point3D *center, double *bounding_radius,
                SGMaterialLib *matlib, ssgBranch* geometry )
{
    ssgSimpleState *state = NULL;

    geometry->setName( (char *)path.c_str() );

    double tex_width = 1000.0;
    // double tex_height;

    // find Ocean material in the properties list
    SGMaterial *mat = matlib->find( "Ocean" );
    if ( mat != NULL ) {
        // set the texture width and height values for this
        // material
        tex_width = mat->get_xsize();
        // tex_height = newmat->get_ysize();
        
        // set ssgState
        state = mat->get_state();
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
    geod_nodes.reserve(4);
    int_list rectangle;
    rectangle.clear();
    rectangle.reserve(4);
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
    double a = sg_random();
    double b = sg_random();
    if ( a + b > 1.0 ) {
        a = 1.0 - a;
        b = 1.0 - b;
    }
    double c = 1 - a - b;

    res[0] = n1[0]*a + n2[0]*b + n3[0]*c;
    res[1] = n1[1]*a + n2[1]*b + n3[1]*c;
    res[2] = n1[2]*a + n2[2]*b + n3[2]*c;
}


/**
 * User data for populating leaves when they come in range.
 */
class LeafUserData : public ssgBase
{
public:
    bool is_filled_in;
    ssgLeaf *leaf;
    SGMaterial *mat;
    ssgBranch *branch;
    float sin_lat;
    float cos_lat;
    float sin_lon;
    float cos_lon;

    void setup_triangle( int i );
};


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
    sgVec3 center;
    double area;
  SGMatObjectGroup * object_group;
  ssgBranch * branch;
    LeafUserData * leafData;
  unsigned int seed;

    void fill_in_triangle();
    void add_object_to_triangle(SGMatObject * object);
    void makeWorldMatrix (sgMat4 ROT, double hdg_deg );
};


/**
 * Fill in a triangle with randomly-placed objects.
 *
 * This method is invoked by a callback when the triangle is in range
 * but not yet populated.
 *
 */

void TriUserData::fill_in_triangle ()
{
                                // generate a repeatable random seed
    sg_srandom(seed);

    int nObjects = object_group->get_object_count();

    for (int i = 0; i < nObjects; i++) {
      SGMatObject * object = object_group->get_object(i);
      double num = area / object->get_coverage_m2();

      // place an object each unit of area
      while ( num > 1.0 ) {
          add_object_to_triangle(object);
          num -= 1.0;
      }
      // for partial units of area, use a zombie door method to
      // create the proper random chance of an object being created
      // for this triangle
      if ( num > 0.0 ) {
        if ( sg_random() <= num ) {
          // a zombie made it through our door
                add_object_to_triangle(object);
        }
      }
    }
}

void TriUserData::add_object_to_triangle (SGMatObject * object)
{
    // Set up the random heading if required.
    double hdg_deg = 0;
    if (object->get_heading_type() == SGMatObject::HEADING_RANDOM)
        hdg_deg = sg_random() * 360;

    sgMat4 mat;
    makeWorldMatrix(mat, hdg_deg);

    ssgTransform * pos = new ssgTransform;
    pos->setTransform(mat);
    pos->addKid( object->get_random_model( globals->get_model_loader(),
                                           globals->get_fg_root(),
                                           globals->get_props(),
                                           globals->get_sim_time_sec() ) );
    branch->addKid(pos);
}

void TriUserData::makeWorldMatrix (sgMat4 mat, double hdg_deg )
{
    if (hdg_deg == 0) {
        mat[0][0] =  leafData->sin_lat * leafData->cos_lon;
        mat[0][1] =  leafData->sin_lat * leafData->sin_lon;
        mat[0][2] = -leafData->cos_lat;
        mat[0][3] =  SG_ZERO;

        mat[1][0] =  -leafData->sin_lon;
        mat[1][1] =  leafData->cos_lon;
        mat[1][2] =  SG_ZERO;
        mat[1][3] =  SG_ZERO;
    } else {
        float sin_hdg = sin( hdg_deg * SGD_DEGREES_TO_RADIANS ) ;
        float cos_hdg = cos( hdg_deg * SGD_DEGREES_TO_RADIANS ) ;
        mat[0][0] =  cos_hdg * leafData->sin_lat * leafData->cos_lon - sin_hdg * leafData->sin_lon;
        mat[0][1] =  cos_hdg * leafData->sin_lat * leafData->sin_lon + sin_hdg * leafData->cos_lon;
        mat[0][2] = -cos_hdg * leafData->cos_lat;
        mat[0][3] =  SG_ZERO;

        mat[1][0] = -sin_hdg * leafData->sin_lat * leafData->cos_lon - cos_hdg * leafData->sin_lon;
        mat[1][1] = -sin_hdg * leafData->sin_lat * leafData->sin_lon + cos_hdg * leafData->cos_lon;
        mat[1][2] =  sin_hdg * leafData->cos_lat;
        mat[1][3] =  SG_ZERO;
    }

    mat[2][0] = leafData->cos_lat * leafData->cos_lon;
    mat[2][1] = leafData->cos_lat * leafData->sin_lon;
    mat[2][2] = leafData->sin_lat;
    mat[2][3] = SG_ZERO;

    // translate to random point in triangle
    sgVec3 result;
    random_pt_inside_tri(result, p1, p2, p3);
    sgSubVec3(mat[3], result, center);

    mat[3][3] = SG_ONE ;
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
        data->fill_in_triangle();
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
class DummyBSphereEntity : public ssgBranch
{
public:
  DummyBSphereEntity (float radius)
  {
    bsphere.setCenter(0, 0, 0);
    bsphere.setRadius(radius);
  }
  virtual ~DummyBSphereEntity () {}
  virtual void recalcBSphere () { bsphere_is_invalid = false; }
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
 */

void LeafUserData::setup_triangle (int i )
{
    short n1, n2, n3;
    leaf->getTriangle(i, &n1, &n2, &n3);

    float * p1 = leaf->getVertex(n1);
    float * p2 = leaf->getVertex(n2);
    float * p3 = leaf->getVertex(n3);

                                // Set up a single center point for LOD
    sgVec3 center;
    sgSetVec3(center,
              (p1[0] + p2[0] + p3[0]) / 3.0,
              (p1[1] + p2[1] + p3[1]) / 3.0,
              (p1[2] + p2[2] + p3[2]) / 3.0);
    double area = sgTriArea(p1, p2, p3);
      
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
    for (int j = 0; j < num_groups; j++) {
                                // Look up the random object.
        SGMatObjectGroup * group = mat->get_object_group(j);

                                // Set up the range selector for the entire
                                // triangle; note that we use the object
                                // range plus the bounding radius here, to
                                // allow for objects far from the center.
        float ranges[] = { 0,
                          group->get_range_m() + bounding_radius,
                SG_MAX };
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
        sgCopyVec3 (data->center, center);
        data->area = area;
        data->object_group = group;
        data->branch = in_range;
        data->leafData = this;
        data->seed = (unsigned int)(p1[0] * j);

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
            data->setup_triangle(i);
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
 * @param material_name The name of the surface's material.
 */
static void
gen_random_surface_objects (ssgLeaf *leaf,
                            ssgBranch *branch,
                            Point3D *center,
                            SGMaterial *mat )
{
                                // If the surface has no triangles, return
                                // now.
    int num_tris = leaf->getNumTriangles();
    if (num_tris < 1)
        return;

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
    float ranges[] = { 0, 20000, 1000000 };
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
    data->sin_lat = sin(lat_deg * SGD_DEGREES_TO_RADIANS);
    data->cos_lat = cos(lat_deg * SGD_DEGREES_TO_RADIANS);
    data->sin_lon = sin(lon_deg * SGD_DEGREES_TO_RADIANS);
    data->cos_lon = cos(lon_deg * SGD_DEGREES_TO_RADIANS);

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

// Load an Binary obj file
bool fgBinObjLoad( const string& path, const bool is_base,
                   Point3D *center,
                   double *bounding_radius,
                   SGMaterialLib *matlib,
                   bool use_random_objects,
                   ssgBranch* geometry,
                   ssgBranch* rwy_lights,
                   ssgBranch* taxi_lights,
                   ssgVertexArray *ground_lights )
{
    SGBinObject obj;

    if ( ! obj.read_bin( path ) ) {
        return false;
    }

    geometry->setName( (char *)path.c_str() );

    // reference point (center offset/bounding sphere)
    *center = obj.get_gbs_center();
    *bounding_radius = obj.get_gbs_radius();

    point_list const& nodes = obj.get_wgs84_nodes();
    // point_list const& colors = obj.get_colors();
    point_list const& normals = obj.get_normals();
    point_list const& texcoords = obj.get_texcoords();

    string material;
    int_list tex_index;

    group_list::size_type i;

    // generate points
    string_list const& pt_materials = obj.get_pt_materials();
    group_list const& pts_v = obj.get_pts_v();
    group_list const& pts_n = obj.get_pts_n();
    for ( i = 0; i < pts_v.size(); ++i ) {
        // cout << "pts_v.size() = " << pts_v.size() << endl;
	if ( pt_materials[i].substr(0, 3) == "RWY" ) {
            sgVec3 up;
            sgSetVec3( up, center->x(), center->y(), center->z() );
            // returns a transform -> lod -> leaf structure
            ssgBranch *branch = sgMakeDirectionalLights( nodes, normals,
                                                         pts_v[i], pts_n[i],
                                                         matlib,
                                                         pt_materials[i], up );
            if ( pt_materials[i].substr(0, 16) == "RWY_BLUE_TAXIWAY" ) {
                taxi_lights->addKid( branch );
            } else {
                rwy_lights->addKid( branch );
            }
        } else {
            material = pt_materials[i];
            tex_index.clear();
            ssgLeaf *leaf = sgMakeLeaf( path, GL_POINTS, matlib, material,
                                        nodes, normals, texcoords,
                                        pts_v[i], pts_n[i], tex_index,
                                        false, ground_lights );
            geometry->addKid( leaf );
        }
    }

    // Put all randomly-placed objects under a separate branch
    // (actually an ssgRangeSelector) named "random-models".
    ssgBranch * random_object_branch = 0;
    if (use_random_objects) {
        float ranges[] = { 0, 20000 }; // Maximum 20km range for random objects
        ssgRangeSelector * object_lod = new ssgRangeSelector;
        object_lod->setRanges(ranges, 2);
        object_lod->setName("random-models");
        geometry->addKid(object_lod);
        random_object_branch = new ssgBranch;
        object_lod->addKid(random_object_branch);
    }

    // generate triangles
    string_list const& tri_materials = obj.get_tri_materials();
    group_list const& tris_v = obj.get_tris_v();
    group_list const& tris_n = obj.get_tris_n();
    group_list const& tris_tc = obj.get_tris_tc();
    for ( i = 0; i < tris_v.size(); ++i ) {
        ssgLeaf *leaf = sgMakeLeaf( path, GL_TRIANGLES, matlib,
                                    tri_materials[i],
                                    nodes, normals, texcoords,
                                    tris_v[i], tris_n[i], tris_tc[i],
                                    is_base, ground_lights );

        if ( use_random_objects ) {
            SGMaterial *mat = matlib->find( tri_materials[i] );
            if ( mat == NULL ) {
                SG_LOG( SG_INPUT, SG_ALERT,
                        "Unknown material for random surface objects = "
                        << tri_materials[i] );
            }
            gen_random_surface_objects( leaf, random_object_branch,
                                        center, mat );
        }
        geometry->addKid( leaf );
    }

    // generate strips
    string_list const& strip_materials = obj.get_strip_materials();
    group_list const& strips_v = obj.get_strips_v();
    group_list const& strips_n = obj.get_strips_n();
    group_list const& strips_tc = obj.get_strips_tc();
    for ( i = 0; i < strips_v.size(); ++i ) {
        ssgLeaf *leaf = sgMakeLeaf( path, GL_TRIANGLE_STRIP,
                                    matlib, strip_materials[i],
                                    nodes, normals, texcoords,
                                    strips_v[i], strips_n[i], strips_tc[i],
                                    is_base, ground_lights );

        if ( use_random_objects ) {
            SGMaterial *mat = matlib->find( strip_materials[i] );
            if ( mat == NULL ) {
                SG_LOG( SG_INPUT, SG_ALERT,
                        "Unknown material for random surface objects = "
                        << strip_materials[i] );
            }
            gen_random_surface_objects( leaf, random_object_branch,
                                        center, mat );
        }
        geometry->addKid( leaf );
    }

    // generate fans
    string_list const& fan_materials = obj.get_fan_materials();
    group_list const& fans_v = obj.get_fans_v();
    group_list const& fans_n = obj.get_fans_n();
    group_list const& fans_tc = obj.get_fans_tc();
    for ( i = 0; i < fans_v.size(); ++i ) {
        ssgLeaf *leaf = sgMakeLeaf( path, GL_TRIANGLE_FAN,
                                    matlib, fan_materials[i],
                                    nodes, normals, texcoords,
                                    fans_v[i], fans_n[i], fans_tc[i],
                                    is_base, ground_lights );
        if ( use_random_objects ) {
            SGMaterial *mat = matlib->find( fan_materials[i] );
            if ( mat == NULL ) {
                SG_LOG( SG_INPUT, SG_ALERT,
                        "Unknown material for random surface objects = "
                        << fan_materials[i] );
            }
            gen_random_surface_objects( leaf, random_object_branch,
                                        center, mat );
        }
        geometry->addKid( leaf );
    }

    return true;
}
