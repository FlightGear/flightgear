/******************************************************************************
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
 	            SG_LOG(SG_INPUT, SG_WARN, "Warning: Could not find parking for a " << 
                                              aircraftType <<
                                              " of flight type " << fltType << 
                                              " of airline     " << airline <<
                                              " at airport     " << dep->getId());
                    return false;
                    char buffer[10];
                    snprintf (buffer, 10, "%d", gateId);
                    //FGTaxiNode *tn = dep->getDynamics()->getGroundNetwork()->findNode(node);
                    waypoint *wpt;
                    wpt = new waypoint;
                    wpt->name      = string(buffer); // fixme: should be the name of the taxiway
                    wpt->latitude  = lat;
                    wpt->longitude = lon;
                    // Elevation is currently disregarded when on_ground is true
                    // because the AIModel obtains a periodic ground elevation estimate.
                   wpt->altitude  = dep->getElevation();
                   wpt->speed = vTaxiBackward;
                   wpt->crossat   = -10000;
                   wpt->gear_down = true;
                   wpt->flaps_down= true;
                   wpt->finished  = false;
                   wpt->on_ground = true;
                   wpt->routeIndex = -1;
                   waypoints.push_back(wpt);
            }
           //cerr << "Success : GateId = " << gateId << endl;
           SG_LOG(SG_INPUT, SG_WARN, "Warning: Succesfully found a parking for a " << 
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
        if ((pushBackNode > 0) && (pushBackRoute == 0)) {
            int node, rte;
            FGTaxiRoute route;
            //cerr << "Creating push-back for " << gateId << " (" << parking->getName() << ") using push-back point " << pushBackNode << endl;
            route = dep->getDynamics()->getGroundNetwork()->findShortestRoute(gateId, pushBackNode, false);
            parking->setPushBackRoute(new FGTaxiRoute(route));
            

            pushBackRoute = parking->getPushBackRoute();
            int size = pushBackRoute->size();
            if (size < 2) {
		SG_LOG(SG_GENERAL, SG_WARN, "Push back route from gate " << gateId << " has only " << size << " nodes.");
                SG_LOG(SG_GENERAL, SG_WARN, "Using  " << pushBackNode);
            }
            pushBackRoute->first();
            waypoint *wpt;
 	    while(pushBackRoute->next(&node, &rte))
	      {
		//FGTaxiNode *tn = apt->getDynamics()->getGroundNetwork()->findSegment(node)->getEnd();
		char buffer[10];
		snprintf (buffer, 10, "%d", node);
		FGTaxiNode *tn = dep->getDynamics()->getGroundNetwork()->findNode(node);
		//ids.pop_back();  
		wpt = new waypoint;
		wpt->name      = string(buffer); // fixme: should be the name of the taxiway
		wpt->latitude  = tn->getLatitude();
		wpt->longitude = tn->getLongitude();
		// Elevation is currently disregarded when on_ground is true
		// because the AIModel obtains a periodic ground elevation estimate.
		wpt->altitude  = dep->getElevation();
		wpt->speed = vTaxiBackward;
		wpt->crossat   = -10000;
		wpt->gear_down = true;
		wpt->flaps_down= true;
		wpt->finished  = false;
		wpt->on_ground = true;
		wpt->routeIndex = rte;
		waypoints.push_back(wpt);
	      }
              // some special considerations for the last point:
              wpt->name = string("PushBackPoint");
              wpt->speed = vTaxi;
              //for (wpt_vector_iterator i = waypoints.begin(); i != waypoints.end(); i++) {
              //    cerr << "Waypoint Name: " << (*i)->name << endl;
              //}
        } else {
           /*
           string rwyClass = getRunwayClassFromTrafficType(fltType);

           // Only set this if it hasn't been set by ATC already.
          if (activeRunway.empty()) {
               //cerr << "Getting runway for " << ac->getTrafficRef()->getCallSign() << " at " << apt->getId() << endl;
              double depHeading = ac->getTrafficRef()->getCourse();
             dep->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway,
                                                 depHeading);
           }
           rwy = dep->getRunwayByIdent(activeRunway);
           SGGeod runwayTakeoff = rwy->pointOnCenterline(5.0);

          FGGroundNetwork *gn = dep->getDynamics()->getGroundNetwork();
          if (!gn->exists()) {
              createDefaultTakeoffTaxi(ac, dep, rwy);
              return true;
          }
          int runwayId = gn->findNearestNode(runwayTakeoff);
          int node = 0;
           // Find out which node to start from
          FGParking *park = dep->getDynamics()->getParking(gateId);
        if (park) {
            node = park->getPushBackPoint();
        }

        if (node == -1) {
            node = gateId;
        }
        // HAndle case where parking doens't have a node
        if ((node == 0) && park) {
            if (firstFlight) {
                node = gateId;
            } else {
                node = gateId;
            }
        }
        //delete taxiRoute;
        //taxiRoute = new FGTaxiRoute;
        FGTaxiRoute tr = gn->findShortestRoute(node, runwayId);
        int route;
        FGTaxiNode *tn;
        waypoint *wpt;
        int nr = 0;
        cerr << "Creating taxiroute from gate: " << gateId << " at " << dep->getId() << endl;
        while (tr.next(&node, &route) && (nr++ < 3)) {
            char buffer[10];
            snprintf(buffer, 10, "%d", node);
             tn = dep->getDynamics()->getGroundNetwork()->findNode(node);
             wpt = createOnGround(ac, buffer, tn->getGeod(), dep->getElevation(),
                               vTaxiReduced);
            wpt->routeIndex = route;
            waypoints.push_back(wpt);
        }
        wpt->name      = "PushBackPoint";
        lastNodeVisited = tn->getIndex();
           //FGTaxiNode *firstNode = findNode(gateId);
           //FGTaxiNode *lastNode =  findNode(runwayId);
           //cerr << "Creating direct forward departure route fragment" << endl;
           */
           double lat2 = 0.0, lon2 = 0.0, az2 = 0.0;
           waypoint *wpt;
           geo_direct_wgs_84 ( 0, lat, lon, heading, 
                               2, &lat2, &lon2, &az2 );
           wpt = new waypoint;
           wpt->name      = "park2";
           wpt->latitude  = lat2;
           wpt->longitude = lon2;
           wpt->altitude  = dep->getElevation();
           wpt->speed     = vTaxiReduced; 
           wpt->crossat   = -10000;
           wpt->gear_down = true;
           wpt->flaps_down= true;
           wpt->finished  = false;
           wpt->on_ground = true;
           wpt->routeIndex = 0;
           waypoints.push_back(wpt); 

           geo_direct_wgs_84 ( 0, lat, lon, heading, 
                               4, &lat2, &lon2, &az2 );
           wpt = new waypoint;
           wpt->name      = "name";
           wpt->latitude  = lat2;
           wpt->longitude = lon2;
           wpt->altitude  = dep->getElevation();
           wpt->speed     = vTaxiReduced; 
           wpt->crossat   = -10000;
           wpt->gear_down = true;
           wpt->flaps_down= true;
           wpt->finished  = false;
           wpt->on_ground = true;
           wpt->routeIndex = 0;
           waypoints.push_back(wpt);

           //cerr << "Creating final push forward point for gate " << gateId << endl;
	   FGTaxiNode *tn = dep->getDynamics()->getGroundNetwork()->findNode(gateId);
	   FGTaxiSegmentVectorIterator ts = tn->getBeginRoute();
	   FGTaxiSegmentVectorIterator te = tn->getEndRoute();
	   if (ts == te) {
               SG_LOG(SG_GENERAL, SG_ALERT, "Gate " << gateId << "doesn't seem to have routes associated with it.");
               //exit(1);
	   }
           tn = (*ts)->getEnd();
           lastNodeVisited = tn->getIndex();
	   if (tn == NULL) {
               SG_LOG(SG_GENERAL, SG_ALERT, "No valid taxinode found");
               exit(1);
           }
           wpt = new waypoint;
           wpt->name      = "PushBackPoint";
           wpt->latitude  = tn->getLatitude();
           wpt->longitude = tn->getLongitude();
           wpt->altitude  = dep->getElevation();
           wpt->speed     = vTaxiReduced; 
           wpt->crossat   = -10000;
           wpt->gear_down = true;
           wpt->flaps_down= true;
           wpt->finished  = false;
           wpt->on_ground = true;
           wpt->routeIndex = (*ts)->getIndex();
           waypoints.push_back(wpt);
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
  waypoint *wpt = new waypoint;
  wpt->name      = "park";
  wpt->latitude  = lat;
  wpt->longitude = lon;
  wpt->altitude  = dep->getElevation();
  wpt->speed     = vTaxiBackward; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;

  waypoints.push_back(wpt); 

  geo_direct_wgs_84 ( 0, lat, lon, heading, 
 		      10, 
 		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "park2";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = dep->getElevation();
  wpt->speed     = vTaxiBackward; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  wpt->routeIndex = 0;
  waypoints.push_back(wpt); 
  geo_direct_wgs_84 ( 0, lat, lon, heading, 
 		      2.2*radius,           
 		      &lat2, &lon2, &az2 );
  wpt = new waypoint;
  wpt->name      = "taxiStart";
  wpt->latitude  = lat2;
  wpt->longitude = lon2;
  wpt->altitude  = dep->getElevation();
  wpt->speed     = vTaxiReduced; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  wpt->routeIndex = 0;
  waypoints.push_back(wpt);
}
