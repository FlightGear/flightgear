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

#include <Airports/dynamics.hxx>
#include <Airports/airportdynamicsmanager.hxx>
#include <Airports/airport.hxx>
#include <Scenery/scenery.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIManager.hxx>
#include <Traffic/Schedule.hxx>
#include <Traffic/SchedFlight.hxx>
#include <AIModel/AIFlightPlan.hxx>

#include "atc_mgr.hxx"


using std::string;

/**
Constructer, initializes values to private boolean and FGATCController instances
*/
FGATCManager::FGATCManager() :
    controller(NULL),
    prevController(NULL),
    networkVisible(false),
    initSucceeded(false)
{
}

/**
Default destructor
*/
FGATCManager::~FGATCManager() {
}

/**
Sets up ATC subsystem parts depending on other subsystems
Override of SGSubsystem::postinit()
Will set private boolean flag "initSucceeded" to true upon conclusion
*/
void FGATCManager::postinit()
{
    int leg = 0;

    trans_num = globals->get_props()->getNode("/sim/atc/transmission-num", true);

    // Assign a controller to the user's aircraft.
    // Three scenarios are considered:
    //  - Starting on ground at a parking position
    //  - Starting on ground at the runway.
    //  - Starting in the air
    bool onGround  = fgGetBool("/sim/presets/onground");
    string runway  = fgGetString("/sim/atc/runway");
    string curAirport = fgGetString("/sim/presets/airport-id");
    string parking = fgGetString("/sim/presets/parkpos");
    destination = fgGetString("/autopilot/route-manager/destination/airport");
    
    FGAIManager* aiManager = globals->get_subsystem<FGAIManager>();
    auto userAircraft = aiManager->getUserAircraft();
    string callsign = userAircraft->getCallSign();
    
    double aircraftRadius = 40; // note that this is currently hardcoded to a one-size-fits all JumboJet value. Should change later.
    
    // In case a destination is not set yet, make it equal to the current airport
    if (destination == "") {
        destination = curAirport;
    }

    // NEXT UP: Create a traffic schedule and fill that with appropriate information. This we can use for flight planning.
    // Note that these are currently only defaults. 
	userAircraftTrafficRef.reset(new FGAISchedule);
    userAircraftTrafficRef->setFlightType("gate");

	userAircraftScheduledFlight.reset(new FGScheduledFlight);
	userAircraftScheduledFlight->setDepartureAirport(curAirport);
	userAircraftScheduledFlight->setArrivalAirport(destination);
	userAircraftScheduledFlight->initializeAirports();
	userAircraftScheduledFlight->setFlightRules("IFR");
	userAircraftScheduledFlight->setCallSign(callsign);
    
    userAircraftTrafficRef->assign(userAircraftScheduledFlight.get());
    std::unique_ptr<FGAIFlightPlan> fp ;
    userAircraft->setTrafficRef(userAircraftTrafficRef.get());
    
    string flightPlanName = curAirport + "-" + destination + ".xml";
    //double cruiseAlt = 100; // Doesn't really matter right now.
    //double courseToDest = 180; // Just use something neutral; this value might affect the runway that is used though...
    //time_t deptime = 0;        // just make sure how flightplan processing is affected by this...


    FGAirportDynamicsRef dcs(flightgear::AirportDynamicsManager::find(curAirport));
    if (dcs && onGround) {// && !runway.empty()) {

        ParkingAssignment pk;

        if (parking == "AVAILABLE") {
            double radius = fgGetDouble("/sim/dimensions/radius-m");
            if (radius > 0) {
                pk = dcs->getAvailableParking(radius, string(), string(), string());
                if (pk.isValid()) {
                    fgGetString("/sim/presets/parkpos");
                    fgSetString("/sim/presets/parkpos", pk.parking()->getName());
                }
            }
            
            if (!pk.isValid()) {
                FGParkingList pkl(dcs->getParkings(true, "gate"));
                if (!pkl.empty()) {
                    std::sort(pkl.begin(), pkl.end(), [](const FGParkingRef& a, const FGParkingRef& b) {
                        return a->getRadius() > b->getRadius();
                    });
                    pk = ParkingAssignment(pkl.front(), dcs);
                    fgSetString("/sim/presets/parkpos", pkl.front()->getName());
                }
            }
        } else if (!parking.empty()) {
            pk = dcs->getAvailableParkingByName(parking);
        }
      
        if (pk.isValid()) {
            dcs->setParkingAvailable(pk.parking(), false);
            fp.reset(new FGAIFlightPlan);
            controller = dcs->getStartupController();
            int stationFreq = dcs->getGroundFrequency(1);
            if (stationFreq > 0)
            {
                SG_LOG(SG_ATC, SG_DEBUG, "Setting radio frequency to : " << stationFreq);
                fgSetDouble("/instrumentation/comm[0]/frequencies/selected-mhz", ((double) stationFreq / 100.0));
            }
            leg = 1;
            //double, lat, lon, head; // Unused variables;
            //int getId = apt->getDynamics()->getParking(gateId, &lat, &lon, &head);
            aircraftRadius = pk.parking()->getRadius();
            string fltType = pk.parking()->getType(); // gate / ramp, ga, etc etc.
            string aircraftType; // Unused.
            string airline;      // Currently used for gate selection, but a fallback mechanism will apply when not specified.
            fp->setGate(pk);
            if (!(fp->createPushBack(userAircraft,
                                     false,
                                     dcs->parent(),
                                     aircraftRadius,
                                     fltType,
                                     aircraftType,
                                     airline))) {
                controller = 0;
                return;
            }

            
            
        } else if (!runway.empty()) {
            // on a runway

            controller = dcs->getTowerController();
            int stationFreq = dcs->getTowerFrequency(2);
            if (stationFreq > 0)
            {
                SG_LOG(SG_ATC, SG_DEBUG, "Setting radio frequency to inair frequency : " << stationFreq);
                fgSetDouble("/instrumentation/comm[0]/frequencies/selected-mhz", ((double) stationFreq / 100.0));
            }
            fp.reset(new FGAIFlightPlan);
            leg = 3;
            string fltType = "ga";
            fp->setRunway(runway);
            fp->createTakeOff(userAircraft, false, dcs->parent(), 0, fltType);
            userAircraft->setTakeOffStatus(2);
        } else {
            // We're on the ground somewhere. Handle this case later.

            // important : we are on the ground, so reset the AIFlightPlan back to
            // a dummy one. Otherwise, in the reposition case, we end up with a
            // stale flight-plan which confuses other code (eg, PositionInit::finalizeForParking)
            // see unit test: PosInitTests::testRepositionAtOccupied
            fp.reset(FGAIFlightPlan::createDummyUserPlan());
            userAircraft->FGAIBase::setFlightPlan(std::move(fp));
            controller = nullptr;
            
            initSucceeded = true; // should be false?
            return;
        }

        if (fp && !fp->empty()) {
            fp->getLastWaypoint()->setName( fp->getLastWaypoint()->getName() + string("legend"));
        }
     } else {
        controller = 0;
     }

    // Create an initial flightplan and assign it to the ai_ac. We won't use this flightplan, but it is necessary to
    // keep the ATC code happy.
    // note in the reposition case, 'fp' is only the new FlightPlan; if we didn't create one here.
    // we will continue using the existing flight plan (and not restart it, for example)
    if (fp) {
        fp->restart();
        fp->setLeg(leg);
        userAircraft->FGAIBase::setFlightPlan(std::move(fp));
    }
    
    if (controller) {
        FGAIFlightPlan* plan = userAircraft->GetFlightPlan();
        const int routeIndex = (plan && plan->getCurrentWaypoint()) ? plan->getCurrentWaypoint()->getRouteIndex() : 0;
        controller->announcePosition(userAircraft->getID(), plan,
                                     routeIndex,
                                     userAircraft->_getLatitude(),
                                     userAircraft->_getLongitude(),
                                     userAircraft->_getHeading(),
                                     userAircraft->getSpeed(),
                                     userAircraft->getAltitude(),
                                     aircraftRadius, leg, userAircraft);
    }
    initSucceeded = true;
}

