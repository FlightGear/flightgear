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

#include <simgear/math/sg_geodesy.hxx>
#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>
#include <AIModel/AIManager.hxx>
#include <AIModel/performancedb.hxx>
#include <Airports/airport.hxx>
#include <Airports/airportdynamicsmanager.hxx>
#include <Traffic/TrafficMgr.hxx>
#include <Time/TimeManager.hxx>

#include <ATC/atc_mgr.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void TrafficTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("timeManager");
    FGTestApi::setUp::initTestGlobals("Traffic");
    FGTestApi::setUp::initNavDataCache();


    auto props = globals->get_props();
    props->setBoolValue("sim/ai/enabled", true);
    props->setBoolValue("/sim/traffic-manager/enabled", true);
    props->setBoolValue("sim/signals/fdm-initialized", false);
    props->setDoubleValue("/environment/visibility-m", 100.0);


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
    FGAirportRef egph = FGAirport::getByIdent("EGPH");
    
    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");
    fgSetInt("/environment/visibility-m", 1000);

    time_t rawtime;
    struct tm * timeinfo;
    char dep [11];
    char arr [11];

    time (&rawtime);
    rawtime = rawtime + 50;
    timeinfo = gmtime (&rawtime);
    strftime (dep,11,"%w/%H:%M:%S",timeinfo);

    rawtime = rawtime + 320;
    timeinfo = gmtime (&rawtime);
    strftime (arr,11,"%w/%H:%M:%S",timeinfo);


    FGAISchedule* schedule = new FGAISchedule(
        "B737", "KLM", "EGPH", "G-BLA", "ID", false, "B737", "KLM", "N", "gate", 24, 8);
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

    const int radius = 18.0;
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
                                                          cruiseSpeedKnots, "gate",
                                                          aiAircraft->getAcType(),
                                                          aiAircraft->getCompany())); 
                                                          
    if (fp->isValidPlan()) {
        aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
        globals->get_subsystem<FGAIManager>()->attach(aiAircraft);
    }
    flightgear::SGGeodVec geods = flightgear::SGGeodVec();
    for (size_t i = 0; i < 6000 && aiAircraft->GetFlightPlan()->getLeg() < 8; i++)
    {
        //this->dump(aiAircraft);
        
        if(geods.empty()||SGGeodesy::distanceM(aiAircraft->getGeodPos(), geods.back()) > 1) {
          geods.insert(geods.end(), aiAircraft->getGeodPos());
        }

        FGTestApi::runForTime(3.0);    
    }
    FGTestApi::setUp::logPositionToKML("flight_EDPH_" + std::to_string(departureTime));    
    FGTestApi::writeGeodsToKML("Aircraft", geods);
    CPPUNIT_ASSERT_EQUAL( 5, aiAircraft->GetFlightPlan()->getLeg());
}

void TrafficTests::testPushback2()
{
    FGAirportRef egph = FGAirport::getByIdent("EGPH");
    
    FGAirportRef egpf = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", "EGPH");
    fgSetInt("/environment/visibility-m", 1000);

    time_t rawtime;
    struct tm * timeinfo;
    char dep [11];
    char arr [11];

    time (&rawtime);
    rawtime = rawtime + 50;
    timeinfo = gmtime (&rawtime);
    strftime (dep,11,"%w/%H:%M:%S",timeinfo);

    rawtime = rawtime + 320;
    timeinfo = gmtime (&rawtime);
    strftime (arr,11,"%w/%H:%M:%S",timeinfo);


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
                                                          
    if (fp->isValidPlan()) {
        aiAircraft->FGAIBase::setFlightPlan(std::move(fp));
        globals->get_subsystem<FGAIManager>()->attach(aiAircraft);
    }
    flightgear::SGGeodVec geods = flightgear::SGGeodVec();
    for (size_t i = 0; i < 6000 && aiAircraft->GetFlightPlan()->getLeg() < 10; i++)
    {
        // this->dump(aiAircraft);
        
        if(geods.empty()||SGGeodesy::distanceM(aiAircraft->getGeodPos(), geods.back()) > 1) {
          geods.insert(geods.end(), aiAircraft->getGeodPos());
        }

        FGTestApi::runForTime(3.0);    
    }
    FGTestApi::setUp::logPositionToKML("flight_cargo_EDPH_" + std::to_string(departureTime));    
    FGTestApi::writeGeodsToKML("Aircraft", geods);
    CPPUNIT_ASSERT_EQUAL( 5, aiAircraft->GetFlightPlan()->getLeg());
}

