// AIManager.cxx  Based on David Luff's AIMgr:
// - a global management type for AI objects
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
  for (int i=0; i < FGAIBase::MAX_OBJECTS; i++)
     numObjects[i] = 0;
  _dt = 0.0;
  dt_count = 9;
  scenario_filename = "";
  ai_list.clear();
}

FGAIManager::~FGAIManager() {
  ai_list_itr = ai_list.begin();
  while(ai_list_itr != ai_list.end()) {
      delete (*ai_list_itr);
      ++ai_list_itr;
    }
  ai_list.clear();
}


void FGAIManager::init() {
  root = fgGetNode("sim/ai", true);

  enabled = root->getNode("enabled", true)->getBoolValue();
  if (!enabled)
      return;

  wind_from_down_node = fgGetNode("/environment/wind-from-down-fps", true);
 
  scenario_filename = root->getNode("scenario", true)->getStringValue();

  if (scenario_filename != "") processScenario( scenario_filename );
  initDone = true;
}


void FGAIManager::bind() {
   root = globals->get_props()->getNode("ai/models", true);
   root->tie("count", SGRawValuePointer<int>(&numObjects[0]));
}


void FGAIManager::unbind() {
    root->untie("count");
}


void FGAIManager::update(double dt) {

        // initialize these for finding nearest thermals
        range_nearest = 10000.0;
        strength = 0.0;

        if (!enabled)
            return;

        _dt = dt;	

        ai_list_itr = ai_list.begin();
        while(ai_list_itr != ai_list.end()) {
                if ((*ai_list_itr)->getDie()) {      
                   --numObjects[(*ai_list_itr)->getType()];
                   --numObjects[0];
                   delete (*ai_list_itr);
                   if ( ai_list_itr == ai_list.begin() ) {
                       ai_list.erase(ai_list_itr);
                       ai_list_itr = ai_list.begin();
                       continue;
                   } else {
                       ai_list.erase(ai_list_itr--);
                   }
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
        wind_from_down_node->setDoubleValue( strength );

}


void*
FGAIManager::createAircraft( FGAIModelEntity *entity ) {
     
        FGAIAircraft* ai_plane = new FGAIAircraft(this);
        ai_list.push_back(ai_plane);
        ++numObjects[0];
        ++numObjects[FGAIBase::otAircraft];
        if (entity->m_class == "light") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::LIGHT]);
        } else if (entity->m_class == "ww2_fighter") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::WW2_FIGHTER]);
        } else if (entity->m_class ==  "jet_transport") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
        } else if (entity->m_class == "jet_fighter") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_FIGHTER]);
        } else if (entity->m_class ==  "tanker") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
          ai_plane->SetTanker(true);
        } else {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);
        }
        ai_plane->setHeading(entity->heading);
        ai_plane->setSpeed(entity->speed);
        ai_plane->setPath(entity->path);
        ai_plane->setAltitude(entity->altitude);
        ai_plane->setLongitude(entity->longitude);
        ai_plane->setLatitude(entity->latitude);
        ai_plane->setBank(entity->roll);

        if ( entity->fp ) {
          ai_plane->SetFlightPlan(entity->fp);
        }

        ai_plane->init();
        ai_plane->bind();
        return ai_plane;
}

void*
FGAIManager::createShip( FGAIModelEntity *entity ) {

        FGAIShip* ai_ship = new FGAIShip(this);
        ai_list.push_back(ai_ship);
        ++numObjects[0];
        ++numObjects[FGAIBase::otShip];
        ai_ship->setHeading(entity->heading);
        ai_ship->setSpeed(entity->speed);
        ai_ship->setPath(entity->path);
        ai_ship->setAltitude(entity->altitude);
        ai_ship->setLongitude(entity->longitude);
        ai_ship->setLatitude(entity->latitude);
        ai_ship->setBank(entity->rudder);

        if ( entity->fp ) {
           ai_ship->setFlightPlan(entity->fp);
        }

        ai_ship->init();
        ai_ship->bind();
        return ai_ship;
}

void*
FGAIManager::createBallistic( FGAIModelEntity *entity ) {

        FGAIBallistic* ai_ballistic = new FGAIBallistic(this);
        ai_list.push_back(ai_ballistic);
        ++numObjects[0];
        ++numObjects[FGAIBase::otBallistic];
        ai_ballistic->setAzimuth(entity->azimuth);
        ai_ballistic->setElevation(entity->elevation);
        ai_ballistic->setSpeed(entity->speed);
        ai_ballistic->setPath(entity->path);
        ai_ballistic->setAltitude(entity->altitude);
        ai_ballistic->setLongitude(entity->longitude);
        ai_ballistic->setLatitude(entity->latitude);
        ai_ballistic->setDragArea(entity->eda);
        ai_ballistic->setLife(entity->life);
        ai_ballistic->setBuoyancy(entity->buoyancy);
        ai_ballistic->setWind_from_east(entity->wind_from_east);
        ai_ballistic->setWind_from_north(entity->wind_from_north);
        ai_ballistic->setWind(entity->wind);
        ai_ballistic->setRoll(entity->roll);
		ai_ballistic->setCd(entity->cd);
		ai_ballistic->setWeight(entity->weight);
        ai_ballistic->init();
        ai_ballistic->bind();
        return ai_ballistic;
}

void*
FGAIManager::createStorm( FGAIModelEntity *entity ) {

        FGAIStorm* ai_storm = new FGAIStorm(this);
        ai_list.push_back(ai_storm);
        ++numObjects[0];
        ++numObjects[FGAIBase::otStorm];
        ai_storm->setHeading(entity->heading);
        ai_storm->setSpeed(entity->speed);
        ai_storm->setPath(entity->path);
        ai_storm->setAltitude(entity->altitude);
        ai_storm->setLongitude(entity->longitude);
        ai_storm->setLatitude(entity->latitude);
        ai_storm->init();
        ai_storm->bind();
        return ai_storm;
}

void*
FGAIManager::createThermal( FGAIModelEntity *entity ) {

        FGAIThermal* ai_thermal = new FGAIThermal(this);
        ai_list.push_back(ai_thermal);
        ++numObjects[0];
        ++numObjects[FGAIBase::otThermal];
        ai_thermal->setLongitude(entity->longitude);
        ai_thermal->setLatitude(entity->latitude);
        ai_thermal->setMaxStrength(entity->strength);
        ai_thermal->setDiameter(entity->diameter / 6076.11549);
        ai_thermal->init();
        ai_thermal->bind();
        return ai_thermal;
}

void FGAIManager::destroyObject( void* ID ) {
        ai_list_itr = ai_list.begin();
        while(ai_list_itr != ai_list.end()) {
            if ((*ai_list_itr)->getID() == ID) {
              --numObjects[0];
              --numObjects[(*ai_list_itr)->getType()];
              delete (*ai_list_itr);
              ai_list.erase(ai_list_itr);

              break;
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
  FGAIScenario* s = new FGAIScenario( filename );
  for (int i=0;i<s->nEntries();i++) {
    FGAIModelEntity* en = s->getNextEntry();

    if (en) {

 
      if (en->m_class == "aircraft") {
         createAircraft( en );

      } else if (en->m_class == "ship") {
           createShip( en );

      } else if (en->m_class == "storm") {
        createStorm( en );

      } else if (en->m_class == "thermal") {
        createThermal( en );

      } else if (en->m_class == "ballistic") {
        createBallistic( en );
      }      
    }
  }

  delete s;
}

//end AIManager.cxx
