/*
 * SPDX-FileName: EnRouteController.cxx
 * SPDX-FileComment: ATC Controller controlling Leg 3/4
 * SPDX-FileCopyrightText: Copyright (C) 2025  Keith Paterson - keith.paterson@gmx.de
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <Airports/airports_fwd.hxx>

#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <ATC/ATCController.hxx>
#include <ATC/EnRouteController.hxx>

using std::string;

string EnRouteController::getTransponderCode(const string& fltRules)
{
    return genTransponderCode(fltRules);
};

int EnRouteController::getFrequency() { return 1; };
void EnRouteController::announcePosition(int id,
                                         FGAIFlightPlan* intendedRoute, int currentRoute,
                                         double lat, double lon,
                                         double heading, double speed,
                                         double alt, double radius,
                                         int leg,
                                         FGAIAircraft* aircraft)
{
    // Search activeTraffic for a record matching our id
    TrafficVectorIterator i = FGATCController::searchActiveTraffic(id);

    // Add a new TrafficRecord if no one exsists for this aircraft.
    if (i == activeTraffic.end() || activeTraffic.empty()) {
        FGTrafficRecord* rec = new FGTrafficRecord();
        rec->setId(id);

        rec->setPositionAndHeading(lat, lon, heading, speed, alt);
        rec->setRunway(intendedRoute->getRunway());
        rec->setLeg(leg);
        rec->setCallsign(aircraft->getCallSign());
        rec->setAircraft(aircraft);
        rec->setPlannedArrivalTime(intendedRoute->getArrivalTime());
        activeTraffic.push_back(rec);
    } else {
        (*i)->setPositionAndHeading(lat, lon, heading, speed, alt);
        (*i)->setPlannedArrivalTime(intendedRoute->getArrivalTime());
    }
};
void EnRouteController::updateAircraftInformation(int id, SGGeod geod,
                                                  double heading, double speed, double alt, double dt) {};
void EnRouteController::render(bool) {};
std::string EnRouteController::getName() { return "EnRoute Controller"; };
void EnRouteController::update(double) {};
