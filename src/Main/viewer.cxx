// viewer.cxx -- class for managing a viewer in the flightgear world.
//
// Written by Curtis Olson, started August 1997.
//                          overhaul started October 2000.
//   partially rewritten by Jim Wilson jim@kelcomaine.com using interface
//                          by David Megginson March 2002
//
// Copyright (C) 1997 - 2000  Curtis L. Olson  - curt@flightgear.org
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

#include <Scenery/scenery.hxx>
//#include <Main/location.hxx>

#include <simgear/math/vector.hxx>
#include <Main/globals.hxx>
#include <Model/acmodel.hxx>
#include <Model/model.hxx>

#include "viewer.hxx"


//////////////////////////////////////////////////////////////////
// Norman's Optimized matrix rotators!                          //
//////////////////////////////////////////////////////////////////


// Since these are pure rotation matrices we can save some bookwork
// by considering them to be 3x3 until the very end -- NHV
static void MakeVIEW_OFFSET( sgMat4 dst,
                      const float angle1, const sgVec3 axis1,
                      const float angle2, const sgVec3 axis2 )
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

    // multiply matrices
    for ( int j = 0 ; j < 3 ; j++ ) {
        dst[0][j] = mat2[0][0] * mat1[0][j] +
                    mat2[0][1] * mat1[1][j] +
                    mat2[0][2] * mat1[2][j];

        dst[1][j] = mat2[1][0] * mat1[0][j] +
                    mat2[1][1] * mat1[1][j] +
                    mat2[1][2] * mat1[2][j];

        dst[2][j] = mat2[2][0] * mat1[0][j] +
                    mat2[2][1] * mat1[1][j] +
                    mat2[2][2] * mat1[2][j];
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

// Constructor
FGViewer::FGViewer( fgViewType Type, bool from_model, int from_model_index, bool at_model, int at_model_index, double x_offset_m, double y_offset_m, double z_offset_m, double near_m ):
    _scaling_type(FG_SCALING_MAX),
    _fov_deg(55.0),
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
    _heading_offset_deg(0),
    _pitch_offset_deg(0),
    _roll_offset_deg(0),
    _goal_heading_offset_deg(0.0),
    _goal_pitch_offset_deg(0.0)
{
    sgdZeroVec3(_absolute_view_pos);
    _type = Type;
    _from_model = from_model;
    _from_model_index = from_model_index;
    _at_model = at_model;
    _at_model_index = at_model_index;
    _x_offset_m = x_offset_m;
    _y_offset_m = y_offset_m;
    _z_offset_m = z_offset_m;
    _ground_level_nearplane_m = near_m;
    //a reasonable guess for init, so that the math doesn't blow up
}


// Destructor
FGViewer::~FGViewer( void ) {
}

void
FGViewer::init ()
{
  if ( _from_model )
    _location = (FGLocation *) globals->get_aircraft_model()->get3DModel()->getFGLocation();
  else
    _location = (FGLocation *) new FGLocation;

  if ( _type == FG_LOOKAT ) {
    if ( _at_model )
      _target_location = (FGLocation *) globals->get_aircraft_model()->get3DModel()->getFGLocation();
    else
      _target_location = (FGLocation *) new FGLocation;
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

float *
FGViewer::getZeroElevViewPos () 
{
  if (_dirty)
    recalc();
  return _zero_elev_view_pos;
}

void
FGViewer::updateFromModelLocation (FGLocation * location)
{
  sgCopyMat4(LOCAL, location->getCachedTransformMatrix());
  _lon_deg = location->getLongitude_deg();
  _lat_deg = location->getLatitude_deg();
  _alt_ft = location->getAltitudeASL_ft();
  _roll_deg = location->getRoll_deg();
  _pitch_deg = location->getPitch_deg();
  _heading_deg = location->getHeading_deg();
  sgCopyVec3(_zero_elev_view_pos,  location->get_zero_elev());
  sgCopyVec3(_relative_view_pos, location->get_view_pos());
  sgdCopyVec3(_absolute_view_pos, location->get_absolute_view_pos());
  sgCopyMat4(UP, location->getCachedUpMatrix());
  sgCopyVec3(_world_up, location->get_world_up());
  // these are the vectors that the sun and moon code like to get...
  sgCopyVec3(_surface_east, location->get_surface_east());
  sgCopyVec3(_surface_south, location->get_surface_south());
}

void
FGViewer::recalcOurOwnLocation (double lon_deg, double lat_deg, double alt_ft, 
                        double roll_deg, double pitch_deg, double heading_deg)
{
  // update from our own data...
  _location->setPosition( lon_deg, lat_deg, alt_ft );
  _location->setOrientation( roll_deg, pitch_deg, heading_deg );
  sgCopyMat4(LOCAL, _location->getTransformMatrix());
  sgCopyVec3(_zero_elev_view_pos,  _location->get_zero_elev());
  sgCopyVec3(_relative_view_pos, _location->get_view_pos());
  sgdCopyVec3(_absolute_view_pos, _location->get_absolute_view_pos());
  // these are the vectors that the sun and moon code like to get...
  sgCopyVec3(_surface_east, _location->get_surface_east());
  sgCopyVec3(_surface_south, _location->get_surface_south());
}

// recalc() is done every time one of the setters is called (making the 
// cached data "dirty") on the next "get".  It calculates all the outputs 
// for viewer.
void
FGViewer::recalc ()
{
  sgVec3 minus_z, right, forward, tilt;
  sgMat4 tmpROT;  // temp rotation work matrices
  sgVec3 eye_pos, at_pos;
  sgVec3 position_offset; // eye position offsets (xyz)

  // The position vectors originate from the view point or target location
  // depending on the type of view.

  if (_type == FG_LOOKFROM) {
    // LOOKFROM mode...
    if ( _from_model ) {
      // update or data from model location
      updateFromModelLocation(_location);
    } else {
      // update from our own data...
      recalcOurOwnLocation( _lon_deg, _lat_deg, _alt_ft, 
            _roll_deg, _pitch_deg, _heading_deg );
      // get world up data from just recalced location
      sgCopyMat4(UP, _location->getUpMatrix());
      sgCopyVec3(_world_up, _location->get_world_up());
    }

  } else {

    // LOOKAT mode...
    if ( _from_model ) {
      // update or data from model location
      updateFromModelLocation(_location);
    } else {
      // update from our own data, just the rotation here...
      recalcOurOwnLocation( _lon_deg, _lat_deg, _alt_ft, 
            _roll_deg, _pitch_deg, _heading_deg );
      // get world up data from just recalced location
      sgCopyMat4(UP, _location->getUpMatrix());
      sgCopyVec3(_world_up, _location->get_world_up());
    }
    // save they eye positon...
    sgCopyVec3(eye_pos,  _location->get_view_pos());
    // save the eye rotation before getting target values!!!
    sgCopyMat4(tmpROT, LOCAL);

    if ( _at_model ) {
      // update or data from model location
      updateFromModelLocation(_target_location);
    } else {
      // if not model then calculate our own target position...
      recalcOurOwnLocation( _target_lon_deg, _target_lat_deg, _target_alt_ft, 
            _target_roll_deg, _target_pitch_deg, _target_heading_deg );
    }
    // restore the eye rotation (the from position rotation)
    sgCopyMat4(LOCAL, tmpROT);

  }

  // the coordinates generated by the above "recalcPositionVectors"
  sgCopyVec3(_zero_elev, _zero_elev_view_pos);
  sgCopyVec3(_view_pos, _relative_view_pos);

  // FIXME:
  // Doing this last recalc here for published values...where the airplane is
  // This should be per aircraft or model (for published values) before
  // multiple FDM can be done.
  // This info should come directly from the model (not through viewer), 
  // because in some cases there is no model directly assigned as a lookfrom
  // position.  The problem that arises is related to the FDM interface looking
  // indirectly to the viewer to find the altitude of the aircraft on the runway.
  //
  // Note that recalcPositionVectors can be removed from viewer when this 
  // issue is addressed.
  //
  if (!_from_model) {
    recalcPositionVectors(fgGetDouble("/position/longitude-deg"),
                        fgGetDouble("/position/latitude-deg"),
                        fgGetDouble("/position/altitude-ft"));
  }

  // make sg vectors view up, right and forward vectors from LOCAL
  sgSetVec3( _view_up, LOCAL[2][0], LOCAL[2][1], LOCAL[2][2] );
  sgSetVec3( right, LOCAL[1][0], LOCAL[1][1], LOCAL[1][2] );
  sgSetVec3( forward, -LOCAL[0][0], -LOCAL[0][1], -LOCAL[0][2] );

  if (_type == FG_LOOKAT) {

    // Note that when in "lookat" view the "world up" vector is always applied
    // to the viewer.  World up is based on verticle at a given lon/lat (see
    // matrix "UP" above).

    // Orientation Offsets matrix
    MakeVIEW_OFFSET( VIEW_OFFSET,
      (_heading_offset_deg -_heading_deg) * SG_DEGREES_TO_RADIANS, _world_up,
      _pitch_offset_deg * SG_DEGREES_TO_RADIANS, right );
   
    // add in the Orientation Offsets here
    sgSetVec3( position_offset, _x_offset_m, _y_offset_m, _z_offset_m );
    sgXformVec3( position_offset, position_offset, UP);

    sgXformVec3( position_offset, position_offset, VIEW_OFFSET );

    // add the Position offsets from object to the eye position
    sgAddVec3( eye_pos, eye_pos, position_offset );

    // at position (what we are looking at)
    sgCopyVec3( at_pos, _view_pos );

    // Make the VIEW matrix for a "LOOKAT".
    sgMakeLookAtMat4( VIEW, eye_pos, at_pos, _view_up );
  }

  if (_type == FG_LOOKFROM) {

    // Note that when in "lookfrom" view the "view up" vector is always applied
    // to the viewer.  View up is based on verticle of the aircraft itself. (see
    // "LOCAL" matrix above)

    // Orientation Offsets matrix
    MakeVIEW_OFFSET( VIEW_OFFSET,
      _heading_offset_deg  * SG_DEGREES_TO_RADIANS, _view_up,
      _pitch_offset_deg  * SG_DEGREES_TO_RADIANS, right );

    // Make the VIEW matrix.
    sgSetVec4(VIEW[0], right[0], right[1], right[2],SG_ZERO);
    sgSetVec4(VIEW[1], forward[0], forward[1], forward[2],SG_ZERO);
    sgSetVec4(VIEW[2], _view_up[0], _view_up[1], _view_up[2],SG_ZERO);
    sgSetVec4(VIEW[3], SG_ZERO, SG_ZERO, SG_ZERO,SG_ONE);

    // rotate matrix to get a matrix to apply Eye Position Offsets
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

  set_clean();
}

void
FGViewer::recalcPositionVectors (double lon_deg, double lat_deg, double alt_ft) const
{
  double sea_level_radius_m;
  double lat_geoc_rad;


				// Convert from geodetic to geocentric
				// coordinates.
  sgGeodToGeoc(lat_deg * SGD_DEGREES_TO_RADIANS,
	       alt_ft * SG_FEET_TO_METER,
	       &sea_level_radius_m,
	       &lat_geoc_rad);

				// Calculate the cartesian coordinates
				// of point directly below at sea level.
                                // aka Zero Elevation Position
  Point3D p = Point3D(lon_deg * SG_DEGREES_TO_RADIANS,
		      lat_geoc_rad,
		      sea_level_radius_m);
  Point3D tmp = sgPolarToCart3d(p) - scenery.get_next_center();
  sgSetVec3(_zero_elev_view_pos, tmp[0], tmp[1], tmp[2]);

				// Calculate the absolute view position
				// in fgfs coordinates.
                                // aka Absolute View Position
  p.setz(p.radius() + alt_ft * SG_FEET_TO_METER);
  tmp = sgPolarToCart3d(p);
  sgdSetVec3(_absolute_view_pos, tmp[0], tmp[1], tmp[2]);

				// Calculate the relative view position
				// from the scenery center.
                                // aka Relative View Position
  sgdVec3 scenery_center;
  sgdSetVec3(scenery_center,
	     scenery.get_next_center().x(),
	     scenery.get_next_center().y(),
	     scenery.get_next_center().z());
  sgdVec3 view_pos;
  sgdSubVec3(view_pos, _absolute_view_pos, scenery_center);
  sgSetVec3(_relative_view_pos, view_pos);

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
	    return atan(tan(_fov_deg/2 * SG_DEGREES_TO_RADIANS) / _aspect_ratio) *
		SG_RADIANS_TO_DEGREES * 2;
	}
    default:
	assert(false);
    }
}

double
FGViewer::get_v_fov()
{
    switch (_scaling_type) {
    case FG_SCALING_WIDTH:  // h_fov == fov
	return atan(tan(_fov_deg/2 * SG_DEGREES_TO_RADIANS) * _aspect_ratio) *
	    SG_RADIANS_TO_DEGREES * 2;
    case FG_SCALING_MAX:
	if (_aspect_ratio < 1.0) {
	    // h_fov == fov
	    return atan(tan(_fov_deg/2 * SG_DEGREES_TO_RADIANS) * _aspect_ratio) *
		SG_RADIANS_TO_DEGREES * 2;
	} else {
	    // v_fov == fov
	    return _fov_deg;
	}
    default:
	assert(false);
    }
}

void
FGViewer::update (int dt)
{
  int i;
  for ( i = 0; i < dt; i++ ) {
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

  for ( i = 0; i < dt; i++ ) {
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
}
