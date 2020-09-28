// trafficrecord.cxx - Implementation of AIModels ATC code.
//
// Written by Durk Talsma, started September 2006.
//
// Copyright (C) 2006 Durk Talsma.
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

#include <config.h>

#include <algorithm>
#include <cstdio>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Shape>

#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Scenery/scenery.hxx>

#include "trafficcontrol.hxx"
#include "atc_mgr.hxx"
#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/performancedata.hxx>
#include <ATC/atc_mgr.hxx>
#include <Traffic/TrafficMgr.hxx>
#include <Airports/groundnetwork.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/airport.hxx>
#include <Radio/radio.hxx>
#include <signal.h>

using std::sort;
using std::string;

namespace {

void clearTrafficControllers(TrafficVector& vec)
{
    TrafficVectorIterator it = vec.begin();
    for (; it != vec.end(); ++it) {
        it->getAircraft()->clearATCController();
    }
}

TrafficVectorIterator searchActiveTraffic(TrafficVector& vec, int id)
{
    return std::find_if(vec.begin(), vec.end(), [id] (const FGTrafficRecord& rec)
                 { return rec.getId() == id; });
}

} // of anonymous namespace

/***************************************************************************
 * ActiveRunway
 **************************************************************************/
 
/*
* Fetch next slot for the active runway
* @param eta time of slot requested
* @return newEta: next slot available; starts at eta paramater
* and adds separation as needed
*/
time_t ActiveRunway::requestTimeSlot(time_t eta)
{
    time_t newEta = 0;
    // default separation - 60 seconds
    time_t separation = 60;
    //if (wakeCategory == "heavy_jet") {
    //   SG_LOG(SG_ATC, SG_DEBUG, "Heavy jet, using extra separation");
    //    time_t separation = 120;
    //}
    bool found = false;
    
    // if the aircraft is the first arrival, add to the vector and return eta directly
    if (estimatedArrivalTimes.empty()) {
        estimatedArrivalTimes.push_back(eta);
        SG_LOG(SG_ATC, SG_DEBUG, "Checked eta slots, using" << eta);
        return eta;
    } else {
        // First check the already assigned slots to see where we need to fit the flight in
        TimeVectorIterator i = estimatedArrivalTimes.begin();
        SG_LOG(SG_ATC, SG_DEBUG, "Checking eta slots " << eta << ": ");
        
        // is this needed - just a debug output?
        for (i = estimatedArrivalTimes.begin();
                i != estimatedArrivalTimes.end(); i++) {
            SG_LOG(SG_ATC, SG_BULK, "Stored time : " << (*i));
        }
        
        // if the flight is before the first scheduled slot + separation
        i = estimatedArrivalTimes.begin();
        if ((eta + separation) < (*i)) {
            newEta = eta;
            SG_LOG(SG_ATC, SG_BULK, "Storing at beginning");
            SG_LOG(SG_ATC, SG_DEBUG, "Done. New ETA : " << newEta);
            slotHousekeeping(newEta);
            return newEta;
        }
        
        // else, look through the rest of the slots
        while ((i != estimatedArrivalTimes.end()) && (!found)) {
            TimeVectorIterator j = i + 1;
            
            // if the flight is after the last scheduled slot check if separation is needed
            if (j == estimatedArrivalTimes.end()) {
                if (((*i) + separation) < eta) {
                    SG_LOG(SG_ATC, SG_BULK, "Storing at end");
                    newEta = eta;
                } else {
                    newEta = (*i) + separation;
                    SG_LOG(SG_ATC, SG_BULK, "Storing at end + separation");
                }
                SG_LOG(SG_ATC, SG_DEBUG, "Done. New ETA : " << newEta);
                slotHousekeeping(newEta);
                return newEta;
            } else {
                // potential slot found
                // check the distance between the previous and next slots
                // distance msut be greater than 2* separation
                if ((((*j) - (*i)) > (separation * 2))) {
                    // now check whether this slot is usable:
                    // eta should fall between the two points
                    // i.e. eta > i AND eta < j
                    SG_LOG(SG_ATC, SG_DEBUG, "Found potential slot after " << (*i));
                    if (eta > (*i) && (eta < (*j))) {
                        found = true;
                        if (eta < ((*i) + separation)) {
                            newEta = (*i) + separation;
                            SG_LOG(SG_ATC, SG_BULK, "Using  original" << (*i) << " + separation ");
                        } else {
                            newEta = eta;
                            SG_LOG(SG_ATC, SG_BULK, "Using original after " << (*i));
                        }
                    } else if (eta < (*i)) {
                        found = true;
                        newEta = (*i) + separation;
                        SG_LOG(SG_ATC, SG_BULK, "Using delayed slot after " << (*i));
                    }
                    /*
                       if (((*j) - separation) < eta) {
                       found = true;
                       if (((*i) + separation) < eta) {
                       newEta = eta;
                       SG_LOG(SG_ATC, SG_BULK, "Using original after " << (*i));
                       } else {
                       newEta = (*i) + separation;
                       SG_LOG(SG_ATC, SG_BULK, "Using  " << (*i) << " + separation ");
                       }
                       } */
                }
            }
            i++;
        }
    }

    SG_LOG(SG_ATC, SG_DEBUG, "Done. New ETA : " << newEta);
    slotHousekeeping(newEta);
    return newEta;
}

void ActiveRunway::slotHousekeeping(time_t newEta)
{
    // add the slot to the vector and resort the vector
    estimatedArrivalTimes.push_back(newEta);
    sort(estimatedArrivalTimes.begin(), estimatedArrivalTimes.end());
    
    // do some housekeeping : remove any slots that are past
    time_t now = globals->get_time_params()->get_cur_time();

    TimeVectorIterator i = estimatedArrivalTimes.begin();
    while (i != estimatedArrivalTimes.end()) {
        if ((*i) < now) {
            SG_LOG(SG_ATC, SG_BULK, "Deleting timestamp " << (*i) << " (now = " << now << "). ");
            estimatedArrivalTimes.erase(i);
            i = estimatedArrivalTimes.begin();
        } else {
            i++;
        }
    }
}

/* Output the contents of the departure queue vector nicely formatted*/
void ActiveRunway::printdepartureQueue()
{
    SG_LOG(SG_ATC, SG_DEBUG, "Departure queue for " << rwy << ": ");
    for (AircraftVecIterator atc = departureQueue.begin(); atc != departureQueue.end(); atc++) {
        SG_LOG(SG_ATC, SG_DEBUG, "     " << (*atc)->getCallSign() << " " << (*atc)->getTakeOffStatus());
        SG_LOG(SG_ATC, SG_DEBUG, " " << (*atc)->_getLatitude() << " " << (*atc)->_getLongitude() << (*atc)->getSpeed() << " " << (*atc)->getAltitude());
    }
    
}

