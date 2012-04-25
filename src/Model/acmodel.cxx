// acmodel.cxx - manage a 3D aircraft model.
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
#include <Viewer/renderer.hxx>
#include <Viewer/viewmgr.hxx>
#include <Viewer/viewer.hxx>
#include <Scenery/scenery.hxx>
#include <Sound/fg_fx.hxx>

#include "model_panel.hxx"

#include "acmodel.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel
////////////////////////////////////////////////////////////////////////

FGAircraftModel::FGAircraftModel ()
  : _aircraft(0),
    _velocity(SGVec3d::zeros()),
    _fx(0),
    _lon(0),
    _lat(0),
    _alt(0),
    _pitch(0),
    _roll(0),
    _heading(0),
    _speed_n(0),
    _speed_e(0),
    _speed_d(0)
{
    SGSoundMgr *smgr = globals->get_soundmgr();
    _fx = new FGFX(smgr, "fx");
    _fx->init();
}

FGAircraftModel::~FGAircraftModel ()
{
  // drop reference
  _fx = 0;
  deinit();
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
    SG_LOG(SG_AIRCRAFT, SG_ALERT, "Failed to load aircraft from " << path << ':');
    SG_LOG(SG_AIRCRAFT, SG_ALERT, "  " << ex.getFormattedMessage());
    SG_LOG(SG_AIRCRAFT, SG_ALERT, "(Falling back to glider.ac.)");
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
FGAircraftModel::reinit()
{
  deinit();
  _fx->reinit();
  init();
}

void
FGAircraftModel::deinit()
{
  if (!_aircraft) {
    return;
  }
  
  osg::Node* node = _aircraft->getSceneGraph();
  globals->get_scenery()->get_aircraft_branch()->removeChild(node);

  delete _aircraft;
  _aircraft = NULL;
}

void
FGAircraftModel::bind ()
{
   _lon = fgGetNode("position/longitude-deg", true);
   _lat = fgGetNode("position/latitude-deg", true);
   _alt = fgGetNode("position/altitude-ft", true);
   _pitch = fgGetNode("orientation/pitch-deg", true);
   _roll = fgGetNode("orientation/roll-deg", true);
   _heading = fgGetNode("orientation/heading-deg", true);
   _speed_n = fgGetNode("velocities/speed-north-fps", true);
   _speed_e = fgGetNode("velocities/speed-east-fps", true);
   _speed_d = fgGetNode("velocities/speed-down-fps", true);
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

  _aircraft->setPosition(_lon->getDoubleValue(),
			 _lat->getDoubleValue(),
			 _alt->getDoubleValue());
  _aircraft->setOrientation(_roll->getDoubleValue(),
			    _pitch->getDoubleValue(),
			    _heading->getDoubleValue());
  _aircraft->update();

  // update model's audio sample values
  SGGeod position = _aircraft->getPosition();
  _fx->set_position_geod( position );

  SGQuatd orient = SGQuatd::fromYawPitchRollDeg(_heading->getDoubleValue(),
                                                _pitch->getDoubleValue(),
                                                _roll->getDoubleValue());
  _fx->set_orientation( orient );
 
  _velocity = SGVec3d( _speed_n->getDoubleValue(),
                       _speed_e->getDoubleValue(),
                       _speed_d->getDoubleValue() );
  _fx->set_velocity( _velocity );
}


// end of model.cxx
