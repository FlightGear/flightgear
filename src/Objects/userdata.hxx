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


#include <plib/ssg.h>

class SGMaterial;
class SGMatModel;
class SGMatModelGroup;


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
    SGMatModelGroup * object_group;
    ssgBranch * branch;
    LeafUserData * leafData;
    unsigned int seed;

    void fill_in_triangle();
    void add_object_to_triangle(SGMatModel * object);
    void makeWorldMatrix (sgMat4 ROT, double hdg_deg );
};


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


