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

#include <math.h>
#include <cstring>
#include <memory>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <simgear/math/sg_geodesy.hxx>
#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/performancedb.hxx>
#include <Airports/airport.hxx>
#include <Airports/airportdynamicsmanager.hxx>
#include <Airports/groundnetwork.hxx>
#include <Scenery/scenery.hxx>
#include <Time/TimeManager.hxx>
#include <Traffic/TrafficMgr.hxx>

#include <simgear/math/sg_geodesy.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>

#include <simgear/timing/sg_time.hxx>

#include <ATC/atc_mgr.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>


/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void TrafficTests::setUp()
{
    time_t t = time(0);   // get time now

    this->currentWorldTime = t - t%86400 + 86400 + 9 * 60;


    FGTestApi::setUp::initTestGlobals("Traffic");
    FGTestApi::setUp::initNavDataCache();

    fgSetBool("sim/ai/enabled", true);
    fgSetBool("sim/traffic-manager/enabled", true);
    fgSetBool("sim/signals/fdm-initialized", true);
    fgSetInt("/environment/visibility-m", 1000);
    fgSetBool("/environment/realwx/enabled", false);
    fgSetBool("/environment/metar/valid", false);
    fgSetBool("/sim/terrasync/ai-data-update-now", false);
    fgSetBool("/sim/sound/atc/enabled", true);
    fgSetDouble("/instrumentation/comm[0]/frequencies/selected-mhz", 121.70);
    fgSetString("/sim/multiplay/callsign", "AI-Shadow");

    globals->append_data_path(SGPath::fromUtf8(FG_TEST_SUITE_DATA), false);
    globals->set_download_dir(globals->get_fg_home());

    // ensure EDDF has a valid ground net for parking testing
    FGAirport::clearAirportsCache();
    FGAirportRef egph = FGAirport::getByIdent("EGPH");
    egph->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "EGPH.groundnet.xml");
    FGAirportRef yssy = FGAirport::getByIdent("YSSY");
    yssy->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "YSSY.groundnet.xml");
    FGAirportRef ybbn = FGAirport::getByIdent("YBBN");
    ybbn->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "YBBN.groundnet.xml");

    globals->get_subsystem_mgr()->add<PerformanceDB>();
    globals->get_subsystem_mgr()->add<FGATCManager>();
    globals->get_subsystem_mgr()->add<FGAIManager>();
    globals->get_subsystem_mgr()->add<flightgear::AirportDynamicsManager>();
    globals->get_subsystem_mgr()->add<FGTrafficManager>();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();
    // This means time is always 00:09
    FGTestApi::adjustSimulationWorldTime(this->currentWorldTime);
}

// Clean up after each test.
void TrafficTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void TrafficTests::testPushback()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("EGPH");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("EGPF");

    fgSetString("/sim/presets/airport-id", departureAirport->getId());
    fgSetInt("/environment/visibility-m", 1000);
    fgSetInt("/environment/metar/base-wind-speed-kt", 10);
    fgSetInt("/environment/metar/base-wind-dir-deg", 160);

    // Time to depart
    std::string dep = getTimeString(30);
    // Time to arrive
    std::string arr = getTimeString(320);

    const int radius = 18.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;
    const char* flighttype = "gate";

    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", departureAirport->getId(), "G-BLA", "ID", false, "B737", "KLM", "N", flighttype, radius, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushback", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();

    ParkingAssignment parking = departureAirport->getDynamics()->getParkingByName("north-cargo208");

    FGTestApi::setPositionAndStabilise(departureAirport->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);


    const auto flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime + 90;

    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, departureTime+3000,
                                                          departureAirport, arrivalAirport, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "gate",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_testPushback_EGPH_EGPF_" + std::to_string(departureTime));
}

void TrafficTests::testPushbackCargo()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");

    // Time to depart
    std::string dep = getTimeString(30);
    // Time to arrive
    std::string arr = getTimeString(320);


    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", "EGPH", "G-BLA", "ID", false, "B737", "KLM", "N", "cargo", 24, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushbackCargo", "", "EGPH", "EGPF", 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = egph->geod();
    ParkingAssignment parking = egph->getDynamics()->getParkingByName("north-cargo208");
    FGTestApi::setPositionAndStabilise(egph->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const auto flightPlanName = egph->getId() + "-" + egpf->getId() + ".xml";

    const int radius = 16.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;

    const double crs = SGGeodesy::courseDeg(egph->geod(), egpf->geod()); // direct course
    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime + 90;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, departureTime+3000,
                                                          egph, egpf, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "cargo",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_cargo_EGPH_EGPF_" + std::to_string(departureTime));
}

