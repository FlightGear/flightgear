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

#include <simgear/misc/exception.hxx>

#include STL_IOSTREAM

#include <ATC/ATCdisplay.hxx>
#include <Autopilot/newauto.hxx>
#include <Aircraft/aircraft.hxx>
#include <Time/tmp.hxx>
#include <FDM/UIUCModel/uiuc_aircraftdir.h>
#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif
#include <Objects/matlib.hxx>

#include <GUI/gui.h>

#include "globals.hxx"
#include "fgfs.hxx"
#include "fg_props.hxx"
#include "viewmgr.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(istream);
SG_USING_STD(ostream);
#endif

static double getWindNorth ();
static double getWindEast ();
static double getWindDown ();

// Allow the view to be set from two axes (i.e. a joystick hat)
// This needs to be in FGViewer itself, somehow.
static double axisLong = 0.0;
static double axisLat = 0.0;

static bool winding_ccw = true; // FIXME: temporary

static bool fdm_data_logging = false; // FIXME: temporary


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
  } else if (axisLong == 0) {	// Longitudinal axis neutral
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

struct LogClassMapping {
  sgDebugClass c;
  string name;
  LogClassMapping(sgDebugClass cc, string nname) { c = cc; name = nname; }
};

LogClassMapping log_class_mappings [] = {
  LogClassMapping(SG_NONE, "none"),
  LogClassMapping(SG_TERRAIN, "terrain"),
  LogClassMapping(SG_ASTRO, "astro"),
  LogClassMapping(SG_FLIGHT, "flight"),
  LogClassMapping(SG_INPUT, "input"),
  LogClassMapping(SG_GL, "gl"),
  LogClassMapping(SG_VIEW, "view"),
  LogClassMapping(SG_COCKPIT, "cockpit"),
  LogClassMapping(SG_GENERAL, "general"),
  LogClassMapping(SG_MATH, "math"),
  LogClassMapping(SG_EVENT, "event"),
  LogClassMapping(SG_AIRCRAFT, "aircraft"),
  LogClassMapping(SG_AUTOPILOT, "autopilot"),
  LogClassMapping(SG_IO, "io"),
  LogClassMapping(SG_CLIPPER, "clipper"),
  LogClassMapping(SG_NETWORK, "network"),
  LogClassMapping(SG_UNDEFD, "")
};


/**
 * Get the logging classes.
 */
static string
getLoggingClasses ()
{
  sgDebugClass classes = logbuf::get_log_classes();
  string result = "";
  for (int i = 0; log_class_mappings[i].c != SG_UNDEFD; i++) {
    if ((classes&log_class_mappings[i].c) > 0) {
      if (result != (string)"")
	result += '|';
      result += log_class_mappings[i].name;
    }
  }
  return result;
}


static void addLoggingClass (const string &name)
{
  sgDebugClass classes = logbuf::get_log_classes();
  for (int i = 0; log_class_mappings[i].c != SG_UNDEFD; i++) {
    if (name == log_class_mappings[i].name) {
      logbuf::set_log_classes(sgDebugClass(classes|log_class_mappings[i].c));
      return;
    }
  }
  SG_LOG(SG_GENERAL, SG_ALERT, "Unknown logging class: " << name);
}


/**
 * Set the logging classes.
 */
static void
setLoggingClasses (string classes)
{
  logbuf::set_log_classes(SG_NONE);

  if (classes == "none") {
    SG_LOG(SG_GENERAL, SG_INFO, "Disabled all logging classes");
    return;
  }

  if (classes == "" || classes == "all") { // default
    logbuf::set_log_classes(SG_ALL);
    SG_LOG(SG_GENERAL, SG_INFO, "Enabled all logging classes: "
	   << getLoggingClasses());
    return;
  }

  string rest = classes;
  string name = "";
  int sep = rest.find('|');
  while (sep > 0) {
    name = rest.substr(0, sep);
    rest = rest.substr(sep+1);
    addLoggingClass(name);
    sep = rest.find('|');
  }
  addLoggingClass(rest);
  SG_LOG(SG_GENERAL, SG_INFO, "Set logging classes to "
	 << getLoggingClasses());
}


/**
 * Get the logging priority.
 */
static string
getLoggingPriority ()
{
  switch (logbuf::get_log_priority()) {
  case SG_BULK:
    return "bulk";
  case SG_DEBUG:
    return "debug";
  case SG_INFO:
    return "info";
  case SG_WARN:
    return "warn";
  case SG_ALERT:
    return "alert";
  default:
    SG_LOG(SG_GENERAL, SG_WARN, "Internal: Unknown logging priority number: "
	   << logbuf::get_log_priority());
    return "unknown";
  }
}


/**
 * Set the logging priority.
 */
static void
setLoggingPriority (string priority)
{
  if (priority == "bulk") {
    logbuf::set_log_priority(SG_BULK);
  } else if (priority == "debug") {
    logbuf::set_log_priority(SG_DEBUG);
  } else if (priority == "" || priority == "info") { // default
    logbuf::set_log_priority(SG_INFO);
  } else if (priority == "warn") {
    logbuf::set_log_priority(SG_WARN);
  } else if (priority == "alert") {
    logbuf::set_log_priority(SG_ALERT);
  } else {
    SG_LOG(SG_GENERAL, SG_WARN, "Unknown logging priority " << priority);
  }
  SG_LOG(SG_GENERAL, SG_INFO, "Logging priority is " << getLoggingPriority());
}


/**
 * Get the pause state of the sim.
 */
static bool
getFreeze ()
{
  return globals->get_freeze();
}


/**
 * Set the pause state of the sim.
 */
static void
setFreeze (bool freeze)
{
    globals->set_freeze(freeze);
    if ( freeze ) {
        // BusyCursor( 0 );
        current_atcdisplay->CancelRepeatingMessage();
        current_atcdisplay->RegisterRepeatingMessage("****    SIM IS PAUSED    ****    SIM IS PAUSED    ****");
    } else {
        // BusyCursor( 1 );
        current_atcdisplay->CancelRepeatingMessage();
    }
}

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
    while ( offset < 0 ) {
	offset += 360.0;
    }
    while ( offset > 360.0 ) {
	offset -= 360.0;
    }
    // Snap to center if we are close
    if ( fabs(offset) < 1.0 ||  fabs(offset) > 359.0 ) {
	offset = 0.0;
    }

    globals->get_current_view()
	->set_goal_view_offset(offset * SGD_DEGREES_TO_RADIANS);
}


