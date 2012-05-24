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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <algorithm>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Shape>

#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/material/mat.hxx>
#include <Scenery/scenery.hxx>

#include "trafficcontrol.hxx"
#include "atc_mgr.hxx"
#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/performancedata.hxx>
#include <AIModel/performancedb.hxx>
#include <ATC/atc_mgr.hxx>
#include <Traffic/TrafficMgr.hxx>
#include <Airports/groundnetwork.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/simple.hxx>
#include <Radio/radio.hxx>
#include <signal.h>

using std::sort;

/***************************************************************************
 * ActiveRunway
 **************************************************************************/
time_t ActiveRunway::requestTimeSlot(time_t eta)
{
    time_t newEta;
    time_t separation = 90;
    bool found = false;
    if (estimatedArrivalTimes.size() == 0) {
        estimatedArrivalTimes.push_back(eta);
        return eta;
    } else {
        TimeVectorIterator i = estimatedArrivalTimes.begin();
        //cerr << "Checking eta slots " << eta << ": " << endl;
        for (i = estimatedArrivalTimes.begin();
                i != estimatedArrivalTimes.end(); i++) {
            //cerr << "Stored time : " << (*i) << endl;
        }
        i = estimatedArrivalTimes.begin();
        if ((eta + separation) < (*i)) {
            newEta = eta;
            found = true;
            //cerr << "Storing at beginning" << endl;
        }
        while ((i != estimatedArrivalTimes.end()) && (!found)) {
            TimeVectorIterator j = i + 1;
            if (j == estimatedArrivalTimes.end()) {
                if (((*i) + separation) < eta) {
                    //cerr << "Storing at end" << endl;
                    newEta = eta;
                } else {
                    newEta = (*i) + separation;
                    //cerr << "Storing at end + separation" << endl;
                }
            } else {
                if ((((*j) - (*i)) > (separation * 2))) {       // found a potential slot
                    // now check whether this slot is usable:
                    // 1) eta should fall between the two points
                    //    i.e. eta > i AND eta < j
                    //
                    //cerr << "Found potential slot after " << (*i) << endl;
                    if (eta > (*i) && (eta < (*j))) {
                        found = true;
                        if (eta < ((*i) + separation)) {
                            newEta = (*i) + separation;
                            //cerr << "Using  original" << (*i) << " + separation " << endl;
                        } else {
                            newEta = eta;
                            //cerr << "Using original after " << (*i) << endl;
                        }
                    } else if (eta < (*i)) {
                        found = true;
                        newEta = (*i) + separation;
                        //cerr << "Using delayed slot after " << (*i) << endl;
                    }
                    /*
                       if (((*j) - separation) < eta) {
                       found = true;
                       if (((*i) + separation) < eta) {
                       newEta = eta;
                       cerr << "Using original after " << (*i) << endl;
                       } else {
                       newEta = (*i) + separation;
                       cerr << "Using  " << (*i) << " + separation " << endl;
                       }
                       } */
                }
            }
            i++;
        }
    }
    //cerr << ". done. New ETA : " << newEta << endl;

    estimatedArrivalTimes.push_back(newEta);
    sort(estimatedArrivalTimes.begin(), estimatedArrivalTimes.end());
    // do some housekeeping : remove any timestamps that are past
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");
    TimeVectorIterator i = estimatedArrivalTimes.begin();
    while (i != estimatedArrivalTimes.end()) {
        if ((*i) < now) {
            //cerr << "Deleting timestamp " << (*i) << " (now = " << now << "). " << endl;
            estimatedArrivalTimes.erase(i);
            i = estimatedArrivalTimes.begin();
        } else {
            i++;
        }
    }
    return newEta;
}

void ActiveRunway::printDepartureCue()
{
    cout << "Departure cue for " << rwy << ": " << endl;
    for (AircraftVecIterator atc = departureCue.begin(); atc != departureCue.end(); atc++) {
        cout << "     " << (*atc)->getCallSign() << " "  << (*atc)->getTakeOffStatus();
        cout << " " << (*atc)->_getLatitude() << " " << (*atc)->_getLongitude() << (*atc)-> getSpeed() << " " << (*atc)->getAltitude() << endl;
    }
    
}

