// AIManager.cxx  Based on David Luff's AIMgr:
// - a global management class for AI objects
//
// Written by David Culp, started October 2003.
// - davidculp2@comcast.net
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

#include <simgear/misc/sg_path.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include <list>

#include "AIManager.hxx"
#include "AIAircraft.hxx"
#include "AIShip.hxx"
#include "AIBallistic.hxx"
#include "AIStorm.hxx"
#include "AIThermal.hxx"

SG_USING_STD(list);


FGAIManager::FGAIManager() {
  initDone = false;
  numObjects = 0;
  _dt = 0.0;
  dt_count = 9;
}

FGAIManager::~FGAIManager() {
  ai_list_itr = ai_list.begin();
  while(ai_list_itr != ai_list.end()) {
      delete (*ai_list_itr);
      ++ai_list_itr;
    }
  ai_list.clear();
  ids.clear();
}


void FGAIManager::init() {
  int rval;
  root = fgGetNode("sim/ai", true);
  wind_from_down = fgGetNode("/environment/wind-from-down-fps", true);

  for (int i = 0; i < root->nChildren(); i++) {
    const SGPropertyNode * entry = root->getChild(i);

    if (!strcmp(entry->getName(), "entry")) {
      if (!strcmp(entry->getStringValue("type", ""), "aircraft")) { 

        rval = createAircraft( entry->getStringValue("class", ""),
                               entry->getStringValue("path"),
                               entry->getDoubleValue("latitude"),
                               entry->getDoubleValue("longitude"),
                               entry->getDoubleValue("altitude-ft"),
                               entry->getDoubleValue("heading"),
                               entry->getDoubleValue("speed-KTAS"),
                               0.0, 
                               entry->getDoubleValue("bank") );

      } else if (!strcmp(entry->getStringValue("type", ""), "ship")) {

        rval = createShip( entry->getStringValue("path"),
                           entry->getDoubleValue("latitude"),
                           entry->getDoubleValue("longitude"),
                           entry->getDoubleValue("altitude-ft"),
                           entry->getDoubleValue("heading"),
                           entry->getDoubleValue("speed-KTAS"),
                           entry->getDoubleValue("rudder") );

      } else if (!strcmp(entry->getStringValue("type", ""), "ballistic")) {

        rval = createBallistic( entry->getStringValue("path"),
                                entry->getDoubleValue("latitude"),
                                entry->getDoubleValue("longitude"),
                                entry->getDoubleValue("altitude-ft"),
                                entry->getDoubleValue("azimuth"),
                                entry->getDoubleValue("elevation"),
                                entry->getDoubleValue("speed") );

      } else if (!strcmp(entry->getStringValue("type", ""), "storm")) {

        rval = createStorm( entry->getStringValue("path"),
                            entry->getDoubleValue("latitude"),
                            entry->getDoubleValue("longitude"),
                            entry->getDoubleValue("altitude-ft"),
                            entry->getDoubleValue("heading"),
                            entry->getDoubleValue("speed-KTAS") );

      } else if (!strcmp(entry->getStringValue("type", ""), "thermal")) {

        rval = createThermal( entry->getDoubleValue("latitude"),
                              entry->getDoubleValue("longitude"),
                              entry->getDoubleValue("strength-fps"),
                              entry->getDoubleValue("diameter-ft") );

      }       
    }
  }

  //**********  Flight Plan test code !!!! ****************
  processScenario( "default_scenario" );
  //******************************************************* 

  initDone = true;
}


void FGAIManager::bind() {
   root = globals->get_props()->getNode("ai/models", true);
   root->tie("count", SGRawValuePointer<int>(&numObjects));
}


void FGAIManager::unbind() {
    root->untie("count");
}


