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
#include <Airports/groundnetwork.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>

#include "AIFlightPlan.hxx"
#include "AIAircraft.hxx"
#include "performancedata.hxx"

using std::string;

// TODO: Use James Turner's createOnGround functions.
bool FGAIFlightPlan::createPushBack(FGAIAircraft *ac,
                                    bool firstFlight, 
                                    FGAirport *dep,
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

    if (!(dep->getDynamics()->getGroundController()->exists())) {
        //cerr << "Push Back fallback" << endl;
        SG_LOG(SG_AI, SG_DEV_WARN, "No groundcontroller createPushBackFallBack at " << dep->getId());
        createPushBackFallBack(ac, firstFlight, dep,
                               radius, fltType, aircraftType, airline);
      return true;
    }

    if (firstFlight || !dep->getDynamics()->hasParking(gate.parking())) {
      // establish the parking position / gate
      // if the airport has no parking positions defined, don't log
      // the warning below.
      if (!dep->getDynamics()->hasParkings()) {
          return false;
      }
      gate = dep->getDynamics()->getAvailableParking(radius, fltType,
                                                        aircraftType, airline);
      if (!gate.isValid()) {
        SG_LOG(SG_AI, SG_DEV_WARN, "Could not find parking for a " <<
                aircraftType <<
                " of flight type " << fltType <<
                " of airline     " << airline <<
                " at airport     " << dep->getId());
        return false;
      }
    }


    if (!gate.isValid()) {
        SG_LOG(SG_AI, SG_DEV_WARN, "Gate " << gate.parking()->ident() << " not valid createPushBackFallBack at " << dep->getId());
        createPushBackFallBack(ac, firstFlight, dep,
                               radius, fltType, aircraftType, airline);
        return true;

    }

    FGGroundNetwork* groundNet = dep->groundNetwork();
    FGParking *parking = gate.parking();
    if (parking && parking->getPushBackPoint() != nullptr) {
        FGTaxiRoute route = groundNet->findShortestRoute(parking, parking->getPushBackPoint(), false);
        SG_LOG(SG_AI, SG_BULK, "Creating Pushback from " << parking->ident() << " to " << parking->getPushBackPoint()->getIndex());

        int size = route.size();
        if (size < 2) {
            SG_LOG(SG_AI, SG_DEV_WARN, "Push back route from gate " << parking->ident() << " has only " << size << " nodes.\n" << "Using  " << parking->getPushBackPoint());
        }

        route.first();
        FGTaxiNodeRef node;
        int rte;

        if (waypoints.size()>0) {
          // This will be a parking from a previous leg which still contains the forward speed
          waypoints.back()->setSpeed(vTaxiBackward);
        }
                

        while (route.next(node, &rte))
        {
            char buffer[20];
            sprintf (buffer, "pushback-%03d",  (short)node->getIndex());
            FGAIWaypoint *wpt = createOnGround(ac, string(buffer), node->geod(), dep->getElevation(), vTaxiBackward);

            /*
            if (previous) {
              FGTaxiSegment* segment = groundNet->findSegment(previous, node);
              wpt->setRouteIndex(segment->getIndex());
            } else {
              // not on the route yet, make up a unique segment ID
              int x = (int) tn->guid();
              wpt->setRouteIndex(x);
            }*/

            wpt->setRouteIndex(rte);
            pushBackWaypoint(wpt);
            //previous = node;
        }
        // some special considerations for the last point:
        // This will trigger the release of parking
        waypoints.back()->setName(string("PushBackPoint"));
        waypoints.back()->setSpeed(vTaxi);
        ac->setTaxiClearanceRequest(true);
    } else {  // In case of a push forward departure...
        ac->setTaxiClearanceRequest(false);
        double az2 = 0.0;

        FGTaxiSegment* pushForwardSegment = dep->groundNetwork()->findSegmentByHeading(parking, parking->getHeading()); 

        if (!pushForwardSegment) {
            // there aren't any routes for this parking, so create a simple segment straight ahead for 2 meters based on the parking heading
            SG_LOG(SG_AI, SG_DEV_WARN, "Gate " << parking->ident() << " at " << dep->getId()
                 << " doesn't seem to have pushforward routes associated with it.");

            FGAIWaypoint *wpt = createOnGround(ac, string("park"), parking->geod(), dep->getElevation(), vTaxiReduced);
            pushBackWaypoint(wpt);

            SGGeod coord;
            SGGeodesy::direct(parking->geod(), parking->getHeading(), 2.0, coord, az2);
            wpt = createOnGround(ac, string("taxiStart"), coord, dep->getElevation(), vTaxiReduced);
            pushBackWaypoint(wpt);
            return true;
        }

        lastNodeVisited = pushForwardSegment->getEnd();
        double distance = pushForwardSegment->getLength();        

        double parkingHeading = parking->getHeading();

        SG_LOG(SG_AI, SG_BULK, "Creating Pushforward from ID " << pushForwardSegment->getEnd()->getIndex() << " Length : \t" << distance);
// Add the parking if on first leg and not repeat
        if (waypoints.size() == 0) {
          pushBackWaypoint( createOnGround(ac, parking->getName(), parking->geod(), dep->getElevation(), vTaxiReduced));
        }
// Make sure we have at least three WPs         
        int numSegments = distance>15?(distance/5.0):3;
        for (int i = 1; i < numSegments; i++) {
            SGGeod pushForwardPt;

            SGGeodesy::direct(parking->geod(), parkingHeading,
                              (((double)i / numSegments) * distance), pushForwardPt, az2);
            char buffer[20];
            sprintf(buffer, "pushforward-%03d", (short)i);
            FGAIWaypoint *wpt = createOnGround(ac, string(buffer), pushForwardPt, dep->getElevation(), vTaxiReduced);

            wpt->setRouteIndex(pushForwardSegment->getIndex());
            pushBackWaypoint(wpt);
        }

        // This will trigger the release of parking
        waypoints.back()->setName(string("PushBackPoint-pushforward"));
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
