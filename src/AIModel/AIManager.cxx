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
        if (!strcmp(entry->getStringValue("class", ""), "light")) {
          PERF_STRUCT ps = {2.0, 2.0, 450.0, 1000.0, 70.0, 80.0, 100.0, 80.0, 60.0};
          ai_plane->SetPerformance(ps);
        } else if (!strcmp(entry->getStringValue("class", ""), "ww2_fighter")) {
          PERF_STRUCT ps = {4.0, 2.0, 3000.0, 1500.0, 110.0, 180.0, 250.0, 200.0, 100.0};
          ai_plane->SetPerformance(ps);
        } else if (!strcmp(entry->getStringValue("class", ""), "jet_transport")) {
          PERF_STRUCT ps = {5.0, 2.0, 3000.0, 1500.0, 140.0, 300.0, 430.0, 300.0, 130.0};
          ai_plane->SetPerformance(ps);
        } else if (!strcmp(entry->getStringValue("class", ""), "jet_fighter")) {
          PERF_STRUCT ps = {7.0, 3.0, 4000.0, 2000.0, 150.0, 350.0, 500.0, 350.0, 150.0};
          ai_plane->SetPerformance(ps);
        }
        ai_plane->setHeading(entry->getDoubleValue("heading"));
        ai_plane->setSpeed(entry->getDoubleValue("speed-KTAS"));
        ai_plane->setPath(entry->getStringValue("path"));
        ai_plane->setAltitude(entry->getDoubleValue("altitude-ft"));
        ai_plane->setLongitude(entry->getDoubleValue("longitude"));
        ai_plane->setLatitude(entry->getDoubleValue("latitude"));
        ai_plane->init();

      } else if (!strcmp(entry->getStringValue("type", ""), "ship")) {
        FGAIShip* ai_ship = new FGAIShip;
        ai_list.push_back(ai_ship);
        ai_ship->setHeading(entry->getDoubleValue("heading"));
        ai_ship->setSpeed(entry->getDoubleValue("speed-KTAS"));
        ai_ship->setPath(entry->getStringValue("path"));
        ai_ship->setAltitude(entry->getDoubleValue("altitude-ft"));
        ai_ship->setLongitude(entry->getDoubleValue("longitude"));
        ai_ship->setLatitude(entry->getDoubleValue("latitude"));
        ai_ship->init();

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
      } 
     }
  }

  initDone = true;
}


void FGAIManager::bind() {
}


void FGAIManager::unbind() {
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
