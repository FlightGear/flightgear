// bfi.cxx - Big Friendly Interface implementation
//
// Written by David Megginson, started February, 2000.
//
// Copyright (C) 2000  David Megginson - david@megginson.com
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

#if defined( FG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#else
#  include <iostream>
#endif

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
#include "save.hxx"
#include "fg_init.hxx"
#include <simgear/misc/props.hxx>

FG_USING_NAMESPACE(std);


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

  cout << "BFI: start reinit\n";

				// TODO: add more AP stuff
  double elevator = FGBFI::getElevator();
  double aileron = FGBFI::getAileron();
  double rudder = FGBFI::getRudder();
  double throttle = FGBFI::getThrottle();
  double elevator_trim = FGBFI::getElevatorTrim();
  double flaps = FGBFI::getFlaps();
  double brake = FGBFI::getBrakes();
  bool apHeadingLock = FGBFI::getAPHeadingLock();
  double apHeadingMag = FGBFI::getAPHeadingMag();
  bool apAltitudeLock = FGBFI::getAPAltitudeLock();
  double apAltitude = FGBFI::getAPAltitude();
  const string &targetAirport = FGBFI::getTargetAirport();
  bool gpsLock = FGBFI::getGPSLock();
  // double gpsLatitude = FGBFI::getGPSTargetLatitude();
  // double gpsLongitude = FGBFI::getGPSTargetLongitude();

  FGBFI::setTargetAirport("");
  cout << "Target airport is " << globals->get_options()->get_airport_id() << endl;

  fgReInitSubsystems();

				// FIXME: this is wrong.
				// All of these are scheduled events,
				// and it should be possible to force
				// them all to run once.
  fgUpdateSunPos();
  fgUpdateMoonPos();
  cur_light_params.Update();
  fgUpdateLocalTime();
  fgUpdateWeatherDatabase();
  fgRadioSearch();

				// Restore all of the old states.
  FGBFI::setElevator(elevator);
  FGBFI::setAileron(aileron);
  FGBFI::setRudder(rudder);
  FGBFI::setThrottle(throttle);
  FGBFI::setElevatorTrim(elevator_trim);
  FGBFI::setFlaps(flaps);
  FGBFI::setBrakes(brake);
  FGBFI::setAPHeadingLock(apHeadingLock);
  FGBFI::setAPHeadingMag(apHeadingMag);
  FGBFI::setAPAltitudeLock(apAltitudeLock);
  FGBFI::setAPAltitude(apAltitude);
  FGBFI::setTargetAirport(targetAirport);
  FGBFI::setGPSLock(gpsLock);

  _needReinit = false;

  cout << "BFI: end reinit\n";
}



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
  FG_LOG(FG_GENERAL, FG_INFO, "Starting BFI init");
				// Simulation
  current_properties.tieInt("/sim/flight-model",
			    getFlightModel, setFlightModel);
  current_properties.tieString("/sim/aircraft",
			       getAircraft, setAircraft);
  current_properties.tieString("/sim/aircraft-dir",
			       getAircraftDir, setAircraftDir);
  // TODO: timeGMT
  current_properties.tieString("/sim/time/gmt",
			       getDateString, setDateString);
  current_properties.tieString("/sim/time/gmt-string",
			       getGMTString, 0);
  current_properties.tieBool("/sim/hud/visibility",
			     getHUDVisible, setHUDVisible);
  current_properties.tieBool("/sim/panel/visibility",
			     getPanelVisible, setPanelVisible);
  current_properties.tieInt("/sim/panel/x-offset",
			    getPanelXOffset, setPanelXOffset);
  current_properties.tieInt("/sim/panel/y-offset",
			    getPanelYOffset, setPanelYOffset);

				// Position
  current_properties.tieString("/position/airport-id",
				getTargetAirport, setTargetAirport);
  current_properties.tieDouble("/position/latitude",
				getLatitude, setLatitude);
  current_properties.tieDouble("/position/longitude",
			       getLongitude, setLongitude);
  current_properties.tieDouble("/position/altitude",
			       getAltitude, setAltitude);
