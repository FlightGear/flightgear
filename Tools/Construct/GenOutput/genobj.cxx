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


#include <time.h>

#include <Polygon/names.hxx>
#include <Tools/scenery_version.hxx>

#include "genobj.hxx"


// calculate the global bounding sphere.  Center is the average of the
// points.
void FGGenOutput::calc_gbs( FGConstruct& c ) {
    double dist_squared;
    double radius_squared = 0;
    
    gbs_center = Point3D( 0.0 );

    point_list wgs84_nodes = c.get_wgs84_nodes();
    const_point_list_iterator current = wgs84_nodes.begin();
    const_point_list_iterator last = wgs84_nodes.end();

    for ( ; current != last; ++current ) {
	gbs_center += *current;
    }

    gbs_center /= wgs84_nodes.size();

    current =  wgs84_nodes.begin();
    for ( ; current != last; ++current ) {
        dist_squared = gbs_center.distance3Dsquared(*current);
	if ( dist_squared > radius_squared ) {
            radius_squared = dist_squared;
        }
    }

    gbs_radius = sqrt(radius_squared);
}


#define FG_STANDARD_TEXTURE_DIMENSION 1000.0 // meters

// traverse the specified fan and attempt to calculate "none
// stretching" texture coordinates
int_list FGGenOutput::calc_tex_coords( FGConstruct& c, point_list geod_nodes,
				       int_list fan )
{
    // cout << "calculating texture coordinates for a specific fan of size = "
    //      << fan.size() << endl;

    FGBucket b = c.get_bucket();
    double clat = b.get_center_lat();
    double clat_rad = clat * DEG_TO_RAD;
    double cos_lat = cos( clat_rad );
    double local_radius = cos_lat * EQUATORIAL_RADIUS_M;
    double local_perimeter = 2.0 * local_radius * FG_PI;
    double degree_width = local_perimeter / 360.0;

    // cout << "clat = " << clat << endl;
    // cout << "clat (radians) = " << clat_rad << endl;
    // cout << "cos(lat) = " << cos_lat << endl;
    // cout << "local_radius = " << local_radius << endl;
    // cout << "local_perimeter = " << local_perimeter << endl;
    // cout << "degree_width = " << degree_width << endl;

    double perimeter = 2.0 * EQUATORIAL_RADIUS_M * FG_PI;
    double degree_height = perimeter / 360.0;
    // cout << "degree_height = " << degree_height << endl;

    // find min/max of fan
    Point3D min, max, p, t;
    bool first = true;

    for ( int i = 0; i < (int)fan.size(); ++i ) {
	p = geod_nodes[ fan[i] ];
	t.setx( p.x() * ( degree_width / FG_STANDARD_TEXTURE_DIMENSION ) );
	t.sety( p.y() * ( degree_height / FG_STANDARD_TEXTURE_DIMENSION ) );

	if ( first ) {
	    min = max = t;
	    first = false;
	} else {
	    if ( t.x() < min.x() ) {
		min.setx( t.x() );
	    }
	    if ( t.y() < min.y() ) {
		min.sety( t.y() );
	    }
	    if ( t.x() > max.x() ) {
		max.setx( t.x() );
	    }
	    if ( t.y() > max.y() ) {
		max.sety( t.y() );
	    }
	}
    }
    min.setx( (double)( (int)min.x() - 1 ) );
    min.sety( (double)( (int)min.y() - 1 ) );
    // cout << "found min = " << min << endl;

    // generate tex_list
    Point3D shifted_t;
    int index;
    int_list tex;
    tex.clear();
    for ( int i = 0; i < (int)fan.size(); ++i ) {
	p = geod_nodes[ fan[i] ];
	t.setx( p.x() * ( degree_width / FG_STANDARD_TEXTURE_DIMENSION ) );
	t.sety( p.y() * ( degree_height / FG_STANDARD_TEXTURE_DIMENSION ) );
	shifted_t = t - min;
	if ( shifted_t.x() < FG_EPSILON ) {
	    shifted_t.setx( 0.0 );
	}
	if ( shifted_t.y() < FG_EPSILON ) {
	    shifted_t.sety( 0.0 );
	}
	shifted_t.setz( 0.0 );
	// cout << "shifted_t = " << shifted_t << endl;
	index = tex_coords.unique_add( shifted_t );
	tex.push_back( index );
    }

    return tex;
}


