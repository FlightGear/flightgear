/******************************************************************************
 * atc_mgr.cxx
 * Written by Durk Talsma, started August 1, 2010.
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
 *
 **************************************************************************/


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <iostream>

#include <Airports/dynamics.hxx>
#include <Airports/simple.hxx>
#include <Scenery/scenery.hxx>
#include "atc_mgr.hxx"


FGATCManager::FGATCManager() {
    controller = 0;
    prevController = 0;
    networkVisible = false;
    initSucceeded  = false;
}

FGATCManager::~FGATCManager() {

}

void FGATCManager::init() {
    SGSubsystem::init();

    int leg = 0;

    // find a reasonable controller for our user's aircraft..
    // Let's start by working out the following three scenarios: 
    // Starting on ground at a parking position
    // Starting on ground at the runway.
    // Starting in the Air
    bool onGround  = fgGetBool("/sim/presets/onground");
    string runway  = fgGetString("/sim/atc/runway");
    string airport = fgGetString("/sim/presets/airport-id");
    string parking = fgGetString("/sim/presets/parkpos");
    

    // Create an (invisible) AIAircraft represenation of the current
    // Users, aircraft, that mimicks the user aircraft's behavior.
    string callsign= fgGetString("/sim/multiplay/callsign");
    double longitude = fgGetDouble("/position/longitude-deg");
    double latitude  = fgGetDouble("/position/latitude-deg");
    double altitude  = fgGetDouble("/position/altitude-ft");
    double heading   = fgGetDouble("/orientation/heading-deg");
    double speed     = fgGetDouble("/velocities/groundspeed-kt");
    double aircraftRadius = 40; // note that this is currently hardcoded to a one-size-fits all JumboJet value. Should change later;


    ai_ac.setCallSign ( callsign  );
    ai_ac.setLongitude( longitude );
    ai_ac.setLatitude ( latitude  );
    ai_ac.setAltitude ( altitude  );
    ai_ac.setPerformance("jet_transport");

    // NEXT UP: Create a traffic Schedule and fill that with appropriate information. This we can use to flight planning.
    // Note that these are currently only defaults. 
    FGAISchedule *trafficRef = new FGAISchedule;
    trafficRef->setFlightType("gate");

    FGScheduledFlight *flight =  new FGScheduledFlight;
    flight->setDepartureAirport(airport);
    flight->setArrivalAirport(airport);
    flight->initializeAirports();
    flight->setFlightRules("IFR");
    flight->setCallSign(callsign);
    
    trafficRef->assign(flight);
    FGAIFlightPlan *fp = 0; 
    ai_ac.setTrafficRef(trafficRef);
    
    string flightPlanName = airport + "-" + airport + ".xml";
    //double cruiseAlt = 100; // Doesn't really matter right now.
    //double courseToDest = 180; // Just use something neutral; this value might affect the runway that is used though...
    //time_t deptime = 0;        // just make sure how flightplan processing is affected by this...


    FGAirport *apt = FGAirport::findByIdent(airport); 
    if (apt && onGround) {// && !runway.empty()) {
        FGAirportDynamics* dcs = apt->getDynamics();
        int park_index = dcs->getNrOfParkings() - 1;
        //cerr << "found information: " << runway << " " << airport << ": parking = " << parking << endl;
        fp = new FGAIFlightPlan;
        while (park_index >= 0 && dcs->getParkingName(park_index) != parking) park_index--;
        // No valid parking location, so either at the runway or at a random location.
        if (parking.empty() || (park_index < 0)) {
            if (!runway.empty()) {
                controller = apt->getDynamics()->getTowerController();
                int stationFreq = apt->getDynamics()->getTowerFrequency(2);
                if (stationFreq > 0)
                {
                    //cerr << "Setting radio frequency to in airfrequency: " << stationFreq << endl;
                    fgSetDouble("/instrumentation/comm[0]/frequencies/selected-mhz", ((double) stationFreq / 100.0));
                }
                leg = 3;
                string fltType = "ga";
                fp->setRunway(runway);
                fp->createTakeOff(&ai_ac, false, apt, 0, fltType);
                ai_ac.setTakeOffStatus(2);
            } else {
                // We're on the ground somewhere. Handle this case later.
            }
        } else {
            controller = apt->getDynamics()->getStartupController();
            int stationFreq = apt->getDynamics()->getGroundFrequency(1);
            if (stationFreq > 0)
            {
                //cerr << "Setting radio frequency to : " << stationFreq << endl;
                fgSetDouble("/instrumentation/comm[0]/frequencies/selected-mhz", ((double) stationFreq / 100.0));
            }
            leg = 1;
            //double, lat, lon, head; // Unused variables;
            //int getId = apt->getDynamics()->getParking(gateId, &lat, &lon, &head);
            FGParking* parking = dcs->getParking(park_index);
            aircraftRadius = parking->getRadius();
            string fltType = parking->getType(); // gate / ramp, ga, etc etc. 
            string aircraftType; // Unused.
            string airline;      // Currently used for gate selection, but a fallback mechanism will apply when not specified.
            fp->setGate(park_index);
            if (!(fp->createPushBack(&ai_ac,
                               false, 
                               apt, 
                               latitude,
                               longitude,
                               aircraftRadius,
                               fltType,
                               aircraftType,
                               airline))) {
                controller = 0;
                return;
            }

        }
        fp->getLastWaypoint()->setName( fp->getLastWaypoint()->getName() + string("legend")); 
     } else {
        controller = 0;
     }

    // Create an initial flightplan and assign it to the ai_ac. We won't use this flightplan, but it is necessary to
    // keep the ATC code happy. 
    if (fp) {
        fp->restart();
        fp->setLeg(leg);
        ai_ac.SetFlightPlan(fp);
    }
    if (controller) {
        controller->announcePosition(ai_ac.getID(), fp, fp->getCurrentWaypoint()->getRouteIndex(),
                                      ai_ac._getLatitude(), ai_ac._getLongitude(), heading, speed, altitude,
                                      aircraftRadius, leg, &ai_ac);

    //dialog.init();

   //osg::Node* node = apt->getDynamics()->getGroundNetwork()->getRenderNode();
   //cerr << "Adding groundnetWork to the scenegraph::init" << endl;
   //globals->get_scenery()->get_scene_graph()->addChild(node);
   }
   initSucceeded = true;
}

