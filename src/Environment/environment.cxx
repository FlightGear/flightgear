// environment.cxx -- routines to model the natural environment
//
// Written by David Megginson, started February 2002.
//
// Copyright (C) 2002  David Megginson - david@megginson.com
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <GL/glut.h>
#include <GL/gl.h>

#include <math.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>

#include <Main/fg_props.hxx>
#include <Aircraft/aircraft.hxx>
#include "environment.hxx"


// This is a record containing current environment info
FGEnvironment current_environment;


FGEnvironment::FGEnvironment()
  : visibility_m(32000),
    wind_from_heading_deg(0),
    wind_speed_kt(0),
    wind_from_north_fps(0),
    wind_from_east_fps(0),
    wind_from_down_fps(0),
    fog_exp_density(0),
    fog_exp2_density(0)
{
}


FGEnvironment::~FGEnvironment()
{
}


// Initialize the environment modeling subsystem
void FGEnvironment::init ()
{
    SG_LOG( SG_GENERAL, SG_INFO, "Initializing environment subsystem");
}


void
FGEnvironment::bind ()
{
  fgTie("/environment/visibility-m", this,
	&FGEnvironment::get_visibility_m, &FGEnvironment::set_visibility_m);
  fgSetArchivable("/environment/visibility-m");
  fgTie("/environment/wind-from-heading-deg", this,
	&FGEnvironment::get_wind_from_heading_deg,
	&FGEnvironment::set_wind_from_heading_deg);
  fgTie("/environment/wind-speed-kt", this,
	&FGEnvironment::get_wind_speed_kt, &FGEnvironment::set_wind_speed_kt);
  fgTie("/environment/wind-from-north-fps", this,
	&FGEnvironment::get_wind_from_north_fps,
	&FGEnvironment::set_wind_from_north_fps, false);
  fgSetArchivable("/environment/wind-from-north-fps");
  fgTie("/environment/wind-from-east-fps", this,
	&FGEnvironment::get_wind_from_east_fps,
	&FGEnvironment::set_wind_from_east_fps, false);
  fgSetArchivable("/environment/wind-from-east-fps");
  fgTie("/environment/wind-from-down-fps", this,
	&FGEnvironment::get_wind_from_down_fps,
	&FGEnvironment::set_wind_from_down_fps, false);
  fgSetArchivable("/environment/wind-from-down-fps");
}

void
FGEnvironment::unbind ()
{
  fgUntie("/environment/visibility-m");
  fgUntie("/environment/wind-from-heading-deg");
  fgUntie("/environment/wind-speed-kt");
  fgUntie("/environment/wind-from-north-fps");
  fgUntie("/environment/wind-from-east-fps");
  fgUntie("/environment/wind-from-down-fps");
}

void FGEnvironment::update (int dt)
{
				// FIXME: the FDMs should update themselves
  current_aircraft.fdm_state
    ->set_Velocities_Local_Airmass(wind_from_north_fps,
				   wind_from_east_fps,
				   wind_from_down_fps);
}

void
FGEnvironment::set_visibility_m (double v)
{
	glMatrixMode(GL_MODELVIEW);
	// in meters
	visibility_m = v;

        // for GL_FOG_EXP
	fog_exp_density = -log(0.01 / visibility_m);

	// for GL_FOG_EXP2
	fog_exp2_density = sqrt( -log(0.01) ) / visibility_m;

	// Set correct opengl fog density
	glFogf (GL_FOG_DENSITY, fog_exp2_density);
	glFogi( GL_FOG_MODE, GL_EXP2 );

	// SG_LOG( SG_INPUT, SG_DEBUG, "Fog density = " << fog_density );
	// SG_LOG( SG_INPUT, SG_INFO, 
	//     	   "Fog exp2 density = " << fog_exp2_density );
}

void
FGEnvironment::set_wind_from_heading_deg (double h)
{
  wind_from_heading_deg = h;
  _recalc_ne();
}

void
FGEnvironment::set_wind_speed_kt (double s)
{
  wind_speed_kt = s;
  _recalc_ne();
}

void
FGEnvironment::set_wind_from_north_fps (double n)
{
  wind_from_north_fps = n;
  _recalc_hdgspd();
}

void
FGEnvironment::set_wind_from_east_fps (double e)
{
  wind_from_east_fps = e;
  _recalc_hdgspd();
}

void
FGEnvironment::set_wind_from_down_fps (double d)
{
  wind_from_down_fps = d;
  _recalc_hdgspd();
}

void
FGEnvironment::_recalc_hdgspd ()
{
  double angle_rad = atan(wind_from_north_fps/wind_from_east_fps);
  wind_from_heading_deg = angle_rad * SGD_RADIANS_TO_DEGREES;
  if (wind_from_east_fps >= 0)
    wind_from_heading_deg = 90 - wind_from_heading_deg;
  else
    wind_from_heading_deg = 270 - wind_from_heading_deg;
  if (angle_rad == 0)
    wind_speed_kt = fabs(wind_from_east_fps
			 * SG_METER_TO_NM * SG_FEET_TO_METER * 3600);
  else
    wind_speed_kt = (wind_from_north_fps / sin(angle_rad))
      * SG_METER_TO_NM * SG_FEET_TO_METER * 3600;
}

void
FGEnvironment::_recalc_ne ()
{
  double speed_fps =
    wind_speed_kt * SG_NM_TO_METER * SG_METER_TO_FEET * (1.0/3600);

  wind_from_north_fps = speed_fps *
    cos(wind_from_heading_deg * SGD_DEGREES_TO_RADIANS);
  wind_from_east_fps = speed_fps *
    sin(wind_from_heading_deg * SGD_DEGREES_TO_RADIANS);
}

// end of environment.cxx

