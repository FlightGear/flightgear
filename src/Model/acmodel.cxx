// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>		// for strcmp()

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/viewmgr.hxx>
#include "acmodel.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel
////////////////////////////////////////////////////////////////////////

FGAircraftModel::FGAircraftModel ()
  : _aircraft(0),
    _scene(new ssgRoot),
    _nearplane(0.01f),
    _farplane(5000.0f)
{
}

FGAircraftModel::~FGAircraftModel ()
{
  delete _aircraft;
  delete _scene;
}

void 
FGAircraftModel::init ()
{
  _aircraft = new FG3DModel;
  _aircraft->init(fgGetString("/sim/model/path", "Models/Geometry/glider.ac"));
  _scene->addKid(_aircraft->getSceneGraph());
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
FGAircraftModel::update (int dt)
{
  int view_number = globals->get_viewmgr()->get_current();

  if (view_number == 0 && !fgGetBool("/sim/view/internal")) {
    _aircraft->setVisible(false);
    return;
  }

  _aircraft->setVisible(true);

  _aircraft->setPosition(fgGetDouble("/position/longitude-deg"),
			 fgGetDouble("/position/latitude-deg"),
			 fgGetDouble("/position/altitude-ft"));
  _aircraft->setOrientation(fgGetDouble("/orientation/roll-deg"),
			    fgGetDouble("/orientation/pitch-deg"),
			    fgGetDouble("/orientation/heading-deg"));
  _aircraft->update(dt);

				// OK, now adjust the clip planes and draw
				// FIXME: view number shouldn't be 
				// hard-coded.
  if (globals->get_current_view()->getType() == 0) {
    glClearDepth(1);
    glClear(GL_DEPTH_BUFFER_BIT);
    ssgSetNearFar(_nearplane, _farplane);
  }
  ssgCullAndDraw(_scene);
}


// end of model.cxx
