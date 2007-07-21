// dynamics.hxx - a class to manage the higher order airport ground activities
// Written by Durk Talsma, started December 2004.
//
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _AIRPORT_DYNAMICS_HXX_
#define _AIRPORT_DYNAMICS_HXX_


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/xml/easyxml.hxx>

#include "parking.hxx"
#include "groundnetwork.hxx"
#include "runwayprefs.hxx"
#include "trafficcontrol.hxx"

class FGAirport;

class FGAirportDynamics {

private:
  FGAirport* _ap;

  FGParkingVec       parkings;
  FGRunwayPreference rwyPrefs;
  FGGroundNetwork    groundNetwork;
  FGTowerController  towerController;

  time_t lastUpdate;
  string prevTrafficType;
  stringVec landing;
  stringVec takeoff;
  stringVec milActive, comActive, genActive, ulActive;
  stringVec *currentlyActive;

  // Experimental keep a running average of wind dir and speed to prevent
  // Erratic runway changes. 
  // Note: I should add these to the copy constructor and assigment operator to be
  // constistent
  //double avWindHeading [10];
  //double avWindSpeed   [10];

  string chooseRunwayFallback();

public:
  FGAirportDynamics(FGAirport* ap);
  FGAirportDynamics(const FGAirportDynamics &other);
  ~FGAirportDynamics();


  void init();
  double getLongitude() const; 
  // Returns degrees
  double getLatitude()  const; 
  // Returns ft
  double getElevation() const; 
  const string& getId() const; 
  
  void getActiveRunway(const string& trafficType, int action, string& runway);

  void addParking(FGParking& park);
  bool getAvailableParking(double *lat, double *lon, 
			   double *heading, int *gate, double rad, const string& fltype, 
			   const string& acType, const string& airline);
  void getParking         (int id, double *lat, double* lon, double *heading);
  FGParking *getParking(int i);
  void releaseParking(int id);
  string getParkingName(int i); 
  //FGAirport *getAddress() { return this; };
  //const string &getName() const { return _name;};
  // Returns degrees

  FGGroundNetwork   *getGroundNetwork()   { return &groundNetwork; };
  FGTowerController *getTowerController() { return &towerController; };
  

  void setRwyUse(const FGRunwayPreference& ref);
};



#endif
