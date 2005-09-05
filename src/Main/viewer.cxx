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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


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


//////////////////////////////////////////////////////////////////
// Norman's Optimized matrix rotators!                          //
//////////////////////////////////////////////////////////////////


// Since these are pure rotation matrices we can save some bookwork
// by considering them to be 3x3 until the very end -- NHV
static void MakeVIEW_OFFSET( sgMat4 dst,
                      const float angle1, const sgVec3 axis1,
                      const float angle2, const sgVec3 axis2,
                      const float angle3, const sgVec3 axis3 )
{
    // make rotmatrix1 from angle and axis
    float s = (float) sin ( angle1 ) ;
    float c = (float) cos ( angle1 ) ;
    float t = SG_ONE - c ;

    sgMat3 mat1;
    float tmp = t * axis1[0];
    mat1[0][0] = tmp * axis1[0] + c ;
    mat1[0][1] = tmp * axis1[1] + s * axis1[2] ;
    mat1[0][2] = tmp * axis1[2] - s * axis1[1] ;

    tmp = t * axis1[1];
    mat1[1][0] = tmp * axis1[0] - s * axis1[2] ;
    mat1[1][1] = tmp * axis1[1] + c ;
    mat1[1][2] = tmp * axis1[2] + s * axis1[0] ;

    tmp = t * axis1[2];
    mat1[2][0] = tmp * axis1[0] + s * axis1[1] ;
    mat1[2][1] = tmp * axis1[1] - s * axis1[0] ;
    mat1[2][2] = tmp * axis1[2] + c ;

    // make rotmatrix2 from angle and axis
    s = (float) sin ( angle2 ) ;
    c = (float) cos ( angle2 ) ;
    t = SG_ONE - c ;

    sgMat3 mat2;
    tmp = t * axis2[0];
    mat2[0][0] = tmp * axis2[0] + c ;
    mat2[0][1] = tmp * axis2[1] + s * axis2[2] ;
    mat2[0][2] = tmp * axis2[2] - s * axis2[1] ;

    tmp = t * axis2[1];
    mat2[1][0] = tmp * axis2[0] - s * axis2[2] ;
    mat2[1][1] = tmp * axis2[1] + c ;
    mat2[1][2] = tmp * axis2[2] + s * axis2[0] ;

    tmp = t * axis2[2];
    mat2[2][0] = tmp * axis2[0] + s * axis2[1] ;
    mat2[2][1] = tmp * axis2[1] - s * axis2[0] ;
    mat2[2][2] = tmp * axis2[2] + c ;


    // make rotmatrix3 from angle and axis (roll)
    s = (float) sin ( angle3 ) ;
    c = (float) cos ( angle3 ) ;
    t = SG_ONE - c ;

    sgMat3 mat3;
    tmp = t * axis3[0];
    mat3[0][0] = tmp * axis3[0] + c ;
    mat3[0][1] = tmp * axis3[1] + s * axis3[2] ;
    mat3[0][2] = tmp * axis3[2] - s * axis3[1] ;

    tmp = t * axis3[1];
    mat3[1][0] = tmp * axis3[0] - s * axis3[2] ;
    mat3[1][1] = tmp * axis3[1] + c ;
    mat3[1][2] = tmp * axis3[2] + s * axis3[0] ;

    tmp = t * axis3[2];
    mat3[2][0] = tmp * axis3[0] + s * axis3[1] ;
    mat3[2][1] = tmp * axis3[1] - s * axis3[0] ;
    mat3[2][2] = tmp * axis3[2] + c ;

    sgMat3 matt;

    // multiply matrices
    for ( int j = 0 ; j < 3 ; j++ ) {
        matt[0][j] = mat2[0][0] * mat1[0][j] +
                    mat2[0][1] * mat1[1][j] +
                    mat2[0][2] * mat1[2][j];

        matt[1][j] = mat2[1][0] * mat1[0][j] +
                    mat2[1][1] * mat1[1][j] +
                    mat2[1][2] * mat1[2][j];

        matt[2][j] = mat2[2][0] * mat1[0][j] +
                    mat2[2][1] * mat1[1][j] +
                    mat2[2][2] * mat1[2][j];
    }

    // multiply matrices
    for ( int j = 0 ; j < 3 ; j++ ) {
        dst[0][j] = mat3[0][0] * matt[0][j] +
                    mat3[0][1] * matt[1][j] +
                    mat3[0][2] * matt[2][j];

        dst[1][j] = mat3[1][0] * matt[0][j] +
                    mat3[1][1] * matt[1][j] +
                    mat3[1][2] * matt[2][j];

        dst[2][j] = mat3[2][0] * matt[0][j] +
                    mat3[2][1] * matt[1][j] +
                    mat3[2][2] * matt[2][j];
    }
    // fill in 4x4 matrix elements
    dst[0][3] = SG_ZERO; 
    dst[1][3] = SG_ZERO; 
    dst[2][3] = SG_ZERO;
    dst[3][0] = SG_ZERO;
    dst[3][1] = SG_ZERO;
    dst[3][2] = SG_ZERO;
    dst[3][3] = SG_ONE;
}


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
    _scaling_type(FG_SCALING_MAX)
{
    sgdZeroVec3(_absolute_view_pos);
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

    _x_offset_m = x_offset_m;
    _y_offset_m = y_offset_m;
    _z_offset_m = z_offset_m;
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
    _target_x_offset_m = target_x_offset_m;
    _target_y_offset_m = target_y_offset_m;
    _target_z_offset_m = target_z_offset_m;
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
    _location = (SGLocation *) new SGLocation;

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
  _x_offset_m = x_offset_m;
}

