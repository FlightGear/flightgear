// fg_props.cxx -- support for FlightGear properties.
//
// Written by David Megginson, started 2000.
//
// Copyright (C) 2000, 2001 David Megginson - david@megginson.com
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
#  include <simgear/compiler.h>
#endif

#include STL_IOSTREAM

#include <Autopilot/newauto.hxx>
#include <Aircraft/aircraft.hxx>
#include <Time/tmp.hxx>
#include <FDM/UIUCModel/uiuc_aircraftdir.h>
#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "fgfs.hxx"
#include "fg_props.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(istream);
SG_USING_STD(ostream);
#endif

#define DEFAULT_AP_HEADING_LOCK FGAutopilot::FG_DG_HEADING_LOCK

static double getWindNorth ();
static double getWindEast ();
static double getWindDown ();

// Allow the view to be set from two axes (i.e. a joystick hat)
// This needs to be in FGViewer itself, somehow.
static double axisLong = 0.0;
static double axisLat = 0.0;

/**
 * Utility function.
 */
static inline void
_set_view_from_axes ()
{
				// Take no action when hat is centered
  if ( ( axisLong <  0.01 ) &&
       ( axisLong > -0.01 ) &&
       ( axisLat  <  0.01 ) &&
       ( axisLat  > -0.01 )
     )
    return;

  double viewDir = 999;

  /* Do all the quick and easy cases */
  if (axisLong < 0) {		// Longitudinal axis forward
    if (axisLat == axisLong)
      viewDir = 45;
    else if (axisLat == - axisLong)
      viewDir = 315;
    else if (axisLat == 0)
      viewDir = 0;
  } else if (axisLong > 0) {	// Longitudinal axis backward
    if (axisLat == - axisLong)
      viewDir = 135;
    else if (axisLat == axisLong)
      viewDir = 225;
    else if (axisLat == 0)
      viewDir = 180;
  } else if (axisLong = 0) {	// Longitudinal axis neutral
    if (axisLat < 0)
      viewDir = 90;
    else if (axisLat > 0)
      viewDir = 270;
    else return; /* And assertion failure maybe? */
  }

  /* Do all the difficult cases */
  if ( viewDir > 900 )
    viewDir = SGD_RADIANS_TO_DEGREES * atan2 ( -axisLat, -axisLong );
  if ( viewDir < -1 ) viewDir += 360;

//  SG_LOG(SG_INPUT, SG_ALERT, "Joystick Lat=" << axisLat << "   and Long="
//	<< axisLong << "  gave angle=" << viewDir );

  globals->get_current_view()->set_goal_view_offset(viewDir*SGD_DEGREES_TO_RADIANS);
//   globals->get_current_view()->set_view_offset(viewDir*SGD_DEGREES_TO_RADIANS);
}


////////////////////////////////////////////////////////////////////////
// Default property bindings (not yet handled by any module).
////////////////////////////////////////////////////////////////////////

/**
 * Return the current aircraft directory (UIUC) as a string.
 */
static string 
getAircraftDir ()
{
  return aircraft_dir;
}


/**
 * Set the current aircraft directory (UIUC).
 */
static void
setAircraftDir (string dir)
{
  if (getAircraftDir() != dir) {
    aircraft_dir = dir;
//     needReinit(); FIXME!!
  }
}


/**
 * Get the current view offset in degrees.
 */
static double
getViewOffset ()
{
  return (globals->get_current_view()
	  ->get_view_offset() * SGD_RADIANS_TO_DEGREES);
}


static void
setViewOffset (double offset)
{
  globals->get_current_view()->set_view_offset(offset * SGD_DEGREES_TO_RADIANS);
}

static double
getGoalViewOffset ()
{
  return (globals->get_current_view()
	  ->get_goal_view_offset() * SGD_RADIANS_TO_DEGREES);
}

static void
setGoalViewOffset (double offset)
{
  globals->get_current_view()
    ->set_goal_view_offset(offset * SGD_DEGREES_TO_RADIANS);
}


/**
 * Return the current Zulu time.
 */
