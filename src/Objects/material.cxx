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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Include/compiler.h>

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include STL_STRING

#include <Debug/logstream.hxx>
#include <Misc/fgpath.hxx>
#include <Misc/fgstream.hxx>

#include "material.hxx"
#include "texload.h"

FG_USING_STD(string);


// Constructor
FGMaterial::FGMaterial ( void )
    : loaded(false),
      texture_name(""),
      alpha(0)
    // , list_size(0)
{
    ambient[0]  = ambient[1]  = ambient[2]  = ambient[3]  = 0.0;
    diffuse[0]  = diffuse[1]  = diffuse[2]  = diffuse[3]  = 0.0;
    specular[0] = specular[1] = specular[2] = specular[3] = 0.0;
    emission[0] = emission[1] = emission[2] = emission[3] = 0.0;
}


istream&
operator >> ( istream& in, FGMaterial& m )
{
    string token;

    for (;;) {
	in >> token;
	if ( token == "texture" ) {
	    in >> token >> m.texture_name;
	} else if ( token == "xsize" ) {
	    in >> token >> m.xsize;
	} else if ( token == "ysize" ) {
	    in >> token >> m.ysize;
	} else if ( token == "ambient" ) {
	    in >> token >> m.ambient[0] >> m.ambient[1]
	       >> m.ambient[2] >> m.ambient[3];
	} else if ( token == "diffuse" ) {
	    in >> token >> m.diffuse[0] >> m.diffuse[1]
	       >> m.diffuse[2] >> m.diffuse[3];
	} else if ( token == "specular" ) {
	    in >> token >> m.specular[0] >> m.specular[1]
	       >> m.specular[2] >> m.specular[3];
	} else if ( token == "emission" ) {
	    in >> token >> m.emission[0] >> m.emission[1]
	       >> m.emission[2] >> m.emission[3];
	} else if ( token == "alpha" ) {
	    in >> token >> token;
	    if ( token == "yes" ) {
		m.alpha = 1;
	    } else if ( token == "no" ) {
		m.alpha = 0;
	    } else {
		FG_LOG( FG_TERRAIN, FG_INFO, "Bad alpha value " << token );
	    }
	} else if ( token[0] == '}' ) {
	    break;
	}
    }

    return in;
}


void
FGMaterial::load_texture( const string& root )
{
    GLubyte *texbuf;
    int width, height;

    FG_LOG( FG_TERRAIN, FG_INFO,
	    "  Loading texture for material " << texture_name );

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

    // set the texture parameters back to the defaults for loading
    // this texture
    xglPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    xglPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    xglPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    xglPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ) ;
    xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ) ;
    xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    xglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
		      /* GL_LINEAR */ 
		      /* GL_NEAREST_MIPMAP_LINEAR */
		      GL_LINEAR_MIPMAP_LINEAR ) ;

    // load in the texture data
    FGPath base_path( root );
    base_path.append( "Textures" );
    base_path.append( texture_name );

    FGPath tpath = base_path;
    tpath.concat( ".rgb" );

    FGPath fg_tpath = tpath;
    fg_tpath.concat( ".gz" );

    FGPath fg_raw_tpath = base_path;
    fg_raw_tpath.concat( ".raw" );

    // create string names for msfs compatible textures
    FGPath fg_r8_tpath = base_path;
    fg_r8_tpath.concat( ".r8" );

    FGPath fg_tex_tpath = base_path;
    fg_tex_tpath.concat( ".txt" );

    FGPath fg_pat_tpath = base_path;
    fg_pat_tpath.concat( ".pat" );

    FGPath fg_oav_tpath = base_path;
    fg_oav_tpath.concat( ".oav" );

    if ( alpha == 0 ) {
	// load rgb texture

	// Try uncompressed
	if ( (texbuf = 
	      read_rgb_texture(tpath.c_str(), &width, &height)) != 
	     NULL )
	    ;
	// Try compressed
	else if ( (texbuf = 
		   read_rgb_texture(fg_tpath.c_str(), &width, &height)) 
		  != NULL )
	    ;
	// Try raw
	else if ( (texbuf = 
		   read_raw_texture(fg_raw_tpath.c_str(), &width, &height)) 
		  != NULL )
	    ;
	// Try r8
	else if ( (texbuf =
		   read_r8_texture(fg_r8_tpath.c_str(), &width, &height)) 
		  != NULL )
	    ;
	// Try tex
	else if ( (texbuf =
		   read_r8_texture(fg_tex_tpath.c_str(), &width, &height)) 
		  != NULL )
	    ;
	// Try pat
	else if ( (texbuf =
		   read_r8_texture(fg_pat_tpath.c_str(), &width, &height)) 
		  != NULL )
	    ;
	// Try oav
	else if ( (texbuf =
		   read_r8_texture(fg_oav_tpath.c_str(), &width, &height)) 
		  != NULL )
	    ;
	else {
	    FG_LOG( FG_GENERAL, FG_ALERT, 
		    "Error in loading texture " << tpath.str() );
	    exit(-1);
	} 

	if ( gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, width, height, 
				GL_RGB, GL_UNSIGNED_BYTE, texbuf ) != 0 )
	{
	    FG_LOG( FG_GENERAL, FG_ALERT, "Error building mipmaps");
	    exit(-1);
	}
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
		FG_LOG( FG_GENERAL, FG_ALERT, 
			"Error in loading texture " << tpath.str() );
		exit(-1);
	    } 
	} 

	xglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
		      GL_RGBA, GL_UNSIGNED_BYTE, texbuf);
    }

    loaded = true;
}


// Destructor
void FGMaterial::dump_info () {
    FG_LOG( FG_TERRAIN, FG_INFO, "{" << endl << "  texture = " 
	    << texture_name );
    FG_LOG( FG_TERRAIN, FG_INFO, "  xsize = " << xsize );
    FG_LOG( FG_TERRAIN, FG_INFO, "  ysize = " << ysize );
    FG_LOG( FG_TERRAIN, FG_INFO, "  ambient = " << ambient[0] << " "
	    << ambient[1] <<" "<< ambient[2] <<" "<< ambient[3] );
    FG_LOG( FG_TERRAIN, FG_INFO, "  diffuse = " << diffuse[0] << " " 
	    << diffuse[1] << " " << diffuse[2] << " " << diffuse[3] );
    FG_LOG( FG_TERRAIN, FG_INFO, "  specular = " << specular[0] << " " 
	    << specular[1] << " " << specular[2] << " " << specular[3]);
    FG_LOG( FG_TERRAIN, FG_INFO, "  emission = " << emission[0] << " " 
	    << emission[1] << " " << emission[2] << " " << emission[3]);
    FG_LOG( FG_TERRAIN, FG_INFO, "  alpha = " << alpha << endl <<"}" );
	    
}


// Destructor
FGMaterial::~FGMaterial ( void ) {
}
