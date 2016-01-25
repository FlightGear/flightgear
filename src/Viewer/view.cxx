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

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include "CameraGroup.hxx"

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
                    double target_z_offset_m, double near_m, bool internal ):
    _dirty(true),
    _roll_deg(0),
    _pitch_deg(0),
    _heading_deg(0),
    _target_roll_deg(0),
    _target_pitch_deg(0),
    _target_heading_deg(0),
    _scaling_type(FG_SCALING_MAX)
{
    _absolute_view_pos = SGVec3d(0, 0, 0);
    _type = Type;
    _from_model = from_model;
    _from_model_index = from_model_index;
    _at_model = at_model;
    _at_model_index = at_model_index;

    _internal = internal;

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

    _aspect_ratio_multiplier = aspect_ratio_multiplier;
    _target_offset_m.x() = target_x_offset_m;
    _target_offset_m.y() = target_y_offset_m;
    _target_offset_m.z() = target_z_offset_m;
    _configTargetOffset_m = _target_offset_m;

    _ground_level_nearplane_m = near_m;
    // a reasonable guess for init, so that the math doesn't blow up
}

View* View::createFromProperties(SGPropertyNode_ptr config)
{
    double aspect_ratio_multiplier
        = fgGetDouble("/sim/current-view/aspect-ratio-multiplier");

    // find out if this is an internal view (e.g. in cockpit, low near plane)
    // FIXME : should be a child of config
    bool internal = config->getParent()->getBoolValue("internal", false);


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

    View* v = 0;
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

        v = new View ( FG_LOOKAT, from_model, from_model_index,
                           at_model, at_model_index,
                           damp_roll, damp_pitch, damp_heading,
                           x_offset_m, y_offset_m,z_offset_m,
                           heading_offset_deg, pitch_offset_deg,
                           roll_offset_deg, fov_deg, aspect_ratio_multiplier,
                           target_x_offset_m, target_y_offset_m,
                         target_z_offset_m, near_m, internal );
        if (!from_model) {
            v->_targetProperties.init(config, "target-");
        }
    } else {
        v = new View ( FG_LOOKFROM, from_model, from_model_index,
                       false, 0, 0.0, 0.0, 0.0,
                       x_offset_m, y_offset_m, z_offset_m,
                       heading_offset_deg, pitch_offset_deg,
                       roll_offset_deg, fov_deg, aspect_ratio_multiplier,
                         0, 0, 0, near_m, internal );
    }

    if (!from_model) {
        v->_eyeProperties.init(config, "eye-");
    }

    v->_name = config->getParent()->getStringValue("name");
    v->_typeString = type;
    v->_configHeadingOffsetDeg = config->getDoubleValue("default-heading-offset-deg");

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
                        &View::get_fov, &View::set_fov,
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

    _tiedProperties.Tie("x-offset-m", this, &View::getXOffset_m,
                        &View::setXOffset_m, false);
    _tiedProperties.Tie("y-offset-m", this, &View::getYOffset_m,
                        &View::setYOffset_m, false);
    _tiedProperties.Tie("z-offset-m", this, &View::getZOffset_m,
                        &View::setZOffset_m, false);

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

// recalculate for LookFrom view type...
void
View::recalcLookFrom ()
{
  // Update location data ...
  if ( _from_model ) {
    _position = globals->get_aircraft_position();
    globals->get_aircraft_orientation(_heading_deg, _pitch_deg, _roll_deg);
  }

  double head = _heading_deg;
  double pitch = _pitch_deg;
  double roll = _roll_deg;
  if ( !_from_model ) {
    // update from our own data...
    setDampTarget(roll, pitch, head);
    getDampOutput(roll, pitch, head);
  }

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

  _absolute_view_pos = position + (ec2body*q).backTransform(_offset_m);
  mViewOrientation = ec2body*mViewOffsetOr*q;
}

