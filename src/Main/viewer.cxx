// viewer.cxx -- class for managing a viewer in the flightgear world.
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

#include <simgear/compiler.h>

#include "fg_props.hxx"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/scene/model/location.hxx>
#include <simgear/scene/model/placement.hxx>
#include <simgear/math/vector.hxx>

#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Model/acmodel.hxx>

#include "viewer.hxx"


////////////////////////////////////////////////////////////////////////
// Implementation of FGViewer.
////////////////////////////////////////////////////////////////////////

// Constructor...
FGViewer::FGViewer( fgViewType Type, bool from_model, int from_model_index,
                    bool at_model, int at_model_index,
                    double damp_roll, double damp_pitch, double damp_heading,
                    double x_offset_m, double y_offset_m, double z_offset_m,
                    double heading_offset_deg, double pitch_offset_deg,
                    double roll_offset_deg,
                    double fov_deg, double aspect_ratio_multiplier,
                    double target_x_offset_m, double target_y_offset_m,
                    double target_z_offset_m, double near_m, bool internal ):
    _dirty(true),
    _lon_deg(0),
    _lat_deg(0),
    _alt_ft(0),
    _target_lon_deg(0),
    _target_lat_deg(0),
    _target_alt_ft(0),
    _roll_deg(0),
    _pitch_deg(0),
    _heading_deg(0),
    _damp_sync(0),
    _damp_roll(0),
    _damp_pitch(0),
    _damp_heading(0),
    _scaling_type(FG_SCALING_MAX),
    _location(0),
    _target_location(0)
{
    _absolute_view_pos = SGVec3d(0, 0, 0);
    _type = Type;
    _from_model = from_model;
    _from_model_index = from_model_index;
    _at_model = at_model;
    _at_model_index = at_model_index;

    _internal = internal;

    if (damp_roll > 0.0)
      _damp_roll = 1.0 / pow(10.0, fabs(damp_roll));
    if (damp_pitch > 0.0)
      _damp_pitch = 1.0 / pow(10.0, fabs(damp_pitch));
    if (damp_heading > 0.0)
      _damp_heading = 1.0 / pow(10.0, fabs(damp_heading));

    _offset_m.x() = x_offset_m;
    _offset_m.y() = y_offset_m;
    _offset_m.z() = z_offset_m;
    _heading_offset_deg = heading_offset_deg;
    _pitch_offset_deg = pitch_offset_deg;
    _roll_offset_deg = roll_offset_deg;
    _goal_heading_offset_deg = heading_offset_deg;
    _goal_pitch_offset_deg = pitch_offset_deg;
    _goal_roll_offset_deg = roll_offset_deg;
    if (fov_deg > 0) {
      _fov_deg = fov_deg;
    } else {
      _fov_deg = 55;
    }
    _aspect_ratio_multiplier = aspect_ratio_multiplier;
    _target_offset_m.x() = target_x_offset_m;
    _target_offset_m.y() = target_y_offset_m;
    _target_offset_m.z() = target_z_offset_m;
    _ground_level_nearplane_m = near_m;
    // a reasonable guess for init, so that the math doesn't blow up
}


// Destructor
FGViewer::~FGViewer( void ) {
}

void
FGViewer::init ()
{
  if ( _from_model )
    _location = (SGLocation *) globals->get_aircraft_model()->get3DModel()->getSGLocation();
  else
    _location = new SGLocation;

  if ( _type == FG_LOOKAT ) {
    if ( _at_model )
      _target_location = (SGLocation *) globals->get_aircraft_model()->get3DModel()->getSGLocation();
    else
      _target_location = (SGLocation *) new SGLocation;
  }
}

void
FGViewer::bind ()
{
}

void
FGViewer::unbind ()
{
}

void
FGViewer::setType ( int type )
{
  if (type == 0)
    _type = FG_LOOKFROM;
  if (type == 1)
    _type = FG_LOOKAT;
}

void
FGViewer::setInternal ( bool internal )
{
  _internal = internal;
}

