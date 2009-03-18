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
    _nearplane(0.10f),
    _farplane(1000.0f)
{
}

FGAircraftModel::~FGAircraftModel ()
{
  osg::Node* node = _aircraft->getSceneGraph();
  globals->get_scenery()->get_aircraft_branch()->removeChild(node);

  delete _aircraft;
}

void 
FGAircraftModel::init ()
{
  _aircraft = new SGModelPlacement;
  string path = fgGetString("/sim/model/path", "Models/Geometry/glider.ac");
  try {
    osg::Node *model = fgLoad3DModelPanel( path, globals->get_props());
    _aircraft->init( model );
  } catch (const sg_exception &ex) {
    SG_LOG(SG_GENERAL, SG_ALERT, "Failed to load aircraft from " << path << ':');
    SG_LOG(SG_GENERAL, SG_ALERT, "  " << ex.getFormattedMessage());
    SG_LOG(SG_GENERAL, SG_ALERT, "(Falling back to glider.ac.)");
    osg::Node *model = fgLoad3DModelPanel( "Models/Geometry/glider.ac",
                                           globals->get_props());
    _aircraft->init( model );
  }
  osg::Node* node = _aircraft->getSceneGraph();
  // Do not do altitude computations with that model
  node->setNodeMask(~SG_NODEMASK_TERRAIN_BIT);
  globals->get_scenery()->get_aircraft_branch()->addChild(node);
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
