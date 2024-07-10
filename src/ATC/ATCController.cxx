/*
 * SPDX-FileName: ATCController.cxx
 * SPDX-FileComment: Extracted from trafficrecord.cxx - Implementation of AIModels ATC code.
 * SPDX-FileCopyrightText: Copyright (C) 2006 Durk Talsma
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>

#include <algorithm>
#include <cstdio>
#include <random>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/Shape>

#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Scenery/scenery.hxx>

#include "atc_mgr.hxx"
#include "trafficcontrol.hxx"
#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/performancedata.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/groundnetwork.hxx>
#include <Radio/radio.hxx>
#include <Traffic/TrafficMgr.hxx>
#include <signal.h>

#include <ATC/ATCController.hxx>
#include <ATC/atc_mgr.hxx>
#include <ATC/trafficcontrol.hxx>

using std::sort;
using std::string;

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
    if (initialized) {
        auto mgr = globals->get_subsystem<FGATCManager>();
        mgr->removeController(this);
    }
    _isDestroying = true;
    clearTrafficControllers();
}

void FGATCController::init()
{
    if (!initialized) {
        auto mgr = globals->get_subsystem<FGATCManager>();
        mgr->addController(this);
        initialized = true;
    }
}

string FGATCController::getGateName(FGAIAircraft* ref)
{
    return ref->atGate();
}

bool FGATCController::isUserAircraft(FGAIAircraft* ac)
{
    return (ac->getCallSign() == fgGetString("/sim/multiplay/callsign")) ? true : false;
};

bool FGATCController::checkTransmissionState(int minState, int maxState, TrafficVectorIterator i, time_t now, AtcMsgId msgId,
                                             AtcMsgDir msgDir)
{
    int state = (*i)->getState();
    if ((state >= minState) && (state <= maxState) && available) {
        if ((msgDir == ATC_AIR_TO_GROUND) && isUserAircraft((*i)->getAircraft())) {
            SG_LOG(SG_ATC, SG_BULK, "Checking state " << state << " for " << (*i)->getAircraft()->getCallSign());
            SGPropertyNode_ptr trans_num = globals->get_props()->getNode("/sim/atc/transmission-num", true);
            int n = trans_num->getIntValue();
            if (n == 0) {
                trans_num->setIntValue(-1);
                // PopupCallback(n);
                SG_LOG(SG_ATC, SG_DEBUG, "Selected transmission message " << n);
                //auto atc = globals->get_subsystem<FGATCManager>();
                //FGATCDialogNew::instance()->removeEntry(1);
            } else {
                SG_LOG(SG_ATC, SG_BULK, "Sending message for " << (*i)->getAircraft()->getCallSign());
                transmit(*i, parent, msgId, msgDir, false);
                return false;
            }
        }
        transmit(*i, parent, msgId, msgDir, true);
        (*i)->updateState();
        lastTransmission = now;
        available = false;
        return true;
    }
    return false;
}

void FGATCController::transmit(FGTrafficRecord* rec, FGAirportDynamics* parent, AtcMsgId msgId,
                               AtcMsgDir msgDir, bool audible)
{
    string sender, receiver;
    int stationFreq = 0;
    int taxiFreq = 0;
    int towerFreq = 0;
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
    FGAIFlightPlan* fp;
    string fltRules;
    string instructionText;
    int ground_to_air = 0;

    //double commFreqD;
    sender = rec->getCallsign();
    if (rec->getAircraft()->getTaxiClearanceRequest()) {
        instructionText = "push-back and taxi";
    } else {
        instructionText = "taxi";
    }

    SG_LOG(SG_ATC, SG_BULK, "transmitting for: " << sender << " at Leg " << rec->getLeg());

    //FIXME move departure and arrival to rec
    auto depApt = rec->getAircraft()->getTrafficRef()->getDepartureAirport();
    auto arrApt = rec->getAircraft()->getTrafficRef()->getArrivalAirport();

    if (!depApt) {
        SG_LOG(SG_ATC, SG_DEV_ALERT, "TrafficRec has empty departure airport, can't transmit");
        return;
    }
    if (!arrApt) {
        SG_LOG(SG_ATC, SG_DEV_ALERT, "TrafficRec has empty arrival airport, can't transmit");
        return;
    }

    stationFreq = getFrequency();
    taxiFreq = depApt->getDynamics()->getGroundFrequency(2);
    towerFreq = depApt->getDynamics()->getTowerFrequency(2);
    receiver = getName();
    atisInformation = depApt->getDynamics()->getAtisSequence();

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
            rec->getAircraft()->getTrafficRef()->getArrivalAirport()->getName() + ". Request start-up.";
        break;
        // Acknowledge engine startup permission
        // Assign departure runway
        // Assign SID, if necessary (TODO)
    case MSG_PERMIT_ENGINE_START:
        taxiFreqStr = formatATCFrequency3_2(taxiFreq);

        heading = rec->getAircraft()->getTrafficRef()->getCourse();
        fltType = rec->getAircraft()->getTrafficRef()->getFlightType();
        rwyClass =
            rec->getAircraft()->GetFlightPlan()->getRunwayClassFromTrafficType(fltType);

        rec->getAircraft()->getTrafficRef()->getDepartureAirport()->getDynamics()->getActiveRunway(rwyClass, 1, activeRunway,
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
            "For " + instructionText + " clearance call " + taxiFreqStr + ". " +
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
        text = receiver + ". Holding short runway " + activeRunway + ". " + sender + ".";
        break;
    case MSG_ACKNOWLEDGE_REPORT_RUNWAY_HOLD_SHORT:
        activeRunway = rec->getAircraft()->GetFlightPlan()->getRunway();
        text = receiver + " Roger. Holding short runway " + activeRunway + ". " + sender + ".";
        break;
    case MSG_CLEARED_FOR_TAKEOFF:
        activeRunway = rec->getAircraft()->GetFlightPlan()->getRunway();
        text = receiver + ". Cleared for takeoff runway " + activeRunway + ". " + sender + ".";
        break;
    case MSG_ACKNOWLEDGE_CLEARED_FOR_TAKEOFF:
        activeRunway = rec->getAircraft()->GetFlightPlan()->getRunway();
        text = receiver + " Roger. Cleared for takeoff runway " + activeRunway + ". " + sender + ".";
        break;
    case MSG_SWITCH_TOWER_FREQUENCY:
        towerFreqStr = formatATCFrequency3_2(towerFreq);
        text = receiver + " Contact Tower at " + towerFreqStr + ". " + sender + ".";
        break;
    case MSG_ACKNOWLEDGE_SWITCH_TOWER_FREQUENCY:
        towerFreqStr = formatATCFrequency3_2(towerFreq);
        text = receiver + " Roger, switching to tower at " + towerFreqStr + ". " + sender + ".";
        break;
    case MSG_ARRIVAL:
        text = receiver + ". " + sender + " Information delta.";
        break;
    case MSG_ACKNOWLEDGE_ARRIVAL:
        activeRunway = rec->getRunway();
        text = receiver + " expect ILS approach " + activeRunway + ". " + sender;
        break;
    case MSG_CLEARED_TO_LAND:
        activeRunway = rec->getRunway();
        // TODO: Weather
        text = receiver + " runway " + activeRunway + " cleared to land. " + sender;
        break;
    case MSG_ACKNOWLEDGE_CLEARED_TO_LAND:
        activeRunway = rec->getAircraft()->GetFlightPlan()->getRunway();
        text = receiver + " runway " + activeRunway + " cleared to land. " + sender;
        break;
    case MSG_HOLD:
        text = receiver + " hold as published . " + sender;
        break;
    case MSG_ACKNOWLEDGE_HOLD:
        text = receiver + " holding as published . " + sender;
        break;
    default:
        text = text + sender + ". Transmitting unknown Message. MsgId " + std::to_string(msgId);
        break;
    }

    const bool atcAudioEnabled = fgGetBool("/sim/sound/atc/enabled", false);
    if (audible && atcAudioEnabled) {
        double onBoardRadioFreq0 =
            fgGetDouble("/instrumentation/comm[0]/frequencies/selected-mhz");
        double onBoardRadioFreq1 =
            fgGetDouble("/instrumentation/comm[1]/frequencies/selected-mhz");
        int onBoardRadioFreqI0 = (int)floor(onBoardRadioFreq0 * 100 + 0.5);
        int onBoardRadioFreqI1 = (int)floor(onBoardRadioFreq1 * 100 + 0.5);
        SG_LOG(SG_ATC, SG_DEBUG, "Using " << onBoardRadioFreq0 << ", " << onBoardRadioFreq1 << " and " << stationFreq << " for " << text << std::endl);
        if (stationFreq == 0) {
            SG_LOG(SG_ATC, SG_DEBUG, getName() << " stationFreq not found");
        }

        // Display ATC message only when one of the radios is tuned
        // the relevant frequency.
        // Note that distance attenuation is currently not yet implemented

        if ((stationFreq > 0) &&
            ((onBoardRadioFreqI0 == stationFreq) ||
             (onBoardRadioFreqI1 == stationFreq))) {
            if (rec->allowTransmissions()) {
                if (fgGetBool("/sim/radio/use-itm-attenuation", false)) {
                    SG_LOG(SG_ATC, SG_DEBUG, "Using ITM radio propagation");
                    FGRadioTransmission* radio = new FGRadioTransmission();
                    SGGeod sender_pos;
                    if (ground_to_air) {
                        sender_pos = parent->parent()->geod();
                    } else {
                        sender_pos = rec->getPos();
                    }
                    double frequency = ((double)stationFreq) / 100;
                    radio->receiveATC(sender_pos, frequency, text, ground_to_air);
                    delete radio;
                } else {
                    SG_LOG(SG_ATC, SG_BULK, "Transmitting " << text);
                    fgSetString("/sim/messages/atc", text.c_str());
                }
            }
        }
    } else {
        //FGATCDialogNew::instance()->addEntry(1, text);
    }
}

/**
 * Sign off the aircraft with the id from this controller.
 * @param id the id of the aircraft
*/