/**
 * Pilot position offset from CG.
 */
static float
getPilotPositionXOffset ()
{
  FGViewer * pilot_view = globals->get_viewmgr()->get_view(0);
  float * offset = pilot_view->get_pilot_offset();
  return offset[0];
}

static void
setPilotPositionXOffset (float x)
{
  FGViewer * pilot_view = globals->get_viewmgr()->get_view(0);
  float * offset = pilot_view->get_pilot_offset();
  pilot_view->set_pilot_offset(x, offset[1], offset[2]);
}

static float
getPilotPositionYOffset ()
{
  FGViewer * pilot_view = globals->get_viewmgr()->get_view(0);
  float * offset = pilot_view->get_pilot_offset();
  return offset[1];
}

static void
setPilotPositionYOffset (float y)
{
  FGViewer * pilot_view = globals->get_viewmgr()->get_view(0);
  float * offset = pilot_view->get_pilot_offset();
  pilot_view->set_pilot_offset(offset[0], y, offset[2]);
}

static float
getPilotPositionZOffset ()
{
  FGViewer * pilot_view = globals->get_viewmgr()->get_view(0);
  float * offset = pilot_view->get_pilot_offset();
  return offset[2];
}

static void
setPilotPositionZOffset (float z)
{
  FGViewer * pilot_view = globals->get_viewmgr()->get_view(0);
  float * offset = pilot_view->get_pilot_offset();
  pilot_view->set_pilot_offset(offset[0], offset[1], z);
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
 * Get the texture rendering state.
 */
static bool
getTextures ()
{
  return (material_lib.get_step() == 0);
}


/**
 * Set the texture rendering state.
 */
static void
setTextures (bool textures)
{
  if (textures)
    material_lib.set_step(0);
  else
    material_lib.set_step(1);
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
    return (current_autopilot->get_AltitudeEnabled() &&
	    current_autopilot->get_AltitudeMode()
	    == FGAutopilot::FG_ALTITUDE_LOCK);
}


/**
 * Set the autopilot altitude lock (true=on).
 */
static void
setAPAltitudeLock (bool lock)
{
  if (lock)
    current_autopilot->set_AltitudeMode(FGAutopilot::FG_ALTITUDE_LOCK);
  if (current_autopilot->get_AltitudeMode() == FGAutopilot::FG_ALTITUDE_LOCK)
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
    return (current_autopilot->get_AltitudeEnabled() &&
	    (current_autopilot->get_AltitudeMode()
	     == FGAutopilot::FG_ALTITUDE_GS1));
}


/**
 * Set the autopilot altitude lock (true=on).
 */
static void
setAPGSLock (bool lock)
{
  if (lock)
    current_autopilot->set_AltitudeMode(FGAutopilot::FG_ALTITUDE_GS1);
  if (current_autopilot->get_AltitudeMode() == FGAutopilot::FG_ALTITUDE_GS1)
    current_autopilot->set_AltitudeEnabled(lock);
}


/**
 * Get the autopilot terrain lock (true=on).
 */
static bool
getAPTerrainLock ()
{
    return (current_autopilot->get_AltitudeEnabled() &&
	    (current_autopilot->get_AltitudeMode()
	     == FGAutopilot::FG_ALTITUDE_TERRAIN));
}


/**
 * Set the autopilot terrain lock (true=on).
 */
static void
setAPTerrainLock (bool lock)
{
  if (lock) {
    current_autopilot->set_AltitudeMode(FGAutopilot::FG_ALTITUDE_TERRAIN);
    current_autopilot
      ->set_TargetAGL(current_aircraft.fdm_state->get_Altitude_AGL() *
		      SG_FEET_TO_METER);
    cout << "Target AGL = "
	 << current_aircraft.fdm_state->get_Altitude_AGL() * SG_FEET_TO_METER
	 << endl;
  }
  if (current_autopilot->get_AltitudeMode() == FGAutopilot::FG_ALTITUDE_TERRAIN)
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
  if (lock)
    current_autopilot->set_HeadingMode(DEFAULT_AP_HEADING_LOCK);
  if (current_autopilot->get_HeadingMode() == DEFAULT_AP_HEADING_LOCK)
    current_autopilot->set_HeadingEnabled(lock);
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
    if (lock)
	current_autopilot->set_HeadingMode(FGAutopilot::FG_TC_HEADING_LOCK);
    if (current_autopilot->get_HeadingMode() == FGAutopilot::FG_TC_HEADING_LOCK)
      current_autopilot->set_HeadingEnabled(lock);
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
  if (lock)
    current_autopilot->set_HeadingMode(FGAutopilot::FG_HEADING_NAV1);
  if (current_autopilot->get_HeadingMode() == FGAutopilot::FG_HEADING_NAV1)
    current_autopilot->set_HeadingEnabled(lock);
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
        return globals->get_controls()->get_rudder();
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
        globals->get_controls()->set_rudder(value);
    }
}