void
FGViewer::setLongitude_deg (double lon_deg)
{
  _dirty = true;
  _lon_deg = lon_deg;
}

void
FGViewer::setLatitude_deg (double lat_deg)
{
  _dirty = true;
  _lat_deg = lat_deg;
}

void
FGViewer::setAltitude_ft (double alt_ft)
{
  _dirty = true;
  _alt_ft = alt_ft;
}

void
FGViewer::setPosition (double lon_deg, double lat_deg, double alt_ft)
{
  _dirty = true;
  _lon_deg = lon_deg;
  _lat_deg = lat_deg;
  _alt_ft = alt_ft;
}

void
FGViewer::setTargetLongitude_deg (double lon_deg)
{
  _dirty = true;
  _target_lon_deg = lon_deg;
}

void
FGViewer::setTargetLatitude_deg (double lat_deg)
{
  _dirty = true;
  _target_lat_deg = lat_deg;
}

void
FGViewer::setTargetAltitude_ft (double alt_ft)
{
  _dirty = true;
  _target_alt_ft = alt_ft;
}

void
FGViewer::setTargetPosition (double lon_deg, double lat_deg, double alt_ft)
{
  _dirty = true;
  _target_lon_deg = lon_deg;
  _target_lat_deg = lat_deg;
  _target_alt_ft = alt_ft;
}

void
FGViewer::setRoll_deg (double roll_deg)
{
  _dirty = true;
  _roll_deg = roll_deg;
}

void
FGViewer::setPitch_deg (double pitch_deg)
{
  _dirty = true;
  _pitch_deg = pitch_deg;
}

void
FGViewer::setHeading_deg (double heading_deg)
{
  _dirty = true;
  _heading_deg = heading_deg;
}

void
FGViewer::setOrientation (double roll_deg, double pitch_deg, double heading_deg)
{
  _dirty = true;
  _roll_deg = roll_deg;
  _pitch_deg = pitch_deg;
  _heading_deg = heading_deg;
}

void
FGViewer::setTargetRoll_deg (double target_roll_deg)
{
  _dirty = true;
  _target_roll_deg = target_roll_deg;
}

void
FGViewer::setTargetPitch_deg (double target_pitch_deg)
{
  _dirty = true;
  _target_pitch_deg = target_pitch_deg;
}

void
FGViewer::setTargetHeading_deg (double target_heading_deg)
{
  _dirty = true;
  _target_heading_deg = target_heading_deg;
}

void
FGViewer::setTargetOrientation (double target_roll_deg, double target_pitch_deg, double target_heading_deg)
{
  _dirty = true;
  _target_roll_deg = target_roll_deg;
  _target_pitch_deg = target_pitch_deg;
  _target_heading_deg = target_heading_deg;
}

void
FGViewer::setXOffset_m (double x_offset_m)
{
  _dirty = true;
  _offset_m.x() = x_offset_m;
}

void
FGViewer::setYOffset_m (double y_offset_m)
{
  _dirty = true;
  _offset_m.y() = y_offset_m;
}

void
FGViewer::setZOffset_m (double z_offset_m)
{
  _dirty = true;
  _offset_m.z() = z_offset_m;
}

void
FGViewer::setTargetXOffset_m (double target_x_offset_m)
{
  _dirty = true;
  _target_offset_m.x() = target_x_offset_m;
}

void
FGViewer::setTargetYOffset_m (double target_y_offset_m)
{
  _dirty = true;
  _target_offset_m.y() = target_y_offset_m;
}

void
FGViewer::setTargetZOffset_m (double target_z_offset_m)
{
  _dirty = true;
  _target_offset_m.z() = target_z_offset_m;
}

void
FGViewer::setPositionOffsets (double x_offset_m, double y_offset_m, double z_offset_m)
{
  _dirty = true;
  _offset_m.x() = x_offset_m;
  _offset_m.y() = y_offset_m;
  _offset_m.z() = z_offset_m;
}

void
FGViewer::setRollOffset_deg (double roll_offset_deg)
{
  _dirty = true;
  _roll_offset_deg = roll_offset_deg;
}