void
View::recalcLookAt ()
{
  // The geodetic position of our target to look at
  if ( _at_model ) {
    _target = globals->get_aircraft_position();
    globals->get_aircraft_orientation(_target_heading_deg,
                                      _target_pitch_deg,
                                      _target_roll_deg);
  } else {
    // if not model then calculate our own target position...
    setDampTarget(_target_roll_deg, _target_pitch_deg, _target_heading_deg);
    getDampOutput(_target_roll_deg, _target_pitch_deg, _target_heading_deg);
  }

  SGQuatd geodTargetOr = SGQuatd::fromYawPitchRollDeg(_target_heading_deg,
                                                   _target_pitch_deg,
                                                   _target_roll_deg);
  SGQuatd geodTargetHlOr = SGQuatd::fromLonLat(_target);


  if ( _from_model ) {
    _position = globals->get_aircraft_position();
    globals->get_aircraft_orientation(_heading_deg, _pitch_deg, _roll_deg);
  } else {
    // update from our own data, just the rotation here...
    setDampTarget(_roll_deg, _pitch_deg, _heading_deg);
    getDampOutput(_roll_deg, _pitch_deg, _heading_deg);
  }
  SGQuatd geodEyeOr = SGQuatd::fromYawPitchRollDeg(_heading_deg, _pitch_deg, _roll_deg);
  SGQuatd geodEyeHlOr = SGQuatd::fromLonLat(_position);

  // the rotation offset, don't know why heading is negative here ...
  mViewOffsetOr =
    SGQuatd::fromYawPitchRollDeg(-_heading_offset_deg + 180, _pitch_offset_deg,
                                 _roll_offset_deg);

  // Offsets to the eye position
  SGVec3d eyeOff(-_offset_m.z(), _offset_m.x(), -_offset_m.y());
  SGQuatd ec2eye = geodEyeHlOr*geodEyeOr;
  SGVec3d eyeCart = SGVec3d::fromGeod(_position);
  eyeCart += (ec2eye*mViewOffsetOr).backTransform(eyeOff);

  SGVec3d atCart = SGVec3d::fromGeod(_target);

  // add target offsets to at_position...
  SGVec3d target_pos_off(-_target_offset_m.z(), _target_offset_m.x(),
                         -_target_offset_m.y());
  target_pos_off = (geodTargetHlOr*geodTargetOr).backTransform(target_pos_off);
  atCart += target_pos_off;
  eyeCart += target_pos_off;

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
View::updateData()
{
    if (!_from_model) {
        SGGeod pos = _eyeProperties.position();
        SGVec3d att = _eyeProperties.attitude();
        setPosition(pos);
        setOrientation(att[2], att[1], att[0]);
    } else {
        set_dirty();
    }

    // if lookat (type 1) then get target data...
    if (getType() == FG_LOOKAT) {
        if (!_from_model) {
            SGGeod pos = _targetProperties.position();
            SGVec3d att = _targetProperties.attitude();
            setTargetPosition(pos);
            setTargetOrientation(att[2], att[1], att[0]);
        } else {
            set_dirty();
        }
    }
}

void
View::update (double dt)
{
    updateData();
  updateDampOutput(dt);

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

View::PositionAttitudeProperties::PositionAttitudeProperties()
{
}

View::PositionAttitudeProperties::~PositionAttitudeProperties()
{
}

void View::PositionAttitudeProperties::init(SGPropertyNode_ptr parent, const std::string& prefix)
{
    _lonPathProp = parent->getNode(prefix + "lon-deg-path", true);
    _latPathProp = parent->getNode(prefix + "lat-deg-path", true);
    _altPathProp = parent->getNode(prefix + "alt-ft-path", true);
    _headingPathProp = parent->getNode(prefix + "heading-deg-path", true);
    _pitchPathProp = parent->getNode(prefix + "pitch-deg-path", true);
    _rollPathProp = parent->getNode(prefix + "roll-deg-path", true);

    // update the real properties now
    valueChanged(NULL);

    _lonPathProp->addChangeListener(this);
    _latPathProp->addChangeListener(this);
    _altPathProp->addChangeListener(this);
    _headingPathProp->addChangeListener(this);
    _pitchPathProp->addChangeListener(this);
    _rollPathProp->addChangeListener(this);
}

void View::PositionAttitudeProperties::valueChanged(SGPropertyNode* node)
{
    _lonProp = resolvePathProperty(_lonPathProp);
    _latProp = resolvePathProperty(_latPathProp);
    _altProp = resolvePathProperty(_altPathProp);
    _headingProp = resolvePathProperty(_headingPathProp);
    _pitchProp = resolvePathProperty(_pitchPathProp);
    _rollProp = resolvePathProperty(_rollPathProp);
}

SGPropertyNode_ptr View::PositionAttitudeProperties::resolvePathProperty(SGPropertyNode_ptr p)
{
    if (!p)
        return SGPropertyNode_ptr();

    std::string path = p->getStringValue();
    if (path.empty())
        return SGPropertyNode_ptr();

    return fgGetNode(path, true);
}

SGGeod View::PositionAttitudeProperties::position() const
{
    double lon = _lonProp ? _lonProp->getDoubleValue() : 0.0;
    double lat = _latProp ? _latProp->getDoubleValue() : 0.0;
    double alt = _altProp ? _altProp->getDoubleValue() : 0.0;
    return SGGeod::fromDegFt(lon, lat, alt);
}

SGVec3d View::PositionAttitudeProperties::attitude() const
{
    double heading = _headingProp ? _headingProp->getDoubleValue() : 0.0;
    double pitch = _pitchProp ? _pitchProp->getDoubleValue() : 0.0;
    double roll = _rollProp ? _rollProp->getDoubleValue() : 0.0;
    return SGVec3d(heading, pitch, roll);
}
