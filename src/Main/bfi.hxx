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
  static void setFlightModel (int flightModel);

  static const string &getAircraft ();
  static void setAircraft (const string &aircraft);

  static const string &getAircraftDir ();
  static void setAircraftDir (const string &aircraftDir);

  static time_t getTimeGMT ();
  static void setTimeGMT (time_t time);

  static const string &getGMTString ();

  static bool getHUDVisible ();
  static void setHUDVisible (bool hudVisible);

  static bool getPanelVisible ();
  static void setPanelVisible (bool panelVisible);

  static int getPanelXOffset ();
  static void setPanelXOffset (int i);

  static int getPanelYOffset ();
  static void setPanelYOffset (int i);


				// Position
  static double getLatitude ();
  static void setLatitude (double latitude);

  static double getLongitude ();
  static void setLongitude (double longitude);

  static double getAltitude ();
  static void setAltitude (double altitude);

  static double getAGL ();


				// Attitude
  static double getHeading ();	  // true heading
  static void setHeading (double heading);

  static double getHeadingMag (); // exact magnetic heading

  static double getPitch ();
  static void setPitch (double pitch);

  static double getRoll ();
  static void setRoll (double roll);

				// Engine
  static double getRPM ();
  static void setRPM ( double rpm );

  static double getEGT ();
  static double getCHT ();
  static double getMP ();

				// Velocities
  static double getAirspeed ();
  static void setAirspeed (double speed);

  static double getSideSlip ();

  static double getVerticalSpeed ();

  static double getSpeedNorth ();

  static double getSpeedEast ();

  static double getSpeedDown ();

//   static void setSpeedNorth (double speed);
//   static void setSpeedEast (double speed);
//   static void setSpeedDown (double speed);


				// Controls
  static double getThrottle ();
  static void setThrottle (double throttle);

  static double getMixture ();
  static void setMixture (double mixture);

  static double getPropAdvance ();
  static void setPropAdvance (double pitch);

  static double getFlaps ();
  static void setFlaps (double flaps);

  static double getAileron ();
  static void setAileron (double aileron);

  static double getRudder ();
  static void setRudder (double rudder);

  static double getElevator ();
  static void setElevator (double elevator);

  static double getElevatorTrim ();
  static void setElevatorTrim (double trim);

  static double getBrakes ();
  static void setBrakes (double brake);

  static double getLeftBrake ();
  static void setLeftBrake (double brake);

  static double getRightBrake ();
  static void setRightBrake (double brake);

  static double getCenterBrake ();
  static void setCenterBrake (double brake);


				// Autopilot
  static bool getAPAltitudeLock ();
  static void setAPAltitudeLock (bool lock);

  static double getAPAltitude ();
  static void setAPAltitude (double altitude);

  static bool getAPHeadingLock ();
  static void setAPHeadingLock (bool lock);

  static double getAPHeading ();
  static void setAPHeading (double heading);

  static double getAPHeadingMag ();
  static void setAPHeadingMag (double heading);

  static bool getAPNAV1Lock ();
  static void setAPNAV1Lock (bool lock);

				// Radio Navigation
  static double getNAV1Freq ();
  static void setNAV1Freq (double freq);

  static double getNAV1AltFreq ();
  static void setNAV1AltFreq (double freq);

  static double getNAV1Radial ();

  static double getNAV1SelRadial ();
  static void setNAV1SelRadial (double radial);

  static double getNAV1DistDME ();

  static bool getNAV1TO ();

  static bool getNAV1FROM ();

  static bool getNAV1InRange ();

  static bool getNAV1DMEInRange ();

  static double getNAV2Freq ();
  static void setNAV2Freq (double freq);

  static double getNAV2AltFreq ();
  static void setNAV2AltFreq (double freq);

  static double getNAV2Radial ();

  static double getNAV2SelRadial ();
  static void setNAV2SelRadial (double radial);

  static double getNAV2DistDME ();

  static bool getNAV2TO ();

  static bool getNAV2FROM ();

  static bool getNAV2InRange ();

  static bool getNAV2DMEInRange ();

  static double getADFFreq ();
  static void setADFFreq (double freq);

  static double getADFAltFreq ();
  static void setADFAltFreq (double freq);

  static double getADFRotation ();
  static void setADFRotation (double rot);

				// GPS
  static const string &getTargetAirport ();
  static void setTargetAirport (const string &targetAirport);

  static bool getGPSLock ();
  static void setGPSLock (bool lock);

  static double getGPSTargetLatitude ();

  static double getGPSTargetLongitude ();


				// Weather
  static double getVisibility ();
  static void setVisibility (double visiblity);


                                // Time (this varies with time) huh, huh
  static double getMagVar (); 
  static double getMagDip (); 


private:
				// Will cause a linking error if invoked.
  FGBFI ();

};

// end of bfi.hxx
