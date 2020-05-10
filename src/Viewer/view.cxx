// view.cxx -- class for managing a view in the flightgear world.
//
// Written by Curtis Olson, started August 1997.
//                          overhaul started October 2000.
//   partially rewritten by Jim Wilson jim@kelcomaine.com using interface
//                          by David Megginson March 2002
//
// Copyright (C) 1997 - 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "view.hxx"

#include <simgear/compiler.h>
#include <cassert>

#include <simgear/debug/logstream.hxx>
#include <simgear/constants.h>
#include <simgear/scene/model/placement.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/structure/event_mgr.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <MultiPlayer/multiplaymgr.hxx>
#include <AIModel/AIMultiplayer.hxx>
#include "CameraGroup.hxx"

#include "ViewPropertyEvaluator.hxx"

using namespace flightgear;

////////////////////////////////////////////////////////////////////////
// Implementation of FGViewer.
////////////////////////////////////////////////////////////////////////


// Constructor...
View::View( ViewType Type, bool from_model, int from_model_index,
                    bool at_model, int at_model_index,
                    double damp_roll, double damp_pitch, double damp_heading,
                    double x_offset_m, double y_offset_m, double z_offset_m,
                    double heading_offset_deg, double pitch_offset_deg,
                    double roll_offset_deg,
                    double fov_deg, double aspect_ratio_multiplier,
                    double target_x_offset_m, double target_y_offset_m,
                    double target_z_offset_m, double near_m, bool internal,
                    bool lookat_agl, double lookat_agl_damping,
                    int view_index ):
    _dirty(true),
    _roll_deg(0),
    _pitch_deg(0),
    _heading_deg(0),
    _target_roll_deg(0),
    _target_pitch_deg(0),
    _target_heading_deg(0),
    _lookat_agl_damping(lookat_agl_damping /*damping*/, 0 /*min*/, 0 /*max*/),
    _lookat_agl_ground_altitude(0),
    _scaling_type(FG_SCALING_MAX)
{
    _absolute_view_pos = SGVec3d(0, 0, 0);
    _type = Type;
    _from_model = from_model;
    _from_model_index = from_model_index;
    _at_model = at_model;
    _at_model_index = at_model_index;

    _internal = internal;
    _lookat_agl = lookat_agl;
    _view_index = view_index;

    _dampFactor = SGVec3d::zeros();
    _dampOutput = SGVec3d::zeros();
    _dampTarget = SGVec3d::zeros();

    if (damp_roll > 0.0)
      _dampFactor[0] = 1.0 / pow(10.0, fabs(damp_roll));
    if (damp_pitch > 0.0)
      _dampFactor[1] = 1.0 / pow(10.0, fabs(damp_pitch));
    if (damp_heading > 0.0)
      _dampFactor[2] = 1.0 / pow(10.0, fabs(damp_heading));

    _offset_m.x() = x_offset_m;
    _offset_m.y() = y_offset_m;
    _offset_m.z() = z_offset_m;
    _configOffset_m = _offset_m;

    _heading_offset_deg = heading_offset_deg;
    _pitch_offset_deg = pitch_offset_deg;
    _roll_offset_deg = roll_offset_deg;
    _goal_heading_offset_deg = heading_offset_deg;
    _goal_pitch_offset_deg = pitch_offset_deg;
    _goal_roll_offset_deg = roll_offset_deg;

    _configHeadingOffsetDeg = heading_offset_deg;
    _configPitchOffsetDeg = pitch_offset_deg;
    _configRollOffsetDeg = roll_offset_deg;

    if (fov_deg > 0) {
      _fov_deg = fov_deg;
    } else {
      _fov_deg = 55;
    }

    _configFOV_deg = _fov_deg;

    _fov_user_deg = _fov_deg;

    _aspect_ratio_multiplier = aspect_ratio_multiplier;
    _target_offset_m.x() = target_x_offset_m;
    _target_offset_m.y() = target_y_offset_m;
    _target_offset_m.z() = target_z_offset_m;
    _configTargetOffset_m = _target_offset_m;

    _ground_level_nearplane_m = near_m;
    // a reasonable guess for init, so that the math doesn't blow up

    resetOffsetsAndFOV();
}

View* View::createFromProperties(SGPropertyNode_ptr config, int view_index)
{
    double aspect_ratio_multiplier
        = fgGetDouble("/sim/current-view/aspect-ratio-multiplier");

    // find out if this is an internal view (e.g. in cockpit, low near plane)
    // FIXME : should be a child of config
    bool internal = config->getParent()->getBoolValue("internal", false);

    std::string root = config->getPath();
    // Will typically be /sim/view[]/config.

    // FIXME:
    // this is assumed to be an aircraft model...we will need to read
    // model-from-type as well.

    // find out if this is a model we are looking from...
    bool from_model = config->getBoolValue("from-model");
    int from_model_index = config->getIntValue("from-model-idx");

    double x_offset_m = config->getDoubleValue("x-offset-m");
    double y_offset_m = config->getDoubleValue("y-offset-m");
    double z_offset_m = config->getDoubleValue("z-offset-m");

    double heading_offset_deg = config->getDoubleValue("heading-offset-deg");
  //  config->setDoubleValue("heading-offset-deg", heading_offset_deg);
    double pitch_offset_deg = config->getDoubleValue("pitch-offset-deg");
  //  config->setDoubleValue("pitch-offset-deg", pitch_offset_deg);
    double roll_offset_deg = config->getDoubleValue("roll-offset-deg");
   // config->setDoubleValue("roll-offset-deg", roll_offset_deg);

    double fov_deg = config->getDoubleValue("default-field-of-view-deg");
    double near_m = config->getDoubleValue("ground-level-nearplane-m");

    View* v = nullptr;
    // supporting two types "lookat" = 1 and "lookfrom" = 0
    const char *type = config->getParent()->getStringValue("type");
    if (!strcmp(type, "lookat")) {
        bool at_model = config->getBoolValue("at-model");
        int at_model_index = config->getIntValue("at-model-idx");

        double damp_roll = config->getDoubleValue("at-model-roll-damping");
        double damp_pitch = config->getDoubleValue("at-model-pitch-damping");
        double damp_heading = config->getDoubleValue("at-model-heading-damping");

        double target_x_offset_m = config->getDoubleValue("target-x-offset-m");
        double target_y_offset_m = config->getDoubleValue("target-y-offset-m");
        double target_z_offset_m = config->getDoubleValue("target-z-offset-m");
        bool lookat_agl = config->getBoolValue("lookat-agl");
        double lookat_agl_damping = config->getDoubleValue("lookat-agl-damping");

        v = new View ( FG_LOOKAT, from_model, from_model_index,
                           at_model, at_model_index,
                           damp_roll, damp_pitch, damp_heading,
                           x_offset_m, y_offset_m,z_offset_m,
                           heading_offset_deg, pitch_offset_deg,
                           roll_offset_deg, fov_deg, aspect_ratio_multiplier,
                           target_x_offset_m, target_y_offset_m,
                           target_z_offset_m, near_m, internal, lookat_agl,
                           lookat_agl_damping, view_index );
    } else {
        v = new View ( FG_LOOKFROM, from_model, from_model_index,
                       false, 0, 0.0, 0.0, 0.0,
                       x_offset_m, y_offset_m, z_offset_m,
                       heading_offset_deg, pitch_offset_deg,
                       roll_offset_deg, fov_deg, aspect_ratio_multiplier,
                         0, 0, 0, near_m, internal, false, 0.0, view_index );
    }

    v->_name = config->getParent()->getStringValue("name");
    v->_typeString = type;
    v->_configHeadingOffsetDeg = config->getDoubleValue("default-heading-offset-deg");
    v->_config = config;

    return v;
}