static string 
getDateString ()
{
  string out;
  char buf[64];
  struct tm * t = globals->get_time_params()->getGmt();
  sprintf(buf, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",
	  t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
	  t->tm_hour, t->tm_min, t->tm_sec);
  out = buf;
  return out;
}


/**
 * Set the current Zulu time.
 */
static void
setDateString (string date_string)
{
  SGTime * st = globals->get_time_params();
  struct tm * current_time = st->getGmt();
  struct tm new_time;

				// Scan for basic ISO format
				// YYYY-MM-DDTHH:MM:SS
  int ret = sscanf(date_string.c_str(), "%d-%d-%dT%d:%d:%d",
		   &(new_time.tm_year), &(new_time.tm_mon),
		   &(new_time.tm_mday), &(new_time.tm_hour),
		   &(new_time.tm_min), &(new_time.tm_sec));

				// Be pretty picky about this, so
				// that strange things don't happen
				// if the save file has been edited
				// by hand.
  if (ret != 6) {
    SG_LOG(SG_INPUT, SG_ALERT, "Date/time string " << date_string
	   << " not in YYYY-MM-DDTHH:MM:SS format; skipped");
    return;
  }

				// OK, it looks like we got six
				// values, one way or another.
  new_time.tm_year -= 1900;
  new_time.tm_mon -= 1;

				// Now, tell flight gear to use
				// the new time.  This was far
				// too difficult, by the way.
  long int warp =
    mktime(&new_time) - mktime(current_time) + globals->get_warp();
  double lon = current_aircraft.fdm_state->get_Longitude();
  double lat = current_aircraft.fdm_state->get_Latitude();
  globals->set_warp(warp);
  st->update(lon, lat, warp);
  fgUpdateSkyAndLightingParams();
}

/**
 * Return the GMT as a string.
 */
static string 
getGMTString ()
{
  string out;
  char buf[16];
  struct tm * t = globals->get_time_params()->getGmt();
  sprintf(buf, " %.2d:%.2d:%.2d",
	  t->tm_hour, t->tm_min, t->tm_sec);
  out = buf;
  return out;
}

/**
 * Return the magnetic variation
 */
