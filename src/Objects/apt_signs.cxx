// apt_signs.cxx -- build airport signs on the fly
//
// Written by Curtis Olson, started July 2001.
//
// Copyright (C) 2001  Curtis L. Olson  - curt@flightgear.org
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


#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_types.hxx>

#include "apt_signs.hxx"
#include "obj.hxx"


ssgBranch *gen_taxi_sign( const string path, const string content ) {
    // for demo purposes we assume each element (letter) is 1x1 meter.
    // Sign is placed 0.25 meters above the ground

    ssgBranch *object = new ssgBranch();
    object->setName( (char *)content.c_str() );

    double offset = content.length() / 2.0;

    for ( unsigned int i = 0; i < content.length(); ++i ) {
	string material;

	char item = content[i];
	if ( item == '<' ) {
	    material = "ArrowL.rgb";
	} else if ( item == '>' ) {
	    material = "ArrowR.rgb";
	} else if ( item >= 'A' && item <= 'Z' ) {
	    material = "Letter";
	    material += item;
	    material += ".rgb";
	} else if ( item >= 'a' && item <= 'z' ) {
	    int tmp = item - 'a';
	    char c = 'A' + tmp;
	    material = "Black";
	    material += c;
	    material += ".rgb";
	} else {
	    SG_LOG( SG_TERRAIN, SG_ALERT,
                    "Unknown taxi sign code = '" << item << "' !!!!" );
	    return NULL;
	}

	point_list nodes; nodes.clear();
	point_list normals; normals.clear();
	point_list texcoords; texcoords.clear();
	int_list vertex_index; vertex_index.clear();
	int_list normal_index; normal_index.clear();
	int_list tex_index; tex_index.clear();

	nodes.push_back( Point3D( -offset + i, 0, 0.25 ) );
	nodes.push_back( Point3D( -offset + i + 1, 0, 0.25 ) );
	nodes.push_back( Point3D( -offset + i, 0, 1.25 ) );
	nodes.push_back( Point3D( -offset + i + 1, 0, 1.25 ) );

	normals.push_back( Point3D( 0, -1, 0 ) );

	texcoords.push_back( Point3D( 0, 0, 0 ) );
	texcoords.push_back( Point3D( 1, 0, 0 ) );
	texcoords.push_back( Point3D( 0, 1, 0 ) );
	texcoords.push_back( Point3D( 1, 1, 0 ) );

	vertex_index.push_back( 0 );
	vertex_index.push_back( 1 );
	vertex_index.push_back( 2 );
	vertex_index.push_back( 3 );

	normal_index.push_back( 0 );
	normal_index.push_back( 0 );
	normal_index.push_back( 0 );
	normal_index.push_back( 0 );

	tex_index.push_back( 0 );
	tex_index.push_back( 1 );
	tex_index.push_back( 2 );
	tex_index.push_back( 3 );

	ssgLeaf *leaf = gen_leaf( path, GL_TRIANGLE_STRIP, material,
				  nodes, normals, texcoords,
				  vertex_index, normal_index, tex_index,
				  false, NULL );

	object->addKid( leaf );
    }

    return object;
}


ssgBranch *gen_runway_sign( const string path, const string name ) {
    // for demo purposes we assume each element (letter) is 1x1 meter.
    // Sign is placed 0.25 meters above the ground

    ssgBranch *object = new ssgBranch();
    object->setName( (char *)name.c_str() );

    double width = name.length() / 3.0;

    string material = name + ".rgb";

    point_list nodes; nodes.clear();
    point_list normals; normals.clear();
    point_list texcoords; texcoords.clear();
    int_list vertex_index; vertex_index.clear();
    int_list normal_index; normal_index.clear();
    int_list tex_index; tex_index.clear();

    nodes.push_back( Point3D( -width, 0, 0.25 ) );
    nodes.push_back( Point3D( width + 1, 0, 0.25 ) );
    nodes.push_back( Point3D( -width, 0, 1.25 ) );
    nodes.push_back( Point3D( width + 1, 0, 1.25 ) );

    normals.push_back( Point3D( 0, -1, 0 ) );

    texcoords.push_back( Point3D( 0, 0, 0 ) );
    texcoords.push_back( Point3D( 1, 0, 0 ) );
    texcoords.push_back( Point3D( 0, 1, 0 ) );
    texcoords.push_back( Point3D( 1, 1, 0 ) );

    vertex_index.push_back( 0 );
    vertex_index.push_back( 1 );
    vertex_index.push_back( 2 );
    vertex_index.push_back( 3 );

    normal_index.push_back( 0 );
    normal_index.push_back( 0 );
    normal_index.push_back( 0 );
    normal_index.push_back( 0 );

    tex_index.push_back( 0 );
    tex_index.push_back( 1 );
    tex_index.push_back( 2 );
    tex_index.push_back( 3 );

    ssgLeaf *leaf = gen_leaf( path, GL_TRIANGLE_STRIP, material,
			      nodes, normals, texcoords,
			      vertex_index, normal_index, tex_index,
			      false, NULL );

    object->addKid( leaf );

    return object;
}
