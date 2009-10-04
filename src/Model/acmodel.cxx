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
#include <Sound/fg_fx.hxx>

#include "model_panel.hxx"

#include "acmodel.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of FGAircraftModel
////////////////////////////////////////////////////////////////////////

FGAircraftModel::FGAircraftModel ()
  : _aircraft(0),
    _fx(0),
    _lon(0),
    _lat(0),
    _alt(0),
    _pitch(0),
    _roll(0),
    _heading(0),
    _speed_north(0),
    _speed_east(0),
    _speed_up(0)
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
   _lon = fgGetNode("position/longitude-deg", true);
   _lat = fgGetNode("position/latitude-deg", true);
   _alt = fgGetNode("position/altitude-ft", true);
   _pitch = fgGetNode("orientation/pitch-deg", true);
   _roll = fgGetNode("orientation/roll-deg", true);
   _heading = fgGetNode("orientation/heading-deg", true);
   _speed_north = fgGetNode("/velocities/speed-north-fps", true);
   _speed_east = fgGetNode("/velocities/speed-east-fps", true);
   _speed_up = fgGetNode("/velocities/vertical-speed-fps", true);
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

  if ( !_fx) {
    SGSoundMgr *smgr = (SGSoundMgr *)globals->get_subsystem("soundmgr");
    if (smgr) {
        _fx = new FGFX(smgr, "fx");
        _fx->init();
    }
  }

  if (_fx) {
    // Get the Cartesian coordinates in meters
    SGVec3d pos = SGVec3d::fromGeod(_aircraft->getPosition());
    _fx->set_position( pos );

    SGQuatd orient_m = SGQuatd::fromLonLat(_aircraft->getPosition());
    orient_m *= SGQuatd::fromYawPitchRollDeg(_heading->getDoubleValue(),
                                             _pitch->getDoubleValue(),
                                             _roll->getDoubleValue());
    SGVec3d orient = orient_m.rotateBack(SGVec3d::e1());
    _fx->set_orientation( toVec3f(orient) );
 
    SGVec3f vel = SGVec3f( _speed_north->getFloatValue(),
                           _speed_east->getFloatValue(),
                           _speed_up->getFloatValue());
// TODO: rotate to properly align with the model orientation

    _fx->set_velocity( vel*SG_FEET_TO_METER );
  }
}


// end of model.cxx
