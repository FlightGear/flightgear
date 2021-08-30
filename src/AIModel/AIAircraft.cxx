// FGAIAircraft - FGAIBase-derived class creates an AI airplane
//
// Written by David Culp, started October 2003.
//
// Copyright (C) 2003  David P. Culp - davidculp2@comcast.net
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/airport.hxx>
#include <Main/util.hxx>
#include <Traffic/Schedule.hxx>

#include <simgear/timing/sg_time.hxx>
#include <simgear/structure/exception.hxx>

#include <string>
#include <cmath>
#include <ctime>

// defined in AIShip.cxx
extern double fgIsFinite(double x);

#include "AIManager.hxx"
#include "AIAircraft.hxx"
#include "AIFlightPlan.hxx"
#include "performancedata.hxx"
#include "performancedb.hxx"
#include <signal.h>
#include <iostream>

using std::string;
using std::cerr;
using std::endl;

//#include <Airports/trafficcontroller.hxx>

FGAIAircraft::FGAIAircraft(FGAISchedule* ref) : /* HOT must be disabled for AI Aircraft,
      * otherwise traffic detection isn't working as expected.*/
                                                FGAIBaseAircraft(),
                                                _performance(nullptr)
{
    trafficRef = ref;
    if (trafficRef) {
        groundOffset = trafficRef->getGroundOffset();
        setCallSign(trafficRef->getCallSign());
    }
    else
        groundOffset = 0;

    fp              = 0;
    controller      = 0;
    prevController  = 0;
    towerController = 0;
    dt_count = 0;
    dt_elev_count = 0;
    use_perf_vs = true;

    no_roll = false;
    tgt_speed = 0;
    speed = 0;
    groundTargetSpeed = 0;
    spinCounter = 0;

    // set heading and altitude locks
    hdg_lock = false;
    alt_lock = false;
    roll = 0;
    headingChangeRate = 0.0;
    headingError = 0;
    minBearing = 360;
    speedFraction =1.0;
    prevSpeed = 0.0;
    prev_dist_to_go = 0.0;

    holdPos = false;
    needsTaxiClearance = false;
    _needsGroundElevation = true;

    PerformanceDB* perfDB = globals->get_subsystem<PerformanceDB>();
    if (perfDB) {
        _performance = perfDB->getDefaultPerformance();
    } else {
        SG_LOG(SG_AI, SG_DEV_ALERT, "no performance DB loaded");
        _performance = PerformanceData::getDefaultData();
    }

    takeOffStatus = 0;
    timeElapsed = 0;

    trackCache.remainingLength = 0;
    trackCache.startWptName = "-";
    
    tcasThreatNode = props->getNode("tcas/threat-level", true);
    tcasRANode = props->getNode("tcas/ra-sense", true);
    _searchOrder = PREFER_AI;
}

void FGAIAircraft::lazyInitControlsNodes()
{
    _controlsLateralModeNode = props->getNode("controls/flight/lateral-mode", true);
    _controlsVerticalModeNode = props->getNode("controls/flight/vertical-mode", true);
    _controlsTargetHeadingNode = props->getNode("controls/flight/target-hdg", true);
    _controlsTargetRollNode = props->getNode("controls/flight/target-roll", true);
    _controlsTargetAltitude = props->getNode("controls/flight/target-alt", true);
    _controlsTargetPitch = props->getNode("controls/flight/target-pitch", true);
    _controlsTargetSpeed = props->getNode("controls/flight/target-spd", true);
}

FGAIAircraft::~FGAIAircraft()
{
    assert(!controller);
    if (controller) {
        // we no longer signOff from controller here, controller should
        // have been cleared using clearATCControllers
        // see FLIGHTGEAR-15 on Sentry
        SG_LOG(SG_AI, SG_ALERT, "Destruction of AIAircraft which was not unbound");
    }
}


void FGAIAircraft::readFromScenario(SGPropertyNode* scFileNode) {
    if (!scFileNode)
        return;

    FGAIBase::readFromScenario(scFileNode);

    setPerformance("", scFileNode->getStringValue("class", "jet_transport"));
    setFlightPlan(scFileNode->getStringValue("flightplan"),
                  scFileNode->getBoolValue("repeat", false));
    setCallSign(scFileNode->getStringValue("callsign"));
}


void FGAIAircraft::bind() {
    FGAIBaseAircraft::bind();

    tie("transponder-id",
        SGRawValueMethods<FGAIAircraft,const char*>(*this,
                &FGAIAircraft::_getTransponderCode));
}

void FGAIAircraft::update(double dt) {
    FGAIBase::update(dt);
    Run(dt);
    Transform();
}

void FGAIAircraft::unbind()
{
    FGAIBase::unbind();
    clearATCController();
}

void FGAIAircraft::setPerformance(const std::string& acType, const std::string& acclass)
{
    PerformanceDB* perfDB = globals->get_subsystem<PerformanceDB>();
    if (perfDB) {
        _performance = perfDB->getDataFor(acType, acclass);
    }

    if (!_performance) {
        SG_LOG(SG_AI, SG_DEV_ALERT, "no AI performance data found for: " << acType << "/" << acclass);
        _performance = PerformanceData::getDefaultData();
    }
}

 void FGAIAircraft::Run(double dt)
{
    // We currently have one situation in which an AIAircraft object is used that is not attached to the
    // AI manager. In this particular case, the AIAircraft is used to shadow the user's aircraft's behavior in the AI world.
    // Since we perhaps don't want a radar entry of our own aircraft, the following conditional should probably be adequate
    // enough
    const bool isUserAircraft = (manager == nullptr);

    bool outOfSight = false,
         flightplanActive = true;

    // user aircraft speed, heading and position are synchronzied in
    // FGAIManager::fetchUserState()
    if (!isUserAircraft) {
        updatePrimaryTargetValues(dt, flightplanActive, outOfSight); // target hdg, alt, speed
        if (outOfSight) {
            return;
        }
    }

     if (!flightplanActive) {
        groundTargetSpeed = 0;
     }

     handleATCRequests(dt); // ATC also has a word to say
     updateSecondaryTargetValues(dt); // target roll, vertical speed, pitch
     updateActualState(dt);

     updateModelProperties(dt);


     if (!isUserAircraft) {
         UpdateRadar(manager);
         invisible = !manager->isVisible(pos);
     }
  }


void FGAIAircraft::AccelTo(double speed) {
    tgt_speed = speed;
    if (!isStationary()) {
        _needsGroundElevation = true;
    }
}


void FGAIAircraft::PitchTo(double angle) {
    tgt_pitch = angle;
    alt_lock = false;
}


void FGAIAircraft::RollTo(double angle) {
    tgt_roll = angle;
    hdg_lock = false;
}


void FGAIAircraft::YawTo(double angle) {
    tgt_yaw = angle;
}


void FGAIAircraft::ClimbTo(double alt_ft ) {
    tgt_altitude_ft = alt_ft;
    alt_lock = true;
}


void FGAIAircraft::TurnTo(double heading) {
    if( fabs(heading) < 0.1 ) {
        SG_LOG(SG_AI, SG_WARN, "Heading reset to zero");
    }
    tgt_heading = heading;
    // SG_LOG(SG_AI, SG_BULK, "Turn tgt_heading to " << tgt_heading);
    hdg_lock = true;
}


