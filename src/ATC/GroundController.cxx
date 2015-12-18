// GroundController.hxx - forked from groundnetwork.cxx

// Written by Durk Talsma, started June 2005.
//
// Copyright (C) 2004 Durk Talsma.
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

#include <cmath>
#include <algorithm>
#include <fstream>
#include <map>
#include <boost/foreach.hpp>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Shape>

#include <simgear/debug/logstream.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/runways.hxx>
#include <Airports/groundnetwork.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <AIModel/AIAircraft.hxx>
#include <AIModel/performancedata.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <Navaids/NavDataCache.hxx>

#include <ATC/atc_mgr.hxx>

#include <Scenery/scenery.hxx>

#include "GroundController.hxx"

using std::string;


/***************************************************************************
 * FGGroundController()
 **************************************************************************/

bool compare_trafficrecords(FGTrafficRecord a, FGTrafficRecord b)
{
    return (a.getIntentions().size() < b.getIntentions().size());
}

FGGroundController::FGGroundController() :
    parent(NULL)
{
    hasNetwork = false;
    count = 0;
    currTraffic = activeTraffic.begin();
    group = 0;
    version = 0;
    networkInitialized = false;

}

FGGroundController::~FGGroundController()
{
}

void FGGroundController::init(FGAirportDynamics* aDynamics)
{
    FGATCController::init();

    dynamics = aDynamics;
    parent = dynamics->parent();
    hasNetwork = true;
    networkInitialized = true;
}

void FGGroundController::announcePosition(int id,
                                       FGAIFlightPlan * intendedRoute,
                                       int currentPosition, double lat,
                                       double lon, double heading,
                                       double speed, double alt,
                                       double radius, int leg,
                                       FGAIAircraft * aircraft)
{
    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id alread has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    // Add a new TrafficRecord if no one exsists for this aircraft.
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        FGTrafficRecord rec;
        rec.setId(id);
        rec.setLeg(leg);
        rec.setPositionAndIntentions(currentPosition, intendedRoute);
        rec.setPositionAndHeading(lat, lon, heading, speed, alt);
        rec.setRadius(radius);  // only need to do this when creating the record.
        rec.setAircraft(aircraft);
        if (leg == 2) {
            activeTraffic.push_front(rec);
        } else {
            activeTraffic.push_back(rec);   
        }
        
    } else {
        i->setPositionAndIntentions(currentPosition, intendedRoute);
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
    }
}


void FGGroundController::signOff(int id)
{
    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id alread has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off at " << SG_ORIGIN);
    } else {
        i = activeTraffic.erase(i);
    }
}
/**
 * The ground network can deal with the following states:
 * 0 =  Normal; no action required
 * 1 = "Acknowledge "Hold position
 * 2 = "Acknowledge "Resume taxi".
 * 3 = "Issue TaxiClearance"
 * 4 = Acknowledge Taxi Clearance"
 * 5 = Post acknowlegde taxiclearance: Start taxiing
 * 6 = Report runway
 * 7 = Acknowledge report runway
 * 8 = Switch tower frequency
 * 9 = Acknowledge switch tower frequency
 *************************************************************************************************************************/
bool FGGroundController::checkTransmissionState(int minState, int maxState, TrafficVectorIterator i, time_t now, AtcMsgId msgId,
        AtcMsgDir msgDir)
{
    int state = i->getState();
    if ((state >= minState) && (state <= maxState) && available) {
        if ((msgDir == ATC_AIR_TO_GROUND) && isUserAircraft(i->getAircraft())) {
            //cerr << "Checking state " << state << " for " << i->getAircraft()->getCallSign() << endl;
            SGPropertyNode_ptr trans_num = globals->get_props()->getNode("/sim/atc/transmission-num", true);
            int n = trans_num->getIntValue();
            if (n == 0) {
                trans_num->setIntValue(-1);
                // PopupCallback(n);
                //cerr << "Selected transmission message " << n << endl;
                //FGATCManager *atc = (FGATCManager*) globals->get_subsystem("atc");
                FGATCDialogNew::instance()->removeEntry(1);
            } else {
                //cerr << "creating message for " << i->getAircraft()->getCallSign() << endl;
                transmit(&(*i), dynamics, msgId, msgDir, false);
                return false;
            }
        }
        transmit(&(*i), dynamics, msgId, msgDir, true);
        i->updateState();
        lastTransmission = now;
        available = false;
        return true;
    }
    return false;
}