void FGATCController::signOff(int id)
{
    TrafficVectorIterator i = searchActiveTraffic(id);
    if (i == activeTraffic.end()) {
        // Dead traffic should never reach here
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off from " << getName() << " at " << SG_ORIGIN << " list " << activeTraffic.empty());
        return;
    }
    SG_LOG(SG_ATC, SG_DEBUG, (*i)->getCallsign() << " (" << (*i)->getId() << ") signing off from " << getName() << "(" << getFrequency() << ")");
    int oldSize = activeTraffic.size();
    activeTraffic.erase(i);
    if ((oldSize - activeTraffic.size()) != 1) {
        SG_LOG(SG_ATC, SG_WARN, (*i)->getCallsign() << " not removed ");
    }
}

bool FGATCController::hasInstruction(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = searchActiveTraffic(id);

    if (i == activeTraffic.end()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return (*i)->hasInstruction();
    }
    return false;
}


FGATCInstruction FGATCController::getInstruction(int id)
{
    if (activeTraffic.size() != 0) {
        // Search activeTraffic for a record matching our id
        TrafficVectorIterator i = searchActiveTraffic(id);

        if (i != activeTraffic.end())
            return (*i)->getInstruction();
    }

    SG_LOG(SG_ATC, SG_ALERT, "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    return FGATCInstruction();
}


/*
* Format integer frequency xxxyy as xxx.yy
* @param freq - integer value
* @return the formatted string
*/
string FGATCController::formatATCFrequency3_2(int freq)
{
    char buffer[7]; // does this ever need to be freed?
    snprintf(buffer, 7, "%3.2f", ((float)freq / 100.0));
    return string(buffer);
}

// TODO: Set transponder codes according to real-world routes.
// The current version just returns a random string of four octal numbers.
string FGATCController::genTransponderCode(const string& fltRules)
{
    if (fltRules == "VFR")
        return string("1200");

    std::default_random_engine generator;
    std::uniform_int_distribution<unsigned> distribution(0, 7);

    unsigned val = (distribution(generator) * 1000 +
                    distribution(generator) * 100 +
                    distribution(generator) * 10 +
                    distribution(generator));

    return std::to_string(val);
}

void FGATCController::eraseDeadTraffic()
{
    auto it = std::remove_if(activeTraffic.begin(), activeTraffic.end(), [](const FGTrafficRecord* traffic) {
        if (traffic->isDead()) {
            SG_LOG(SG_ATC, SG_DEBUG, "Remove dead " << traffic->getId() << " " << traffic->isDead());
        }
        return traffic->isDead();
    });
    activeTraffic.erase(it, activeTraffic.end());
}

/**
* Search activeTraffic vector to find matching id
* @param id integer to search for in the vector
* @return the matching item OR activeTraffic.end()
*/
TrafficVectorIterator FGATCController::searchActiveTraffic(int id)
{
    if (activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_DEBUG, "searchActiveTraffic empty list");
        return activeTraffic.end();
    }
    return std::find_if(activeTraffic.begin(), activeTraffic.end(),
                        [id](const FGTrafficRecord* rec) { return rec->getId() == id; });
}

void FGATCController::clearTrafficControllers()
{
    for (const auto& traffic : activeTraffic) {
        traffic->clearATCController();
    }
}
