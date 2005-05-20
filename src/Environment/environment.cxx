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
#include <simgear/environment/visual_enviro.hxx>

#include <Main/fg_props.hxx>
#include "environment.hxx"



////////////////////////////////////////////////////////////////////////
// Atmosphere model.
////////////////////////////////////////////////////////////////////////

// Copied from YASim Atmosphere.cxx, with m converted to ft, degK
// converted to degC, Pa converted to inHG, and kg/m^3 converted to
// slug/ft^3; they were then converted to deltas from the sea-level
// defaults (approx. 15degC, 29.92inHG, and 0.00237slugs/ft^3).

// Original comment from YASim:

// Copied from McCormick, who got it from "The ARDC Model Atmosphere"
// Note that there's an error in the text in the first entry,
// McCormick lists 299.16/101325/1.22500, but those don't agree with
// R=287.  I chose to correct the temperature to 288.20, since 79F is
// pretty hot for a "standard" atmosphere.

// Elevation (ft), temperature factor (degK), pressure factor (inHG)
static double atmosphere_data[][3] = {
 { 0.00, 1.00, 1.000 },
 { 2952.76, 0.98, 0.898 },
 { 5905.51, 0.96, 0.804 },
 { 8858.27, 0.94, 0.719 },
 { 11811.02, 0.92, 0.641 },
 { 14763.78, 0.90, 0.570 },
 { 17716.54, 0.88, 0.506 },
 { 20669.29, 0.86, 0.447 },
 { 23622.05, 0.84, 0.394 },
 { 26574.80, 0.82, 0.347 },
 { 29527.56, 0.80, 0.304 },
 { 32480.31, 0.78, 0.266 },
 { 35433.07, 0.76, 0.231 },
 { 38385.83, 0.75, 0.201 },
 { 41338.58, 0.75, 0.174 },
 { 44291.34, 0.75, 0.151 },
 { 47244.09, 0.75, 0.131 },
 { 50196.85, 0.75, 0.114 },
 { 53149.61, 0.75, 0.099 },
 { 56102.36, 0.75, 0.086 },
 { 59055.12, 0.75, 0.075 },
 { 62007.87, 0.75, 0.065 },
 { -1, -1, -1 }
};

static SGInterpTable * _temperature_degc_table = 0;
static SGInterpTable * _pressure_inhg_table = 0;