void
FGViewer::setPitchOffset_deg (double pitch_offset_deg)
{
  _dirty = true;
  _pitch_offset_deg = pitch_offset_deg;
}

void
FGViewer::setHeadingOffset_deg (double heading_offset_deg)
{
  _dirty = true;
  _heading_offset_deg = heading_offset_deg;
}

void
FGViewer::setGoalRollOffset_deg (double goal_roll_offset_deg)
{
  _dirty = true;
  _goal_roll_offset_deg = goal_roll_offset_deg;
}

void
FGViewer::setGoalPitchOffset_deg (double goal_pitch_offset_deg)
{
  _dirty = true;
  _goal_pitch_offset_deg = goal_pitch_offset_deg;
  if ( _goal_pitch_offset_deg < -90 ) {
    _goal_pitch_offset_deg = -90.0;
  }
  if ( _goal_pitch_offset_deg > 90.0 ) {
    _goal_pitch_offset_deg = 90.0;
  }

}

void
FGViewer::setGoalHeadingOffset_deg (double goal_heading_offset_deg)
{
  _dirty = true;
  _goal_heading_offset_deg = goal_heading_offset_deg;
  while ( _goal_heading_offset_deg < 0.0 ) {
    _goal_heading_offset_deg += 360;
  }
  while ( _goal_heading_offset_deg > 360 ) {
    _goal_heading_offset_deg -= 360;
  }
}

void
FGViewer::setOrientationOffsets (double roll_offset_deg, double pitch_offset_deg, double heading_offset_deg)
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
FGViewer::recalc ()
{
  if (_type == FG_LOOKFROM) {
    recalcLookFrom();
  } else {
    recalcLookAt();
  }

  SGGeod geodEyePoint = SGGeod::fromCart(_absolute_view_pos);
  geodEyePoint.setElevationM(0);
  _zero_elev = SGVec3d::fromGeod(geodEyePoint);
  
  SGQuatd hlOr = SGQuatd::fromLonLat(geodEyePoint);
  _surface_south = toVec3f(hlOr.backTransform(-SGVec3d::e1()));
  _surface_east = toVec3f(hlOr.backTransform(SGVec3d::e2()));
  _world_up = toVec3f(hlOr.backTransform(-SGVec3d::e3()));

  // Update viewer's postion data for the eye location...
  _lon_deg = _location->getLongitude_deg();
  _lat_deg = _location->getLatitude_deg();
  _alt_ft = _location->getAltitudeASL_ft();
  _roll_deg = _location->getRoll_deg();
  _pitch_deg = _location->getPitch_deg();
  _heading_deg = _location->getHeading_deg();

  // Update viewer's postion data for the target (at object) location
  if (_type == FG_LOOKAT) {
    _target_lon_deg = _target_location->getLongitude_deg();
    _target_lat_deg = _target_location->getLatitude_deg();
    _target_alt_ft = _target_location->getAltitudeASL_ft();
    _target_roll_deg = _target_location->getRoll_deg();
    _target_pitch_deg = _target_location->getPitch_deg();
    _target_heading_deg = _target_location->getHeading_deg();
  }

  set_clean();
}