void TrafficTests::testPushbackCargoInProgress()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");

    // Time to depart
    std::string dep = getTimeString(-100);
    // Time to arrive
    std::string arr = getTimeString(190);


    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", "EGPH", "G-BLA", "ID", false, "B737", "KLM", "N", "cargo", 24, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushbackCargo", "", "EGPH", "EGPF", 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = SGGeodesy::direct(egph->geod(), 270, 50000);
    const double crs = SGGeodesy::courseDeg(position, egpf->geod()); // direct course
    ParkingAssignment parking = egph->getDynamics()->getParkingByName("north-cargo208");

    FGTestApi::setPositionAndStabilise(egph->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);
    aiAircraft->setHeading(crs);

    const auto flightPlanName = egph->getId() + "-" + egpf->getId() + ".xml";

    const int radius = 16.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;

    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime - 6000;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, 100,
                                                          egph, egpf, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "cargo",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_cargo_in_progress_EGPH_EGPF_" + std::to_string(departureTime));
}

void TrafficTests::testPushbackCargoInProgressDownWindEast()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");

    // Time to depart
    std::string dep = getTimeString(-100);
    // Time to arrive
    std::string arr = getTimeString(190);


    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", "EGPH", "G-BLA", "ID", false, "B737", "KLM", "N", "cargo", 24, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushbackCargoInProgressDownWindEast", "", "EGPH", "EGPF", 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = SGGeodesy::direct(egph->geod(), 30, 50000);
    const double crs = SGGeodesy::courseDeg(position, egpf->geod()); // direct course
    ParkingAssignment parking = egph->getDynamics()->getParkingByName("north-cargo208");

    FGTestApi::setPositionAndStabilise(egph->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);
    aiAircraft->setHeading(crs);

    const auto flightPlanName = egph->getId() + "-" + egpf->getId() + ".xml";

    const int radius = 16.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;

    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime - 6000;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, 100,
                                                          egph, egpf, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "cargo",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_cargo_in_progress_downwind_east_EGPH_EGPF_" + std::to_string(departureTime));
}

void TrafficTests::testPushbackCargoInProgressDownWindWest()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");

    // Time to depart
    std::string dep = getTimeString(-100);
    // Time to arrive
    std::string arr = getTimeString(190);


    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", "EGPH", "G-BLA", "ID", false, "B737", "KLM", "N", "cargo", 24, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushbackCargoInProgressDownWindWest", "", "EGPH", "EGPF", 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = SGGeodesy::direct(egph->geod(), 300, 50000);
    const double crs = SGGeodesy::courseDeg(position, egpf->geod()); // direct course
    ParkingAssignment parking = egph->getDynamics()->getParkingByName("north-cargo208");

    FGTestApi::setPositionAndStabilise(egph->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);
    aiAircraft->setHeading(crs);

    const auto flightPlanName = egph->getId() + "-" + egpf->getId() + ".xml";

    const int radius = 16.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;

    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime - 6000;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, 100,
                                                          egph, egpf, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "cargo",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_cargo_in_progress_downwind_west_EGPH_EGPF_" + std::to_string(departureTime));
}

void TrafficTests::testPushbackCargoInProgressNotBeyond()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");

    // Time to depart
    std::string dep = getTimeString(-100);
    // Time to arrive
    std::string arr = getTimeString(190);


    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", "EGPH", "G-BLA", "ID", false, "B737", "KLM", "N", "cargo", 24, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushbackCargo", "", "EGPH", "EGPF", 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    // Position west of runway
    const SGGeod position = SGGeodesy::direct(egpf->geod(), 270, 5000);
    const double crs = SGGeodesy::courseDeg(position, egpf->geod()); // direct course
    ParkingAssignment parking = egph->getDynamics()->getParkingByName("north-cargo208");

    FGTestApi::setPositionAndStabilise(egph->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);
    aiAircraft->setHeading(crs);

    const auto flightPlanName = egph->getId() + "-" + egpf->getId() + ".xml";

    const int radius = 16.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;

    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime - 6000;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, 100,
                                                          egph, egpf, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "cargo",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_cargo_in_progress_not_beyond_EGPH_EGPF_" + std::to_string(departureTime));
}