// Destructor
View::~View( void )
{
    _tiedProperties.Untie();
}

void
View::init ()
{
}

void
View::bind ()
{
    // Perform an immediate recalculation to ensure that the data for the
    // viewer position is correct.
    recalc();

    _tiedProperties.setRoot(fgGetNode("/sim/current-view", true));
    _tiedProperties.Tie("heading-offset-deg", this,
                         &View::getHeadingOffset_deg,
                         &View::setHeadingOffset_deg_property,
                        false /* do not set current property value */);

     fgSetArchivable("/sim/current-view/heading-offset-deg");

    _tiedProperties.Tie("goal-heading-offset-deg", this,
                        &View::getGoalHeadingOffset_deg,
                        &View::setGoalHeadingOffset_deg,
                        false /* do not set current property value */);

    fgSetArchivable("/sim/current-view/goal-heading-offset-deg");

    _tiedProperties.Tie("pitch-offset-deg", this,
                        &View::getPitchOffset_deg,
                        &View::setPitchOffset_deg_property,
                        false /* do not set current property value */);
    fgSetArchivable("/sim/current-view/pitch-offset-deg");
    _tiedProperties.Tie("goal-pitch-offset-deg", this,
                        &View::getGoalPitchOffset_deg,
                        &View::setGoalPitchOffset_deg,
                        false /* do not set current property value */);
    fgSetArchivable("/sim/current-view/goal-pitch-offset-deg");
    _tiedProperties.Tie("roll-offset-deg", this,
                        &View::getRollOffset_deg,
                        &View::setRollOffset_deg_property,
                        false /* do not set current property value */);
    fgSetArchivable("/sim/current-view/roll-offset-deg");
    _tiedProperties.Tie("goal-roll-offset-deg", this,
                        &View::getGoalRollOffset_deg,
                        &View::setGoalRollOffset_deg,
                        false /* do not set current property value */);
    fgSetArchivable("/sim/current-view/goal-roll-offset-deg");


    _tiedProperties.Tie("field-of-view", this,
                        &View::get_fov_user, &View::set_fov_user,
                        false);
    fgSetArchivable("/sim/current-view/field-of-view");


    _tiedProperties.Tie("aspect-ratio-multiplier", this,
                        &View::get_aspect_ratio_multiplier,
                        &View::set_aspect_ratio_multiplier,
                        false);

    _tiedProperties.Tie("ground-level-nearplane-m", this,
                        &View::getNear_m, &View::setNear_m, false);
    fgSetArchivable("/sim/current-view/ground-level-nearplane-m");


    _tiedProperties.Tie("viewer-lon-deg", this, &View::getLon_deg);
    _tiedProperties.Tie("viewer-lat-deg", this, &View::getLat_deg);
    _tiedProperties.Tie("viewer-elev-ft", this, &View::getElev_ft);

    _tiedProperties.Tie("x-offset-m", this, &View::getAdjustXOffset_m,
                        &View::setAdjustXOffset_m, false);
    _tiedProperties.Tie("y-offset-m", this, &View::getAdjustYOffset_m,
                        &View::setAdjustYOffset_m, false);
    _tiedProperties.Tie("z-offset-m", this, &View::getAdjustZOffset_m,
                        &View::setAdjustZOffset_m, false);

    _tiedProperties.Tie("target-x-offset-m", this, &View::getTargetXOffset_m,
                        &View::setTargetXOffset_m, false);
    _tiedProperties.Tie("target-y-offset-m", this, &View::getTargetYOffset_m,
                        &View::setTargetYOffset_m, false);
    _tiedProperties.Tie("target-z-offset-m", this, &View::getTargetZOffset_m,
                        &View::setTargetZOffset_m, false);

// expose various quaternions under the debug/ subtree
    _tiedProperties.Tie("debug/orientation-w", this, &View::getOrientation_w);
    _tiedProperties.Tie("debug/orientation-x", this, &View::getOrientation_x);
    _tiedProperties.Tie("debug/orientation-y", this, &View::getOrientation_y);
    _tiedProperties.Tie("debug/orientation-z", this, &View::getOrientation_z);

    _tiedProperties.Tie("debug/orientation_offset-w", this,
                        &View::getOrOffset_w);
    _tiedProperties.Tie("debug/orientation_offset-x", this,
                        &View::getOrOffset_x);
    _tiedProperties.Tie("debug/orientation_offset-y", this,
                        &View::getOrOffset_y);
    _tiedProperties.Tie("debug/orientation_offset-z", this,
                        &View::getOrOffset_z);

    _tiedProperties.Tie("debug/frame-w", this, &View::getFrame_w);
    _tiedProperties.Tie("debug/frame-x", this, &View::getFrame_x);
    _tiedProperties.Tie("debug/frame-y", this, &View::getFrame_y);
    _tiedProperties.Tie("debug/frame-z", this, &View::getFrame_z);


// expose the raw (OpenGL) orientation to the property tree,
// for the sound-manager
    _tiedProperties.Tie("raw-orientation", 0, this, &View::getRawOrientation_w);
    _tiedProperties.Tie("raw-orientation", 1, this, &View::getRawOrientation_x);
    _tiedProperties.Tie("raw-orientation", 2, this, &View::getRawOrientation_y);
    _tiedProperties.Tie("raw-orientation", 3, this, &View::getRawOrientation_z);

    _tiedProperties.Tie("viewer-x-m", this, &View::getAbsolutePosition_x);
    _tiedProperties.Tie("viewer-y-m", this, &View::getAbsolutePosition_y);
    _tiedProperties.Tie("viewer-z-m", this, &View::getAbsolutePosition_z);

// following config properties are exposed on current-view but don't change,
// so we can simply copy them here.
    _tiedProperties.getRoot()->setStringValue("name", _name);
    _tiedProperties.getRoot()->setStringValue("type", _typeString);
    _tiedProperties.getRoot()->setBoolValue("internal", _internal);

    SGPropertyNode_ptr config = _tiedProperties.getRoot()->getChild("config", 0, true);
    config->setBoolValue("from-model", _from_model);
    config->setDoubleValue("heading-offset-deg", _configHeadingOffsetDeg);
    config->setDoubleValue("pitch-offset-deg", _configPitchOffsetDeg);
    config->setDoubleValue("roll-offset-deg", _configRollOffsetDeg);
    config->setDoubleValue("default-field-of-view-deg", _configFOV_deg);
}