/* Fetch the first aircraft in the departure cue with a certain status */
FGAIAircraft* ActiveRunway::getFirstOfStatus(int stat)
{
    for (AircraftVecIterator atc =departureQueue.begin(); atc != departureQueue.end(); atc++) {
        if ((*atc)->getTakeOffStatus() == stat)
            return (*atc);
    }
    return 0;
}



/***************************************************************************
 * FGTrafficRecord
 **************************************************************************/
 
FGTrafficRecord::FGTrafficRecord():
        id(0), waitsForId(0),
        currentPos(0),
        leg(0),
        frequencyId(0),
        state(0),
        allowTransmission(true),
        allowPushback(true),
        priority(0),
        timer(0),
        latitude(0), longitude(0), heading(0), speed(0), altitude(0), radius(0)
{
}

FGTrafficRecord::~FGTrafficRecord()
{
}

void FGTrafficRecord::setPositionAndIntentions(int pos,
        FGAIFlightPlan * route)
{
    SG_LOG(SG_ATC, SG_DEBUG, "Position: " << pos);
    currentPos = pos;
    if (!intentions.empty()) {
        intVecIterator i = intentions.begin();
        if ((*i) != currentPos) {
            SG_LOG(SG_ATC, SG_ALERT,
                   "Error in FGTrafficRecord::setPositionAndIntentions at " << SG_ORIGIN << ", " << (*i));
        }
        intentions.erase(i);
    } else {
        //FGAIFlightPlan::waypoint* const wpt= route->getCurrentWaypoint();
        int size = route->getNrOfWayPoints();
        SG_LOG(SG_ATC, SG_DEBUG, "Setting pos" << currentPos);
        SG_LOG(SG_ATC, SG_DEBUG, "Setting intentions");
        for (int i = 2; i < size; i++) {
            int val = route->getRouteIndex(i);
            intentions.push_back(val);
        }
    }
}

void FGTrafficRecord::setAircraft(FGAIAircraft *ref)
{
    aircraft = ref;
}

FGAIAircraft* FGTrafficRecord::getAircraft() const
{
    return aircraft.ptr();
}

/*
* Check if another aircraft is ahead of the current one, and on the same taxiway
* @return true / false if this is/isn't the case.
*/
bool FGTrafficRecord::checkPositionAndIntentions(FGTrafficRecord & other)
{
    bool result = false;
    SG_LOG(SG_ATC, SG_BULK, "Start check 1");
    if (currentPos == other.currentPos) {
        SG_LOG(SG_ATC, SG_BULK, ": Check Position and intentions: we are on the same taxiway; Index = " << currentPos);
        result = true;
    }
    //  else if (! other.intentions.empty())
    //     {
    //       SG_LOG(SG_ATC, SG_BULK, "Start check 2");
    //       intVecIterator i = other.intentions.begin();
    //       while (!((i == other.intentions.end()) || ((*i) == currentPos)))
    //     i++;
    //       if (i != other.intentions.end()) {
    //    SG_LOG(SG_ATC, SG_BULK, "Check Position and intentions: current matches other.intentions");
    //     result = true;
    //       }
    else if (! intentions.empty()) {
        SG_LOG(SG_ATC, SG_BULK, "Start check 3");
        intVecIterator i = intentions.begin();
        //while (!((i == intentions.end()) || ((*i) == other.currentPos)))
        while (i != intentions.end()) {
            if ((*i) == other.currentPos) {
                break;
            }
            i++;
        }
        if (i != intentions.end()) {
            SG_LOG(SG_ATC, SG_BULK, ": Check Position and intentions: .other.current matches Index = " << (*i));
            result = true;
        }
    }
    SG_LOG(SG_ATC, SG_BULK, "Done!");
    return result;
}

void FGTrafficRecord::setPositionAndHeading(double lat, double lon,
        double hdg, double spd,
        double alt)
{
    latitude = lat;
    longitude = lon;
    heading = hdg;
    speed = spd;
    altitude = alt;
}

int FGTrafficRecord::crosses(FGGroundNetwork * net,
                             FGTrafficRecord & other)
{
    if (checkPositionAndIntentions(other)
            || (other.checkPositionAndIntentions(*this)))
        return -1;
    intVecIterator i, j;
    int currentTargetNode = 0, otherTargetNode = 0;
    if (currentPos > 0)
        currentTargetNode = net->findSegment(currentPos)->getEnd()->getIndex(); // OKAY,...
    if (other.currentPos > 0)
        otherTargetNode = net->findSegment(other.currentPos)->getEnd()->getIndex();     // OKAY,...
    if ((currentTargetNode == otherTargetNode) && currentTargetNode > 0)
        return currentTargetNode;
    if (! intentions.empty()) {
        for (i = intentions.begin(); i != intentions.end(); i++) {
            if ((*i) > 0) {
                if (currentTargetNode ==
                        net->findSegment(*i)->getEnd()->getIndex()) {
                    SG_LOG(SG_ATC, SG_BULK, "Current crosses at " << currentTargetNode);
                    return currentTargetNode;
                }
            }
        }
    }
    if (! other.intentions.empty()) {
        for (i = other.intentions.begin(); i != other.intentions.end();
                i++) {
            if ((*i) > 0) {
                if (otherTargetNode ==
                        net->findSegment(*i)->getEnd()->getIndex()) {
                    SG_LOG(SG_ATC, SG_BULK, "Other crosses at " << currentTargetNode);
                    return otherTargetNode;
                }
            }
        }
    }
    if (! intentions.empty() && ! other.intentions.empty()) {
        for (i = intentions.begin(); i != intentions.end(); i++) {
            for (j = other.intentions.begin(); j != other.intentions.end();
                    j++) {
                SG_LOG(SG_ATC, SG_BULK, "finding segment " << *i << " and " << *j);
                if (((*i) > 0) && ((*j) > 0)) {
                    currentTargetNode =
                        net->findSegment(*i)->getEnd()->getIndex();
                    otherTargetNode =
                        net->findSegment(*j)->getEnd()->getIndex();
                    if (currentTargetNode == otherTargetNode) {
                        SG_LOG(SG_ATC, SG_BULK, "Routes will cross at " << currentTargetNode);
                        return currentTargetNode;
                    }
                }
            }
        }
    }
    return -1;
}