void TrafficTests::testPushbackCargoInProgressNotBeyondNorth()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");

    // Time to depart
    std::string dep = getTimeString(-100);
    // Time to arrive
    std::string arr = getTimeString(190);


    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", "EGPH", "G-BLA", "ID", false, "B737", "KLM", "N", "cargo", 24, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushbackCargo", "", "EGPH", "EGPF", 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    // Position west of runway
    const SGGeod position = SGGeodesy::direct(egpf->geod(), 270, 5000);
    const double crs = 0; // direct course
    ParkingAssignment parking = egph->getDynamics()->getParkingByName("north-cargo208");

    FGTestApi::setPositionAndStabilise(egph->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);
    aiAircraft->setHeading(crs);

    const auto flightPlanName = egph->getId() + "-" + egpf->getId() + ".xml";

    const int radius = 16.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;

    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime - 6000;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, 100,
                                                          egph, egpf, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "cargo",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_cargo_in_progress_not_beyond_north_EGPH_EGPF_" + std::to_string(departureTime));
}

void TrafficTests::testPushbackCargoInProgressBeyond()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");

    // Time to depart
    std::string dep = getTimeString(-100);
    // Time to arrive
    std::string arr = getTimeString(190);


    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", "EGPH", "G-BLA", "ID", false, "B737", "KLM", "N", "cargo", 24, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushbackCargo", "", "EGPH", "EGPF", 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    // Position east of runway pointing away from runway
    const SGGeod position = SGGeodesy::direct(egpf->geod(), 90, 5000);

    const double crs = SGMiscd::normalizePeriodic(0, 360, SGGeodesy::courseDeg(position, egpf->geod()));
    ParkingAssignment parking = egph->getDynamics()->getParkingByName("north-cargo208");

    FGTestApi::setPositionAndStabilise(egph->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);
    aiAircraft->setHeading(crs);

    const auto flightPlanName = egph->getId() + "-" + egpf->getId() + ".xml";

    const int radius = 16.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;

    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime - 6000;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, 100,
                                                          egph, egpf, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "cargo",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_cargo_in_progress_beyond_EGPH_EGPF_" + std::to_string(departureTime));
}

void TrafficTests::testPushbackCargoInProgressBeyondNorth()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");

    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");

    // Time to depart
    std::string dep = getTimeString(-100);
    // Time to arrive
    std::string arr = getTimeString(190);


    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", "EGPH", "G-BLA", "ID", false, "B737", "KLM", "N", "cargo", 24, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushbackCargo", "", "EGPH", "EGPF", 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    // Position east of runway pointing away from runway
    const SGGeod position = SGGeodesy::direct(egpf->geod(), 90, 5000);

    const double crs = 300;
    ParkingAssignment parking = egph->getDynamics()->getParkingByName("north-cargo208");

    FGTestApi::setPositionAndStabilise(egph->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);
    aiAircraft->setHeading(crs);

    const auto flightPlanName = egph->getId() + "-" + egpf->getId() + ".xml";

    const int radius = 16.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;

    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime - 6000;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, 100,
                                                          egph, egpf, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, "cargo",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_cargo_in_progress_beyond_north_EGPH_EGPF_" + std::to_string(departureTime));
}

void TrafficTests::testChangeRunway()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("EGPH");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", departureAirport->getId());
    fgSetInt("/environment/visibility-m", 1000);
    fgSetInt("/environment/metar/base-wind-speed-kt", 10);
    fgSetInt("/environment/metar/base-wind-dir-deg", 160);

    // Time to depart
    std::string dep = getTimeString(60);
    // Time to arrive
    std::string arr = getTimeString(320);

    const int radius = 24.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;
    const char* flighttype = "gate";

    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", departureAirport->getId(), "G-BLA", "ID", false, "B737", "KLM", "N", flighttype, radius, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testChangeRunway", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const auto flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime + 90;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, departureTime+3000,
                                                          departureAirport, arrivalAirport, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, flighttype,
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_change_runway_EGPH_EGPF_" + std::to_string(departureTime));
}


