// bfi.cxx - Big Friendly Interface implementation
//
// Written by David Megginson, started February, 2000.
//
// Copyright (C) 2000  David Megginson - david@megginson.com
//
// THIS CLASS IS DEPRECATED; USE THE PROPERTY MANAGER INSTEAD.
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

#include "fgfs.hxx"

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/math/sg_types.hxx>
#include <simgear/misc/props.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Aircraft/aircraft.hxx>
#include <FDM/UIUCModel/uiuc_aircraftdir.h>
#include <Controls/controls.hxx>
#include <Autopilot/newauto.hxx>
#include <Scenery/scenery.hxx>
#include <Time/light.hxx>
#include <Time/event.hxx>
#include <Time/sunpos.hxx>
#include <Time/tmp.hxx>
#include <Cockpit/radiostack.hxx>
#include <Cockpit/panel.hxx>
#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "globals.hxx"
#include "fg_init.hxx"
#include "fg_props.hxx"

SG_USING_NAMESPACE(std);


#include "bfi.hxx"



////////////////////////////////////////////////////////////////////////
// Static variables.
////////////////////////////////////////////////////////////////////////

				// Yech -- not thread-safe, etc. etc.
static bool _needReinit = false;
static string _temp;

static inline void needReinit ()
{
  _needReinit = true;
}


/**
 * Reinitialize FGFS to use the new BFI settings.
 */
static inline void
reinit ()
{
				// Save the state of everything
				// that's going to get clobbered
				// when we reinit the subsystems.

  SG_LOG(SG_GENERAL, SG_INFO, "Starting BFI reinit");

				// TODO: add more AP stuff
  bool apHeadingLock = FGBFI::getAPHeadingLock();
  bool apAltitudeLock = FGBFI::getAPAltitudeLock();
  double apAltitude = FGBFI::getAPAltitude();
  bool gpsLock = FGBFI::getGPSLock();
  // double gpsLatitude = FGBFI::getGPSTargetLatitude();
  // double gpsLongitude = FGBFI::getGPSTargetLongitude();

  fgReInitSubsystems();

				// FIXME: this is wrong.
				// All of these are scheduled events,
				// and it should be possible to force
				// them all to run once.
  fgUpdateSunPos();
  fgUpdateMoonPos();
  cur_light_params.Update();
  fgUpdateLocalTime();
#ifndef FG_OLD_WEATHER
  fgUpdateWeatherDatabase();
#endif
  current_radiostack->search();

				// Restore all of the old states.
  FGBFI::setAPHeadingLock(apHeadingLock);
  FGBFI::setAPAltitudeLock(apAltitudeLock);
  FGBFI::setAPAltitude(apAltitude);
  FGBFI::setGPSLock(gpsLock);

  _needReinit = false;

  SG_LOG(SG_GENERAL, SG_INFO, "Ending BFI reinit");
}

// BEGIN: kludge
// Allow the view to be set from two axes (i.e. a joystick hat)
// This needs to be in FGViewer itself, somehow.
static double axisLong = 0.0;
static double axisLat = 0.0;

static inline void
_set_view_from_axes ()
{
				// Take no action when hat is centered
  if (axisLong == 0 && axisLat == 0)
    return;

  double viewDir = 0;

  if (axisLong < 0) {		// Longitudinal axis forward
    if (axisLat < 0)
      viewDir = 45;
    else if (axisLat > 0)
      viewDir = 315;
    else
      viewDir = 0;
  } else if (axisLong > 0) {	// Longitudinal axis backward
    if (axisLat < 0)
      viewDir = 135;
    else if (axisLat > 0)
      viewDir = 225;
    else
      viewDir = 180;
  } else {			// Longitudinal axis neutral
    if (axisLat < 0)
      viewDir = 90;
    else
      viewDir = 270;
  }

  globals->get_current_view()->set_goal_view_offset(viewDir*SGD_DEGREES_TO_RADIANS);
//   globals->get_current_view()->set_view_offset(viewDir*SGD_DEGREES_TO_RADIANS);
}