bool FGTrafficRecord::onRoute(FGGroundNetwork * net,
                              FGTrafficRecord & other)
{
    int node = -1, othernode = -1;
    if (currentPos > 0)
        node = net->findSegment(currentPos)->getEnd()->getIndex();
    if (other.currentPos > 0)
        othernode =
            net->findSegment(other.currentPos)->getEnd()->getIndex();
    if ((node == othernode) && (node != -1))
        return true;
    if (! other.intentions.empty()) {
        for (intVecIterator i = other.intentions.begin();
                i != other.intentions.end(); i++) {
            if (*i > 0) {
                othernode = net->findSegment(*i)->getEnd()->getIndex();
                if ((node == othernode) && (node > -1))
                    return true;
            }
        }
    }
    //if (other.currentPos > 0)
    //  othernode = net->findSegment(other.currentPos)->getEnd()->getIndex();
    //if (! intentions.empty())
    //  {
    //    for (intVecIterator i = intentions.begin(); i != intentions.end(); i++)
    //    {
    //      if (*i > 0)
    //        {
    //          node = net->findSegment(*i)->getEnd()->getIndex();
    //          if ((node == othernode) && (node > -1))
    //            return true;
    //        }
    //    }
    //  }
    return false;
}


bool FGTrafficRecord::isOpposing(FGGroundNetwork * net,
                                 FGTrafficRecord & other, int node)
{
    // Check if current segment is the reverse segment for the other aircraft
    FGTaxiSegment *opp;
    SG_LOG(SG_ATC, SG_BULK, "Current segment " << currentPos);
    if ((currentPos > 0) && (other.currentPos > 0)) {
        opp = net->findSegment(currentPos)->opposite();
        if (opp) {
            if (opp->getIndex() == other.currentPos)
                return true;
        }

        for (intVecIterator i = intentions.begin(); i != intentions.end();
                i++) {
            if ((opp = net->findSegment(other.currentPos)->opposite())) {
                if ((*i) > 0)
                    if (opp->getIndex() ==
                            net->findSegment(*i)->getIndex()) {
                        if (net->findSegment(*i)->getStart()->getIndex() ==
                                node) {
                            {
                                SG_LOG(SG_ATC, SG_BULK, "Found the node " << node);
                                return true;
                            }
                        }
                    }
            }
            if (! other.intentions.empty()) {
                for (intVecIterator j = other.intentions.begin();
                        j != other.intentions.end(); j++) {
                    SG_LOG(SG_ATC, SG_BULK, "Current segment 1 " << (*i));
                    if ((*i) > 0) {
                        if ((opp = net->findSegment(*i)->opposite())) {
                            if (opp->getIndex() ==
                                    net->findSegment(*j)->getIndex()) {
                                SG_LOG(SG_ATC, SG_BULK, "Nodes " << net->findSegment(*i)->getIndex()
                                   << " and  " << net->findSegment(*j)->getIndex()
                                    << " are opposites ");
                                if (net->findSegment(*i)->getStart()->
                                        getIndex() == node) {
                                    {
                                        SG_LOG(SG_ATC, SG_BULK, "Found the node " << node);
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool FGTrafficRecord::isActive(int margin) const
{
    if (aircraft->getDie()) {
        return false;
    }
    
    time_t now = globals->get_time_params()->get_cur_time();
    time_t deptime = aircraft->getTrafficRef()->getDepartureTime();
    return ((now + margin) > deptime);
}


void FGTrafficRecord::setSpeedAdjustment(double spd)
{
    instruction.setChangeSpeed(true);
    instruction.setSpeed(spd);
}

void FGTrafficRecord::setHeadingAdjustment(double heading)
{
    instruction.setChangeHeading(true);
    instruction.setHeading(heading);
}

bool FGTrafficRecord::pushBackAllowed() const
{
    return allowPushback;
}



/***************************************************************************
 * FGATCInstruction
 * 
 **************************************************************************/
 
FGATCInstruction::FGATCInstruction()
{
    holdPattern = false;
    holdPosition = false;
    changeSpeed = false;
    changeHeading = false;
    changeAltitude = false;
    resolveCircularWait = false;

    speed = 0;
    heading = 0;
    alt = 0;
}

bool FGATCInstruction::hasInstruction() const
{
    return (holdPattern || holdPosition || changeSpeed || changeHeading
            || changeAltitude || resolveCircularWait);
}



/***************************************************************************
 * FGATCController
 *
 **************************************************************************/

FGATCController::FGATCController()
{
    dt_count = 0;
    available = true;
    lastTransmission = 0;
    initialized = false;
    lastTransmissionDirection = ATC_AIR_TO_GROUND;
    group = NULL;
}

FGATCController::~FGATCController()
{
    auto mgr = globals->get_subsystem<FGATCManager>();
    mgr->removeController(this);
}

void FGATCController::init()
{
    if (!initialized) {
        auto mgr = globals->get_subsystem<FGATCManager>();
        mgr->addController(this);
        initialized = true;
    }
}

string FGATCController::getGateName(FGAIAircraft * ref)
{
    return ref->atGate();
}

bool FGATCController::isUserAircraft(FGAIAircraft* ac)
{
    return (ac->getCallSign() == fgGetString("/sim/multiplay/callsign")) ? true : false;
};

void FGATCController::transmit(FGTrafficRecord * rec, FGAirportDynamics *parent, AtcMsgId msgId,
                               AtcMsgDir msgDir, bool audible)
{
    string sender, receiver;
    int stationFreq = 0;
    int taxiFreq = 0;
    int towerFreq = 0;
    int freqId = 0;
    string atisInformation;
    string text;
    string taxiFreqStr;
    string towerFreqStr;
    double heading = 0;
    string activeRunway;
    string fltType;
    string rwyClass;
    string SID;
    string transponderCode;
    FGAIFlightPlan *fp;
    string fltRules;
    string instructionText;
    int ground_to_air=0;

    //double commFreqD;
    sender = rec->getAircraft()->getTrafficRef()->getCallSign();
    if (rec->getAircraft()->getTaxiClearanceRequest()) {
        instructionText = "push-back and taxi";
    } else {
        instructionText = "taxi";
    }
   
    SG_LOG(SG_ATC, SG_DEBUG, "transmitting for: " << sender << "Leg = " << rec->getLeg());

    auto depApt = rec->getAircraft()->getTrafficRef()->getDepartureAirport();

    switch (rec->getLeg()) {
    case 1:
    case 2:
        // avoid crash FLIGHTGEAR-ER
        if (!depApt) {
            SG_LOG(SG_ATC, SG_DEV_ALERT, "TrafficRec has empty departure airport, can't transmit");
            return;
        }

        freqId = rec->getNextFrequency();
        stationFreq = depApt->getDynamics()->getGroundFrequency(rec->getLeg() + freqId);
        taxiFreq = depApt->getDynamics()->getGroundFrequency(2);
        towerFreq = depApt->getDynamics()->getTowerFrequency(2);
        receiver = depApt->getName() + "-Ground";
        atisInformation = depApt->getDynamics()->getAtisSequence();
        break;
    case 3:
        if (!depApt) {
            SG_LOG(SG_ATC, SG_DEV_ALERT, "TrafficRec has empty departure airport, can't transmit");
            return;
        }

        receiver = depApt->getName() + "-Tower";
        break;
    }
    
    // Swap sender and receiver value in case of a ground to air transmission
    if (msgDir == ATC_GROUND_TO_AIR) {
        string tmp = sender;
        sender = receiver;
        receiver = tmp;
        ground_to_air = 1;
    }
    
    switch (msgId) {
    case MSG_ANNOUNCE_ENGINE_START:
        text = sender + ". Ready to Start up.";
        break;
    case MSG_REQUEST_ENGINE_START:
        text =
            receiver + ", This is " + sender + ". Position " +
            getGateName(rec->getAircraft()) + ". Information " +
            atisInformation + ". " +
            rec->getAircraft()->getTrafficRef()->getFlightRules() +
            " to " +
            rec->getAircraft()->getTrafficRef()->getArrivalAirport()->
            getName() + ". Request start-up.";
        break;
        // Acknowledge engine startup permission
        // Assign departure runway
        // Assign SID, if necessery (TODO)
    case MSG_PERMIT_ENGINE_START:
        taxiFreqStr = formatATCFrequency3_2(taxiFreq);

        heading = rec->getAircraft()->getTrafficRef()->getCourse();
        fltType = rec->getAircraft()->getTrafficRef()->getFlightType();
        rwyClass =
            rec->getAircraft()->GetFlightPlan()->
            getRunwayClassFromTrafficType(fltType);

        rec->getAircraft()->getTrafficRef()->getDepartureAirport()->
        getDynamics()->getActiveRunway(rwyClass, 1, activeRunway,
                                       heading);
        rec->getAircraft()->GetFlightPlan()->setRunway(activeRunway);
        fp = NULL;
        rec->getAircraft()->GetFlightPlan()->setSID(fp);
        if (fp) {
            SID = fp->getName() + " departure";
        } else {
            SID = "fly runway heading ";
        }
        //snprintf(buffer, 7, "%3.2f", heading);
        fltRules = rec->getAircraft()->getTrafficRef()->getFlightRules();
        transponderCode = genTransponderCode(fltRules);
        rec->getAircraft()->SetTransponderCode(transponderCode);
        text =
            receiver + ". Start-up approved. " + atisInformation +
            " correct, runway " + activeRunway + ", " + SID + ", squawk " +
            transponderCode + ". " +
            "For "+ instructionText + " clearance call " + taxiFreqStr + ". " +
            sender + " control.";
        break;
    case MSG_DENY_ENGINE_START:
        text = receiver + ". Standby.";
        break;
    case MSG_ACKNOWLEDGE_ENGINE_START:
        fp = rec->getAircraft()->GetFlightPlan()->getSID();
        if (fp) {
            SID =
                rec->getAircraft()->GetFlightPlan()->getSID()->getName() +
                " departure";
        } else {
            SID = "fly runway heading ";
        }
        taxiFreqStr = formatATCFrequency3_2(taxiFreq);
        activeRunway = rec->getAircraft()->GetFlightPlan()->getRunway();
        transponderCode = rec->getAircraft()->GetTransponderCode();

        text =
            receiver + ". Start-up approved. " + atisInformation +
            " correct, runway " + activeRunway + ", " + SID + ", squawk " +
            transponderCode + ". " +
            "For " + instructionText + " clearance call " + taxiFreqStr + ". " +
            sender + ".";
        break;
    case MSG_ACKNOWLEDGE_SWITCH_GROUND_FREQUENCY:
        taxiFreqStr = formatATCFrequency3_2(taxiFreq);
        text = receiver + ". Switching to " + taxiFreqStr + ". " + sender + ".";
        break;
    case MSG_INITIATE_CONTACT:
        text = receiver + ". With you. " + sender + ".";
        break;
    case MSG_ACKNOWLEDGE_INITIATE_CONTACT:
        text = receiver + ". Roger. " + sender + ".";
        break;
    case MSG_REQUEST_PUSHBACK_CLEARANCE:
        if (rec->getAircraft()->getTaxiClearanceRequest()) {
            text = receiver + ". Request push-back. " + sender + ".";
        } else {
            text = receiver + ". Request Taxi clearance. " + sender + ".";
        }
        break;
    case MSG_PERMIT_PUSHBACK_CLEARANCE:
        if (rec->getAircraft()->getTaxiClearanceRequest()) {
            text = receiver + ". Push-back approved. " + sender + ".";
        } else {
            text = receiver + ". Cleared to Taxi. " + sender + "."; 
        }
        break;
    case MSG_HOLD_PUSHBACK_CLEARANCE:
        text = receiver + ". Standby. " + sender + ".";
        break;
    case MSG_REQUEST_TAXI_CLEARANCE:
        text = receiver + ". Ready to Taxi. " + sender + ".";
        break;
    case MSG_ISSUE_TAXI_CLEARANCE:
        text = receiver + ". Cleared to taxi. " + sender + ".";
        break;
    case MSG_ACKNOWLEDGE_TAXI_CLEARANCE:
        text = receiver + ". Cleared to taxi. " + sender + ".";
        break;
    case MSG_HOLD_POSITION:
        text = receiver + ". Hold Position. " + sender + ".";
        break;
    case MSG_ACKNOWLEDGE_HOLD_POSITION:
        text = receiver + ". Holding Position. " + sender + ".";
        break;
    case MSG_RESUME_TAXI:
        text = receiver + ". Resume Taxiing. " + sender + ".";
        break;
    case MSG_ACKNOWLEDGE_RESUME_TAXI:
        text = receiver + ". Continuing Taxi. " + sender + ".";
        break;
    case MSG_REPORT_RUNWAY_HOLD_SHORT:
        activeRunway = rec->getAircraft()->GetFlightPlan()->getRunway();
        //activeRunway = "test";
        text = receiver + ". Holding short runway "
               + activeRunway
               + ". " + sender + ".";
        //text = "test1";
        SG_LOG(SG_ATC, SG_DEBUG, "1 Currently at leg " << rec->getLeg());
        break;
    case MSG_ACKNOWLEDGE_REPORT_RUNWAY_HOLD_SHORT:
        activeRunway = rec->getAircraft()->GetFlightPlan()->getRunway();
        text = receiver + " Roger. Holding short runway "
               //                + activeRunway
               + ". " + sender + ".";
        //text = "test2";
        SG_LOG(SG_ATC, SG_DEBUG, "2 Currently at leg " << rec->getLeg());
        break;
    case MSG_SWITCH_TOWER_FREQUENCY:
        towerFreqStr = formatATCFrequency3_2(towerFreq);
        text = receiver + " Contact Tower at " + towerFreqStr + ". " + sender + ".";
        //text = "test3";
        SG_LOG(SG_ATC, SG_DEBUG, "3 Currently at leg " << rec->getLeg());
        break;
    case MSG_ACKNOWLEDGE_SWITCH_TOWER_FREQUENCY:
        towerFreqStr = formatATCFrequency3_2(towerFreq);
        text = receiver + " Roger, switching to tower at " + towerFreqStr + ". " + sender + ".";
        //text = "test4";
        SG_LOG(SG_ATC, SG_DEBUG, "4 Currently at leg " << rec->getLeg());
        break;
    default:
        //text = "test3";
        text = text + sender + ". Transmitting unknown Message.";
        break;
    }
    
    if (audible) {
        double onBoardRadioFreq0 =
            fgGetDouble("/instrumentation/comm[0]/frequencies/selected-mhz");
        double onBoardRadioFreq1 =
            fgGetDouble("/instrumentation/comm[1]/frequencies/selected-mhz");
        int onBoardRadioFreqI0 = (int) floor(onBoardRadioFreq0 * 100 + 0.5);
        int onBoardRadioFreqI1 = (int) floor(onBoardRadioFreq1 * 100 + 0.5);
        SG_LOG(SG_ATC, SG_DEBUG, "Using " << onBoardRadioFreq0 << ", " << onBoardRadioFreq1 << " and " << stationFreq << " for " << text << endl);

        // Display ATC message only when one of the radios is tuned
        // the relevant frequency.
        // Note that distance attenuation is currently not yet implemented
                
        if ((stationFreq > 0)&&
            ((onBoardRadioFreqI0 == stationFreq)||
             (onBoardRadioFreqI1 == stationFreq))) {
            if (rec->allowTransmissions()) {
                
                if( fgGetBool( "/sim/radio/use-itm-attenuation", false ) ) {
                    SG_LOG(SG_ATC, SG_DEBUG, "Using ITM radio propagation");
                    FGRadioTransmission* radio = new FGRadioTransmission();
                    SGGeod sender_pos;
                    double sender_alt_ft, sender_alt;
                    if(ground_to_air) {
                  sender_pos = parent->parent()->geod();
                     }
                    else {
                          sender_alt_ft = rec->getAltitude();
                          sender_alt = sender_alt_ft * SG_FEET_TO_METER;
                          sender_pos= SGGeod::fromDegM( rec->getLongitude(),
                                 rec->getLatitude(), sender_alt );
                    }
                    double frequency = ((double)stationFreq) / 100;
                    radio->receiveATC(sender_pos, frequency, text, ground_to_air);
                    delete radio;
                }
                else {
                    fgSetString("/sim/messages/atc", text.c_str());
                }
            }
        }
    } else {
        FGATCDialogNew::instance()->addEntry(1, text);
    }
}

/*
* Format integer frequency xxxyy as xxx.yy
* @param freq - integer value
* @return the formatted string
*/
string FGATCController::formatATCFrequency3_2(int freq)
{
    char buffer[7]; // does this ever need to be freed?
    snprintf(buffer, 7, "%3.2f", ((float) freq / 100.0));
    return string(buffer);
}

// TODO: Set transponder codes according to real-world routes.
// The current version just returns a random string of four octal numbers.
string FGATCController::genTransponderCode(const string& fltRules)
{
    if (fltRules == "VFR") {
        return string("1200");
    } else {
        char buffer[5];
        snprintf(buffer, sizeof(buffer), "%d%d%d%d", rand() % 8, rand() % 8, rand() % 8,
                 rand() % 8);
        return string(buffer);
    }
}

void FGATCController::eraseDeadTraffic(TrafficVector& vec)
{
    auto it = std::remove_if(vec.begin(), vec.end(), [](const FGTrafficRecord& traffic)
    {
        if (!traffic.getAircraft()) {
            return true;
        }
        return traffic.getAircraft()->getDie();
    });
    vec.erase(it, vec.end());
}



/***************************************************************************
 * class FGTowerController
 * subclass of FGATCController
 **************************************************************************/
 
FGTowerController::FGTowerController(FGAirportDynamics *par) :
        FGATCController()
{
    parent = par;
}

FGTowerController::~FGTowerController()
{
    // to avoid the exception described in:
    // https://sourceforge.net/p/flightgear/codetickets/1864/
    // we want to ensure AI aircraft signing-off is a no-op now

    clearTrafficControllers(activeTraffic);
}

//
void FGTowerController::announcePosition(int id,
        FGAIFlightPlan * intendedRoute,
        int currentPosition, double lat,
        double lon, double heading,
        double speed, double alt,
        double radius, int leg,
        FGAIAircraft * ref)
{
    init();
	
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    // Add a new TrafficRecord if no one exsists for this aircraft.
    if (i == activeTraffic.end() || (activeTraffic.empty())) {
        FGTrafficRecord rec;
        rec.setId(id);

        rec.setPositionAndHeading(lat, lon, heading, speed, alt);
        rec.setRunway(intendedRoute->getRunway());
        rec.setLeg(leg);
        //rec.setCallSign(callsign);
        rec.setRadius(radius);
        rec.setAircraft(ref);
        activeTraffic.push_back(rec);
        // Don't just schedule the aircraft for the tower controller, also assign if to the correct active runway.
        ActiveRunwayVecIterator rwy = activeRunways.begin();
        if (! activeRunways.empty()) {
            while (rwy != activeRunways.end()) {
                if (rwy->getRunwayName() == intendedRoute->getRunway()) {
                    break;
                }
                rwy++;
            }
        }
        if (rwy == activeRunways.end()) {
            ActiveRunway aRwy(intendedRoute->getRunway(), id);
            aRwy.addTodepartureQueue(ref);
            activeRunways.push_back(aRwy);
            rwy = (activeRunways.end()-1);
        } else {
            rwy->addTodepartureQueue(ref);
        }

        SG_LOG(SG_ATC, SG_DEBUG, ref->getTrafficRef()->getCallSign() << " You are number " << rwy->getdepartureQueueSize() << " for takeoff ");
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
    }
}

void FGTowerController::updateAircraftInformation(int id, double lat, double lon,
        double heading, double speed, double alt,
        double dt)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    setDt(getDt() + dt);

    if (i == activeTraffic.end() || (activeTraffic.empty())) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: updating aircraft without traffic record at " <<
               SG_ORIGIN);
        return;
    }

    // Update the position of the current aircraft
    FGTrafficRecord& current = *i;
    current.setPositionAndHeading(lat, lon, heading, speed, alt);

    // see if we already have a clearance record for the currently active runway
    // NOTE: dd. 2011-08-07: Because the active runway has been constructed in the announcePosition function, we may safely assume that is
    // already exists here. So, we can simplify the current code.
    
    ActiveRunwayVecIterator rwy = activeRunways.begin();
    //if (parent->getId() == fgGetString("/sim/presets/airport-id")) {
    //    for (rwy = activeRunways.begin(); rwy != activeRunways.end(); rwy++) {
    //        rwy->printdepartureQueue();
    //    }
    //}
    
    rwy = activeRunways.begin();
    while (rwy != activeRunways.end()) {
        if (rwy->getRunwayName() == current.getRunway()) {
            break;
        }
        rwy++;
    }

    // only bother running the following code if the current aircraft is the
    // first in line for depature
    /* if (current.getAircraft() == rwy->getFirstAircraftIndepartureQueue()) {
        if (rwy->getCleared()) {
            if (id == rwy->getCleared()) {
                current.setHoldPosition(false);
            } else {
                current.setHoldPosition(true);
            }
        } else {
            // For now. At later stages, this will probably be the place to check for inbound traffc.
            rwy->setCleared(id);
        }
    } */
    // only bother with aircraft that have a takeoff status of 2, since those are essentially under tower control
    FGAIAircraft* ac= rwy->getFirstAircraftIndepartureQueue();
    if (ac->getTakeOffStatus() == 1) {
        // transmit takeoff clearance
        ac->setTakeOffStatus(2);
    }
    if (current.getAircraft()->getTakeOffStatus() == 2) {
        current.setHoldPosition(false);
    } else {
        current.setHoldPosition(true);
    }
    int clearanceId = rwy->getCleared();
    if (clearanceId) {
        if (id == clearanceId) {
            current.setHoldPosition(false);
        }
    } else {
        if (current.getAircraft() == rwy->getFirstAircraftIndepartureQueue()) {
            rwy->setCleared(id);
            FGAIAircraft *ac = rwy->getFirstOfStatus(1);
            if (ac)
                ac->setTakeOffStatus(2);
                // transmit takeoff clearacne? But why twice?
        }
    }
} 


void FGTowerController::signOff(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    if (i == activeTraffic.end() || (activeTraffic.empty())) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off from tower at " << SG_ORIGIN);
        return;
    }

    const auto trafficRunway = i->getRunway();
    auto runwayIt = std::find_if(activeRunways.begin(), activeRunways.end(),
                                 [&trafficRunway](const ActiveRunway& ar) {
                                     return ar.getRunwayName() == trafficRunway;
                                 });

    if (runwayIt != activeRunways.end()) {
        runwayIt->setCleared(0);
        runwayIt->updatedepartureQueue();
    } else {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Attempting to erase non-existing runway clearance record in FGTowerController::signoff at " << SG_ORIGIN);
    }

    i->getAircraft()->resetTakeOffStatus();
    activeTraffic.erase(i);
    SG_LOG(SG_ATC, SG_DEBUG, "Signing off from tower controller");
}

// NOTE:
// IF WE MAKE TRAFFICRECORD A MEMBER OF THE BASE CLASS
// THE FOLLOWING THREE FUNCTIONS: SIGNOFF, HAS INSTRUCTION AND GETINSTRUCTION CAN
// BECOME DEVIRTUALIZED AND BE A MEMBER OF THE BASE ATCCONTROLLER CLASS
// WHICH WOULD SIMPLIFY CODE MAINTENANCE.
// Note that this function is probably obsolete
bool FGTowerController::hasInstruction(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->hasInstruction();
    }
    return false;
}


FGATCInstruction FGTowerController::getInstruction(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->getInstruction();
    }
    return FGATCInstruction();
}

void FGTowerController::render(bool visible) {
    // this should be bulk, since its called quite often
    SG_LOG(SG_ATC, SG_BULK, "FGTowerController::render function not yet implemented");
}

string FGTowerController::getName() {
    return string(parent->getId() + "-tower");
}

void FGTowerController::update(double dt)
{
    eraseDeadTraffic(activeTraffic);
}



/***************************************************************************
 * class FGStartupController
 * subclass of FGATCController
 **************************************************************************/
FGStartupController::FGStartupController(FGAirportDynamics *par):
        FGATCController()
{
    parent = par;
}

FGStartupController::~FGStartupController()
{
    clearTrafficControllers(activeTraffic);
}

void FGStartupController::announcePosition(int id,
        FGAIFlightPlan * intendedRoute,
        int currentPosition, double lat,
        double lon, double heading,
        double speed, double alt,
        double radius, int leg,
        FGAIAircraft * ref)
{
    init();
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    // Add a new TrafficRecord if no one exsists for this aircraft.
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        FGTrafficRecord rec;
        rec.setId(id);

        rec.setPositionAndHeading(lat, lon, heading, speed, alt);
        rec.setRunway(intendedRoute->getRunway());
        rec.setLeg(leg);
        rec.setPositionAndIntentions(currentPosition, intendedRoute);
        //rec.setCallSign(callsign);
        rec.setAircraft(ref);
        rec.setHoldPosition(true);
        activeTraffic.push_back(rec);
    } else {
        i->setPositionAndIntentions(currentPosition, intendedRoute);
        i->setPositionAndHeading(lat, lon, heading, speed, alt);

    }
}

// NOTE:
// IF WE MAKE TRAFFICRECORD A MEMBER OF THE BASE CLASS
// THE FOLLOWING THREE FUNCTIONS: SIGNOFF, HAS INSTRUCTION AND GETINSTRUCTION CAN
// BECOME DEVIRTUALIZED AND BE A MEMBER OF THE BASE ATCCONTROLLER CLASS
// WHICH WOULD SIMPLIFY CODE MAINTENANCE.
// Note that this function is probably obsolete
bool FGStartupController::hasInstruction(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->hasInstruction();
    }
    return false;
}


FGATCInstruction FGStartupController::getInstruction(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->getInstruction();
    }
    return FGATCInstruction();
}

void FGStartupController::signOff(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off from tower at " << SG_ORIGIN);
    } else {
        SG_LOG(SG_ATC, SG_DEBUG, i->getAircraft()->getCallSign() << " signing off from startupcontroller");
        i = activeTraffic.erase(i);
    }
}

