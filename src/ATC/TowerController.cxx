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
#include <ATC/ATCController.hxx>
#include <ATC/trafficcontrol.hxx>
#include <ATC/TowerController.hxx>

using std::sort;
using std::string;

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
    TrafficVectorIterator i = FGATCController::searchActiveTraffic(id);

    // Add a new TrafficRecord if no one exists for this aircraft.
    if (i == activeTraffic.end() || (activeTraffic.empty())) {
        FGTrafficRecord* rec = new FGTrafficRecord();
        rec->setId(id);

        rec->setPositionAndHeading(lat, lon, heading, speed, alt);
        rec->setRunway(intendedRoute->getRunway());
        rec->setLeg(leg);
        rec->setCallsign(ref->getCallSign());
        rec->setRadius(radius);
        rec->setAircraft(ref);
        SGSharedPtr<FGTrafficRecord> sharedRec = static_cast<FGTrafficRecord*>(rec);
        activeTraffic.push_back(sharedRec);
        airportGroundRadar->add(sharedRec);
        // Don't just schedule the aircraft for the tower controller, also assign if to the correct active runway.
        ActiveRunwayVecIterator rwy = activeRunways.begin();
        if (! activeRunways.empty()) {
            while (rwy != activeRunways.end()) {
                if (rwy->getRunwayName() == intendedRoute->getRunway()) {
                    break;
                }
                ++rwy;
            }
        }
        if (rwy == activeRunways.end()) {
            ActiveRunway aRwy(intendedRoute->getRunway(), id);
            aRwy.addToDepartureQueue(ref);
            time_t now = globals->get_time_params()->get_cur_time();
            rec->setPlannedArrivalTime(now);
            rec->setRunwaySlot(aRwy.requestTimeSlot(now));
            activeRunways.push_back(aRwy);
            rwy = (activeRunways.end()-1);
        } else {
            time_t now = globals->get_time_params()->get_cur_time();
            rec->setPlannedArrivalTime(now);
            rec->setRunwaySlot(rwy->requestTimeSlot(now));
            rwy->addToDepartureQueue(ref);
        }

        SG_LOG(SG_ATC, SG_DEBUG, ref->getTrafficRef()->getCallSign() << "(" << ref->getID() << ") You are number " << rwy->getdepartureQueueSize() << " for takeoff from " << parent->parent()->getId() << "/" << rwy->getRunwayName());
    } else {
        airportGroundRadar->move(SGRect<double>(lat, lon), *i);
        (*i)->setPositionAndHeading(lat, lon, heading, speed, alt);
        auto blocker = airportGroundRadar->getBlockedBy(*i);
        if (blocker!=nullptr) {
            (*i)->setWaitsForId(blocker->getId());
            double distM = SGGeodesy::distanceM((*i)->getPos(), blocker->getPos());
            int newSpeed = blocker->getSpeed() * (distM / 100);
            SG_LOG(SG_ATC, SG_DEBUG,
                (*i)->getCallsign() << "(" << (*i)->getId() << ") is blocked for takeoff by " << blocker->getCallsign() << "(" << blocker->getId() << ") new speed " << newSpeed);
            (*i)->setSpeedAdjustment(newSpeed);
        }
    }
}