void
View::unbind ()
{
    _tiedProperties.Untie();
}

void View::resetOffsetsAndFOV()
{
    _target_offset_m = _configTargetOffset_m;
    _offset_m = _configOffset_m;
    _adjust_offset_m = _configOffset_m;
    _pitch_offset_deg = _configPitchOffsetDeg;
    _heading_offset_deg = _configHeadingOffsetDeg;
    _roll_offset_deg = _configRollOffsetDeg;
    _fov_deg = _configFOV_deg;
}

void
View::setType ( int type )
{
  if (type == 0)
    _type = FG_LOOKFROM;
  if (type == 1)
    _type = FG_LOOKAT;
}

void
View::setInternal ( bool internal )
{
  _internal = internal;
}

void
View::setPosition (const SGGeod& geod)
{
  _dirty = true;
    _position = geod;
}

void
View::setTargetPosition (const SGGeod& geod)
{
  _dirty = true;
    _target = geod;
}

void
View::setRoll_deg (double roll_deg)
{
  _dirty = true;
  _roll_deg = roll_deg;
}

void
View::setPitch_deg (double pitch_deg)
{
  _dirty = true;
  _pitch_deg = pitch_deg;
}

void
View::setHeading_deg (double heading_deg)
{
  _dirty = true;
  _heading_deg = heading_deg;
}

void
View::setOrientation (double roll_deg, double pitch_deg, double heading_deg)
{
  _dirty = true;
  _roll_deg = roll_deg;
  _pitch_deg = pitch_deg;
  _heading_deg = heading_deg;
}

void
View::setTargetRoll_deg (double target_roll_deg)
{
  _dirty = true;
  _target_roll_deg = target_roll_deg;
}

void
View::setTargetPitch_deg (double target_pitch_deg)
{
  _dirty = true;
  _target_pitch_deg = target_pitch_deg;
}

void
View::setTargetHeading_deg (double target_heading_deg)
{
  _dirty = true;
  _target_heading_deg = target_heading_deg;
}

void
View::setTargetOrientation (double target_roll_deg, double target_pitch_deg, double target_heading_deg)
{
  _dirty = true;
  _target_roll_deg = target_roll_deg;
  _target_pitch_deg = target_pitch_deg;
  _target_heading_deg = target_heading_deg;
}

void
View::setXOffset_m (double x_offset_m)
{
  _dirty = true;
  _offset_m.x() = x_offset_m;
}

void
View::setYOffset_m (double y_offset_m)
{
  _dirty = true;
  _offset_m.y() = y_offset_m;
}

void
View::setZOffset_m (double z_offset_m)
{
  _dirty = true;
  _offset_m.z() = z_offset_m;
}

void
View::setTargetXOffset_m (double target_x_offset_m)
{
  _dirty = true;
  _target_offset_m.x() = target_x_offset_m;
}

void
View::setTargetYOffset_m (double target_y_offset_m)
{
  _dirty = true;
  _target_offset_m.y() = target_y_offset_m;
}

void
View::setTargetZOffset_m (double target_z_offset_m)
{
  _dirty = true;
  _target_offset_m.z() = target_z_offset_m;
}

void
View::setAdjustXOffset_m (double x_offset_m)
{
  _dirty = true;
  _adjust_offset_m.x() = x_offset_m;
}

void
View::setAdjustYOffset_m (double y_offset_m)
{
  _dirty = true;
  _adjust_offset_m.y() = y_offset_m;
}

void
View::setAdjustZOffset_m (double z_offset_m)
{
  _dirty = true;
  _adjust_offset_m.z() = z_offset_m;
}

void
View::setPositionOffsets (double x_offset_m, double y_offset_m, double z_offset_m)
{
  _dirty = true;
  _offset_m.x() = x_offset_m;
  _offset_m.y() = y_offset_m;
  _offset_m.z() = z_offset_m;
}

void
View::setRollOffset_deg (double roll_offset_deg)
{
  _dirty = true;
  _roll_offset_deg = roll_offset_deg;
}

void
View::setPitchOffset_deg (double pitch_offset_deg)
{
  _dirty = true;
  _pitch_offset_deg = pitch_offset_deg;
}

void
View::setHeadingOffset_deg (double heading_offset_deg)
{
  _dirty = true;
  if (_at_model && (_offset_m.x() == 0.0)&&(_offset_m.z() == 0.0))
  {
      /* avoid optical effects (e.g. rotating sky) when "looking at" with
       * heading offsets x==z==0 (view heading cannot change). */
      _heading_offset_deg = 0.0;
  }
  else
      _heading_offset_deg = heading_offset_deg;
}

void
View::setHeadingOffset_deg_property (double heading_offset_deg)
{
    setHeadingOffset_deg(heading_offset_deg);
    setGoalHeadingOffset_deg(heading_offset_deg);
}

void
View::setPitchOffset_deg_property (double pitch_offset_deg)
{
    setPitchOffset_deg(pitch_offset_deg);
    setGoalPitchOffset_deg(pitch_offset_deg);
}

void
View::setRollOffset_deg_property (double roll_offset_deg)
{
    setRollOffset_deg(roll_offset_deg);
    setGoalRollOffset_deg(roll_offset_deg);
}