double FGAIAircraft::sign(double x) {
    if (x == 0.0)
        return x;
    else
        return x/fabs(x);
}


void FGAIAircraft::setFlightPlan(const std::string& flightplan, bool repeat)
{
    if (flightplan.empty()) {
        // this is the case for Nasal-scripted aircraft
        return;
    }
    
    std::unique_ptr<FGAIFlightPlan> plan(new FGAIFlightPlan(flightplan));
    if (plan->isValidPlan()) {
        plan->setRepeat(repeat);
        FGAIBase::setFlightPlan(std::move(plan));
    } else {
        SG_LOG(SG_AI, SG_WARN, "setFlightPlan: invalid flightplan specified:" << flightplan);
    }
}

void FGAIAircraft::ProcessFlightPlan( double dt, time_t now ) {

    // the one behind you
    FGAIWaypoint* prev = 0;
    // the one ahead
    FGAIWaypoint* curr = 0;
    // the next plus 1
    FGAIWaypoint* next = 0;
    /**The angle of the next turn.*/
    int nextTurnAngle = 0;

    prev = fp->getPreviousWaypoint();
    curr = fp->getCurrentWaypoint();
    next = fp->getNextWaypoint();
    nextTurnAngle = fp->getNextTurnAngle();

    dt_count += dt;

    ///////////////////////////////////////////////////////////////////////////
    // Initialize the flightplan
    //////////////////////////////////////////////////////////////////////////
    if (!prev) {
        handleFirstWaypoint();
        return;
    }                            // end of initialization
    if (!fpExecutable(now)) {
        return;
    }
    dt_count = 0;

    double distanceToDescent;
    if (reachedEndOfCruise(distanceToDescent)) {
        if (!loadNextLeg(distanceToDescent)) {
            setDie(true);
            return;
        }
        prev = fp->getPreviousWaypoint();
        SG_LOG(SG_AI, SG_BULK, "Previous WP \t" << prev->getName());
        curr = fp->getCurrentWaypoint();
        SG_LOG(SG_AI, SG_BULK, "Current WP \t" << curr->getName());
        next = fp->getNextWaypoint();
        if( next ) {
            SG_LOG(SG_AI, SG_BULK, "Next WP \t" << next->getName());
        }
    }
    if (!curr) {
        SG_LOG(SG_AI, SG_WARN, "No current WP" << next->getName());
        return;
    }

    if (!leadPointReached(curr, next, nextTurnAngle)) {
        controlHeading(curr);
        controlSpeed(curr, next);
    } else {
        if (curr->isFinished()) {     //end of the flight plan
            SG_LOG(SG_AI, SG_BULK, "Flightplan ended");            
            if (fp->getRepeat()) {
                fp->restart();
            }
            else {
                setDie(true);
            }
            return;
        }

        if (next) {
            //TODO more intelligent method in AIFlightPlan, no need to send data it already has :-)
            tgt_heading = fp->getBearing(curr, next);
            spinCounter = 0;
            SG_LOG(SG_AI, SG_BULK, "Set tgt_heading to " << tgt_heading);
        }

        //TODO let the fp handle this (loading of next leg)
        fp->IncrementWaypoint( trafficRef != 0 );
        if (((!(fp->getNextWaypoint()))) && (trafficRef != 0) ) {
            if (!loadNextLeg()) {
                setDie(true);
                return;
            }
        }

        prev = fp->getPreviousWaypoint();
        SG_LOG(SG_AI, SG_BULK, "Previous WP \t" << prev->getName() << "\t" << prev->getPos());
        curr = fp->getCurrentWaypoint();
        if (curr) {
          SG_LOG(SG_AI, SG_BULK, "Current WP \t" << curr->getName() << "\t" << curr->getPos());
        }
        next = fp->getNextWaypoint();
        if(next) {
            SG_LOG(SG_AI, SG_BULK, "Next WP \t" << next->getName() << "\t" << next->getPos());
        }

        // Now that we have incremented the waypoints, excute some traffic manager specific code
        if (trafficRef) {
            //TODO isn't this best executed right at the beginning?
            if (! aiTrafficVisible()) {
                setDie(true);
                return;
            }

            if (! handleAirportEndPoints(prev, now)) {
                setDie(true);
                return;
            }

            announcePositionToController();
            if (fp && props) {
                props->getChild("arrival-time-sec", 0, true)->setIntValue(fp->getArrivalTime());                
            }
        }

        if (next && !curr->contains("END") && !curr->contains("PushBackPointlegend")) {
            fp->setLeadDistance(tgt_speed, tgt_heading, curr, next);
        } else {
        // If we are ending in a parking  
            fp->setLeadDistance(1);
        }

        // Calculate a target altitude for any leg in which at least one waypoint is in the air.

        if (prev->getInAir() && curr->getInAir()) {
            // Completely in-air leg, so calculate the target altitude and VS.
            if (curr->getCrossat() > -1000.0) {
                use_perf_vs = false;
                double dist_m       = fp->getDistanceToGo(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr);
                double vert_dist_ft = curr->getCrossat() - altitude_ft;            
                double err_dist     = prev->getCrossat() - altitude_ft;
                tgt_vs = calcVerticalSpeed(vert_dist_ft, dist_m, speed, err_dist);
                tgt_altitude_ft = curr->getCrossat();            
                checkTcas();
            } else {
                use_perf_vs = true;
                tgt_altitude_ft = prev->getCrossat();
            }
        } else if (curr->getInAir()) {
            // Take-off leg (prev is on ground)
            if (curr->getCrossat() > -1000.0) {
                // Altitude restriction
                use_perf_vs = false;
                double dist_m       = fp->getDistanceToGo(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr);
                double vert_dist_ft = curr->getCrossat() - altitude_ft;            
                double err_dist     = - altitude_ft;
                tgt_vs = calcVerticalSpeed(vert_dist_ft, dist_m, speed, err_dist);
                tgt_altitude_ft = curr->getCrossat();
            } else {
                // no cross-at, so assume same as previous
                use_perf_vs = true;
                tgt_altitude_ft = prev->getCrossat();
            }
        } else if (prev->getInAir()) {
            // Landing Leg (curr is on ground).

            // Assume we want to touch down on the point, and not early!
            use_perf_vs = false;
            double dist_m       = fp->getDistanceToGo(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr);
            double vert_dist_ft = curr->getAltitude() - altitude_ft;
            double err_dist     = - altitude_ft;
            tgt_vs = calcVerticalSpeed(vert_dist_ft, dist_m, speed, err_dist);
            tgt_altitude_ft = curr->getAltitude();
        }

        AccelTo(prev->getSpeed());
        hdg_lock = alt_lock = true;
        no_roll = (prev->getOn_ground() && curr->getOn_ground());
    }
}

double FGAIAircraft::calcVerticalSpeed(double vert_ft, double dist_m, double speed, double err)
{
    // err is negative when we passed too high
    double vert_m = vert_ft * SG_FEET_TO_METER;
    double speedMs = (speed * SG_NM_TO_METER) / 3600;
    double vs = 0;
    if (dist_m) {
        vs = ((vert_m) / dist_m) * speedMs;
    }
    // Convert to feet per minute
    vs *= (SG_METER_TO_FEET * 60);
    return vs;
}