// 			       getAltitude, setAltitude, false);
  current_properties.tieDouble("/position/altitude-agl",
			       getAGL, 0);

				// Orientation
  current_properties.tieDouble("/orientation/heading",
			       getHeading, setHeading);
  current_properties.tieDouble("/orientation/heading-magnetic",
			       getHeadingMag, 0);
  current_properties.tieDouble("/orientation/pitch",
			       getPitch, setPitch);
  current_properties.tieDouble("/orientation/roll",
			       getRoll, setRoll);

				// Engine
  current_properties.tieDouble("/engines/engine0/rpm",
			       getRPM, 0);
  current_properties.tieDouble("/engines/engine0/egt",
			       getEGT, 0);
  current_properties.tieDouble("/engines/engine0/cht",
			       getCHT, 0);
  current_properties.tieDouble("/engines/engine0/mp",
			       getMP, 0);

				// Velocities
  current_properties.tieDouble("/velocities/airspeed",
			       getAirspeed, setAirspeed);
  current_properties.tieDouble("/velocities/side-slip",
			       getSideSlip, 0);
  current_properties.tieDouble("/velocities/vertical-speed",
			       getVerticalSpeed, 0);
  current_properties.tieDouble("/velocities/speed-north",
			       getSpeedNorth, 0);
  current_properties.tieDouble("/velocities/speed-east",
			       getSpeedEast, 0);
  current_properties.tieDouble("/velocities/speed-down",
			       getSpeedDown, 0);

				// Controls
  current_properties.tieDouble("/controls/throttle",
			       getThrottle, setThrottle);
  current_properties.tieDouble("/controls/mixture",
			       getMixture, setMixture);
  current_properties.tieDouble("/controls/propellor-pitch",
			       getPropAdvance, setPropAdvance);
  current_properties.tieDouble("/controls/flaps",
			       getFlaps, setFlaps);
  current_properties.tieDouble("/controls/aileron",
			       getAileron, setAileron);
  current_properties.tieDouble("/controls/rudder",
			       getRudder, setRudder);
  current_properties.tieDouble("/controls/elevator",
			       getElevator, setElevator);
  current_properties.tieDouble("/controls/elevator-trim",
			       getElevatorTrim, setElevatorTrim);
  current_properties.tieDouble("/controls/brakes/all",
			       getBrakes, setBrakes);
  current_properties.tieDouble("/controls/brakes/left",
			       getLeftBrake, setLeftBrake);
  current_properties.tieDouble("/controls/brakes/right",
			       getRightBrake, setRightBrake);
  current_properties.tieDouble("/controls/brakes/center",
			       getRightBrake, setCenterBrake);

				// Autopilot
  current_properties.tieBool("/autopilot/locks/altitude",
			     getAPAltitudeLock, setAPAltitudeLock);
  current_properties.tieDouble("/autopilot/settings/altitude",
			       getAPAltitude, setAPAltitude);
  current_properties.tieBool("/autopilot/locks/heading",
			     getAPHeadingLock, setAPHeadingLock);
  current_properties.tieDouble("/autopilot/settings/heading",
			       getAPHeading, setAPHeading);
  current_properties.tieDouble("/autopilot/settings/heading-magnetic",
			       getAPHeadingMag, setAPHeadingMag);
  current_properties.tieBool("/autopilot/locks/nav1",
			     getAPNAV1Lock, setAPNAV1Lock);

				// Radio navigation
  current_properties.tieDouble("/radios/nav1/frequencies/selected",
			       getNAV1Freq, setNAV1Freq);
  current_properties.tieDouble("/radios/nav1/frequencies/standby",
			       getNAV1AltFreq, setNAV1AltFreq);
  current_properties.tieDouble("/radios/nav1/radials/actual",
			       getNAV1Radial, 0);
  current_properties.tieDouble("/radios/nav1/radials/selected",
			       getNAV1SelRadial, setNAV1SelRadial);
  current_properties.tieDouble("/radios/nav1/dme/distance",
			       getNAV1DistDME, 0);
  current_properties.tieBool("/radios/nav1/to-flag",
			     getNAV1TO, 0);
  current_properties.tieBool("/radios/nav1/from-flag",
			     getNAV1FROM, 0);
  current_properties.tieBool("/radios/nav1/in-range",
			     getNAV1InRange, 0);
  current_properties.tieBool("/radios/nav1/dme/in-range",
			     getNAV1DMEInRange, 0);
			       
  current_properties.tieDouble("/radios/nav2/frequencies/selected",
			       getNAV2Freq, setNAV2Freq);
  current_properties.tieDouble("/radios/nav2/frequencies/standby",
			       getNAV2AltFreq, setNAV2AltFreq);
  current_properties.tieDouble("/radios/nav2/radials/actual",
			       getNAV2Radial, 0);
  current_properties.tieDouble("/radios/nav2/radials/selected",
			       getNAV2SelRadial, setNAV2SelRadial);
  current_properties.tieDouble("/radios/nav2/dme/distance",
			       getNAV2DistDME, 0);
  current_properties.tieBool("/radios/nav2/to-flag",
			     getNAV2TO, 0);
  current_properties.tieBool("/radios/nav2/from-flag",
			     getNAV2FROM, 0);
  current_properties.tieBool("/radios/nav2/in-range",
			     getNAV2InRange, 0);
  current_properties.tieBool("/radios/nav2/dme/in-range",
			     getNAV2DMEInRange, 0);

  current_properties.tieDouble("/radios/adf/frequencies/selected",
			       getADFFreq, setADFFreq);
  current_properties.tieDouble("/radios/adf/frequencies/standby",
			       getADFAltFreq, setADFAltFreq);
  current_properties.tieDouble("/radios/adf/rotation",
			       getADFRotation, setADFRotation);

  current_properties.tieDouble("/environment/visibility",
			       getVisibility, setVisibility);

  _needReinit = false;

  FG_LOG(FG_GENERAL, FG_INFO, "Ending BFI init");
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
  if (_needReinit) {
    reinit();
  }
}