bool FGStartupController::checkTransmissionState(int st, time_t now, time_t startTime, TrafficVectorIterator i, AtcMsgId msgId,
        AtcMsgDir msgDir)
{
    int state = i->getState();
    if ((state == st) && available) {
        if ((msgDir == ATC_AIR_TO_GROUND) && isUserAircraft(i->getAircraft())) {

            SG_LOG(SG_ATC, SG_BULK, "Checking state " << st << " for " << i->getAircraft()->getCallSign());
            SGPropertyNode_ptr trans_num = globals->get_props()->getNode("/sim/atc/transmission-num", true);
            int n = trans_num->getIntValue();
            if (n == 0) {
                trans_num->setIntValue(-1);
                // PopupCallback(n);
                SG_LOG(SG_ATC, SG_BULK, "Selected transmission message " << n);
                FGATCDialogNew::instance()->removeEntry(1);
            } else {
                SG_LOG(SG_ATC, SG_BULK, "Creating message for " << i->getAircraft()->getCallSign());
                transmit(&(*i), &(*parent), msgId, msgDir, false);
                return false;
            }
        }
        if (now > startTime) {
            SG_LOG(SG_ATC, SG_BULK, "Transmitting startup msg");
            transmit(&(*i), &(*parent), msgId, msgDir, true);
            i->updateState();
            lastTransmission = now;
            available = false;
            return true;
        }
    }
    return false;
}

