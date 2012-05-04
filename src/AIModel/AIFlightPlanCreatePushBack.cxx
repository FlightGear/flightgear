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

#include <simgear/math/sg_geodesy.hxx>

#include <Airports/simple.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>

#include "AIFlightPlan.hxx"
#include "AIAircraft.hxx"
#include "performancedata.hxx"


// TODO: Use James Turner's createOnGround functions.
bool FGAIFlightPlan::createPushBack(FGAIAircraft *ac,
                                    bool firstFlight, FGAirport *dep,
                                    double latitude,
                                    double longitude,
                                    double radius,
                                    const string& fltType,
                                    const string& aircraftType,
                                    const string& airline)
{
    double lat, lon, heading;
    double vTaxi = ac->getPerformance()->vTaxi();
    double vTaxiBackward = vTaxi * (-2.0/3.0);
    double vTaxiReduced  = vTaxi * (2.0/3.0);
    FGTaxiRoute *pushBackRoute;
    // Active runway can be conditionally set by ATC, so at the start of a new flight, this
    // must be reset.
    activeRunway.clear();

    if (!(dep->getDynamics()->getGroundNetwork()->exists())) {
        //cerr << "Push Back fallback" << endl;
        createPushBackFallBack(ac, firstFlight, dep, latitude, longitude,
                               radius, fltType, aircraftType, airline);
    } else {
        if (firstFlight) {

            if (!(dep->getDynamics()->getAvailableParking(&lat, &lon,
                    &heading, &gateId,
                    radius, fltType,
                    aircraftType, airline))) {
                SG_LOG(SG_AI, SG_WARN, "Warning: Could not find parking for a " <<
                       aircraftType <<
                       " of flight type " << fltType <<
                       " of airline     " << airline <<
                       " at airport     " << dep->getId());
                return false;
                char buffer[10];
                snprintf (buffer, 10, "%d", gateId);
                SGGeod coord = coord.fromDeg(lon, lat);
                //FGTaxiNode *tn = dep->getDynamics()->getGroundNetwork()->findNode(node);
                FGAIWaypoint *wpt = createOnGround(ac, string(buffer), coord, dep->getElevation(), vTaxiBackward);
                wpt->setRouteIndex(-1);
                pushBackWaypoint(wpt);
            }
            //cerr << "Success : GateId = " << gateId << endl;
            SG_LOG(SG_AI, SG_WARN, "Warning: Succesfully found a parking for a " <<
                   aircraftType <<
                   " of flight type " << fltType <<
                   " of airline     " << airline <<
                   " at airport     " << dep->getId());
        } else {
            //cerr << "Push Back follow-up Flight" << endl;
            dep->getDynamics()->getParking(gateId, &lat, &lon, &heading);
        }
        if (gateId < 0) {
            createPushBackFallBack(ac, firstFlight, dep, latitude, longitude,
                                   radius, fltType, aircraftType, airline);
            return true;

        }
        //cerr << "getting parking " << gateId;
        //cerr << " for a " <<
        //                                      aircraftType <<
        //                                      " of flight type " << fltType <<
        //                                      " of airline     " << airline <<
        //                                      " at airport     " << dep->getId() << endl;
        FGParking *parking = dep->getDynamics()->getParking(gateId);
        int pushBackNode = parking->getPushBackPoint();


        pushBackRoute = parking->getPushBackRoute();
        if ((pushBackNode > 0) && (pushBackRoute == 0)) {  // Load the already established route for this gate
            int node, rte;
            FGTaxiRoute route;
            //cerr << "Creating push-back for " << gateId << " (" << parking->getName() << ") using push-back point " << pushBackNode << endl;
            route = dep->getDynamics()->getGroundNetwork()->findShortestRoute(gateId, pushBackNode, false);
            parking->setPushBackRoute(new FGTaxiRoute(route));


            pushBackRoute = parking->getPushBackRoute();
            int size = pushBackRoute->size();
            if (size < 2) {
                SG_LOG(SG_AI, SG_ALERT, "Push back route from gate " << gateId << " has only " << size << " nodes.");
                SG_LOG(SG_AI, SG_ALERT, "Using  " << pushBackNode);
            }
            pushBackRoute->first();
            while (pushBackRoute->next(&node, &rte))
            {
                //FGTaxiNode *tn = apt->getDynamics()->getGroundNetwork()->findSegment(node)->getEnd();
                char buffer[10];
                snprintf (buffer, 10, "%d", node);
                FGTaxiNode *tn = dep->getDynamics()->getGroundNetwork()->findNode(node);
                //ids.pop_back();
                //wpt = new waypoint;
                SGGeod coord = coord.fromDeg(tn->getLongitude(), tn->getLatitude());
                FGAIWaypoint *wpt = createOnGround(ac, string(buffer), coord, dep->getElevation(), vTaxiBackward);

                wpt->setRouteIndex(rte);
                pushBackWaypoint(wpt);
            }
            // some special considerations for the last point:
            waypoints.back()->setName(string("PushBackPoint"));
            waypoints.back()->setSpeed(vTaxi);
            ac->setTaxiClearanceRequest(true);
        } else {  // In case of a push forward departure...
            ac->setTaxiClearanceRequest(false);
            double lat2 = 0.0, lon2 = 0.0, az2 = 0.0;

            //cerr << "Creating final push forward point for gate " << gateId << endl;
            FGTaxiNode *tn = dep->getDynamics()->getGroundNetwork()->findNode(gateId);
            FGTaxiSegmentVectorIterator ts = tn->getBeginRoute();
            FGTaxiSegmentVectorIterator te = tn->getEndRoute();
            // if the starting node equals the ending node, then there aren't any routes for this parking.
            // in cases like these we should flag the gate as being inoperative and return false
            if (ts == te) {
                SG_LOG(SG_AI, SG_ALERT, "Gate " << gateId << "doesn't seem to have routes associated with it.");
                parking->setAvailable(false);
                return false;
            }
            tn = (*ts)->getEnd();
            lastNodeVisited = tn->getIndex();
            if (tn == NULL) {
                SG_LOG(SG_AI, SG_ALERT, "No valid taxinode found");
                exit(1);
            }
            double distance = (*ts)->getLength();
            //cerr << "Length of push forward route = " << distance << " and heading is " << heading << endl;
            lat2 =  tn->getLatitude();
            lon2 =  tn->getLongitude();

            for (int i = 1; i < 10; i++) {
                geo_direct_wgs_84 ( 0, lat, lon, heading,
                                    ((i / 10.0) * distance), &lat2, &lon2, &az2 );
                char buffer[16];
                snprintf(buffer, 16, "pushback-%02d", i);
                SGGeod coord = coord.fromDeg(lon2, lat2);
                //cerr << i << endl;
                FGAIWaypoint *wpt = createOnGround(ac, string(buffer), coord, dep->getElevation(), vTaxiReduced);

                wpt->setRouteIndex((*ts)->getIndex());
                pushBackWaypoint(wpt);
            }
            // cerr << "Done " << endl;
            waypoints.back()->setName(string("PushBackPoint"));
            // cerr << "Done assinging new name" << endl;
        }
    }
    return true;
}
/*******************************************************************
* createPushBackFallBack
* This is the backup function for airports that don't have a
* network yet.
******************************************************************/
void FGAIFlightPlan::createPushBackFallBack(FGAIAircraft *ac, bool firstFlight, FGAirport *dep,
        double latitude,
        double longitude,
        double radius,
        const string& fltType,
        const string& aircraftType,
        const string& airline)
{
    double heading;
    double lat;
    double lon;
    double lat2 = 0.0;
    double lon2 = 0.0;
    double az2 = 0.0;

    double vTaxi = ac->getPerformance()->vTaxi();
    double vTaxiBackward = vTaxi * (-2.0/3.0);
    double vTaxiReduced  = vTaxi * (2.0/3.0);



    dep->getDynamics()->getParking(-1, &lat, &lon, &heading);

    heading += 180.0;
    if (heading > 360)
        heading -= 360;

    SGGeod coord = coord.fromDeg(lon, lat);
    FGAIWaypoint *wpt = createOnGround(ac, string("park"), coord, dep->getElevation(), vTaxiBackward);

    pushBackWaypoint(wpt);

    geo_direct_wgs_84 ( 0, lat, lon, heading,
                        10,
                        &lat2, &lon2, &az2 );
    coord = coord.fromDeg(lon2, lat2);
    wpt = createOnGround(ac, string("park2"), coord, dep->getElevation(), vTaxiBackward);

    pushBackWaypoint(wpt);

    geo_direct_wgs_84 ( 0, lat, lon, heading,
                        2.2*radius,
                        &lat2, &lon2, &az2 );
    coord = coord.fromDeg(lon2, lat2);
    wpt = createOnGround(ac, string("taxiStart"), coord, dep->getElevation(), vTaxiReduced);
    pushBackWaypoint(wpt);

}