void FGGroundController::updateAircraftInformation(int id, double lat, double lon,
        double heading, double speed, double alt,
        double dt)
{
    // Check whether aircraft are on hold due to a preceding pushback. If so, make sure to
    // Transmit air-to-ground "Ready to taxi request:
    // Transmit ground to air approval / hold
    // Transmit confirmation ...
    // Probably use a status mechanism similar to the Engine start procedure in the startup controller.


    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    TrafficVectorIterator current, closest;
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    // update position of the current aircraft
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: updating aircraft without traffic record at " << SG_ORIGIN);
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
        current = i;
    }

    setDt(getDt() + dt);

    // Update every three secs, but add some randomness
    // to prevent all IA objects doing this in synchrony
    //if (getDt() < (3.0) + (rand() % 10))
    //  return;
    //else
    //  setDt(0);
    current->clearResolveCircularWait();
    current->setWaitsForId(0);
    checkSpeedAdjustment(id, lat, lon, heading, speed, alt);
    bool needsTaxiClearance = current->getAircraft()->getTaxiClearanceRequest();
    if (!needsTaxiClearance) {
        checkHoldPosition(id, lat, lon, heading, speed, alt);
        //if (checkForCircularWaits(id)) {
        //    i->setResolveCircularWait();
        //}
    } else {
        current->setHoldPosition(true);
        int state = current->getState();
        time_t now = time(NULL) + fgGetLong("/sim/time/warp");
        if ((now - lastTransmission) > 15) {
            available = true;
        }
        if (checkTransmissionState(0,2, current, now, MSG_REQUEST_TAXI_CLEARANCE, ATC_AIR_TO_GROUND)) {
            current->setState(3);
        }
        if (checkTransmissionState(3,3, current, now, MSG_ISSUE_TAXI_CLEARANCE, ATC_GROUND_TO_AIR)) {
            current->setState(4);
        }
        if (checkTransmissionState(4,4, current, now, MSG_ACKNOWLEDGE_TAXI_CLEARANCE, ATC_AIR_TO_GROUND)) {
            current->setState(5);
        }
        if ((state == 5) && available) {
            current->setState(0);
            current->getAircraft()->setTaxiClearanceRequest(false);
            current->setHoldPosition(false);
            available = false;
        }

    }
}

/**
   Scan for a speed adjustment change. Find the nearest aircraft that is in front
   and adjust speed when we get too close. Only do this when current position and/or
   intentions of the current aircraft match current taxiroute position of the proximate
   aircraft. For traffic that is on other routes we need to issue a "HOLD Position"
   instruction. See below for the hold position instruction.

   Note that there currently still is one flaw in the logic that needs to be addressed.
   There can be situations where one aircraft is in front of the current aircraft, on a separate
   route, but really close after an intersection coming off the current route. This
   aircraft is still close enough to block the current aircraft. This situation is currently
   not addressed yet, but should be.
*/

void FGGroundController::checkSpeedAdjustment(int id, double lat,
        double lon, double heading,
        double speed, double alt)
{

    TrafficVectorIterator current, closest, closestOnNetwork;
    TrafficVectorIterator i = activeTraffic.begin();
    bool otherReasonToSlowDown = false;
//    bool previousInstruction;
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && (i != activeTraffic.end()))
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    } else {
        return;
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkSpeedAdjustment at " << SG_ORIGIN);
    }
    current = i;
    //closest = current;