static double
getMagVar ()
{
  return globals->get_mag()->get_magvar() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Return the magnetic dip
 */
static double
getMagDip ()
{
  return globals->get_mag()->get_magdip() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Return the current heading in degrees.
 */
static double
getHeadingMag ()
{
  return current_aircraft.fdm_state->get_Psi() * SGD_RADIANS_TO_DEGREES - getMagVar();
}


/**
 * Return the current engine0 rpm
 */
static double
getRPM ()
{
  if ( current_aircraft.fdm_state->get_engine(0) != NULL ) {
      return current_aircraft.fdm_state->get_engine(0)->get_RPM();
  } else {
      return 0.0;
  }
}


/**
 * Return the current engine0 EGT.
 */
static double
getEGT ()
{
  if ( current_aircraft.fdm_state->get_engine(0) != NULL ) {
      return current_aircraft.fdm_state->get_engine(0)->get_EGT();
  } else {
      return 0.0;
  }
}

/**
 * Return the current engine0 CHT.
 */
static double
getCHT ()
{
  if ( current_aircraft.fdm_state->get_engine(0) != NULL ) {
      return current_aircraft.fdm_state->get_engine(0)->get_CHT();
  } else {
      return 0.0;
  }
}

/**
 * Return the current engine0 Manifold Pressure.
 */
static double
getMP ()
{
  if ( current_aircraft.fdm_state->get_engine(0) != NULL ) {
      return current_aircraft.fdm_state->get_engine(0)->get_Manifold_Pressure();
  } else {
      return 0.0;
  }
}


/**
 * Return the current engine0 fuel flow
 */
static double
getFuelFlow ()
{
  if ( current_aircraft.fdm_state->get_engine(0) != NULL ) {
      return current_aircraft.fdm_state->get_engine(0)->get_Fuel_Flow();
  } else {
      return 0.0;
  }
}

/**
 * Return the fuel level in tank 1
 */
static double
getTank1Fuel ()
{
  return current_aircraft.fdm_state->get_Tank1Fuel();
}

static void
setTank1Fuel ( double gals )
{
  current_aircraft.fdm_state->set_Tank1Fuel( gals );
}

/**
 * Return the fuel level in tank 2
 */
static double
getTank2Fuel ()
{
  return current_aircraft.fdm_state->get_Tank2Fuel();
}

static void
setTank2Fuel ( double gals )
{
  current_aircraft.fdm_state->set_Tank2Fuel( gals );
}


/**
 * Get the autopilot altitude lock (true=on).
 */
static bool
getAPAltitudeLock ()
{
    return current_autopilot->get_AltitudeEnabled();
}


/**
 * Set the autopilot altitude lock (true=on).
 */
static void
setAPAltitudeLock (bool lock)
{
  current_autopilot->set_AltitudeMode(FGAutopilot::FG_ALTITUDE_LOCK);
  current_autopilot->set_AltitudeEnabled(lock);
}

/**
 * Get the autopilot target altitude in feet.
 */
static double
getAPAltitude ()
{
  return current_autopilot->get_TargetAltitude() * SG_METER_TO_FEET;
}


/**
 * Set the autopilot target altitude in feet.
 */
static void
setAPAltitude (double altitude)
{
    current_autopilot->set_TargetAltitude( altitude * SG_FEET_TO_METER );
}

/**
 * Get the autopilot altitude lock (true=on).
 */
static bool
getAPGSLock ()
{
    return current_autopilot->get_AltitudeEnabled();
}


/**
 * Set the autopilot altitude lock (true=on).
 */
static void
setAPGSLock (bool lock)
{
  current_autopilot->set_AltitudeMode(FGAutopilot::FG_ALTITUDE_GS1);
  current_autopilot->set_AltitudeEnabled(lock);
}


/**
 * Get the autopilot target altitude in feet.
 */
static double
getAPClimb ()
{
  return current_autopilot->get_TargetClimbRate() * SG_METER_TO_FEET;
}


/**
 * Set the autopilot target altitude in feet.
 */
static void
setAPClimb (double rate)
{
    current_autopilot->set_TargetClimbRate( rate * SG_FEET_TO_METER );
}


/**
 * Get the autopilot heading lock (true=on).
 */
static bool
getAPHeadingLock ()
{
    return
      (current_autopilot->get_HeadingEnabled() &&
       current_autopilot->get_HeadingMode() == DEFAULT_AP_HEADING_LOCK);
}


/**
 * Set the autopilot heading lock (true=on).
 */
static void
setAPHeadingLock (bool lock)
{
    if (lock) {
	current_autopilot->set_HeadingMode(DEFAULT_AP_HEADING_LOCK);
	current_autopilot->set_HeadingEnabled(true);
    } else {
	current_autopilot->set_HeadingEnabled(false);
    }
}


/**
 * Get the autopilot heading bug in degrees.
 */
static double
getAPHeadingBug ()
{
  return current_autopilot->get_DGTargetHeading();
}


/**
 * Set the autopilot heading bug in degrees.
 */
static void
setAPHeadingBug (double heading)
{
  current_autopilot->set_DGTargetHeading( heading );
}


/**
 * Get the autopilot wing leveler lock (true=on).
 */
static bool
getAPWingLeveler ()
{
    return
      (current_autopilot->get_HeadingEnabled() &&
       current_autopilot->get_HeadingMode() == FGAutopilot::FG_TC_HEADING_LOCK);
}


/**
 * Set the autopilot wing leveler lock (true=on).
 */
static void
setAPWingLeveler (bool lock)
{
    if (lock) {
	current_autopilot->set_HeadingMode(FGAutopilot::FG_TC_HEADING_LOCK);
	current_autopilot->set_HeadingEnabled(true);
    } else {
	current_autopilot->set_HeadingEnabled(false);
    }
}

/**
 * Return true if the autopilot is locked to NAV1.
 */
static bool
getAPNAV1Lock ()
{
  return
    (current_autopilot->get_HeadingEnabled() &&
     current_autopilot->get_HeadingMode() == FGAutopilot::FG_HEADING_NAV1);
}


/**
 * Set the autopilot NAV1 lock.
 */
static void
setAPNAV1Lock (bool lock)
{
  if (lock) {
    current_autopilot->set_HeadingMode(FGAutopilot::FG_HEADING_NAV1);
    current_autopilot->set_HeadingEnabled(true);
  } else if (current_autopilot->get_HeadingMode() ==
	     FGAutopilot::FG_HEADING_NAV1) {
    current_autopilot->set_HeadingEnabled(false);
  }
}

/**
 * Get the autopilot autothrottle lock.
 */
static bool
getAPAutoThrottleLock ()
{
  return current_autopilot->get_AutoThrottleEnabled();
}


/**
 * Set the autothrottle lock.
 */
static void
setAPAutoThrottleLock (bool lock)
{
  current_autopilot->set_AutoThrottleEnabled(lock);
}


// kludge
static double
getAPRudderControl ()
{
    if (getAPHeadingLock())
        return current_autopilot->get_TargetHeading();
    else
        return controls.get_rudder();
}

// kludge
static void
setAPRudderControl (double value)
{
    if (getAPHeadingLock()) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "setAPRudderControl " << value );
        value -= current_autopilot->get_TargetHeading();
        current_autopilot->HeadingAdjust(value < 0.0 ? -1.0 : 1.0);
    } else {
        controls.set_rudder(value);
    }
}