void FGAIManager::update(double dt) {

        // initialize these for finding nearest thermals
        range_nearest = 10000.0;
        strength = 0.0;

        _dt = dt;	

        ai_list_itr = ai_list.begin();
        while(ai_list_itr != ai_list.end()) {
                if ((*ai_list_itr)->getDie()) {      
                   freeID((*ai_list_itr)->getID());
                   delete (*ai_list_itr);
                   ai_list.erase(ai_list_itr);
                   --ai_list_itr;
                   --numObjects;
                } else {
                   fetchUserState();
                   if ((*ai_list_itr)->isa(FGAIBase::otThermal)) {
                       processThermal((FGAIThermal*)*ai_list_itr); 
                   } else { 
                      (*ai_list_itr)->update(_dt);
                   }
                }
                ++ai_list_itr;
        }
        wind_from_down->setDoubleValue( strength );
}


// This function returns the next available ID
int FGAIManager::assignID() {
  int maxint = 30000;
  int x; 
  bool used;
  for (x=0; x<maxint; x++) {
     used = false;
     id_itr = ids.begin();
     while( id_itr != ids.end() ) {
       if ((*id_itr) == x) used = true;
       ++id_itr;
     }
     if (!used) {
       ids.push_back(x);
       return x;
     } 
  }
  return -1;  // no available ID's
}


// This function removes an ID from the ID array, making it
// available for assignment to another AI object
void FGAIManager::freeID( int ID ) {
    id_itr = ids.begin();
    while( id_itr != ids.end() ) {
      if (*id_itr == ID) {
        ids.erase( id_itr );
        return;
      }
      ++id_itr;
    }  
}

int FGAIManager::createAircraft( string model_class, string path,
              double latitude, double longitude, double altitude,
              double heading, double speed, double pitch, double roll ) {
     
        FGAIAircraft* ai_plane = new FGAIAircraft(this);
        ai_list.push_back(ai_plane);
        ai_plane->setID( assignID() );
        ++numObjects;
        if (model_class == "light") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::LIGHT]);
        } else if (model_class == "ww2_fighter") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::WW2_FIGHTER]);
        } else if (model_class ==  "jet_transport") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
        } else if (model_class == "jet_fighter") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_FIGHTER]);
        } else {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
        }
        ai_plane->setHeading(heading);
        ai_plane->setSpeed(speed);
        ai_plane->setPath(path.c_str());
        ai_plane->setAltitude(altitude);
        ai_plane->setLongitude(longitude);
        ai_plane->setLatitude(latitude);
        ai_plane->setBank(roll);
        ai_plane->init();
        ai_plane->bind();
        return ai_plane->getID();
}


int FGAIManager::createAircraft( string model_class, string path,
              FGAIFlightPlan* flightplan ) {
     
        FGAIAircraft* ai_plane = new FGAIAircraft(this);
        ai_list.push_back(ai_plane);
        ai_plane->setID( assignID() );
        ++numObjects;
        if (model_class == "light") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::LIGHT]);
        } else if (model_class == "ww2_fighter") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::WW2_FIGHTER]);
        } else if (model_class ==  "jet_transport") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
        } else if (model_class == "jet_fighter") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_FIGHTER]);
        } else {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
        }
        ai_plane->setPath(path.c_str());
        ai_plane->SetFlightPlan(flightplan);
        ai_plane->init();
        ai_plane->bind();
        return ai_plane->getID();
}


int FGAIManager::createShip( string path, double latitude, double longitude,
                             double altitude, double heading, double speed,
                             double rudder ) {

        FGAIShip* ai_ship = new FGAIShip(this);
        ai_list.push_back(ai_ship);
        ai_ship->setID( assignID() );
        ++numObjects;
        ai_ship->setHeading(heading);
        ai_ship->setSpeed(speed);
        ai_ship->setPath(path.c_str());
        ai_ship->setAltitude(altitude);
        ai_ship->setLongitude(longitude);
        ai_ship->setLatitude(latitude);
        ai_ship->setBank(rudder);
        ai_ship->init();
        ai_ship->bind();
        return ai_ship->getID();
}


