// bfi.hxx - Big Flat Interface
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

#include <time.h>
#include <string>

#include <simgear/compiler.h>

FG_USING_NAMESPACE(std);


/**
 * Big Flat Interface
 *
 * This class implements the Facade design pattern (GOF p.185) to provide
 * a single, (deceptively) simple flat interface for the FlightGear
 * subsystems.  
 *
 * To help cut down on interdependence, subsystems should
 * use the BFI whenever possible for inter-system communication.
 *
 * TODO:
 * - add selectors to switch the current plane, throttle, brake, etc.
 * - add more autopilot settings
 */
class FGBFI
{
public:

				// Initialize before first use.
  static void init ();

				// Reinit if necessary.
  static void update ();

				// Simulation
  static int getFlightModel ();
  static const string getAircraft ();
  static const string getAircraftDir ();
  static time_t getTimeGMT ();
  static const string &getGMTString ();
  static bool getHUDVisible ();
  static bool getPanelVisible ();

  static void setFlightModel (int flightModel);
  static void setAircraft (const string &aircraft);
  static void setAircraftDir (const string &aircraftDir);
  static void setTimeGMT (time_t time);
  static void setHUDVisible (bool hudVisible);
  static void setPanelVisible (bool panelVisible);


				// Position
  static double getLatitude ();
  static double getLongitude ();
  static double getAltitude ();
  static double getAGL ();

  static void setLatitude (double latitude);
  static void setLongitude (double longitude);
  static void setAltitude (double altitude);


				// Attitude
  static double getHeading ();	  // true heading
  static double getHeadingMag (); // exact magnetic heading
  static double getPitch ();
  static double getRoll ();

  static void setHeading (double heading);
  static void setPitch (double pitch);
  static void setRoll (double roll);

				// Engine
  static double getRPM ();
  static void setRPM ( double rpm );
  static double getEGT ();

				// Velocities
  static double getAirspeed ();
  static double getSideSlip ();
  static double getVerticalSpeed ();
  static double getSpeedNorth ();
  static double getSpeedEast ();
  static double getSpeedDown ();

  static void setSpeedNorth (double speed);
  static void setSpeedEast (double speed);
  static void setSpeedDown (double speed);


				// Controls
  static double getThrottle ();
  static double getMixture ();
  static double getPropAdvance ();
  static double getFlaps ();
  static double getAileron ();
  static double getRudder ();
  static double getElevator ();
  static double getElevatorTrim ();
  static double getBrakes ();
  static double getLeftBrake ();
  static double getRightBrake ();
  static double getCenterBrake ();

  static void setThrottle (double throttle);
  static void setMixture (double mixture);
  static void setPropAdvance (double pitch);
  static void setFlaps (double flaps);
  static void setAileron (double aileron);
  static void setRudder (double rudder);
  static void setElevator (double elevator);
  static void setElevatorTrim (double trim);
  static void setBrakes (double brake);
  static void setLeftBrake (double brake);
  static void setRightBrake (double brake);
  static void setCenterBrake (double brake);


				// Autopilot
  static bool getAPAltitudeLock ();
  static double getAPAltitude ();
  static bool getAPHeadingLock ();
  static double getAPHeading ();
  static double getAPHeadingMag ();

  static void setAPAltitudeLock (bool lock);
  static void setAPAltitude (double altitude);
  static void setAPHeadingLock (bool lock);
  static void setAPHeading (double heading);
  static void setAPHeadingMag (double heading);

  static bool getAPNAV1Lock ();
  static void setAPNAV1Lock (bool lock);

				// Radio Navigation
  static double getNAV1Freq ();
  static double getNAV1AltFreq ();
  static double getNAV1Radial ();
  static double getNAV1SelRadial ();
  static double getNAV1DistDME ();
  static bool getNAV1TO ();
  static bool getNAV1FROM ();
  static bool getNAV1InRange ();
  static bool getNAV1DMEInRange ();

  static double getNAV2Freq ();
  static double getNAV2AltFreq ();
  static double getNAV2Radial ();
  static double getNAV2SelRadial ();
  static double getNAV2DistDME ();
  static bool getNAV2TO ();
  static bool getNAV2FROM ();
  static bool getNAV2InRange ();
  static bool getNAV2DMEInRange ();

  static double getADFFreq ();
  static double getADFAltFreq ();
  static double getADFRotation ();

  static void setNAV1Freq (double freq);
  static void setNAV1AltFreq (double freq);
  static void setNAV1SelRadial (double radial);

  static void setNAV2Freq (double freq);
  static void setNAV2AltFreq (double freq);
  static void setNAV2SelRadial (double radial);

  static void setADFFreq (double freq);
  static void setADFAltFreq (double freq);
  static void setADFRotation (double rot);

				// GPS
  static const string &getTargetAirport ();
  static bool getGPSLock ();
  static double getGPSTargetLatitude ();
  static double getGPSTargetLongitude ();

  static void setTargetAirport (const string &targetAirport);
  static void setGPSLock (bool lock);
  static void setGPSTargetLatitude (double latitude);
  static void setGPSTargetLongitude (double longitude);


				// Weather
  static double getVisibility ();
  static bool getClouds ();
  static double getCloudsASL ();

  static void setVisibility (double visiblity);
  static void setClouds (bool clouds);
  static void setCloudsASL (double cloudsASL);


                                // Time (this varies with time) huh, huh
  static double getMagVar (); 
  static double getMagDip (); 


private:
				// Will cause a linking error if invoked.
  FGBFI ();

  static void reinit ();
  static void needReinit () { _needReinit = true; }
  static bool _needReinit;
};

// end of bfi.hxx