// kludge
static double
getAPElevatorControl ()
{
  if (getAPAltitudeLock())
      return current_autopilot->get_TargetAltitude();
  else
    return controls.get_elevator();
}

// kludge
static void
setAPElevatorControl (double value)
{
    if (getAPAltitudeLock()) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "setAPElevatorControl " << value );
        value -= current_autopilot->get_TargetAltitude();
        current_autopilot->AltitudeAdjust(value < 0.0 ? 100.0 : -100.0);
    } else {
        controls.set_elevator(value);
    }
}

// kludge
static double
getAPThrottleControl ()
{
  if (getAPAutoThrottleLock())
    return 0.0;			// always resets
  else
    return controls.get_throttle(0);
}

// kludge
static void
setAPThrottleControl (double value)
{
  if (getAPAutoThrottleLock())
    current_autopilot->AutoThrottleAdjust(value < 0.0 ? -0.01 : 0.01);
  else
    controls.set_throttle(0, value);
}


/**
 * Get the current visibility (meters).
 */
static double
getVisibility ()
{
#ifndef FG_OLD_WEATHER
  return WeatherDatabase->getWeatherVisibility();
#else
  return current_weather.get_visibility();
#endif
}


/**
 * Set the current visibility (meters).
 */
static void
setVisibility (double visibility)
{
#ifndef FG_OLD_WEATHER
  WeatherDatabase->setWeatherVisibility(visibility);
#else
  current_weather.set_visibility(visibility);
#endif
}

/**
 * Get the current wind north velocity (feet/second).
 */
static double
getWindNorth ()
{
  return current_aircraft.fdm_state->get_V_north_airmass();
}


/**
 * Set the current wind north velocity (feet/second).
 */
static void
setWindNorth (double speed)
{
  current_aircraft.fdm_state
    ->set_Velocities_Local_Airmass(speed, getWindEast(), getWindDown());
}


/**
 * Get the current wind east velocity (feet/second).
 */
static double
getWindEast ()
{
  return current_aircraft.fdm_state->get_V_east_airmass();
}


/**
 * Set the current wind east velocity (feet/second).
 */
static void
setWindEast (double speed)
{
  cout << "Set wind-east to " << speed << endl;
  current_aircraft.fdm_state->set_Velocities_Local_Airmass(getWindNorth(),
							   speed,
							   getWindDown());
}


/**
 * Get the current wind down velocity (feet/second).
 */
static double
getWindDown ()
{
  return current_aircraft.fdm_state->get_V_down_airmass();
}


/**
 * Set the current wind down velocity (feet/second).
 */
static void
setWindDown (double speed)
{
  current_aircraft.fdm_state->set_Velocities_Local_Airmass(getWindNorth(),
							   getWindEast(),
							   speed);
}

static double
getFOV ()
{
  return globals->get_current_view()->get_fov();
}