/**
Shutdown method
Clears activeStations vector in preparation for clean shutdown
Override of SGSubsystem::shutdown()
*/
void FGATCManager::shutdown()
{
    activeStations.clear();
    userAircraftTrafficRef.reset();
    userAircraftScheduledFlight.reset();
}

void FGATCManager::reposition()
{
    prevController = controller = nullptr;
    
// remove any parking assignment form the user flight-plan, so it's
// available again. postinit() will recompute a new value if required
    FGAIManager* aiManager = globals->get_subsystem<FGAIManager>();
    auto userAircraft = aiManager->getUserAircraft();
    if (userAircraft) {
        if (userAircraft->GetFlightPlan()) {
              auto userAIFP = userAircraft->GetFlightPlan();
              userAIFP->setGate({}); // clear any assignment
          }

        userAircraft->clearATCController();
    }

    postinit(); // critical for position-init logic
}

/**
Adds FGATCController instance to std::vector activeStations.
FGATCController is a basic class for every controller
*/
void FGATCManager::addController(FGATCController *controller) {
    activeStations.push_back(controller);
}

/**
Searches for and removes FGATCController instance from std::vector activeStations
*/
void FGATCManager::removeController(FGATCController *controller)
{
    AtcVecIterator it;
    it = std::find(activeStations.begin(), activeStations.end(), controller);
    if (it != activeStations.end()) {
        activeStations.erase(it);
    }
}

