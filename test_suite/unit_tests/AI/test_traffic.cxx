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
#include <unistd.h>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

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

#include <ATC/atc_mgr.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void TrafficTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("Traffic");
    FGTestApi::setUp::initNavDataCache();

    fgSetBool("sim/ai/enabled", true);
    fgSetBool("sim/traffic-manager/enabled", true);
    fgSetBool("sim/signals/fdm-initialized", true);
    fgSetInt("/environment/visibility-m", 1000);
    fgSetBool("/environment/realwx/enabled", false);
    fgSetBool("/environment/metar/valid", false);
    fgSetBool("/sim/terrasync/ai-data-update-now", false);

    globals->append_data_path(SGPath::fromUtf8(FG_TEST_SUITE_DATA), false);

    // ensure EDDF has a valid ground net for parking testing
    FGAirport::clearAirportsCache();
    FGAirportRef egph = FGAirport::getByIdent("EGPH");
    egph->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "EGPH.groundnet.xml");
    FGAirportRef yssy = FGAirport::getByIdent("YSSY");
    yssy->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "YSSY.groundnet.xml");
    FGAirportRef ybbn = FGAirport::getByIdent("YBBN");
    ybbn->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "YBBN.groundnet.xml");

    globals->add_new_subsystem<PerformanceDB>(SGSubsystemMgr::GENERAL);
    globals->add_new_subsystem<FGATCManager>(SGSubsystemMgr::GENERAL);
    globals->add_new_subsystem<FGAIManager>(SGSubsystemMgr::GENERAL);
    globals->add_new_subsystem<flightgear::AirportDynamicsManager>(SGSubsystemMgr::GENERAL);
    globals->add_new_subsystem<FGTrafficManager>(SGSubsystemMgr::GENERAL);

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
    FGAirportRef departureAirport = FGAirport::getByIdent("EGPH");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", departureAirport->getId());

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
    FGScheduledFlight* flight = new FGScheduledFlight("", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    FGAIAircraft* aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();

    ParkingAssignment parking = departureAirport->getDynamics()->getParkingByName("north-cargo208");

    FGTestApi::setPositionAndStabilise(departureAirport->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const string flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime;
    time(&departureTime); // now
    departureTime = departureTime - 50;

    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs, departureTime,
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

    aiAircraft = flyAI(aiAircraft, "flight_EGPH_EGPF_" + std::to_string(departureTime));
    CPPUNIT_ASSERT_EQUAL(5, aiAircraft->GetFlightPlan()->getLeg());
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
    FGScheduledFlight* flight = new FGScheduledFlight("", "", "EGPH", "EGPF", 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    FGAIAircraft* aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = egph->geod();
    ParkingAssignment parking = egph->getDynamics()->getParkingByName("north-cargo208");
    FGTestApi::setPositionAndStabilise(egph->getDynamics()->getParkingByName("ga206").parking()->geod());

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const string flightPlanName = egph->getId() + "-" + egpf->getId() + ".xml";

    const int radius = 16.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;

    const double crs = SGGeodesy::courseDeg(egph->geod(), egpf->geod()); // direct course
    time_t departureTime;
    time(&departureTime); // now
    departureTime = departureTime - 50;

    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs, departureTime,
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

    aiAircraft = flyAI(aiAircraft, "flight_cargo_EDPH_" + std::to_string(departureTime));

    CPPUNIT_ASSERT_EQUAL(5, aiAircraft->GetFlightPlan()->getLeg());
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
    FGScheduledFlight* flight = new FGScheduledFlight("", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    FGAIAircraft* aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const string flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime;
    time(&departureTime); // now
    departureTime = departureTime - 50;

    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs, departureTime,
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

    aiAircraft = flyAI(aiAircraft, "flight_runway_EGPH_" + std::to_string(departureTime));

    CPPUNIT_ASSERT_EQUAL(5, aiAircraft->GetFlightPlan()->getLeg());
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
    FGScheduledFlight* flight = new FGScheduledFlight("", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    FGAIAircraft* aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("jet_transport", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const string flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime;
    time(&departureTime); // now
    departureTime = departureTime - 50;

    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs, departureTime,
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

    CPPUNIT_ASSERT_EQUAL(5, aiAircraft->GetFlightPlan()->getLeg());
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
    FGScheduledFlight* flight = new FGScheduledFlight("", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    FGAIAircraft* aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("NotValid", "jet_transport");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const string flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime;
    time(&departureTime); // now
    departureTime = departureTime - 50;

    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs, departureTime,
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

    CPPUNIT_ASSERT_EQUAL(5, aiAircraft->GetFlightPlan()->getLeg());
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
    FGScheduledFlight* flight = new FGScheduledFlight("gaParkYSSY", "", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "HBR_BN_2");
    schedule->assign(flight);

    FGAIAircraft* aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("ga", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const string flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime;
    time(&departureTime); // now
    departureTime = departureTime - 50;

    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs, departureTime,
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
            std::cout << (*it)->name() << "\t" << (*it)->getHeading()
                      << "\t" << shortestDistance << "\t" << (*it)->geod() << "\n";
        }
    }

    CPPUNIT_ASSERT_EQUAL(true, aiAircraft->getDie());
    CPPUNIT_ASSERT_EQUAL(0, shortestDistance);
    // CPPUNIT_ASSERT_EQUAL(nearestParking->getHeading(), lastHeading);
}

void TrafficTests::testPushforwardParkYBBNRepeat()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("YBBN");

    FGAirportRef arrivalAirport = FGAirport::getByIdent("YSSY");

    fgSetString("/sim/presets/airport-id", arrivalAirport->getId());

    // Time to depart
    std::string dep = getTimeString(60);
    // Time to arrive
    std::string arr = getTimeString(3260);
    // Time to arrive back
    std::string ret = getTimeString(6460);

    const int radius = 8.0;
    const int cruiseAltFt = 32000;
    const int cruiseSpeedKnots = 80;
    const char* flighttype = "ga";

    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", departureAirport->getId(), "G-BLA", "TST_BN_2", false, "B737", "KLM", "N", flighttype, radius, 8);
    FGScheduledFlight* flight = new FGScheduledFlight("gaParkYSSY", "VFR", departureAirport->getId(), arrivalAirport->getId(), 24, dep, arr, "WEEK", "TST_BN_2");
    schedule->assign(flight);

    FGScheduledFlight* returnFlight = new FGScheduledFlight("gaParkYSSY", "", arrivalAirport->getId(), departureAirport->getId(), 24, arr, ret, "WEEK", "TST_BN_2");
    schedule->assign(returnFlight);

    FGAIAircraft* aiAircraft = new FGAIAircraft{schedule};

    const SGGeod position = departureAirport->geod();
    FGTestApi::setPositionAndStabilise(position);

    aiAircraft->setPerformance("ga", "");
    aiAircraft->setCompany("KLM");
    aiAircraft->setAcType("B737");
    aiAircraft->setSpeed(0);
    aiAircraft->setBank(0);

    const string flightPlanName = departureAirport->getId() + "-" + arrivalAirport->getId() + ".xml";

    const double crs = SGGeodesy::courseDeg(departureAirport->geod(), arrivalAirport->geod()); // direct course
    time_t departureTime;
    time(&departureTime); // now
    departureTime = departureTime - 50;

    std::unique_ptr<FGAIFlightPlan> fp(new FGAIFlightPlan(aiAircraft,
                                                          flightPlanName, crs, departureTime,
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
            std::cout << (*it)->name() << "\t" << (*it)->getHeading()
                      << "\t" << shortestDistance << "\t" << (*it)->geod() << "\n";
        }
    }

    CPPUNIT_ASSERT_EQUAL(true, aiAircraft->getDie());
    CPPUNIT_ASSERT_EQUAL(0, shortestDistance);
    // CPPUNIT_ASSERT_EQUAL(nearestParking->getHeading(), lastHeading);
}

FGAIAircraft * TrafficTests::flyAI(FGAIAircraft * aiAircraft, std::string fName) {
    FGTestApi::setUp::logLinestringsToKML(fName);
    flightgear::SGGeodVec geods = flightgear::SGGeodVec();
    char buffer[50];
    int iteration = 1;
    int lastLeg = -1;
    double lastHeading = 0;
    for (size_t i = 0; i < 12000000 && !(aiAircraft->getDie()) && aiAircraft->GetFlightPlan()->getLeg() < 10; i++) {
        if (!aiAircraft->getDie()) {
            // collect position
            if (geods.empty() ||
                SGGeodesy::distanceM(aiAircraft->getGeodPos(), geods.back()) > 0.1) {
                geods.insert(geods.end(), aiAircraft->getGeodPos());
                lastHeading = aiAircraft->_getHeading();
            }
            if (geods.empty() ||
                (aiAircraft->getSpeed() > 0 &&
                 SGGeodesy::distanceM(aiAircraft->getGeodPos(), FGTestApi::getPosition()) > 50 &&
                    /* stop following towards the end*/
                    aiAircraft->GetFlightPlan()->getLeg() < 8)) {
                // std::cout << "Reposition to " << aiAircraft->getGeodPos() << "\t" << aiAircraft->isValid() << "\t" << aiAircraft->getDie() << "\n";
                FGTestApi::setPositionAndStabilise(aiAircraft->getGeodPos());
            }
        }
        // in cruise
        if (aiAircraft->GetFlightPlan()->getLeg() == 9) {
            this->dump(aiAircraft);
        }
        if (aiAircraft->GetFlightPlan()->getLeg() != lastLeg) {
            sprintf(buffer, "AI Leg %d Callsign %s Iteration %d", lastLeg, aiAircraft->getCallSign().c_str(), iteration);
            FGTestApi::writeGeodsToKML(buffer, geods);
            if (aiAircraft->GetFlightPlan()->getLeg() < lastLeg) {
                iteration++;
            }
            lastLeg = aiAircraft->GetFlightPlan()->getLeg();
            SGGeod last = geods.back();
            geods.clear();
            geods.insert(geods.end(), last);
        }
        CPPUNIT_ASSERT_LESSEQUAL(10, aiAircraft->GetFlightPlan()->getLeg());
        CPPUNIT_ASSERT_MESSAGE( "Aircraft has not completed test in time.", i < 30000);
        FGTestApi::runForTime(3);
    }
    lastLeg = aiAircraft->GetFlightPlan()->getLeg();
    sprintf(buffer, "AI Leg %d Callsign %s Iteration %d", lastLeg, aiAircraft->getCallSign().c_str(), iteration);
    FGTestApi::writeGeodsToKML(buffer, geods);
    geods.clear();
    return aiAircraft;
}

void TrafficTests::dump(FGAIAircraft* aiAircraft)
{
    std::cout << "********************\n";
    std::cout << "Geod " << aiAircraft->getGeodPos() << "\t Speed : " << aiAircraft->getSpeed() << "\n";
    std::cout << "Heading " << aiAircraft->getTrueHeadingDeg() << "\t VSpeed : " << aiAircraft->getVerticalSpeedFPM() << "\n";
    FGAIWaypoint* currentWP = aiAircraft->GetFlightPlan()->getCurrentWaypoint();
    if (currentWP) {
        std::cout << "WP       " << currentWP->getName() << "\t" << aiAircraft->GetFlightPlan()->getCurrentWaypoint()->getPos() << "\r\n";
        std::cout << "Distance " << SGGeodesy::distanceM(aiAircraft->getGeodPos(), currentWP->getPos()) << "\n";
    } else {
        std::cout << "No Current WP\n";
    }
    std::cout << "Flightplan "
              << "\n";
    FGAIFlightPlan* fp = aiAircraft->GetFlightPlan();
    if (fp->isValidPlan()) {
        std::cout << "Leg    : " << fp->getLeg() << "\n";
        std::cout << "Length : " << fp->getNrOfWayPoints() << "\n";
    }
}

std::string TrafficTests::getTimeString(int timeOffset)
{
    char ret[11];
    time_t rawtime;
    time (&rawtime);
    rawtime = rawtime + timeOffset;
    tm* timeinfo = gmtime(&rawtime);
    strftime(ret, 11, "%w/%H:%M:%S", timeinfo);
    return ret;
}