void FGAIAircraft::clearATCController()
{
    if (controller) {
        controller->signOff(getID());
    }

    controller = 0;
    prevController = 0;
    towerController = 0;
}

void FGAIAircraft::assertSpeed(double speed)
{
    if ((speed < -50) || (speed > 1000)) {
        cerr << getCallSign() << " " 
            << "Previous waypoint " << fp->getPreviousWaypoint()->getName() << " "
            << "Departure airport " << trafficRef->getDepartureAirport() << " "
            << "Leg " << fp->getLeg() <<  " "
            << "target_speed << " << tgt_speed <<  " "
            << "speedFraction << " << speedFraction << " "
            << "Currecnt speed << " << speed << " "
            << endl;
    }
}



void FGAIAircraft::checkTcas(void)
{
    if (tcasThreatNode && tcasThreatNode->getIntValue()==3)
    {
        const int RASense = tcasRANode->getIntValue();
        if ((RASense>0)&&(tgt_vs<4000)) {
            // upward RA: climb!
            tgt_vs = 4000;
        } else if (RASense<0) {
            // downward RA: descend!
            if (altitude_ft < 1000)
            {
                // too low: level off
                if (tgt_vs>0)
                    tgt_vs = 0;
            }
            else
            {
                if (tgt_vs >- 4000)
                    tgt_vs = -4000;
            }
        }
    }
}

void FGAIAircraft::initializeFlightPlan() {
}

const char * FGAIAircraft::_getTransponderCode() const {
  return transponderCode.c_str();
}

// NOTE: Check whether the new (delayed leg increment code has any effect on this code.
// Probably not, because it should only be executed after we have already passed the leg incrementing waypoint. 

bool FGAIAircraft::loadNextLeg(double distance) {

    int leg;
    if ((leg = fp->getLeg()) == 9) {
        if (!trafficRef->next()) {
            //FIXME I'm on leg 9 and don't even reach parking.
            return false;
        }
        setCallSign(trafficRef->getCallSign());
        leg = 0;
        fp->setLeg(leg);
    }

    FGAirport *dep = trafficRef->getDepartureAirport();
    FGAirport *arr = trafficRef->getArrivalAirport();
    if (!(dep && arr)) {
        setDie(true);

    } else {
        double cruiseAlt = trafficRef->getCruiseAlt() * 100;

        bool ok = fp->create (this,
                    dep,
                    arr,
                    leg+1,
                    cruiseAlt,
                    trafficRef->getSpeed(),
                    _getLatitude(),
                    _getLongitude(),
                    false,
                    trafficRef->getRadius(),
                    trafficRef->getFlightType(),
                    acType,
                    company,
                    distance);

        if (!ok) {
            SG_LOG(SG_AI, SG_WARN, "Failed to create waypoints for leg:" << leg+1);
        }
    }
    return true;
}


// Note: This code is copied from David Luff's AILocalTraffic
// Warning - ground elev determination is CPU intensive
// Either this function or the logic of how often it is called
// will almost certainly change.

void FGAIAircraft::getGroundElev(double dt) {
    dt_elev_count += dt;

    if (!needGroundElevation())
        return;
    // Update minimally every three secs, but add some randomness
    // to prevent all AI objects doing this in synchrony
    if (dt_elev_count < (3.0) + (rand() % 10))
        return;

    dt_elev_count = 0;

    // Only do the proper hitlist stuff if we are within visible range of the viewer.
    if (!invisible) {
        double visibility_meters = fgGetDouble("/environment/visibility-m");        
        if (SGGeodesy::distanceM(globals->get_view_position(), pos) > visibility_meters) {
            return;
        }

        double range = 500.0;
        if (globals->get_scenery()->schedule_scenery(pos, range, 5.0))
        {
            double alt;
            if (getGroundElevationM(SGGeod::fromGeodM(pos, 20000), alt, 0))
            {
                tgt_altitude_ft = alt * SG_METER_TO_FEET;
                if (isStationary())
                {
                    // aircraft is stationary and we obtained altitude for this spot - we're done.
                    _needsGroundElevation = false;
                }
            }
        }
    }
}


void FGAIAircraft::doGroundAltitude() {
    if ((fp->getLeg() == 7) && ((altitude_ft -  tgt_altitude_ft) > 5)) {
        tgt_vs = -500;
    } else {
        if ((fabs(altitude_ft - (tgt_altitude_ft+groundOffset)) > 1000.0)||
            (isStationary()))
            altitude_ft = (tgt_altitude_ft + groundOffset);
        else
            altitude_ft += 0.1 * ((tgt_altitude_ft+groundOffset) - altitude_ft);
        tgt_vs = 0;
    }
}


void FGAIAircraft::announcePositionToController() {
    if (!trafficRef) {
        return;
    }
    
    int leg = fp->getLeg();
    if (!fp->getCurrentWaypoint()) {
        // http://code.google.com/p/flightgear-bugs/issues/detail?id=1153
        // throw an exception so this aircraft gets killed by the AIManager.
        throw sg_exception("bad AI flight plan");
    }
    
    // Note that leg has been incremented after creating the current leg, so we should use
    // leg numbers here that are one higher than the number that is used to create the leg
    // NOTE: As of July, 30, 2011, the post-creation leg updating is no longer happening. 
    // Leg numbers are updated only once the aircraft passes the last waypoint created for that legm so I should probably just use
    // the original leg numbers here!
    switch (leg) {
      case 1:              // Startup and Push back
        if (trafficRef->getDepartureAirport()->getDynamics())
            controller = trafficRef->getDepartureAirport()->getDynamics()->getStartupController();
        break;
    case 2:              // Taxiing to runway
        if (trafficRef->getDepartureAirport()->getDynamics()->getGroundController()->exists())
            controller = trafficRef->getDepartureAirport()->getDynamics()->getGroundController();
        break;
    case 3:              //Take off tower controller
        if (trafficRef->getDepartureAirport()->getDynamics()) {
            controller = trafficRef->getDepartureAirport()->getDynamics()->getTowerController();
            towerController = 0;
        } else {
            cerr << "Error: Could not find Dynamics at airport : " << trafficRef->getDepartureAirport()->getId() << endl;
        }
        break;
    case 6:
         if (trafficRef->getArrivalAirport()->getDynamics()) {
             controller = trafficRef->getArrivalAirport()->getDynamics()->getApproachController();
          }
          break;
    case 8:              // Taxiing for parking
        if (trafficRef->getArrivalAirport()->getDynamics()->getGroundController()->exists())
            controller = trafficRef->getArrivalAirport()->getDynamics()->getGroundController();
        break;
    default:
        controller = 0;
        break;
    }

    if ((controller != prevController) && (prevController != 0)) {
        prevController->signOff(getID());
    }
    prevController = controller;
    if (controller) {
        controller->announcePosition(getID(), fp.get(), fp->getCurrentWaypoint()->getRouteIndex(),
                                     _getLatitude(), _getLongitude(), hdg, speed, altitude_ft,
                                     trafficRef->getRadius(), leg, this);
    }
}