// END: kludge



////////////////////////////////////////////////////////////////////////
// Local functions
////////////////////////////////////////////////////////////////////////


/**
 * Initialize the BFI by binding its functions to properties.
 *
 * TODO: perhaps these should migrate into the individual modules
 * (i.e. they should register themselves).
 */
void
FGBFI::init ()
{
  SG_LOG(SG_GENERAL, SG_INFO, "Starting BFI init");

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

				// Weather
  fgTie("/environment/visibility", getVisibility, setVisibility);
  fgTie("/environment/wind-north", getWindNorth, setWindNorth);
  fgTie("/environment/wind-east", getWindEast, setWindEast);
  fgTie("/environment/wind-down", getWindDown, setWindDown);

				// View
  fgTie("/sim/field-of-view", getFOV, setFOV);
  fgTie("/sim/view/axes/long", (double(*)())0, setViewAxisLong);
  fgTie("/sim/view/axes/lat", (double(*)())0, setViewAxisLat);

  _needReinit = false;

  SG_LOG(SG_GENERAL, SG_INFO, "Ending BFI init");
}


/**
 * Reinitialize FGFS if required.
 *
 * Some changes (especially those in aircraft position) require that
 * FGFS be reinitialized afterwards.  Rather than reinitialize after
 * every change, the setter methods simply set a flag so that there
 * can be a single reinit at the end of the frame.
 */
void
FGBFI::update ()
{
  _set_view_from_axes();
  if (_needReinit) {
    reinit();
  }
}



////////////////////////////////////////////////////////////////////////
// Simulation.
////////////////////////////////////////////////////////////////////////


/**
 * Return the current aircraft directory (UIUC) as a string.
 */
string 
FGBFI::getAircraftDir ()
{
  return aircraft_dir;
}


/**
 * Set the current aircraft directory (UIUC).
 */
void
FGBFI::setAircraftDir (string dir)
{
  if (getAircraftDir() != dir) {
    aircraft_dir = dir;
    needReinit();
  }
}


/**
 * Get the current view offset in degrees.
 */
double
FGBFI::getViewOffset ()
{
  return (globals->get_current_view()
	  ->get_view_offset() * SGD_RADIANS_TO_DEGREES);
}

void
FGBFI::setViewOffset (double offset)
{
  globals->get_current_view()->set_view_offset(offset * SGD_DEGREES_TO_RADIANS);
}

double
FGBFI::getGoalViewOffset ()
{
  return (globals->get_current_view()
	  ->get_goal_view_offset() * SGD_RADIANS_TO_DEGREES);
}

void
FGBFI::setGoalViewOffset (double offset)
{
  globals->get_current_view()
    ->set_goal_view_offset(offset * SGD_DEGREES_TO_RADIANS);
}




/**
 * Return the current Zulu time.
 */
string 
FGBFI::getDateString ()
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
void
FGBFI::setDateString (string date_string)
// FGBFI::setTimeGMT (time_t time)
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
  // double alt = current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER;
  globals->set_warp(warp);
  st->update(lon, lat, warp);
  fgUpdateSkyAndLightingParams();
}


/**
 * Return the GMT as a string.
 */
string 
FGBFI::getGMTString ()
{
  string out;
  char buf[16];
  struct tm * t = globals->get_time_params()->getGmt();
  sprintf(buf, " %.2d:%.2d:%.2d",
	  t->tm_hour, t->tm_min, t->tm_sec);
  out = buf;
  return out;
}



////////////////////////////////////////////////////////////////////////
// Position
////////////////////////////////////////////////////////////////////////


/**
 * Return the current latitude in degrees (negative for south).
 */
