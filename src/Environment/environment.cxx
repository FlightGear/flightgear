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

#include <math.h>

#include <plib/sg.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>

#include <Main/fg_props.hxx>
#include "environment.hxx"


FGEnvironment::FGEnvironment()
  : visibility_m(32000),
    temperature_sea_level_degc(20),
    pressure_sea_level_inhg(28),
    wind_from_heading_deg(0),
    wind_speed_kt(0),
    wind_from_north_fps(0),
    wind_from_east_fps(0),
    wind_from_down_fps(0)
{
}

FGEnvironment::FGEnvironment (const FGEnvironment &env)
  : elevation_ft(env.elevation_ft),
    visibility_m(env.visibility_m),
    temperature_sea_level_degc(env.temperature_sea_level_degc),
    pressure_sea_level_inhg(env.pressure_sea_level_inhg),
    wind_from_heading_deg(env.wind_from_heading_deg),
    wind_speed_kt(env.wind_speed_kt),
    wind_from_north_fps(env.wind_from_north_fps),
    wind_from_east_fps(env.wind_from_east_fps),
    wind_from_down_fps(env.wind_from_down_fps)
{
}

FGEnvironment::~FGEnvironment()
{
}


double
FGEnvironment::get_visibility_m () const
{
  return visibility_m;
}

double
FGEnvironment::get_temperature_sea_level_degc () const
{
  return temperature_sea_level_degc;
}

double
FGEnvironment::get_temperature_degc () const
{
  return temperature_degc;
}

double
FGEnvironment::get_pressure_sea_level_inhg () const
{
  return pressure_sea_level_inhg;
}

double
FGEnvironment::get_pressure_inhg () const
{
  return pressure_inhg;
}

double
FGEnvironment::get_wind_from_heading_deg () const
{
  return wind_from_heading_deg;
}

double
FGEnvironment::get_wind_speed_kt () const
{
  return wind_speed_kt;
}

double
FGEnvironment::get_wind_from_north_fps () const
{
  return wind_from_north_fps;
}

double
FGEnvironment::get_wind_from_east_fps () const
{
  return wind_from_east_fps;
}

double
FGEnvironment::get_wind_from_down_fps () const
{
  return wind_from_down_fps;
}

double
FGEnvironment::get_elevation_ft () const
{
  return elevation_ft;
}



void
FGEnvironment::set_visibility_m (double v)
{
  visibility_m = v;
}

void
FGEnvironment::set_temperature_sea_level_degc (double t)
{
  temperature_sea_level_degc = t;
  _recalc_alt_temp();
}

void
FGEnvironment::set_temperature_degc (double t)
{
  temperature_degc = t;
  _recalc_sl_temp();
}

void
FGEnvironment::set_pressure_sea_level_inhg (double p)
{
  pressure_sea_level_inhg = p;
  _recalc_alt_press();
}

void
FGEnvironment::set_pressure_inhg (double p)
{
  pressure_inhg = p;
  _recalc_sl_press();
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
FGEnvironment::set_elevation_ft (double e)
{
  elevation_ft = e;
  _recalc_alt_temp();
  _recalc_alt_press();
}

void
FGEnvironment::_recalc_hdgspd ()
{
  double angle_rad;

  if (wind_from_east_fps == 0) {
    angle_rad = (wind_from_north_fps >= 0 ? SGD_PI/2 : -SGD_PI/2);
  } else {
    angle_rad = atan(wind_from_north_fps/wind_from_east_fps);
  }
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

void
FGEnvironment::_recalc_sl_temp ()
{
  // Earth atmosphere model from
  // http://www.grc.nasa.gov/WWW/K-12/airplane/atmos.html

  // Stratospheric temperatures are not really reversible, so use 15degC.

  if (elevation_ft < 36152)	// Troposphere
    temperature_sea_level_degc =
      temperature_degc + (.00649 * SG_FEET_TO_METER * elevation_ft);
  // If we're in the stratosphere, leave sea-level temp alone
}

void
FGEnvironment::_recalc_alt_temp ()
{
  // Earth atmosphere model from
  // http://www.grc.nasa.gov/WWW/K-12/airplane/atmos.html

  if (elevation_ft < 36152)	// Troposphere
    temperature_degc =
      temperature_sea_level_degc - (.00649 * SG_FEET_TO_METER * elevation_ft);
  else if (elevation_ft < 82345) // Lower Stratosphere
    temperature_degc = -56.46;
  else
    temperature_degc = -131.21 + (.00299 * SG_FEET_TO_METER * elevation_ft);
}

void
FGEnvironment::_recalc_sl_press ()
{
  // FIXME: calculate properly
  pressure_sea_level_inhg = pressure_inhg;
}

void
FGEnvironment::_recalc_alt_press ()
{
  // FIXME: calculate properly
  pressure_inhg = pressure_sea_level_inhg;
}

// end of environment.cxx