void FGAIAircraft::scheduleForATCTowerDepartureControl(int state) {
    if (!takeOffStatus) {
        int leg = fp->getLeg();
        if (trafficRef) {
            if (trafficRef->getDepartureAirport()->getDynamics()) {
                towerController = trafficRef->getDepartureAirport()->getDynamics()->getTowerController();
            } else {
                cerr << "Error: Could not find Dynamics at airport : " << trafficRef->getDepartureAirport()->getId() << endl;
            }
            if (towerController) {
                towerController->announcePosition(getID(), fp.get(), fp->getCurrentWaypoint()->getRouteIndex(),
                                                   _getLatitude(), _getLongitude(), hdg, speed, altitude_ft,
                                                    trafficRef->getRadius(), leg, this);
            }
        }
    }
    takeOffStatus = state;
}

// Process ATC instructions and report back

void FGAIAircraft::processATC(const FGATCInstruction& instruction) {
    if (instruction.getCheckForCircularWait()) {
        // This is not exactly an elegant solution, 
        // but at least it gives me a chance to check
        // if circular waits are resolved.
        // For now, just take the offending aircraft 
        // out of the scene
    setDie(true);
        // a more proper way should be - of course - to
        // let an offending aircraft take an evasive action
        // for instance taxi back a little bit.
    }
    if (instruction.getHoldPattern   ()) {}

    // Hold Position
    if (instruction.getHoldPosition  ()) {
        if (!holdPos) {
            holdPos = true;
        }
        AccelTo(0.0);
    } else {
        if (holdPos) {
            holdPos = false;
        }
        // Change speed Instruction. This can only be excecuted when there is no
        // Hold position instruction.
        if (instruction.getChangeSpeed   ()) {
            AccelTo(instruction.getSpeed());
        } else {
            if (fp) AccelTo(fp->getPreviousWaypoint()->getSpeed());
        }
    }
    if (instruction.getChangeHeading ()) {
        hdg_lock = false;
        TurnTo(instruction.getHeading());
    } else {
        if (fp) {
            hdg_lock = true;
        }
    }
    if (instruction.getChangeAltitude()) {}

}


void FGAIAircraft::handleFirstWaypoint() {
    bool eraseWaypoints;         //TODO YAGNI
    headingError = 0;
    if (trafficRef) {
        eraseWaypoints = true;
    } else {
        eraseWaypoints = false;
    }

    FGAIWaypoint* prev = 0; // the one behind you
    FGAIWaypoint* curr = 0; // the one ahead
    FGAIWaypoint* next = 0;// the next plus 1

    spinCounter = 0;

    //TODO fp should handle this
    fp->IncrementWaypoint(eraseWaypoints);
    if (!(fp->getNextWaypoint()) && trafficRef) {
        if (!loadNextLeg()) {
            setDie(true);
            return;
        }
    }

    prev = fp->getPreviousWaypoint();   //first waypoint
    SG_LOG(SG_AI, SG_BULK, "Previous WP \t" << prev->getName());
    curr = fp->getCurrentWaypoint();    //second waypoint
    SG_LOG(SG_AI, SG_BULK, "Current WP \t" << curr->getName());
    next = fp->getNextWaypoint();       //third waypoint (might not exist!)
    if( next ) {
        SG_LOG(SG_AI, SG_BULK, "Next WP \t" << next->getName());
    }

    setLatitude(prev->getLatitude());
    setLongitude(prev->getLongitude());
    if(fp->getLeg()==1) {
        setSpeed(0);
    } else {
        setSpeed(prev->getSpeed());
    }
    setAltitude(prev->getAltitude());

    if (prev->getSpeed() > 0.0)
        setHeading(fp->getBearing(prev, curr));
    else {
        // FIXME When going to parking it must be the heading of the parking
        setHeading(fp->getBearing(curr, prev));
    }

    // If next doesn't exist, as in incrementally created flightplans for
    // AI/Trafficmanager created plans,
    // Make sure lead distance is initialized otherwise
    // If we are ending in a parking 
    if (next && !curr->contains("END") && !curr->contains("PushBackPointlegend")) {
        fp->setLeadDistance(speed, hdg, curr, next);
    } else {
        fp->setLeadDistance(1);
    }

    if (curr->getCrossat() > -1000.0) //use a calculated descent/climb rate
    {
        use_perf_vs = false;
        double vert_dist_ft = curr->getCrossat() - altitude_ft;
        double err_dist     = prev->getCrossat() - altitude_ft;
        double dist_m       = fp->getDistanceToGo(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr);
        tgt_vs = calcVerticalSpeed(vert_dist_ft, dist_m, speed, err_dist);
        checkTcas();
        tgt_altitude_ft = curr->getCrossat();
    } else {
        use_perf_vs = true;
        tgt_altitude_ft = prev->getAltitude();
    }
    alt_lock = hdg_lock = true;
    no_roll = prev->getOn_ground();
    if (no_roll) {
        Transform();             // make sure aip is initialized.
        getGroundElev(60.1);     // make sure it's executed first time around, so force a large dt value
        doGroundAltitude();
        _needsGroundElevation = true; // check ground elevation again (maybe scenery wasn't available yet)
    }
    // Make sure to announce the aircraft's position
    announcePositionToController();
    prevSpeed = 0;
}

/**
 * Check Execution time (currently once every 100 ms)
 * Add a bit of randomization to prevent the execution of all flight plans
 * in synchrony, which can add significant periodic framerate flutter.
 *
 * @param now
 * @return
 */
bool FGAIAircraft::fpExecutable(time_t now) {
    double rand_exec_time = (rand() % 100) / 100;
    return (dt_count > (0.1+rand_exec_time)) && (fp->isActive(now));
}


/**
 * Check to see if we've reached the lead point for our next turn
 *
 * @param curr the WP we are currently targeting at.
 * @param next the WP that will follow. Used to detect passed WPs (heading diff curr/next > 120°) 
 * @param nextTurnAngle to detect sharp corners
 * @return
 */
