// weather.cxx -- routines to model weather
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@me.umn.edu
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
#include <Weather/weather.hxx>


// This is a record containing current weather info
FGWeather current_weather;


FGWeather::FGWeather()
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


FGWeather::~FGWeather()
{
}


// Initialize the weather modeling subsystem
void FGWeather::init ()
{
    SG_LOG( SG_GENERAL, SG_INFO, "Initializing weather subsystem");
}


void
FGWeather::bind ()
{
  fgTie("/environment/visibility-m", this,
	&FGWeather::get_visibility_m, &FGWeather::set_visibility_m);
  fgTie("/environment/wind-from-heading-deg", this,
	&FGWeather::get_wind_from_heading_deg,
	&FGWeather::set_wind_from_heading_deg);
  fgTie("/environment/wind-speed-kt", this,
	&FGWeather::get_wind_speed_kt, &FGWeather::set_wind_speed_kt);
  fgTie("/environment/wind-from-north-fps", this,
	&FGWeather::get_wind_from_north_fps,
	&FGWeather::set_wind_from_north_fps);
  fgTie("/environment/wind-from-east-fps", this,
	&FGWeather::get_wind_from_east_fps,
	&FGWeather::set_wind_from_east_fps);
  fgTie("/environment/wind-from-down-fps", this,
	&FGWeather::get_wind_from_down_fps,
	&FGWeather::set_wind_from_down_fps);
}

void
FGWeather::unbind ()
{
  fgUntie("/environment/visibility-m");
  fgUntie("/environment/wind-from-heading-deg");
  fgUntie("/environment/wind-speed-kt");
  fgUntie("/environment/wind-from-north-fps");
  fgUntie("/environment/wind-from-east-fps");
  fgUntie("/environment/wind-from-down-fps");
}

void FGWeather::update (int dt)
{
				// FIXME: the FDMs should update themselves
  current_aircraft.fdm_state
    ->set_Velocities_Local_Airmass(wind_from_north_fps,
				   wind_from_east_fps,
				   wind_from_down_fps);
}

void
FGWeather::set_visibility_m (double v)
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
FGWeather::set_wind_from_heading_deg (double h)
{
  wind_from_heading_deg = h;
  _recalc_ne();
}

void
FGWeather::set_wind_speed_kt (double s)
{
  wind_speed_kt = s;
  _recalc_ne();
}

void
FGWeather::set_wind_from_north_fps (double n)
{
  wind_from_north_fps = n;
  _recalc_hdgspd();
}

void
FGWeather::set_wind_from_east_fps (double e)
{
  wind_from_east_fps = e;
  _recalc_hdgspd();
}

void
FGWeather::set_wind_from_down_fps (double d)
{
  wind_from_down_fps = d;
  _recalc_hdgspd();
}

void
FGWeather::_recalc_hdgspd ()
{
  wind_from_heading_deg = acos(wind_from_north_fps / wind_speed_kt);
  wind_speed_kt = asin(wind_from_north_fps / wind_speed_kt);
}

void
FGWeather::_recalc_ne ()
{
  wind_from_north_fps = wind_speed_kt * cos(wind_from_heading_deg);
  wind_from_east_fps = wind_speed_kt * sin(wind_from_heading_deg);
}

// end of weather.cxx