double
FGBFI::getLatitude ()
{
  return current_aircraft.fdm_state->get_Latitude() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Set the current latitude in degrees (negative for south).
 */
void
FGBFI::setLatitude (double latitude)
{
  current_aircraft.fdm_state->set_Latitude(latitude * SGD_DEGREES_TO_RADIANS);
}


/**
 * Return the current longitude in degrees (negative for west).
 */
double
FGBFI::getLongitude ()
{
  return current_aircraft.fdm_state->get_Longitude() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Set the current longitude in degrees (negative for west).
 */
void
FGBFI::setLongitude (double longitude)
{
  current_aircraft.fdm_state->set_Longitude(longitude * SGD_DEGREES_TO_RADIANS);
}


/**
 * Return the current altitude in feet.
 */
double
FGBFI::getAltitude ()
{
  return current_aircraft.fdm_state->get_Altitude();
}



/**
 * Return the current altitude in above the terrain.
 */
double
FGBFI::getAGL ()
{
  return current_aircraft.fdm_state->get_Altitude()
	 - (scenery.cur_elev * SG_METER_TO_FEET);
}


/**
 * Set the current altitude in feet.
 */
void
FGBFI::setAltitude (double altitude)
{
  current_aircraft.fdm_state->set_Altitude(altitude);
}



////////////////////////////////////////////////////////////////////////
// Attitude
////////////////////////////////////////////////////////////////////////


/**
 * Return the current heading in degrees.
 */
double
FGBFI::getHeading ()
{
  return current_aircraft.fdm_state->get_Psi() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Return the current heading in degrees.
 */
double
FGBFI::getHeadingMag ()
{
  return current_aircraft.fdm_state->get_Psi() * SGD_RADIANS_TO_DEGREES - getMagVar();
}


/**
 * Set the current heading in degrees.
 */
void
FGBFI::setHeading (double heading)
{
  FGInterface * fdm = current_aircraft.fdm_state;
  fdm->set_Euler_Angles(fdm->get_Phi(), fdm->get_Theta(),
			heading * SGD_DEGREES_TO_RADIANS);
}


/**
 * Return the current pitch in degrees.
 */
double
FGBFI::getPitch ()
{
  return current_aircraft.fdm_state->get_Theta() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Set the current pitch in degrees.
 */
void
FGBFI::setPitch (double pitch)
{
  FGInterface * fdm = current_aircraft.fdm_state;
  fdm->set_Euler_Angles(fdm->get_Phi(), pitch * SGD_DEGREES_TO_RADIANS, fdm->get_Psi());
}


/**
 * Return the current roll in degrees.
 */
double
FGBFI::getRoll ()
{
  return current_aircraft.fdm_state->get_Phi() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Set the current roll in degrees.
 */
void
FGBFI::setRoll (double roll)
{
  FGInterface * fdm = current_aircraft.fdm_state;
  fdm->set_Euler_Angles(roll * SGD_DEGREES_TO_RADIANS, fdm->get_Theta(), fdm->get_Psi());
}


/**
 * Return the current engine0 rpm
 */
double
FGBFI::getRPM ()
{
  if ( current_aircraft.fdm_state->get_engine(0) != NULL ) {
      return current_aircraft.fdm_state->get_engine(0)->get_RPM();
  } else {
      return 0.0;
  }
}


/**
 * Set the current engine0 rpm
 */
void
FGBFI::setRPM (double rpm)
{
    if ( current_aircraft.fdm_state->get_engine(0) != NULL ) {
	if (getRPM() != rpm) {
	    current_aircraft.fdm_state->get_engine(0)->set_RPM( rpm );
	}
    }
}


/**
 * Return the current engine0 EGT.
 */
double
FGBFI::getEGT ()
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
double
FGBFI::getCHT ()
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
double
FGBFI::getMP ()
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
double
FGBFI::getFuelFlow ()
{
  if ( current_aircraft.fdm_state->get_engine(0) != NULL ) {
      return current_aircraft.fdm_state->get_engine(0)->get_Fuel_Flow();
  } else {
      return 0.0;
  }
}

////////////////////////////////////////////////////////////////////////
// Consumables
////////////////////////////////////////////////////////////////////////

/**
 * Return the fuel level in tank 1
 */
double
FGBFI::getTank1Fuel ()
{
  return current_aircraft.fdm_state->get_Tank1Fuel();
}

void
FGBFI::setTank1Fuel ( double gals )
{
  current_aircraft.fdm_state->set_Tank1Fuel( gals );
}

/**
 * Return the fuel level in tank 2
 */
double
FGBFI::getTank2Fuel ()
{
  return current_aircraft.fdm_state->get_Tank2Fuel();
}

void
FGBFI::setTank2Fuel ( double gals )
{
  current_aircraft.fdm_state->set_Tank2Fuel( gals );
}


////////////////////////////////////////////////////////////////////////
// Velocities
////////////////////////////////////////////////////////////////////////


/**
 * Return the current airspeed in knots.
 */
double
FGBFI::getAirspeed ()
{
				// FIXME: should we add speed-up?
  return current_aircraft.fdm_state->get_V_calibrated_kts();
}


/**
 * Set the calibrated airspeed in knots.
 */
void
FGBFI::setAirspeed (double speed)
{
  current_aircraft.fdm_state->set_V_calibrated_kts(speed);
}


/**
 * Return the current sideslip (FIXME: units unknown).
 */
double
FGBFI::getSideSlip ()
{
  return current_aircraft.fdm_state->get_Beta();
}


/**
 * Return the current climb rate in feet/minute
 */
double
FGBFI::getVerticalSpeed ()
{
				// What about meters?
  return current_aircraft.fdm_state->get_Climb_Rate() * 60.0;
}


/**
 * Get the current north velocity (units??).
 */
double
FGBFI::getSpeedNorth ()
{
  return current_aircraft.fdm_state->get_V_north();
}


// /**
//  * Set the current north velocity (units??).
//  */
// void
// FGBFI::setSpeedNorth (double speed)
// {
//   FGInterface * fdm = current_aircraft.fdm_state;
// //   fdm->set_Velocities_Local(speed, fdm->get_V_east(), fdm->get_V_down());
// }


/**
 * Get the current east velocity (units??).
 */
double
FGBFI::getSpeedEast ()
{
  return current_aircraft.fdm_state->get_V_east();
}


// /**
//  * Set the current east velocity (units??).
//  */
// void
// FGBFI::setSpeedEast (double speed)
// {
//   FGInterface * fdm = current_aircraft.fdm_state;
// //   fdm->set_Velocities_Local(fdm->get_V_north(), speed, fdm->get_V_down());
// }


/**
 * Get the current down velocity (units??).
 */
double
FGBFI::getSpeedDown ()
{
  return current_aircraft.fdm_state->get_V_down();
}


// /**
//  * Set the current down velocity (units??).
//  */
// void
// FGBFI::setSpeedDown (double speed)
// {
//   FGInterface * fdm = current_aircraft.fdm_state;
// //   fdm->set_Velocities_Local(fdm->get_V_north(), fdm->get_V_east(), speed);
// }



////////////////////////////////////////////////////////////////////////
// Autopilot
////////////////////////////////////////////////////////////////////////


/**
 * Get the autopilot altitude lock (true=on).
 */
bool
FGBFI::getAPAltitudeLock ()
{
    return current_autopilot->get_AltitudeEnabled();
}


/**
 * Set the autopilot altitude lock (true=on).
 */
void
FGBFI::setAPAltitudeLock (bool lock)
{
  current_autopilot->set_AltitudeMode(FGAutopilot::FG_ALTITUDE_LOCK);
  current_autopilot->set_AltitudeEnabled(lock);
}


/**
 * Get the autopilot altitude lock (true=on).
 */
bool
FGBFI::getAPGSLock ()
{
    return current_autopilot->get_AltitudeEnabled();
}


/**
 * Set the autopilot altitude lock (true=on).
 */
void
FGBFI::setAPGSLock (bool lock)
{
  current_autopilot->set_AltitudeMode(FGAutopilot::FG_ALTITUDE_GS1);
  current_autopilot->set_AltitudeEnabled(lock);
}


/**
 * Get the autopilot target altitude in feet.
 */
double
FGBFI::getAPAltitude ()
{
  return current_autopilot->get_TargetAltitude() * SG_METER_TO_FEET;
}


/**
 * Set the autopilot target altitude in feet.
 */
void
FGBFI::setAPAltitude (double altitude)
{
    current_autopilot->set_TargetAltitude( altitude * SG_FEET_TO_METER );
}


/**
 * Get the autopilot target altitude in feet.
 */
double
FGBFI::getAPClimb ()
{
  return current_autopilot->get_TargetClimbRate() * SG_METER_TO_FEET;
}


/**
 * Set the autopilot target altitude in feet.
 */
void
FGBFI::setAPClimb (double rate)
{
    current_autopilot->set_TargetClimbRate( rate * SG_FEET_TO_METER );
}


/**
 * Get the autopilot heading lock (true=on).
 */
bool
FGBFI::getAPHeadingLock ()
{
    return
      (current_autopilot->get_HeadingEnabled() &&
       current_autopilot->get_HeadingMode() == DEFAULT_AP_HEADING_LOCK);
}


/**
 * Set the autopilot heading lock (true=on).
 */
void
FGBFI::setAPHeadingLock (bool lock)
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
double
FGBFI::getAPHeadingBug ()
{
  return current_autopilot->get_DGTargetHeading();
}


/**
 * Set the autopilot heading bug in degrees.
 */
void
FGBFI::setAPHeadingBug (double heading)
{
  current_autopilot->set_DGTargetHeading( heading );
}


/**
 * Get the autopilot wing leveler lock (true=on).
 */
bool
FGBFI::getAPWingLeveler ()
{
    return
      (current_autopilot->get_HeadingEnabled() &&
       current_autopilot->get_HeadingMode() == FGAutopilot::FG_TC_HEADING_LOCK);
}


/**
 * Set the autopilot wing leveler lock (true=on).
 */
void
FGBFI::setAPWingLeveler (bool lock)
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
bool
FGBFI::getAPNAV1Lock ()
{
  return
    (current_autopilot->get_HeadingEnabled() &&
     current_autopilot->get_HeadingMode() == FGAutopilot::FG_HEADING_NAV1);
}


/**
 * Set the autopilot NAV1 lock.
 */
void
FGBFI::setAPNAV1Lock (bool lock)
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
bool
FGBFI::getAPAutoThrottleLock ()
{
  return current_autopilot->get_AutoThrottleEnabled();
}


/**
 * Set the autothrottle lock.
 */
void
FGBFI::setAPAutoThrottleLock (bool lock)
{
  current_autopilot->set_AutoThrottleEnabled(lock);
}


// kludge
double
FGBFI::getAPRudderControl ()
{
    if (getAPHeadingLock())
        return current_autopilot->get_TargetHeading();
    else
        return controls.get_rudder();
}

// kludge
void
FGBFI::setAPRudderControl (double value)
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
double
FGBFI::getAPElevatorControl ()
{
  if (getAPAltitudeLock())
      return current_autopilot->get_TargetAltitude();
  else
    return controls.get_elevator();
}

// kludge
void
FGBFI::setAPElevatorControl (double value)
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
double
FGBFI::getAPThrottleControl ()
{
  if (getAPAutoThrottleLock())
    return 0.0;			// always resets
  else
    return controls.get_throttle(0);
}

// kludge
void
FGBFI::setAPThrottleControl (double value)
{
  if (getAPAutoThrottleLock())
    current_autopilot->AutoThrottleAdjust(value < 0.0 ? -0.01 : 0.01);
  else
    controls.set_throttle(0, value);
}



////////////////////////////////////////////////////////////////////////
// GPS
////////////////////////////////////////////////////////////////////////


/**
 * Get the autopilot GPS lock (true=on).
 */
bool
FGBFI::getGPSLock ()
{
  return (current_autopilot->get_HeadingEnabled() &&
	  (current_autopilot->get_HeadingMode() ==
	   FGAutopilot::FG_HEADING_WAYPOINT ));
}


/**
 * Set the autopilot GPS lock (true=on).
 */
void
FGBFI::setGPSLock (bool lock)
{
  if (lock) {
    current_autopilot->set_HeadingMode(FGAutopilot::FG_HEADING_WAYPOINT);
    current_autopilot->set_HeadingEnabled(true);
  } else if (current_autopilot->get_HeadingMode() ==
	     FGAutopilot::FG_HEADING_WAYPOINT) {
    current_autopilot->set_HeadingEnabled(false);
  }
}


/**
 * Get the GPS target latitude in degrees (negative for south).
 */
double
FGBFI::getGPSTargetLatitude ()
{
    return current_autopilot->get_TargetLatitude();
}


/**
 * Get the GPS target longitude in degrees (negative for west).
 */
double
FGBFI::getGPSTargetLongitude ()
{
  return current_autopilot->get_TargetLongitude();
}



////////////////////////////////////////////////////////////////////////
// Weather
////////////////////////////////////////////////////////////////////////


/**
 * Get the current visibility (meters).
 */
double
FGBFI::getVisibility ()
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
void
FGBFI::setVisibility (double visibility)
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
double
FGBFI::getWindNorth ()
{
  return current_aircraft.fdm_state->get_V_north_airmass();
}


/**
 * Set the current wind north velocity (feet/second).
 */
void
FGBFI::setWindNorth (double speed)
{
  current_aircraft.fdm_state->set_Velocities_Local_Airmass(speed,
							   getWindEast(),
							   getWindDown());
}


/**
 * Get the current wind east velocity (feet/second).
 */
double
FGBFI::getWindEast ()
{
  return current_aircraft.fdm_state->get_V_east_airmass();
}


/**
 * Set the current wind east velocity (feet/second).
 */
void
FGBFI::setWindEast (double speed)
{
  cout << "Set wind-east to " << speed << endl;
  current_aircraft.fdm_state->set_Velocities_Local_Airmass(getWindNorth(),
							   speed,
							   getWindDown());
}


/**
 * Get the current wind down velocity (feet/second).
 */
double
FGBFI::getWindDown ()
{
  return current_aircraft.fdm_state->get_V_down_airmass();
}


/**
 * Set the current wind down velocity (feet/second).
 */
void
FGBFI::setWindDown (double speed)
{
  current_aircraft.fdm_state->set_Velocities_Local_Airmass(getWindNorth(),
							   getWindEast(),
							   speed);
}



////////////////////////////////////////////////////////////////////////
// View.
////////////////////////////////////////////////////////////////////////

double
FGBFI::getFOV ()
{
  return globals->get_current_view()->get_fov();
}

void
FGBFI::setFOV (double fov)
{
  globals->get_current_view()->set_fov( fov );
}

void
FGBFI::setViewAxisLong (double axis)
{
  axisLong = axis;
}

void
FGBFI::setViewAxisLat (double axis)
{
  axisLat = axis;
}


////////////////////////////////////////////////////////////////////////
// Time
////////////////////////////////////////////////////////////////////////

/**
 * Return the magnetic variation
 */
double
FGBFI::getMagVar ()
{
  return globals->get_mag()->get_magvar() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Return the magnetic variation
 */
double
FGBFI::getMagDip ()
{
  return globals->get_mag()->get_magdip() * SGD_RADIANS_TO_DEGREES;
}


// end of bfi.cxx