bool FGAIAircraft::leadPointReached(FGAIWaypoint* curr, FGAIWaypoint* next, int nextTurnAngle) {
    double dist_to_go_m = fp->getDistanceToGo(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr);
    // Leaddistance should be ft
    double lead_dist = fp->getLeadDistance();
    const double arrivalDist = fabs(10.0*fp->getCurrentWaypoint()->getSpeed());
    // arrive at pushback end
    if ((dist_to_go_m < arrivalDist) && (speed < 0) && (tgt_speed < 0) && fp->getCurrentWaypoint()->contains("PushBackPoint")) {        
//        tgt_speed = -(dist_to_go_m / 10.0);
        tgt_speed = -std::sqrt((pow(arrivalDist,2)-pow(arrivalDist-dist_to_go_m,2)));
        SG_LOG(SG_AI, SG_BULK, "tgt_speed " << tgt_speed);
        if (tgt_speed > -1) {
            // Speed is int and cannot go below 1 knot
            tgt_speed = -1;
        }

        if (fp->getPreviousWaypoint()->getSpeed() < tgt_speed) {
            SG_LOG(SG_AI, SG_BULK, "Set speed of WP from " << fp->getPreviousWaypoint()->getSpeed() << " to  " << tgt_speed);
            fp->getPreviousWaypoint()->setSpeed(tgt_speed);
        }
    } 
    // arrive at parking
    if ((dist_to_go_m < arrivalDist) && (speed > 0) && (tgt_speed > 0) && fp->getCurrentWaypoint()->contains("END")) {
        tgt_speed = (dist_to_go_m / 10.0);
        if (tgt_speed < 1) {
            // Speed is int and cannot go below 1 knot
            tgt_speed = 1;
        }

        if (fp->getPreviousWaypoint()->getSpeed() < tgt_speed) {
            SG_LOG(SG_AI, SG_BULK, "Set speed of WP from " << fp->getPreviousWaypoint()->getSpeed() << " to  " << tgt_speed);
            fp->getPreviousWaypoint()->setSpeed(tgt_speed);
        }
    } 
    
    if (lead_dist < fabs(2*speed)) {
      //don't skip over the waypoint
        lead_dist = fabs(2*speed);
    }
    
    double bearing = 0;
    // don't do bearing calculations for ground traffic
    bearing = getBearing(fp->getBearing(pos, curr));
    double nextBearing = bearing;
    if (next) {
        nextBearing = getBearing(fp->getBearing(pos, next));
    }
    double bearingDiff = fabs(bearing-nextBearing);
    if (onGround() && nextTurnAngle > 30 && bearingDiff > 50) {
        // Next turn is pretty sharp so we do a preturn
        SG_LOG(SG_AI, SG_BULK, "Leadpoint reached due to excessive heading diff " << bearingDiff);
        minBearing = 360;
        speedFraction = 1.0;
        prev_dist_to_go = HUGE_VAL;
        return true;
    }
    if (bearing < minBearing) {
        minBearing = bearing;
        if (minBearing < 10) {
            minBearing = 10;
        }
        if ((minBearing < 360.0) && (minBearing > 10.0)) {
            speedFraction = 0.5 + (cos(minBearing *SG_DEGREES_TO_RADIANS) * 0.5);
        } else {
            speedFraction = 1.0;
        }
    }

    if ((dist_to_go_m < lead_dist) ||
        ((dist_to_go_m > prev_dist_to_go) && (bearing > (minBearing * 1.1))) ) {
        SG_LOG(SG_AI, SG_BULK, "Leadpoint reached Bearing : " << bearing << "\tNext Bearing : " << nextBearing);
        minBearing = 360;
        speedFraction = 1.0;
        prev_dist_to_go = HUGE_VAL;
        return true;
    } else {
        if (prev_dist_to_go == dist_to_go_m && fabs(groundTargetSpeed)>0) {
            //FIXME must be suppressed when parked
            SG_LOG(SG_AI, SG_BULK, "Aircraft " << _callsign << " stuck. Speed " << speed);
            stuckCounter++;
            if (stuckCounter>AI_STUCK_LIMIT) {
                SG_LOG(SG_AI, SG_WARN, "Stuck flight " << _callsign << " killed" );
                setDie(true);
            }
        } else {
            stuckCounter = 0;
        }
        prev_dist_to_go = dist_to_go_m;
        return false;
    }
}


bool FGAIAircraft::aiTrafficVisible()
{
    SGVec3d cartPos = SGVec3d::fromGeod(pos);
    const double d2 = (TRAFFICTOAIDISTTODIE * SG_NM_TO_METER) *
        (TRAFFICTOAIDISTTODIE * SG_NM_TO_METER);
    return (distSqr(cartPos, globals->get_aircraft_position_cart()) < d2);
}


/**
 * Handle release of parking gate, once were taxiing. Also ensure service time at the gate
 * in the case of an arrival.
 *
 * @param prev
 * @return
 */

//TODO the trafficRef is the right place for the method
bool FGAIAircraft::handleAirportEndPoints(FGAIWaypoint* prev, time_t now) {
    // prepare routing from one airport to another
    FGAirport * dep = trafficRef->getDepartureAirport();
    FGAirport * arr = trafficRef->getArrivalAirport();

    if (!( dep && arr))
        return false;

    // This waypoint marks the fact that the aircraft has passed the initial taxi
    // departure waypoint, so it can release the parking.
    if (prev->contains("PushBackPoint")) {
      // clearing the parking assignment will release the gate
        fp->setGate(ParkingAssignment());
        AccelTo(0.0);
    }
    if (prev->contains("legend")) {
        fp->incrementLeg();
    }
    if (prev->contains(string("DepartureHold"))) {
        //cerr << "Passing point DepartureHold" << endl;
        scheduleForATCTowerDepartureControl(1);
    }
    if (prev->contains(string("Accel"))) {
        takeOffStatus = 3;
    }

    // This is the last taxi waypoint, and marks the the end of the flight plan
    // so, the schedule should update and wait for the next departure time.
    if (prev->contains("END")) {
        SG_LOG(SG_AI, SG_BULK, "Reached " << prev->getName() );

        //FIXME Heading Error should be reset
        time_t nextDeparture = trafficRef->getDepartureTime();
        // make sure to wait at least 20 minutes at parking to prevent "nervous" taxi behavior
        if (nextDeparture < (now+1200)) {
            nextDeparture = now + 1200;
        }
        fp->setTime(nextDeparture);
    }

    return true;
}


/**
 * Check difference between target bearing and current heading and correct if necessary.
 *
 * @param curr
 */
void FGAIAircraft::controlHeading(FGAIWaypoint* curr) {
    const double calc_bearing = speed < 0?SGMiscd::normalizePeriodic(0, 360, fp->getBearing(pos, curr) + 180.0):fp->getBearing(pos, curr);

    if (fgIsFinite(calc_bearing)) {
        double hdg_error = calc_bearing - tgt_heading;
        if (fabs(hdg_error) > 0.01) {
            TurnTo( calc_bearing );
        }

    } else {
        cerr << "calc_bearing is not a finite number : "
        << "Speed " << speed
        << "pos : " << pos.getLatitudeDeg() << ", " << pos.getLongitudeDeg()
        << ", waypoint: " << curr->getLatitude() << ", " << curr->getLongitude() << endl;
        cerr << "waypoint name: '" << curr->getName() << "'" << endl;
    }
}


/**
 * Update the lead distance calculation if speed has changed sufficiently
 * to prevent spinning (hopefully);
 *
 * @param curr
 * @param next
 */
void FGAIAircraft::controlSpeed(FGAIWaypoint* curr, FGAIWaypoint* next) {
    double speed_diff = speed - prevSpeed;

    if (fabs(speed_diff) > 10) {
        prevSpeed = speed;
        if (next) {
            if (next && !curr->contains("END") && !curr->contains("PushBackPointlegend")) {
                fp->setLeadDistance(speed, tgt_heading, curr, next);
            } else {
            // If we are ending in a parking the heading will be a 
                fp->setLeadDistance(1);
            }
        }
    }
}


/**
 * Update target values (heading, alt, speed) depending on flight plan or control properties
 */