void
FGViewer::setYOffset_m (double y_offset_m)
{
  _dirty = true;
  _y_offset_m = y_offset_m;
}

void
FGViewer::setZOffset_m (double z_offset_m)
{
  _dirty = true;
  _z_offset_m = z_offset_m;
}

void
FGViewer::setTargetXOffset_m (double target_x_offset_m)
{
  _dirty = true;
  _target_x_offset_m = target_x_offset_m;
}

void
FGViewer::setTargetYOffset_m (double target_y_offset_m)
{
  _dirty = true;
  _target_y_offset_m = target_y_offset_m;
}

void
FGViewer::setTargetZOffset_m (double target_z_offset_m)
{
  _dirty = true;
  _target_z_offset_m = target_z_offset_m;
}

void
FGViewer::setPositionOffsets (double x_offset_m, double y_offset_m, double z_offset_m)
{
  _dirty = true;
  _x_offset_m = x_offset_m;
  _y_offset_m = y_offset_m;
  _z_offset_m = z_offset_m;
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

double *
FGViewer::get_absolute_view_pos () 
{
  if (_dirty)
    recalc();
  return _absolute_view_pos;
}

float *
FGViewer::getRelativeViewPos () 
{
  if (_dirty)
    recalc();
  return _relative_view_pos;
}

void
FGViewer::updateFromModelLocation (SGLocation * location)
{
  sgCopyMat4(LOCAL, location->getTransformMatrix());
}

void
FGViewer::updateAtModelLocation (SGLocation * location)
{
  sgCopyMat4(ATLOCAL, location->getTransformMatrix());
}

void
FGViewer::recalcOurOwnLocation (SGLocation * location, double lon_deg, double lat_deg, double alt_ft, 
                        double roll_deg, double pitch_deg, double heading_deg)
{
  // update from our own data...
  dampEyeData(roll_deg, pitch_deg, heading_deg);
  location->setPosition( lon_deg, lat_deg, alt_ft );
  location->setOrientation( roll_deg, pitch_deg, heading_deg );
  sgCopyMat4(LOCAL, location->getTransformMatrix());
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

  set_clean();
}

// recalculate for LookFrom view type...
void
FGViewer::recalcLookFrom ()
{

  sgVec3 right, forward;
  // sgVec3 eye_pos;
  sgVec3 position_offset; // eye position offsets (xyz)

  // LOOKFROM mode...

  // Update location data...
  if ( _from_model ) {
    // update or data from model location
    updateFromModelLocation(_location);
  } else {
    // update from our own data...
    recalcOurOwnLocation( _location, _lon_deg, _lat_deg, _alt_ft, 
          _roll_deg, _pitch_deg, _heading_deg );
  }

  // copy data from location class to local items...
  copyLocationData();

  // make sg vectors view up, right and forward vectors from LOCAL
  sgSetVec3( _view_up, LOCAL[2][0], LOCAL[2][1], LOCAL[2][2] );
  sgSetVec3( right, LOCAL[1][0], LOCAL[1][1], LOCAL[1][2] );
  sgSetVec3( forward, -LOCAL[0][0], -LOCAL[0][1], -LOCAL[0][2] );

  // Note that when in "lookfrom" view the "view up" vector is always applied
  // to the viewer.  View up is based on verticle of the aircraft itself. (see
  // "LOCAL" matrix above)

  // Orientation Offsets matrix
  MakeVIEW_OFFSET( VIEW_OFFSET,
    _heading_offset_deg  * SG_DEGREES_TO_RADIANS, _view_up,
    _pitch_offset_deg  * SG_DEGREES_TO_RADIANS, right,
    _roll_offset_deg * SG_DEGREES_TO_RADIANS, forward );

  // Make the VIEW matrix.
  sgSetVec4(VIEW[0], right[0], right[1], right[2],SG_ZERO);
  sgSetVec4(VIEW[1], forward[0], forward[1], forward[2],SG_ZERO);
  sgSetVec4(VIEW[2], _view_up[0], _view_up[1], _view_up[2],SG_ZERO);
  sgSetVec4(VIEW[3], SG_ZERO, SG_ZERO, SG_ZERO,SG_ONE);

  // rotate model or local matrix to get a matrix to apply Eye Position Offsets
  sgMat4 VIEW_UP; // L0 forward L1 right L2 up
  sgCopyVec4(VIEW_UP[0], LOCAL[1]); 
  sgCopyVec4(VIEW_UP[1], LOCAL[2]);
  sgCopyVec4(VIEW_UP[2], LOCAL[0]);
  sgZeroVec4(VIEW_UP[3]);

  // Eye Position Offsets to vector
  sgSetVec3( position_offset, _x_offset_m, _y_offset_m, _z_offset_m );
  sgXformVec3( position_offset, position_offset, VIEW_UP);

  // add the offsets including rotations to the translation vector
  sgAddVec3( _view_pos, position_offset );

  // multiply the OFFSETS (for heading and pitch) into the VIEW
  sgPostMultMat4(VIEW, VIEW_OFFSET);

  // add the position data to the matrix
  sgSetVec4(VIEW[3], _view_pos[0], _view_pos[1], _view_pos[2],SG_ONE);

}

void
FGViewer::recalcLookAt ()
{

  sgVec3 right, forward;
  sgVec3 eye_pos, at_pos;
  sgVec3 position_offset; // eye position offsets (xyz)
  sgVec3 target_position_offset; // target position offsets (xyz)

  // The position vectors originate from the view point or target location
  // depending on the type of view.

  // LOOKAT mode...

  // Update location data for target...
  if ( _at_model ) {
    // update or data from model location
    updateAtModelLocation(_target_location);
  } else {
    // if not model then calculate our own target position...
    recalcOurOwnLocation( _target_location, _target_lon_deg, _target_lat_deg, _target_alt_ft, 
          _target_roll_deg, _target_pitch_deg, _target_heading_deg );
  }
  // calculate the "at" target object positon relative to eye or view's tile center...
  Point3D center = globals->get_scenery()->get_next_center();
  sgdVec3 dVec3;
  sgdSetVec3(dVec3, center[0], center[1], center[2]);
  sgdSubVec3(dVec3, _target_location->get_absolute_view_pos(), dVec3 );
  sgSetVec3(at_pos, dVec3[0], dVec3[1], dVec3[2]);

  // Update location data for eye...
  if ( _from_model ) {
    // update or data from model location
    updateFromModelLocation(_location);
  } else {
    // update from our own data, just the rotation here...
    recalcOurOwnLocation( _location, _lon_deg, _lat_deg, _alt_ft, 
          _roll_deg, _pitch_deg, _heading_deg );
  }
  // save the eye positon...
  sgCopyVec3(eye_pos,  _location->get_view_pos(center));

  // copy data from location class to local items...
  copyLocationData();

  // make sg vectors view up, right and forward vectors from LOCAL
  sgSetVec3( _view_up, LOCAL[2][0], LOCAL[2][1], LOCAL[2][2] );
  sgSetVec3( right, LOCAL[1][0], LOCAL[1][1], LOCAL[1][2] );
  sgSetVec3( forward, -LOCAL[0][0], -LOCAL[0][1], -LOCAL[0][2] );

  // rotate model or local matrix to get a matrix to apply Eye Position Offsets
  sgMat4 VIEW_UP; // L0 forward L1 right L2 up
  sgCopyVec4(VIEW_UP[0], LOCAL[1]); 
  sgCopyVec4(VIEW_UP[1], LOCAL[2]);
  sgCopyVec4(VIEW_UP[2], LOCAL[0]);
  sgZeroVec4(VIEW_UP[3]);

  // get Orientation Offsets matrix
  MakeVIEW_OFFSET( VIEW_OFFSET,
    (_heading_offset_deg - 180) * SG_DEGREES_TO_RADIANS, _view_up,
    _pitch_offset_deg * SG_DEGREES_TO_RADIANS, right,
    _roll_offset_deg * SG_DEGREES_TO_RADIANS, forward );

  // add in the position offsets
  sgSetVec3( position_offset, _y_offset_m, _x_offset_m, _z_offset_m );
  sgXformVec3( position_offset, position_offset, VIEW_UP);

  // apply the Orientation offsets
  sgXformVec3( position_offset, position_offset, VIEW_OFFSET );

  // add the Position offsets from object to the eye position
  sgAddVec3( eye_pos, eye_pos, position_offset );

  // add target offsets to at_position...
  sgSetVec3(target_position_offset, _target_z_offset_m,  _target_x_offset_m,
                                    _target_y_offset_m );
  sgXformVec3(target_position_offset, target_position_offset, ATLOCAL);
  sgAddVec3( at_pos, at_pos, target_position_offset);

  sgAddVec3( eye_pos, eye_pos, target_position_offset);

  // Make the VIEW matrix for a "LOOKAT".
  sgMakeLookAtMat4( VIEW, eye_pos, at_pos, _view_up );

}

// copy results from location class to viewer...
// FIXME: some of these should be changed to reference directly to SGLocation...
void
FGViewer::copyLocationData()
{
  Point3D center = globals->get_scenery()->get_center();
  // Get our friendly vectors from the eye location...
  sgdCopyVec3(_absolute_view_pos, _location->get_absolute_view_pos());
  sgCopyVec3(_relative_view_pos, _location->get_view_pos(center));
  sgCopyMat4(UP, _location->getCachedUpMatrix());
  sgCopyVec3(_world_up, _location->get_world_up());
  // these are the vectors that the sun and moon code like to get...
  sgCopyVec3(_surface_east, _location->get_surface_east());
  sgCopyVec3(_surface_south, _location->get_surface_south());

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

  // copy coordinates to outputs for viewer...
  sgCopyVec3(_zero_elev, _relative_view_pos);
  sgAddScaledVec3(_zero_elev, _world_up, -_alt_ft*SG_FEET_TO_METER);
  sgCopyVec3(_view_pos, _relative_view_pos);
}

void
FGViewer::dampEyeData (double &roll_deg, double &pitch_deg, double &heading_deg)
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