////////////////////////////////////////////////////////////////////////
// Simulation.
////////////////////////////////////////////////////////////////////////


/**
 * Return the flight model as an integer.
 *
 * TODO: use a string instead.
 */
int
FGBFI::getFlightModel ()
{
  return globals->get_options()->get_flight_model();
}


/**
 * Return the current aircraft as a string.
 */
const string &
FGBFI::getAircraft ()
{
  _temp = globals->get_options()->get_aircraft();
  return _temp;
}


/**
 * Return the current aircraft directory (UIUC) as a string.
 */
const string &
FGBFI::getAircraftDir ()
{
  _temp = aircraft_dir;
  return _temp;
}


/**
 * Set the flight model as an integer.
 *
 * TODO: use a string instead.
 */
void
FGBFI::setFlightModel (int model)
{
  if (getFlightModel() != model) {
    globals->get_options()->set_flight_model(model);
    needReinit();
  }
}


/**
 * Set the current aircraft.
 */
void
FGBFI::setAircraft (const string &aircraft)
{
  if (getAircraft() != aircraft) {
    globals->get_options()->set_aircraft(aircraft);
    needReinit();
  }
}


/**
 * Set the current aircraft directory (UIUC).
 */
void
FGBFI::setAircraftDir (const string &dir)
{
  if (getAircraftDir() != dir) {
    aircraft_dir = dir;
    needReinit();
  }
}


/**
 * Return the current Zulu time.
 */
