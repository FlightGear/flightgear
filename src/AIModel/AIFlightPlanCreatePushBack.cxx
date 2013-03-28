/****************************************************************************
* AIFlightPlanCreatePushBack.cxx
* Written by Durk Talsma, started August 1, 2007.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of the
* License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
**************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstdlib>
#include <cstdio>

#include <simgear/math/sg_geodesy.hxx>

#include <Airports/airport.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>

#include "AIFlightPlan.hxx"
#include "AIAircraft.hxx"
#include "performancedata.hxx"

using std::string;

// TODO: Use James Turner's createOnGround functions.
bool FGAIFlightPlan::createPushBack(FGAIAircraft *ac,
                                    bool firstFlight, FGAirport *dep,
                                    double radius,
                                    const string& fltType,
                                    const string& aircraftType,
                                    const string& airline)
{
    double vTaxi = ac->getPerformance()->vTaxi();
    double vTaxiBackward = vTaxi * (-2.0/3.0);
    double vTaxiReduced  = vTaxi * (2.0/3.0);
    // Active runway can be conditionally set by ATC, so at the start of a new flight, this
    // must be reset.
    activeRunway.clear();

    if (!(dep->getDynamics()->getGroundNetwork()->exists())) {
        //cerr << "Push Back fallback" << endl;
        createPushBackFallBack(ac, firstFlight, dep,
                               radius, fltType, aircraftType, airline);
      return true;
    }
  
  // establish the parking position / gate if required
    if (firstFlight) {
      gate = dep->getDynamics()->getAvailableParking(radius, fltType,
                                                       aircraftType, airline);
      if (!gate.isValid()) {
        SG_LOG(SG_AI, SG_WARN, "Warning: Could not find parking for a " <<
               aircraftType <<
               " of flight type " << fltType <<
               " of airline     " << airline <<
               " at airport     " << dep->getId());
        return false;
      }
    }
  
    if (!gate.isValid()) {
        createPushBackFallBack(ac, firstFlight, dep,
                               radius, fltType, aircraftType, airline);
        return true;

    }
  
    FGGroundNetwork* groundNet = dep->getDynamics()->getGroundNetwork();
    FGParking *parking = gate.parking();
    if (parking && parking->getPushBackPoint() > 0) {
        FGTaxiRoute route = groundNet->findShortestRoute(parking->guid(), parking->getPushBackPoint(), false);
      
        int size = route.size();
        if (size < 2) {
            SG_LOG(SG_AI, SG_ALERT, "Push back route from gate " << parking->ident() << " has only " << size << " nodes.");
            SG_LOG(SG_AI, SG_ALERT, "Using  " << parking->getPushBackPoint());
        }
        
        route.first();
        PositionedID node, previous= 0;
      
        while (route.next(&node))
        {
            char buffer[10];
            snprintf (buffer, 10, "%lld",  (long long int) node);
            FGTaxiNode *tn = groundNet->findNode(node);
            FGAIWaypoint *wpt = createOnGround(ac, string(buffer), tn->geod(), dep->getElevation(), vTaxiBackward);
          
            if (previous) {
              FGTaxiSegment* segment = groundNet->findSegment(previous, node);
              wpt->setRouteIndex(segment->getIndex());
            } else {
              // not on the route yet, make up a unique segment ID
              int x = (int) tn->guid();
              wpt->setRouteIndex(x);
            }
          
            pushBackWaypoint(wpt);
            previous = node;
        }
        // some special considerations for the last point:
        waypoints.back()->setName(string("PushBackPoint"));
        waypoints.back()->setSpeed(vTaxi);
        ac->setTaxiClearanceRequest(true);
    } else {  // In case of a push forward departure...
        ac->setTaxiClearanceRequest(false);
        double az2 = 0.0;

      FGTaxiSegment* pushForwardSegment = dep->getDynamics()->getGroundNetwork()->findSegment(parking->guid(), 0);
      // there aren't any routes for this parking.
      if (!pushForwardSegment) {
          SG_LOG(SG_AI, SG_ALERT, "Gate " << parking->ident() << "doesn't seem to have routes associated with it.");
          return false;
      }

      lastNodeVisited = pushForwardSegment->getEnd()->getIndex();
      double distance = pushForwardSegment->getLength();

      double parkingHeading = parking->getHeading();
    
      for (int i = 1; i < 10; i++) {
          SGGeod pushForwardPt;
          SGGeodesy::direct(parking->geod(), parkingHeading,
                            ((i / 10.0) * distance), pushForwardPt, az2);
          char buffer[16];
          snprintf(buffer, 16, "pushback-%02d", i);
          FGAIWaypoint *wpt = createOnGround(ac, string(buffer), pushForwardPt, dep->getElevation(), vTaxiReduced);

          wpt->setRouteIndex(pushForwardSegment->getIndex());
          pushBackWaypoint(wpt);
      }

      waypoints.back()->setName(string("PushBackPoint"));
      // cerr << "Done assinging new name" << endl;
    }

    return true;
}
/*******************************************************************
* createPushBackFallBack
* This is the backup function for airports that don't have a
* network yet.
******************************************************************/
void FGAIFlightPlan::createPushBackFallBack(FGAIAircraft *ac, bool firstFlight, FGAirport *dep,
        double radius,
        const string& fltType,
        const string& aircraftType,
        const string& airline)
{
    double az2 = 0.0;

    double vTaxi = ac->getPerformance()->vTaxi();
    double vTaxiBackward = vTaxi * (-2.0/3.0);
    double vTaxiReduced  = vTaxi * (2.0/3.0);

    double heading = 180.0; // this is a completely arbitrary heading!
    FGAIWaypoint *wpt = createOnGround(ac, string("park"), dep->geod(), dep->getElevation(), vTaxiBackward);

    pushBackWaypoint(wpt);

    SGGeod coord;
    SGGeodesy::direct(dep->geod(), heading, 10, coord, az2);
    wpt = createOnGround(ac, string("park2"), coord, dep->getElevation(), vTaxiBackward);

    pushBackWaypoint(wpt);
  
    SGGeodesy::direct(dep->geod(), heading, 2.2 * radius, coord, az2);
    wpt = createOnGround(ac, string("taxiStart"), coord, dep->getElevation(), vTaxiReduced);
    pushBackWaypoint(wpt);

}
