// genobj.hxx -- Generate the flight gear "obj" file format from the
//               triangle output
//
// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _GENOBJ_HXX
#define _GENOBJ_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>

#include STL_STRING

#include <Bucket/newbucket.hxx>
#include <Math/fg_geodesy.hxx>
#include <Math/point3d.hxx>

#include <Combine/genfans.hxx>
#include <Main/construct_types.hxx>
#include <Main/construct.hxx>
#include <Triangulate/triangle.hxx>

FG_USING_STD(string);
FG_USING_STD(vector);


typedef vector < int_list > belongs_to_list;
typedef belongs_to_list::iterator belongs_to_list_iterator;
typedef belongs_to_list::const_iterator belongs_to_list_tripoly_iterator;


class FGGenOutput {

private:

    // node list in geodetic coordinates
    point_list geod_nodes;

    // face normal list (for flat shading)
    point_list face_normals;

    // normal list (for each point) in cart coords (for smooth
    // shading)
    point_list point_normals;

    // triangles (by index into point list)
    triele_list tri_elements;

    // fan list
    fan_list fans[FG_MAX_AREA_TYPES];

    // for each node, a list of triangle indices that contain this node
    belongs_to_list reverse_ele_lookup;

    // global bounding sphere
    Point3D gbs_center;
    double gbs_radius;

    // build the node -> element (triangle) reverse lookup table.
    // there is an entry for each point containing a list of all the
    // triangles that share that point.
    void gen_node_ele_lookup_table( FGConstruct& c );

    // calculate the normals for each point in wgs84_nodes
    void gen_normals( FGConstruct& c );

    // build the face normal list
    void gen_face_normals( FGConstruct& c );

    // caclulate the normal for the specified triangle face
    Point3D calc_normal( FGConstruct& c, int i );

    // calculate the global bounding sphere.  Center is the average of
    // the points.
    void calc_gbs( FGConstruct& c );

    // caclulate the bounding sphere for a list of triangle faces
    void calc_group_bounding_sphere( FGConstruct& c, const fan_list& fans, 
				     Point3D *center, double *radius );

    // caclulate the bounding sphere for the specified triangle face
    void calc_bounding_sphere( FGConstruct& c, const FGTriEle& t, 
			       Point3D *center, double *radius );

public:

    // Constructor && Destructor
    inline FGGenOutput() { }
    inline ~FGGenOutput() { }

    // build the necessary output structures based on the
    // triangulation data
    int build( FGConstruct& c, const FGArray& array );

    // write out the fgfs scenery file
    int write( FGConstruct &c );
};


#endif // _GENOBJ_HXX