/**
Update the subsystem.
FlightGear invokes this method every time the subsystem should
update its state.

@param time The delta time, in seconds, since the last
update.  On first update, delta time will be 0.
*/
void FGATCManager::update ( double time ) {
    SG_LOG(SG_ATC, SG_BULK, "ATC update code is running at time: " << time);
    
    // Test code: let my virtual co-pilot handle ATC
    FGAIManager* aiManager = globals->get_subsystem<FGAIManager>();
    FGAIAircraft* user_ai_ac = aiManager->getUserAircraft();
    FGAIFlightPlan *fp = user_ai_ac->GetFlightPlan();
    
    // Update destination
    string result = fgGetString("/autopilot/route-manager/destination/airport");
    if (destination != result && result != "") {
        destination = result;
        userAircraftScheduledFlight->setArrivalAirport(destination);
		userAircraftScheduledFlight->initializeAirports();
		userAircraftTrafficRef->clearAllFlights();
		userAircraftTrafficRef->assign(userAircraftScheduledFlight.get());
        
        auto userAircraft = aiManager->getUserAircraft();
        userAircraft->setTrafficRef(userAircraftTrafficRef.get());
    }

    /* test code : find out how the routing develops */
    if (fp) {
        int size = fp->getNrOfWayPoints();
        //SG_LOG(SG_ATC, SG_DEBUG, "Setting pos" << pos << " ");
        //SG_LOG(SG_ATC, SG_DEBUG, "Setting intentions");
        // This indicates that we have run out of waypoints: Im future versions, the
        // user should be able to select a new route, but for now just shut down the
        // system. 
        if (size < 3) {
            return;
        }
#if 0
        // Test code: Print how far we're progressing along the taxi route. 
        SG_LOG(SG_ATC, SG_DEBUG, "Size of waypoint cue " << size);
        for (int i = 0; i < size; i++) {
            int val = fp->getRouteIndex(i);
            SG_LOG(SG_ATC, SG_BULK, fp->getWayPoint(i)->getName() << " ");
            //if ((val) && (val != pos)) {
            //    intentions.push_back(val);
            SG_LOG(SG_ATC, SG_BULK, "[done ]");
            //}
        }
        SG_LOG(SG_ATC, SG_BULK, "[done ]");
#endif
    }
    if (fp) {
        SG_LOG(SG_ATC, SG_DEBUG, "Currently at leg : " << fp->getLeg());
    }
    
    // Call getATCController method; returns what FGATCController presently controls the user aircraft
    // - e.g. FGStartupController
    controller = user_ai_ac->getATCController();

    // Update the ATC dialog
    FGATCDialogNew::instance()->update(time);

    // Controller manager - if controller is set, then will update controller
    if (controller) {
        SG_LOG(SG_ATC, SG_DEBUG, "name of previous waypoint : " << fp->getPreviousWaypoint()->getName());
        SG_LOG(SG_ATC, SG_DEBUG, "Currently under control of " << controller->getName());

        // update aircraft information (simulates transponder)

        controller->updateAircraftInformation(user_ai_ac->getID(),
                                              user_ai_ac->_getLatitude(),
                                              user_ai_ac->_getLongitude(),
                                              user_ai_ac->_getHeading(),
                                              user_ai_ac->getSpeed(),
                                              user_ai_ac->getAltitude(), time);

        //string airport = fgGetString("/sim/presets/airport-id");
        //FGAirport *apt = FGAirport::findByIdent(airport); 
        // AT this stage we should update the flightplan, so that waypoint incrementing is conducted as well as leg loading. 

        // Ground network visibility:
        // a) check to see if the message to toggle visibility was called
        // b) if so, toggle network visibility and reset the transmission
        // c) therafter disable rendering for the old controller (TODO: should this be earlier?)
        // d) and render if enabled for the new controller
        int n = trans_num->getIntValue();

        if (n == 1) {
            SG_LOG(SG_ATC, SG_DEBUG, "Toggling ground network visibility " << networkVisible);
            networkVisible = !networkVisible;
            trans_num->setIntValue(-1);
        }
        
        // stop rendering the old controller's groundnetwork
        if ((controller != prevController) && (prevController)) {
            prevController->render(false);
        }

        // render the path for the present controller if the ground network is set to visible
        controller->render(networkVisible);

        SG_LOG(SG_ATC, SG_BULK, "Adding ground network to the scenegraph::update");

        // reset previous controller for next update() iteration
        prevController = controller;
   }

   // update the active ATC stations
   for (AtcVecIterator atc = activeStations.begin(); atc != activeStations.end(); atc++) {
       (*atc)->update(time);
   }
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGATCManager> registrantFGATCManager(
    SGSubsystemMgr::POST_FDM,
    {{"FGAIManager", SGSubsystemMgr::Dependency::HARD}});