// recalculate for LookFrom view type...
void
FGViewer::recalcLookFrom ()
{
  // Update location data ...
  if ( !_from_model ) {
    _location->setPosition( _lon_deg, _lat_deg, _alt_ft );
    _location->setOrientation( _roll_deg, _pitch_deg, _heading_deg );
    _location->getTransformMatrix();
  }
  double lat = _location->getLatitude_deg();
  double lon = _location->getLongitude_deg();
  double alt = _location->getAltitudeASL_ft();
  double head = _location->getHeading_deg();
  double pitch = _location->getPitch_deg();
  double roll = _location->getRoll_deg();
  if ( !_from_model ) {
    // update from our own data...
    dampEyeData(roll, pitch, head);
  }

  // The geodetic position of our base view position
  SGGeod geodPos = SGGeod::fromDegFt(lon, lat, alt);
  // The rotation rotating from the earth centerd frame to
  // the horizontal local OpenGL frame
  SGQuatd hlOr = SGQuatd::viewHL(geodPos);

  // the rotation from the horizontal local frame to the basic view orientation
  SGQuatd hlToBody = SGQuatd::fromYawPitchRollDeg(head, pitch, roll);
  hlToBody = SGQuatd::simToView(hlToBody);

  // The cartesian position of the basic view coordinate
  SGVec3d position = SGVec3d::fromGeod(geodPos);
  // the rotation offset, don't know why heading is negative here ...
  SGQuatd viewOffsetOr = SGQuatd::simToView(
    SGQuatd::fromYawPitchRollDeg(-_heading_offset_deg, _pitch_offset_deg,
                                 _roll_offset_deg));

  // Compute the eyepoints orientation and position
  // wrt the earth centered frame - that is global coorinates
  SGQuatd ec2body = hlOr*hlToBody;
  _absolute_view_pos = position + ec2body.backTransform(_offset_m);
  mViewOrientation = ec2body*viewOffsetOr;
}

void
FGViewer::recalcLookAt ()
{
  // The geodetic position of our target to look at
  SGGeod geodTargetPos;
  SGQuatd geodTargetOr;
  if ( _at_model ) {
    geodTargetPos = SGGeod::fromDegFt(_target_location->getLongitude_deg(),
                                      _target_location->getLatitude_deg(),
                                      _target_location->getAltitudeASL_ft());
    double head = _target_location->getHeading_deg();
    double pitch = _target_location->getPitch_deg();
    double roll = _target_location->getRoll_deg();
    geodTargetOr = SGQuatd::fromYawPitchRollDeg(head, pitch, roll);
  } else {
    dampEyeData(_target_roll_deg, _target_pitch_deg, _target_heading_deg);
    _target_location->setPosition( _target_lon_deg, _target_lat_deg, _target_alt_ft );
    _target_location->setOrientation( _target_roll_deg, _target_pitch_deg, _target_heading_deg );
    _target_location->getTransformMatrix();

    // if not model then calculate our own target position...
    geodTargetPos = SGGeod::fromDegFt(_target_lon_deg,
                                      _target_lat_deg,
                                      _target_alt_ft);
    geodTargetOr = SGQuatd::fromYawPitchRollDeg(_target_heading_deg,
                                                _target_pitch_deg,
                                                _target_roll_deg);
  }
  SGQuatd geodTargetHlOr = SGQuatd::fromLonLat(geodTargetPos);


  SGGeod geodEyePos;
  SGQuatd geodEyeOr;
  if ( _from_model ) {
    geodEyePos = SGGeod::fromDegFt(_location->getLongitude_deg(),
                                   _location->getLatitude_deg(),
                                   _location->getAltitudeASL_ft());
    double head = _location->getHeading_deg();
    double pitch = _location->getPitch_deg();
    double roll = _location->getRoll_deg();
    geodEyeOr = SGQuatd::fromYawPitchRollDeg(head, pitch, roll);
  } else {
    dampEyeData(_roll_deg, _pitch_deg, _heading_deg);
    _location->setPosition( _lon_deg, _lat_deg, _alt_ft );
    _location->setOrientation( _roll_deg, _pitch_deg, _heading_deg );
    _location->getTransformMatrix();

    // update from our own data, just the rotation here...
    geodEyePos = SGGeod::fromDegFt(_lon_deg, _lat_deg, _alt_ft);
    geodEyeOr = SGQuatd::fromYawPitchRollDeg(_heading_deg,
                                             _pitch_deg,
                                             _roll_deg);
  }
  SGQuatd geodEyeHlOr = SGQuatd::fromLonLat(geodEyePos);

  // the rotation offset, don't know why heading is negative here ...
  SGQuatd eyeOffsetOr =
    SGQuatd::fromYawPitchRollDeg(-_heading_offset_deg + 180, _pitch_offset_deg,
                                 _roll_offset_deg);

  // Offsets to the eye position
  SGVec3d eyeOff(-_offset_m.z(), _offset_m.x(), -_offset_m.y());
  SGQuatd ec2eye = geodEyeHlOr*geodEyeOr;
  SGVec3d eyeCart = SGVec3d::fromGeod(geodEyePos);
  eyeCart += (ec2eye*eyeOffsetOr).backTransform(eyeOff);

  SGVec3d atCart = SGVec3d::fromGeod(geodTargetPos);

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
  // rotate dir to the 0-th unit vector
  // rotate up to 2-th unit vector
  mViewOrientation = SGQuatd::fromRotateTo(-dir, 2, up, 1);
}

