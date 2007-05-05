// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>		// for strcmp()

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/renderer.hxx>
#include <Main/viewmgr.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>

#include "model_panel.hxx"

#include "acmodel.hxx"


////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel
////////////////////////////////////////////////////////////////////////

FGAircraftModel::FGAircraftModel ()
  : _aircraft(0),
    _selector(new osg::Switch),
    _nearplane(0.10f),
    _farplane(1000.0f)
{
}

FGAircraftModel::~FGAircraftModel ()
{
  delete _aircraft;
				// SSG will delete it
  globals->get_scenery()->get_aircraft_branch()->removeChild(_selector.get());
}

void 
FGAircraftModel::init ()
{
  SGPath liveryPath;
  _aircraft = new SGModelPlacement;
  string path = fgGetString("/sim/model/path", "Models/Geometry/glider.ac");
  string texture_path = fgGetString("/sim/model/texture-path");
  if( texture_path.size() ) {
      SGPath temp_path;
      if ( !ulIsAbsolutePathName( texture_path.c_str() ) ) {
          temp_path = globals->get_fg_root();
          temp_path.append( SGPath( path ).dir() );
          temp_path.append( texture_path );
          liveryPath = temp_path;
      } else
          liveryPath = texture_path;
  }
  try {
    osg::Node *model = fgLoad3DModelPanel( globals->get_fg_root(),
                                           path,
                                           globals->get_props(),
                                           globals->get_sim_time_sec(),
                                           liveryPath);
    _aircraft->init( model );
  } catch (const sg_exception &ex) {
    SG_LOG(SG_GENERAL, SG_ALERT, "Failed to load aircraft from " << path);
    SG_LOG(SG_GENERAL, SG_ALERT, "(Falling back to glider.ac.)");
    osg::Node *model = fgLoad3DModelPanel( globals->get_fg_root(),
                                           "Models/Geometry/glider.ac",
                                           globals->get_props(),
                                           globals->get_sim_time_sec(),
                                           liveryPath);
    _aircraft->init( model );
  }
  _selector->addChild(_aircraft->getSceneGraph());
  // Do not do altitude computations with that model
  _selector->setNodeMask(~SG_NODEMASK_TERRAIN_BIT);
  globals->get_scenery()->get_aircraft_branch()->addChild(_selector.get());
}

void
FGAircraftModel::bind ()
{
  // No-op
}

void
FGAircraftModel::unbind ()
{
  // No-op
}

void
FGAircraftModel::update (double dt)
{
  int view_number = globals->get_viewmgr()->get_current();
  int is_internal = fgGetBool("/sim/current-view/internal");

  if (view_number == 0 && !is_internal) {
    _aircraft->setVisible(false);
  } else {
    _aircraft->setVisible(true);
  }

  _aircraft->setPosition(fgGetDouble("/position/longitude-deg"),
			 fgGetDouble("/position/latitude-deg"),
			 fgGetDouble("/position/altitude-ft"));
  _aircraft->setOrientation(fgGetDouble("/orientation/roll-deg"),
			    fgGetDouble("/orientation/pitch-deg"),
			    fgGetDouble("/orientation/heading-deg"));
  _aircraft->update();
}


// end of model.cxx
