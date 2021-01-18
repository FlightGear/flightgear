/*
 * Copyright (C) 2020 James Turner
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "test_traffic.hxx"

#include <cstring>
#include <memory>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/performancedb.hxx>
#include <Airports/airport.hxx>
#include <Airports/airportdynamicsmanager.hxx>
#include <Traffic/TrafficMgr.hxx>

#include <ATC/atc_mgr.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void TrafficTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("Traffic");
    FGTestApi::setUp::initNavDataCache();


    auto props = globals->get_props();
    props->setBoolValue("sim/ai/enabled", true);
    props->setBoolValue("sim/signals/fdm-initialized", false);


    // ensure EDDF has a valid ground net for parking testing
    FGAirport::clearAirportsCache();
    FGAirportRef egph = FGAirport::getByIdent("EGPH");
    egph->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "EGPH.groundnet.xml");


    globals->add_new_subsystem<PerformanceDB>();
    globals->add_new_subsystem<FGATCManager>();
    globals->add_new_subsystem<FGAIManager>();
    globals->add_new_subsystem<flightgear::AirportDynamicsManager>();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();
}

// Clean up after each test.
void TrafficTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void TrafficTests::testPushback()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");
    FGAirportRef lfbd = FGAirport::getByIdent("LFBD");

    FGAISchedule* schedule = new FGAISchedule;

    FGAIAircraft* aiAircraft = new FGAIAircraft{schedule};

    // TODO: select a parking stand? or ...
    const SGGeod position = egph->geod();

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    //   aiAircraft->setPath(modelPath.c_str());
    //aircraft->setFlightPlan(flightPlanName);
    aiAircraft->setLatitude(position.getLatitudeDeg());
    aiAircraft->setLongitude(position.getLongitudeDeg());
    //aiAircraft->setAltitude(flight->getCruiseAlt()*100); // convert from FL to feet
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const string flightPlanName = egph->getId() + "-" + lfbd->getId() + ".xml";

    const int radius = 18.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 285;

    const double crs = SGGeodesy::courseDeg(egph->geod(), lfbd->geod()); // direct course
    time_t departureTime;
    time(&departureTime); // now

    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs, departureTime,
                                                          egph, lfbd, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "gate",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    if (fp->isValidPlan()) {
        aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
        globals->get_subsystem<FGAIManager>()->attach(aiAircraft);
    }
}

void TrafficTests::testTrafficManager()
{
    fgSetBool("/sim/traffic-manager/enabled", true);
    fgSetBool("/sim/ai/enabled", true);
    fgSetBool("/environment/realwx/enabled", false);
    fgSetBool("/environment/metar/valid", false);
    fgSetBool("/sim/traffic-manager/active", true);
    fgSetBool("/sim/terrasync/ai-data-update-now", false);

    auto tfc = globals->add_new_subsystem<FGTrafficManager>();


    // specify traffic files to read

    tfc->bind();
    tfc->init();
}