void FGStartupController::updateAircraftInformation(int id, double lat, double lon,
        double heading, double speed, double alt,
        double dt)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
	TrafficVectorIterator current, closest;
	
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: updating aircraft without traffic record at " << SG_ORIGIN);
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
        current = i;
    }
    setDt(getDt() + dt);

    int state = i->getState();

    // Sentry FLIGHTGEAR-2Q : don't crash on null TrafficRef
    if (!i->getAircraft()->getTrafficRef()) {
        SG_LOG(SG_ATC, SG_ALERT, "AI traffic: updating aircraft without traffic ref");
        return;
    }

    // The user controlled aircraft should have crased here, because it doesn't have a traffic reference.
    // NOTE: if we create a traffic schedule for the user aircraft, we can use this to plan a flight.
    time_t startTime = i->getAircraft()->getTrafficRef()->getDepartureTime();
    time_t now = globals->get_time_params()->get_cur_time();

    SG_LOG(SG_ATC, SG_BULK, i->getAircraft()->getTrafficRef()->getCallSign()
         << " is scheduled to depart in " << startTime-now << " seconds. Available = " << available
         << " at parking " << getGateName(i->getAircraft()));

    if ((now - lastTransmission) > 3 + (rand() % 15)) {
        available = true;
    }

    checkTransmissionState(0, now, (startTime + 0  ), i, MSG_ANNOUNCE_ENGINE_START,                     ATC_AIR_TO_GROUND);
    checkTransmissionState(1, now, (startTime + 60 ), i, MSG_REQUEST_ENGINE_START,                      ATC_AIR_TO_GROUND);
    checkTransmissionState(2, now, (startTime + 80 ), i, MSG_PERMIT_ENGINE_START,                       ATC_GROUND_TO_AIR);
    checkTransmissionState(3, now, (startTime + 100), i, MSG_ACKNOWLEDGE_ENGINE_START,                  ATC_AIR_TO_GROUND);
    if (checkTransmissionState(4, now, (startTime + 130), i, MSG_ACKNOWLEDGE_SWITCH_GROUND_FREQUENCY,       ATC_AIR_TO_GROUND)) {
        i->nextFrequency();
    }
    checkTransmissionState(5, now, (startTime + 140), i, MSG_INITIATE_CONTACT,                          ATC_AIR_TO_GROUND);
    checkTransmissionState(6, now, (startTime + 150), i, MSG_ACKNOWLEDGE_INITIATE_CONTACT,              ATC_GROUND_TO_AIR);
    checkTransmissionState(7, now, (startTime + 180), i, MSG_REQUEST_PUSHBACK_CLEARANCE,                ATC_AIR_TO_GROUND);



    if ((state == 8) && available) {
        if (now > startTime + 200) {
            if (i->pushBackAllowed()) {
                i->allowRepeatedTransmissions();
                transmit(&(*i), &(*parent), MSG_PERMIT_PUSHBACK_CLEARANCE,
                         ATC_GROUND_TO_AIR, true);
                i->updateState();
            } else {
                transmit(&(*i), &(*parent), MSG_HOLD_PUSHBACK_CLEARANCE,
                         ATC_GROUND_TO_AIR, true);
                i->suppressRepeatedTransmissions();
            }
            lastTransmission = now;
            available = false;
        }
    }
    if ((state == 9) && available) {
        i->setHoldPosition(false);
    }
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