void FGTowerController::updateAircraftInformation(int id, SGGeod geod,
        double heading, double speed, double alt,
        double dt)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = FGATCController::searchActiveTraffic(id);

    setDt(getDt() + dt);

    time_t now = globals->get_time_params()->get_cur_time();
    if (i == activeTraffic.end() || (activeTraffic.empty())) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: updating aircraft without traffic record at " <<
               SG_ORIGIN);
        return;
    }

    // Update the position of the current aircraft
    (*i)->setPositionAndHeading(geod.getLatitudeDeg(), geod.getLongitudeDeg(), heading, speed, alt);

    // see if we already have a clearance record for the currently active runway
    // NOTE: dd. 2011-08-07: Because the active runway has been constructed in the announcePosition function, we may safely assume that is
    // already exists here. So, we can simplify the current code.

    ActiveRunwayVecIterator rwy = activeRunways.begin();
    //if (parent->getId() == fgGetString("/sim/presets/airport-id")) {
    //    for (rwy = activeRunways.begin(); rwy != activeRunways.end(); ++rwy) {
    //        rwy->printdepartureQueue();
    //    }
    //}

    // rwy = activeRunways.begin();
    while (rwy != activeRunways.end()) {
        if (rwy->getRunwayName() == (*i)->getRunway()) {
            break;
        }
        ++rwy;
    }

    // only bother running the following code if the current aircraft is the
    // first in line for departure
    /* if (current.getAircraft() == rwy->getFirstAircraftIndepartureQueue()) {
        if (rwy->getCleared()) {
            if (id == rwy->getCleared()) {
                current.setHoldPosition(false);
            } else {
                current.setHoldPosition(true);
            }
        } else {
            // For now. At later stages, this will probably be the place to check for inbound traffic.
            rwy->setCleared(id);
        }
    } */
    // only bother with aircraft that have a takeoff status of 2, since those are essentially under tower control
    auto ac = rwy->getFirstAircraftInDepartureQueue();
    if (ac) {
        if (ac->getTakeOffStatus() == AITakeOffStatus::QUEUED) {
            // transmit takeoff clearance
            ac->setTakeOffStatus(AITakeOffStatus::CLEARED_FOR_TAKEOFF);
            TrafficVectorIterator first = searchActiveTraffic(ac->getID());
            //FIXME use checkTransmissionState
            if (first == activeTraffic.end() || activeTraffic.empty()) {
                SG_LOG(SG_ATC, SG_ALERT,
                "FGApproachController updating aircraft without traffic record at " << SG_ORIGIN);
            } else {
                (*first)->setState(ATCMessageState::CLEARED_TAKEOFF);
                transmit((*first), &(*parent), MSG_CLEARED_FOR_TAKEOFF, ATC_GROUND_TO_AIR, true);
            }
        }
    }
    //FIXME Make it an member of traffic record
    if ((*i)->getAircraft()->getTakeOffStatus() == AITakeOffStatus::CLEARED_FOR_TAKEOFF &&
        (*i)->getRunwaySlot() < now) {
        (*i)->setHoldPosition(false);
        if (checkTransmissionState(ATCMessageState::CLEARED_TAKEOFF, ATCMessageState::CLEARED_TAKEOFF, i, now, MSG_ACKNOWLEDGE_CLEARED_FOR_TAKEOFF, ATC_AIR_TO_GROUND)) {
            (*i)->setState(ATCMessageState::ANNOUNCE_ARRIVAL);
        }
    } else {
        (*i)->setHoldPosition(true);
        SG_LOG(SG_ATC, SG_BULK,
                (*i)->getCallsign() << "| Waiting for " << ((*i)->getRunwaySlot() - now) << " seconds");
    }
    int clearanceId = rwy->getCleared();
    if (clearanceId) {
        if (id == clearanceId) {
            SG_LOG(SG_ATC, SG_BULK, (*i)->getCallsign() << "| Unset Hold " << clearanceId << " for rwy " << rwy->getRunwayName());
            (*i)->setHoldPosition(false);
        } else {
            SG_LOG(SG_ATC, SG_BULK, (*i)->getCallsign() << "| Not cleared " << id << " Currently cleared " << clearanceId);
        }
    } else {
        if ((*i)->getAircraft() == rwy->getFirstAircraftInDepartureQueue()) {
            SG_LOG(SG_ATC, SG_BULK,
               (*i)->getCallsign() << "| Cleared for runway " << getName() << " " << rwy->getRunwayName() << " Id " << id);
            auto blocker = airportGroundRadar->getBlockedBy(*i);
            if (blocker==nullptr) {
                // FIXME presumably this can be replaced by ground radar
                rwy->setCleared(id);
                auto l_ac = rwy->getFirstOfStatus(AITakeOffStatus::QUEUED);
                if (l_ac) {
                    l_ac->setTakeOffStatus(AITakeOffStatus::QUEUED);
                    // transmit takeoff clearance? But why twice?
                }
            } else {
                (*i)->setWaitsForId(blocker->getId());
                double distM = SGGeodesy::distanceM((*i)->getPos(), blocker->getPos());
                int newSpeed = blocker->getSpeed() * (distM / 100);
                SG_LOG(SG_ATC, SG_DEBUG,
                    (*i)->getCallsign() << "(" << (*i)->getId() << ") is blocked for takeoff by " << blocker->getCallsign() << "(" << blocker->getId() << ") new speed " << newSpeed);
                (*i)->setSpeedAdjustment(newSpeed);
            }
        } else {
#if 0   // Ticket #2770 : ATC/TowerController floods log
            SG_LOG(SG_ATC, SG_BULK,
               "Not cleared " << current.getAircraft()->getCallSign() << " " << rwy->getFirstAircraftInDepartureQueue()->getCallSign());
#endif
        }
    }
}

