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
// (Log is kept at end of this file)


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
#include <Triangulate/triangle.hxx>

FG_USING_STD(string);
FG_USING_STD(vector);


typedef vector < int > belongs_to;
typedef belongs_to::iterator belongs_to_iterator;
typedef belongs_to::const_iterator belongs_to_tripoly_iterator;

typedef vector < belongs_to > belongs_to_list;
typedef belongs_to_list::iterator belongs_to_list_iterator;
typedef belongs_to_list::const_iterator belongs_to_list_tripoly_iterator;


class FGGenOutput {

private:

    // node list in geodetic coordinats
    point_list geod_nodes;

    // node list in cartesian coords (wgs84 model)
    point_list wgs84_nodes;

    // face normal list (for flat shading)
    point_list face_normals;

    // normal list (for each point) in cart coords (for smooth
    // shading)
    point_list point_normals;

    // triangles (by index into point list)
    triele_list tri_elements;

    // for each node, a list of triangle indices that contain this node
    belongs_to_list reverse_ele_lookup;

    // global bounding sphere
    Point3D gbs_center;
    double gbs_radius;

    // build the wgs-84 point list
    void gen_wgs84_points( const FGArray& array );

    // build the node -> element (triangle) reverse lookup table.
    // there is an entry for each point containing a list of all the
    // triangles that share that point.
    void gen_node_ele_lookup_table();

    // calculate the normals for each point in wgs84_nodes
    void gen_normals();

    // build the face normal list
    void gen_face_normals();

    // caclulate the normal for the specified triangle face
    Point3D calc_normal( int i );

    // calculate the global bounding sphere.  Center is the average of
    // the points.
    void calc_gbs();

    // caclulate the bounding sphere for a list of triangle faces
    void calc_group_bounding_sphere( const triele_list& tris, 
				     Point3D *center, double *radius );

    // caclulate the bounding sphere for the specified triangle face
    void calc_bounding_sphere( const FGTriEle& t, 
			       Point3D *center, double *radius );

public:

    // Constructor && Destructor
    inline FGGenOutput() { }
    inline ~FGGenOutput() { }

    // build the necessary output structures based on the
    // triangulation data
    int build( const FGArray& array, const FGTriangle& t );

    // write out the fgfs scenery file
    int write( const string& base, const FGBucket& b );
};


#endif // _GENOBJ_HXX


// $Log$
// Revision 1.6  1999/03/27 14:06:43  curt
// Tweaks to bounding sphere calculation routines.
// Group like triangles together for output to be in a single display list,
// even though they are individual, non-fanified, triangles.
//
// Revision 1.5  1999/03/27 05:23:23  curt
// Interpolate real z value of all nodes from dem data.
// Write scenery file to correct location.
// Pass along correct triangle attributes and write to output file.
//
// Revision 1.4  1999/03/25 19:04:22  curt
// Preparations for outputing scenery file to correct location.
//
// Revision 1.3  1999/03/23 22:02:04  curt
// Worked on creating data to output ... normals, bounding spheres, etc.
//
// Revision 1.2  1999/03/23 17:44:49  curt
// Beginning work on generating output scenery.
//
// Revision 1.1  1999/03/22 23:51:51  curt
// Initial revision.
//
