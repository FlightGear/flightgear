// material.cxx -- class to handle material properties
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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <string.h>
#include <string>

#include "Include/fg_stl_config.h"
#include <Debug/fg_debug.h>
#include <Main/options.hxx>
#include <Misc/fgstream.hxx>
#include <Main/views.hxx>
#include <Scenery/tile.hxx>

#include "material.hxx"
#include "fragment.hxx"
#include "texload.h"


// global material management class
fgMATERIAL_MGR material_mgr;


// Constructor
fgMATERIAL::fgMATERIAL ( void )
    : texture_name(""),
      alpha(0),
      list_size(0)
{
    ambient[0]  = ambient[1]  = ambient[2]  = ambient[3]  = 0.0;
    diffuse[0]  = diffuse[1]  = diffuse[2]  = diffuse[3]  = 0.0;
    specular[0] = specular[1] = specular[2] = specular[3] = 0.0;
    emissive[0] = emissive[1] = emissive[2] = emissive[3] = 0.0;
}


int
fgMATERIAL::append_sort_list( fgFRAGMENT *object )
{
    if ( list_size < FG_MAX_MATERIAL_FRAGS ) {
	list[ list_size++ ] = object;
	return 1;
    } else {
	return 0;
    }
}

istream&
operator >> ( istream& in, fgMATERIAL& m )
{
    string token;

    for (;;) {
	in >> token;
	if ( token == "texture" )
	{
	    in >> token >> m.texture_name;
	}
	else if ( token == "ambient" )
	{
	    in >> token >> m.ambient[0] >> m.ambient[1]
			>> m.ambient[2] >> m.ambient[3];
	}
	else if ( token == "diffuse" )
	{
	    in >> token >> m.diffuse[0] >> m.diffuse[1]
			>> m.diffuse[2] >> m.diffuse[3];
	}
	else if ( token == "specular" )
	{
	    in >> token >> m.specular[0] >> m.specular[1]
			>> m.specular[2] >> m.specular[3];
	}
	else if ( token == "emissive" )
	{
	    in >> token >> m.emissive[0] >> m.emissive[1]
			>> m.emissive[2] >> m.emissive[3];
	}
	else if ( token == "alpha" )
	{
	    in >> token >> token;
	    if ( token == "yes" )
		m.alpha = 1;
	    else if ( token == "no" )
		m.alpha = 0;
	    else
	    {
		fgPrintf( FG_TERRAIN, FG_INFO,
			  "Bad alpha value '%s'\n", token.c_str() );
	    }
	}
	else if ( token[0] == '}' )
	{
	    break;
	}
    }

    return in;
}

void
fgMATERIAL::load_texture()
{
	GLubyte *texbuf;
	int width, height;

	// create the texture object and bind it
#ifdef GL_VERSION_1_1
	xglGenTextures(1, &texture_id );
	xglBindTexture(GL_TEXTURE_2D, texture_id );
#elif GL_EXT_texture_object
	xglGenTexturesEXT(1, &texture_id );
	xglBindTextureEXT(GL_TEXTURE_2D, texture_id );
#else
#  error port me
#endif

	// set the texture parameters for this texture
	xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ) ;
	xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ) ;
	xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
			  GL_LINEAR );
	// xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
	//                   GL_NEAREST_MIPMAP_NEAREST );
	xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
			  /* GL_LINEAR */ 
			  /* GL_NEAREST_MIPMAP_LINEAR */
			  GL_LINEAR_MIPMAP_LINEAR ) ;

	/* load in the texture data */
	string tpath = current_options.get_fg_root() + "/Textures/" + 
	    texture_name + ".rgb";
	string fg_tpath = tpath + ".gz";

	if ( alpha == 0 ) {
	    // load rgb texture

	    // Try uncompressed
	    if ( (texbuf = 
		  read_rgb_texture(tpath.c_str(), &width, &height)) == 
		 NULL )
	    {
		// Try compressed
		if ( (texbuf = 
		      read_rgb_texture(fg_tpath.c_str(), &width, &height)) 
		     == NULL )
		{
		    fgPrintf( FG_GENERAL, FG_EXIT, 
			      "Error in loading texture %s\n", 
			      tpath.c_str() );
		    return;
		} 
	    } 

	    /* xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
	       GL_RGB, GL_UNSIGNED_BYTE, texbuf); */

	    gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, width, height, 
			       GL_RGB, GL_UNSIGNED_BYTE, texbuf );
	} else if ( alpha == 1 ) {
	    // load rgba (alpha) texture

	    // Try uncompressed
	    if ( (texbuf = 
		  read_alpha_texture(tpath.c_str(), &width, &height))
		 == NULL )
	    {
		// Try compressed
		if ((texbuf = 
		     read_alpha_texture(fg_tpath.c_str(), &width, &height))
		    == NULL )
		{
		    fgPrintf( FG_GENERAL, FG_EXIT, 
			      "Error in loading texture %s\n",
			      tpath.c_str() );
		    return;
		} 
	    } 

	    xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			  GL_RGBA, GL_UNSIGNED_BYTE, texbuf);
	}
}