const string &
FGBFI::getDateString ()
{
  static string out;		// FIXME: not thread-safe
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
FGBFI::setDateString (const string &date_string)
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
    FG_LOG(FG_INPUT, FG_ALERT, "Date/time string " << date_string
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
  double alt = current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER;
  globals->set_warp(warp);
  st->update(lon, lat, warp);
  fgUpdateSkyAndLightingParams();
}


/**
 * Return the GMT as a string.
 */
const string &
FGBFI::getGMTString ()
{
  static string out;		// FIXME: not thread-safe
  char buf[16];
  struct tm * t = globals->get_time_params()->getGmt();
  sprintf(buf, " %.2d:%.2d:%.2d",
	  t->tm_hour, t->tm_min, t->tm_sec);
  out = buf;
  return out;
}


/**
 * Return true if the HUD is visible.
 */
bool
FGBFI::getHUDVisible ()
{
  return globals->get_options()->get_hud_status();
}


/**
 * Ensure that the HUD is visible or hidden.
 */
void
FGBFI::setHUDVisible (bool visible)
{
  globals->get_options()->set_hud_status(visible);
}


/**
 * Return true if the 2D panel is visible.
 */
bool
FGBFI::getPanelVisible ()
{
  return globals->get_options()->get_panel_status();
}


/**
 * Ensure that the 2D panel is visible or hidden.
 */
void
FGBFI::setPanelVisible (bool visible)
{
  if (globals->get_options()->get_panel_status() != visible) {
    globals->get_options()->toggle_panel();
  }
}


/**
 * Get the panel's current x-shift.
 */
int
FGBFI::getPanelXOffset ()
{
  if (current_panel != 0)
    return current_panel->getXOffset();
  else
    return 0;
}


/**
 * Set the panel's current x-shift.
 */
void
FGBFI::setPanelXOffset (int offset)
{
  if (current_panel != 0)
    current_panel->setXOffset(offset);
}


/**
 * Get the panel's current y-shift.
 */
int
FGBFI::getPanelYOffset ()
{
  if (current_panel != 0)
    return current_panel->getYOffset();
  else
    return 0;
}


/**
 * Set the panel's current y-shift.
 */
void
FGBFI::setPanelYOffset (int offset)
{
  if (current_panel != 0)
    current_panel->setYOffset(offset);
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
  return current_aircraft.fdm_state->get_Latitude() * RAD_TO_DEG;
}


/**
 * Set the current latitude in degrees (negative for south).
 */
void
FGBFI::setLatitude (double latitude)
{
  current_aircraft.fdm_state->set_Latitude(latitude * DEG_TO_RAD);
  fgUpdateSkyAndLightingParams();
}


/**
 * Return the current longitude in degrees (negative for west).
 */
double
FGBFI::getLongitude ()
{
  return current_aircraft.fdm_state->get_Longitude() * RAD_TO_DEG;
}


/**
 * Set the current longitude in degrees (negative for west).
 */
void
FGBFI::setLongitude (double longitude)
{
  current_aircraft.fdm_state->set_Longitude(longitude * DEG_TO_RAD);
  fgUpdateSkyAndLightingParams();
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
	 - (scenery.cur_elev * METER_TO_FEET);
}


/**
 * Set the current altitude in feet.
 */
void
FGBFI::setAltitude (double altitude)
{
  current_aircraft.fdm_state->set_Altitude(altitude);
  fgUpdateSkyAndLightingParams();
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
  return current_aircraft.fdm_state->get_Psi() * RAD_TO_DEG;
}


/**
 * Return the current heading in degrees.
 */
double
FGBFI::getHeadingMag ()
{
  return current_aircraft.fdm_state->get_Psi() * RAD_TO_DEG - getMagVar();
}


/**
 * Set the current heading in degrees.
 */
void
FGBFI::setHeading (double heading)
{
  FGInterface * fdm = current_aircraft.fdm_state;
  fdm->set_Euler_Angles(fdm->get_Phi(), fdm->get_Theta(),
			heading * DEG_TO_RAD);
}


/**
 * Return the current pitch in degrees.
 */
double
FGBFI::getPitch ()
{
  return current_aircraft.fdm_state->get_Theta() * RAD_TO_DEG;
}


/**
 * Set the current pitch in degrees.
 */
void
FGBFI::setPitch (double pitch)
{
  FGInterface * fdm = current_aircraft.fdm_state;
  fdm->set_Euler_Angles(fdm->get_Phi(), pitch * DEG_TO_RAD, fdm->get_Psi());
}


/**
 * Return the current roll in degrees.
 */
double
FGBFI::getRoll ()
{
  return current_aircraft.fdm_state->get_Phi() * RAD_TO_DEG;
}


/**
 * Set the current roll in degrees.
 */
void
FGBFI::setRoll (double roll)
{
  FGInterface * fdm = current_aircraft.fdm_state;
  fdm->set_Euler_Angles(roll * DEG_TO_RAD, fdm->get_Theta(), fdm->get_Psi());
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
 * Return the current engine0 CHT.
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
// Controls
////////////////////////////////////////////////////////////////////////


/**
 * Get the throttle setting, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getThrottle ()
{
				// FIXME: add engine selector
  return controls.get_throttle(0);
}


/**
 * Set the throttle, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setThrottle (double throttle)
{
				// FIXME: allow engine selection
  controls.set_throttle(0, throttle);
}


/**
 * Get the fuel mixture setting, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getMixture ()
{
				// FIXME: add engine selector
  return controls.get_mixture(0);
}


/**
 * Set the fuel mixture, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setMixture (double mixture)
{
				// FIXME: allow engine selection
  controls.set_mixture(0, mixture);
}


/**
 * Get the propellor pitch setting, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getPropAdvance ()
{
				// FIXME: add engine selector
  return controls.get_prop_advance(0);
}


/**
 * Set the propellor pitch, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setPropAdvance (double pitch)
{
				// FIXME: allow engine selection
  controls.set_prop_advance(0, pitch);
}


/**
 * Get the flaps setting, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getFlaps ()
{
  return controls.get_flaps();
}


/**
 * Set the flaps, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setFlaps (double flaps)
{
				// FIXME: clamp?
  controls.set_flaps(flaps);
}


/**
 * Get the aileron, from -1.0 (left) to 1.0 (right).
 */
double
FGBFI::getAileron ()
{
  return controls.get_aileron();
}


/**
 * Set the aileron, from -1.0 (left) to 1.0 (right).
 */
void
FGBFI::setAileron (double aileron)
{
				// FIXME: clamp?
  controls.set_aileron(aileron);
}


/**
 * Get the rudder setting, from -1.0 (left) to 1.0 (right).
 */
double
FGBFI::getRudder ()
{
  return controls.get_rudder();
}


/**
 * Set the rudder, from -1.0 (left) to 1.0 (right).
 */
void
FGBFI::setRudder (double rudder)
{
				// FIXME: clamp?
  controls.set_rudder(rudder);
}


/**
 * Get the elevator setting, from -1.0 (down) to 1.0 (up).
 */
double
FGBFI::getElevator ()
{
  return controls.get_elevator();
}


/**
 * Set the elevator, from -1.0 (down) to 1.0 (up).
 */
void
FGBFI::setElevator (double elevator)
{
				// FIXME: clamp?
  controls.set_elevator(elevator);
}


/**
 * Get the elevator trim, from -1.0 (down) to 1.0 (up).
 */
double
FGBFI::getElevatorTrim ()
{
  return controls.get_elevator_trim();
}


/**
 * Set the elevator trim, from -1.0 (down) to 1.0 (up).
 */
void
FGBFI::setElevatorTrim (double trim)
{
				// FIXME: clamp?
  controls.set_elevator_trim(trim);
}


/**
 * Get the highest brake setting, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getBrakes ()
{
  double b1 = getCenterBrake();
  double b2 = getLeftBrake();
  double b3 = getRightBrake();
  return (b1 > b2 ? (b1 > b3 ? b1 : b3) : (b2 > b3 ? b2 : b3));
}


/**
 * Set all brakes, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setBrakes (double brake)
{
  setCenterBrake(brake);
  setLeftBrake(brake);
  setRightBrake(brake);
}


/**
 * Get the center brake, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getCenterBrake ()
{
  return controls.get_brake(2);
}


/**
 * Set the center brake, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setCenterBrake (double brake)
{
  controls.set_brake(2, brake);
}


/**
 * Get the left brake, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getLeftBrake ()
{
  return controls.get_brake(0);
}


/**
 * Set the left brake, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setLeftBrake (double brake)
{
  controls.set_brake(0, brake);
}


/**
 * Get the right brake, from 0.0 (none) to 1.0 (full).
 */
double
FGBFI::getRightBrake ()
{
  return controls.get_brake(1);
}


/**
 * Set the right brake, from 0.0 (none) to 1.0 (full).
 */
void
FGBFI::setRightBrake (double brake)
{
  controls.set_brake(1, brake);
}





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
 * Get the autopilot target altitude in feet.
 */
double
FGBFI::getAPAltitude ()
{
  return current_autopilot->get_TargetAltitude() * METER_TO_FEET;
}


/**
 * Set the autopilot target altitude in feet.
 */
void
FGBFI::setAPAltitude (double altitude)
{
    current_autopilot->set_TargetAltitude( altitude );
}


/**
 * Get the autopilot heading lock (true=on).
 */
bool
FGBFI::getAPHeadingLock ()
{
    return
      (current_autopilot->get_HeadingEnabled() &&
       current_autopilot->get_HeadingMode() == FGAutopilot::FG_HEADING_LOCK);
}


/**
 * Set the autopilot heading lock (true=on).
 */
void
FGBFI::setAPHeadingLock (bool lock)
{
  if (lock) {
				// We need to do this so that
				// it's possible to lock onto a
				// heading other than the current
				// heading.
    double heading = getAPHeadingMag();
    current_autopilot->set_HeadingMode(FGAutopilot::FG_HEADING_LOCK);
    current_autopilot->set_HeadingEnabled(true);
    setAPHeadingMag(heading);
  } else if (current_autopilot->get_HeadingMode() ==
	     FGAutopilot::FG_HEADING_LOCK) {
    current_autopilot->set_HeadingEnabled(false);
  }
}


/**
 * Get the autopilot target heading in degrees.
 */
double
FGBFI::getAPHeading ()
{
  return current_autopilot->get_TargetHeading();
}


/**
 * Set the autopilot target heading in degrees.
 */
void
FGBFI::setAPHeading (double heading)
{
  current_autopilot->set_TargetHeading( heading );
}


/**
 * Get the autopilot target heading in degrees.
 */
double
FGBFI::getAPHeadingMag ()
{
  return current_autopilot->get_TargetHeading() - getMagVar();
}


/**
 * Set the autopilot target heading in degrees.
 */
void
FGBFI::setAPHeadingMag (double heading)
{
  current_autopilot->set_TargetHeading( heading + getMagVar() );
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



////////////////////////////////////////////////////////////////////////
// Radio navigation.
////////////////////////////////////////////////////////////////////////

double
FGBFI::getNAV1Freq ()
{
  return current_radiostack->get_nav1_freq();
}

double
FGBFI::getNAV1AltFreq ()
{
  return current_radiostack->get_nav1_alt_freq();
}

double
FGBFI::getNAV1Radial ()
{
  return current_radiostack->get_nav1_radial();
}

double
FGBFI::getNAV1SelRadial ()
{
  return current_radiostack->get_nav1_sel_radial();
}

double
FGBFI::getNAV1DistDME ()
{
  return current_radiostack->get_nav1_dme_dist();
}

bool 
FGBFI::getNAV1TO ()
{
  if (current_radiostack->get_nav1_inrange()) {
    double heading = current_radiostack->get_nav1_heading();
    double radial = current_radiostack->get_nav1_radial();
    double var = FGBFI::getMagVar();
    if (current_radiostack->get_nav1_loc()) {
      double offset = fabs(heading - radial);
      return (offset<= 8.0 || offset >= 352.0);
    } else {
      double offset =
	fabs(heading - var - radial);
      return (offset <= 20.0 || offset >= 340.0);
    }
  } else {
    return false;
  }
}

bool
FGBFI::getNAV1FROM ()
{
  if (current_radiostack->get_nav1_inrange()) {
    double heading = current_radiostack->get_nav1_heading();
    double radial = current_radiostack->get_nav1_radial();
    double var = FGBFI::getMagVar();
    if (current_radiostack->get_nav1_loc()) {
      double offset = fabs(heading - radial);
      return (offset >= 172.0 && offset<= 188.0);
    } else {
      double offset =
	fabs(heading - var - radial);
      return (offset >= 160.0 && offset <= 200.0);
    }
  } else {
    return false;
  }
}

bool
FGBFI::getNAV1InRange ()
{
  return current_radiostack->get_nav1_inrange();
}

bool
FGBFI::getNAV1DMEInRange ()
{
  return (current_radiostack->get_nav1_inrange() &&
	  current_radiostack->get_nav1_has_dme());
}

double
FGBFI::getNAV2Freq ()
{
  return current_radiostack->get_nav2_freq();
}

double
FGBFI::getNAV2AltFreq ()
{
  return current_radiostack->get_nav2_alt_freq();
}

double
FGBFI::getNAV2Radial ()
{
  return current_radiostack->get_nav2_radial();
}

double
FGBFI::getNAV2SelRadial ()
{
  return current_radiostack->get_nav2_sel_radial();
}

double
FGBFI::getNAV2DistDME ()
{
  return current_radiostack->get_nav2_dme_dist();
}

bool 
FGBFI::getNAV2TO ()
{
  if (current_radiostack->get_nav2_inrange()) {
    double heading = current_radiostack->get_nav2_heading();
    double radial = current_radiostack->get_nav2_radial();
    double var = FGBFI::getMagVar();
    if (current_radiostack->get_nav2_loc()) {
      double offset = fabs(heading - radial);
      return (offset<= 8.0 || offset >= 352.0);
    } else {
      double offset =
	fabs(heading - var - radial);
      return (offset <= 20.0 || offset >= 340.0);
    }
  } else {
    return false;
  }
}

bool 
FGBFI::getNAV2FROM ()
{
  if (current_radiostack->get_nav2_inrange()) {
    double heading = current_radiostack->get_nav2_heading();
    double radial = current_radiostack->get_nav2_radial();
    double var = FGBFI::getMagVar();
    if (current_radiostack->get_nav2_loc()) {
      double offset = fabs(heading - radial);
      return (offset >= 172.0 && offset<= 188.0);
    } else {
      double offset =
	fabs(heading - var - radial);
      return (offset >= 160.0 && offset <= 200.0);
    }
  } else {
    return false;
  }
}


bool
FGBFI::getNAV2InRange ()
{
  return current_radiostack->get_nav2_inrange();
}

bool
FGBFI::getNAV2DMEInRange ()
{
  return (current_radiostack->get_nav2_inrange() &&
	  current_radiostack->get_nav2_has_dme());
}

double
FGBFI::getADFFreq ()
{
  return current_radiostack->get_adf_freq();
}

double
FGBFI::getADFAltFreq ()
{
  return current_radiostack->get_adf_alt_freq();
}

double
FGBFI::getADFRotation ()
{
  return current_radiostack->get_adf_rotation();
}

void
FGBFI::setNAV1Freq (double freq)
{
  current_radiostack->set_nav1_freq(freq);
}

void
FGBFI::setNAV1AltFreq (double freq)
{
  current_radiostack->set_nav1_alt_freq(freq);
}

void
FGBFI::setNAV1SelRadial (double radial)
{
  current_radiostack->set_nav1_sel_radial(radial);
}

void
FGBFI::setNAV2Freq (double freq)
{
  current_radiostack->set_nav2_freq(freq);
}

void
FGBFI::setNAV2AltFreq (double freq)
{
  current_radiostack->set_nav2_alt_freq(freq);
}

void
FGBFI::setNAV2SelRadial (double radial)
{
  current_radiostack->set_nav2_sel_radial(radial);
}

void
FGBFI::setADFFreq (double freq)
{
  current_radiostack->set_adf_freq(freq);
}

void
FGBFI::setADFAltFreq (double freq)
{
  current_radiostack->set_adf_alt_freq(freq);
}

void
FGBFI::setADFRotation (double rot)
{
  current_radiostack->set_adf_rotation(rot);
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
 * Get the GPS target airport code.
 */
const string &
FGBFI::getTargetAirport ()
{
  // FIXME: not thread-safe
  static string out;
  out = globals->get_options()->get_airport_id();

  return out;
}


/**
 * Set the GPS target airport code.
 */
void
FGBFI::setTargetAirport (const string &airportId)
{
  // cout << "setting target airport id = " << airportId << endl;
  globals->get_options()->set_airport_id(airportId);
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

#if 0
/**
 * Set the GPS target longitude in degrees (negative for west).
 */
void
FGBFI::setGPSTargetLongitude (double longitude)
{
  current_autopilot->set_TargetLongitude( longitude );
}
#endif



////////////////////////////////////////////////////////////////////////
// Weather
////////////////////////////////////////////////////////////////////////


/**
 * Get the current visible (units??).
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
 * Set the current visibility (units??).
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



////////////////////////////////////////////////////////////////////////
// Time
////////////////////////////////////////////////////////////////////////

/**
 * Return the magnetic variation
 */
double
FGBFI::getMagVar ()
{
  return globals->get_mag()->get_magvar() * RAD_TO_DEG;
}


/**
 * Return the magnetic variation
 */
double
FGBFI::getMagDip ()
{
  return globals->get_mag()->get_magdip() * RAD_TO_DEG;
}


// end of bfi.cxx