void
View::setGoalRollOffset_deg (double goal_roll_offset_deg)
{
  _dirty = true;
  _goal_roll_offset_deg = goal_roll_offset_deg;
}

void
View::setGoalPitchOffset_deg (double goal_pitch_offset_deg)
{
  _dirty = true;
  _goal_pitch_offset_deg = goal_pitch_offset_deg;
  /* The angle is set to 1/1000th of a degree from the poles to avoid the
   * singularity where the azimuthal angle becomes undefined, inducing optical
   * artefacts.  The arbitrary angle offset is visually unnoticeable while
   * avoiding any possible floating point truncation artefacts. */
  if ( _goal_pitch_offset_deg < -89.999 ) {
    _goal_pitch_offset_deg = -89.999;
  }
  if ( _goal_pitch_offset_deg > 89.999 ) {
    _goal_pitch_offset_deg = 89.999;
  }

}

void
View::setGoalHeadingOffset_deg (double goal_heading_offset_deg)
{
  _dirty = true;
  if (_at_model && (_offset_m.x() == 0.0)&&(_offset_m.z() == 0.0))
  {
      /* avoid optical effects (e.g. rotating sky) when "looking at" with
       * heading offsets x==z==0 (view heading cannot change). */
      _goal_heading_offset_deg = 0.0;
      return;
  }

  _goal_heading_offset_deg = goal_heading_offset_deg;
  while ( _goal_heading_offset_deg < 0.0 ) {
    _goal_heading_offset_deg += 360;
  }
  while ( _goal_heading_offset_deg > 360 ) {
    _goal_heading_offset_deg -= 360;
  }
}

void
View::setOrientationOffsets (double roll_offset_deg, double pitch_offset_deg, double heading_offset_deg)
{
  _dirty = true;
  _roll_offset_deg = roll_offset_deg;
  _pitch_offset_deg = pitch_offset_deg;
  _heading_offset_deg = heading_offset_deg;
}

// recalc() is done every time one of the setters is called (making the
// cached data "dirty") on the next "get".  It calculates all the outputs
// for viewer.
void
View::recalc ()
{
  if (_type == FG_LOOKFROM) {
    recalcLookFrom();
  } else {
    recalcLookAt();
  }

  set_clean();
}


/* Gets position and orientation of user aircraft or multiplayer aircraft.

root:
    Location of aircraft position and oriention properties. Use '' for user's
    aircraft, or /ai/models/multiplayer[] for a multiplayer aircraft.

position:
head:
pitch:
roll:
    Out parameters.
*/
static void getAircraftPositionOrientation(
    SGGeod& position,
    double& head,
    double& pitch,
    double& roll
    )
{
    position = SGGeod::fromDegFt(
        ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/position/longitude-deg)"),
        ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/position/latitude-deg)"),
        ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/position/altitude-ft)")
        );

    head  = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/orientation/true-heading-deg)");
    pitch = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/orientation/pitch-deg)");
    roll  = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/orientation/roll-deg)");
}


/* Finds the offset of a view position from an aircraft model's origin.

We look in either /sim/view[]/config/ for user's aircraft, or
/ai/models/multiplayer[]/set/sim/view[]/config/ for multiplayer aircraft.

If offset information is not available because we can't find the -set.xml files
for a multiplayer aircraft), we set all elements of offset_m to zero.

root:
    Path that defines which aircraft. Either '' to use the user's aircraft, or
    /ai/models/multiplayer[].
view_index:
    The view number. We will look in .../sim/view[view_index].
infix:
    We look at .../view[view_index]/config/<infix>x-offset-m etc. Views appear
    to use 'z-offset-m' to define the location of pilot's eyes in cockpit
    view, but 'target-z-offset' when defining viewpoint of other views such as
    Helicopter View. So one should typically set <infix> to '' or 'target-'.
adjust:
    If not NULL, typically points to copy of offsets in
    /sim/current-view/?-offset-m, and we add *adjust to the return value, and
    also subtract /sim/view[]/config/?-offset-m. We do the latter to preserve
    expectations that /sim/current-view/?-offset-m includes the view offset, so
    that aircraft that define custom view behaviour continue to work correctly.
offset_m:
    Out param.

Returns true if any component (x, y or z) was found, otherwise false.
*/
static void getViewOffsets(
    bool target_infix,
    const SGVec3d* adjust,
    SGVec3d& offset_m
    )
{
    std::string root = ViewPropertyEvaluator::getStringValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/root)");
    if (root == "/" || root == "") {
        if (target_infix) {
            offset_m.x() = ViewPropertyEvaluator::getDoubleValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/target-x-offset-m)");
            offset_m.y() = ViewPropertyEvaluator::getDoubleValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/target-y-offset-m)");
            offset_m.z() = ViewPropertyEvaluator::getDoubleValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/target-z-offset-m)");
        }
        else {
            offset_m.x() = ViewPropertyEvaluator::getDoubleValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/x-offset-m)");
            offset_m.y() = ViewPropertyEvaluator::getDoubleValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/y-offset-m)");
            offset_m.z() = ViewPropertyEvaluator::getDoubleValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/z-offset-m)");
        }
    }
    else {
        if (target_infix) {
            offset_m.x() = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/set/sim/view[(/sim/current-view/view-number-raw)]/config/target-x-offset-m)");
            offset_m.y() = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/set/sim/view[(/sim/current-view/view-number-raw)]/config/target-y-offset-m)");
            offset_m.z() = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/set/sim/view[(/sim/current-view/view-number-raw)]/config/target-z-offset-m)");
        }
        else {
            offset_m.x() = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/set/sim/view[(/sim/current-view/view-number-raw)]/config/x-offset-m)");
            offset_m.y() = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/set/sim/view[(/sim/current-view/view-number-raw)]/config/y-offset-m)");
            offset_m.z() = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/set/sim/view[(/sim/current-view/view-number-raw)]/config/z-offset-m)");
        }
    }
    if (adjust) {
        SGVec3d offset = *adjust;
        /* Note that we subtract the raw /sim/view[]/config/?-offset-m
        regardless of whether we are viwing a multiplayer aircraft or not. */
        offset.x() -= ViewPropertyEvaluator::getDoubleValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/x-offset-m)");
        offset.y() -= ViewPropertyEvaluator::getDoubleValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/y-offset-m)");
        offset.z() -= ViewPropertyEvaluator::getDoubleValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/z-offset-m)");
        offset_m += offset;
    }
}