void TrafficTests::testPushforward()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("YSSY");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("YBBN");
    fgSetString("/sim/presets/airport-id", departureAirport->getId());

    // Time to depart
    std::string dep = getTimeString(60);
    // Time to arrive
    std::string arr = getTimeString(320);

    const int radius = 8.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;
    const char* flighttype = "ga";

    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", departureAirport->getId(), "G-BLA", "ID", false, "B737", "KLM", "N", flighttype, radius, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushforward", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const auto flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime + 90;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, departureTime+3000,
                                                          departureAirport, arrivalAirport, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, flighttype,
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_ga_YSSY_depart_" + std::to_string(departureTime));
}

void TrafficTests::testPushforwardSpeedy()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("YSSY");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("YBBN");
    fgSetString("/sim/presets/airport-id", departureAirport->getId());

    // Time to depart
    std::string dep = getTimeString(60);
    // Time to arrive
    std::string arr = getTimeString(320);

    const int radius = 8.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;
    const char* flighttype = "ga";

    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", departureAirport->getId(), "G-BLA", "ID", false, "B737", "KLM", "N", flighttype, radius, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushforwardSpeedy", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("NotValid", "jet_transport");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const auto flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime + 90;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, departureTime+3000,
                                                          departureAirport, arrivalAirport, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, flighttype,
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_ga_YSSY_fast_depart_" + std::to_string(departureTime));
}

void TrafficTests::testPushforwardParkYBBN()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("YBBN");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("YSSY");

    fgSetString("/sim/presets/airport-id", arrivalAirport->getId());

    // Time to depart
    std::string dep = getTimeString(60);
    // Time to arrive
    std::string arr = getTimeString(3260);

    const int radius = 8.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;
    const char* flighttype = "ga";

    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", departureAirport->getId(), "G-BLA", "ID", false, "B737", "KLM", "N", flighttype, radius, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushforwardParkYBBN", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("ga", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const auto flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime + 90;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, departureTime+3000,
                                                          departureAirport, arrivalAirport, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, flighttype,
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));

    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_ga_YSSY_YBBN_park_" + std::to_string(departureTime));

    int shortestDistance = 10000;
    const FGParkingList& parkings(arrivalAirport->groundNetwork()->allParkings());
    FGParkingList::const_iterator it;
    FGParking* nearestParking = 0;
    for (it = parkings.begin(); it != parkings.end(); ++it) {
        int currentDistance = !nearestParking ? 9999 : SGGeodesy::distanceM(nearestParking->geod(), (*it)->geod());
        if (currentDistance < shortestDistance) {
            nearestParking = (*it);
            shortestDistance = currentDistance;
            /*
            std::cout << (*it)->name() << "\t" << (*it)->getHeading()
                      << "\t" << shortestDistance << "\t" << (*it)->geod() << "\n";
            */
        }
    }

    CPPUNIT_ASSERT_EQUAL(true, aiAircraft->getDie());
}

void TrafficTests::testPushforwardParkYBBNRepeatGa()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("YBBN");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("YSSY");

    fgSetString("/sim/presets/airport-id", arrivalAirport->getId());

    // Time to depart
    std::string dep = getTimeString(120);
    // Time to arrive
    std::string arr = getTimeString(3260);
    // Time to arrive back
    std::string ret = getTimeString(6460);

    const int radius = 8.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;
    const char* flighttype = "ga";

    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", departureAirport->getId(), "G-BLA", "TST_BN_1", false, "B737", "KLM", "N", flighttype, radius, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushforwardParkYBBNRepeatGa", "VFR", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "TST_BN_1");
    schedule->assign(flight);

    FGScheduledFlight* returnFlight = new FGScheduledFlight("testPushforwardParkYBBNRepeatGa", "", arrivalAirport->getId(), departureAirport->getId(), 24, arr, ret, "WEEK", "TST_BN_1");
    schedule->assign(returnFlight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("ga", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const auto flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime + 90;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs, departureTime, departureTime+3000,
                                                          departureAirport, arrivalAirport, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, flighttype,
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));
    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    aiAircraft = flyAI(aiAircraft, "flight_ga_YSSY_YBBN_park_repeat" + std::to_string(departureTime));

    int shortestDistance = 10000;
    const FGParkingList& parkings(arrivalAirport->groundNetwork()->allParkings());
    FGParkingList::const_iterator it;
    FGParking* nearestParking = 0;
    for (it = parkings.begin(); it != parkings.end(); ++it) {
        int currentDistance = !nearestParking ? 9999 : SGGeodesy::distanceM(nearestParking->geod(), (*it)->geod());
        if (currentDistance < shortestDistance) {
            nearestParking = (*it);
            shortestDistance = currentDistance;
        }
    }

    CPPUNIT_ASSERT_EQUAL(true, (aiAircraft->getDie() || aiAircraft->GetFlightPlan()->getCurrentWaypoint()->getName() == "park"));
}