//    previousInstruction = current->getSpeedAdjustment();
    double mindist = HUGE_VAL;
    if (activeTraffic.size()) {
        double course, dist, bearing, az2; // minbearing,
        SGGeod curr(SGGeod::fromDegM(lon, lat, alt));
        //TrafficVector iterator closest;
        closest = current;
        closestOnNetwork = current;
        for (TrafficVectorIterator i = activeTraffic.begin();
                i != activeTraffic.end(); i++) {
            if (i == current) {
                continue;
            }

            SGGeod other(SGGeod::fromDegM(i->getLongitude(),
                                          i->getLatitude(),
                                          i->getAltitude()));
            SGGeodesy::inverse(curr, other, course, az2, dist);
            bearing = fabs(heading - course);
            if (bearing > 180)
                bearing = 360 - bearing;
            if ((dist < mindist) && (bearing < 60.0)) {
                mindist = dist;
                closest = i;
                closestOnNetwork = i;
//                minbearing = bearing;
                
            }
        }
        //Check traffic at the tower controller
        if (towerController->hasActiveTraffic()) {
            for (TrafficVectorIterator i =
                        towerController->getActiveTraffic().begin();
                    i != towerController->getActiveTraffic().end(); i++) {
                //cerr << "Comparing " << current->getId() << " and " << i->getId() << endl;
                SGGeod other(SGGeod::fromDegM(i->getLongitude(),
                                              i->getLatitude(),
                                              i->getAltitude()));
                SGGeodesy::inverse(curr, other, course, az2, dist);
                bearing = fabs(heading - course);
                if (bearing > 180)
                    bearing = 360 - bearing;
                if ((dist < mindist) && (bearing < 60.0)) {
                    //cerr << "Current aircraft " << current->getAircraft()->getTrafficRef()->getCallSign()
                    //     << " is closest to " << i->getAircraft()->getTrafficRef()->getCallSign()
                    //     << ", which has status " << i->getAircraft()->isScheduledForTakeoff()
                    //     << endl;
                    mindist = dist;
                    closest = i;
//                    minbearing = bearing;
                    otherReasonToSlowDown = true;
                }
            }
        }
        // Finally, check UserPosition
        // Note, as of 2011-08-01, this should no longer be necessecary.
        /*
        double userLatitude = fgGetDouble("/position/latitude-deg");
        double userLongitude = fgGetDouble("/position/longitude-deg");
        SGGeod user(SGGeod::fromDeg(userLongitude, userLatitude));
        SGGeodesy::inverse(curr, user, course, az2, dist);

        bearing = fabs(heading - course);
        if (bearing > 180)
            bearing = 360 - bearing;
        if ((dist < mindist) && (bearing < 60.0)) {
            mindist = dist;
            //closest = i;
            minbearing = bearing;
            otherReasonToSlowDown = true;
        }
        */
        current->clearSpeedAdjustment();
        bool needBraking = false;
        if (current->checkPositionAndIntentions(*closest)
                || otherReasonToSlowDown) {
            double maxAllowableDistance =
                (1.1 * current->getRadius()) +
                (1.1 * closest->getRadius());
            if (mindist < 2 * maxAllowableDistance) {
                if (current->getId() == closest->getWaitsForId())
                    return;
                else
                    current->setWaitsForId(closest->getId());
                if (closest->getId() != current->getId()) {
                    current->setSpeedAdjustment(closest->getSpeed() *
                                                (mindist / 100));
                    needBraking = true;
                    
//                     if (
//                         closest->getAircraft()->getTakeOffStatus() &&
//                         (current->getAircraft()->getTrafficRef()->getDepartureAirport() ==  closest->getAircraft()->getTrafficRef()->getDepartureAirport()) &&
//                         (current->getAircraft()->GetFlightPlan()->getRunway() == closest->getAircraft()->GetFlightPlan()->getRunway())
//                     )
//                         current->getAircraft()->scheduleForATCTowerDepartureControl(1);
                } else {
                    current->setSpeedAdjustment(0);     // This can only happen when the user aircraft is the one closest
                }
                if (mindist < maxAllowableDistance) {
                    //double newSpeed = (maxAllowableDistance-mindist);
                    //current->setSpeedAdjustment(newSpeed);
                    //if (mindist < 0.5* maxAllowableDistance)
                    //  {
                    current->setSpeedAdjustment(0);
                    //  }
                }
            }
        }
        if ((closest->getId() == closestOnNetwork->getId()) && (current->getPriority() < closest->getPriority()) && needBraking) {
            swap(current, closest);
        }
    }
}

/**
   Check for "Hold position instruction".
   The hold position should be issued under the following conditions:
   1) For aircraft entering or crossing a runway with active traffic on it, or landing aircraft near it
   2) For taxiing aircraft that use one taxiway in opposite directions
   3) For crossing or merging taxiroutes.
*/

