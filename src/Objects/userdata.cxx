// userdata.hxx -- two classes for populating ssg user data slots in association
//                 with our implimenation of random surface objects.
//
// Written by David Megginson, started December 2001.
//
// Copyright (C) 2001 - 2003  Curtis L. Olson  - curt@flightgear.org
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


#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/sg_inlines.h>
#include <simgear/math/point3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matmodel.hxx>

#include <Main/globals.hxx>

#include "userdata.hxx"


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
      SGMatModel * object = object_group->get_object(i);
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

void TriUserData::add_object_to_triangle (SGMatModel * object)
{
    // Set up the random heading if required.
    double hdg_deg = 0;
    if (object->get_heading_type() == SGMatModel::HEADING_RANDOM)
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
        SGMatModelGroup * group = mat->get_object_group(j);

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