// recalculate for LookFrom view type...
// E.g. Cockpit View and Tower View Look From.
void
View::recalcLookFrom ()
{
  double head;
  double pitch;
  double roll;

  if (_from_model ) {
    /* Look up aircraft position; this works for user's aircaft or multiplayer
    aircraft. */
    getAircraftPositionOrientation( _position, head, pitch, roll);
  }
  else {
    /* Tower View Look From.

    <_config> will be /sim/view[4]/config, so <_config>/eye-lon-deg-path
    will be /sim/view[4]/config/eye-lon-deg-path which will contain
    /sim/tower/longitude-deg (see fgdata:defaults.xml).

    [Might be nice to make a 'TowerView Look From Multiplayer' that uses the
    tower that is nearest to a particular multiplayer aircraft. We'd end up
    looking at /ai/models/multiplayer[]/sim/tower/longitude-deg, so we'd need
    to somehow set up /ai/models/multiplayer[]/sim/tower. ]
    */
    _position = SGGeod::fromDegFt(
        ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-lon-deg-path))"),
        ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-lat-deg-path))"),
        ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-alt-ft-path))")
        );
    head    = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-heading-deg-path))");
    pitch   = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-pitch-deg-path))");
    roll    = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-roll-deg-path))");
  }

  /* Find the offset of the view position relative to the aircraft model's
  origin. */
  SGVec3d offset_m;
  getViewOffsets(false /*target_infix*/, &_adjust_offset_m, offset_m);

  set_fov(_fov_user_deg);

  // The rotation rotating from the earth centerd frame to
  // the horizontal local frame
  SGQuatd hlOr = SGQuatd::fromLonLat(_position);

  // The rotation from the horizontal local frame to the basic view orientation
  SGQuatd hlToBody = SGQuatd::fromYawPitchRollDeg(head, pitch, roll);

  // The rotation offset, don't know why heading is negative here ...
  mViewOffsetOr
      = SGQuatd::fromYawPitchRollDeg(-_heading_offset_deg, _pitch_offset_deg,
                                     _roll_offset_deg);

  // Compute the eyepoints orientation and position
  // wrt the earth centered frame - that is global coorinates
  SGQuatd ec2body = hlOr*hlToBody;

  // The cartesian position of the basic view coordinate
  SGVec3d position = SGVec3d::fromGeod(_position);

  // This is rotates the x-forward, y-right, z-down coordinate system the where
  // simulation runs into the OpenGL camera system with x-right, y-up, z-back.
  SGQuatd q(-0.5, -0.5, 0.5, 0.5);

  _absolute_view_pos = position + (ec2body*q).backTransform(offset_m);
  mViewOrientation = ec2body*mViewOffsetOr*q;
}


/* Change angle and field of view so that we can see the aircraft and the
ground immediately below it. */

/* Some aircraft appear to have elevation that is slightly below ground level
when on the ground, e.g.  SenecaII, which makes get_elevation_m() fail. So we
pass a slightly incremented elevation. */
void View::handleAGL()
{
  /* Change angle and field of view so that we can see the aircraft and the
  ground immediately below it. */

  /* Some aircraft appear to have elevation that is slightly below ground
  level when on the ground, e.g.  SenecaII, which makes get_elevation_m()
  fail. So we pass a slightly incremented elevation. */
  double                      ground_altitude = 0;
  const simgear::BVHMaterial* material = NULL;
  SGGeod                      target_plus = _target;
  target_plus.setElevationM(target_plus.getElevationM() + 1);
  bool ok = globals->get_scenery()->get_elevation_m(target_plus, ground_altitude, &material);

  if (ok) {
    _lookat_agl_ground_altitude = ground_altitude;
  }
  else {
      /* get_elevation_m() can fail if scenery has been un-cached, which
      appears to happen quite often with remote multiplayer aircraft, so we
      preserve the previous ground altitude to give some consistency and avoid
      confusing zooming when switching between views.

      [Might be better to have per-aircraft state too, so that switching
      between multiplayer aircraft doesn't cause zooming.] */
      ground_altitude = _lookat_agl_ground_altitude;
      SG_LOG(SG_VIEW, SG_DEBUG, "get_elevation_m() failed. _target=" << _target << "\n");
  }

  double    h_distance = SGGeodesy::distanceM(_position, _target);
  if (h_distance == 0) {
      /* Not sure this should ever happen, but we need to handle this here
      otherwise we'll get divide-by-zero. Just use whatever field of view the
      user has set. */
      set_fov(_fov_user_deg);
  }
  else {
    /* Find vertical region we want to be able to see. */
    double  relative_height_target = _target.getElevationM() - _position.getElevationM();
    double  relative_height_ground = ground_altitude - _position.getElevationM();

    /* We expand the field of view so that it hopefully shows the
    whole aircraft and a little more of the ground.

    We use chase-distance as a crude measure of the aircraft's size. There
    doesn't seem to be any more definitive information.

    We damp our measure of ground level, to avoid the view jumping around if
    an aircraft flies over buildings.
    */

    relative_height_ground -= 2;

    double    chase_distance_m;
    const std::string&  root = ViewPropertyEvaluator::getStringValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/root)");
    if (root == "/" || root == "") {
        chase_distance_m = ViewPropertyEvaluator::getDoubleValue("(/sim/chase-distance-m)", -25);
    }
    else {
        chase_distance_m = ViewPropertyEvaluator::getDoubleValue(
                "((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/set/sim/chase-distance-m)",
                -25
                );
    }
    if (chase_distance_m == 0) {
        /* ViewPropertyEvaluator::getDoubleValue() doesn't handle default
        values very well because it always creates empty nodes if they don't
        exist. So we override the value here. */
        chase_distance_m = -25;
    }
    double    aircraft_size_vertical = fabs(chase_distance_m) * 0.3;
    double    aircraft_size_horizontal = fabs(chase_distance_m) * 0.9;

    double    relative_height_target_plus = relative_height_target + aircraft_size_vertical;
    double    relative_height_ground_ = relative_height_ground;
    _lookat_agl_damping.updateTarget(relative_height_ground);
    if (relative_height_ground > relative_height_target) {
        /* Damping of relative_height_ground can result in it being
        temporarily above the aircraft, so we ensure the aircraft is visible.
        */
        _lookat_agl_damping.reset(relative_height_ground_);
        relative_height_ground = relative_height_ground_;
    }

    /* Apply scaling from user field of view setting, altering only
    relative_height_ground so that the aircraft is always in view. */
    {
        double  delta = relative_height_target_plus - relative_height_ground;
        delta *= (_fov_user_deg / _configFOV_deg);
        relative_height_ground = relative_height_target_plus - delta;
    }

    double  angle_v_target = atan(relative_height_target_plus / h_distance);
    double  angle_v_ground = atan(relative_height_ground / h_distance);

    /* The target we want to use is determined by the midpoint of the two
    angles we've calculated. */
    double  angle_v_mid = (angle_v_target + angle_v_ground) / 2;
    _target.setElevationM(_position.getElevationM() + h_distance * tan(angle_v_mid));

    /* Set field of view. We use fabs to avoid things being upside down if
    target is below ground level (e.g. new multiplayer aircraft are briefly
    at -9999ft). */
    double  fov_v = fabs(angle_v_target - angle_v_ground);
    double  fov_v_deg = fov_v / 3.1415 * 180;

    set_fov( fov_v_deg);

    /* Ensure that we can see entire horizontal extent of the aircraft
    (assuming airplane is horizontal), and also correct things if display
    aspect ratio results in set_fov() not setting the vertical field of view
    that we want. */
    double  fov_h;
    fov_h = 2 * atan(aircraft_size_horizontal / 2 / h_distance);
    double  fov_h_deg = fov_h / 3.1415 * 180;

    double  correction_v = fov_v_deg / get_v_fov();
    double  correction_h = fov_h_deg / get_h_fov();
    double  correction = std::max(correction_v, correction_h);
    if (correction > 1) {
        fov_v_deg *= correction;
        set_fov(fov_v_deg);
    }

    SG_LOG(SG_VIEW, SG_DEBUG, ""
            << " fov_v_deg=" << fov_v_deg
            << " _position=" << _position
            << " _target=" << _target
            << " ground_altitude=" << ground_altitude
            << " relative_height_target_plus=" << relative_height_target_plus
            << " relative_height_ground=" << relative_height_ground
            << " chase_distance_m=" << chase_distance_m
            );
  }
}