// Destructor
fgMATERIAL::~fgMATERIAL ( void ) {
}


// Constructor
fgMATERIAL_MGR::fgMATERIAL_MGR ( void ) {
    textures_loaded = false;
}


void
fgMATERIAL::render_fragments()
{
    // cout << "rendering " + texture_name + " = " << list_size << "\n";

    if ( empty() )
	return;

    if ( current_options.get_textures() )
    {
#ifdef GL_VERSION_1_1
	xglBindTexture(GL_TEXTURE_2D, texture_id);
#elif GL_EXT_texture_object
	xglBindTextureEXT(GL_TEXTURE_2D, texture_id);
#else
#  error port me
#endif
    } else {
	xglMaterialfv (GL_FRONT, GL_AMBIENT, ambient);
	xglMaterialfv (GL_FRONT, GL_DIFFUSE, diffuse);
    }

    fgTILE* last_tile_ptr = NULL;
    for ( size_t i = 0; i < list_size; ++i )
    {
	fgFRAGMENT* frag_ptr = list[i];
	current_view.tris_rendered += frag_ptr->num_faces();
	if ( frag_ptr->tile_ptr != last_tile_ptr )
	{
	    // new tile, new translate
	    last_tile_ptr = frag_ptr->tile_ptr;
	    xglLoadMatrixd( frag_ptr->tile_ptr->model_view );
	}

	// Woohoo!!!  We finally get to draw something!
	// printf("  display_list = %d\n", frag_ptr->display_list);
	xglCallList( frag_ptr->display_list );
    }
}


// Load a library of material properties
int
fgMATERIAL_MGR::load_lib ( void )
{
    string material_name;

    // build the path name to the material db
    string mpath = current_options.get_fg_root() + "/materials";
    fg_gzifstream in( mpath );
    if ( ! in )
	fgPrintf( FG_GENERAL, FG_EXIT, "Cannot open file: %s\n", 
		  mpath.c_str() );

    while ( ! in.eof() ) {
        // printf("%s", line);

	// strip leading white space and comments
	in.eat_comments();

	// set to zero to prevent its value accidently being '{'
	// after a failed >> operation.
	char token = 0;

	in.stream() >> material_name >> token;

	if ( token == '{' ) {
	    printf( "  Loading material %s\n", material_name.c_str() );
	    fgMATERIAL m;
	    in.stream() >> m;

	    if ( current_options.get_textures() ) {
		m.load_texture();
	    }

	    material_mgr.material_map[material_name] = m;
	}
    }

    if ( current_options.get_textures() ) {
	textures_loaded = true;
    }

    return(1);
}


// Initialize the transient list of fragments for each material property
void
fgMATERIAL_MGR::init_transient_material_lists( void )
{
    iterator last = end();
    for ( iterator it = begin(); it != last; ++it )
    {
	(*it).second.init_sort_list();
    }
}


