// Extracted from trafficrecord.cxx - Implementation of AIModels ATC code.
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
#include <random>

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
#include <Traffic/TrafficMgr.hxx>
#include <Airports/groundnetwork.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/airport.hxx>
#include <Radio/radio.hxx>
#include <signal.h>

#include <ATC/atc_mgr.hxx>
#include <ATC/trafficcontrol.hxx>
#include <ATC/ATCController.hxx>
#include <ATC/ApproachController.hxx>

using std::sort;
using std::string;

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
    TrafficVectorIterator i = FGATCController::searchActiveTraffic(id);

    // Add a new TrafficRecord if no one exsists for this aircraft.
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        FGTrafficRecord* rec = new FGTrafficRecord();
        rec->setId(id);

        rec->setPositionAndHeading(lat, lon, heading, speed, alt);
        rec->setRunway(intendedRoute->getRunway());
        rec->setLeg(leg);
        rec->setCallsign(ref->getCallSign());
        rec->setAircraft(ref);
        rec->setPlannedArrivalTime(intendedRoute->getArrivalTime());
        activeTraffic.push_back(rec);
    } else {
        (*i)->setPositionAndHeading(lat, lon, heading, speed, alt);
        (*i)->setPlannedArrivalTime(intendedRoute->getArrivalTime());
    }
}

void FGApproachController::updateAircraftInformation(int id, SGGeod geod,
        double heading, double speed, double alt,
        double dt)
{
    time_t now = globals->get_time_params()->get_cur_time();
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = FGATCController::searchActiveTraffic(id);
    TrafficVectorIterator current;

    // update position of the current aircraft
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "FGApproachController updating aircraft without traffic record at " << SG_ORIGIN);
    } else {
        (*i)->setPositionAndHeading(geod.getLatitudeDeg(), geod.getLongitudeDeg(), heading, speed, alt);
        current = i;
        if((*current)->getAircraft()) {
            //FIXME No call to aircraft! -> set instruction
            time_t time_diff =
                (*current)->getAircraft()->
                checkForArrivalTime(string("final001"));
            if (time_diff != 0) {
                SG_LOG(SG_ATC, SG_BULK, (*current)->getCallsign() << "|ApproachController: checking for speed " << time_diff);
            }
            if (time_diff > 15) {
                (*current)->setSpeedAdjustment((*current)->getAircraft()->
                                            getPerformance()->vDescent() *
                                            1.35);
            } else if (time_diff > 5) {
                (*current)->setSpeedAdjustment((*current)->getAircraft()->
                                            getPerformance()->vDescent() *
                                            1.2);
            } else if (time_diff < -15) {
                (*current)->setSpeedAdjustment((*current)->getAircraft()->
                                            getPerformance()->vDescent() *
                                            0.65);
            } else if (time_diff < -5) {
                (*current)->setSpeedAdjustment((*current)->getAircraft()->
                                            getPerformance()->vDescent() *
                                            0.8);
            } else {
                (*current)->clearSpeedAdjustment();
            }
            if ((now - lastTransmission) > 15) {
                available = true;
            }
            //Start of our status runimplicit "announce arrival"
            if (checkTransmissionState(ATCMessageState::NORMAL, ATCMessageState::NORMAL, current, now, MSG_ARRIVAL, ATC_AIR_TO_GROUND)) {
                (*current)->setRunwaySlot(getRunway((*current)->getRunway())->requestTimeSlot((*current)->getPlannedArrivalTime()));
                (*current)->setState(ATCMessageState::ACK_ARRIVAL);
            }
            if (checkTransmissionState(ATCMessageState::ACK_ARRIVAL, ATCMessageState::ACK_ARRIVAL, current, now, MSG_ACKNOWLEDGE_ARRIVAL, ATC_GROUND_TO_AIR)) {
                if ((*current)->getRunwaySlot() > (*current)->getPlannedArrivalTime()) {
                    (*current)->setState(ATCMessageState::HOLD);
                } else {
                    (*current)->setState(ATCMessageState::CLEARED_TO_LAND);
                }
            }
            if (checkTransmissionState(ATCMessageState::HOLD, ATCMessageState::HOLD, current, now, MSG_ACKNOWLEDGE_HOLD, ATC_AIR_TO_GROUND)) {
                (*current)->setState(ATCMessageState::ACK_HOLD);
            }
            if (checkTransmissionState(ATCMessageState::CLEARED_TO_LAND, ATCMessageState::CLEARED_TO_LAND, current, now, MSG_CLEARED_TO_LAND, ATC_GROUND_TO_AIR)) {
                (*current)->setState(ATCMessageState::ACK_CLEARED_TO_LAND);
            }
            if (checkTransmissionState(ATCMessageState::ACK_CLEARED_TO_LAND, ATCMessageState::ACK_CLEARED_TO_LAND, current, now, MSG_ACKNOWLEDGE_CLEARED_TO_LAND, ATC_AIR_TO_GROUND)) {
                (*current)->setState(ATCMessageState::SWITCH_GROUND_TOWER);
            }
            if (checkTransmissionState(ATCMessageState::SWITCH_GROUND_TOWER, ATCMessageState::SWITCH_GROUND_TOWER, current, now, MSG_SWITCH_TOWER_FREQUENCY, ATC_GROUND_TO_AIR)) {
            }
            if (checkTransmissionState(ATCMessageState::ACK_SWITCH_GROUND_TOWER, ATCMessageState::ACK_SWITCH_GROUND_TOWER, current, now, MSG_ACKNOWLEDGE_SWITCH_TOWER_FREQUENCY, ATC_AIR_TO_GROUND)) {
                (*current)->setState(ATCMessageState::LANDING_TAXI);
            }
        }
        //(*current)->setSpeedAdjustment((*current)->getAircraft()->getPerformance()->vDescent() + time_diff);
    }
    setDt(getDt() + dt);
}

/** Periodically check for and remove dead traffic records */
void FGApproachController::update(double dt)
{
    FGATCController::eraseDeadTraffic();
}



ActiveRunway *FGApproachController::getRunway(const string& name)
{
    ActiveRunwayVecIterator rwy = activeRunways.begin();
    if (activeRunways.size()) {
        while (rwy != activeRunways.end()) {
            if (rwy->getRunwayName() == name) {
                break;
            }
            ++rwy;
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
    return string(parent->parent()->getName() + "-approach");
}

int FGApproachController::getFrequency() {
    int groundFreq = parent->getApproachFrequency(2);
    int towerFreq = parent->getTowerFrequency(2);
    return groundFreq>0?groundFreq:towerFreq;
}