void FGGroundController::checkHoldPosition(int id, double lat,
                                        double lon, double heading,
                                        double speed, double alt)
{
    FGGroundNetwork* network = dynamics->getGroundNetwork();
    TrafficVectorIterator current;
    TrafficVectorIterator i = activeTraffic.begin();
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end())
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    } else {
        return;
    }
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkHoldPosition at " << SG_ORIGIN);
    }
    current = i;
    // 
    if (current->getAircraft()->getTakeOffStatus() == 1) {
        current->setHoldPosition(true);
        return;
    }
    if (current->getAircraft()->getTakeOffStatus() == 2) {
        //cerr << current->getAircraft()->getCallSign() << ". Taxi in position and hold" << endl;
        current->setHoldPosition(false);
        current->clearSpeedAdjustment();
        return;
    }
    bool origStatus = current->hasHoldPosition();
    current->setHoldPosition(false);
    //SGGeod curr(SGGeod::fromDegM(lon, lat, alt));
    int currentRoute = i->getCurrentPosition();
    int nextRoute;
    if (i->getIntentions().size()) {
        nextRoute    = (*(i->getIntentions().begin()));
    } else {
        nextRoute = 0;
    }       
    if (currentRoute > 0) {
        FGTaxiSegment *tx = network->findSegment(currentRoute);
        FGTaxiSegment *nx;
        if (nextRoute) {
            nx = network->findSegment(nextRoute);
        } else {
            nx = tx;
        }
        //if (tx->hasBlock(now) || nx->hasBlock(now) ) {
        //   current->setHoldPosition(true);
        //}
        SGGeod start(SGGeod::fromDeg((i->getLongitude()), (i->getLatitude())));
        SGGeod end  (nx->getStart()->geod());

        double distance = SGGeodesy::distanceM(start, end);
        if (nx->hasBlock(now) && (distance < i->getRadius() * 4)) {
            current->setHoldPosition(true);
        } else {
            intVecIterator ivi = i->getIntentions().begin();
            while (ivi != i->getIntentions().end()) {
                if ((*ivi) > 0) {
                    FGTaxiSegment* seg = network->findSegment((*ivi)-1);
                    distance += seg->getLength();
                    if ((seg->hasBlock(now)) && (distance < i->getRadius() * 4)) {
                        current->setHoldPosition(true);
                        break;
                    }
                }
                ivi++;
            }
        } 
    }
    bool currStatus = current->hasHoldPosition();
    current->setHoldPosition(origStatus);
    // Either a Hold Position or a resume taxi transmission has been issued
    if ((now - lastTransmission) > 2) {
        available = true;
    }
    if (current->getState() == 0) {
        if ((origStatus != currStatus) && available) {
            //cerr << "Issueing hold short instrudtion " << currStatus << " " << available << endl;
            if (currStatus == true) { // No has a hold short instruction
                transmit(&(*current), dynamics, MSG_HOLD_POSITION, ATC_GROUND_TO_AIR, true);
                //cerr << "Transmittin hold short instrudtion " << currStatus << " " << available << endl;
                current->setState(1);
            } else {
                transmit(&(*current), dynamics, MSG_RESUME_TAXI, ATC_GROUND_TO_AIR, true);
                //cerr << "Transmittig resume instrudtion " << currStatus << " " << available << endl;
                current->setState(2);
            }
            lastTransmission = now;
            available = false;
            // Don't act on the changed instruction until the transmission is confirmed
            // So set back to original status
            //cerr << "Current state " << current->getState() << endl;
        }

    }
    // 6 = Report runway
    // 7 = Acknowledge report runway
    // 8 = Switch tower frequency
    //9 = Acknowledge switch tower frequency

    //int state = current->getState();
    if (checkTransmissionState(1,1, current, now, MSG_ACKNOWLEDGE_HOLD_POSITION, ATC_AIR_TO_GROUND)) {
        current->setState(0);
        current->setHoldPosition(true);
    }
    if (checkTransmissionState(2,2, current, now, MSG_ACKNOWLEDGE_RESUME_TAXI, ATC_AIR_TO_GROUND)) {
        current->setState(0);
        current->setHoldPosition(false);
    }
    if (current->getAircraft()->getTakeOffStatus() && (current->getState() == 0)) {
        //cerr << "Scheduling " << current->getAircraft()->getCallSign() << " for hold short" << endl;
        current->setState(6);
    }
    if (checkTransmissionState(6,6, current, now, MSG_REPORT_RUNWAY_HOLD_SHORT, ATC_AIR_TO_GROUND)) {
    }
    if (checkTransmissionState(7,7, current, now, MSG_ACKNOWLEDGE_REPORT_RUNWAY_HOLD_SHORT, ATC_GROUND_TO_AIR)) {
    }
    if (checkTransmissionState(8,8, current, now, MSG_SWITCH_TOWER_FREQUENCY, ATC_GROUND_TO_AIR)) {
    }
    if (checkTransmissionState(9,9, current, now, MSG_ACKNOWLEDGE_SWITCH_TOWER_FREQUENCY, ATC_AIR_TO_GROUND)) {
    }



    //current->setState(0);
}

/**
 * Check whether situations occur where the current aircraft is waiting for itself
 * due to higher order interactions.
 * A 'circular' wait is a situation where a waits for b, b waits for c, and c waits
 * for a. Ideally each aircraft only waits for one other aircraft, so by tracing
 * through this list of waiting aircraft, we can check if we'd eventually end back
 * at the current aircraft.
 *
 * Note that we should consider the situation where we are actually checking aircraft
 * d, which is waiting for aircraft a. d is not part of the loop, but is held back by
 * the looping aircraft. If we don't check for that, this function will get stuck into
 * endless loop.
 */

