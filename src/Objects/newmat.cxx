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

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "newmat.hxx"



////////////////////////////////////////////////////////////////////////
// Local static functions.
////////////////////////////////////////////////////////////////////////

/**
 * Internal method to test whether a file exists.
 *
 * TODO: this should be moved to a SimGear library of local file
 * functions.
 */
static inline bool
local_file_exists( const string& path ) {
    sg_gzifstream in( path );
    if ( ! in.is_open() ) {
	return false;
    } else {
	return true;
    }
}



////////////////////////////////////////////////////////////////////////
// Constructors and destructor.
////////////////////////////////////////////////////////////////////////


FGNewMat::FGNewMat (const SGPropertyNode * props)
{
    init();
    read_properties(props);
    build_ssg_state(false);
}

FGNewMat::FGNewMat (const string &texture_path)
{
    init();
    build_ssg_state(true);
}

FGNewMat::FGNewMat (ssgSimpleState * s)
{
    init();
    set_ssg_state(s);
}

FGNewMat::~FGNewMat (void)
{
}



////////////////////////////////////////////////////////////////////////
// Public methods.
////////////////////////////////////////////////////////////////////////

void
FGNewMat::read_properties (const SGPropertyNode * props)
{
				// Get the path to the texture
  string tname = props->getStringValue("texture", "unknown.rgb");
  SGPath tpath(globals->get_fg_root());
  tpath.append("Textures.high");
  tpath.append(tname);
  if (!local_file_exists(tpath.str())) {
    tpath = SGPath(globals->get_fg_root());
    tpath.append("Textures");
    tpath.append(tname);
  }
  texture_path = tpath.str();

  xsize = props->getDoubleValue("xsize", 0.0);
  ysize = props->getDoubleValue("ysize", 0.0);
  wrapu = props->getBoolValue("wrapu", true);
  wrapv = props->getBoolValue("wrapv", true);
  mipmap = props->getBoolValue("mipmap", true);
  light_coverage = props->getDoubleValue("light-coverage");

  ambient[0] = props->getDoubleValue("ambient/r", 0.0);
  ambient[1] = props->getDoubleValue("ambient/g", 0.0);
  ambient[2] = props->getDoubleValue("ambient/b", 0.0);
  ambient[3] = props->getDoubleValue("ambient/a", 0.0);

  diffuse[0] = props->getDoubleValue("diffuse/r", 0.0);
  diffuse[1] = props->getDoubleValue("diffuse/g", 0.0);
  diffuse[2] = props->getDoubleValue("diffuse/b", 0.0);
  diffuse[3] = props->getDoubleValue("diffuse/a", 0.0);

  specular[0] = props->getDoubleValue("specular/r", 0.0);
  specular[1] = props->getDoubleValue("specular/g", 0.0);
  specular[2] = props->getDoubleValue("specular/b", 0.0);
  specular[3] = props->getDoubleValue("specular/a", 0.0);

  emission[0] = props->getDoubleValue("emissive/r", 0.0);
  emission[1] = props->getDoubleValue("emissive/g", 0.0);
  emission[2] = props->getDoubleValue("emissive/b", 0.0);
  emission[3] = props->getDoubleValue("emissive/a", 0.0);
}



////////////////////////////////////////////////////////////////////////
// Private methods.
////////////////////////////////////////////////////////////////////////

void 
FGNewMat::init ()
{
  texture_path = "";
  state = 0;
  textured = 0;
  nontextured = 0;
  xsize = 0;
  ysize = 0;
  wrapu = true;
  wrapv = true;
  mipmap = true;
  texture_loaded = false;
  refcount = 0;
  for (int i = 0; i < 4; i++)
    ambient[i] = diffuse[i] = specular[i] = emission[i] = 0.0;
}

bool
FGNewMat::load_texture ()
{
  if (texture_loaded) {
    return false;
  } else {
    SG_LOG( SG_GENERAL, SG_INFO, "Loading deferred texture " << texture_path );
    textured->setTexture((char *)texture_path.c_str(), wrapu, wrapv, mipmap );
    texture_loaded = true;
    return true;
  }
}


void 
FGNewMat::build_ssg_state (bool defer_tex_load)
{
    GLenum shade_model =
      (fgGetBool("/sim/rendering/shading") ? GL_SMOOTH : GL_FLAT);
    bool texture_default = fgGetBool("/sim/rendering/textures");

    state = new ssgStateSelector(2);
    state->ref();

    textured = new ssgSimpleState();
    textured->ref();

    nontextured = new ssgSimpleState();
    nontextured->ref();

    // Set up the textured state
    textured->setShadeModel( shade_model );
    textured->enable( GL_LIGHTING );
    textured->enable ( GL_CULL_FACE ) ;
    textured->enable( GL_TEXTURE_2D );
    textured->disable( GL_BLEND );
    textured->disable( GL_ALPHA_TEST );
    if ( !defer_tex_load ) {
	textured->setTexture( (char *)texture_path.c_str(), wrapu, wrapv );
	texture_loaded = true;
    } else {
	texture_loaded = false;
    }
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


void FGNewMat::set_ssg_state( ssgSimpleState *s )
{
    state = new ssgStateSelector(2);
    state->ref();

    textured = s;

    nontextured = new ssgSimpleState();
    nontextured->ref();

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

// end of newmat.cxx