/* Views of an aircraft e.g. Helicopter View and Tower View. */
void
View::recalcLookAt ()
{
  /* 2019-06-12: i think maybe all LookAt views have _at_model=true? */
  if ( _at_model ) {
    getAircraftPositionOrientation(
            _target,
            _target_heading_deg,
            _target_pitch_deg,
            _target_roll_deg
            );
  }

  double eye_heading = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-heading-deg-path))");
  double eye_roll    = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-roll-deg-path))");
  double eye_pitch   = ViewPropertyEvaluator::getDoubleValue("((/sim/view[(/sim/current-view/view-number-raw)]/config/root)/(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-pitch-deg-path))");

  setDampTarget(eye_roll, eye_pitch, eye_heading);
  getDampOutput(eye_roll, eye_pitch, eye_heading);

  SGQuatd geodTargetOr = SGQuatd::fromYawPitchRollDeg(_target_heading_deg,
                                                   _target_pitch_deg,
                                                   _target_roll_deg);
  SGQuatd geodTargetHlOr = SGQuatd::fromLonLat(_target);

  SGVec3d target_pos_off;
  getViewOffsets(true /*target_infix*/, NULL /*adjust*/, target_pos_off);
  target_pos_off = SGVec3d(
        -target_pos_off.z(),
        target_pos_off.x(),
        -target_pos_off.y()
        );
  target_pos_off = (geodTargetHlOr*geodTargetOr).backTransform(target_pos_off);

  SGVec3d targetCart = SGVec3d::fromGeod(_target);
  SGVec3d targetCart2 = targetCart + target_pos_off;
  SGGeodesy::SGCartToGeod(targetCart2, _target);

  _position = _target;

  bool  eye_fixed = ViewPropertyEvaluator::getBoolValue("(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-fixed)");
  if (eye_fixed) {
    _position.setLongitudeDeg(
            ViewPropertyEvaluator::getDoubleValue(
                    "((/sim/view[(/sim/current-view/view-number-raw)]/config/root)(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-lon-deg-path))",
                    _position.getLongitudeDeg()
                    )
            );
    _position.setLatitudeDeg(
            ViewPropertyEvaluator::getDoubleValue(
                    "((/sim/view[(/sim/current-view/view-number-raw)]/config/root)(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-lat-deg-path))",
                    _position.getLatitudeDeg()
                    )
            );
    _position.setElevationFt(
            ViewPropertyEvaluator::getDoubleValue(
                    "((/sim/view[(/sim/current-view/view-number-raw)]/config/root)(/sim/view[(/sim/current-view/view-number-raw)]/config/eye-alt-ft-path))",
                    _position.getElevationFt()
                    )
            );
  }

  _target.setLongitudeDeg(
          ViewPropertyEvaluator::getDoubleValue(
                  "((/sim/view[(/sim/current-view/view-number-raw)]/config/root)(/sim/view[(/sim/current-view/view-number-raw)]/config/target-lon-deg-path))",
                  _target.getLongitudeDeg()
                  )
          );
  _target.setLatitudeDeg(
          ViewPropertyEvaluator::getDoubleValue(
                  "((/sim/view[(/sim/current-view/view-number-raw)]/config/root)(/sim/view[(/sim/current-view/view-number-raw)]/config/target-lat-deg-path))",
                  _target.getLatitudeDeg()
                  )
          );
  _target.setElevationFt(
          ViewPropertyEvaluator::getDoubleValue(
                  "((/sim/view[(/sim/current-view/view-number-raw)]/config/root)(/sim/view[(/sim/current-view/view-number-raw)]/config/target-alt-ft-path))",
                  _target.getElevationFt()
                  )
          );

  if (_lookat_agl) {
    handleAGL();
  }
  else {
    set_fov(_fov_user_deg);
  }

  SGQuatd geodEyeOr = SGQuatd::fromYawPitchRollDeg(eye_heading, eye_pitch, eye_roll);
  SGQuatd geodEyeHlOr = SGQuatd::fromLonLat(_position);

  // the rotation offset, don't know why heading is negative here ...
  mViewOffsetOr =
    SGQuatd::fromYawPitchRollDeg(-_heading_offset_deg + 180, _pitch_offset_deg,
                                 _roll_offset_deg);

  // Offsets to the eye position
  getViewOffsets(false /*target_infix*/, &_adjust_offset_m, _offset_m);
  SGVec3d eyeOff(-_offset_m.z(), _offset_m.x(), -_offset_m.y());
  SGQuatd ec2eye = geodEyeHlOr*geodEyeOr;
  SGVec3d eyeCart = SGVec3d::fromGeod(_position);
  eyeCart += (ec2eye*mViewOffsetOr).backTransform(eyeOff);

  SGVec3d atCart = SGVec3d::fromGeod(_target);

  // add target offsets to at_position...
  // Compute the eyepoints orientation and position
  // wrt the earth centered frame - that is global coorinates
  _absolute_view_pos = eyeCart;

  // the view direction
  SGVec3d dir = normalize(atCart - eyeCart);
  // the up directon
  SGVec3d up = ec2eye.backTransform(SGVec3d(0, 0, -1));
  // rotate -dir to the 2-th unit vector
  // rotate up to 1-th unit vector
  // Note that this matches the OpenGL camera coordinate system
  // with x-right, y-up, z-back.
  mViewOrientation = SGQuatd::fromRotateTo(-dir, 2, up, 1);
}