void TrafficTests::testTrafficManager()
{
    FGAirportRef egeo = FGAirport::getByIdent("EGEO");

    fgSetString("/sim/presets/airport-id", "EGEO");

    std::cout << globals->get_fg_root() << "\r\n";
    globals->set_fg_root(SGPath::fromUtf8(FG_TEST_SUITE_DATA));
    std::cout << globals->get_fg_root() << "\r\n";

    fgSetBool("/sim/traffic-manager/enabled", true);
    fgSetBool("/sim/traffic-manager/active", false);
    fgSetBool("/sim/ai/enabled", true);
    fgSetBool("/environment/realwx/enabled", false);
    fgSetBool("/environment/metar/valid", false);
    fgSetBool("/sim/terrasync/ai-data-update-now", false);
    
    fgSetBool("/sim/traffic-manager/instantaneous-action", true);
    fgSetBool("/sim/traffic-manager/heuristics", true);
    fgSetBool("/sim/traffic-manager/dumpdata", false);

    fgSetBool("/sim/signals/fdm-initialized", true);

    FGTestApi::setPositionAndStabilise(egeo->geod());

    auto tmgr = globals->add_new_subsystem<FGTrafficManager>();

    tmgr->bind();
    tmgr->init();

    for( int i = 0; i < 30; i++) {
        bool active = fgGetBool("/sim/traffic-manager/inited");
        // std::cout << "Inited " << "\t" << i << "\t" << active << "\r\n";
        FGTestApi::runForTime(5.0);
        if(active) {
            break;
        }
//        SGTimeStamp::sleepForMSec(300);
    }

    const SGPropertyNode *tm = fgGetNode("/sim/traffic-manager", true);

    for (int i = 0; i < tm->nChildren(); i++) {
        const SGPropertyNode *model = tm->getChild(i);
        std::cout << "TM : " << model->getDisplayName() << "\t" << model->nChildren() << "\n";
        for (int g = 0; g < model->nChildren(); g++) {
            const SGPropertyNode *v;
            v = model->getChild(g);
            std::cout << "Node " << g << "\t" << v->getDisplayName() << "\n";
        }
    }

    FGTestApi::runForTime(240.0);

    FGScheduledFlightVecIterator fltBegin, fltEnd;
    fltBegin = tmgr->getFirstFlight("HBR_BN_2");
    fltEnd = tmgr->getLastFlight("HBR_BN_2");

    if (fltBegin == fltEnd) {
        CPPUNIT_FAIL("No Traffic found");
    }

    int counter = 0;
    for (FGScheduledFlightVecIterator i = fltBegin; i != fltEnd; i++) {
        cout << (*i)->getDepartureAirport()->getId() << "\t" << (*i)->getArrivalAirport()->getId() << "\t" << (*i)->getDepartureTime() << "\n";
        counter++;
        //sort(fltBegin, fltEnd, compareScheduledFlights);
        //cerr << counter++ << endl;
    }
    const SGPropertyNode *ai = fgGetNode("/ai/models", false);

    std::cout << "Num Children " << ai->nChildren() << "\n";

    for (int i = 0; i < ai->nChildren(); i++) {
        const SGPropertyNode *model;
        model = ai->getChild(i);
        std::cout << "Model : " << model->getDisplayName() << "\t" << model->nChildren() << "\n";
        for (int g = 0; g < model->nChildren(); g++) {
            const SGPropertyNode *v;
            v = model->getChild(g);
            std::cout << "Node " << g << "\t" << v->getDisplayName() << "\n";
        }
    }
    const SGPropertyNode *modelCount = fgGetNode("/ai/models/count", false);
    for (size_t i = 0; i < 20; i++)
    {
        FGTestApi::runForTime(1000.0);
        // std::cout << "AI Count " << modelCount->getIntValue() << "\r\n";
        if(modelCount->getIntValue()>0)
          break;
    }
    CPPUNIT_ASSERT_EQUAL(25, counter);
}

void TrafficTests::testChangeRunway()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("EGPH");
    
    FGAirportRef arrivalAirport = FGAirport::getByIdent("EGPF");
    fgSetString("/sim/presets/airport-id", departureAirport->getId());
    fgSetInt("/environment/visibility-m", 1000);
    fgSetInt("/environment/metar/base-wind-speed-kt", 10);
    fgSetInt("/environment/metar/base-wind-dir-deg", 160);


    time_t rawtime;
    struct tm * timeinfo;
    char dep [11];
    char arr [11];

    time (&rawtime);
    rawtime = rawtime + 50;
    timeinfo = gmtime (&rawtime);
    strftime (dep,11,"%w/%H:%M:%S",timeinfo);

    rawtime = rawtime + 320;
    timeinfo = gmtime (&rawtime);
    strftime (arr,11,"%w/%H:%M:%S",timeinfo);

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

    CPPUNIT_ASSERT_EQUAL( fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));  

    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    auto prefs =     departureAirport->getDynamics();
    flightgear::SGGeodVec geods = flightgear::SGGeodVec();
    for (size_t i = 0; i < 6000; i++)
    {
        if(geods.empty()||SGGeodesy::distanceM(aiAircraft->getGeodPos(), geods.back()) > 1) {
          geods.insert(geods.end(), aiAircraft->getGeodPos());
        }

        FGTestApi::runForTime(3.0);    
    }
    FGTestApi::setUp::logPositionToKML("flight_runway_EGPH_" + std::to_string(departureTime));    
    FGTestApi::writeGeodsToKML("Aircraft", geods);
    CPPUNIT_ASSERT_EQUAL( 5, aiAircraft->GetFlightPlan()->getLeg());
}


