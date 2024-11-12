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
#include <simgear/math/sg_geodesy.hxx>

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

/***************************************************************************
 * ActiveRunway
 **************************************************************************/

ActiveRunway::ActiveRunway(const std::string& r, int cc) :
    rwy(r)
{
    currentlyCleared = cc;
    distanceToFinal = 6.0 * SG_NM_TO_METER;
};

void ActiveRunway::removeFromDepartureQueue(int id) {
    if (id!=currentlyCleared) {
        printDepartureQueue();
        SG_LOG(SG_ATC, SG_WARN, "Not cleared id being removed from DepartureQueue " << id << " currently cleared " << currentlyCleared);
    } else {
        setCleared(0);
        updateDepartureQueue();
        printDepartureQueue();
    }
}

void ActiveRunway::updateDepartureQueue()
{
    departureQueue.erase(departureQueue.begin());
}

/**
* Fetch next slot for the active runway
* @param eta time of slot requested
* @return newEta: next slot available; starts at eta paramater
* and adds separation as needed
*/
time_t ActiveRunway::requestTimeSlot(time_t eta)
{
    time_t newEta = 0;

    // if the aircraft is the first arrival, add to the vector and return eta directly
    if (estimatedArrivalTimes.empty()) {
        estimatedArrivalTimes.push_back(eta);
        SG_LOG(SG_ATC, SG_BULK, getRunwayName() << " Checked eta slots, using " << eta);
        return eta;
    } else {
        // First check the already assigned slots to see where we need to fit the flight in
        SG_LOG(SG_ATC, SG_BULK, getRunwayName() << " Checking eta slots " << eta << " : " << estimatedArrivalTimes.size() << " Timediff : " << (eta - globals->get_time_params()->get_cur_time()));

        // is this needed - just a debug output?
        TimeVectorIterator i;
        for (i = estimatedArrivalTimes.begin();
                i != estimatedArrivalTimes.end(); ++i) {
            SG_LOG(SG_ATC, SG_BULK, "Stored time : " << (*i));
        }

        // if the flight is before the first scheduled slot + separation
        time_t separation = 60;
        i = estimatedArrivalTimes.begin();
        if ((eta + separation) < (*i)) {
            newEta = eta;
            SG_LOG(SG_ATC, SG_DEBUG, "Start. New ETA : " << newEta );
            slotHousekeeping(newEta);
            return newEta;
        }

        // else, look through the rest of the slots
        bool found = false;
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
                SG_LOG(SG_ATC, SG_DEBUG, "End. New ETA : " << newEta << " Timediff : " << (newEta-eta));
                slotHousekeeping(newEta);
                return newEta;
            } else {
                // potential slot found
                // check the distance between the previous and next slots
                // distance must be greater than 2* separation
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
            ++i;
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

/** Output the contents of the departure queue vector nicely formatted*/
void ActiveRunway::printDepartureQueue()
{
    SG_LOG(SG_ATC, SG_DEBUG, "Departure queue for " << rwy << ": ");
    for (auto acft : departureQueue) {
        SG_LOG(SG_ATC, SG_DEBUG, "     " << acft->getCallSign() << " " << acft->getTakeOffStatus());
        SG_LOG(SG_ATC, SG_DEBUG, " " << acft->_getLatitude() << " " << acft->_getLongitude() << acft->getSpeed() << " " << acft->getAltitude());
    }

}

/** Fetch the first aircraft in the departure queue with a certain status */
SGSharedPtr<FGAIAircraft>ActiveRunway::getFirstOfStatus(int stat) const
{
    auto it = std::find_if(departureQueue.begin(), departureQueue.end(), [stat](const SGSharedPtr<FGAIAircraft>& acft) {
        return acft->getTakeOffStatus() == stat;
    });

    if (it == departureQueue.end()) {
        return {};
    }

    return *it;
}

SGSharedPtr<FGAIAircraft> ActiveRunway::getFirstAircraftInDepartureQueue() const
{
    if (departureQueue.empty()) {
        return {};
    }

    return departureQueue.front();
};

void ActiveRunway::addToDepartureQueue(FGAIAircraft *ac)
{
    assert(ac);
    assert(!ac->getDie());
    departureQueue.push_back(ac);
};


/***************************************************************************
 * FGTrafficRecord
 **************************************************************************/

FGTrafficRecord::FGTrafficRecord():
        id(0),
        currentPos(0),
        leg(0),
        frequencyId(0),
        state(0),
        allowTransmission(true),
        allowPushback(true),
        priority(0),
        timer(0),
        heading(0), speed(0), altitude(0), radius(0)
{
}

FGTrafficRecord::~FGTrafficRecord()
{
}

void FGTrafficRecord::setPositionAndIntentions(int pos,
        FGAIFlightPlan * route)
{
    SG_LOG(SG_AI, SG_BULK, "Traffic record position: " << pos);
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
        SG_LOG(SG_ATC, SG_DEBUG, "Setting pos to " << currentPos << " and intentions");
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

bool FGTrafficRecord::isDead() const {
        if (!aircraft) {
            return true;
        }
        return aircraft->getDie();
    }

    void FGTrafficRecord::clearATCController() const {
        if (aircraft) {
           aircraft->clearATCController();
        }
    }

    FGAIAircraft* FGTrafficRecord::getAircraft() const
    {
        if(aircraft.valid()) {
          return aircraft.ptr();
        }
        return 0;
    }
/**
* Check if another aircraft is ahead of the current one, and on the same taxiway
* @return true / false if this is/isn't the case.
*/
bool FGTrafficRecord::checkPositionAndIntentions(FGTrafficRecord & other)
{
    bool result = false;
    SG_LOG(SG_ATC, SG_BULK, getCallsign() << "| checkPositionAndIntentions CurrentPos : " << currentPos << " Other : " << other.currentPos << " Leg : " << leg << " Other Leg : " << other.leg );
    if (currentPos == other.currentPos && getId() != other.getId() ) {
        SG_LOG(SG_ATC, SG_BULK, getCallsign() << "| Check Position and intentions: " << other.getCallsign() << " we are on the same taxiway; Index = " << currentPos);
        int headingTowards = SGGeodesy::courseDeg( other.getPos(), getPos() );
        int headingDiff = SGMiscd::normalizePeriodic(-180, 180, headingTowards - getHeading() );
        SG_LOG(SG_ATC, SG_BULK, getCallsign() << "| " << heading << "\t" << headingTowards << "\t" << headingDiff);
        // getHeading()
        result = abs(headingDiff) < 89;
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
    else if (!intentions.empty()) {
        SG_LOG(SG_ATC, SG_BULK, getCallsign() << "| Itentions Size " << intentions.size());
        intVecIterator i = intentions.begin();
        //while (!((i == intentions.end()) || ((*i) == other.currentPos)))
        while (i != intentions.end()) {
            if ((*i) == other.currentPos) {
                break;
            }
            ++i;
        }
        if (i != intentions.end()) {
            SG_LOG(SG_ATC, SG_BULK, getCallsign() << "| Check Position and intentions: " << other.getCallsign()<< " matches Index = " << (*i));
            int headingTowards = SGGeodesy::courseDeg( other.getPos(), getPos() );
            int distanceM = SGGeodesy::distanceM( other.getPos(), getPos() );
            int headingDiff = SGMiscd::normalizePeriodic(-180, 180, headingTowards - getHeading() );
            SG_LOG(SG_ATC, SG_BULK, getCallsign() << "| Heading : " << heading << "\t Heading Other->Current" << headingTowards << "\t Heading Diff :" << headingDiff << "\t Distance : " << distanceM);
            // difference of heading is small and it's actually near
            result = abs(headingDiff) < 89 && distanceM < 400;
//            result = true;
        }
    }
    return result;
}

void FGTrafficRecord::setPositionAndHeading(double lat, double lon,
        double hdg, double spd,
        double alt)
{
    this->pos = SGGeod::fromDegFt(lon, lat, alt);
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
        for (i = intentions.begin(); i != intentions.end(); ++i) {
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
        for (i = other.intentions.begin(); i != other.intentions.end(); ++i) {
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
        for (i = intentions.begin(); i != intentions.end(); ++i) {
            for (j = other.intentions.begin(); j != other.intentions.end(); ++j) {
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
                i != other.intentions.end(); ++i) {
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
    SG_LOG(SG_ATC, SG_BULK, "Current segment " << currentPos);

    if ((currentPos > 0) && (other.currentPos > 0)) {
        FGTaxiSegment *opp = net->findSegment(currentPos)->opposite();
        if (opp) {
            if (opp->getIndex() == other.currentPos)
                return true;
        }

        for (intVecIterator i = intentions.begin(); i != intentions.end(); ++i) {
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
                        j != other.intentions.end(); ++j) {
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
    waitsForId = 0;
}

bool FGATCInstruction::hasInstruction() const
{
    return (holdPattern || holdPosition || changeSpeed || changeHeading
            || changeAltitude || resolveCircularWait);
}