void FGStartupController::render(bool visible)
{
    SG_LOG(SG_ATC, SG_DEBUG, "Rendering startup controller");
    SGMaterialLib *matlib = globals->get_matlib();
    if (group) {
        //int nr = ;
        globals->get_scenery()->get_scene_graph()->removeChild(group);
        //while (group->getNumChildren()) {
        // SG_LOG(SG_ATC, SG_BULK, "Number of children: " << group->getNumChildren());
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
        //double elevation_meters = 0.0;
        //double elevation_feet = 0.0;

        FGGroundNetwork* groundNet = parent->parent()->groundNetwork();

        //for ( FGTaxiSegmentVectorIterator i = segments.begin(); i != segments.end(); i++) {
        double dx = 0;
        time_t now = globals->get_time_params()->get_cur_time();

        for   (TrafficVectorIterator i = activeTraffic.begin(); i != activeTraffic.end(); i++) {
            if (i->isActive(300)) {
                // Handle start point
                int pos = i->getCurrentPosition();
                SG_LOG(SG_ATC, SG_BULK, "rendering for " << i->getAircraft()->getCallSign() << "pos = " << pos);
                if (pos > 0) {
                    FGTaxiSegment *segment = groundNet->findSegment(pos);
                    SGGeod start(SGGeod::fromDeg((i->getLongitude()), (i->getLatitude())));
                    SGGeod end  (segment->getEnd()->geod());

                    double length = SGGeodesy::distanceM(start, end);
                    //heading = SGGeodesy::headingDeg(start->geod(), end->geod());

                    double az2, heading; //, distanceM;
                    SGGeodesy::inverse(start, end, heading, az2, length);
                    double coveredDistance = length * 0.5;
                    SGGeod center;
                    SGGeodesy::direct(start, heading, coveredDistance, center, az2);
                    SG_LOG(SG_ATC, SG_BULK, "Active Aircraft : Centerpoint = (" << center.getLatitudeDeg() << ", " << center.getLongitudeDeg() << "). Heading = " << heading);
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
                        elevationStart = ((i)->getAircraft()->_getAltitude() * SG_FEET_TO_METER);
                    }
                    double elevationEnd   = segment->getEnd()->getElevationM();
                    if ((elevationEnd == 0) || (elevationEnd == parent->getElevation())) {
                        SGGeod center2 = end;
                        center2.setElevationM(SG_MAX_ELEVATION_M);
                        if (local_scenery->get_elevation_m( center2, elevationEnd, NULL )) {
                            //elevation_feet = elevationEnd * SG_METER_TO_FEET + 0.5;
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

                    SG_LOG(SG_ATC, SG_BULK, "1. Using mean elevation : " << elevationMean << " and " << slope);

                    WorldCoordinate( obj_pos, center.getLatitudeDeg(), center.getLongitudeDeg(), elevationMean + 0.5 + dx, -(heading), slope );
                    ;

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
                    SG_LOG(SG_ATC, SG_DEBUG, "BIG FAT WARNING: current position is here : " << pos);
                }
                for (intVecIterator j = (i)->getIntentions().begin(); j != (i)->getIntentions().end(); j++) {
                    osg::Matrix obj_pos;
                    int k = (*j);
                    if (k > 0) {
                        SG_LOG(SG_ATC, SG_BULK, "rendering for " << i->getAircraft()->getCallSign() << "intention = " << k);
                        osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
                        obj_trans->setDataVariance(osg::Object::STATIC);
                        FGTaxiSegment *segment  = groundNet->findSegment(k);

                        double elevationStart = segment->getStart()->getElevationM();
                        double elevationEnd   = segment->getEnd  ()->getElevationM();
                        if ((elevationStart == 0) || (elevationStart == parent->getElevation())) {
                            SGGeod center2 = segment->getStart()->geod();
                            center2.setElevationM(SG_MAX_ELEVATION_M);
                            if (local_scenery->get_elevation_m( center2, elevationStart, NULL )) {
                                //elevation_feet = elevationStart * SG_METER_TO_FEET + 0.5;
                                //elevation_meters += 0.5;
                            }
                            else {
                                elevationStart = parent->getElevation();
                            }
                            segment->getStart()->setElevation(elevationStart);
                        }
                        if ((elevationEnd == 0) || (elevationEnd == parent->getElevation())) {
                            SGGeod center2 = segment->getEnd()->geod();
                            center2.setElevationM(SG_MAX_ELEVATION_M);
                            if (local_scenery->get_elevation_m( center2, elevationEnd, NULL )) {
                                //elevation_feet = elevationEnd * SG_METER_TO_FEET + 0.5;
                                //elevation_meters += 0.5;
                            }
                            else {
                                elevationEnd = parent->getElevation();
                            }
                            segment->getEnd()->setElevation(elevationEnd);
                        }

                        double elevationMean  = (elevationStart + elevationEnd) / 2.0;
                        double elevDiff       = elevationEnd - elevationStart;
                        double length         = segment->getLength();
                        double slope = atan2(elevDiff, length) * SGD_RADIANS_TO_DEGREES;

                        SG_LOG(SG_ATC, SG_BULK, "2. Using mean elevation : " << elevationMean << " and " << slope);

                        SGGeod segCenter(segment->getCenter());
                        WorldCoordinate( obj_pos, segCenter.getLatitudeDeg(),
                                        segCenter.getLongitudeDeg(), elevationMean + 0.5 + dx, -(segment->getHeading()), slope );

                        //WorldCoordinate( obj_pos, segment->getLatitude(), segment->getLongitude(), parent->getElevation()+8+dx, -(segment->getHeading()) );

                        obj_trans->setMatrix( obj_pos );
                        //osg::Vec3 center(0, 0, 0)

                        float width = segment->getLength() /2.0;
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
                    } else {
                        SG_LOG(SG_ATC, SG_DEBUG, "BIG FAT WARNING: k is here : " << pos);
                    }
                }
                dx += 0.2;
            }
        }
        globals->get_scenery()->get_scene_graph()->addChild(group);
    }
}

string FGStartupController::getName() {
    return string(parent->getId() + "-startup");
}

void FGStartupController::update(double dt)
{
    eraseDeadTraffic(activeTraffic);
}



/***************************************************************************
 * class FGApproachController
 * subclass of FGATCController
 **************************************************************************/
 
FGApproachController::FGApproachController(FGAirportDynamics *par):
        FGATCController()
{
    parent = par;
}

FGApproachController::~FGApproachController()
{
    clearTrafficControllers(activeTraffic);
}


void FGApproachController::announcePosition(int id,
        FGAIFlightPlan * intendedRoute,
        int currentPosition,
        double lat, double lon,
        double heading, double speed,
        double alt, double radius,
        int leg, FGAIAircraft * ref)
{
    init();
    
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    // Add a new TrafficRecord if no one exsists for this aircraft.
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        FGTrafficRecord rec;
        rec.setId(id);

        rec.setPositionAndHeading(lat, lon, heading, speed, alt);
        rec.setRunway(intendedRoute->getRunway());
        rec.setLeg(leg);
        //rec.setCallSign(callsign);
        rec.setAircraft(ref);
        activeTraffic.push_back(rec);
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
    }
}