// build the necessary output structures based on the triangulation
// data
int FGGenOutput::build( FGConstruct& c ) {
    FGTriNodes trinodes = c.get_tri_nodes();

    // copy the geodetic node list into this class
    geod_nodes = trinodes.get_node_list();

    // copy the triangle list into this class
    tri_elements = c.get_tri_elements();

    // build the trifan list
    cout << "total triangles = " << tri_elements.size() << endl;
    FGGenFans f;
    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	triele_list area_tris;
	area_tris.erase( area_tris.begin(), area_tris.end() );

	const_triele_list_iterator t_current = tri_elements.begin();
	const_triele_list_iterator t_last = tri_elements.end();
	for ( ; t_current != t_last; ++t_current ) {
	    if ( (int)t_current->get_attribute() == i ) {
		area_tris.push_back( *t_current );
	    }
	}

	if ( (int)area_tris.size() > 0 ) {
	    cout << "generating fans for area = " << i << endl;
	    fans[i] = f.greedy_build( area_tris );
	}
    }

    // build the texture coordinate list and make a parallel structure
    // to the fan list for pointers into the texture list
    cout << "calculating texture coordinates" << endl;
    tex_coords.clear();

    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	for ( int j = 0; j < (int)fans[i].size(); ++j ) {
	    int_list t_list = calc_tex_coords( c, geod_nodes, fans[i][j] );
	    // cout << fans[i][j].size() << " === " 
	    //      << t_list.size() << endl; 
	    textures[i].push_back( t_list );
	}
    }

    // calculate the global bounding sphere
    calc_gbs( c );
    cout << "center = " << gbs_center << " radius = " << gbs_radius << endl;

    return 1;
}


// caclulate the bounding sphere for a list of triangle faces
void FGGenOutput::calc_group_bounding_sphere( FGConstruct& c, 
					      const fan_list& fans, 
					      Point3D *center, double *radius )
{
    cout << "calculate group bounding sphere for " << fans.size() << " fans." 
	 << endl;

    point_list wgs84_nodes = c.get_wgs84_nodes();

    // generate a list of unique points from the triangle list
    FGTriNodes nodes;

    const_fan_list_iterator f_current = fans.begin();
    const_fan_list_iterator f_last = fans.end();
    for ( ; f_current != f_last; ++f_current ) {
	const_int_list_iterator i_current = f_current->begin();
	const_int_list_iterator i_last = f_current->end();
	for ( ; i_current != i_last; ++i_current ) {
	    Point3D p1 = wgs84_nodes[ *i_current ];
	    nodes.unique_add(p1);
	}
    }

    // find average of point list
    *center = Point3D( 0.0 );
    point_list points = nodes.get_node_list();
    // cout << "found " << points.size() << " unique nodes" << endl;
    point_list_iterator p_current = points.begin();
    point_list_iterator p_last = points.end();
    for ( ; p_current != p_last; ++p_current ) {
	*center += *p_current;
    }
    *center /= points.size();

    // find max radius
    double dist_squared;
    double max_squared = 0;

    p_current = points.begin();
    p_last = points.end();
    for ( ; p_current != p_last; ++p_current ) {
	dist_squared = (*center).distance3Dsquared(*p_current);
	if ( dist_squared > max_squared ) {
	    max_squared = dist_squared;
	}
    }

    *radius = sqrt(max_squared);
}