void FGAIAircraft::updatePrimaryTargetValues(double dt, bool& flightplanActive, bool& aiOutOfSight) {
    if (fp && fp->isValidPlan()) // AI object has a flightplan
    {
        //TODO make this a function of AIBase
        time_t now = globals->get_time_params()->get_cur_time();

        //cerr << "UpateTArgetValues() " << endl;
        ProcessFlightPlan(dt, now);

        // Do execute Ground elev for inactive aircraft, so they
        // Are repositioned to the correct ground altitude when the user flies within visibility range.
        // In addition, check whether we are out of user range, so this aircraft
        // can be deleted.
        if (onGround()) {
                Transform();     // make sure aip is initialized.
                getGroundElev(dt);
                doGroundAltitude();
                // Transform();
                pos.setElevationFt(altitude_ft);
        }
        if (trafficRef) {
            aiOutOfSight = !aiTrafficVisible();
            if (aiOutOfSight) {
                setDie(true);
                aiOutOfSight = true;
                return;
            }
        }
        timeElapsed = now - fp->getStartTime();
        flightplanActive = fp->isActive(now);
    } else {
        // no flight plan, update target heading, speed, and altitude
        // from control properties.  These default to the initial
        // settings in the config file, but can be changed "on the
        // fly".

        if (!_controlsLateralModeNode) {
            lazyInitControlsNodes();
        }


        const string& lat_mode = _controlsLateralModeNode->getStringValue();
        if ( lat_mode == "roll" ) {
            const double angle = _controlsTargetRollNode->getDoubleValue();
            RollTo( angle );
        } else {
            const double angle = _controlsTargetHeadingNode->getDoubleValue();
            TurnTo( angle );
        }

        string vert_mode = _controlsVerticalModeNode->getStringValue();
        if (vert_mode == "alt") {
            const double alt = _controlsTargetAltitude->getDoubleValue();
            ClimbTo( alt );
        } else {
            const double angle = _controlsTargetPitch->getDoubleValue();
            PitchTo( angle );
        }

        AccelTo(_controlsTargetSpeed->getDoubleValue());
    }
}

void FGAIAircraft::updateHeading(double dt) {
    // adjust heading based on current bank angle
    if (roll == 0.0)
        roll = 0.01;

    if (roll != 0.0) {
        // If on ground, calculate heading change directly
        if (onGround()) {
            const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, hdg-tgt_heading);

            // When pushback behind us we still want to move but ...
            groundTargetSpeed = tgt_speed * cos(headingDiff * SG_DEGREES_TO_RADIANS);

            if (sign(groundTargetSpeed) != sign(tgt_speed) && fabs(tgt_speed) > 0) {
                if (fabs(speed) < 2 && fp->isActive(globals->get_time_params()->get_cur_time())) {
                    // This seems to happen in case there is a change from forward to pushback.
                    // which should never happen.
                    SG_LOG(SG_AI, SG_BULK, "Oh dear " << _callsign << " might get stuck aka next point is behind us. Speed is " << speed );
                    stuckCounter++;
                    if (stuckCounter>AI_STUCK_LIMIT) {
                        SG_LOG(SG_AI, SG_WARN, "Stuck flight " << _callsign << " killed" );
                        setDie(true);
                    }
                }
                // Negative Cosinus means angle > 90°
                groundTargetSpeed = 0.21 * sign(tgt_speed); // to prevent speed getting stuck in 'negative' mode
            }

            // Only update the target values when we're not moving because otherwise we might introduce an enormous target change rate while waiting a the gate, or holding.
            if (speed != 0) {
                if (fabs(headingDiff) > 30.0) {
                    // invert if pushed backward
                    if( sign(headingChangeRate) == sign(headingDiff)) {
                        // left/right change
                        headingChangeRate = 10.0 * dt * sign(headingDiff) * -1;
                    } else {
                        headingChangeRate -= 10.0 * dt * sign(headingDiff);
                    }

                    // Clamp the maximum steering rate to 30 degrees per second,
                    // But only do this when the heading error is decreasing.
                    // FIXME
                    if ((headingDiff < headingError)) {
                        if (headingChangeRate > 30)
                            headingChangeRate = 30;
                        else if (headingChangeRate < -30)
                            headingChangeRate = -30;
                    }
                } else {
                    if (speed != 0) {
                        if( sign(headingChangeRate) == sign(headingDiff)) {
                            // left/right change
                           headingChangeRate = 3 * dt * sign(headingDiff) * -1;
                        } else {
                            headingChangeRate -= 3 * dt * sign(headingDiff);
                        }
                        /*
                        if (headingChangeRate > headingDiff || 
                            headingChangeRate < headingDiff) {
                            headingChangeRate = headingDiff*sign(roll);
                        }
                        else {
                            headingChangeRate += dt * sign(roll);
                        }
                        */
                    }
                }
            }

            hdg += headingChangeRate * dt * sqrt(fabs(speed) / 15);

            headingError = headingDiff;
            // SG_LOG(SG_AI, SG_BULK, "Headingerror " << headingError );
            if (fabs(headingError) < 1.0) {
                hdg = tgt_heading;
            }
        } else {
            if (fabs(speed) > 1.0) {
                turn_radius_ft = 0.088362 * speed * speed
                                 / tan( fabs(roll) / SG_RADIANS_TO_DEGREES );
            } else {
                // Check if turn_radius_ft == 0; this might lead to a division by 0.
                turn_radius_ft = 1.0;
            }
            double turn_circum_ft = SGD_2PI * turn_radius_ft;
            double dist_covered_ft = speed * 1.686 * dt;
            double alpha = dist_covered_ft / turn_circum_ft * 360.0;
            hdg += alpha * sign(roll);
        }
        while ( hdg > 360.0 ) {
            hdg -= 360.0;
            spinCounter++;
        }
        while ( hdg < 0.0) {
            hdg += 360.0;
            spinCounter--;
        }
    }
}


void FGAIAircraft::updateBankAngleTarget() {
    // adjust target bank angle if heading lock engaged
    if (hdg_lock) {
        double bank_sense = 0.0;
        double diff = fabs(hdg - tgt_heading);
        if (diff > 180)
            diff = fabs(diff - 360);

        double sum = hdg + diff;
        if (sum > 360.0)
            sum -= 360.0;
        if (fabs(sum - tgt_heading) < 1.0) {
            bank_sense = 1.0;    // right turn
        } else {
            bank_sense = -1.0;   // left turn
        }
        if (diff < _performance->maximumBankAngle()) {
            tgt_roll = diff * bank_sense;
        } else {
            tgt_roll = _performance->maximumBankAngle() * bank_sense;
        }
        if ((fabs((double) spinCounter) > 1) && (diff > _performance->maximumBankAngle())) {
            tgt_speed *= 0.999;  // Ugly hack: If aircraft get stuck, they will continually spin around.
            // The only way to resolve this is to make them slow down.
        }
    }
}

