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
#include <XGL/xgl.h>

#include <Include/compiler.h>

#include <string.h>
#include STL_STRING

#include <Debug/logstream.hxx>
#include <Misc/fgpath.hxx>
#include <Misc/fgstream.hxx>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Scenery/tileentry.hxx>

#include "materialmgr.hxx"
#include "fragment.hxx"

FG_USING_STD(string);


// global material management class
fgMATERIAL_MGR material_mgr;


// Constructor
FGMaterialSlot::FGMaterialSlot ( void ) { }


// Destructor
FGMaterialSlot::~FGMaterialSlot ( void ) {
}


// Constructor
fgMATERIAL_MGR::fgMATERIAL_MGR ( void ) {
    materials_loaded = false;
}


void
FGMaterialSlot::render_fragments()
{
    int tris_rendered = current_view.get_tris_rendered();

    // cout << "rendering " + texture_name + " = " << list_size << "\n";

    if ( empty() ) {
	return;
    }

    if ( current_options.get_textures() ) {

	if ( !m.is_loaded() ) {
	    m.load_texture( current_options.get_fg_root() );

	    // build the ssgSimpleState
	    GLuint tex_id = m.get_texture_id();
	    state.setTexture( tex_id );
	    state.enable( GL_TEXTURE_2D );
	    state.enable( GL_LIGHTING );
	    state.setShadeModel( GL_SMOOTH );
	    state.enable ( GL_CULL_FACE      ) ;
	    state.setMaterial ( GL_AMBIENT_AND_DIFFUSE, 1, 1, 1, 1 ) ;
	}

#ifdef GL_VERSION_1_1
	xglBindTexture( GL_TEXTURE_2D, m.get_texture_id() );
#elif GL_EXT_texture_object
	xglBindTextureEXT( GL_TEXTURE_2D, m.get_texture_id() );
#else
#  error port me
#endif
    } else {
	xglMaterialfv (GL_FRONT, GL_AMBIENT, m.get_ambient() );
	xglMaterialfv (GL_FRONT, GL_DIFFUSE, m.get_diffuse() );
    }

    FGTileEntry* last_tile_ptr = NULL;
    frag_list_iterator current = list.begin();
    frag_list_iterator last = list.end();

    for ( ; current != last; ++current ) {
	fgFRAGMENT* frag_ptr = *current;
	tris_rendered += frag_ptr->num_faces();
	if ( frag_ptr->tile_ptr != last_tile_ptr ) {
	    // new tile, new translate
	    last_tile_ptr = frag_ptr->tile_ptr;
	    xglLoadMatrixf( frag_ptr->tile_ptr->model_view );
	}

	// Woohoo!!!  We finally get to draw something!
	// printf("  display_list = %d\n", frag_ptr->display_list);
	xglCallList( frag_ptr->display_list );
    }

    current_view.set_tris_rendered( tris_rendered );
}


// Load a library of material properties
int
fgMATERIAL_MGR::load_lib ( void )
{
    string material_name;

    // build the path name to the material db
    FGPath mpath( current_options.get_fg_root() );
    mpath.append( "materials" );

    fg_gzifstream in( mpath.str() );
    if ( ! in ) {
	FG_LOG( FG_GENERAL, FG_ALERT, "Cannot open file: " << mpath.str() );
	exit(-1);
    }

    while ( ! in.eof() ) {
        // printf("%s", line);

	// strip leading white space and comments
	in >> skipcomment;

	// set to zero to prevent its value accidently being '{'
	// after a failed >> operation.
	char token = 0;

	in >> material_name >> token;

	if ( token == '{' ) {
	    FG_LOG( FG_TERRAIN, FG_INFO,
		    "  Loading material " << material_name );
	    FGMaterial m;
	    in >> m;

	    // if ( current_options.get_textures() ) {
	    //    m.load_texture( current_options.get_fg_root() );
	    // }

	    FGMaterialSlot m_slot;
	    m_slot.set_m( m );
	    material_mgr.material_map[material_name] = m_slot;
	}
    }

    if ( current_options.get_textures() ) {
	materials_loaded = true;
    }

    return(1);
}


// Initialize the transient list of fragments for each material property
void
fgMATERIAL_MGR::init_transient_material_lists( void )
{
    iterator last = end();
    for ( iterator it = begin(); it != last; ++it ) {
	(*it).second.init_sort_list();
    }
}


bool
fgMATERIAL_MGR::find( const string& material, FGMaterialSlot*& mtl_ptr )
{
    iterator it = material_map.find( material );
    if ( it != end() ) {
	mtl_ptr = &((*it).second);
	return true;
    }

    return false;
}


// Destructor
fgMATERIAL_MGR::~fgMATERIAL_MGR ( void ) {
}


void
fgMATERIAL_MGR::render_fragments()
{
    current_view.set_tris_rendered( 0 );

    iterator last = end();
    for ( iterator current = begin(); current != last; ++current ) {
	(*current).second.render_fragments();
    }
}


