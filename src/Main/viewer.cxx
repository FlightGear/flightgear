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

#include <fg_props.hxx>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include <Scenery/scenery.hxx>

/* from lookat */
#include <simgear/math/vector.hxx>
#include "globals.hxx"
/* end from lookat */

#include "viewer.hxx"


//////////////////////////////////////////////////////////////////
// Norman's Optimized matrix rotators!                          //
//////////////////////////////////////////////////////////////////

static void fgMakeLOCAL( sgMat4 dst, const double Theta,
				const double Phi, const double Psi)
{
    SGfloat cosTheta = (SGfloat) cos(Theta);
    SGfloat sinTheta = (SGfloat) sin(Theta);
    SGfloat cosPhi   = (SGfloat) cos(Phi);
    SGfloat sinPhi   = (SGfloat) sin(Phi);
    SGfloat sinPsi   = (SGfloat) sin(Psi) ;
    SGfloat cosPsi   = (SGfloat) cos(Psi) ;
	
    dst[0][0] = cosPhi * cosTheta;
    dst[0][1] =	sinPhi * cosPsi + cosPhi * -sinTheta * -sinPsi;
    dst[0][2] =	sinPhi * sinPsi + cosPhi * -sinTheta * cosPsi;
    dst[0][3] =	SG_ZERO;

    dst[1][0] = -sinPhi * cosTheta;
    dst[1][1] =	cosPhi * cosPsi + -sinPhi * -sinTheta * -sinPsi;
    dst[1][2] =	cosPhi * sinPsi + -sinPhi * -sinTheta * cosPsi;
    dst[1][3] = SG_ZERO ;
	
    dst[2][0] = sinTheta;
    dst[2][1] =	cosTheta * -sinPsi;
    dst[2][2] =	cosTheta * cosPsi;
    dst[2][3] = SG_ZERO;
	
    dst[3][0] = SG_ZERO;
    dst[3][1] = SG_ZERO;
    dst[3][2] = SG_ZERO;
    dst[3][3] = SG_ONE ;
}


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

// Taking advantage of the 3x3 nature of this -- NHV
inline static void MakeWithWorldUp( sgMat4 dst, const sgMat4 UP, const sgMat4 LOCAL )
{
    sgMat4 tmp;

    float a = UP[0][0];
    float b = UP[1][0];
    float c = UP[2][0];
    tmp[0][0] = a*LOCAL[0][0] + b*LOCAL[0][1] + c*LOCAL[0][2] ;
    tmp[1][0] = a*LOCAL[1][0] + b*LOCAL[1][1] + c*LOCAL[1][2] ;
    tmp[2][0] = a*LOCAL[2][0] + b*LOCAL[2][1] + c*LOCAL[2][2] ;
    tmp[3][0] = SG_ZERO ;

    a = UP[0][1];
    b = UP[1][1];
    c = UP[2][1];
    tmp[0][1] = a*LOCAL[0][0] + b*LOCAL[0][1] + c*LOCAL[0][2] ;
    tmp[1][1] = a*LOCAL[1][0] + b*LOCAL[1][1] + c*LOCAL[1][2] ;
    tmp[2][1] = a*LOCAL[2][0] + b*LOCAL[2][1] + c*LOCAL[2][2] ;
    tmp[3][1] = SG_ZERO ;

    a = UP[0][2];
    c = UP[2][2];
    tmp[0][2] = a*LOCAL[0][0] + c*LOCAL[0][2] ;
    tmp[1][2] = a*LOCAL[1][0] + c*LOCAL[1][2] ;
    tmp[2][2] = a*LOCAL[2][0] + c*LOCAL[2][2] ;
    tmp[3][2] = SG_ZERO ;

    tmp[0][3] = SG_ZERO ;
    tmp[1][3] = SG_ZERO ;
    tmp[2][3] = SG_ZERO ;
    tmp[3][3] = SG_ONE ;
    sgCopyMat4(dst, tmp);
}


////////////////////////////////////////////////////////////////////////
// Implementation of FGViewer.
////////////////////////////////////////////////////////////////////////