void FGAIAircraft::updateVerticalSpeedTarget(double dt) {
    // adjust target Altitude, based on ground elevation when on ground
    if (onGround()) {
        getGroundElev(dt);
        doGroundAltitude();
    } else if (alt_lock) {
        // find target vertical speed
        if (use_perf_vs) {
            if (altitude_ft < tgt_altitude_ft) {
                tgt_vs = std::min(tgt_altitude_ft - altitude_ft, _performance->climbRate());
            } else {
                tgt_vs = std::max(tgt_altitude_ft - altitude_ft, -_performance->descentRate());
            }
        } else if (fp->getCurrentWaypoint()) {
            double vert_dist_ft = fp->getCurrentWaypoint()->getCrossat() - altitude_ft;
            double err_dist     = 0;
            double dist_m       = fp->getDistanceToGo(pos.getLatitudeDeg(), pos.getLongitudeDeg(), fp->getCurrentWaypoint());
            tgt_vs = calcVerticalSpeed(vert_dist_ft, dist_m, speed, err_dist);
        } else {
            // avoid crashes when fp has no current waypoint
            // eg see FLIGHTGEAR-68 on sentry; we crashed in getCrossat()
            tgt_vs = 0.0;
        }
    }

    checkTcas();
}

void FGAIAircraft::updatePitchAngleTarget() {
    // if on ground and above vRotate -> initial rotation
    if (onGround() && (speed > _performance->vRotate()))
        tgt_pitch = 8.0; // some rough B737 value 

    //TODO pitch angle on approach and landing
    
    // match pitch angle to vertical speed
    else if (tgt_vs > 0) {
        tgt_pitch = tgt_vs * 0.005;
    } else {
        tgt_pitch = tgt_vs * 0.002;
    }
}

const string& FGAIAircraft::atGate()
{
     if ((fp->getLeg() < 3) && trafficRef) {
       if (fp->getParkingGate()) {
         return fp->getParkingGate()->ident();
       }
     }
       
     static const string empty;
     return empty;
}

void FGAIAircraft::handleATCRequests(double dt)
{
    if (!this->getTrafficRef()) {
        return;
    }

    //TODO implement NullController for having no ATC to save the conditionals
    if (controller) {
        controller->updateAircraftInformation(getID(),
                                              pos.getLatitudeDeg(),
                                              pos.getLongitudeDeg(),
                                              hdg,
                                              speed,
                                              altitude_ft, dt);
        processATC(controller->getInstruction(getID()));
    }
    if (towerController) {
        towerController->updateAircraftInformation(getID(),
                                              pos.getLatitudeDeg(),
                                              pos.getLongitudeDeg(),
                                              hdg,
                                              speed,
                                              altitude_ft, dt);
    }
}

void FGAIAircraft::updateActualState(double dt)
{
    //update current state
    //TODO have a single tgt_speed and check speed limit on ground on setting tgt_speed
    double distance = speed * SG_KT_TO_MPS * dt;
    pos = SGGeodesy::direct(pos, hdg, distance);

    if (onGround())
        speed = _performance->actualSpeed(this, groundTargetSpeed, dt, holdPos);
    else
        speed = _performance->actualSpeed(this, (tgt_speed *speedFraction), dt, false);

    updateHeading(dt);
    roll = _performance->actualBankAngle(this, tgt_roll, dt);

    // adjust altitude (meters) based on current vertical speed (fpm)
    altitude_ft += vs_fps * dt;
    pos.setElevationFt(altitude_ft);

    vs_fps = _performance->actualVerticalSpeed(this, tgt_vs, dt) / 60;
    pitch = _performance->actualPitch(this, tgt_pitch, dt);
}

void FGAIAircraft::updateSecondaryTargetValues(double dt) {
    // derived target state values
    updateBankAngleTarget();
    updateVerticalSpeedTarget(dt);
    updatePitchAngleTarget();
    //TODO calculate wind correction angle (tgt_yaw)
}


bool FGAIAircraft::reachedEndOfCruise(double &distance) {
    FGAIWaypoint* curr = fp->getCurrentWaypoint();
    if (!curr) {
        SG_LOG(SG_AI, SG_WARN, "FGAIAircraft::reachedEndOfCruise: no current waypoint");

        // return true (=done) here, so we don't just get stuck on this forever
        return true;
    }
    
    if (curr->getName() == string("BOD")) {
        // Sentry: FLIGHTGEAR-893
        if (!trafficRef->getArrivalAirport()) {
            SG_LOG(SG_AI, SG_WARN, trafficRef->getCallSign() << "FGAIAircraft::reachedEndOfCruise: no arrival airport");
            setDie(true);
            // return 'done' here, we are broken
            return true;
        }
        
        double dist = fp->getDistanceToGo(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr);
        double descentSpeed = (getPerformance()->vDescent() * SG_NM_TO_METER) / 3600.0;     // convert from kts to meter/s
        double descentRate  = (getPerformance()->descentRate() * SG_FEET_TO_METER) / 60.0;  // convert from feet/min to meter/s

        double verticalDistance  = ((altitude_ft - 2000.0) - trafficRef->getArrivalAirport()->getElevation()) *SG_FEET_TO_METER;
        double descentTimeNeeded = verticalDistance / descentRate;
        double distanceCovered   = descentSpeed * descentTimeNeeded; 

        if (trafficRef->getCallSign() != "" && 
            trafficRef->getCallSign() == fgGetString("/ai/track-callsign")) {
            cerr << "Checking for end of cruise stage for :" << trafficRef->getCallSign() << endl;
            cerr << "Descent rate      : " << descentRate << endl;
            cerr << "Descent speed     : " << descentSpeed << endl;
            cerr << "VerticalDistance  : " << verticalDistance << ". Altitude : " << altitude_ft << ". Elevation " << trafficRef->getArrivalAirport()->getElevation() << endl;
            cerr << "DecentTimeNeeded  : " << descentTimeNeeded << endl;
            cerr << "DistanceCovered   : " << distanceCovered   << endl;
        }

        distance = distanceCovered;
        if (dist < distanceCovered) {
            if (trafficRef->getCallSign() == fgGetString("/ai/track-callsign")) {
                //exit(1);
            }
            SG_LOG(SG_AI, SG_BULK, "End Of Cruise");
            return true;
        } else {
              return false;
        }
    } else {
         return false;
    }
}

void FGAIAircraft::resetPositionFromFlightPlan()
{
    // the one behind you
    FGAIWaypoint* prev = 0;
    // the one ahead
    FGAIWaypoint* curr = 0;
    // the next plus 1
    FGAIWaypoint* next = 0;

    prev = fp->getPreviousWaypoint();
    curr = fp->getCurrentWaypoint();
    next = fp->getNextWaypoint();

    setLatitude(prev->getLatitude());
    setLongitude(prev->getLongitude());
    setHeading(fp->getBearing(curr, next));
    setAltitude(prev->getAltitude());
    setSpeed(prev->getSpeed());
}

/**
 * Returns a normalised bearing
 */

double FGAIAircraft::getBearing(double crse) 
{
  double hdgDiff = fabs(hdg-crse);
  if (hdgDiff > 180)
      hdgDiff = fabs(hdgDiff - 360);
  return hdgDiff;
}

