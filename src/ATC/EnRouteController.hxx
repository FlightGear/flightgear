/*
 * SPDX-FileName: EnRouteController.hxx
 * SPDX-FileComment: ATC Controller controlling Leg 4
 * SPDX-FileCopyrightText: Copyright (C) 2025  Keith Paterson - keith.paterson@gmx.de
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <Airports/airports_fwd.hxx>

#include <ATC/ATCController.hxx>

using std::string;

class EnRouteController : public FGATCController
{
public:
    string getTransponderCode(const string& fltRules);

    int getFrequency();
    void announcePosition(int id,
                          FGAIFlightPlan* intendedRoute, int currentRoute,
                          double lat, double lon,
                          double heading, double speed,
                          double alt, double radius,
                          int leg,
                          FGAIAircraft* aircraft);
    void updateAircraftInformation(int id, SGGeod geod,
                                   double heading, double speed, double alt, double dt);
    void render(bool);
    std::string getName();
    void update(double);
};