static void
setFOV (double fov)
{
  globals->get_current_view()->set_fov( fov );
}


static void
setViewAxisLong (double axis)
{
  axisLong = axis;
}

static void
setViewAxisLat (double axis)
{
  axisLat = axis;
}


void
fgInitProps ()
{
				// Simulation
  fgTie("/sim/aircraft-dir", getAircraftDir, setAircraftDir);
  fgTie("/sim/view/offset", getViewOffset, setViewOffset);
  fgTie("/sim/view/goal-offset", getGoalViewOffset, setGoalViewOffset);
  fgTie("/sim/time/gmt", getDateString, setDateString);
  fgTie("/sim/time/gmt-string", getGMTString);

				// Orientation
  fgTie("/orientation/heading-magnetic", getHeadingMag);

				// Engine
  fgTie("/engines/engine0/rpm", getRPM);
  fgTie("/engines/engine0/egt", getEGT);
  fgTie("/engines/engine0/cht", getCHT);
  fgTie("/engines/engine0/mp", getMP);
  fgTie("/engines/engine0/fuel-flow", getFuelFlow);

  //consumables
  fgTie("/consumables/fuel/tank1/level", getTank1Fuel, setTank1Fuel, false);
  fgTie("/consumables/fuel/tank2/level", getTank2Fuel, setTank2Fuel, false);

				// Autopilot
  fgTie("/autopilot/locks/altitude", getAPAltitudeLock, setAPAltitudeLock);
  fgTie("/autopilot/settings/altitude", getAPAltitude, setAPAltitude);
  fgTie("/autopilot/locks/glide-slope", getAPGSLock, setAPGSLock);
  fgTie("/autopilot/settings/climb-rate", getAPClimb, setAPClimb, false);
  fgTie("/autopilot/locks/heading", getAPHeadingLock, setAPHeadingLock);
  fgTie("/autopilot/settings/heading-bug", getAPHeadingBug, setAPHeadingBug,
	false);
  fgTie("/autopilot/locks/wing-leveler", getAPWingLeveler, setAPWingLeveler);
  fgTie("/autopilot/locks/nav1", getAPNAV1Lock, setAPNAV1Lock);
  fgTie("/autopilot/locks/auto-throttle",
	getAPAutoThrottleLock, setAPAutoThrottleLock);
  fgTie("/autopilot/control-overrides/rudder",
	getAPRudderControl, setAPRudderControl);
  fgTie("/autopilot/control-overrides/elevator",
	getAPElevatorControl, setAPElevatorControl);
  fgTie("/autopilot/control-overrides/throttle",
	getAPThrottleControl, setAPThrottleControl);

				// Environment
  fgTie("/environment/visibility", getVisibility, setVisibility);
  fgTie("/environment/wind-north", getWindNorth, setWindNorth);
  fgTie("/environment/wind-east", getWindEast, setWindEast);
  fgTie("/environment/wind-down", getWindDown, setWindDown);
  fgTie("/environment/magnetic-variation", getMagVar);
  fgTie("/environment/magnetic-dip", getMagDip);

				// View
  fgTie("/sim/field-of-view", getFOV, setFOV);
  fgTie("/sim/view/axes/long", (double(*)())0, setViewAxisLong);
  fgTie("/sim/view/axes/lat", (double(*)())0, setViewAxisLat);
}


void
fgUpdateProps ()
{
  _set_view_from_axes();
}



////////////////////////////////////////////////////////////////////////
// Save and restore.
////////////////////////////////////////////////////////////////////////


/**
 * Save the current state of the simulator to a stream.
 */
bool
fgSaveFlight (ostream &output)
{
  return writeProperties(output, globals->get_props());
}


/**
 * Restore the current state of the simulator from a stream.
 */
bool
fgLoadFlight (istream &input)
{
  SGPropertyNode props;
  if (readProperties(input, &props)) {
    copyProperties(&props, globals->get_props());
				// When loading a flight, make it the
				// new initial state.
    globals->saveInitialState();
  } else {
    SG_LOG(SG_INPUT, SG_ALERT, "Error restoring flight; aborted");
    return false;
  }

  return true;
}

// end of fg_props.cxx

