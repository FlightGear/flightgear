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


#include <time.h>

#include <Tools/scenery_version.hxx>
#include <Math/mat3.h>

#include "genobj.hxx"


// build the wgs-84 point list
void FGGenOutput::gen_wgs84_points() {
    cout << "calculating wgs84 point" << endl;
    Point3D geod, radians, cart;

    const_point_list_iterator current = geod_nodes.begin();
    const_point_list_iterator last = geod_nodes.end();

    for ( ; current != last; ++current ) {
	geod = *current;

	// convert to radians
	radians = Point3D( geod.x() * DEG_TO_RAD,
			   geod.y() * DEG_TO_RAD,
			   geod.z() );

        cart = fgGeodToCart(radians);
	// cout << cart << endl;
        wgs84_nodes.push_back(cart);
    }
}


// build the node -> element (triangle) reverse lookup table.  there
// is an entry for each point containing a list of all the triangles
// that share that point.
void FGGenOutput::gen_node_ele_lookup_table() {
    belongs_to ele_list;
    ele_list.erase( ele_list.begin(), ele_list.end() );

    // initialize reverse_ele_lookup structure by creating an empty
    // list for each point
    const_point_list_iterator w_current = wgs84_nodes.begin();
    const_point_list_iterator w_last = wgs84_nodes.end();
    for ( ; w_current != w_last; ++w_current ) {
	reverse_ele_lookup.push_back( ele_list );
    }

    // traverse triangle structure building reverse lookup table
    const_triele_list_iterator current = tri_elements.begin();
    const_triele_list_iterator last = tri_elements.end();
    int counter = 0;
    for ( ; current != last; ++current ) {
	reverse_ele_lookup[ current->get_n1() ].push_back( counter );
	reverse_ele_lookup[ current->get_n2() ].push_back( counter );
	reverse_ele_lookup[ current->get_n3() ].push_back( counter );
	++counter;
    }
}


// caclulate the normal for the specified triangle face
Point3D FGGenOutput::calc_normal( int i ) {
    double v1[3], v2[3], normal[3];
    double temp;

    Point3D p1 = wgs84_nodes[ tri_elements[i].get_n1() ];
    Point3D p2 = wgs84_nodes[ tri_elements[i].get_n2() ];
    Point3D p3 = wgs84_nodes[ tri_elements[i].get_n3() ];

    v1[0] = p2.x() - p1.x(); v1[1] = p2.y() - p1.y(); v1[2] = p2.z() - p1.z();
    v2[0] = p3.x() - p1.x(); v2[1] = p3.y() - p1.y(); v2[2] = p3.z() - p1.z();

    MAT3cross_product(normal, v1, v2);
    MAT3_NORMALIZE_VEC(normal,temp);

    return Point3D( normal[0], normal[1], normal[2] );
}


// build the face normal list
void FGGenOutput::gen_face_normals() {
    // traverse triangle structure building the face normal table

    cout << "calculating face normals" << endl;

    for ( int i = 0; i < (int)tri_elements.size(); i++ ) {
	// cout << calc_normal( i ) << endl;
	face_normals.push_back( calc_normal( i ) );
    }

}


// calculate the normals for each point in wgs84_nodes
void FGGenOutput::gen_normals() {
    Point3D normal;
    cout << "caculating node normals" << endl;

    // for each node
    for ( int i = 0; i < (int)wgs84_nodes.size(); ++i ) {
	belongs_to tri_list = reverse_ele_lookup[i];

	belongs_to_iterator current = tri_list.begin();
	belongs_to_iterator last = tri_list.end();

	Point3D average( 0.0 );

	// for each triangle that shares this node
	for ( ; current != last; ++current ) {
	    normal = face_normals[ *current ];
	    average += normal;
	    // cout << normal << endl;
	}

	average /= tri_list.size();
	// cout << "average = " << average << endl;

	point_normals.push_back( average );
    }
}


// calculate the global bounding sphere.  Center is the average of the
// points.
void FGGenOutput::calc_gbs() {
    double x = 0;
    double y = 0;
    double z = 0;

    double dist_squared;
    double radius_squared = 0;
    
    const_point_list_iterator current = wgs84_nodes.begin();
    const_point_list_iterator last = wgs84_nodes.end();

    for ( ; current != last; ++current ) {
	x += current->x();
	y += current->y();
	z += current->z();
    }

    x /= wgs84_nodes.size();
    y /= wgs84_nodes.size();
    z /= wgs84_nodes.size();

    gbs_center = Point3D(x, y, z);

    current =  wgs84_nodes.begin();
    for ( ; current != last; ++current ) {
        dist_squared = gbs_center.distance3Dsquared(*current);
	if ( dist_squared > radius_squared ) {
            radius_squared = dist_squared;
        }
    }

    gbs_radius = sqrt(radius_squared);
}