FGAIAircraft* ActiveRunway::getFirstOfStatus(int stat)
{
    for (AircraftVecIterator atc =departureCue.begin(); atc != departureCue.end(); atc++) {
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
        latitude(0), longitude(0), heading(0), speed(0), altitude(0), radius(0)
{
}

void FGTrafficRecord::setPositionAndIntentions(int pos,
        FGAIFlightPlan * route)
{

    currentPos = pos;
    if (intentions.size()) {
        intVecIterator i = intentions.begin();
        if ((*i) != pos) {
            SG_LOG(SG_ATC, SG_ALERT,
                   "Error in FGTrafficRecord::setPositionAndIntentions at " << SG_ORIGIN);
        }
        intentions.erase(i);
    } else {
        //FGAIFlightPlan::waypoint* const wpt= route->getCurrentWaypoint();
        int size = route->getNrOfWayPoints();
        //cerr << "Setting pos" << pos << " ";
        //cerr << "setting intentions ";
        for (int i = 2; i < size; i++) {
            int val = route->getRouteIndex(i);
            intentions.push_back(val);
        }
    }
}
/**
 * Check if another aircraft is ahead of the current one, and on the same
 * return true / false is the is/isn't the case.
 *
 ****************************************************************************/

bool FGTrafficRecord::checkPositionAndIntentions(FGTrafficRecord & other)
{
    bool result = false;
    //cerr << "Start check 1" << endl;
    if (currentPos == other.currentPos) {
        //cerr << callsign << ": Check Position and intentions: we are on the same taxiway" << other.callsign << "Index = " << currentPos << endl;
        result = true;
    }
    //  else if (other.intentions.size())
    //     {
    //       cerr << "Start check 2" << endl;
    //       intVecIterator i = other.intentions.begin();
    //       while (!((i == other.intentions.end()) || ((*i) == currentPos)))
    //     i++;
    //       if (i != other.intentions.end()) {
    //     cerr << "Check Position and intentions: current matches other.intentions" << endl;
    //     result = true;
    //       }
    else if (intentions.size()) {
        //cerr << "Start check 3" << endl;
        intVecIterator i = intentions.begin();
        //while (!((i == intentions.end()) || ((*i) == other.currentPos)))
        while (i != intentions.end()) {
            if ((*i) == other.currentPos) {
                break;
            }
            i++;
        }
        if (i != intentions.end()) {
            //cerr << callsign << ": Check Position and intentions: .other.current matches" << other.callsign << "Index = " << (*i) << endl;
            result = true;
        }
    }
    //cerr << "Done !!" << endl;
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
    if (intentions.size()) {
        for (i = intentions.begin(); i != intentions.end(); i++) {
            if ((*i) > 0) {
                if (currentTargetNode ==
                        net->findSegment(*i)->getEnd()->getIndex()) {
                    //cerr << "Current crosses at " << currentTargetNode <<endl;
                    return currentTargetNode;
                }
            }
        }
    }
    if (other.intentions.size()) {
        for (i = other.intentions.begin(); i != other.intentions.end();
                i++) {
            if ((*i) > 0) {
                if (otherTargetNode ==
                        net->findSegment(*i)->getEnd()->getIndex()) {
                    //cerr << "Other crosses at " << currentTargetNode <<endl;
                    return otherTargetNode;
                }
            }
        }
    }
    if (intentions.size() && other.intentions.size()) {
        for (i = intentions.begin(); i != intentions.end(); i++) {
            for (j = other.intentions.begin(); j != other.intentions.end();
                    j++) {
                //cerr << "finding segment " << *i << " and " << *j << endl;
                if (((*i) > 0) && ((*j) > 0)) {
                    currentTargetNode =
                        net->findSegment(*i)->getEnd()->getIndex();
                    otherTargetNode =
                        net->findSegment(*j)->getEnd()->getIndex();
                    if (currentTargetNode == otherTargetNode) {
                        //cerr << "Routes will cross at " << currentTargetNode << endl;
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
    if (other.intentions.size()) {
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
    //if (intentions.size())
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
    //cerr << "Current segment " << currentPos << endl;
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
                                //cerr << "Found the node " << node << endl;
                                return true;
                            }
                        }
                    }
            }
            if (other.intentions.size()) {
                for (intVecIterator j = other.intentions.begin();
                        j != other.intentions.end(); j++) {
                    // cerr << "Current segment 1 " << (*i) << endl;
                    if ((*i) > 0) {
                        if ((opp = net->findSegment(*i)->opposite())) {
                            if (opp->getIndex() ==
                                    net->findSegment(*j)->getIndex()) {
                                //cerr << "Nodes " << net->findSegment(*i)->getIndex()
                                //   << " and  " << net->findSegment(*j)->getIndex()
                                //   << " are opposites " << endl;
                                if (net->findSegment(*i)->getStart()->
                                        getIndex() == node) {
                                    {
                                        //cerr << "Found the node " << node << endl;
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

bool FGTrafficRecord::isActive(int margin)
{
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");
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

bool FGTrafficRecord::pushBackAllowed()
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


bool FGATCInstruction::hasInstruction()
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
    //cerr << "running FGATController constructor" << endl;
    dt_count = 0;
    available = true;
    lastTransmission = 0;
    initialized = false;
}

FGATCController::~FGATCController()
{
    //cerr << "running FGATController destructor" << endl;
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
    //cerr << "transmitting for: " << sender << "Leg = " << rec->getLeg() << endl;
    switch (rec->getLeg()) {
    case 1:
    case 2:
        freqId = rec->getNextFrequency();
        stationFreq =
            rec->getAircraft()->getTrafficRef()->getDepartureAirport()->
            getDynamics()->getGroundFrequency(rec->getLeg() + freqId);
        taxiFreq =
            rec->getAircraft()->getTrafficRef()->getDepartureAirport()->
            getDynamics()->getGroundFrequency(2);
        towerFreq =
            rec->getAircraft()->getTrafficRef()->getDepartureAirport()->
            getDynamics()->getTowerFrequency(2);
        receiver =
            rec->getAircraft()->getTrafficRef()->getDepartureAirport()->
            getName() + "-Ground";
        atisInformation =
            rec->getAircraft()->getTrafficRef()->getDepartureAirport()->
            getDynamics()->getAtisSequence();
        break;
    case 3:
        receiver =
            rec->getAircraft()->getTrafficRef()->getDepartureAirport()->
            getName() + "-Tower";
        break;
    }
    // Swap sender and receiver value in case of a ground to air transmission
    if (msgDir == ATC_GROUND_TO_AIR) {
        string tmp = sender;
        sender = receiver;
        receiver = tmp;
        ground_to_air=1;
    }
    switch (msgId) {
    case MSG_ANNOUNCE_ENGINE_START:
        text = sender + ". Ready to Start up";
        break;
    case MSG_REQUEST_ENGINE_START:
        text =
            receiver + ", This is " + sender + ". Position " +
            getGateName(rec->getAircraft()) + ". Information " +
            atisInformation + ". " +
            rec->getAircraft()->getTrafficRef()->getFlightRules() +
            " to " +
            rec->getAircraft()->getTrafficRef()->getArrivalAirport()->
            getName() + ". Request start-up";
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
        fp = rec->getAircraft()->getTrafficRef()->getDepartureAirport()->
             getDynamics()->getSID(activeRunway, heading);
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
        text = receiver + ". Standby";
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
            sender;
        break;
    case MSG_ACKNOWLEDGE_SWITCH_GROUND_FREQUENCY:
        taxiFreqStr = formatATCFrequency3_2(taxiFreq);
        text = receiver + ". Switching to " + taxiFreqStr + ". " + sender;
        break;
    case MSG_INITIATE_CONTACT:
        text = receiver + ". With you. " + sender;
        break;
    case MSG_ACKNOWLEDGE_INITIATE_CONTACT:
        text = receiver + ". Roger. " + sender;
        break;
    case MSG_REQUEST_PUSHBACK_CLEARANCE:
        if (rec->getAircraft()->getTaxiClearanceRequest()) {
            text = receiver + ". Request push-back. " + sender;
        } else {
            text = receiver + ". Request Taxi clearance. " + sender;
        }
        break;
    case MSG_PERMIT_PUSHBACK_CLEARANCE:
        if (rec->getAircraft()->getTaxiClearanceRequest()) {
            text = receiver + ". Push-back approved. " + sender;
        } else {
            text = receiver + ". Cleared to Taxi." + sender;
        }
        break;
    case MSG_HOLD_PUSHBACK_CLEARANCE:
        text = receiver + ". Standby. " + sender;
        break;
    case MSG_REQUEST_TAXI_CLEARANCE:
        text = receiver + ". Ready to Taxi. " + sender;
        break;
    case MSG_ISSUE_TAXI_CLEARANCE:
        text = receiver + ". Cleared to taxi. " + sender;
        break;
    case MSG_ACKNOWLEDGE_TAXI_CLEARANCE:
        text = receiver + ". Cleared to taxi. " + sender;
        break;
    case MSG_HOLD_POSITION:
        text = receiver + ". Hold Position. " + sender;
        break;
    case MSG_ACKNOWLEDGE_HOLD_POSITION:
        text = receiver + ". Holding Position. " + sender;
        break;
    case MSG_RESUME_TAXI:
        text = receiver + ". Resume Taxiing. " + sender;
        break;
    case MSG_ACKNOWLEDGE_RESUME_TAXI:
        text = receiver + ". Continuing Taxi. " + sender;
        break;
    case MSG_REPORT_RUNWAY_HOLD_SHORT:
        activeRunway = rec->getAircraft()->GetFlightPlan()->getRunway();
        //activeRunway = "test";
        text = receiver + ". Holding short runway "
               + activeRunway
               + ". " + sender;
        //text = "test1";
        //cerr << "1 Currently at leg " << rec->getLeg() << endl;
        break;
    case MSG_ACKNOWLEDGE_REPORT_RUNWAY_HOLD_SHORT:
        activeRunway = rec->getAircraft()->GetFlightPlan()->getRunway();
        text = receiver + "Roger. Holding short runway "
               //                + activeRunway
               + ". " + sender;
        //text = "test2";
        //cerr << "2 Currently at leg " << rec->getLeg() << endl;
        break;
    case MSG_SWITCH_TOWER_FREQUENCY:
        towerFreqStr = formatATCFrequency3_2(towerFreq);
        text = receiver + "Contact Tower at " + towerFreqStr + ". " + sender;
        //text = "test3";
        //cerr << "3 Currently at leg " << rec->getLeg() << endl;
        break;
    case MSG_ACKNOWLEDGE_SWITCH_TOWER_FREQUENCY:
        towerFreqStr = formatATCFrequency3_2(towerFreq);
        text = receiver + "Roger, switching to tower at " + towerFreqStr + ". " + sender;
        //text = "test4";
        //cerr << "4 Currently at leg " << rec->getLeg() << endl;
        break;
    default:
        //text = "test3";
        text = text + sender + ". Transmitting unknown Message";
        break;
    }
    if (audible) {
        double onBoardRadioFreq0 =
            fgGetDouble("/instrumentation/comm[0]/frequencies/selected-mhz");
        double onBoardRadioFreq1 =
            fgGetDouble("/instrumentation/comm[1]/frequencies/selected-mhz");
        int onBoardRadioFreqI0 = (int) floor(onBoardRadioFreq0 * 100 + 0.5);
        int onBoardRadioFreqI1 = (int) floor(onBoardRadioFreq1 * 100 + 0.5);
        //cerr << "Using " << onBoardRadioFreq0 << ", " << onBoardRadioFreq1 << " and " << stationFreq << " for " << text << endl;

        // Display ATC message only when one of the radios is tuned
        // the relevant frequency.
        // Note that distance attenuation is currently not yet implemented
                
        if ((stationFreq > 0)&&
            ((onBoardRadioFreqI0 == stationFreq)||
             (onBoardRadioFreqI1 == stationFreq))) {
            if (rec->allowTransmissions()) {
            	
            	if( fgGetBool( "/sim/radio/use-itm-attenuation", false ) ) {
            		//cerr << "Using ITM radio propagation" << endl;
            		FGRadioTransmission* radio = new FGRadioTransmission();
            		SGGeod sender_pos;
            		double sender_alt_ft, sender_alt;
            		if(ground_to_air) {
			              sender_alt_ft = parent->getElevation();
			              sender_alt = sender_alt_ft * SG_FEET_TO_METER;
			              sender_pos= SGGeod::fromDegM( parent->getLongitude(),
		                      parent->getLatitude(), sender_alt );
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


string FGATCController::formatATCFrequency3_2(int freq)
{
    char buffer[7];
    snprintf(buffer, 7, "%3.2f", ((float) freq / 100.0));
    return string(buffer);
}

// TODO: Set transponder codes according to real-world routes.
// The current version just returns a random string of four octal numbers.
string FGATCController::genTransponderCode(string fltRules)
{
    if (fltRules == "VFR") {
        return string("1200");
    } else {
        char buffer[5];
        snprintf(buffer, 5, "%d%d%d%d", rand() % 8, rand() % 8, rand() % 8,
                 rand() % 8);
        return string(buffer);
    }
}

void FGATCController::init()
{
    if (!initialized) {
        FGATCManager *mgr = (FGATCManager*) globals->get_subsystem("ATC");
        mgr->addController(this);
        initialized = true;
    }
}

/***************************************************************************
 * class FGTowerController
 *
 **************************************************************************/
FGTowerController::FGTowerController(FGAirportDynamics *par) :
        FGATCController()
{
    parent = par;
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
    TrafficVectorIterator i = activeTraffic.begin();
    // Search whether the current id alread has an entry
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

        rec.setPositionAndHeading(lat, lon, heading, speed, alt);
        rec.setRunway(intendedRoute->getRunway());
        rec.setLeg(leg);
        //rec.setCallSign(callsign);
        rec.setRadius(radius);
        rec.setAircraft(ref);
        activeTraffic.push_back(rec);
        // Don't just schedule the aircraft for the tower controller, also assign if to the correct active runway.
        ActiveRunwayVecIterator rwy = activeRunways.begin();
        if (activeRunways.size()) {
            while (rwy != activeRunways.end()) {
                if (rwy->getRunwayName() == intendedRoute->getRunway()) {
                    break;
                }
                rwy++;
            }
        }
        if (rwy == activeRunways.end()) {
            ActiveRunway aRwy(intendedRoute->getRunway(), id);
            aRwy.addToDepartureCue(ref);
            activeRunways.push_back(aRwy);
            rwy = (activeRunways.end()-1);
        } else {
            rwy->addToDepartureCue(ref);
        }

        //cerr << ref->getTrafficRef()->getCallSign() << " You are number " << rwy->getDepartureCueSize() <<  " for takeoff " << endl;
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
    }
}

void FGTowerController::updateAircraftInformation(int id, double lat, double lon,
        double heading, double speed, double alt,
        double dt)
{
    TrafficVectorIterator i = activeTraffic.begin();
    // Search whether the current id has an entry
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
//    // update position of the current aircraft
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: updating aircraft without traffic record at " << SG_ORIGIN);
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
        current = i;
    }
    setDt(getDt() + dt);

    // see if we already have a clearance record for the currently active runway
    // NOTE: dd. 2011-08-07: Because the active runway has been constructed in the announcePosition function, we may safely assume that is
    // already exists here. So, we can simplify the current code.
    
    ActiveRunwayVecIterator rwy = activeRunways.begin();
    //if (parent->getId() == fgGetString("/sim/presets/airport-id")) {
    //    for (rwy = activeRunways.begin(); rwy != activeRunways.end(); rwy++) {
    //        rwy->printDepartureCue();
    //    }
    //}
    
    rwy = activeRunways.begin();
    while (rwy != activeRunways.end()) {
        if (rwy->getRunwayName() == current->getRunway()) {
            break;
        }
        rwy++;
    }

    // only bother running the following code if the current aircraft is the
    // first in line for depature
    /* if (current->getAircraft() == rwy->getFirstAircraftInDepartureCue()) {
        if (rwy->getCleared()) {
            if (id == rwy->getCleared()) {
                current->setHoldPosition(false);
            } else {
                current->setHoldPosition(true);
            }
        } else {
            // For now. At later stages, this will probably be the place to check for inbound traffc.
            rwy->setCleared(id);
        }
    } */
    // only bother with aircraft that have a takeoff status of 2, since those are essentially under tower control
    FGAIAircraft* ac= rwy->getFirstAircraftInDepartureCue();
    if (ac->getTakeOffStatus() == 1) {
        ac->setTakeOffStatus(2);
    }
    if (current->getAircraft()->getTakeOffStatus() == 2) {
        current -> setHoldPosition(false);
    } else {
        current->setHoldPosition(true);
    }
    int clearanceId = rwy->getCleared();
    if (clearanceId) {
        if (id == clearanceId) {
            current->setHoldPosition(false);
        }
    } else {
        if (current->getAircraft() == rwy->getFirstAircraftInDepartureCue()) {
            rwy->setCleared(id);
            FGAIAircraft *ac = rwy->getFirstOfStatus(1);
            if (ac)
                ac->setTakeOffStatus(2);
        }
    }
} 


void FGTowerController::signOff(int id)
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
    // If this aircraft has left the runway, we can clear the departure record for this runway
    ActiveRunwayVecIterator rwy = activeRunways.begin();
    if (activeRunways.size()) {
        //while ((rwy->getRunwayName() != i->getRunway()) && (rwy != activeRunways.end())) {
        while (rwy != activeRunways.end()) {
            if (rwy->getRunwayName() == i->getRunway()) {
                break;
            }
            rwy++;
        }
        if (rwy != activeRunways.end()) {
            rwy->setCleared(0);
            rwy->updateDepartureCue();
        } else {
            SG_LOG(SG_ATC, SG_ALERT,
                   "AI error: Attempting to erase non-existing runway clearance record in FGTowerController::signoff at " << SG_ORIGIN);
        }
    }
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off from tower at " << SG_ORIGIN);
    } else {
        i->getAircraft()->resetTakeOffStatus();
        i = activeTraffic.erase(i);
        //cerr << "Signing off from tower controller" << endl;
    }
}

// NOTE:
// IF WE MAKE TRAFFICRECORD A MEMBER OF THE BASE CLASS
// THE FOLLOWING THREE FUNCTIONS: SIGNOFF, HAS INSTRUCTION AND GETINSTRUCTION CAN
// BECOME DEVIRTUALIZED AND BE A MEMBER OF THE BASE ATCCONTROLLER CLASS
// WHICH WOULD SIMPLIFY CODE MAINTENANCE.
// Note that this function is probably obsolete
bool FGTowerController::hasInstruction(int id)
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
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->hasInstruction();
    }
    return false;
}


FGATCInstruction FGTowerController::getInstruction(int id)
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
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->getInstruction();
    }
    return FGATCInstruction();
}

void FGTowerController::render(bool visible) {
    //cerr << "FGTowerController::render function not yet implemented" << endl;
}

string FGTowerController::getName() {
    return string(parent->getId() + "-tower");
}

void FGTowerController::update(double dt)
{

}



/***************************************************************************
 * class FGStartupController
 *
 **************************************************************************/
FGStartupController::FGStartupController(FGAirportDynamics *par):
        FGATCController()
{
    parent = par;
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
    TrafficVectorIterator i = activeTraffic.begin();
    // Search whether the current id alread has an entry
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
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->hasInstruction();
    }
    return false;
}


FGATCInstruction FGStartupController::getInstruction(int id)
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
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->getInstruction();
    }
    return FGATCInstruction();
}

void FGStartupController::signOff(int id)
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
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off from tower at " << SG_ORIGIN);
    } else {
        //cerr << i->getAircraft()->getCallSign() << " signing off from startupcontroller" << endl;
        i = activeTraffic.erase(i);
    }
}

bool FGStartupController::checkTransmissionState(int st, time_t now, time_t startTime, TrafficVectorIterator i, AtcMsgId msgId,
        AtcMsgDir msgDir)
{
    int state = i->getState();
    if ((state == st) && available) {
        if ((msgDir == ATC_AIR_TO_GROUND) && isUserAircraft(i->getAircraft())) {

            //cerr << "Checking state " << st << " for " << i->getAircraft()->getCallSign() << endl;
            static SGPropertyNode_ptr trans_num = globals->get_props()->getNode("/sim/atc/transmission-num", true);
            int n = trans_num->getIntValue();
            if (n == 0) {
                trans_num->setIntValue(-1);
                // PopupCallback(n);
                //cerr << "Selected transmission message " << n << endl;
                FGATCDialogNew::instance()->removeEntry(1);
            } else {
                //cerr << "creading message for " << i->getAircraft()->getCallSign() << endl;
                transmit(&(*i), &(*parent), msgId, msgDir, false);
                return false;
            }
        }
        if (now > startTime) {
            //cerr << "Transmitting startup msg" << endl;
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
//    // update position of the current aircraft

    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: updating aircraft without traffic record at " << SG_ORIGIN);
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
        current = i;
    }
    setDt(getDt() + dt);

    int state = i->getState();

    // The user controlled aircraft should have crased here, because it doesn't have a traffic reference.
    // NOTE: if we create a traffic schedule for the user aircraft, we can use this to plan a flight.
    time_t startTime = i->getAircraft()->getTrafficRef()->getDepartureTime();
    time_t now = time(NULL) + fgGetLong("/sim/time/warp");
    //cerr << i->getAircraft()->getTrafficRef()->getCallSign()
    //     << " is scheduled to depart in " << startTime-now << " seconds. Available = " << available
    //     << " at parking " << getGateName(i->getAircraft()) << endl;

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

    SGMaterialLib *matlib = globals->get_matlib();
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
        //double elevation_meters = 0.0;
        //double elevation_feet = 0.0;


        //for ( FGTaxiSegmentVectorIterator i = segments.begin(); i != segments.end(); i++) {
        double dx = 0;
        time_t now = time(NULL) + fgGetLong("/sim/time/warp");
        for   (TrafficVectorIterator i = activeTraffic.begin(); i != activeTraffic.end(); i++) {
            if (i->isActive(300)) {
                // Handle start point
                int pos = i->getCurrentPosition();
                //cerr << "rendering for " << i->getAircraft()->getCallSign() << "pos = " << pos << endl;
                if (pos > 0) {
                    FGTaxiSegment *segment  = parent->getGroundNetwork()->findSegment(pos);
                    SGGeod start(SGGeod::fromDeg((i->getLongitude()), (i->getLatitude())));
                    SGGeod end  (SGGeod::fromDeg(segment->getEnd()->getLongitude(), segment->getEnd()->getLatitude()));

                    double length = SGGeodesy::distanceM(start, end);
                    //heading = SGGeodesy::headingDeg(start->getGeod(), end->getGeod());

                    double az2, heading; //, distanceM;
                    SGGeodesy::inverse(start, end, heading, az2, length);
                    double coveredDistance = length * 0.5;
                    SGGeod center;
                    SGGeodesy::direct(start, heading, coveredDistance, center, az2);
                    //cerr << "Active Aircraft : Centerpoint = (" << center.getLatitudeDeg() << ", " << center.getLongitudeDeg() << "). Heading = " << heading << endl;
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
                    double elevationEnd   = segment->getEnd()->getElevationM(parent->getElevation()*SG_FEET_TO_METER);
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

                    //cerr << "1. Using mean elevation : " << elevationMean << " and " << slope << endl;

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
                        mat = matlib->find("UnidirectionalTaperRed");
                    } else {
                        mat = matlib->find("UnidirectionalTaperGreen");
                    }
                    if (mat)
                        geode->setEffect(mat->get_effect());
                    obj_trans->addChild(geode);
                    // wire as much of the scene graph together as we can
                    //->addChild( obj_trans );
                    group->addChild( obj_trans );
                    /////////////////////////////////////////////////////////////////////
                } else {
                    //cerr << "BIG FAT WARNING: current position is here : " << pos << endl;
                }
                for (intVecIterator j = (i)->getIntentions().begin(); j != (i)->getIntentions().end(); j++) {
                    osg::Matrix obj_pos;
                    int k = (*j);
                    if (k > 0) {
                        //cerr << "rendering for " << i->getAircraft()->getCallSign() << "intention = " << k << endl;
                        osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
                        obj_trans->setDataVariance(osg::Object::STATIC);
                        FGTaxiSegment *segment  = parent->getGroundNetwork()->findSegment(k);

                        double elevationStart = segment->getStart()->getElevationM(parent->getElevation()*SG_FEET_TO_METER);
                        double elevationEnd   = segment->getEnd  ()->getElevationM(parent->getElevation()*SG_FEET_TO_METER);
                        if ((elevationStart == 0) || (elevationStart == parent->getElevation())) {
                            SGGeod center2 = segment->getStart()->getGeod();
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
                            SGGeod center2 = segment->getEnd()->getGeod();
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

                        //cerr << "2. Using mean elevation : " << elevationMean << " and " << slope << endl;


                        WorldCoordinate( obj_pos, segment->getLatitude(), segment->getLongitude(), elevationMean + 0.5 + dx, -(segment->getHeading()), slope );

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
                            mat = matlib->find("UnidirectionalTaperRed");
                        } else {
                            mat = matlib->find("UnidirectionalTaperGreen");
                        }
                        if (mat)
                            geode->setEffect(mat->get_effect());
                        obj_trans->addChild(geode);
                        // wire as much of the scene graph together as we can
                        //->addChild( obj_trans );
                        group->addChild( obj_trans );
                    } else {
                        //cerr << "BIG FAT WARNING: k is here : " << pos << endl;
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

}



/***************************************************************************
 * class FGApproachController
 *
 **************************************************************************/
FGApproachController::FGApproachController(FGAirportDynamics *par):
        FGATCController()
{
    parent = par;
}

//
void FGApproachController::announcePosition(int id,
        FGAIFlightPlan * intendedRoute,
        int currentPosition,
        double lat, double lon,
        double heading, double speed,
        double alt, double radius,
        int leg, FGAIAircraft * ref)
{
    init();
    TrafficVectorIterator i = activeTraffic.begin();
    // Search whether the current id alread has an entry
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
//    // update position of the current aircraft
    if (i == activeTraffic.end() || (activeTraffic.size() == 0)) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: updating aircraft without traffic record at " << SG_ORIGIN);
    } else {
        i->setPositionAndHeading(lat, lon, heading, speed, alt);
        current = i;
        //cerr << "ApproachController: checking for speed" << endl;
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

void FGApproachController::signOff(int id)
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
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off from approach at " << SG_ORIGIN);
    } else {
        i = activeTraffic.erase(i);
    }
}

void FGApproachController::update(double dt)
{

}



bool FGApproachController::hasInstruction(int id)
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
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->hasInstruction();
    }
    return false;
}


FGATCInstruction FGApproachController::getInstruction(int id)
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
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return i->getInstruction();
    }
    return FGATCInstruction();
}


ActiveRunway *FGApproachController::getRunway(string name)
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
    //cerr << "FGApproachController::render function not yet implemented" << endl;
}



string FGApproachController::getName() {
    return string(parent->getId() + "-approach");
}
