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

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <GL/gl.h>

#include <simgear/compiler.h>
#include <simgear/misc/exception.hxx>

#include <string.h>
#include STL_STRING

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>

#include <Include/general.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/tileentry.hxx>

#include "matlib.hxx"

SG_USING_STD(string);


// global material management class
FGMaterialLib material_lib;


// Constructor
FGMaterialLib::FGMaterialLib ( void ) {
  set_step(0);
}


// Load a library of material properties
bool FGMaterialLib::load( const string& mpath ) {

  SGPropertyNode materials;

  cout << "Reading materials from " << mpath << endl;
  try {
    readProperties(mpath, &materials);
  } catch (const sg_exception &ex) {
    SG_LOG(SG_INPUT, SG_ALERT, "Error reading materials: " << ex.getMessage());
    throw ex;
  }

  int nMaterials = materials.nChildren();
  for (int i = 0; i < nMaterials; i++) {
    const SGPropertyNode * node = materials.getChild(i);
    if (node->getName() == "material") {
      FGNewMat * m = new FGNewMat(node);

      vector<const SGPropertyNode *>names = node->getChildren("name");
      for (int j = 0; j < names.size(); j++) {
	m->ref();
	matlib[names[j]->getStringValue()] = m;
	SG_LOG( SG_TERRAIN, SG_INFO, "  Loading material "
		<< names[j]->getStringValue());
      }
    } else {
      SG_LOG(SG_INPUT, SG_ALERT,
	     "Skipping bad material entry " << node->getName());
    }
  }

    // hard coded light state
    ssgSimpleState *lights = new ssgSimpleState;
    lights->ref();
    lights->disable( GL_TEXTURE_2D );
    lights->enable( GL_CULL_FACE );
    lights->enable( GL_COLOR_MATERIAL );
    lights->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    lights->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    lights->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    lights->enable( GL_BLEND );
    lights->disable( GL_ALPHA_TEST );
    lights->disable( GL_LIGHTING );

    matlib["LIGHTS"] = new FGNewMat(lights);

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

    SG_LOG( SG_TERRAIN, SG_INFO, "  Loading material " 
	    << mat_name << " (" << full_path << ")");

    material_lib.matlib[mat_name] = new FGNewMat(full_path);

    return true;
}


// Load a library of material properties
bool FGMaterialLib::add_item ( const string &mat_name, ssgSimpleState *state )
{
    FGNewMat *m = new FGNewMat(state);

    SG_LOG( SG_TERRAIN, SG_INFO, "  Loading material given a premade "
	    << "ssgSimpleState = " << mat_name );

    material_lib.matlib[mat_name] = m;

    return true;
}


// find a material record by material name
FGNewMat *FGMaterialLib::find( const string& material ) {
    FGNewMat *result = NULL;
    material_map_iterator it = matlib.find( material );
    if ( it != end() ) {
	result = it->second;
	return result;
    }

    return NULL;
}


// Destructor
FGMaterialLib::~FGMaterialLib ( void ) {
    // Free up all the material entries first
    for ( material_map_iterator it = begin(); it != end(); it++ ) {
	FGNewMat *slot = it->second;
	slot->deRef();
	if ( slot->getRef() <= 0 ) {
            delete slot;
        }
    }
}


// Set the step for all of the state selectors in the material slots
void FGMaterialLib::set_step ( int step )
{
    // container::iterator it = begin();
    for ( material_map_iterator it = begin(); it != end(); it++ ) {
	const string &key = it->first;
	SG_LOG( SG_GENERAL, SG_INFO,
		"Updating material " << key << " to step " << step );
	FGNewMat *slot = it->second;
	slot->get_state()->selectStep(step);
    }
}


// Get the step for the state selectors
int FGMaterialLib::get_step ()
{
  material_map_iterator it = begin();
  return it->second->get_state()->getSelectStep();
}


// Load one pending "deferred" texture.  Return true if a texture
// loaded successfully, false if no pending, or error.
void FGMaterialLib::load_next_deferred() {
    // container::iterator it = begin();
    for ( material_map_iterator it = begin(); it != end(); it++ ) {
	const string &key = it->first;
	FGNewMat *slot = it->second;
	if (slot->load_texture())
	  return;
    }
}
