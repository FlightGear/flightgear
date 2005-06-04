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

#include <Airports/simple.hxx>
#include <Traffic/SchedFlight.hxx>
#include <Traffic/Schedule.hxx>
#include <Traffic/TrafficMgr.hxx>

#include <list>

#include "AIManager.hxx"
#include "AIAircraft.hxx"
#include "AIShip.hxx"
#include "AIBallistic.hxx"
#include "AIStorm.hxx"
#include "AIThermal.hxx"
#include "AICarrier.hxx"
#include "AIStatic.hxx"

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
  ai_list_iterator ai_list_itr = ai_list.begin();
  while(ai_list_itr != ai_list.end()) {
      (*ai_list_itr)->unbind();
      delete (*ai_list_itr);
      ++ai_list_itr;
    }
  ai_list.clear();
  ModelVecIterator i = loadedModels.begin();
  while (i != loadedModels.end())
    {
      i->getModelId()->deRef();
    }
}


void FGAIManager::init() {
  root = fgGetNode("sim/ai", true);

  enabled = root->getNode("enabled", true)->getBoolValue();
  if (!enabled)
      return;

  wind_from_down_node = fgGetNode("/environment/wind-from-down-fps", true);
  user_latitude_node  = fgGetNode("/position/latitude-deg", true);
  user_longitude_node = fgGetNode("/position/longitude-deg", true);
  user_altitude_node  = fgGetNode("/position/altitude-ft", true);
  user_heading_node   = fgGetNode("/orientation/heading-deg", true);
  user_pitch_node     = fgGetNode("/orientation/pitch-deg", true);
  user_yaw_node       = fgGetNode("/orientation/side-slip-deg", true);
  user_speed_node     = fgGetNode("/velocities/uBody-fps", true);
 
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
	FGTrafficManager *tmgr = (FGTrafficManager*) globals->get_subsystem("Traffic Manager");

        if (!enabled)
            return;

        _dt = dt;

        ai_list_iterator ai_list_itr = ai_list.begin();
        while(ai_list_itr != ai_list.end()) {
                if ((*ai_list_itr)->getDie()) {      
		  tmgr->release((*ai_list_itr)->getID());
                   --numObjects[(*ai_list_itr)->getType()];
                   --numObjects[0];
                   (*ai_list_itr)->unbind();
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
        wind_from_down_node->setDoubleValue( strength ); // for thermals

}


void*
FGAIManager::createAircraft( FGAIModelEntity *entity,   FGAISchedule *ref) {
     
        FGAIAircraft* ai_plane = new FGAIAircraft(this, ref);
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
	ai_plane->setAcType(entity->acType);
	ai_plane->setCompany(entity->company);
        ai_plane->setHeading(entity->heading);
        ai_plane->setSpeed(entity->speed);
        ai_plane->setPath(entity->path.c_str());
        ai_plane->setAltitude(entity->altitude);
        ai_plane->setLongitude(entity->longitude);
        ai_plane->setLatitude(entity->latitude);
        ai_plane->setBank(entity->roll);

        if ( entity->fp ) {
          ai_plane->SetFlightPlan(entity->fp);
        }
        if (entity->repeat) {
          ai_plane->GetFlightPlan()->setRepeat(true);
        }
        ai_plane->init();
        ai_plane->bind();
        return ai_plane;
}

void*
FGAIManager::createShip( FGAIModelEntity *entity ) {
    
     // cout << "creating ship" << endl;    

        FGAIShip* ai_ship = new FGAIShip(this);
        ai_list.push_back(ai_ship);
        ++numObjects[0];
        ++numObjects[FGAIBase::otShip];
        ai_ship->setHeading(entity->heading);
        ai_ship->setSpeed(entity->speed);
        ai_ship->setPath(entity->path.c_str());
        ai_ship->setAltitude(entity->altitude);
        ai_ship->setLongitude(entity->longitude);
        ai_ship->setLatitude(entity->latitude);
        ai_ship->setBank(entity->rudder);
        ai_ship->setName(entity->name);

        if ( entity->fp ) {
           ai_ship->setFlightPlan(entity->fp);
        }

        ai_ship->init();
        ai_ship->bind();
        return ai_ship;
}

void*
FGAIManager::createCarrier( FGAIModelEntity *entity ) {
    
    // cout << "creating carrier" << endl;

        FGAICarrier* ai_carrier = new FGAICarrier(this);
        ai_list.push_back(ai_carrier);
        ++numObjects[0];
        ++numObjects[FGAIBase::otCarrier];
        ai_carrier->setHeading(entity->heading);
        ai_carrier->setSpeed(entity->speed);
        ai_carrier->setPath(entity->path.c_str());
        ai_carrier->setAltitude(entity->altitude);
        ai_carrier->setLongitude(entity->longitude);
        ai_carrier->setLatitude(entity->latitude);
        ai_carrier->setBank(entity->rudder);
        ai_carrier->setSolidObjects(entity->solid_objects);
        ai_carrier->setWireObjects(entity->wire_objects);
        ai_carrier->setCatapultObjects(entity->catapult_objects);
        ai_carrier->setParkingPositions(entity->ppositions);
        ai_carrier->setRadius(entity->radius);
        ai_carrier->setSign(entity->pennant_number);
        ai_carrier->setName(entity->name);
        ai_carrier->setFlolsOffset(entity->flols_offset);

        if ( entity->fp ) {
           ai_carrier->setFlightPlan(entity->fp);
        }

        ai_carrier->init();
        ai_carrier->bind();
        return ai_carrier;
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
        ai_ballistic->setPath(entity->path.c_str());
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
        ai_ballistic->setMass(entity->mass);
        ai_ballistic->setStabilisation(entity->aero_stabilised);
        ai_ballistic->init();
        ai_ballistic->bind();
        return ai_ballistic;
}

void*
FGAIManager::createStorm( FGAIModelEntity *entity ) {

        FGAIStorm* ai_storm = new FGAIStorm(this);
        ++numObjects[0];
        ++numObjects[FGAIBase::otStorm];
        ai_storm->setHeading(entity->heading);
        ai_storm->setSpeed(entity->speed);
        ai_storm->setPath(entity->path.c_str());
        ai_storm->setAltitude(entity->altitude); 
        ai_storm->setDiameter(entity->diameter / 6076.11549); 
        ai_storm->setHeight(entity->height_msl); 
        ai_storm->setStrengthNorm(entity->strength); 
        ai_storm->setLongitude(entity->longitude);
        ai_storm->setLatitude(entity->latitude);
        ai_storm->init();
        ai_storm->bind();
        ai_list.push_back(ai_storm);
        return ai_storm;
}

void*
FGAIManager::createThermal( FGAIModelEntity *entity ) {

        FGAIThermal* ai_thermal = new FGAIThermal(this);
        ++numObjects[0];
        ++numObjects[FGAIBase::otThermal];
        ai_thermal->setLongitude(entity->longitude);
        ai_thermal->setLatitude(entity->latitude);
        ai_thermal->setMaxStrength(entity->strength);
        ai_thermal->setDiameter(entity->diameter / 6076.11549);
        ai_thermal->setHeight(entity->height_msl);
        ai_thermal->init();
        ai_thermal->bind();
        ai_list.push_back(ai_thermal);
        return ai_thermal;
}

void*
FGAIManager::createStatic( FGAIModelEntity *entity ) {
     
     // cout << "creating static object" << endl;    

        FGAIStatic* ai_static = new FGAIStatic(this);
        ai_list.push_back(ai_static);
        ++numObjects[0];
        ++numObjects[FGAIBase::otStatic];
        ai_static->setHeading(entity->heading);
        ai_static->setPath(entity->path.c_str());
        ai_static->setAltitude(entity->altitude);
        ai_static->setLongitude(entity->longitude);
        ai_static->setLatitude(entity->latitude);
        ai_static->init();
        ai_static->bind();
        return ai_static;
}

void FGAIManager::destroyObject( void* ID ) {
        ai_list_iterator ai_list_itr = ai_list.begin();
        while(ai_list_itr != ai_list.end()) {
            if ((*ai_list_itr)->getID() == ID) {
              --numObjects[0];
              --numObjects[(*ai_list_itr)->getType()];
              (*ai_list_itr)->unbind();
              delete (*ai_list_itr);
              ai_list.erase(ai_list_itr);

              break;
            }
            ++ai_list_itr;
        }
}


void FGAIManager::fetchUserState( void ) {
     user_latitude  = user_latitude_node->getDoubleValue();
     user_longitude = user_longitude_node->getDoubleValue();
     user_altitude  = user_altitude_node->getDoubleValue();
     user_heading   = user_heading_node->getDoubleValue();
     user_pitch     = user_pitch_node->getDoubleValue();
     user_yaw       = user_yaw_node->getDoubleValue();
     user_speed     = user_speed_node->getDoubleValue() * 0.592484;
}


// only keep the results from the nearest thermal
void FGAIManager::processThermal( FGAIThermal* thermal ) {
  thermal->update(_dt);
  if ( thermal->_getRange() < range_nearest ) {
     range_nearest = thermal->_getRange();
     strength = thermal->getStrength();
  }
}


void FGAIManager::processScenario( string &filename ) {
  FGAIScenario* s = new FGAIScenario( filename );
  for (int i=0;i<s->nEntries();i++) {
    FGAIModelEntity* en = s->getNextEntry();

    if (en) {
      if ( en->m_type == "aircraft") {
        createAircraft( en );

      } else if ( en->m_type == "ship") {
        createShip( en );

      } else if ( en->m_type == "carrier") {
        createCarrier( en );

      } else if ( en->m_type == "thunderstorm") {
        createStorm( en );

      } else if ( en->m_type == "thermal") {
        createThermal( en );

      } else if ( en->m_type == "ballistic") {
        createBallistic( en );

      } else if ( en->m_type == "static") {
        createStatic( en );
      }            
    }
  }

  delete s;
}

// This code keeps track of models that have already been loaded
// Eventually we'd prbably need to find a way to keep track of models
// that are unloaded again
ssgBranch * FGAIManager::getModel(const string& path)
{
  ModelVecIterator i = loadedModels.begin();
  while (i != loadedModels.end())
    {
      if (i->getPath() == path)
	return i->getModelId();
      i++;
    }
  return 0;
}

void FGAIManager::setModel(const string& path, ssgBranch *model)
{
  loadedModels.push_back(FGModelID(path,model));
}


//end AIManager.cxx