void FGApproachController::updateAircraftInformation(int id, double lat, double lon,
        double heading, double speed, double alt,
        double dt)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    TrafficVectorIterator current;
	
    // update position of the current aircraft
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: updating aircraft without traffic record at " << SG_ORIGIN);
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
        current = i;
        SG_LOG(SG_ATC, SG_BULK, "ApproachController: checking for speed");
        time_t time_diff =
            current->getAircraft()->
            checkForArrivalTime(string("final001"));
        if (time_diff > 15) {
            current->setSpeedAdjustment(current->getAircraft()->
                                        getPerformance()->vDescent() *
                                        1.35);
        } else if (time_diff > 5) {
            current->setSpeedAdjustment(current->getAircraft()->
                                        getPerformance()->vDescent() *
                                        1.2);
        } else if (time_diff < -15) {
            current->setSpeedAdjustment(current->getAircraft()->
                                        getPerformance()->vDescent() *
                                        0.65);
        } else if (time_diff < -5) {
            current->setSpeedAdjustment(current->getAircraft()->
                                        getPerformance()->vDescent() *
                                        0.8);
        } else {
            current->clearSpeedAdjustment();
        }
        //current->setSpeedAdjustment(current->getAircraft()->getPerformance()->vDescent() + time_diff);
    }
    setDt(getDt() + dt);
}

