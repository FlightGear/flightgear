// sidstar.cxx - Code to manage departure / arrival procedures
// Written by Durk Talsma, started March 2009.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <stdlib.h>

#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>



#include <Airports/simple.hxx>


#include "sidstar.hxx"

using std::cerr;
using std::endl;

FGSidStar::FGSidStar(FGAirport *ap) {
     id = ap->getId();
     initialized = false;
}


FGSidStar::FGSidStar(const FGSidStar &other) {
     cerr << "TODO" << endl;
     exit(1);
}

void FGSidStar::load(SGPath filename) {
  SGPropertyNode root;
  string runway;
  string name;
  try {
      readProperties(filename.str(), &root);
  } catch (const sg_exception &e) {
      SG_LOG(SG_GENERAL, SG_ALERT,
       "Error reading AI flight plan: " << filename.str());
       // cout << path.str() << endl;
     return;
  }

  SGPropertyNode * node = root.getNode("SIDS");
  FGAIFlightPlan *fp;
  for (int i = 0; i < node->nChildren(); i++) { 
     fp = new FGAIFlightPlan;
     SGPropertyNode * fpl_node = node->getChild(i);
     name   =  fpl_node->getStringValue("name", "END");
     runway =  fpl_node->getStringValue("runway", "27");
     //cerr << "Runway = " << runway << endl;
     fp->setName(name);
     SGPropertyNode * wpts_node = fpl_node->getNode("wpts");
     for (int j = 0; j < wpts_node->nChildren(); j++) { 
          FGAIFlightPlan::waypoint* wpt = new FGAIFlightPlan::waypoint;
          SGPropertyNode * wpt_node = wpts_node->getChild(j);
          //cerr << "Reading waypoint " << j << wpt_node->getStringValue("name", "END") << endl;
          wpt->name      = wpt_node->getStringValue("name", "END");
          wpt->latitude  = wpt_node->getDoubleValue("lat", 0);
          wpt->longitude = wpt_node->getDoubleValue("lon", 0);
          wpt->altitude  = wpt_node->getDoubleValue("alt", 0);
          wpt->speed     = wpt_node->getDoubleValue("ktas", 0);
          wpt->crossat   = wpt_node->getDoubleValue("crossat", -10000);
          wpt->gear_down = wpt_node->getBoolValue("gear-down", false);
          wpt->flaps_down= wpt_node->getBoolValue("flaps-down", false);
          wpt->on_ground = wpt_node->getBoolValue("on-ground", false);
          wpt->time_sec   = wpt_node->getDoubleValue("time-sec", 0);
          wpt->time       = wpt_node->getStringValue("time", "");

          if (wpt->name == "END") wpt->finished = true;
          else wpt->finished = false;

          // 
          fp->addWaypoint( wpt );
     }
     data[runway].push_back(fp);
     //cerr << "Runway = " << runway << endl;
   }


  //wpt_iterator = waypoints.begin();
  //cout << waypoints.size() << " waypoints read." << endl;
}


FGAIFlightPlan *FGSidStar::getBest(string activeRunway, double heading)
{
    //cerr << "Getting best procedure for " << activeRunway << endl;
    for (FlightPlanVecIterator i = data[activeRunway].begin(); i != data[activeRunway].end(); i++) {
        //cerr << (*i)->getName() << endl;
    }
    int size = data[activeRunway].size();
    //cerr << "size is " << size << endl;
    if (size) {
        return data[activeRunway][(rand() % size)];
     } else {
        return 0;
    }
}