void TrafficTests::testPushforwardParkYBBNRepeatGaDelayed()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("YBBN");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("YSSY");

    fgSetString("/sim/presets/airport-id", arrivalAirport->getId());

    // Time to depart
    std::string dep = getTimeString(120);
    // Time to arrive
    std::string arr = getTimeString(3260);
    // Time to arrive back
    std::string ret = getTimeString(6460);

    const int radius = 8.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;
    const char* flighttype = "ga";

    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", departureAirport->getId(), "G-BLA", "TST_BN_1", false, "B737", "KLM", "N", flighttype, radius, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("testPushforwardParkYBBNRepeatGaDelayed", "VFR", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "TST_BN_1");
    schedule->assign(flight);

    FGScheduledFlight* returnFlight = new FGScheduledFlight("testPushforwardParkYBBNRepeatGaDelayed", "", arrivalAirport->getId(), departureAirport->getId(), 24, arr, ret, "WEEK", "TST_BN_1");
    schedule->assign(returnFlight);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("ga", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const auto flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime + 90;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, departureTime+3000,
                                                          departureAirport, arrivalAirport, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, flighttype,
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));
    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    FGAirport* departure = aiAircraft->getTrafficRef()->getDepartureAirport();
    FGAirportDynamicsRef departureDynamics = departure->getDynamics();
    ActiveRunway* activeDepartureRunway = departureDynamics->getApproachController()->getRunway("01");
    time_t newDeparture = activeDepartureRunway->requestTimeSlot(aiAircraft->GetFlightPlan()->getStartTime());
// See that the wait queue is filled
    for (size_t i = 0; i < 10; i++)
    {
        newDeparture = activeDepartureRunway->requestTimeSlot(newDeparture);
    }

    FGAirport* arrival = aiAircraft->getTrafficRef()->getArrivalAirport();
    FGAirportDynamicsRef arrivalDynamics = arrival->getDynamics();
    ActiveRunway* activeYSSYRunway = arrivalDynamics->getApproachController()->getRunway("16R");
    time_t newArrival = activeYSSYRunway->requestTimeSlot(aiAircraft->GetFlightPlan()->getStartTime());
// See that the wait queue is filled
    for (size_t i = 0; i < 100; i++)
    {
        newArrival = activeYSSYRunway->requestTimeSlot(newArrival);
    }

    aiAircraft = flyAI(aiAircraft, "flight_ga_YSSY_YBBN_park_repeatdelayed" + std::to_string(departureTime));

    int shortestDistance = 10000;
    const FGParkingList& parkings(arrivalAirport->groundNetwork()->allParkings());
    FGParkingList::const_iterator it;
    FGParking* nearestParking = 0;
    for (it = parkings.begin(); it != parkings.end(); ++it) {
        int currentDistance = !nearestParking ? 9999 : SGGeodesy::distanceM(nearestParking->geod(), (*it)->geod());
        if (currentDistance < shortestDistance) {
            nearestParking = (*it);
            shortestDistance = currentDistance;
        }
    }

    CPPUNIT_ASSERT_EQUAL(true, (aiAircraft->getDie() || aiAircraft->GetFlightPlan()->getCurrentWaypoint()->getName() == "park"));
}