// kludge
static double
getAPElevatorControl ()
{
  if (getAPAltitudeLock())
      return current_autopilot->get_TargetAltitude();
  else
    return globals->get_controls()->get_elevator();
}

// kludge
static void
setAPElevatorControl (double value)
{
    if (value != 0 && getAPAltitudeLock()) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "setAPElevatorControl " << value );
        value -= current_autopilot->get_TargetAltitude();
        current_autopilot->AltitudeAdjust(value < 0.0 ? 100.0 : -100.0);
    } else {
        globals->get_controls()->set_elevator(value);
    }
}

// kludge
static double
getAPThrottleControl ()
{
  if (getAPAutoThrottleLock())
    return 0.0;			// always resets
  else
    return globals->get_controls()->get_throttle(0);
}

// kludge
static void
setAPThrottleControl (double value)
{
  if (getAPAutoThrottleLock())
    current_autopilot->AutoThrottleAdjust(value < 0.0 ? -0.01 : 0.01);
  else
    globals->get_controls()->set_throttle(FGControls::ALL_ENGINES, value);
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

/*
 * Set the current engine0 running flag.
 */
static void
setRunningFlag (bool flag)
{
  if ( current_aircraft.fdm_state->get_num_engines() > 0 ) {
    current_aircraft.fdm_state->get_engine(0)->set_Running_Flag( flag );
  }
}

/*
 * Set the current engine0 cranking flag.
 */
//Although there is no real reason to want to tell the engine that it is cranking,
//this is currently necessary to avoid the cranking sound being played 
//before the engine inits.
static void
setCrankingFlag (bool flag)
{
  if ( current_aircraft.fdm_state->get_num_engines() > 0 ) {
    current_aircraft.fdm_state->get_engine(0)->set_Cranking_Flag( flag );
  }
}

static double
getFOV ()
{
  return globals->get_current_view()->get_fov();
}

static void
setFOV (double fov)
{
  if ( fov < 180 ) {
      globals->get_current_view()->set_fov( fov );
  }
}

static long
getWarp ()
{
  return globals->get_warp();
}

static void
setWarp (long warp)
{
  globals->set_warp(warp);
}

static long
getWarpDelta ()
{
  return globals->get_warp_delta();
}

static void
setWarpDelta (long delta)
{
  globals->set_warp_delta(delta);
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

static bool
getWindingCCW ()
{
  return winding_ccw;
}

static void
setWindingCCW (bool state)
{
  winding_ccw = state;
  if ( winding_ccw )
    glFrontFace ( GL_CCW );
  else
    glFrontFace ( GL_CW );
}

static bool
getFullScreen ()
{
#if defined(FX) && !defined(WIN32)
  return globals->get_fullscreen();
#else
  return false;
#endif
}

static void
setFullScreen (bool state)
{
#if defined(FX) && !defined(WIN32)
  globals->set_fullscreen(state);
#  if defined(XMESA_FX_FULLSCREEN) && defined(XMESA_FX_WINDOW)
  XMesaSetFXmode( state ? XMESA_FX_FULLSCREEN : XMESA_FX_WINDOW );
#  endif
#endif
}

static bool
getFDMDataLogging ()
{
  return fdm_data_logging;
}

static void
setFDMDataLogging (bool state)
{
				// kludge; no getter or setter available
  if (state != fdm_data_logging) {
    fgToggleFDMdataLogging();
    fdm_data_logging = state;
  }
}


////////////////////////////////////////////////////////////////////////
// Tie the properties.
////////////////////////////////////////////////////////////////////////

void
fgInitProps ()
{
				// Simulation
  fgTie("/sim/logging/priority", getLoggingPriority, setLoggingPriority);
  fgTie("/sim/logging/classes", getLoggingClasses, setLoggingClasses);
  fgTie("/sim/freeze", getFreeze, setFreeze);
  fgTie("/sim/aircraft-dir", getAircraftDir, setAircraftDir);
  fgTie("/sim/view/offset-deg", getViewOffset, setViewOffset, false);
  fgSetArchivable("/sim/view/offset-deg");
  fgTie("/sim/view/goal-offset-deg", getGoalViewOffset, setGoalViewOffset, false);
  fgSetArchivable("/sim/view/goal-offset-deg");
  fgTie("/sim/view/pilot/x-offset-m",
	getPilotPositionXOffset, setPilotPositionXOffset);
  fgSetArchivable("/sim/view/pilot/x-offset-m");
  fgTie("/sim/view/pilot/y-offset-m",
	getPilotPositionYOffset, setPilotPositionYOffset);
  fgSetArchivable("/sim/view/pilot/y-offset-m");
  fgTie("/sim/view/pilot/z-offset-m",
	getPilotPositionZOffset, setPilotPositionZOffset);
  fgSetArchivable("/sim/view/pilot/z-offset-m");
  fgTie("/sim/time/gmt", getDateString, setDateString);
  fgSetArchivable("/sim/time/gmt");
  fgTie("/sim/time/gmt-string", getGMTString);
  fgTie("/sim/rendering/textures", getTextures, setTextures);

				// Orientation
  fgTie("/orientation/heading-magnetic-deg", getHeadingMag);

  //consumables
  fgTie("/consumables/fuel/tank[0]/level-gal_us",
	getTank1Fuel, setTank1Fuel, false);
  fgSetArchivable("/consumables/fuel/tank[0]/level-gal_us");
  fgTie("/consumables/fuel/tank[1]/level-gal_us",
	getTank2Fuel, setTank2Fuel, false);
  fgSetArchivable("/consumables/fuel/tank[1]/level-gal_us");

				// Autopilot
  fgTie("/autopilot/locks/altitude", getAPAltitudeLock, setAPAltitudeLock);
  fgSetArchivable("/autopilot/locks/altitude");
  std::cout << "[AP] altitude = " << fgGetDouble("/autopilot/settings/altitude-ft") << std::endl;
  fgTie("/autopilot/settings/altitude-ft", getAPAltitude, setAPAltitude);
  fgSetArchivable("/autopilot/settings/altitude-ft");
  std::cout << "[AP] altitude = " << fgGetDouble("/autopilot/settings/altitude-ft") << std::endl;
  fgTie("/autopilot/locks/glide-slope", getAPGSLock, setAPGSLock);
  fgSetArchivable("/autopilot/locks/glide-slope");
  fgTie("/autopilot/locks/terrain", getAPTerrainLock, setAPTerrainLock);
  fgSetArchivable("/autopilot/locks/terrain");
  fgTie("/autopilot/settings/climb-rate-fpm", getAPClimb, setAPClimb, false);
  fgSetArchivable("/autopilot/settings/climb-rate-fpm");
  fgTie("/autopilot/locks/heading", getAPHeadingLock, setAPHeadingLock);
  fgSetArchivable("/autopilot/locks/heading");
  fgTie("/autopilot/settings/heading-bug-deg",
	getAPHeadingBug, setAPHeadingBug);
  fgSetArchivable("/autopilot/settings/heading-bug-deg");
  fgTie("/autopilot/locks/wing-leveler", getAPWingLeveler, setAPWingLeveler);
  fgSetArchivable("/autopilot/locks/wing-leveler");
  fgTie("/autopilot/locks/nav[0]", getAPNAV1Lock, setAPNAV1Lock);
  fgSetArchivable("/autopilot/locks/nav[0]");
  fgTie("/autopilot/locks/auto-throttle",
	getAPAutoThrottleLock, setAPAutoThrottleLock);
  fgSetArchivable("/autopilot/locks/auto-throttle");
  fgTie("/autopilot/control-overrides/rudder",
	getAPRudderControl, setAPRudderControl);
  fgSetArchivable("/autopilot/control-overrides/rudder");
  fgTie("/autopilot/control-overrides/elevator",
	getAPElevatorControl, setAPElevatorControl);
  fgSetArchivable("/autopilot/control-overrides/elevator");
  fgTie("/autopilot/control-overrides/throttle",
	getAPThrottleControl, setAPThrottleControl);
  fgSetArchivable("/autopilot/control-overrides/throttle");

				// Environment
  fgTie("/environment/visibility-m", getVisibility, setVisibility);
  fgSetArchivable("/environment/visibility-m");
  fgTie("/environment/wind-north-fps", getWindNorth, setWindNorth);
  fgSetArchivable("/environment/wind-north-fps");
  fgTie("/environment/wind-east-fps", getWindEast, setWindEast);
  fgSetArchivable("/environment/wind-east-fps");
  fgTie("/environment/wind-down-fps", getWindDown, setWindDown);
  fgSetArchivable("/environment/wind-down-fps");

  fgTie("/environment/magnetic-variation-deg", getMagVar);
  fgTie("/environment/magnetic-dip-deg", getMagDip);

				// View
  fgTie("/sim/field-of-view", getFOV, setFOV);
  fgSetArchivable("/sim/field-of-view");
  fgTie("/sim/time/warp", getWarp, setWarp, false);
  fgTie("/sim/time/warp-delta", getWarpDelta, setWarpDelta);
  fgTie("/sim/view/axes/long", (double(*)())0, setViewAxisLong);
  fgTie("/sim/view/axes/lat", (double(*)())0, setViewAxisLat);

				// Misc. Temporary junk.
  fgTie("/sim/temp/winding-ccw", getWindingCCW, setWindingCCW, false);
  fgTie("/sim/temp/full-screen", getFullScreen, setFullScreen);
  fgTie("/sim/temp/fdm-data-logging", getFDMDataLogging, setFDMDataLogging);
	
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
  try {
    writeProperties(output, globals->get_props());
  } catch (const sg_exception &e) {
    guiErrorMessage("Error saving flight: ", e);
    return false;
  }
  return true;
}


/**
 * Restore the current state of the simulator from a stream.
 */
bool
fgLoadFlight (istream &input)
{
  SGPropertyNode props;
  try {
    readProperties(input, &props);
  } catch (const sg_exception &e) {
    guiErrorMessage("Error reading saved flight: ", e);
    return false;
  }
  copyProperties(&props, globals->get_props());
  // When loading a flight, make it the
  // new initial state.
  globals->saveInitialState();
  return true;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGCondition.
////////////////////////////////////////////////////////////////////////

FGCondition::FGCondition ()
{
}

FGCondition::~FGCondition ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPropertyCondition.
////////////////////////////////////////////////////////////////////////

FGPropertyCondition::FGPropertyCondition (const string &propname)
  : _node(fgGetNode(propname, true))
{
}

FGPropertyCondition::~FGPropertyCondition ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGNotCondition.
////////////////////////////////////////////////////////////////////////

FGNotCondition::FGNotCondition (FGCondition * condition)
  : _condition(condition)
{
}

FGNotCondition::~FGNotCondition ()
{
  delete _condition;
}

bool
FGNotCondition::test () const
{
  return !(_condition->test());
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAndCondition.
////////////////////////////////////////////////////////////////////////

FGAndCondition::FGAndCondition ()
{
}

FGAndCondition::~FGAndCondition ()
{
  for (unsigned int i = 0; i < _conditions.size(); i++)
    delete _conditions[i];
}

bool
FGAndCondition::test () const
{
  int nConditions = _conditions.size();
  for (int i = 0; i < nConditions; i++) {
    if (!_conditions[i]->test())
      return false;
  }
  return true;
}

void
FGAndCondition::addCondition (FGCondition * condition)
{
  _conditions.push_back(condition);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGOrCondition.
////////////////////////////////////////////////////////////////////////

FGOrCondition::FGOrCondition ()
{
}

FGOrCondition::~FGOrCondition ()
{
  for (unsigned int i = 0; i < _conditions.size(); i++)
    delete _conditions[i];
}

bool
FGOrCondition::test () const
{
  int nConditions = _conditions.size();
  for (int i = 0; i < nConditions; i++) {
    if (_conditions[i]->test())
      return true;
  }
  return false;
}

void
FGOrCondition::addCondition (FGCondition * condition)
{
  _conditions.push_back(condition);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGComparisonCondition.
////////////////////////////////////////////////////////////////////////

static int
doComparison (const SGPropertyNode * left, const SGPropertyNode *right)
{
  switch (left->getType()) {
  case SGPropertyNode::BOOL: {
    bool v1 = left->getBoolValue();
    bool v2 = right->getBoolValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::INT: {
    int v1 = left->getIntValue();
    int v2 = right->getIntValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::LONG: {
    long v1 = left->getLongValue();
    long v2 = right->getLongValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::FLOAT: {
    float v1 = left->getFloatValue();
    float v2 = right->getFloatValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::DOUBLE: {
    double v1 = left->getDoubleValue();
    double v2 = right->getDoubleValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  case SGPropertyNode::STRING: 
  case SGPropertyNode::NONE:
  case SGPropertyNode::UNSPECIFIED: {
    string v1 = left->getStringValue();
    string v2 = right->getStringValue();
    if (v1 < v2)
      return FGComparisonCondition::LESS_THAN;
    else if (v1 > v2)
      return FGComparisonCondition::GREATER_THAN;
    else
      return FGComparisonCondition::EQUALS;
    break;
  }
  }
  throw sg_exception("Unrecognized node type");
  return 0;
}


FGComparisonCondition::FGComparisonCondition (Type type, bool reverse)
  : _type(type),
    _reverse(reverse),
    _left_property(0),
    _right_property(0),
    _right_value(0)
{
}

FGComparisonCondition::~FGComparisonCondition ()
{
  delete _right_value;
}

bool
FGComparisonCondition::test () const
{
				// Always fail if incompletely specified
  if (_left_property == 0 ||
      (_right_property == 0 && _right_value == 0))
    return false;

				// Get LESS_THAN, EQUALS, or GREATER_THAN
  int cmp =
    doComparison(_left_property,
		 (_right_property != 0 ? _right_property : _right_value));
  if (!_reverse)
    return (cmp == _type);
  else
    return (cmp != _type);
}

void
FGComparisonCondition::setLeftProperty (const string &propname)
{
  _left_property = fgGetNode(propname, true);
}

void
FGComparisonCondition::setRightProperty (const string &propname)
{
  delete _right_value;
  _right_value = 0;
  _right_property = fgGetNode(propname, true);
}

void
FGComparisonCondition::setRightValue (const SGPropertyNode *node)
{
  _right_property = 0;
  delete _right_value;
  _right_value = new SGPropertyNode(*node);
}



////////////////////////////////////////////////////////////////////////
// Read a condition and use it if necessary.
////////////////////////////////////////////////////////////////////////

                                // Forward declaration
static FGCondition * readCondition (const SGPropertyNode * node);

static FGCondition *
readPropertyCondition (const SGPropertyNode * node)
{
  return new FGPropertyCondition(node->getStringValue());
}

static FGCondition *
readNotCondition (const SGPropertyNode * node)
{
  int nChildren = node->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * child = node->getChild(i);
    FGCondition * condition = readCondition(child);
    if (condition != 0)
      return new FGNotCondition(condition);
  }
  SG_LOG(SG_COCKPIT, SG_ALERT, "Panel: empty 'not' condition");
  return 0;
}

static FGCondition *
readAndConditions (const SGPropertyNode * node)
{
  FGAndCondition * andCondition = new FGAndCondition;
  int nChildren = node->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * child = node->getChild(i);
    FGCondition * condition = readCondition(child);
    if (condition != 0)
      andCondition->addCondition(condition);
  }
  return andCondition;
}

static FGCondition *
readOrConditions (const SGPropertyNode * node)
{
  FGOrCondition * orCondition = new FGOrCondition;
  int nChildren = node->nChildren();
  for (int i = 0; i < nChildren; i++) {
    const SGPropertyNode * child = node->getChild(i);
    FGCondition * condition = readCondition(child);
    if (condition != 0)
      orCondition->addCondition(condition);
  }
  return orCondition;
}

static FGCondition *
readComparison (const SGPropertyNode * node,
		FGComparisonCondition::Type type,
		bool reverse)
{
  FGComparisonCondition * condition = new FGComparisonCondition(type, reverse);
  condition->setLeftProperty(node->getStringValue("property[0]"));
  if (node->hasValue("property[1]"))
    condition->setRightProperty(node->getStringValue("property[1]"));
  else
    condition->setRightValue(node->getChild("value", 0));

  return condition;
}

static FGCondition *
readCondition (const SGPropertyNode * node)
{
  const string &name = node->getName();
  if (name == "property")
    return readPropertyCondition(node);
  else if (name == "not")
    return readNotCondition(node);
  else if (name == "and")
    return readAndConditions(node);
  else if (name == "or")
    return readOrConditions(node);
  else if (name == "less-than")
    return readComparison(node, FGComparisonCondition::LESS_THAN, false);
  else if (name == "less-than-equals")
    return readComparison(node, FGComparisonCondition::GREATER_THAN, true);
  else if (name == "greater-than")
    return readComparison(node, FGComparisonCondition::GREATER_THAN, false);
  else if (name == "greater-than-equals")
    return readComparison(node, FGComparisonCondition::LESS_THAN, true);
  else if (name == "equals")
    return readComparison(node, FGComparisonCondition::EQUALS, false);
  else if (name == "not-equals")
    return readComparison(node, FGComparisonCondition::EQUALS, true);
  else
    return 0;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGConditional.
////////////////////////////////////////////////////////////////////////

FGConditional::FGConditional ()
  : _condition (0)
{
}

FGConditional::~FGConditional ()
{
  delete _condition;
}

void
FGConditional::setCondition (FGCondition * condition)
{
  delete _condition;
  _condition = condition;
}

bool
FGConditional::test () const
{
  return ((_condition == 0) || _condition->test());
}



// The top-level is always an implicit 'and' group
FGCondition *
fgReadCondition (const SGPropertyNode * node)
{
  return readAndConditions(node);
}


// end of fg_props.cxx
