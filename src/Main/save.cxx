// save.cxx -- class to save and restore a flight.
//
// Written by Curtis Olson, started November 1999.
//
// Copyright (C) 1999  David Megginson - david@megginson.com
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


/*
TODO:
- use a separate options object so that we can roll back on error
- use proper FGFS logging
- add view direction, and other stuff
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>

#include <simgear/constants.h>
#include <simgear/math/fg_types.hxx>

#include "bfi.hxx"

FG_USING_NAMESPACE(std);

#define SAVE(name, value) { output << name << ": " << value << endl; }

/**
 * Save the current state of the simulator to a stream.
 */
bool
fgSaveFlight (ostream &output)
{
  output << "#!fgfs" << endl;

  //
  // Simulation
  //
  SAVE("flight-model", FGBFI::getFlightModel());
  if (FGBFI::getAircraft().length() > 0)
    SAVE("aircraft", FGBFI::getAircraft());
  if (FGBFI::getAircraftDir().length() > 0)
    SAVE("aircraft-dir", FGBFI::getAircraftDir());
  SAVE("time", FGBFI::getTimeGMT());
  SAVE("hud", FGBFI::getHUDVisible());
  SAVE("panel", FGBFI::getPanelVisible());

  // 
  // Location
  //
  SAVE("latitude", FGBFI::getLatitude());
  SAVE("longitude", FGBFI::getLongitude());

				// KLUDGE: deal with gear wierdness
  if (FGBFI::getAGL() < 6) {
    SAVE("altitude", FGBFI::getAltitude() - 6);
  } else {
    SAVE("altitude", FGBFI::getAltitude());
  }

  //
  // Orientation
  //
  SAVE("heading", FGBFI::getHeading());
  SAVE("pitch", FGBFI::getPitch());
  SAVE("roll", FGBFI::getRoll());

  //
  // Velocities
  //
  SAVE("speed-north", FGBFI::getSpeedNorth());
  SAVE("speed-east", FGBFI::getSpeedEast());
  SAVE("speed-down", FGBFI::getSpeedDown());

  //
  // Primary controls
  //
  SAVE("elevator", FGBFI::getElevator());
  SAVE("aileron", FGBFI::getAileron());
  SAVE("rudder", FGBFI::getRudder());
				// FIXME: save each throttle separately
  SAVE("throttle", FGBFI::getThrottle());

  //
  // Secondary controls
  //
  SAVE("elevator-trim", FGBFI::getElevatorTrim());
  SAVE("flaps", FGBFI::getFlaps());
				// FIXME: save each brake separately
  SAVE("brake", FGBFI::getBrake());

  //
  // Radio navigation
  //
  SAVE("nav1-active-frequency", FGBFI::getNAV1Freq());
  SAVE("nav1-standby-frequency", FGBFI::getNAV1AltFreq());
  SAVE("nav1-selected-radial", FGBFI::getNAV1SelRadial());
  SAVE("nav2-active-frequency", FGBFI::getNAV2Freq());
  SAVE("nav2-standby-frequency", FGBFI::getNAV2AltFreq());
  SAVE("nav2-selected-radial", FGBFI::getNAV2SelRadial());
  SAVE("adf-active-frequency", FGBFI::getADFFreq());
  SAVE("adf-standby-frequency", FGBFI::getADFAltFreq());
  SAVE("adf-rotation", FGBFI::getADFRotation());

  //
  // Autopilot and GPS
  //
  if (FGBFI::getTargetAirport().length() > 0)
    SAVE("target-airport", FGBFI::getTargetAirport());
  SAVE("autopilot-altitude-lock", FGBFI::getAPAltitudeLock());
  SAVE("autopilot-altitude", FGBFI::getAPAltitude());
  SAVE("autopilot-heading-lock", FGBFI::getAPHeadingLock());
  SAVE("autopilot-heading", FGBFI::getAPHeadingMag());
  SAVE("autopilot-gps-lock", FGBFI::getGPSLock());
  SAVE("autopilot-gps-lat", FGBFI::getGPSTargetLatitude());
  SAVE("autopilot-gps-lon", FGBFI::getGPSTargetLongitude());

  //
  // Environment.
  //
  SAVE("visibility", FGBFI::getVisibility());
  SAVE("clouds", FGBFI::getClouds());
  SAVE("clouds-asl", FGBFI::getCloudsASL());

  return true;
}