void FGTowerController::signOff(int id)
{
    SG_LOG(SG_ATC, SG_BULK, "Signing off " << id << " from Tower");
    // ensure we don't modify activeTraffic during destruction
    if (_isDestroying)
        return;

    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = FGATCController::searchActiveTraffic(id);
    if (i == activeTraffic.end() || (activeTraffic.empty())) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Aircraft without traffic record is signing off from tower at " << SG_ORIGIN);
        return;
    }

    const auto trafficRunway = (*i)->getRunway();
    auto runwayIt = std::find_if(activeRunways.begin(), activeRunways.end(),
                                 [&trafficRunway](const ActiveRunway& ar) {
                                     return ar.getRunwayName() == trafficRunway;
                                 });

    if (runwayIt != activeRunways.end()) {
        SG_LOG(SG_ATC, SG_BULK, (*i)->getCallsign() << "|Cleared " << id << " from " << runwayIt->getRunwayName() << " cleared " << runwayIt->getCleared() );
        runwayIt->removeFromDepartureQueue(id);
    } else {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: Attempting to erase non-existing runway clearance record in FGTowerController::signoff at " << SG_ORIGIN);
    }

    (*i)->getAircraft()->resetTakeOffStatus();
    FGATCController::signOff(id);
}

// Note:
// if we make trafficrecord a member of the base class
// the following three functions: signOff, hasInstruction and getInstruction can
// become devirtualized and be a member of the base ATCController class
// which would simplify code maintenance.
// note that this function is probably obsolete
bool FGTowerController::hasInstruction(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = FGATCController::searchActiveTraffic(id);

    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: checking ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return (*i)->hasInstruction();
    }
    return false;
}


FGATCInstruction FGTowerController::getInstruction(int id)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = FGATCController::searchActiveTraffic(id);

    if (i == activeTraffic.end() || activeTraffic.empty()) {
        SG_LOG(SG_ATC, SG_ALERT,
               "AI error: requesting ATC instruction for aircraft without traffic record at " << SG_ORIGIN);
    } else {
        return (*i)->getInstruction();
    }
    return FGATCInstruction();
}

void FGTowerController::render(bool visible) {
    // this should be bulk, since its called quite often
    SG_LOG(SG_ATC, SG_BULK, "FGTowerController::render function not yet implemented");
}

string FGTowerController::getName() {
    return string(parent->parent()->getName() + "-tower");
}


void FGTowerController::update(double dt)
{    
    FGATCController::eraseDeadTraffic();
}

int FGTowerController::getFrequency() {
    int towerFreq = parent->getTowerFrequency(2);
    return towerFreq;
}