int FGAIManager::createBallistic( string path, double latitude, double longitude,
                                  double altitude, double azimuth, double elevation,
                                  double speed ) {

        FGAIBallistic* ai_ballistic = new FGAIBallistic(this);
        ai_list.push_back(ai_ballistic);
        ai_ballistic->setID( assignID() );    
        ++numObjects;
        ai_ballistic->setAzimuth(azimuth);
        ai_ballistic->setElevation(elevation);
        ai_ballistic->setSpeed(speed);
        ai_ballistic->setPath(path.c_str());
        ai_ballistic->setAltitude(altitude);
        ai_ballistic->setLongitude(longitude);
        ai_ballistic->setLatitude(latitude);
        ai_ballistic->init();
        ai_ballistic->bind();
        return ai_ballistic->getID();
}

int FGAIManager::createStorm( string path, double latitude, double longitude,
                             double altitude, double heading, double speed ) {

        FGAIStorm* ai_storm = new FGAIStorm(this);
        ai_list.push_back(ai_storm);
        ai_storm->setID( assignID() );
        ++numObjects;
        ai_storm->setHeading(heading);
        ai_storm->setSpeed(speed);
        ai_storm->setPath(path.c_str());
        ai_storm->setAltitude(altitude);
        ai_storm->setLongitude(longitude);
        ai_storm->setLatitude(latitude);
        ai_storm->init();
        ai_storm->bind();
        return ai_storm->getID();
}

int FGAIManager::createThermal( double latitude, double longitude,
                                double strength, double diameter ) {

        FGAIThermal* ai_thermal = new FGAIThermal(this);
        ai_list.push_back(ai_thermal);
        ai_thermal->setID( assignID() );
        ++numObjects;
        ai_thermal->setLongitude(longitude);
        ai_thermal->setLatitude(latitude);
        ai_thermal->setMaxStrength(strength);
        ai_thermal->setDiameter(diameter / 6076.11549);
        ai_thermal->init();
        ai_thermal->bind();
        return ai_thermal->getID();
}

void FGAIManager::destroyObject( int ID ) {
        ai_list_itr = ai_list.begin();
        while(ai_list_itr != ai_list.end()) {
            if ((*ai_list_itr)->getID() == ID) {
              freeID( ID );
              delete (*ai_list_itr);
              ai_list.erase(ai_list_itr);
              --ai_list_itr;
              --numObjects;
              return;
            }
            ++ai_list_itr;
        }
}

// fetch the user's state every 10 sim cycles
void FGAIManager::fetchUserState( void ) {
   ++dt_count;
   if (dt_count == 10) {
     user_latitude  = fgGetDouble("/position/latitude-deg");
     user_longitude = fgGetDouble("/position/longitude-deg");
     user_altitude  = fgGetDouble("/position/altitude-ft");
     user_heading   = fgGetDouble("/orientation/heading-deg");
     user_pitch     = fgGetDouble("/orientation/pitch-deg");
     user_yaw       = fgGetDouble("/orientation/side-slip-deg");
     user_speed     = fgGetDouble("/velocities/uBody-fps") * 0.592484;
     dt_count = 0;
   }
}


// only keep the results from the nearest thermal
void FGAIManager::processThermal( FGAIThermal* thermal ) {
  thermal->update(_dt);
  if ( thermal->_getRange() < range_nearest ) {
     range_nearest = thermal->_getRange();
     strength = thermal->getStrength();
  }
}


void FGAIManager::processScenario( string filename ) {
  //cout << "AIManager: creating a scenario." << endl;
  FGAIScenario* s = new FGAIScenario( filename );
  FGAIScenario::entry* en = s->getNextEntry();
  if (en) {
    FGAIFlightPlan* f = new FGAIFlightPlan( en->flightplan );
    createAircraft("jet_transport", "Aircraft/737/Models/boeing733.xml", f);
  }
  delete s;
}