time_t FGAIAircraft::checkForArrivalTime(const string& wptName) {
     FGAIWaypoint* curr = 0;
     curr = fp->getCurrentWaypoint();

     // don't recalculate tracklength unless the start/stop waypoint has changed
     if (curr &&
         ((curr->getName() != trackCache.startWptName)||
          (wptName != trackCache.finalWptName)))
     {
         trackCache.remainingLength = fp->checkTrackLength(wptName);
         trackCache.startWptName = curr->getName();
         trackCache.finalWptName = wptName;
     }
     double tracklength = trackCache.remainingLength;
     if (tracklength > 0.1) {
          tracklength += fp->getDistanceToGo(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr);
     } else {
         return 0;
     }
     time_t now = globals->get_time_params()->get_cur_time();
     time_t arrivalTime = fp->getArrivalTime();
     
     time_t ete = tracklength / ((speed * SG_NM_TO_METER) / 3600.0); 
     time_t secondsToGo = arrivalTime - now;
     if (trafficRef->getCallSign() == fgGetString("/ai/track-callsign")) {    
          cerr << "Checking arrival time: ete " << ete << ". Time to go : " << secondsToGo << ". Track length = " << tracklength << endl;
     }
     return (ete - secondsToGo); // Positive when we're too slow...
}

double limitRateOfChange(double cur, double target, double maxDeltaSec, double dt)
{
  double delta = target - cur;
  double maxDelta = maxDeltaSec * dt;
  
// if delta is > maxDelta, use maxDelta, but with the sign of delta.
  return (fabs(delta) < maxDelta) ? delta : copysign(maxDelta, delta);
}

// Drive various properties in a semi-realistic fashion.
// Note that we assume that the properties are set at 
// a waypoint rather than in the leg before.  So we need 
// to use the  previous waypoint (i.e. the one just passed)
// rather than the current one (i.e. the next one on the route)
void FGAIAircraft::updateModelProperties(double dt)
{
  if ((!fp) || (!fp->getPreviousWaypoint())) {
    return;
  }
  
  double targetGearPos = fp->getPreviousWaypoint()->getGear_down() ? 1.0 : 0.0;
  double gearPos = getGearPos();
  
  if (gearPos != targetGearPos) {
    gearPos = gearPos + limitRateOfChange(gearPos, targetGearPos, 0.1, dt);
    if (gearPos < 0.001) {
      gearPos = 0.0;
    } else if (gearPos > 0.999) {
      gearPos = 1.0;    
    }
    setGearPos(gearPos);
  } 

  double targetFlapsPos = fp->getPreviousWaypoint()->getFlaps();
  double flapsPos = getFlapsPos();

  if (flapsPos != targetFlapsPos) {
    flapsPos = flapsPos + limitRateOfChange(flapsPos, targetFlapsPos, 0.05, dt);
    if (flapsPos < 0.001) {        
      flapsPos = 0.0;
    } else if (flapsPos > 0.999) {
      flapsPos = 1.0;
    }
    setFlapsPos(flapsPos);
  }

  setSpeedBrakePos(fp->getPreviousWaypoint()->getSpeedBrakes());
  setSpoilerPos(fp->getPreviousWaypoint()->getSpoilers());
  setCabinLight(fp->getPreviousWaypoint()->getCabinLight());
  setBeaconLight(fp->getPreviousWaypoint()->getBeaconLight());
  setLandingLight(fp->getPreviousWaypoint()->getLandingLight());
  setNavLight(fp->getPreviousWaypoint()->getNavLight());
  setStrobeLight(fp->getPreviousWaypoint()->getStrobeLight());
  setTaxiLight(fp->getPreviousWaypoint()->getTaxiLight());
}

void FGAIAircraft::dumpCSVHeader(std::ofstream& o) {
    o << "Index\t";
    o << "Lat\t";
    o << "Lon\t";
    o << "Callsign\t";
    o << "headingDiff\t";
    o << "headingChangeRate\t";
    o << "headingError\t";
    o << "hdg\t";
    o << "tgt_heading\t";
    o << "tgt_speed\t";
    o << "minBearing\t";
    o << "speedFraction\t";
    o << "groundOffset\t";
    o << "speed\t";
    o << "groundTargetSpeed\t";
    o << "getVerticalSpeedFPM\t";
    o << "getTrueHeadingDeg\t";
    o << "bearingToWP\t";

    o << "Name\t";
    o << "WP Lat\t";
    o << "WP Lon\t";
    o << "Dist\t";
    o << "Departuretime\t";
    o << "Time\t";
    o << "Startup diff\t";
    o << "dist_to_go_m\t";
    o << "Leg\t";
    o << "Num WP\t";
    o << "Leaddistance\t";
    o << "no_roll\t";
    o << "roll\t";
    o << "stuckCounter";
    o << endl;
}

void FGAIAircraft::dumpCSV(std::ofstream& o, int lineIndex) {
    const double headingDiff = SGMiscd::normalizePeriodic(-180, 180, hdg-tgt_heading);

    o << lineIndex << "\t";
    o << setprecision(12);
    o << this->getGeodPos().getLatitudeDeg() << "\t";
    o << this->getGeodPos().getLongitudeDeg() << "\t";
    o << this->getCallSign() << "\t";
    o << headingDiff << "\t";
    o << headingChangeRate << "\t";
    o  << headingError << "\t";
    o << hdg << "\t";
    o << tgt_heading << "\t";
    o << tgt_speed << "\t";
    o << minBearing << "\t";
    o << speedFraction << "\t";
    o << groundOffset << "\t";

    o << round(this->getSpeed()) << "\t";
    o << groundTargetSpeed  << "\t";
    o  << round(this->getVerticalSpeedFPM()) << "\t";
    o << this->getTrueHeadingDeg() << "\t";
    FGAIFlightPlan* fp = this->GetFlightPlan();
    FGAIWaypoint* currentWP = this->GetFlightPlan()->getCurrentWaypoint();
    if (currentWP) {
        o << this->GetFlightPlan()->getBearing(this->getGeodPos(), this->GetFlightPlan()->getCurrentWaypoint()) << "\t";
        o << currentWP->getName() << "\t";
        o << this->GetFlightPlan()->getCurrentWaypoint()->getPos().getLatitudeDeg() << "\t";
        o << this->GetFlightPlan()->getCurrentWaypoint()->getPos().getLongitudeDeg() << "\t";
        o << SGGeodesy::distanceM(this->getGeodPos(), currentWP->getPos()) << "\t";
        o << this->GetFlightPlan()->getStartTime() << "\t";
        o << globals->get_time_params()->get_cur_time() << "\t";
        o << this->GetFlightPlan()->getStartTime() - globals->get_time_params()->get_cur_time() << "\t";
        double dist_to_go_m = fp->getDistanceToGo(pos.getLatitudeDeg(), pos.getLongitudeDeg(), currentWP);
        o << dist_to_go_m << "\t";
    } else {
        o << "No WP\t\t\t\t\t\t\t\t";
    }
    if (fp->isValidPlan()) {
        o << fp->getLeg() << "\t";
        o << fp->getNrOfWayPoints() << "\t";
        o << fp->getLeadDistance() << "\t";
    } else {
        o << "FP NotValid\t\t";
    }
    o << this->onGround() << "\t";
    o << roll << "\t";
    o << stuckCounter;
    o << endl;
}

std::string FGAIAircraft::getTimeString(int timeOffset)
{
    char ret[11];
    time_t rawtime;
    time (&rawtime);
    rawtime = rawtime + timeOffset;
    tm* timeinfo = gmtime(&rawtime);
    strftime(ret, 11, "%w/%H:%M:%S", timeinfo);
    return ret;
}