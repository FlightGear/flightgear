// newmat.cxx -- class to handle material properties
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2000  Curtis L. Olson  - curt@flightgear.org
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

#include <simgear/compiler.h>

#ifdef FG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgpath.hxx>
#include <simgear/misc/fgstream.hxx>

#include "newmat.hxx"


// Constructor
FGNewMat::FGNewMat ( void ) {
}


// Constructor
FGNewMat::FGNewMat ( const string &name )
{
    FGNewMat( name, name );
}


// Constructor
FGNewMat::FGNewMat ( const string &mat_name, const string &tex_name )
{
    material_name = mat_name;
    texture_name = tex_name;
    xsize = ysize = 0;
    alpha = 0; 
    ambient[0]  = ambient[1]  = ambient[2]  = ambient[3]  = 1.0;
    diffuse[0]  = diffuse[1]  = diffuse[2]  = diffuse[3]  = 1.0;
    specular[0] = specular[1] = specular[2] = specular[3] = 1.0;
    emission[0] = emission[1] = emission[2] = emission[3] = 1.0;
}


void FGNewMat::build_ssg_state( const string& path,
				GLenum shade_model, bool texture_default )
{
    FGPath tex_file( path );
    tex_file.append( texture_name );

    state = new ssgStateSelector(2);
    textured = new ssgSimpleState();
    nontextured = new ssgSimpleState();

    // Set up the textured state
    textured->setShadeModel( shade_model );
    textured->enable ( GL_CULL_FACE ) ;
    textured->enable( GL_TEXTURE_2D );
    textured->disable( GL_BLEND );
    textured->disable( GL_ALPHA_TEST );
    textured->setTexture( (char *)tex_file.c_str() );
    textured->enable( GL_COLOR_MATERIAL );
    textured->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    textured->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    textured->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );

    // Set up the coloured state
    nontextured->enable( GL_LIGHTING );
    nontextured->setShadeModel( shade_model );
    nontextured->enable ( GL_CULL_FACE      ) ;
    nontextured->disable( GL_TEXTURE_2D );
    nontextured->disable( GL_BLEND );
    nontextured->disable( GL_ALPHA_TEST );
    nontextured->disable( GL_COLOR_MATERIAL );

    /* cout << "ambient = " << ambient[0] << "," << ambient[1] 
       << "," << ambient[2] << endl; */
    nontextured->setMaterial ( GL_AMBIENT, 
			       ambient[0], ambient[1], 
			       ambient[2], ambient[3] ) ;
    nontextured->setMaterial ( GL_DIFFUSE, 
			       diffuse[0], diffuse[1], 
			       diffuse[2], diffuse[3] ) ;
    nontextured->setMaterial ( GL_SPECULAR, 
			       specular[0], specular[1], 
			       specular[2], specular[3] ) ;
    nontextured->setMaterial ( GL_EMISSION, 
			       emission[0], emission[1], 
			       emission[2], emission[3] ) ;

    state->setStep( 0, textured );    // textured
    state->setStep( 1, nontextured ); // untextured

    // Choose the appropriate starting state.
    if ( texture_default ) {
	state->selectStep(0);
    } else {
	state->selectStep(1);
    }
}


void FGNewMat::set_ssg_state( ssgSimpleState *s ) {
    state = new ssgStateSelector(2);
    textured = s;
    nontextured = new ssgSimpleState();

    // Set up the coloured state
    nontextured->enable( GL_LIGHTING );
    nontextured->setShadeModel( GL_FLAT );
    nontextured->enable ( GL_CULL_FACE      ) ;
    nontextured->disable( GL_TEXTURE_2D );
    nontextured->disable( GL_BLEND );
    nontextured->disable( GL_ALPHA_TEST );
    nontextured->disable( GL_COLOR_MATERIAL );

    /* cout << "ambient = " << ambient[0] << "," << ambient[1] 
       << "," << ambient[2] << endl; */
    nontextured->setMaterial ( GL_AMBIENT, 
			       ambient[0], ambient[1], 
			       ambient[2], ambient[3] ) ;
    nontextured->setMaterial ( GL_DIFFUSE, 
			       diffuse[0], diffuse[1], 
			       diffuse[2], diffuse[3] ) ;
    nontextured->setMaterial ( GL_SPECULAR, 
			       specular[0], specular[1], 
			       specular[2], specular[3] ) ;
    nontextured->setMaterial ( GL_EMISSION, 
			       emission[0], emission[1], 
			       emission[2], emission[3] ) ;

    state->setStep( 0, textured );    // textured
    state->setStep( 1, nontextured ); // untextured

    // Choose the appropriate starting state.
    state->selectStep(0);
}


void FGNewMat::dump_info () {
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
FGNewMat::~FGNewMat ( void ) {
}


istream&
operator >> ( istream& in, FGNewMat& m )
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