static void
_setup_tables ()
{
  if (_temperature_degc_table != 0)
      return;

  _temperature_degc_table = new SGInterpTable;
  _pressure_inhg_table = new SGInterpTable;

  for (int i = 0; atmosphere_data[i][0] != -1; i++) {
    _temperature_degc_table->addEntry(atmosphere_data[i][0],
				      atmosphere_data[i][1]);
    _pressure_inhg_table->addEntry(atmosphere_data[i][0],
				   atmosphere_data[i][2]);
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGEnvironment.
////////////////////////////////////////////////////////////////////////

FGEnvironment::FGEnvironment()
  : elevation_ft(0),
    visibility_m(32000),
    temperature_sea_level_degc(15),
    temperature_degc(15),
    dewpoint_sea_level_degc(5), // guess
    dewpoint_degc(5),
    pressure_sea_level_inhg(29.92),
    pressure_inhg(29.92),
    turbulence_magnitude_norm(0),
    turbulence_rate_hz(1),
    wind_from_heading_deg(0),
    wind_speed_kt(0),
    wind_from_north_fps(0),
    wind_from_east_fps(0),
    wind_from_down_fps(0)
{
  _setup_tables();
  _recalc_density();
}

FGEnvironment::FGEnvironment (const FGEnvironment &env)
{
    FGEnvironment();
    copy(env);
}

FGEnvironment::~FGEnvironment()
{
}

void
FGEnvironment::copy (const FGEnvironment &env)
{
    elevation_ft = env.elevation_ft;
    visibility_m = env.visibility_m;
    temperature_sea_level_degc = env.temperature_sea_level_degc;
    temperature_degc = env.temperature_degc;
    dewpoint_sea_level_degc = env.dewpoint_sea_level_degc;
    dewpoint_degc = env.dewpoint_degc;
    pressure_sea_level_inhg = env.pressure_sea_level_inhg;
    wind_from_heading_deg = env.wind_from_heading_deg;
    wind_speed_kt = env.wind_speed_kt;
    wind_from_north_fps = env.wind_from_north_fps;
    wind_from_east_fps = env.wind_from_east_fps;
    wind_from_down_fps = env.wind_from_down_fps;
    turbulence_magnitude_norm = env.turbulence_magnitude_norm;
    turbulence_rate_hz = env.turbulence_rate_hz;
}

static inline bool
maybe_copy_value (FGEnvironment * env, const SGPropertyNode * node,
                  const char * name, void (FGEnvironment::*setter)(double))
{
    const SGPropertyNode * child = node->getNode(name);
                                // fragile: depends on not being typed
                                // as a number
    if (child != 0 && child->hasValue() &&
        child->getStringValue()[0] != '\0') {
        (env->*setter)(child->getDoubleValue());
        return true;
    } else {
        return false;
    }
}

void
FGEnvironment::read (const SGPropertyNode * node)
{
    maybe_copy_value(this, node, "visibility-m",
                     &FGEnvironment::set_visibility_m);

    if (!maybe_copy_value(this, node, "temperature-sea-level-degc",
                          &FGEnvironment::set_temperature_sea_level_degc))
        maybe_copy_value(this, node, "temperature-degc",
                         &FGEnvironment::set_temperature_degc);

    if (!maybe_copy_value(this, node, "dewpoint-sea-level-degc",
                          &FGEnvironment::set_dewpoint_sea_level_degc))
        maybe_copy_value(this, node, "dewpoint-degc",
                         &FGEnvironment::set_dewpoint_degc);

    if (!maybe_copy_value(this, node, "pressure-sea-level-inhg",
                          &FGEnvironment::set_pressure_sea_level_inhg))
        maybe_copy_value(this, node, "pressure-inhg",
                         &FGEnvironment::set_pressure_inhg);

    maybe_copy_value(this, node, "wind-from-heading-deg",
                     &FGEnvironment::set_wind_from_heading_deg);

    maybe_copy_value(this, node, "wind-speed-kt",
                     &FGEnvironment::set_wind_speed_kt);

    maybe_copy_value(this, node, "elevation-ft",
                     &FGEnvironment::set_elevation_ft);

    maybe_copy_value(this, node, "turbulence/magnitude-norm",
                     &FGEnvironment::set_turbulence_magnitude_norm);

    maybe_copy_value(this, node, "turbulence/rate-hz",
                     &FGEnvironment::set_turbulence_rate_hz);
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
FGEnvironment::get_temperature_degf () const
{
  return (temperature_degc * 9.0 / 5.0) + 32.0;
}

double
FGEnvironment::get_dewpoint_sea_level_degc () const
{
  return dewpoint_sea_level_degc;
}

double
FGEnvironment::get_dewpoint_degc () const
{
  return dewpoint_degc;
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
FGEnvironment::get_turbulence_magnitude_norm () const
{
  if( sgEnviro.get_turbulence_enable_state() )
    if (fgGetBool("/environment/params/real-world-weather-fetch") == true)
      return sgEnviro.get_cloud_turbulence();
  return turbulence_magnitude_norm;
}

double
FGEnvironment::get_turbulence_rate_hz () const
{
  return turbulence_rate_hz;
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
  if (dewpoint_sea_level_degc > t)
      dewpoint_sea_level_degc = t;
  _recalc_alt_temperature();
  _recalc_density();
}

void
FGEnvironment::set_temperature_degc (double t)
{
  temperature_degc = t;
  _recalc_sl_temperature();
  _recalc_density();
}

void
FGEnvironment::set_dewpoint_sea_level_degc (double t)
{
  dewpoint_sea_level_degc = t;
  if (temperature_sea_level_degc < t)
      temperature_sea_level_degc = t;
  _recalc_alt_dewpoint();
  _recalc_density();
}

void
FGEnvironment::set_dewpoint_degc (double t)
{
  dewpoint_degc = t;
  _recalc_sl_dewpoint();
  _recalc_density();
}

void
FGEnvironment::set_pressure_sea_level_inhg (double p)
{
  pressure_sea_level_inhg = p;
  _recalc_alt_pressure();
  _recalc_density();
}

void
FGEnvironment::set_pressure_inhg (double p)
{
  pressure_inhg = p;
  _recalc_sl_pressure();
  _recalc_density();
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
FGEnvironment::set_turbulence_magnitude_norm (double t)
{
  turbulence_magnitude_norm = t;
}

void
FGEnvironment::set_turbulence_rate_hz (double r)
{
  turbulence_rate_hz = r;
}

void
FGEnvironment::set_elevation_ft (double e)
{
  elevation_ft = e;
  _recalc_alt_temperature();
  _recalc_alt_dewpoint();
  _recalc_alt_pressure();
  _recalc_density();
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

#if 0
  // FIXME: Windspeed can become negative with these formulas.
  //        which can cause problems for animations that rely
  //        on the windspeed property.
  if (angle_rad == 0)
    wind_speed_kt = fabs(wind_from_east_fps
			 * SG_METER_TO_NM * SG_FEET_TO_METER * 3600);
  else
    wind_speed_kt = (wind_from_north_fps / sin(angle_rad))
      * SG_METER_TO_NM * SG_FEET_TO_METER * 3600;
#else
  wind_speed_kt = sqrt(wind_from_north_fps * wind_from_north_fps +
                       wind_from_east_fps * wind_from_east_fps) 
                  * SG_METER_TO_NM * SG_FEET_TO_METER * 3600;
#endif
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
  // If we're in the stratosphere, leave sea-level temp alone
  if (elevation_ft < 38000) {
    temperature_sea_level_degc =
      (temperature_degc + 273.15)
      /_temperature_degc_table->interpolate(elevation_ft)
      - 273.15;
  }
}

void
FGEnvironment::_recalc_alt_temperature ()
{
  if (elevation_ft < 38000) {
    temperature_degc =
      (temperature_sea_level_degc + 273.15) *
      _temperature_degc_table->interpolate(elevation_ft) - 273.15;
  } else {
    temperature_degc = -56.49;	// Stratosphere is constant
  }
}

void
FGEnvironment::_recalc_sl_dewpoint ()
{
				// 0.2degC/1000ft
				// FIXME: this will work only for low
				// elevations
  dewpoint_sea_level_degc = dewpoint_degc + (elevation_ft * .0002);
  if (dewpoint_sea_level_degc > temperature_sea_level_degc)
    dewpoint_sea_level_degc = temperature_sea_level_degc;
}

void
FGEnvironment::_recalc_alt_dewpoint ()
{
				// 0.2degC/1000ft
				// FIXME: this will work only for low
				// elevations
  dewpoint_degc = dewpoint_sea_level_degc + (elevation_ft * .0002);
  if (dewpoint_degc > temperature_degc)
    dewpoint_degc = temperature_degc;
}

void
FGEnvironment::_recalc_sl_pressure ()
{
  pressure_sea_level_inhg =
    pressure_inhg / _pressure_inhg_table->interpolate(elevation_ft);
}

void
FGEnvironment::_recalc_alt_pressure ()
{
  pressure_inhg =
    pressure_sea_level_inhg * _pressure_inhg_table->interpolate(elevation_ft);
}

void
FGEnvironment::_recalc_density ()
{
  double pressure_psf = pressure_inhg * 70.7487;
  
  // adjust for humidity
  // calculations taken from USA Today (oops!) at
  // http://www.usatoday.com/weather/basics/density-calculations.htm
  double temperature_degk = temperature_degc + 273.15;
  double pressure_mb = pressure_inhg * 33.86;
  double vapor_pressure_mb =
    6.11 * pow(10.0, 7.5 * dewpoint_degc / (237.7 + dewpoint_degc));
  double virtual_temperature_degk = temperature_degk / (1 - (vapor_pressure_mb / pressure_mb) * (1.0 - 0.622));
  double virtual_temperature_degr = virtual_temperature_degk * 1.8;

  density_slugft3 = pressure_psf / (virtual_temperature_degr * 1718);
}



////////////////////////////////////////////////////////////////////////
// Functions.
////////////////////////////////////////////////////////////////////////

static inline double
do_interp (double a, double b, double fraction)
{
    double retval = (a + ((b - a) * fraction));
    return retval;
}

static inline double
do_interp_deg (double a, double b, double fraction)
{
    a = fmod(a, 360);
    b = fmod(b, 360);
    if (fabs(b-a) > 180) {
        if (a < b)
            a += 360;
        else
            b += 360;
    }
    return fmod(do_interp(a, b, fraction), 360);
}

void
interpolate (const FGEnvironment * env1, const FGEnvironment * env2,
             double fraction, FGEnvironment * result)
{
    result->set_visibility_m
        (do_interp(env1->get_visibility_m(),
                   env2->get_visibility_m(),
                   fraction));

    result->set_temperature_sea_level_degc
        (do_interp(env1->get_temperature_sea_level_degc(),
                   env2->get_temperature_sea_level_degc(),
                   fraction));

    result->set_dewpoint_degc
        (do_interp(env1->get_dewpoint_sea_level_degc(),
                   env2->get_dewpoint_sea_level_degc(),
                   fraction));

    result->set_pressure_sea_level_inhg
        (do_interp(env1->get_pressure_sea_level_inhg(),
                   env2->get_pressure_sea_level_inhg(),
                   fraction));

    result->set_wind_from_heading_deg
        (do_interp_deg(env1->get_wind_from_heading_deg(),
                       env2->get_wind_from_heading_deg(),
                       fraction));

    result->set_wind_speed_kt
        (do_interp(env1->get_wind_speed_kt(),
                   env2->get_wind_speed_kt(),
                   fraction));

    result->set_elevation_ft
        (do_interp(env1->get_elevation_ft(),
                   env2->get_elevation_ft(),
                   fraction));

    result->set_turbulence_magnitude_norm
        (do_interp(env1->get_turbulence_magnitude_norm(),
                   env2->get_turbulence_magnitude_norm(),
                   fraction));

    result->set_turbulence_rate_hz
        (do_interp(env1->get_turbulence_rate_hz(),
                   env2->get_turbulence_rate_hz(),
                   fraction));
}

// end of environment.cxx
