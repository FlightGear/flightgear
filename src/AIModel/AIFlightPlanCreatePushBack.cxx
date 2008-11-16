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
#include "AIFlightPlan.hxx"
#include <simgear/math/sg_geodesy.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>


void FGAIFlightPlan::createPushBack(bool firstFlight, FGAirport *dep, 
 				    double latitude,
 				    double longitude,
 				    double radius,
 				    const string& fltType,
 				    const string& aircraftType,
 				    const string& airline)
{
    double lat, lon, heading;
    FGTaxiRoute *pushBackRoute;
    if (!(dep->getDynamics()->getGroundNetwork()->exists())) {
	//cerr << "Push Back fallback" << endl;
        createPushBackFallBack(firstFlight, dep, latitude, longitude,
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
                   wpt->speed = -10;
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
             createPushBackFallBack(firstFlight, dep, latitude, longitude,
                                    radius, fltType, aircraftType, airline);
             return;

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
		wpt->speed = -10;
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
              wpt->speed = 15;
              //for (wpt_vector_iterator i = waypoints.begin(); i != waypoints.end(); i++) {
              //    cerr << "Waypoint Name: " << (*i)->name << endl;
              //}
        } else {
           //cerr << "Creating direct forward departure route fragment" << endl;
           double lat2, lon2, az2;
           waypoint *wpt;
           geo_direct_wgs_84 ( 0, lat, lon, heading, 
                               2, &lat2, &lon2, &az2 );
           wpt = new waypoint;
           wpt->name      = "park2";
           wpt->latitude  = lat2;
           wpt->longitude = lon2;
           wpt->altitude  = dep->getElevation();
           wpt->speed     = 10; 
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
           wpt->speed     = 10; 
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
           wpt->speed     = 10; 
           wpt->crossat   = -10000;
           wpt->gear_down = true;
           wpt->flaps_down= true;
           wpt->finished  = false;
           wpt->on_ground = true;
           wpt->routeIndex = (*ts)->getIndex();
           waypoints.push_back(wpt);


        }

    }
}
/*******************************************************************
 * createPushBackFallBack
 * This is the backup function for airports that don't have a 
 * network yet. 
 ******************************************************************/
void FGAIFlightPlan::createPushBackFallBack(bool firstFlight, FGAirport *dep, 
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
  double lat2;
  double lon2;
  double az2;



  dep->getDynamics()->getParking(-1, &lat, &lon, &heading);

  heading += 180.0;
  if (heading > 360)
    heading -= 360;
  waypoint *wpt = new waypoint;
  wpt->name      = "park";
  wpt->latitude  = lat;
  wpt->longitude = lon;
  wpt->altitude  = dep->getElevation();
  wpt->speed     = -10; 
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
  wpt->speed     = -10; 
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
  wpt->speed     = 10; 
  wpt->crossat   = -10000;
  wpt->gear_down = true;
  wpt->flaps_down= true;
  wpt->finished  = false;
  wpt->on_ground = true;
  wpt->routeIndex = 0;
  waypoints.push_back(wpt);
}