bool FGGroundController::checkForCircularWaits(int id)
{
    //cerr << "Performing Wait check " << id << endl;
    int target = 0;
    TrafficVectorIterator current, other;
    TrafficVectorIterator i = activeTraffic.begin();
    int trafficSize = activeTraffic.size();
    if (trafficSize) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    } else {
        return false;
    }
    if (i == activeTraffic.end() || (trafficSize == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkForCircularWaits at " << SG_ORIGIN);
    }

    current = i;
    target = current->getWaitsForId();
    //bool printed = false; // Note that this variable is for debugging purposes only.
    int counter = 0;

    if (id == target) {
        //cerr << "aircraft waits for user" << endl;
        return false;
    }


    while ((target > 0) && (target != id) && counter++ < trafficSize) {
        //printed = true;
        TrafficVectorIterator i = activeTraffic.begin();
        if (trafficSize) {
            //while ((i->getId() != id) && i != activeTraffic.end())
            while (i != activeTraffic.end()) {
                if (i->getId() == target) {
                    break;
                }
                i++;
            }
        } else {
            return false;
        }
        if (i == activeTraffic.end() || (trafficSize == 0)) {
            //cerr << "[Waiting for traffic at Runway: DONE] " << endl << endl;;
            // The target id is not found on the current network, which means it's at the tower
            //SG_LOG(SG_GENERAL, SG_ALERT, "AI error: Trying to access non-existing aircraft in FGGroundNetwork::checkForCircularWaits");
            return false;
        }
        other = i;
        target = other->getWaitsForId();

        // actually this trap isn't as impossible as it first seemed:
        // the setWaitsForID(id) is set to current when the aircraft
        // is waiting for the user controlled aircraft.
        //if (current->getId() == other->getId()) {
        //    cerr << "Caught the impossible trap" << endl;
        //    cerr << "Current = " << current->getId() << endl;
        //    cerr << "Other   = " << other  ->getId() << endl;
        //    for (TrafficVectorIterator at = activeTraffic.begin();
        //          at != activeTraffic.end();
        //          at++) {
        //        cerr << "currently active aircraft : " << at->getCallSign() << " with Id " << at->getId() << " waits for " << at->getWaitsForId() << endl;
        //    }
        //    exit(1);
        if (current->getId() == other->getId())
            return false;
        //}
        //cerr << current->getCallSign() << " (" << current->getId()  << ") " << " -> " << other->getCallSign()
        //     << " (" << other->getId()  << "); " << endl;;
        //current = other;
    }






    //if (printed)
    //   cerr << "[done] " << endl << endl;;
    if (id == target) {
        SG_LOG(SG_GENERAL, SG_WARN,
               "Detected circular wait condition: Id = " << id <<
               "target = " << target);
        return true;
    } else {
        return false;
    }
}

// Note that this function is probably obsolete...
bool FGGroundController::hasInstruction(int id)
{
    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->hasInstruction();
    }
    return false;
}

FGATCInstruction FGGroundController::getInstruction(int id)
{
    TrafficVectorIterator i = activeTraffic.begin();
    // Search search if the current id has an entry
    // This might be faster using a map instead of a vector, but let's start by taking a safe route
    if (activeTraffic.size()) {
        //while ((i->getId() != id) && i != activeTraffic.end()) {
        while (i != activeTraffic.end()) {
            if (i->getId() == id) {
                break;
            }
            i++;
        }
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_GENERAL, SG_ALERT,
               "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->getInstruction();
    }
    return FGATCInstruction();
}

// Note that this function is copied from simgear. for maintanance purposes, it's probabtl better to make a general function out of that.
static void WorldCoordinate(osg::Matrix& obj_pos, double lat,
                            double lon, double elev, double hdg, double slope)
{
    SGGeod geod = SGGeod::fromDegM(lon, lat, elev);
    obj_pos = makeZUpFrame(geod);
    // hdg is not a compass heading, but a counter-clockwise rotation
    // around the Z axis
    obj_pos.preMult(osg::Matrix::rotate(hdg * SGD_DEGREES_TO_RADIANS,
                                        0.0, 0.0, 1.0));
    obj_pos.preMult(osg::Matrix::rotate(slope * SGD_DEGREES_TO_RADIANS,
                                        0.0, 1.0, 0.0));
}




