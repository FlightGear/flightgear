// materialmgr.cxx -- class to handle material properties
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <simgear/xgl/xgl.h>

#include <simgear/compiler.h>

#include <string.h>
#include STL_STRING

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgpath.hxx>
#include <simgear/misc/fgstream.hxx>

#include <Include/general.hxx>
#include <Main/globals.hxx>
#include <Scenery/tileentry.hxx>

#include "matlib.hxx"

FG_USING_STD(string);


// global material management class
FGMaterialLib material_lib;


// Constructor
FGMaterialLib::FGMaterialLib ( void ) {
}


static bool local_file_exists( const string& path ) {
    fg_gzifstream in( path );
    if ( ! in.is_open() ) {
	return false;
    } else {
	return true;
    }

#if 0
    cout << path << " ";
    FILE *fp = fopen( path.c_str(), "r" );
    if ( fp == NULL ) {
	cout << " doesn't exist\n";
	return false;
    } else {
	cout << " exists\n";
	return true;
    }
#endif
}


// Load a library of material properties
bool FGMaterialLib::load( const string& mpath ) {
    string material_name;

    fg_gzifstream in( mpath );
    if ( ! in.is_open() ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Cannot open file: " << mpath );
	exit(-1);
    }

#ifndef __MWERKS__
    while ( ! in.eof() ) {
#else
    char c = '\0';
    while ( in.get(c) && c != '\0' ) {
	in.putback(c);
#endif
        // printf("%s", line);

	// strip leading white space and comments
	in >> skipcomment;

	// set to zero to prevent its value accidently being '{'
	// after a failed >> operation.
	char token = 0;

	in >> material_name >> token;

	if ( token == '{' ) {
	    FGNewMat m;
	    in >> m;

	    // build the ssgSimpleState
	    FGPath tex_path( globals->get_options()->get_fg_root() );
	    tex_path.append( "Textures.high" );

	    FGPath tmp_path = tex_path;
	    tmp_path.append( m.get_texture_name() );
	    if ( ! local_file_exists(tmp_path.str())
		 || general.get_glMaxTexSize() < 512 ) {
		tex_path = FGPath( globals->get_options()->get_fg_root() );
		tex_path.append( "Textures" );
	    }
	    
	    FG_LOG( FG_TERRAIN, FG_INFO, "  Loading material " 
		    << material_name << " (" << tex_path.c_str() << ")");

	    GLenum shade_model = GL_SMOOTH;
	    if ( globals->get_options()->get_shading() == 1 ) {
		shade_model = GL_SMOOTH;
	    } else {
		shade_model = GL_FLAT;
	    }

	    m.build_ssg_state( tex_path.str(), shade_model,
			       globals->get_options()->get_textures() );

#if EXTRA_DEBUG
	    m.dump_info();
#endif
	    
	    matlib[material_name] = m;
	}
    }

    return true;
}


// Load a library of material properties
bool FGMaterialLib::add_item ( const string &tex_path )
{
    string material_name = tex_path;
    int pos = tex_path.rfind( "/" );
    material_name = material_name.substr( pos + 1 );

    return add_item( material_name, tex_path );
}


// Load a library of material properties
bool FGMaterialLib::add_item ( const string &mat_name, const string &full_path )
{
    int pos = full_path.rfind( "/" );
    string tex_name = full_path.substr( pos + 1 );
    string tex_path = full_path.substr( 0, pos );

    FGNewMat m( mat_name, tex_name );

    FG_LOG( FG_TERRAIN, FG_INFO, "  Loading material " 
	    << mat_name << " (" << tex_path << ")");

#if EXTRA_DEBUG
    m.dump_info();
#endif

    GLenum shade_model = GL_SMOOTH;
    if ( globals->get_options()->get_shading() == 1 ) {
	shade_model = GL_SMOOTH;
    } else {
	shade_model = GL_FLAT;
    }

    m.build_ssg_state( tex_path, shade_model,
		       globals->get_options()->get_textures() );

    material_lib.matlib[mat_name] = m;

    return true;
}


// Load a library of material properties
bool FGMaterialLib::add_item ( const string &mat_name, ssgSimpleState *state )
{
    FGNewMat m( mat_name );
    m.set_ssg_state( state );

    FG_LOG( FG_TERRAIN, FG_INFO, "  Loading material given a premade "
	    << "ssgSimpleState = " << mat_name );

#if EXTRA_DEBUG
    m.dump_info();
#endif

    material_lib.matlib[mat_name] = m;

    return true;
}


// find a material record by material name
FGNewMat *FGMaterialLib::find( const string& material ) {
    FGNewMat *result = NULL;
    material_map_iterator it = matlib.find( material );
    if ( it != end() ) {
	result = &((*it).second);
	return result;
    }

    return NULL;
}


// Destructor
FGMaterialLib::~FGMaterialLib ( void ) {
}


// Set the step for all of the state selectors in the material slots
void FGMaterialLib::set_step ( int step )
{
    // container::iterator it = begin();
    for ( material_map_iterator it = begin(); it != end(); it++ ) {
	const string &key = it->first;
	FG_LOG( FG_GENERAL, FG_INFO,
		"Updating material " << key << " to step " << step );
	FGNewMat &slot = it->second;
	slot.get_state()->selectStep(step);
    }
}
