// location.hxx -- class for determining model location in the flightgear world.
//
// Written by Jim Wilson,  David Megginson, started April 2002.
//                          overhaul started October 2000.
//
// This file is in the Public Domain, and comes with no warranty.



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

#include <simgear/math/vector.hxx>
/*
#include "globals.hxx"
*/

#include "location.hxx"


/**
 * make model transformation Matrix - based on optimizations by NHV
 */
static void MakeTRANS( sgMat4 dst, const double Theta,
			const double Phi, const double Psi, 
                        const sgMat4 UP)
{
    SGfloat cosTheta = (SGfloat) cos(Theta);
    SGfloat sinTheta = (SGfloat) sin(Theta);
    SGfloat cosPhi   = (SGfloat) cos(Phi);
    SGfloat sinPhi   = (SGfloat) sin(Phi);
    SGfloat sinPsi   = (SGfloat) sin(Psi) ;
    SGfloat cosPsi   = (SGfloat) cos(Psi) ;

    sgMat4 tmp;
	
    tmp[0][0] = cosPhi * cosTheta;
    tmp[0][1] =	sinPhi * cosPsi + cosPhi * -sinTheta * -sinPsi;
    tmp[0][2] =	sinPhi * sinPsi + cosPhi * -sinTheta * cosPsi;

    tmp[1][0] = -sinPhi * cosTheta;
    tmp[1][1] =	cosPhi * cosPsi + -sinPhi * -sinTheta * -sinPsi;
    tmp[1][2] =	cosPhi * sinPsi + -sinPhi * -sinTheta * cosPsi;
	
    tmp[2][0] = sinTheta;
    tmp[2][1] =	cosTheta * -sinPsi;
    tmp[2][2] =	cosTheta * cosPsi;
	
    float a = UP[0][0];
    float b = UP[1][0];
    float c = UP[2][0];
    dst[2][0] = a*tmp[0][0] + b*tmp[0][1] + c*tmp[0][2] ;
    dst[1][0] = a*tmp[1][0] + b*tmp[1][1] + c*tmp[1][2] ;
    dst[0][0] = -(a*tmp[2][0] + b*tmp[2][1] + c*tmp[2][2]) ;
    dst[3][0] = SG_ZERO ;

    a = UP[0][1];
    b = UP[1][1];
    c = UP[2][1];
    dst[2][1] = a*tmp[0][0] + b*tmp[0][1] + c*tmp[0][2] ;
    dst[1][1] = a*tmp[1][0] + b*tmp[1][1] + c*tmp[1][2] ;
    dst[0][1] = -(a*tmp[2][0] + b*tmp[2][1] + c*tmp[2][2]) ;
    dst[3][1] = SG_ZERO ;

    a = UP[0][2];
    c = UP[2][2];
    dst[2][2] = a*tmp[0][0] + c*tmp[0][2] ;
    dst[1][2] = a*tmp[1][0] + c*tmp[1][2] ;
    dst[0][2] = -(a*tmp[2][0] + c*tmp[2][2]) ;
    dst[3][2] = SG_ZERO ;

    dst[2][3] = SG_ZERO ;
    dst[1][3] = SG_ZERO ;
    dst[0][3] = SG_ZERO ;
    dst[3][3] = SG_ONE ;

}


////////////////////////////////////////////////////////////////////////
// Implementation of FGLocation.
////////////////////////////////////////////////////////////////////////

// Constructor
FGLocation::FGLocation( void ):
    _dirty(true),
    _lon_deg(0),
    _lat_deg(0),
    _alt_ft(0),
    _roll_deg(0),
    _pitch_deg(0),
    _heading_deg(0)
{
    sgdZeroVec3(_absolute_view_pos);
}


// Destructor
FGLocation::~FGLocation( void ) {
}

void
FGLocation::init ()
{
}

void
FGLocation::bind ()
{
}

void
FGLocation::unbind ()
{
}

void
FGLocation::setPosition (double lon_deg, double lat_deg, double alt_ft)
{
  _dirty = true;
  _lon_deg = lon_deg;
  _lat_deg = lat_deg;
  _alt_ft = alt_ft;
}

void
FGLocation::setOrientation (double roll_deg, double pitch_deg, double heading_deg)
{
  _dirty = true;
  _roll_deg = roll_deg;
  _pitch_deg = pitch_deg;
  _heading_deg = heading_deg;
}

double *
FGLocation::get_absolute_view_pos () 
{
  if (_dirty)
    recalc();
  return _absolute_view_pos;
}

float *
FGLocation::getRelativeViewPos () 
{
  if (_dirty)
    recalc();
  return _relative_view_pos;
}

float *
FGLocation::getZeroElevViewPos () 
{
  if (_dirty)
    recalc();
  return _zero_elev_view_pos;
}


// recalc() is done every time one of the setters is called (making the 
// cached data "dirty") on the next "get".  It calculates all the outputs 
// for viewer.
void
FGLocation::recalc ()
{

  recalcPosition( _lon_deg, _lat_deg, _alt_ft );

  // Make the world up rotation matrix for eye positioin...
  sgMakeRotMat4( UP, _lon_deg, 0.0, -_lat_deg );


  // get the world up radial vector from planet center for output
  sgSetVec3( _world_up, UP[0][0], UP[0][1], UP[0][2] );

  // Creat local matrix with current geodetic position.  Converting
  // the orientation (pitch/roll/heading) to vectors.
  MakeTRANS( TRANS, _pitch_deg * SG_DEGREES_TO_RADIANS,
                      _roll_deg * SG_DEGREES_TO_RADIANS,
                      -_heading_deg * SG_DEGREES_TO_RADIANS,
                      UP);

  // Given a vector pointing straight down (-Z), map into onto the
  // local plane representing "horizontal".  This should give us the
  // local direction for moving "south".
  sgVec3 minus_z;
  sgSetVec3( minus_z, 0.0, 0.0, -1.0 );

  sgmap_vec_onto_cur_surface_plane(_world_up, _relative_view_pos, minus_z,
				     _surface_south);
  sgNormalizeVec3(_surface_south);

  // now calculate the surface east vector
  sgVec3 world_down;
  sgNegateVec3(world_down, _world_up);
  sgVectorProductVec3(_surface_east, _surface_south, world_down);

  set_clean();
}

void
FGLocation::recalcPosition (double lon_deg, double lat_deg, double alt_ft) const
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

void
FGLocation::update (int dt)
{
}