void
View::setDampTarget(double roll, double pitch, double heading)
{
  _dampTarget = SGVec3d(roll, pitch, heading);
}

void
View::getDampOutput(double& roll, double& pitch, double& heading)
{
  roll = _dampOutput[0];
  pitch = _dampOutput[1];
  heading = _dampOutput[2];
}


void
View::updateDampOutput(double dt)
{
  static View *last_view = 0;
  if ((last_view != this) || (dt > 1.0)) {
    _dampOutput = _dampTarget;
    last_view = this;
    return;
  }

  const double interval = 0.01;
  while (dt > interval) {

    for (unsigned int i=0; i<3; ++i) {
      if (_dampFactor[i] <= 0.0) {
        // axis is un-damped, set output to target directly
        _dampOutput[i] = _dampTarget[i];
        continue;
      }

      double d = _dampOutput[i] - _dampTarget[i];
      if (d > 180.0) {
        _dampOutput[i] -= 360.0;
      } else if (d < -180.0) {
        _dampOutput[i] += 360.0;
      }

      _dampOutput[i] = (_dampTarget[i] * _dampFactor[i]) +
        (_dampOutput[i] * (1.0 - _dampFactor[i]));
    } // of axis iteration

    dt -= interval;
  } // of dt subdivision by interval
}


View::Damping::Damping(double damping, double min, double max)
: _id(NULL), _min(min), _max(max), _target(0), _factor(pow(10, -damping)), _current(0)
{
}

void View::Damping::setTarget(double target)
{
  _target = target;
}

void View::Damping::update(double dt, void* id)
{
  if (id != _id || dt > 1.0) {
    _current = _target;
    _id = id;
    return;
  }
  const double interval = 0.01;
  while (dt > interval) {

    if (_factor <= 0.0) {
      // un-damped, set output to target directly
      _current = _target;
      continue;
    }

    double d = _current - _target;
    if (_max > _min) {
        if (d > _max) {
          _current -= (_max - _min);
        } else if (d < _min) {
          _current += (_max - _min);
        }
    }

    _current = (_target * _factor) + _current * (1.0 - _factor);

    dt -= interval;
  }
}

double View::Damping::get()
{
  return _current;
}

void View::Damping::updateTarget(double& io)
{
    setTarget(io);
    io = get();
}

void View::Damping::reset(double target)
{
    _target = target;
    _current = target;
}

double
View::get_h_fov()
{
    double aspectRatio = get_aspect_ratio();
    switch (_scaling_type) {
    case FG_SCALING_WIDTH:  // h_fov == fov
	return _fov_deg;
    case FG_SCALING_MAX:
	if (aspectRatio < 1.0) {
	    // h_fov == fov
	    return _fov_deg;
	} else {
	    // v_fov == fov
	    return
                atan(tan(_fov_deg/2 * SG_DEGREES_TO_RADIANS)
                     / (aspectRatio*_aspect_ratio_multiplier))
                * SG_RADIANS_TO_DEGREES * 2;
	}
    default:
	assert(false);
    }
    return 0.0;
}



double
View::get_v_fov()
{
    double aspectRatio = get_aspect_ratio();
    switch (_scaling_type) {
    case FG_SCALING_WIDTH:  // h_fov == fov
	return
            atan(tan(_fov_deg/2 * SG_DEGREES_TO_RADIANS)
                 * (aspectRatio*_aspect_ratio_multiplier))
            * SG_RADIANS_TO_DEGREES * 2;
    case FG_SCALING_MAX:
	if (aspectRatio < 1.0) {
	    // h_fov == fov
	    return
                atan(tan(_fov_deg/2 * SG_DEGREES_TO_RADIANS)
                     * (aspectRatio*_aspect_ratio_multiplier))
                * SG_RADIANS_TO_DEGREES * 2;
	} else {
	    // v_fov == fov
	    return _fov_deg;
	}
    default:
	assert(false);
    }
    return 0.0;
}