// build the necessary output structures based on the triangulation
// data
int FGGenOutput::build( const FGTriangle& t ) {
    FGTriNodes trinodes = t.get_out_nodes();

    // copy the geodetic node list into this class
    geod_nodes = trinodes.get_node_list();

    // copy the triangle list into this class
    tri_elements = t.get_elelist();

    // generate the point list in wgs-84 coordinates
    gen_wgs84_points();

    // calculate the global bounding sphere
    calc_gbs();
    cout << "center = " << gbs_center << " radius = " << gbs_radius << endl;

    // build the node -> element (triangle) reverse lookup table
    gen_node_ele_lookup_table();

    // build the face normal list
    gen_face_normals();

    // calculate the normals for each point in wgs84_nodes
    gen_normals();

    return 1;
}


// caclulate the bounding sphere for the specified triangle face
void FGGenOutput::calc_bounding_sphere( int i, Point3D *center, 
					double *radius ) {
    Point3D c( 0.0 );

    Point3D p1 = wgs84_nodes[ tri_elements[i].get_n1() ];
    Point3D p2 = wgs84_nodes[ tri_elements[i].get_n2() ];
    Point3D p3 = wgs84_nodes[ tri_elements[i].get_n3() ];

    c = p1 + p2 + p3;
    c /= 3;

    double dist_squared;
    double max_squared = 0;

    dist_squared = c.distance3Dsquared(p1);
    if ( dist_squared > max_squared ) {
	max_squared = dist_squared;
    }

    dist_squared = c.distance3Dsquared(p2);
    if ( dist_squared > max_squared ) {
	max_squared = dist_squared;
    }

    dist_squared = c.distance3Dsquared(p3);
    if ( dist_squared > max_squared ) {
	max_squared = dist_squared;
    }

    *center = c;
    *radius = sqrt(max_squared);
}


// write out the fgfs scenery file
int FGGenOutput::write( const FGBucket& b, const string& path ) {
    Point3D p;

    FILE *fp;
    if ( (fp = fopen( path.c_str(), "w" )) == NULL ) {
	cout << "ERROR: opening " << path << " for writing!" << endl;
	exit(-1);
    }

    // write headers
    fprintf(fp, "# FGFS Scenery Version %s\n", FG_SCENERY_FILE_FORMAT);

    time_t calendar_time = time(NULL);
    struct tm *local_tm;
    local_tm = localtime( &calendar_time );
    char time_str[256];
    strftime( time_str, 256, "%a %b %d %H:%M:%S %Z %Y", local_tm);
    fprintf(fp, "# Created %s\n", time_str );
    fprintf(fp, "\n");

    // write global bounding sphere
    fprintf(fp, "# gbs %.5f %.5f %.5f %.2f\n",
	    gbs_center.x(), gbs_center.y(), gbs_center.z(), gbs_radius);
    fprintf(fp, "\n");

    // write nodes
    fprintf(fp, "# vertex list\n");
    const_point_list_iterator w_current = wgs84_nodes.begin();
    const_point_list_iterator w_last = wgs84_nodes.end();
    for ( ; w_current != w_last; ++w_current ) {
	p = *w_current - gbs_center;
	fprintf(fp, "v %.5f %.5f %.5f\n", p.x(), p.y(), p.z());
    }
    fprintf(fp, "\n");
    
    // write vertex normals
    fprintf(fp, "# vertex normal list\n");
    const_point_list_iterator n_current = point_normals.begin();
    const_point_list_iterator n_last = point_normals.end();
    for ( ; n_current != n_last; ++n_current ) {
	p = *n_current;
	fprintf(fp, "vn %.5f %.5f %.5f\n", p.x(), p.y(), p.z());
    }
    fprintf(fp, "\n");

    // write triangles
    Point3D center;
    double radius;
    fprintf(fp, "# triangle list\n");
    fprintf(fp, "\n");
    const_triele_list_iterator t_current = tri_elements.begin();
    const_triele_list_iterator t_last = tri_elements.end();
    int counter = 0;
    for ( ; t_current != t_last; ++t_current ) {
	calc_bounding_sphere( counter, &center, &radius );
	fprintf(fp, "# usemtl desert1\n");
	fprintf(fp, "# bs %.2f %.2f %.2f %.2f\n", 
		center.x(), center.y(), center.z(), radius);
	fprintf(fp, "f %d %d %d\n", 
		t_current->get_n1(), t_current->get_n2(), t_current->get_n3());
	fprintf(fp, "\n");
	++counter;
    }

    return 1;
}


// $Log$
// Revision 1.3  1999/03/25 19:04:21  curt
// Preparations for outputing scenery file to correct location.
//
// Revision 1.2  1999/03/23 22:02:03  curt
// Worked on creating data to output ... normals, bounding spheres, etc.
//
// Revision 1.1  1999/03/22 23:51:51  curt
// Initial revision.
//