/**
 * Restore the current state of the simulator from a stream.
 */
bool
fgLoadFlight (istream &input)
{
//   FGInterface * f = current_aircraft.fdm_state;
  string text;
  double n;
  long int i;

  if (!input.good() || input.eof()) {
    cout << "Stream is no good!\n";
    return false;
  }

  input >> text;
  if (text != "#!fgfs") {
    cerr << "Bad save file format!\n";
    return false;
  }


  while (input.good() && !input.eof()) {
    input >> text;

    //
    // Simulation.
    //

    if (text == "flight-model:") {
      input >> i;
      cout << "flight model is " << i << endl;
      FGBFI::setFlightModel(i);
    }

    else if (text == "aircraft:") {
      input >> text;
      cout << "aircraft is " << text << endl;
      FGBFI::setAircraft(text);
    }

    else if (text == "aircraft-dir:") {
      input >> text;
      cout << "aircraft-dir is " << text << endl;
      FGBFI::setAircraftDir(text);
    }

    else if (text == "time:") {
      input >> i;
      cout << "saved time is " << i << endl;
      FGBFI::setTimeGMT(i);
    }

    else if (text == "hud:") {
      input >> i;
      cout << "hud status is " << i << endl;
      FGBFI::setHUDVisible(i);
    }

    else if (text == "panel:") {
      input >> i;
      cout << "panel status is " << i << endl;
      FGBFI::setPanelVisible(i);
    }

    //
    // Location
    //
      
    else if (text == "latitude:") {
      input >> n;
      cout << "latitude is " << n << endl;
      FGBFI::setLatitude(n);
    } 

    else if (text == "longitude:") {
      input >> n;
      cout << "longitude is " << n << endl;
      FGBFI::setLongitude(n);
    } 

    else if (text == "altitude:") {
      input >> n;
      cout << "altitude is " << n << endl;
      FGBFI::setAltitude(n);
    } 

    //
    // Orientation
    //

    else if (text == "heading:") {
      input >> n;
      cout << "heading is " << n << endl;
      FGBFI::setHeading(n);
    } 

    else if (text == "pitch:") {
      input >> n;
      cout << "pitch is " << n << endl;
      FGBFI::setPitch(n);
    } 

    else if (text == "roll:") {
      input >> n;
      cout << "roll is " << n << endl;
      FGBFI::setRoll(n);
    } 

    //
    // Velocities
    //

    else if (text == "speed-north:") {
      input >> n;
      cout << "speed north is " << n << endl;
      FGBFI::setSpeedNorth(n);
    } 

    else if (text == "speed-east:") {
      input >> n;
      cout << "speed east is " << n << endl;
      FGBFI::setSpeedEast(n);
    } 

    else if (text == "speed-down:") {
      input >> n;
      cout << "speed down is " << n << endl;
      FGBFI::setSpeedDown(n);
    } 

    //
    // Primary controls
    //

    else if (text == "elevator:") {
      input >> n;
      cout << "elevator is " << n << endl;
      FGBFI::setElevator(n);
    }

    else if (text == "aileron:") {
      input >> n;
      cout << "aileron is " << n << endl;
      FGBFI::setAileron(n);
    }

    else if (text == "rudder:") {
      input >> n;
      cout << "rudder is " << n << endl;
      FGBFI::setRudder(n);
    }

				// FIXME: assumes single engine
    else if (text == "throttle:") {
      input >> n;
      cout << "throttle is " << n << endl;
      FGBFI::setThrottle(n);
    }

    //
    // Secondary controls

    else if (text == "elevator-trim:") {
      input >> n;
      cout << "elevator trim is " << n << endl;
      FGBFI::setElevatorTrim(n);
    }

    else if (text == "flaps:") {
      input >> n;
      cout << "flaps are " << n << endl;
      FGBFI::setFlaps(n);
    }

    else if (text == "brake:") {
      input >> n;
      cout << "brake is " << n << endl;
      FGBFI::setBrake(n);
    }


    //
    // Radio navigation
    //

    else if (text == "nav1-active-frequency:") {
      input >> n;
      FGBFI::setNAV1Freq(n);
    }

    else if (text == "nav1-standby-frequency:") {
      input >> n;
      FGBFI::setNAV1AltFreq(n);
    }

    else if (text == "nav1-selected-radial:") {
      input >> n;
      FGBFI::setNAV1SelRadial(n);
    }

    else if (text == "nav2-active-frequency:") {
      input >> n;
      FGBFI::setNAV2Freq(n);
    }

    else if (text == "nav2-standby-frequency:") {
      input >> n;
      FGBFI::setNAV2AltFreq(n);
    }

    else if (text == "nav2-selected-radial:") {
      input >> n;
      FGBFI::setNAV2SelRadial(n);
    }

    else if (text == "adf-active-frequency:") {
      input >> n;
      FGBFI::setADFFreq(n);
    }

    else if (text == "adf-standby-frequency:") {
      input >> n;
      FGBFI::setADFAltFreq(n);
    }

    else if (text == "adf-rotation:") {
      input >> n;
      FGBFI::setADFRotation(n);
    }


    //
    // Autopilot and GPS
    //

    else if (text == "target-airport:") {
      input >> text;
      cout << "target airport is " << text << endl;
      FGBFI::setTargetAirport(text);
    }

    else if (text == "autopilot-altitude-lock:") {
      input >> i;
      cout << "autopilot altitude lock is " << i << endl;
      FGBFI::setAPAltitudeLock(i);
    }

    else if (text == "autopilot-altitude:") {
      input >> n;
      cout << "autopilot altitude is " << n << endl;
      FGBFI::setAPAltitude(n);
    }

    else if (text == "autopilot-heading-lock:") {
      input >> i;
      cout << "autopilot heading lock is " << i << endl;
      FGBFI::setAPHeadingLock(i);
    }

    else if (text == "autopilot-heading:") {
      input >> n;
      cout << "autopilot heading is " << n << endl;
      FGBFI::setAPHeadingMag(n);
    }

    else if (text == "autopilot-gps-lock:") {
      input >> i;
      cout << "autopilot GPS lock is " << i << endl;
      FGBFI::setGPSLock(i);
    }

    else if (text == "autopilot-gps-lat:") {
      input >> n;
      cout << "GPS target latitude is " << n << endl;
      FGBFI::setGPSTargetLatitude(n);
    }

    else if (text == "autopilot-gps-lon:") {
      input >> n;
      cout << "GPS target longitude is " << n << endl;
      FGBFI::setGPSTargetLongitude(n);
    }

    //
    // Environment.
    //

    else if (text == "visibility:") {
      input >> n;
      cout << "visibility is " << n << endl;
      FGBFI::setVisibility(n);
    }

    else if (text == "clouds:") {
      input >> i;
      cout << "clouds is " << i << endl;
      FGBFI::setClouds(i);
    }

    else if (text == "clouds-asl:") {
      input >> n;
      cout << "clouds-asl is " << n << endl;
      FGBFI::setCloudsASL(n);
    }

    //
    // Don't die if we don't recognize something
    //
    
    else {
      cerr << "Skipping unknown field: " << text << endl;
      input >> text;
    }
  }

  FGBFI::update();

  return true;
}

// end of save.cxx