void FGGroundController::render(bool visible)
{
    SGMaterialLib *matlib = globals->get_matlib();
    FGGroundNetwork* network = dynamics->getGroundNetwork();

    if (group) {
        //int nr = ;
        globals->get_scenery()->get_scene_graph()->removeChild(group);
        //while (group->getNumChildren()) {
        //  cerr << "Number of children: " << group->getNumChildren() << endl;
        //simgear::EffectGeode* geode = (simgear::EffectGeode*) group->getChild(0);
        //osg::MatrixTransform *obj_trans = (osg::MatrixTransform*) group->getChild(0);
        //geode->releaseGLObjects();
        //group->removeChild(geode);
        //delete geode;
        group = 0;
    }
    if (visible) {
        group = new osg::Group;
        FGScenery * local_scenery = globals->get_scenery();
        // double elevation_meters = 0.0;
//        double elevation_feet = 0.0;
        time_t now = time(NULL) + fgGetLong("/sim/time/warp");
        //for ( FGTaxiSegmentVectorIterator i = segments.begin(); i != segments.end(); i++) {
        //double dx = 0;

        for   (TrafficVectorIterator i = activeTraffic.begin(); i != activeTraffic.end(); i++) {
            // Handle start point i.e. the segment that is connected to the aircraft itself on the starting end
            // and to the the first "real" taxi segment on the other end. 
            const int pos = i->getCurrentPosition() - 1;
            if (pos >= 0) {
                FGTaxiSegment* segment = network->findSegment(pos);
                SGGeod start(SGGeod::fromDeg((i->getLongitude()), (i->getLatitude())));
                SGGeod end  (segment->getEnd()->geod());

                double length = SGGeodesy::distanceM(start, end);
                //heading = SGGeodesy::headingDeg(start->geod(), end->geod());

                double az2, heading; //, distanceM;
                SGGeodesy::inverse(start, end, heading, az2, length);
                double coveredDistance = length * 0.5;
                SGGeod center;
                SGGeodesy::direct(start, heading, coveredDistance, center, az2);
                //std::cerr << "Active Aircraft : Centerpoint = (" << center.getLatitudeDeg() << ", " << center.getLongitudeDeg() << "). Heading = " << heading << std::endl;
                ///////////////////////////////////////////////////////////////////////////////
                // Make a helper function out of this
                osg::Matrix obj_pos;
                osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
                obj_trans->setDataVariance(osg::Object::STATIC);
                // Experimental: Calculate slope here, based on length, and the individual elevations
                double elevationStart;
                if (isUserAircraft((i)->getAircraft())) {
                    elevationStart = fgGetDouble("/position/ground-elev-m");
                } else {
                    elevationStart = ((i)->getAircraft()->_getAltitude());
                }
                double elevationEnd   = segment->getEnd()->getElevationM();
                //cerr << "Using elevation " << elevationEnd << endl;

                if ((elevationEnd == 0) || (elevationEnd = parent->getElevation())) {
                    SGGeod center2 = end;
                    center2.setElevationM(SG_MAX_ELEVATION_M);
                    if (local_scenery->get_elevation_m( center2, elevationEnd, NULL )) {
//                        elevation_feet = elevationEnd * SG_METER_TO_FEET + 0.5;
                        //elevation_meters += 0.5;
                    }
                    else {
                        elevationEnd = parent->getElevation();
                    }
                    segment->getEnd()->setElevation(elevationEnd);
                }
                double elevationMean  = (elevationStart + elevationEnd) / 2.0;
                double elevDiff       = elevationEnd - elevationStart;

                double slope = atan2(elevDiff, length) * SGD_RADIANS_TO_DEGREES;

                //cerr << "1. Using mean elevation : " << elevationMean << " and " << slope << endl;

                WorldCoordinate( obj_pos, center.getLatitudeDeg(), center.getLongitudeDeg(), elevationMean+ 0.5, -(heading), slope );

                obj_trans->setMatrix( obj_pos );
                //osg::Vec3 center(0, 0, 0)

                float width = length /2.0;
                osg::Vec3 corner(-width, 0, 0.25f);
                osg::Vec3 widthVec(2*width + 1, 0, 0);
                osg::Vec3 heightVec(0, 1, 0);
                osg::Geometry* geometry;
                geometry = osg::createTexturedQuadGeometry(corner, widthVec, heightVec);
                simgear::EffectGeode* geode = new simgear::EffectGeode;
                geode->setName("test");
                geode->addDrawable(geometry);
                //osg::Node *custom_obj;
                SGMaterial *mat;
                if (segment->hasBlock(now)) {
                    mat = matlib->find("UnidirectionalTaperRed", center);
                } else {
                    mat = matlib->find("UnidirectionalTaperGreen", center);
                }
                if (mat)
                    geode->setEffect(mat->get_effect());
                obj_trans->addChild(geode);
                // wire as much of the scene graph together as we can
                //->addChild( obj_trans );
                group->addChild( obj_trans );
                /////////////////////////////////////////////////////////////////////
            } else {
                //std::cerr << "BIG FAT WARNING: current position is here : " << pos << std::endl;
            }
            // Next: Draw the other taxi segments. 
            for (intVecIterator j = (i)->getIntentions().begin(); j != (i)->getIntentions().end(); j++) {
                osg::Matrix obj_pos;
                const int k = (*j)-1;
                if (k >= 0) {
                    osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
                    obj_trans->setDataVariance(osg::Object::STATIC);
                    FGTaxiSegment* segmentK = network->findSegment(k);
                    // Experimental: Calculate slope here, based on length, and the individual elevations
                    double elevationStart = segmentK->getStart()->getElevationM();
                    double elevationEnd   = segmentK->getEnd  ()->getElevationM();
                    if ((elevationStart == 0)  || (elevationStart == parent->getElevation())) {
                        SGGeod center2 = segmentK->getStart()->geod();
                        center2.setElevationM(SG_MAX_ELEVATION_M);
                        if (local_scenery->get_elevation_m( center2, elevationStart, NULL )) {
//                            elevation_feet = elevationStart * SG_METER_TO_FEET + 0.5;
                            //elevation_meters += 0.5;
                        }
                        else {
                            elevationStart = parent->getElevation();
                        }
                        segmentK->getStart()->setElevation(elevationStart);
                    }
                    if ((elevationEnd == 0) || (elevationEnd == parent->getElevation())) {
                        SGGeod center2 = segmentK->getEnd()->geod();
                        center2.setElevationM(SG_MAX_ELEVATION_M);
                        if (local_scenery->get_elevation_m( center2, elevationEnd, NULL )) {
//                            elevation_feet = elevationEnd * SG_METER_TO_FEET + 0.5;
                            //elevation_meters += 0.5;
                        }
                        else {
                            elevationEnd = parent->getElevation();
                        }
                        segmentK->getEnd()->setElevation(elevationEnd);
                    }

                    double elevationMean  = (elevationStart + elevationEnd) / 2.0;
                    double elevDiff       = elevationEnd - elevationStart;
                    double length         = segmentK->getLength();
                    double slope = atan2(elevDiff, length) * SGD_RADIANS_TO_DEGREES;

                    // cerr << "2. Using mean elevation : " << elevationMean << " and " << slope << endl;

                    SGGeod segCenter = segmentK->getCenter();
                    WorldCoordinate( obj_pos, segCenter.getLatitudeDeg(), segCenter.getLongitudeDeg(),
                                     elevationMean+ 0.5, -(segmentK->getHeading()), slope );

                    obj_trans->setMatrix( obj_pos );
                    //osg::Vec3 center(0, 0, 0)

                    float width = segmentK->getLength() /2.0;
                    osg::Vec3 corner(-width, 0, 0.25f);
                    osg::Vec3 widthVec(2*width + 1, 0, 0);
                    osg::Vec3 heightVec(0, 1, 0);
                    osg::Geometry* geometry;
                    geometry = osg::createTexturedQuadGeometry(corner, widthVec, heightVec);
                    simgear::EffectGeode* geode = new simgear::EffectGeode;
                    geode->setName("test");
                    geode->addDrawable(geometry);
                    //osg::Node *custom_obj;
                    SGMaterial *mat;
                    if (segmentK->hasBlock(now)) {
                        mat = matlib->find("UnidirectionalTaperRed", segCenter);
                    } else {
                        mat = matlib->find("UnidirectionalTaperGreen", segCenter);
                    }
                    if (mat)
                        geode->setEffect(mat->get_effect());
                    obj_trans->addChild(geode);
                    // wire as much of the scene graph together as we can
                    //->addChild( obj_trans );
                    group->addChild( obj_trans );
                }
            }
            //dx += 0.1;
        }
        globals->get_scenery()->get_scene_graph()->addChild(group);
    }
}