/* Search for and erase traffic record with a specific id */
void FGApproachController::signOff(int id)
{
   // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off from approach at " << SG_ORIGIN);
    } else {
        i = activeTraffic.erase(i);
    }
}

/* Periodically check for and remove dead traffic records */
void FGApproachController::update(double dt)
{
    eraseDeadTraffic(activeTraffic);
}

bool FGApproachController::hasInstruction(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->hasInstruction();
    }
    return false;
}


FGATCInstruction FGApproachController::getInstruction(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(activeTraffic, id);
    
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->getInstruction();
    }
    return FGATCInstruction();
}


ActiveRunway *FGApproachController::getRunway(const string& name)
{
    ActiveRunwayVecIterator rwy = activeRunways.begin();
    if (activeRunways.size()) {
        while (rwy != activeRunways.end()) {
            if (rwy->getRunwayName() == name) {
                break;
            }
            rwy++;
        }
    }
    if (rwy == activeRunways.end()) {
        ActiveRunway aRwy(name, 0);
        activeRunways.push_back(aRwy);
        rwy = activeRunways.end() - 1;
    }
    return &(*rwy);
}

void FGApproachController::render(bool visible) {
    // Must be BULK in order to prevent it being called each frame
    SG_LOG(SG_ATC, SG_BULK, "FGApproachController::render function not yet implemented");
}

string FGApproachController::getName() {
    return string(parent->getId() + "-approach");
}