void
FGViewer::dampEyeData(double &roll_deg, double &pitch_deg, double &heading_deg)
{
  const double interval = 0.01;

  static FGViewer *last_view = 0;
  if (last_view != this) {
    _damp_sync = 0.0;
    _damped_roll_deg = roll_deg;
    _damped_pitch_deg = pitch_deg;
    _damped_heading_deg = heading_deg;
    last_view = this;
    return;
  }

  if (_damp_sync < interval) {
    if (_damp_roll > 0.0)
      roll_deg = _damped_roll_deg;
    if (_damp_pitch > 0.0)
      pitch_deg = _damped_pitch_deg;
    if (_damp_heading > 0.0)
      heading_deg = _damped_heading_deg;
    return;
  }

  while (_damp_sync >= interval) {
    _damp_sync -= interval;

    double d;
    if (_damp_roll > 0.0) {
      d = _damped_roll_deg - roll_deg;
      if (d >= 180.0)
        _damped_roll_deg -= 360.0;
      else if (d < -180.0)
        _damped_roll_deg += 360.0;
      roll_deg = _damped_roll_deg = roll_deg * _damp_roll + _damped_roll_deg * (1 - _damp_roll);
    }

    if (_damp_pitch > 0.0) {
      d = _damped_pitch_deg - pitch_deg;
      if (d >= 180.0)
        _damped_pitch_deg -= 360.0;
      else if (d < -180.0)
        _damped_pitch_deg += 360.0;
      pitch_deg = _damped_pitch_deg = pitch_deg * _damp_pitch + _damped_pitch_deg * (1 - _damp_pitch);
    }

    if (_damp_heading > 0.0) {
      d = _damped_heading_deg - heading_deg;
      if (d >= 180.0)
        _damped_heading_deg -= 360.0;
      else if (d < -180.0)
        _damped_heading_deg += 360.0;
      heading_deg = _damped_heading_deg = heading_deg * _damp_heading + _damped_heading_deg * (1 - _damp_heading);
    }
  }
}

double
FGViewer::get_h_fov()
{
    switch (_scaling_type) {
    case FG_SCALING_WIDTH:  // h_fov == fov
	return _fov_deg;
    case FG_SCALING_MAX:
	if (_aspect_ratio < 1.0) {
	    // h_fov == fov
	    return _fov_deg;
	} else {
	    // v_fov == fov
	    return
                atan(tan(_fov_deg/2 * SG_DEGREES_TO_RADIANS)
                     / (_aspect_ratio*_aspect_ratio_multiplier))
                * SG_RADIANS_TO_DEGREES * 2;
	}
    default:
	assert(false);
    }
    return 0.0;
}



double
FGViewer::get_v_fov()
{
    switch (_scaling_type) {
    case FG_SCALING_WIDTH:  // h_fov == fov
	return 
            atan(tan(_fov_deg/2 * SG_DEGREES_TO_RADIANS)
                 * (_aspect_ratio*_aspect_ratio_multiplier))
            * SG_RADIANS_TO_DEGREES * 2;
    case FG_SCALING_MAX:
	if (_aspect_ratio < 1.0) {
	    // h_fov == fov
	    return
                atan(tan(_fov_deg/2 * SG_DEGREES_TO_RADIANS)
                     * (_aspect_ratio*_aspect_ratio_multiplier))
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
FGViewer::update (double dt)
{
  _damp_sync += dt;

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

}