bool
fgMATERIAL_MGR::find( const string& material, fgMATERIAL*& mtl_ptr )
{
    iterator it = material_map.find( material );
    if ( it != end() )
    {
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
    current_view.tris_rendered = 0;
    iterator last = end();
    for ( iterator current = begin(); current != last; ++current )
	(*current).second.render_fragments();
}


// $Log$
// Revision 1.7  1998/09/17 18:35:52  curt
// Tweaks and optimizations by Norman Vine.
//
// Revision 1.6  1998/09/15 01:35:05  curt
// cleaned up my fragment.num_faces hack :-) to use the STL (no need in
// duplicating work.)
// Tweaked fgTileMgrRender() do not calc tile matrix unless necessary.
// removed some unneeded stuff from fgTileMgrCurElev()
//
// Revision 1.5  1998/09/10 19:07:11  curt
// /Simulator/Objects/fragment.hxx
//   Nested fgFACE inside fgFRAGMENT since its not used anywhere else.
//
// ./Simulator/Objects/material.cxx
// ./Simulator/Objects/material.hxx
//   Made fgMATERIAL and fgMATERIAL_MGR bona fide classes with private
//   data members - that should keep the rabble happy :)
//
// ./Simulator/Scenery/tilemgr.cxx
//   In viewable() delay evaluation of eye[0] and eye[1] in until they're
//   actually needed.
//   Change to fgTileMgrRender() to call fgMATERIAL_MGR::render_fragments()
//   method.
//
// ./Include/fg_stl_config.h
// ./Include/auto_ptr.hxx
//   Added support for g++ 2.7.
//   Further changes to other files are forthcoming.
//
// Brief summary of changes required for g++ 2.7.
//   operator->() not supported by iterators: use (*i).x instead of i->x
//   default template arguments not supported,
//   <functional> doesn't have mem_fun_ref() needed by callbacks.
//   some std include files have different names.
//   template member functions not supported.
//
// Revision 1.4  1998/09/01 19:03:08  curt
// Changes contributed by Bernie Bright <bbright@c031.aone.net.au>
//  - The new classes in libmisc.tgz define a stream interface into zlib.
//    I've put these in a new directory, Lib/Misc.  Feel free to rename it
//    to something more appropriate.  However you'll have to change the
//    include directives in all the other files.  Additionally you'll have
//    add the library to Lib/Makefile.am and Simulator/Main/Makefile.am.
//
//    The StopWatch class in Lib/Misc requires a HAVE_GETRUSAGE autoconf
//    test so I've included the required changes in config.tgz.
//
//    There are a fair few changes to Simulator/Objects as I've moved
//    things around.  Loading tiles is quicker but thats not where the delay
//    is.  Tile loading takes a few tenths of a second per file on a P200
//    but it seems to be the post-processing that leads to a noticeable
//    blip in framerate.  I suppose its time to start profiling to see where
//    the delays are.
//
//    I've included a brief description of each archives contents.
//
// Lib/Misc/
//   zfstream.cxx
//   zfstream.hxx
//     C++ stream interface into zlib.
//     Taken from zlib-1.1.3/contrib/iostream/.
//     Minor mods for STL compatibility.
//     There's no copyright associated with these so I assume they're
//     covered by zlib's.
//
//   fgstream.cxx
//   fgstream.hxx
//     FlightGear input stream using gz_ifstream.  Tries to open the
//     given filename.  If that fails then filename is examined and a
//     ".gz" suffix is removed or appended and that file is opened.
//
//   stopwatch.hxx
//     A simple timer for benchmarking.  Not used in production code.
//     Taken from the Blitz++ project.  Covered by GPL.
//
//   strutils.cxx
//   strutils.hxx
//     Some simple string manipulation routines.
//
// Simulator/Airports/
//   Load airports database using fgstream.
//   Changed fgAIRPORTS to use set<> instead of map<>.
//   Added bool fgAIRPORTS::search() as a neater way doing the lookup.
//   Returns true if found.
//
// Simulator/Astro/
//   Modified fgStarsInit() to load stars database using fgstream.
//
// Simulator/Objects/
//   Modified fgObjLoad() to use fgstream.
//   Modified fgMATERIAL_MGR::load_lib() to use fgstream.
//   Many changes to fgMATERIAL.
//   Some changes to fgFRAGMENT but I forget what!
//
// Revision 1.3  1998/08/27 17:02:09  curt
// Contributions from Bernie Bright <bbright@c031.aone.net.au>
// - use strings for fg_root and airport_id and added methods to return
//   them as strings,
// - inlined all access methods,
// - made the parsing functions private methods,
// - deleted some unused functions.
// - propogated some of these changes out a bit further.
//
// Revision 1.2  1998/08/25 20:53:33  curt
// Shuffled $FG_ROOT file layout.
//
// Revision 1.1  1998/08/25 16:51:24  curt
// Moved from ../Scenery
//
// Revision 1.13  1998/08/24 20:11:39  curt
// Tweaks ...
//
// Revision 1.12  1998/08/12 21:41:27  curt
// Need to negate the test for textures so that textures aren't loaded when
// they are disabled rather than visa versa ... :-)
//
// Revision 1.11  1998/08/12 21:13:03  curt
// material.cxx: don't load textures if they are disabled
// obj.cxx: optimizations from Norman Vine
// tile.cxx: minor tweaks
// tile.hxx: addition of num_faces
// tilemgr.cxx: minor tweaks
//
// Revision 1.10  1998/07/24 21:42:06  curt
// material.cxx: whups, double method declaration with no definition.
// obj.cxx: tweaks to avoid errors in SGI's CC.
// tile.cxx: optimizations by Norman Vine.
// tilemgr.cxx: optimizations by Norman Vine.
//
// Revision 1.9  1998/07/13 21:01:57  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.8  1998/07/08 14:47:20  curt
// Fix GL_MODULATE vs. GL_DECAL problem introduced by splash screen.
// polare3d.h renamed to polar3d.hxx
// fg{Cartesian,Polar}Point3d consolodated.
// Added some initial support for calculating local current ground elevation.
//
// Revision 1.7  1998/07/04 00:54:28  curt
// Added automatic mipmap generation.
//
// When rendering fragments, use saved model view matrix from associated tile
// rather than recalculating it with push() translate() pop().
//
// Revision 1.6  1998/06/27 16:54:59  curt
// Check for GL_VERSION_1_1 or GL_EXT_texture_object to decide whether to use
//   "EXT" versions of texture management routines.
//
// Revision 1.5  1998/06/17 21:36:39  curt
// Load and manage multiple textures defined in the Materials library.
// Boost max material fagments for each material property to 800.
// Multiple texture support when rendering.
//
// Revision 1.4  1998/06/12 00:58:04  curt
// Build only static libraries.
// Declare memmove/memset for Sloaris.
//
// Revision 1.3  1998/06/05 22:39:53  curt
// Working on sorting by, and rendering by material properties.
//
// Revision 1.2  1998/06/01 17:56:20  curt
// Incremental additions to material.cxx (not fully functional)
// Tweaked vfc_ratio math to avoid divide by zero.
//
// Revision 1.1  1998/05/30 01:56:45  curt
// Added material.cxx material.hxx
//