void TrafficTests::testPushforwardParkYBBNRepeatGate()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("YBBN");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("YSSY");

    fgSetString("/sim/presets/airport-id", arrivalAirport->getId());

    // Time to depart
    std::string dep = getTimeString(10);
    // Time to arrive
    std::string arr = getTimeString(3260);
    // Time to arrive back
    std::string ret = getTimeString(6460);

    const int radius = 32.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;
    const char* flighttype = "gate";

    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", departureAirport->getId(), "G-BLA", "TST_BN_2", false, "B737", "KLM", "N", flighttype, radius, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("gateParkYSSY", "VFR", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "TST_BN_1");
    schedule->assign(flight);

    FGScheduledFlight* returnFlight = new FGScheduledFlight("gateParkYSSY", "", arrivalAirport->getId(), departureAirport->getId(), 24, arr, ret, "WEEK", "TST_BN_1");
    schedule->assign(returnFlight);

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    SGSharedPtr<FGAIAircraft> aiAircraft = new FGAIAircraft{schedule};

    aiAircraft->setPerformance("gate", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const auto flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime = globals->get_time_params()->get_cur_time();
    departureTime = departureTime + 90;


    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs,
                                                          departureTime, departureTime+3000,
                                                          departureAirport, arrivalAirport, true, radius,
                                                          cruiseAltFt, // cruise alt
                                                          position.getLatitudeDeg(),
                                                          position.getLongitudeDeg(),
                                                          cruiseSpeedKnots, flighttype,
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany()));
    CPPUNIT_ASSERT_EQUAL(fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    CPPUNIT_ASSERT_EQUAL(aiAircraft->GetFlightPlan()->isValidPlan(), true);

    aiAircraft = flyAI(aiAircraft, "flight_gate_YSSY_YBBN_park_repeat" + std::to_string(departureTime));

    int shortestDistance = 10000;
    const FGParkingList& parkings(arrivalAirport->groundNetwork()->allParkings());
    FGParkingList::const_iterator it;
    FGParking* nearestParking = 0;
    for (it = parkings.begin(); it != parkings.end(); ++it) {
        int currentDistance = !nearestParking ? 9999 : SGGeodesy::distanceM(nearestParking->geod(), (*it)->geod());
        if (currentDistance < shortestDistance) {
            nearestParking = (*it);
            shortestDistance = currentDistance;
        }
    }

    CPPUNIT_ASSERT_EQUAL(true, (aiAircraft->getDie() || aiAircraft->GetFlightPlan()->getCurrentWaypoint()->getName() == "park"));
}

/**
 *
 *
 *
 */