// caclulate the bounding sphere for the specified triangle face
void FGGenOutput::calc_bounding_sphere( FGConstruct& c, const FGTriEle& t, 
					Point3D *center, double *radius )
{
    point_list wgs84_nodes = c.get_wgs84_nodes();

    *center = Point3D( 0.0 );

    Point3D p1 = wgs84_nodes[ t.get_n1() ];
    Point3D p2 = wgs84_nodes[ t.get_n2() ];
    Point3D p3 = wgs84_nodes[ t.get_n3() ];

    *center = p1 + p2 + p3;
    *center /= 3;

    double dist_squared;
    double max_squared = 0;

    dist_squared = (*center).distance3Dsquared(p1);
    if ( dist_squared > max_squared ) {
	max_squared = dist_squared;
    }

    dist_squared = (*center).distance3Dsquared(p2);
    if ( dist_squared > max_squared ) {
	max_squared = dist_squared;
    }

    dist_squared = (*center).distance3Dsquared(p3);
    if ( dist_squared > max_squared ) {
	max_squared = dist_squared;
    }

    *radius = sqrt(max_squared);
}


// write out the fgfs scenery file
int FGGenOutput::write( FGConstruct &c ) {
    Point3D p;

    string base = c.get_output_base();
    FGBucket b = c.get_bucket();

    string dir = base + "/Scenery/" + b.gen_base_path();
    string command = "mkdir -p " + dir;
    system(command.c_str());

    string file = dir + "/" + b.gen_index_str();
    cout << "Output file = " << file << endl;

    FILE *fp;
    if ( (fp = fopen( file.c_str(), "w" )) == NULL ) {
	cout << "ERROR: opening " << file << " for writing!" << endl;
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
    point_list wgs84_nodes = c.get_wgs84_nodes();
    fprintf(fp, "# vertex list\n");
    const_point_list_iterator w_current = wgs84_nodes.begin();
    const_point_list_iterator w_last = wgs84_nodes.end();
    for ( ; w_current != w_last; ++w_current ) {
	p = *w_current - gbs_center;
	fprintf(fp, "v %.5f %.5f %.5f\n", p.x(), p.y(), p.z());
    }
    fprintf(fp, "\n");
    
    // write vertex normals
    point_list point_normals = c.get_point_normals();
    fprintf(fp, "# vertex normal list\n");
    const_point_list_iterator n_current = point_normals.begin();
    const_point_list_iterator n_last = point_normals.end();
    for ( ; n_current != n_last; ++n_current ) {
	p = *n_current;
	fprintf(fp, "vn %.5f %.5f %.5f\n", p.x(), p.y(), p.z());
    }
    fprintf(fp, "\n");

    // write texture coordinates
    point_list tex_coord_list = tex_coords.get_node_list();
    for ( int i = 0; i < (int)tex_coord_list.size(); ++i ) {
	p = tex_coord_list[i];
	fprintf(fp, "vt %.5f %.5f\n", p.x(), p.y());
    }
    fprintf(fp, "\n");

    // write triangles (grouped by type for now)
    Point3D center;
    double radius;
    fprintf(fp, "# triangle groups\n");
    fprintf(fp, "\n");

    int total_tris = 0;
    for ( int i = 0; i < FG_MAX_AREA_TYPES; ++i ) {
	if ( (int)fans[i].size() > 0 ) {
	    string attr_name = get_area_name( (AreaType)i );
	    calc_group_bounding_sphere( c, fans[i], &center, &radius );
	    cout << "writing " << (int)fans[i].size() << " fans for " 
		 << attr_name << endl;

	    fprintf(fp, "# usemtl %s\n", attr_name.c_str() );
	    fprintf(fp, "# bs %.4f %.4f %.4f %.2f\n", 
		    center.x(), center.y(), center.z(), radius);

	    for ( int j = 0; j < (int)fans[i].size(); ++j ) {
		fprintf( fp, "tf" );
		total_tris += fans[i][j].size() - 2;
		for ( int k = 0; k < (int)fans[i][j].size(); ++k ) {
		    fprintf( fp, " %d/%d", fans[i][j][k], textures[i][j][k] );
		}
		fprintf( fp, "\n" );
	    }

	    fprintf( fp, "\n" );
	}
    }
    cout << "wrote " << total_tris << " tris to output file" << endl;

    fclose(fp);

    command = "gzip --force --best " + file;
    system(command.c_str());

    return 1;
}


