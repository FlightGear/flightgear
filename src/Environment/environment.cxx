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
#include <simgear/math/interpolater.hxx>

#include <Main/fg_props.hxx>
#include "environment.hxx"


FGEnvironment::FGEnvironment()
  : _temperature_degc_table(new SGInterpTable),
    _pressure_inhg_table(new SGInterpTable),
    _density_slugft3_table(new SGInterpTable),
    visibility_m(32000),
    temperature_sea_level_degc(20),
    pressure_sea_level_inhg(28),
    density_sea_level_slugft3(0.0024),
    wind_from_heading_deg(0),
    wind_speed_kt(0),
    wind_from_north_fps(0),
    wind_from_east_fps(0),
    wind_from_down_fps(0)
{
  _setup_tables();
}

FGEnvironment::FGEnvironment (const FGEnvironment &env)
  : _temperature_degc_table(new SGInterpTable),
    _pressure_inhg_table(new SGInterpTable),
    _density_slugft3_table(new SGInterpTable),
    elevation_ft(env.elevation_ft),
    visibility_m(env.visibility_m),
    temperature_sea_level_degc(env.temperature_sea_level_degc),
    pressure_sea_level_inhg(env.pressure_sea_level_inhg),
    density_sea_level_slugft3(env.density_sea_level_slugft3),
    wind_from_heading_deg(env.wind_from_heading_deg),
    wind_speed_kt(env.wind_speed_kt),
    wind_from_north_fps(env.wind_from_north_fps),
    wind_from_east_fps(env.wind_from_east_fps),
    wind_from_down_fps(env.wind_from_down_fps)
{
  _setup_tables();
}

FGEnvironment::~FGEnvironment()
{
  delete _temperature_degc_table;
  delete _pressure_inhg_table;
  delete _density_slugft3_table;
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
FGEnvironment::get_density_sea_level_slugft3 () const
{
  return density_sea_level_slugft3;
}

double
FGEnvironment::get_density_slugft3 () const
{
  return density_slugft3;
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
  _recalc_alt_temperature();
}

void
FGEnvironment::set_temperature_degc (double t)
{
  temperature_degc = t;
  _recalc_sl_temperature();
}

void
FGEnvironment::set_pressure_sea_level_inhg (double p)
{
  pressure_sea_level_inhg = p;
  _recalc_alt_pressure();
}

void
FGEnvironment::set_pressure_inhg (double p)
{
  pressure_inhg = p;
  _recalc_sl_pressure();
}

void
FGEnvironment::set_density_sea_level_slugft3 (double d)
{
  density_sea_level_slugft3 = d;
  _recalc_alt_density();
}

void
FGEnvironment::set_density_slugft3 (double d)
{
  density_slugft3 = d;
  _recalc_sl_density();
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
  _recalc_alt_temperature();
  _recalc_alt_pressure();
  _recalc_alt_density();
}

// Atmosphere model.

// Copied from YASim Atmosphere.cxx, with m converted to ft, degK
// converted to degC, Pa converted to inHG, and kg/m^3 converted to
// slug/ft^3.

// Original comment from YASim:

// Copied from McCormick, who got it from "The ARDC Model Atmosphere"
// Note that there's an error in the text in the first entry,
// McCormick lists 299.16/101325/1.22500, but those don't agree with
// R=287.  I chose to correct the temperature to 288.20, since 79F is
// pretty hot for a "standard" atmosphere.

// Elevation (ft), temperature (degC), pressure (inHG), density (slug/ft^3)
static double atmosphere_data[][4] = {
  0, 0.00, 0.000, 0.000000,
  2952, -5.89, -3.058, -0.000198,
  5905, -11.74, -5.856, -0.000384,
  8858, -17.58, -8.413, -0.000556,
  11811, -23.43, -10.745, -0.000717,
  14763, -29.27, -12.867, -0.000867,
  17716, -35.11, -14.794, -0.001006,
  20669, -40.95, -16.541, -0.001136,
  23622, -46.79, -18.120, -0.001255,
  26574, -52.62, -19.544, -0.001366,
  29527, -58.46, -20.826, -0.001467,
  32480, -64.29, -21.976, -0.001561,
  35433, -70.12, -23.005, -0.001647,
  38385, -71.54, -23.916, -0.001739,
  41338, -71.54, -24.708, -0.001822,
  44291, -71.54, -25.395, -0.001894,
  47244, -71.54, -25.991, -0.001957,
  50196, -71.54, -26.509, -0.002012,
  53149, -71.54, -26.959, -0.002059,
  56102, -71.54, -27.349, -0.002100,
  59055, -71.54, -27.687, -0.002136,
  62007, -71.54, -27.981, -0.002167,
  -1, -1, -1, -1
};

void
FGEnvironment::_setup_tables ()
{
  for (int i = 0; atmosphere_data[i][0] != -1; i++) {
    _temperature_degc_table->addEntry(atmosphere_data[i][0],
				      atmosphere_data[i][1]);
    _pressure_inhg_table->addEntry(atmosphere_data[i][0],
				   atmosphere_data[i][2]);
    _density_slugft3_table->addEntry(atmosphere_data[i][0],
				     atmosphere_data[i][3]);
  }
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
FGEnvironment::_recalc_sl_temperature ()
{
  if (elevation_ft < 38000) {
    temperature_sea_level_degc =
      temperature_degc - _temperature_degc_table->interpolate(elevation_ft);
  }
  // If we're in the stratosphere, leave sea-level temp alone
}

void
FGEnvironment::_recalc_alt_temperature ()
{
  temperature_degc =
    temperature_sea_level_degc +
    _temperature_degc_table->interpolate(elevation_ft);
}

void
FGEnvironment::_recalc_sl_pressure ()
{
  pressure_sea_level_inhg =
    pressure_inhg - _pressure_inhg_table->interpolate(elevation_ft);
}

void
FGEnvironment::_recalc_alt_pressure ()
{
  pressure_inhg =
    pressure_sea_level_inhg + _pressure_inhg_table->interpolate(elevation_ft);
}

void
FGEnvironment::_recalc_sl_density ()
{
  density_sea_level_slugft3 =
    density_slugft3 - _density_slugft3_table->interpolate(elevation_ft);
}

void
FGEnvironment::_recalc_alt_density ()
{
  density_slugft3 =
    density_sea_level_slugft3 +
    _density_slugft3_table->interpolate(elevation_ft);
}

// end of environment.cxx