void
View::update (double dt)
{
  recalc();
  updateDampOutput(dt);
  _lookat_agl_damping.update(dt, NULL /*id*/);

  int i;
  int dt_ms = int(dt * 1000);
  for ( i = 0; i < dt_ms; i++ ) {
    if ( fabs( _goal_heading_offset_deg - _heading_offset_deg) < 1 ) {
      setHeadingOffset_deg( _goal_heading_offset_deg );
      break;
    } else {
      // move current_view.headingoffset towards
      // current_view.goal_view_offset
      if ( _goal_heading_offset_deg > _heading_offset_deg )
	{
	  if ( _goal_heading_offset_deg - _heading_offset_deg < 180 ){
	    incHeadingOffset_deg( 0.5 );
	  } else {
	    incHeadingOffset_deg( -0.5 );
	  }
	} else {
	  if ( _heading_offset_deg - _goal_heading_offset_deg < 180 ){
	    incHeadingOffset_deg( -0.5 );
	  } else {
	    incHeadingOffset_deg( 0.5 );
	  }
	}
      if ( _heading_offset_deg > 360 ) {
	incHeadingOffset_deg( -360 );
      } else if ( _heading_offset_deg < 0 ) {
	incHeadingOffset_deg( 360 );
      }
    }
  }

  for ( i = 0; i < dt_ms; i++ ) {
    if ( fabs( _goal_pitch_offset_deg - _pitch_offset_deg ) < 1 ) {
      setPitchOffset_deg( _goal_pitch_offset_deg );
      break;
    } else {
      // move current_view.pitch_offset_deg towards
      // current_view.goal_pitch_offset
      if ( _goal_pitch_offset_deg > _pitch_offset_deg )
	{
	  incPitchOffset_deg( 1.0 );
	} else {
	    incPitchOffset_deg( -1.0 );
	}
      if ( _pitch_offset_deg > 90 ) {
	setPitchOffset_deg(90);
      } else if ( _pitch_offset_deg < -90 ) {
	setPitchOffset_deg( -90 );
      }
    }
  }


  for ( i = 0; i < dt_ms; i++ ) {
    if ( fabs( _goal_roll_offset_deg - _roll_offset_deg ) < 1 ) {
      setRollOffset_deg( _goal_roll_offset_deg );
      break;
    } else {
      // move current_view.roll_offset_deg towards
      // current_view.goal_roll_offset
      if ( _goal_roll_offset_deg > _roll_offset_deg )
	{
	  incRollOffset_deg( 1.0 );
	} else {
	    incRollOffset_deg( -1.0 );
	}
      if ( _roll_offset_deg > 90 ) {
	setRollOffset_deg(90);
      } else if ( _roll_offset_deg < -90 ) {
	setRollOffset_deg( -90 );
      }
    }
  }
  recalc();
}

double View::getAbsolutePosition_x() const
{
    return _absolute_view_pos.x();
}

double View::getAbsolutePosition_y() const
{
    return _absolute_view_pos.y();
}

double View::getAbsolutePosition_z() const
{
    return _absolute_view_pos.z();
}

double View::getRawOrientation_w() const
{
    return mViewOrientation.w();
}

double View::getRawOrientation_x() const
{
    return mViewOrientation.x();
}

double View::getRawOrientation_y() const
{
    return mViewOrientation.y();
}

double View::getRawOrientation_z() const
{
    return mViewOrientation.z();
}

// This takes the conventional aviation XYZ body system
// i.e.  x=forward, y=starboard, z=bottom
// which is widely used in FGFS
// and rotates it into the OpenGL camera system
// i.e. Xprime=starboard, Yprime=top, Zprime=aft.
static const SGQuatd fsb2sta()
{
    return SGQuatd(-0.5, -0.5, 0.5, 0.5);
}

// reference frame orientation.
// This is the view orientation you get when you have no
// view offset, i.e. the offset operator is the identity.
//
// For example, in the familiar "cockpit lookfrom" view,
// the reference frame is equal to the aircraft attitude,
// i.e. it is the view looking towards 12:00 straight ahead.
//
// FIXME:  Somebody needs to figure out what is the reference
// frame view for the other view modes.
//
// Conceptually, this quat represents a rotation relative
// to the ECEF reference orientation, as described at
//    http://www.av8n.com/physics/coords.htm#sec-orientation
//
// See the NOTE concerning reference orientations, below.
//
// The components of this quat are expressed in
// the conventional aviation basis set,
// i.e.  x=forward, y=starboard, z=bottom
double View::getFrame_w() const
{
    return ((mViewOrientation*conj(fsb2sta())*conj(mViewOffsetOr))).w();
}

double View::getFrame_x() const
{
    return ((mViewOrientation*conj(fsb2sta())*conj(mViewOffsetOr))).x();
}

double View::getFrame_y() const
{
    return ((mViewOrientation*conj(fsb2sta())*conj(mViewOffsetOr))).y();
}

double View::getFrame_z() const
{
    return ((mViewOrientation*conj(fsb2sta())*conj(mViewOffsetOr))).z();
}


// view offset.
// This rotation takes you from the aforementioned
// reference frame view orientation to whatever
// actual current view orientation is.
//
// The components of this quaternion are expressed in
// the conventional aviation basis set,
// i.e.  x=forward, y=starboard, z=bottom
double View::getOrOffset_w() const{
    return mViewOffsetOr.w();
}
double View::getOrOffset_x() const{
    return mViewOffsetOr.x();
}
double View::getOrOffset_y() const{
    return mViewOffsetOr.y();
}
double View::getOrOffset_z() const{
    return mViewOffsetOr.z();
}


// current view orientation.
// This is a rotation relative to the earth-centered (ec)
// reference frame.
//
// NOTE: Here we remove a factor of fsb2sta so that
// the components of this quat are displayed using the
// conventional ECEF basis set.  This is *not* the way
// the view orientation is stored in the views[] array,
// but is easier for non-graphics hackers to understand.
// If we did not remove this factor of fsb2sta here and
// in getCurrentViewFrame, that would be equivalent to
// the following peculiar reference orientation:
// Suppose you are over the Gulf of Guinea, at (lat,lon) = (0,0).
// Then the reference frame orientation can be achieved via:
//    -- The aircraft X-axis (nose) headed south.
//    -- The aircraft Y-axis (starboard wingtip) pointing up.
//    -- The aircraft Z-axis (belly) pointing west.
// To say the same thing in other words, and perhaps more to the
// point:  If we use the OpenGL camera orientation conventions,
// i.e. Xprime=starboard, Yprime=top, Zprime=aft, then the
// aforementioned peculiar reference orientation at (lat,lon)
//  = (0,0) can be described as:
//    -- aircraft Xprime axis (starboard) pointed up
//    -- aircraft Yprime axis (top) pointed east
//    -- aircraft Zprime axis (aft) pointed north
// meaning the OpenGL axes are aligned with the ECEF axes.
double View::getOrientation_w() const{
    return (mViewOrientation * conj(fsb2sta())).w();
}
double View::getOrientation_x() const{
    return (mViewOrientation * conj(fsb2sta())).x();
}
double View::getOrientation_y() const{
    return (mViewOrientation * conj(fsb2sta())).y();
}
double View::getOrientation_z() const{
    return (mViewOrientation * conj(fsb2sta())).z();
}

double View::get_aspect_ratio() const
{
    return flightgear::CameraGroup::getDefault()->getMasterAspectRatio();
}

double View::getLon_deg() const
{
    return _position.getLongitudeDeg();
}

double View::getLat_deg() const
{
    return _position.getLatitudeDeg();
}

double View::getElev_ft() const
{
    return _position.getElevationFt();
}

// Register the subsystem.
#if 0
SGSubsystemMgr::Registrant<View> registrantView;
#endif
