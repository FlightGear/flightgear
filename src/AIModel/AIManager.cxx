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

SG_USING_STD(list);


FGAIManager::FGAIManager() {
  initDone = false;
}

FGAIManager::~FGAIManager() {
  ai_list.clear();
}

void FGAIManager::init() {
  SGPropertyNode * node = fgGetNode("sim/ai", true);

  for (int i = 0; i < node->nChildren(); i++) {
    const SGPropertyNode * entry = node->getChild(i);

    if (!strcmp(entry->getName(), "entry")) {
      if (!strcmp(entry->getStringValue("type", ""), "aircraft")) { 
        FGAIAircraft* ai_plane = new FGAIAircraft;
        ai_list.push_back(ai_plane);

        string model_class = entry->getStringValue("class", "");
        if (model_class == "light") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::LIGHT]);

        } else if (model_class == "ww2_fighter") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::WW2_FIGHTER]);

        } else if (model_class ==  "jet_transport") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_TRANSPORT]);

        } else if (model_class == "jet_fighter") {
          ai_plane->SetPerformance(&FGAIAircraft::settings[FGAIAircraft::JET_FIGHTER]);
        }

        ai_plane->setHeading(entry->getDoubleValue("heading"));
        ai_plane->setSpeed(entry->getDoubleValue("speed-KTAS"));
        ai_plane->setPath(entry->getStringValue("path"));
        ai_plane->setAltitude(entry->getDoubleValue("altitude-ft"));
        ai_plane->setLongitude(entry->getDoubleValue("longitude"));
        ai_plane->setLatitude(entry->getDoubleValue("latitude"));
        ai_plane->setBank(entry->getDoubleValue("bank"));
        ai_plane->init();
        ai_plane->bind();

      } else if (!strcmp(entry->getStringValue("type", ""), "ship")) {
        FGAIShip* ai_ship = new FGAIShip;
        ai_list.push_back(ai_ship);
        ai_ship->setHeading(entry->getDoubleValue("heading"));
        ai_ship->setSpeed(entry->getDoubleValue("speed-KTAS"));
        ai_ship->setPath(entry->getStringValue("path"));
        ai_ship->setAltitude(entry->getDoubleValue("altitude-ft"));
        ai_ship->setLongitude(entry->getDoubleValue("longitude"));
        ai_ship->setLatitude(entry->getDoubleValue("latitude"));
        ai_ship->setBank(entry->getDoubleValue("rudder"));
        ai_ship->init();
        ai_ship->bind();

      } else if (!strcmp(entry->getStringValue("type", ""), "ballistic")) {
        FGAIBallistic* ai_ballistic = new FGAIBallistic;
        ai_list.push_back(ai_ballistic);
        ai_ballistic->setAzimuth(entry->getDoubleValue("azimuth"));
        ai_ballistic->setElevation(entry->getDoubleValue("elevation"));
        ai_ballistic->setSpeed(entry->getDoubleValue("speed-fps"));
        ai_ballistic->setPath(entry->getStringValue("path"));
        ai_ballistic->setAltitude(entry->getDoubleValue("altitude-ft"));
        ai_ballistic->setLongitude(entry->getDoubleValue("longitude"));
        ai_ballistic->setLatitude(entry->getDoubleValue("latitude"));
        ai_ballistic->init();
        ai_ballistic->bind();
      } 
    }
  }

  initDone = true;
}


void FGAIManager::bind() {
}


void FGAIManager::unbind() {
    ai_list_itr = ai_list.begin();
    while(ai_list_itr != ai_list.end()) {
        (*ai_list_itr)->unbind();
        ++ai_list_itr;
    }
}


void FGAIManager::update(double dt) {
#if 0
	if(!initDone) {
          init();
          SG_LOG(SG_ATC, SG_WARN, "Warning - AIManager::update(...) called before AIManager::init()");
	}
#endif
	
        ai_list_itr = ai_list.begin();
        while(ai_list_itr != ai_list.end()) {
                if ((*ai_list_itr)->getDie()) {
                   ai_list.erase(ai_list_itr, ai_list_itr);
                } else {
                   (*ai_list_itr)->update(dt);
                }
                ++ai_list_itr;
        }
}