void FGATCManager::addController(FGATCController *controller) {
    activeStations.push_back(controller);
}

void FGATCManager::update ( double time ) {
    //cerr << "ATC update code is running at time: " << time << endl;
    // Test code: let my virtual co-pilot handle ATC:
   
    

    FGAIFlightPlan *fp = ai_ac.GetFlightPlan();
        
    /* test code : find out how the routing develops */
    if (fp) {
        int size = fp->getNrOfWayPoints();
        //cerr << "Setting pos" << pos << " ";
        //cerr << "setting intentions " ;
        // This indicates that we have run out of waypoints: Im future versions, the
        // user should be able to select a new route, but for now just shut down the
        // system. 
        if (size < 3) {
            //cerr << "Shutting down the atc_mgr" << endl;
            return;
        }
        //cerr << "Size of waypoint cue " << size << " ";
        //for (int i = 0; i < size; i++) {
        //    int val = fp->getRouteIndex(i);
            //cerr << fp->getWayPoint(i)->getName() << " ";
            //if ((val) && (val != pos)) {
                //intentions.push_back(val);
                //cerr << "[done ] " << endl;
            //}
        //}
        //cerr << "[done ] " << endl;
    }
    if (fp) {
        //cerr << "Currently at leg : " << fp->getLeg() << endl;
    }
    double longitude = fgGetDouble("/position/longitude-deg");
    double latitude  = fgGetDouble("/position/latitude-deg");
    double heading   = fgGetDouble("/orientation/heading-deg");
    double speed     = fgGetDouble("/velocities/groundspeed-kt");
    double altitude  = fgGetDouble("/position/altitude-ft");
    
    /*
    SGGeod me(SGGeod::fromDegM(longitude,
                               latitude,
                               altitude));
    SGGeod wpt1(SGGeod::fromDegM(fp->getWayPoint(1)->getLongitude(), 
                                fp->getWayPoint(1)->getLatitude(),
                                fp->getWayPoint(1)->getAltitude()));
    SGGeod wpt2(SGGeod::fromDegM(fp->getWayPoint(1)->getLongitude(), 
                                fp->getWayPoint(1)->getLatitude(),
                                fp->getWayPoint(1)->getAltitude()));
        
    double course1, az1, dist1;
    double course2, az2, dist2;
    SGGeodesy::inverse(me, wpt1, course1, az1, dist1);
    
    cerr << "Bearing to nearest waypoint : " << course1 << " " << dist1 << ". (course " << course2 << ")." <<  endl;
    */
    ai_ac.setLatitude(latitude);
    ai_ac.setLongitude(longitude);
    ai_ac.setAltitude(altitude);
    ai_ac.setHeading(heading);
    ai_ac.setSpeed(speed);
    ai_ac.update(time);
    controller = ai_ac.getATCController();
    FGATCDialogNew::instance()->update(time);
    if (controller) {
       //cerr << "name of previous waypoint : " << fp->getPreviousWaypoint()->getName() << endl;

        //cerr << "Running FGATCManager::update()" << endl;
        //cerr << "Currently under control of " << controller->getName() << endl;
        controller->updateAircraftInformation(ai_ac.getID(),
                                              latitude,
                                              longitude,
                                              heading,
                                              speed,
                                              altitude, time);
        //string airport = fgGetString("/sim/presets/airport-id");
        //FGAirport *apt = FGAirport::findByIdent(airport); 
        // AT this stage we should update the flightplan, so that waypoint incrementing is conducted as well as leg loading. 
       static SGPropertyNode_ptr trans_num = globals->get_props()->getNode("/sim/atc/transmission-num", true);
            int n = trans_num->getIntValue();
        if (n == 1) {
            //cerr << "Toggling ground network visibility " << networkVisible << endl;
            networkVisible = !networkVisible;
            trans_num->setIntValue(-1);
        }
        if ((controller != prevController) && (prevController)) {
            prevController->render(false);
        }
        controller->render(networkVisible);

        //cerr << "Adding groundnetWork to the scenegraph::update" << endl;
        prevController = controller;
   }
   for (AtcVecIterator atc = activeStations.begin(); atc != activeStations.end(); atc++) {
       (*atc)->update(time);
   }
}