string FGGroundController::getName() {
    return string(parent->getId() + "-ground");
}

void FGGroundController::update(double dt)
{
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");
    FGGroundNetwork* network = dynamics->getGroundNetwork();
    network->unblockAllSegments(now);
    int priority = 1;

    TrafficVector& startupTraffic(dynamics->getStartupController()->getActiveTraffic());
    TrafficVectorIterator i;

    //sort(activeTraffic.begin(), activeTraffic.end(), compare_trafficrecords);
    // Handle traffic that is under ground control first; this way we'll prevent clutter at the gate areas.
    // Don't allow an aircraft to pushback when a taxiing aircraft is currently using part of the intended route.
    for (i = startupTraffic.begin(); i != startupTraffic.end(); ++i) {
        updateStartupTraffic(i, priority, now);
    }

    for (i = activeTraffic.begin(); i != activeTraffic.end(); i++) {
        updateActiveTraffic(i, priority, now);
    }
}

void FGGroundController::updateStartupTraffic(TrafficVectorIterator i,
                                              int& priority,
                                              time_t now)
{
    if (!i->getAircraft()) {
        SG_LOG(SG_ATC, SG_ALERT, "updateStartupTraffic: missing aircraft");
        return;
    }

    if (!i->getAircraft()->getPerformance()) {
        SG_LOG(SG_ATC, SG_ALERT, "updateStartupTraffic: missing aircraft performance");
        return;
    }

    i->allowPushBack();
    i->setPriority(priority++);
    // in meters per second;
    double vTaxi = (i->getAircraft()->getPerformance()->vTaxi() * SG_NM_TO_METER) / 3600;
    if (!i->isActive(0)) {
        return;
    }

    FGGroundNetwork* network = dynamics->getGroundNetwork();

    if (!network) {
        SG_LOG(SG_ATC, SG_ALERT, "updateStartupTraffic: missing ground network");
        return;
    }

    // Check for all active aircraft whether it's current pos segment is
    // an opposite of one of the departing aircraft's intentions
    for (TrafficVectorIterator j = activeTraffic.begin(); j != activeTraffic.end(); j++) {
        int pos = j->getCurrentPosition();
        if (pos > 0) {
            FGTaxiSegment *seg = network->findOppositeSegment(pos-1);
            if (seg) {
                int posReverse = seg->getIndex();
                for (intVecIterator k = i->getIntentions().begin(); k != i->getIntentions().end(); k++) {
                    if ((*k) == posReverse) {
                        i->denyPushBack();
                        network->findSegment(posReverse-1)->block(i->getId(), now, now);
                    }
                }
            }
        }
    }
    // if the current aircraft is still allowed to pushback, we can start reserving a route for if by blocking all the entry taxiways.
    if (!i->pushBackAllowed()) {
        return;
    }

    double length = 0;
    int pos = i->getCurrentPosition();
    if (pos > 0) {
        FGTaxiSegment *seg = network->findSegment(pos-1);
        length = seg->getLength();
        network->blockSegmentsEndingAt(seg, i->getId(), now, now);
    }
    for (intVecIterator j = i->getIntentions().begin(); j != i->getIntentions().end(); j++) {
        int pos = (*j);
        if (pos > 0) {
            FGTaxiSegment *seg = network->findSegment(pos-1);
            length += seg->getLength();
            time_t blockTime = now + (length / vTaxi);
            network->blockSegmentsEndingAt(seg, i->getId(), blockTime - 30, now);
        }
    }
}