void TrafficTests::testPushforward()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("YSSY");
    
    FGAirportRef arrivalAirport = FGAirport::getByIdent("YBBN");
    fgSetString("/sim/presets/airport-id", departureAirport->getId());
    fgSetInt("/environment/visibility-m", 1000);

    time_t rawtime;
    struct tm * timeinfo;
    char dep [11];
    char arr [11];

    time (&rawtime);
    rawtime = rawtime + 50;
    timeinfo = gmtime (&rawtime);
    strftime (dep,11,"%w/%H:%M:%S",timeinfo);

    rawtime = rawtime + 320;
    timeinfo = gmtime (&rawtime);
    strftime (arr,11,"%w/%H:%M:%S",timeinfo);

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

    CPPUNIT_ASSERT_EQUAL( fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));    
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    flightgear::SGGeodVec geods = flightgear::SGGeodVec();
    for (size_t i = 0; i < 6000 && aiAircraft->GetFlightPlan()->getLeg() < 3; i++)
    {
        //this->dump(aiAircraft);
        
        if(geods.empty()||SGGeodesy::distanceM(aiAircraft->getGeodPos(), geods.back()) > 1) {
          geods.insert(geods.end(), aiAircraft->getGeodPos());
        }

        FGTestApi::runForTime(3.0);    
    }
    FGTestApi::setUp::logPositionToKML("flight_ga_YSSY_depart_" + std::to_string(departureTime));    
    FGTestApi::writeGeodsToKML("Aircraft", geods);
    CPPUNIT_ASSERT_EQUAL( 5, aiAircraft->GetFlightPlan()->getLeg());
}

void TrafficTests::testPushforwardParkYBBN()
{
    FGAirportRef departureAirport = FGAirport::getByIdent("YBBN");
    
    FGAirportRef arrivalAirport = FGAirport::getByIdent("YSSY");
    fgSetString("/sim/presets/airport-id", arrivalAirport->getId());
    fgSetInt("/environment/visibility-m", 1000);

    time_t rawtime;
    struct tm * timeinfo;
    char dep [11];
    char arr [11];

    time (&rawtime);
    rawtime = rawtime + 60;
    timeinfo = gmtime (&rawtime);
    strftime (dep,11,"%w/%H:%M:%S",timeinfo);

    rawtime = rawtime + 3200;
    timeinfo = gmtime (&rawtime);
    strftime (arr,11,"%w/%H:%M:%S",timeinfo);

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

    CPPUNIT_ASSERT_EQUAL( fp->isValidPlan(), true);
    aiAircraft->FGAIBase::setFlightPlan(std::move(fp));    
    globals->get_subsystem<FGAIManager>()->attach(aiAircraft);

    FGTestApi::setUp::logPositionToKML("flight_ga_YSSY_park_" + std::to_string(departureTime));    
    flightgear::SGGeodVec geods = flightgear::SGGeodVec();
    int lastLeg = 0;
    for (size_t i = 0; i < 12000000 && aiAircraft->GetFlightPlan()->getLeg() < 10; i++)
    {
        //this->dump(aiAircraft);
        
        if(geods.empty()||SGGeodesy::distanceM(aiAircraft->getGeodPos(), geods.back()) > 1) {
          geods.insert(geods.end(), aiAircraft->getGeodPos());
        }
        if (aiAircraft->GetFlightPlan()->getLeg() > lastLeg)
        {
            lastLeg = aiAircraft->GetFlightPlan()->getLeg();
            char buffer [50];
            sprintf(buffer, "AI Leg %d", lastLeg);
            FGTestApi::writeGeodsToKML(buffer, geods);
            geods.clear();
        }

        FGTestApi::runForTime(5.0);    
    }
    lastLeg = aiAircraft->GetFlightPlan()->getLeg();
    char buffer [50];
    sprintf(buffer, "AI Leg %d", lastLeg);
    FGTestApi::writeGeodsToKML(buffer, geods);
    geods.clear();
    CPPUNIT_ASSERT_EQUAL( 5, aiAircraft->GetFlightPlan()->getLeg());
}

void TrafficTests::dump(FGAIAircraft* aiAircraft) {
    std::cout << "********************\n";
    std::cout << "Geod " << aiAircraft->getGeodPos() << "\t Speed : " << aiAircraft->getSpeed() << "\n";
    std::cout << "WP   " << aiAircraft->GetFlightPlan()->getCurrentWaypoint()->getName() << "\t" << aiAircraft->GetFlightPlan()->getCurrentWaypoint()->getPos() << "\r\n";
    std::cout << "Heading " << aiAircraft->getTrueHeadingDeg() << "\t VSpeed : " << aiAircraft->getVerticalSpeedFPM() << "\n";
}