FGAIAircraft * TrafficTests::flyAI(SGSharedPtr<FGAIAircraft> aiAircraft, std::string testname) {
    int lineIndex = 0;

    CPPUNIT_ASSERT_EQUAL(aiAircraft->GetFlightPlan()->isValidPlan(), true);


    time_t now = globals->get_time_params()->get_cur_time();
    tm* startTime = localtime(&now);
    time_t departureTime = aiAircraft->GetFlightPlan()->getStartTime();

    char buffer[50];
    char buffer2[50];

    strftime (buffer,50,"%FT%TZ",startTime);
    strftime (buffer2,50,"%FT%TZ", localtime(&departureTime));

    SG_LOG(SG_AI, SG_DEBUG, "Start Time " << buffer << " First Departure " << buffer2);

    char fname [160];
    time_t t = time(0);   // get time now
    snprintf (fname, sizeof(fname), "%ld.csv", t);
    SGPath p = SGPath::desktop() / (testname + fname);
    std::unique_ptr<sg_ofstream> csvFile = std::make_unique<sg_ofstream>();
    (*csvFile).open(p);
    if(!(*csvFile).is_open()) {
        SG_LOG(SG_AI, SG_DEBUG, "CSV File " << fname << " couldn't be opened");
    }
    if (sglog().get_log_priority() <= SG_DEBUG) {
        aiAircraft->dumpCSVHeader(csvFile);
        FGTestApi::setUp::logLinestringsToKML(testname);
    }
    flightgear::SGGeodVec geods = flightgear::SGGeodVec();
    int iteration = 1;
    int lastLeg = -1;
    double lastHeading = -500;
    double headingSum = 0;
    int startSpeed = aiAircraft->GetFlightPlan()->getCurrentWaypoint()->getSpeed();
    aiAircraft->AccelTo(startSpeed);

    for (size_t i = 0; i < 12000000 && !(aiAircraft->getDie()) && aiAircraft->GetFlightPlan()->getLeg() <= AILeg::PARKING; i++) {
        CPPUNIT_ASSERT_EQUAL(aiAircraft->GetFlightPlan()->isValidPlan(), true);
        if (!aiAircraft->getDie()) {
            // collect position
            if (geods.empty() ||
                SGGeodesy::distanceM(aiAircraft->getGeodPos(), geods.back()) > 0.05) {
                geods.insert(geods.end(), aiAircraft->getGeodPos());
            }
            // follow aircraft
            if (geods.empty() ||
                (aiAircraft->getSpeed() > 0 &&
                 SGGeodesy::distanceM(aiAircraft->getGeodPos(), FGTestApi::getPosition()) > 10000 &&
                    /* stop following towards the end*/
                    aiAircraft->GetFlightPlan()->getLeg() < 8))
            {
                FGTestApi::setPosition(aiAircraft->getGeodPos());
            }
        }
        // Leg has been incremented
        if (aiAircraft->GetFlightPlan()->getLeg() != lastLeg ) {
        // The current WP is really in our new leg
            if (sglog().get_log_priority() <= SG_DEBUG) {
                snprintf(buffer, sizeof(buffer), "AI Leg %d Callsign %s Iteration %d", lastLeg, aiAircraft->getCallSign().c_str(), iteration);
                FGTestApi::writeGeodsToKML(buffer, geods);
            }
            if (aiAircraft->GetFlightPlan()->getLeg() < lastLeg) {
                iteration++;
            }
            lastLeg = aiAircraft->GetFlightPlan()->getLeg();
            SGGeod last = geods.back();
            geods.clear();
            geods.insert(geods.end(), last);
        }
        if (lastHeading==-500) {
            lastHeading = aiAircraft->getTrueHeadingDeg();
        }
        headingSum += (lastHeading-aiAircraft->getTrueHeadingDeg());
        lastHeading = aiAircraft->getTrueHeadingDeg();
        aiAircraft->dumpCSV(csvFile, lineIndex++);
        // A flight without loops should never reach 400°
        CPPUNIT_ASSERT_LESSEQUAL(400.0, headingSum);
        CPPUNIT_ASSERT_LESSEQUAL( 10, aiAircraft->GetFlightPlan()->getLeg());
        CPPUNIT_ASSERT_MESSAGE( "Aircraft has not completed test in time.", i < 3000000);
        // Arrived at a parking
        int beforeNextDepTime = aiAircraft->getTrafficRef()->getDepartureTime() - 30;

        if (iteration > 1
        && aiAircraft->GetFlightPlan()->getLeg() == 1
        && aiAircraft->getSpeed() == 0
        && this->currentWorldTime < beforeNextDepTime) {
            FGTestApi::adjustSimulationWorldTime(beforeNextDepTime);
            SG_LOG(SG_AI, SG_BULK, "Jumped time " << (beforeNextDepTime - this->currentWorldTime) );
            this->currentWorldTime = beforeNextDepTime;
        }
        FGTestApi::runForTime(1);
        FGTestApi::adjustSimulationWorldTime(++this->currentWorldTime);
    }
    lastLeg = aiAircraft->GetFlightPlan()->getLeg();
    snprintf(buffer, sizeof(buffer), "AI Leg %d Callsign %s Iteration %d", lastLeg, aiAircraft->getCallSign().c_str(), iteration);
    if (sglog().get_log_priority() <= SG_DEBUG) {
        FGTestApi::writeGeodsToKML(buffer, geods);
    }
    geods.clear();
    (*csvFile).close();
    return aiAircraft;
}

std::string TrafficTests::getTimeString(int timeOffset)
{
    char ret[11];
    time_t rawtime = globals->get_time_params()->get_cur_time();
    rawtime = rawtime + timeOffset;
    tm* timeinfo = gmtime(&rawtime);
    strftime(ret, 11, "%w/%H:%M:%S", timeinfo);
    return ret;
}