void FGGroundController::updateActiveTraffic(TrafficVectorIterator i,
                                             int& priority,
                                             time_t now)
{
    if (!i->getAircraft()) {
        SG_LOG(SG_ATC, SG_ALERT, "updateActiveTraffic: missing aircraft");
        return;
    }

    if (!i->getAircraft()->getPerformance()) {
        SG_LOG(SG_ATC, SG_ALERT, "updateActiveTraffic: missing aircraft performance");
        return;
    }

    double length = 0;
    double vTaxi = (i->getAircraft()->getPerformance()->vTaxi() * SG_NM_TO_METER) / 3600;
    FGGroundNetwork* network = dynamics->getGroundNetwork();

    if (!network) {
        SG_LOG(SG_ATC, SG_ALERT, "updateActiveTraffic: missing ground network");
        return;
    }

    i->setPriority(priority++);
    int pos = i->getCurrentPosition();
    if (pos > 0) {
        FGTaxiSegment* segment = network->findSegment(pos-1);
        length = segment->getLength();
        if (segment->hasBlock(now)) {
            //SG_LOG(SG_GENERAL, SG_ALERT, "Taxiway incursion for AI aircraft" << i->getAircraft()->getCallSign());
        }

    }
    intVecIterator ivi;
    for (ivi = i->getIntentions().begin(); ivi != i->getIntentions().end(); ivi++) {
        int segIndex = (*ivi);
        if (segIndex > 0) {
            FGTaxiSegment* seg = network->findSegment(segIndex-1);
            if (seg->hasBlock(now)) {
                break;
            }
        }
    }
    //after this, ivi points just behind the last valid unblocked taxi segment.
    for (intVecIterator j = i->getIntentions().begin(); j != ivi; j++) {
        int pos = (*j);
        if (pos > 0) {
            FGTaxiSegment *seg = network->findSegment(pos-1);
            length += seg->getLength();
            time_t blockTime = now + (length / vTaxi);
            network->blockSegmentsEndingAt(seg, i->getId(), blockTime - 30, now);
        }
    }
}
