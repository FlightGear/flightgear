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


////////////////////////////////////////////////////////////////////////
// Implementation of FGViewer.
////////////////////////////////////////////////////////////////////////

// Constructor
FGViewer::FGViewer( void ):
    scalingType(FG_SCALING_MAX),
    fov(55.0),
    goal_view_offset(0.0),
    goal_view_tilt(0.0),
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
    _roll_offset_deg(0)
{
    sgdZeroVec3(_absolute_view_pos);
    sea_level_radius = SG_EQUATORIAL_RADIUS_M; 
    //a reasonable guess for init, so that the math doesn't blow up
}


// Destructor
FGViewer::~FGViewer( void ) {
}

void
FGViewer::init ()
{
  if ( _type == FG_LOOKAT ) {
      set_reverse_view_offset(true);
  }

  if ( _type == FG_RPH ) {
      set_reverse_view_offset(false);
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
// cached data "dirty").  It calculates all the outputs for viewer.
void
FGViewer::recalc ()
{
  sgVec3 minus_z, right, forward, tilt;
  sgMat4 VIEWo;
  sgMat4 tmpROT;  // temp rotation work matrices
  sgVec3 tmpVec3;  // temp work vector (3)


  // The position vectors originate from the view point or target location
  // depending on the type of view.

  if (_type == FG_RPH) {
    recalcPositionVectors( _lon_deg, _lat_deg, _alt_ft );
  } else {
    recalcPositionVectors( _target_lon_deg, _target_lat_deg, _target_alt_ft );
  }

  sgCopyVec3(zero_elev, _zero_elev_view_pos);
  sgCopyVec3(view_pos, _relative_view_pos);

  if (_type == FG_LOOKAT) {

    // Make the world up rotation matrix for lookat
    sgMakeRotMat4( UP, _target_lon_deg, 0.0, -_target_lat_deg );

    // get the world up verctor from the worldup rotation matrix
    sgSetVec3( world_up, UP[0][0], UP[0][1], UP[0][2] );

    sgCopyVec3( view_up, world_up );
    

    // create offset vector
    sgVec3 lookat_offset;
    sgSetVec3( lookat_offset, _x_offset_m, _y_offset_m, _z_offset_m );

    // Apply heading orientation and orientation offset to lookat_offset...
    sgMakeRotMat4( tmpROT, _heading_offset_deg -_heading_deg, world_up);
    sgXformVec3( lookat_offset, lookat_offset, UP );
    sgXformVec3( lookat_offset, lookat_offset, tmpROT );

    // Apply orientation offset tilt...
    // FIXME: Need to get and use a "right" vector instead of 1-0-0
    sgSetVec3 (tmpVec3, 1, 0, 0);
    sgMakeRotMat4( tmpROT, _pitch_offset_deg, tmpVec3 );
    sgXformPnt3( lookat_offset, lookat_offset, tmpROT );

    // add the offsets including rotations to the coordinates
    sgAddVec3( view_pos, lookat_offset );

    // Make the VIEW matrix.
    fgMakeLookAtMat4( VIEW, view_pos, view_forward, view_up );


    // the VIEW matrix includes both rotation and translation.  Let's
    // knock out the translation part to make the VIEW_ROT matrix
    sgCopyMat4( VIEW_ROT, VIEW );
    VIEW_ROT[3][0] = VIEW_ROT[3][1] = VIEW_ROT[3][2] = 0.0;

  }

  if (_type == FG_RPH) {

    // code to calculate LOCAL matrix calculated from Phi, Theta, and
    // Psi (roll, pitch, yaw) in case we aren't running LaRCsim as our
    // flight model
	
    fgMakeLOCAL( LOCAL, _pitch_deg * SG_DEGREES_TO_RADIANS,
                        _roll_deg * SG_DEGREES_TO_RADIANS,
                        -_heading_deg * SG_DEGREES_TO_RADIANS);
	
    // Make the world up rotation matrix for pilot view
    sgMakeRotMat4( UP, _lon_deg, 0.0, -_lat_deg );

    // get the world up verctor from the worldup rotation matrix
    sgSetVec3( world_up, UP[0][0], UP[0][1], UP[0][2] );

    // VIEWo becomes the rotation matrix with world_up incorporated
    sgCopyMat4( VIEWo, LOCAL );
    sgPostMultMat4( VIEWo, UP );

    // generate the sg view up and forward vectors
    sgSetVec3( view_up, VIEWo[0][0], VIEWo[0][1], VIEWo[0][2] );
    sgSetVec3( right, VIEWo[1][0], VIEWo[1][1], VIEWo[1][2] );
    sgSetVec3( forward, VIEWo[2][0], VIEWo[2][1], VIEWo[2][2] );

    // apply the offsets in world coordinates
    sgVec3 pilot_offset_world;
    sgSetVec3( pilot_offset_world, 
	       _z_offset_m, _y_offset_m, -_x_offset_m );
    sgXformVec3( pilot_offset_world, pilot_offset_world, VIEWo );

    // generate the view offset matrix using orientation offset (heading)
    sgMakeRotMat4( VIEW_OFFSET, _heading_offset_deg, view_up );

    // create a tilt matrix using orientation offset (pitch)
    sgMat4 VIEW_TILT;
    sgMakeRotMat4( VIEW_TILT, _pitch_offset_deg, right );
    sgPreMultMat4(VIEW_OFFSET, VIEW_TILT);
    sgXformVec3( view_forward, forward, VIEW_OFFSET );
    SG_LOG( SG_VIEW, SG_DEBUG, "(RPH) view forward = "
	    << view_forward[0] << "," << view_forward[1] << ","
	    << view_forward[2] );
	
    // VIEW_ROT = LARC_TO_SSG * ( VIEWo * VIEW_OFFSET )
    fgMakeViewRot( VIEW_ROT, VIEW_OFFSET, VIEWo );

    sgVec3 trans_vec;
    sgAddVec3( trans_vec, view_pos, pilot_offset_world );

    // VIEW = VIEW_ROT * TRANS
    sgCopyMat4( VIEW, VIEW_ROT );
    sgPostMultMat4ByTransMat4( VIEW, trans_vec );

  }

  // Given a vector pointing straight down (-Z), map into onto the
  // local plane representing "horizontal".  This should give us the
  // local direction for moving "south".
  sgSetVec3( minus_z, 0.0, 0.0, -1.0 );

  sgmap_vec_onto_cur_surface_plane(world_up, view_pos, minus_z,
				     surface_south);
  sgNormalizeVec3(surface_south);

  // now calculate the surface east vector
  sgVec3 world_down;
  sgNegateVec3(world_down, world_up);
  sgVectorProductVec3(surface_east, surface_south, world_down);

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
    switch (scalingType) {
    case FG_SCALING_WIDTH:  // h_fov == fov
	return fov;
    case FG_SCALING_MAX:
	if (aspect_ratio < 1.0) {
	    // h_fov == fov
	    return fov;
	} else {
	    // v_fov == fov
	    return atan(tan(fov/2 * SG_DEGREES_TO_RADIANS) / aspect_ratio) *
		SG_RADIANS_TO_DEGREES * 2;
	}
    default:
	assert(false);
    }
}

double
FGViewer::get_v_fov()
{
    switch (scalingType) {
    case FG_SCALING_WIDTH:  // h_fov == fov
	return atan(tan(fov/2 * SG_DEGREES_TO_RADIANS) * aspect_ratio) *
	    SG_RADIANS_TO_DEGREES * 2;
    case FG_SCALING_MAX:
	if (aspect_ratio < 1.0) {
	    // h_fov == fov
	    return atan(tan(fov/2 * SG_DEGREES_TO_RADIANS) * aspect_ratio) *
		SG_RADIANS_TO_DEGREES * 2;
	} else {
	    // v_fov == fov
	    return fov;
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
    if ( fabs(get_goal_view_offset() - getHeadingOffset_deg()) < 1 ) {
      setHeadingOffset_deg( get_goal_view_offset() );
      break;
    } else {
      // move current_view.headingoffset towards
      // current_view.goal_view_offset
      if ( get_goal_view_offset() > getHeadingOffset_deg() )
	{
	  if ( get_goal_view_offset() - getHeadingOffset_deg() < 180 ){
	    inc_view_offset( 0.5 );
	  } else {
	    inc_view_offset( -0.5 );
	  }
	} else {
	  if ( getHeadingOffset_deg() - get_goal_view_offset() < 180 ){
	    inc_view_offset( -0.5 );
	  } else {
	    inc_view_offset( 0.5 );
	  }
	}
      if ( getHeadingOffset_deg() > 360 ) {
	inc_view_offset( -360 );
      } else if ( getHeadingOffset_deg() < 0 ) {
	inc_view_offset( 360 );
      }
    }
  }

  for ( i = 0; i < dt; i++ ) {
    if ( fabs(get_goal_view_tilt() - getPitchOffset_deg()) < 1 ) {
      setPitchOffset_deg( get_goal_view_tilt() );
      break;
    } else {
      // move current_view.pitch_offset_deg towards
      // current_view.goal_view_tilt
      if ( get_goal_view_tilt() > getPitchOffset_deg() )
	{
	  if ( get_goal_view_tilt() - getPitchOffset_deg() < 0 ){
	    inc_view_tilt( 1.0 );
	  } else {
	    inc_view_tilt( -1.0 );
	  }
	} else {
	  if ( getPitchOffset_deg() - get_goal_view_tilt() < 0 ){
	    inc_view_tilt( -1.0 );
	  } else {
	    inc_view_tilt( 1.0 );
	  }
	}
      if ( getPitchOffset_deg() > 90 ) {
	setPitchOffset_deg(90);
      } else if ( getPitchOffset_deg() < -90 ) {
	setPitchOffset_deg( -90 );
      }
    }
  }
}


void FGViewer::fgMakeLookAtMat4 ( sgMat4 dst, const sgVec3 eye, const sgVec3 center,
			const sgVec3 up )
{
  // Caveats:
  // 1) In order to compute the line of sight, the eye point must not be equal
  //    to the center point.
  // 2) The up vector must not be parallel to the line of sight from the eye
  //    to the center point.

  /* Compute the direction vectors */
  sgVec3 x,y,z;

  /* Y vector = center - eye */
  sgSubVec3 ( y, center, eye ) ;

  /* Z vector = up */
  sgCopyVec3 ( z, up ) ;

  /* X vector = Y cross Z */
  sgVectorProductVec3 ( x, y, z ) ;

  /* Recompute Z = X cross Y */
  sgVectorProductVec3 ( z, x, y ) ;

  /* Normalize everything */
  sgNormaliseVec3 ( x ) ;
  sgNormaliseVec3 ( y ) ;
  sgNormaliseVec3 ( z ) ;

  /* Build the matrix */
#define M(row,col)  dst[row][col]
  M(0,0) = x[0];    M(0,1) = x[1];    M(0,2) = x[2];    M(0,3) = 0.0;
  M(1,0) = y[0];    M(1,1) = y[1];    M(1,2) = y[2];    M(1,3) = 0.0;
  M(2,0) = z[0];    M(2,1) = z[1];    M(2,2) = z[2];    M(2,3) = 0.0;
  M(3,0) = eye[0];  M(3,1) = eye[1];  M(3,2) = eye[2];  M(3,3) = 1.0;
#undef M
}
/* end from lookat */

/* from rph */
// VIEW_ROT = LARC_TO_SSG * ( VIEWo * VIEW_OFFSET )
// This takes advantage of the fact that VIEWo and VIEW_OFFSET
// only have entries in the upper 3x3 block
// and that LARC_TO_SSG is just a shift of rows   NHV
void FGViewer::fgMakeViewRot( sgMat4 dst, const sgMat4 m1, const sgMat4 m2 )
{
    for ( int j = 0 ; j < 3 ; j++ ) {
	dst[2][j] = m2[0][0] * m1[0][j] +
	    m2[0][1] * m1[1][j] +
	    m2[0][2] * m1[2][j];

	dst[0][j] = m2[1][0] * m1[0][j] +
	    m2[1][1] * m1[1][j] +
	    m2[1][2] * m1[2][j];

	dst[1][j] = m2[2][0] * m1[0][j] +
	    m2[2][1] * m1[1][j] +
	    m2[2][2] * m1[2][j];
    }
    dst[0][3] = 
	dst[1][3] = 
	dst[2][3] = 
	dst[3][0] = 
	dst[3][1] = 
	dst[3][2] = SG_ZERO;
    dst[3][3] = SG_ONE;
}


void FGViewer::fgMakeLOCAL( sgMat4 dst, const double Theta,
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

/* end from rph */