// Constructor
FGViewer::FGViewer( void ):
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
    _x_offset_m(0),
    _y_offset_m(0),
    _z_offset_m(0),
    _heading_offset_deg(0),
    _pitch_offset_deg(0),
    _roll_offset_deg(0),
    _goal_heading_offset_deg(0.0),
    _goal_pitch_offset_deg(0.0)
{
    sgdZeroVec3(_absolute_view_pos);
    //a reasonable guess for init, so that the math doesn't blow up
}


// Destructor
FGViewer::~FGViewer( void ) {
}

void
FGViewer::init ()
{
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
    _type = FG_RPH;
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


// recalc() is done every time one of the setters is called (making the 
// cached data "dirty") on the next "get".  It calculates all the outputs 
// for viewer.
void
FGViewer::recalc ()
{
  sgVec3 minus_z, right, forward, tilt;
  sgMat4 tmpROT;  // temp rotation work matrices
  sgMat4 VIEW_HEADINGOFFSET, VIEW_PITCHOFFSET;
  sgVec3 tmpVec3;  // temp work vector (3)
  sgVec3 eye_pos, object_pos;

  // The position vectors originate from the view point or target location
  // depending on the type of view.
  // FIXME: Later note: actually the object (target) info needs to be held
  //        by the model class.

  if (_type == FG_RPH) {
    // eye position is the location of the pilot
    recalcPositionVectors( _lon_deg, _lat_deg, _alt_ft );
  } else {
    // eye position is now calculated based on lon/lat;
    recalcPositionVectors( _lon_deg, _lat_deg, _alt_ft );
    sgCopyVec3(eye_pos, _relative_view_pos);

    // object position is the location of the object being looked at
    recalcPositionVectors( _target_lon_deg, _target_lat_deg, _target_alt_ft );
  }
  // the coordinates generated by the above "recalcPositionVectors"
  sgCopyVec3(_zero_elev, _zero_elev_view_pos);
  sgCopyVec3(_view_pos, _relative_view_pos);

  // FIXME:
  // Doing this last recalc here for published values...where the airplane is
  // This should be per aircraft or model (for published values) before
  // multiple FDM can be done.
  recalcPositionVectors(fgGetDouble("/position/longitude-deg"),
                        fgGetDouble("/position/latitude-deg"),
                        fgGetDouble("/position/altitude-ft"));



  // Make the world up rotation matrix for eye positioin...
  sgMakeRotMat4( UP, _lon_deg, 0.0, -_lat_deg );


  // get the world up radial vector from planet center
  // (ie. effect of aircraft location on earth "sphere" approximation)
  sgSetVec3( _world_up, UP[0][0], UP[0][1], UP[0][2] );



  // Creat local matrix with current geodetic position.  Converting
  // the orientation (pitch/roll/heading) to vectors.
  fgMakeLOCAL( LOCAL, _pitch_deg * SG_DEGREES_TO_RADIANS,
                      _roll_deg * SG_DEGREES_TO_RADIANS,
                      -_heading_deg * SG_DEGREES_TO_RADIANS);
  // Adjust LOCAL to current world_up vector (adjustment for planet location)
  MakeWithWorldUp( LOCAL, UP, LOCAL );
  // copy the LOCAL matrix to COCKPIT_ROT for publication...
  sgCopyMat4( LOCAL_ROT, LOCAL );

  // make sg vectors view up, right and forward vectors from LOCAL
  sgSetVec3( _view_up, LOCAL[0][0], LOCAL[0][1], LOCAL[0][2] );
  sgSetVec3( right, LOCAL[1][0], LOCAL[1][1], LOCAL[1][2] );
  sgSetVec3( forward, LOCAL[2][0], LOCAL[2][1], LOCAL[2][2] );



  // create xyz offsets Vector
  sgVec3 position_offset;
  sgSetVec3( position_offset, _y_offset_m, _x_offset_m, _z_offset_m );


  // Eye rotations.  
  // Looking up/down left/right in pilot view (lookfrom mode)
  // or Floating Rotatation around the object in chase view (lookat mode).
  // Generate the offset matrix to be applied using offset angles:
  if (_type == FG_LOOKAT) {
    // Note that when in "lookat" view the "world up" vector is always applied
    // to the viewer.  World up is based on verticle at a given lon/lat (see
    // matrix "UP" above).
    MakeVIEW_OFFSET( VIEW_OFFSET,
      _heading_offset_deg * SG_DEGREES_TO_RADIANS, _world_up,
      _pitch_offset_deg * SG_DEGREES_TO_RADIANS, right );
  }
  if (_type == FG_RPH) {
    // Note that when in "lookfrom" view the "view up" vector is always applied
    // to the viewer.  View up is based on verticle of the aircraft itself. (see
    // "LOCAL" matrix above)
    MakeVIEW_OFFSET( VIEW_OFFSET,
      _heading_offset_deg  * SG_DEGREES_TO_RADIANS, _view_up,
      _pitch_offset_deg  * SG_DEGREES_TO_RADIANS, right );
  }



  if (_type == FG_LOOKAT) {
   
    // transfrom "offset" and "orientation offset" to vector
    sgXformVec3( position_offset, position_offset, UP );

    // add heading to offset so that the eye does heading as such...
    sgMakeRotMat4(tmpROT, -_heading_deg, _world_up);
    sgPostMultMat4(VIEW_OFFSET, tmpROT);
    sgXformVec3( position_offset, position_offset, VIEW_OFFSET );

    // add the offsets from object to the eye position
    sgAddVec3( eye_pos, eye_pos, position_offset );
    // copy object
    sgCopyVec3( object_pos, _view_pos );

    // Make the VIEW matrix for "lookat".
    sgMakeLookAtMat4( VIEW, eye_pos, object_pos, _view_up );
  }

  if (_type == FG_RPH) {

    sgXformVec3( position_offset, position_offset, LOCAL);
    // add the offsets including rotations to the coordinates
    sgAddVec3( _view_pos, position_offset );

    // Make the VIEW matrix.
    VIEW[0][0] = right[0];
    VIEW[0][1] = right[1];
    VIEW[0][2] = right[2];
    VIEW[0][3] = 0.0;
    VIEW[1][0] = forward[0];
    VIEW[1][1] = forward[1];
    VIEW[1][2] = forward[2];
    VIEW[1][3] = 0.0;
    VIEW[2][0] = _view_up[0];
    VIEW[2][1] = _view_up[1];
    VIEW[2][2] = _view_up[2];
    VIEW[2][3] = 0.0;
    VIEW[3][0] = 0.0;
    VIEW[3][1] = 0.0;
    VIEW[3][2] = 0.0;
    VIEW[3][3] = 0.0;
    // multiply the OFFSETS (for heading and pitch) into the VIEW
    sgPostMultMat4(VIEW, VIEW_OFFSET);

    // add the position data to the matrix
    VIEW[3][0] = _view_pos[0];
    VIEW[3][1] = _view_pos[1];
    VIEW[3][2] = _view_pos[2];
    VIEW[3][3] = 1.0f;

  }

  // the VIEW matrix includes both rotation and translation.  Let's
  // knock out the translation part to make the VIEW_ROT matrix
  sgCopyMat4( VIEW_ROT, VIEW );
  VIEW_ROT[3][0] = VIEW_ROT[3][1] = VIEW_ROT[3][2] = 0.0;

  // Given a vector pointing straight down (-Z), map into onto the
  // local plane representing "horizontal".  This should give us the
  // local direction for moving "south".
  sgSetVec3( minus_z, 0.0, 0.0, -1.0 );

  sgmap_vec_onto_cur_surface_plane(_world_up, _view_pos, minus_z,
				     _surface_south);
  sgNormalizeVec3(_surface_south);

  // now calculate the surface east vector
  sgVec3 world_down;
  sgNegateVec3(world_down, _world_up);
  sgVectorProductVec3(_surface_east, _surface_south, world_down);

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





