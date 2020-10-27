// Written by James Turner, started 2017.
//
// Copyright (C) 2017  James Turner
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

#include "config.h"
#include "test_posinit.hxx"

#include <thread>
#include <chrono>

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/scene_graph.hxx"

#include <simgear/props/props_io.hxx>
#include <AIModel/AIManager.hxx>

#include "Main/positioninit.hxx"
#include "Main/options.hxx"
#include "Main/globals.hxx"
#include "Main/fg_props.hxx"
#include "Navaids/navlist.hxx"

#include <AIModel/performancedb.hxx>

#include "Airports/airport.hxx"
#include "Airports/groundnetwork.hxx"
#include <Airports/airportdynamicsmanager.hxx>
#include <Airports/dynamics.hxx>

#include "ATC/atc_mgr.hxx"

using namespace flightgear;

void PosInitTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("posinit");
    FGTestApi::setUp::initNavDataCache();
    Options::reset();
    fgLoadProps("defaults.xml", globals->get_props());
    
    // ensure EDDF has a valid ground net for parking testing
    FGAirport::clearAirportsCache();
    auto apt = FGAirport::getByIdent("EDDF");


    apt->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "EDDF.groundnet.xml");
    
    globals->add_new_subsystem<flightgear::AirportDynamicsManager>();
    globals->add_new_subsystem<PerformanceDB>();
    globals->add_new_subsystem<FGATCManager>();
    globals->add_new_subsystem<FGAIManager>();
}


void PosInitTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void PosInitTests::checkAlt(float value)
{
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/position/altitude-ft", value, globals->get_props()->getDoubleValue("/position/altitude-ft"), 10.0);
}

void PosInitTests::checkHeading(float value)
{
  CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/orientation/heading-deg incorrect", value, globals->get_props()->getDoubleValue("/orientation/heading-deg"), 1.0);
}

void PosInitTests::checkPosition(SGGeod expectedPos, float delta)
{
  double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                     expectedPos);
  CPPUNIT_ASSERT_MESSAGE("Unexpected Position", dist < delta);
}

void PosInitTests::checkClosestAirport(std::string icao)
{
  std::string closest = globals->get_props()->getStringValue("/sim/airport/closest-airport-id");
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Closest Airport (/sim/airport/closest-airport-id) Incorrect", icao, closest);
}

void PosInitTests::checkStringProp(std::string property, std::string expected)
{
  std::string value = globals->get_props()->getStringValue(property);
  CPPUNIT_ASSERT_EQUAL_MESSAGE(property, expected, value);
}

void PosInitTests::checkRunway(std::string expected)
{
  checkStringProp("/sim/presets/runway", expected);
}

void PosInitTests::checkOnGround()
{
  CPPUNIT_ASSERT_MESSAGE("/sim/presets/onground!=true for ground start", fgGetBool("/sim/presets/onground"));
}

void PosInitTests::checkInAir()
{
  CPPUNIT_ASSERT_MESSAGE("/sim/presets/onground != false for in air start", fgGetBool("/sim/presets/onground") == false);
  CPPUNIT_ASSERT_MESSAGE("/sim/presets/trim != true for in air start", fgGetBool("/sim/presets/trim") == true);
}

void PosInitTests::testDefaultStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath"};
        opts->init(1, (char**) args, SGPath());
        opts->processOptions();
    }

    initPosition();

    // verify we got the location specified in location-preset.xml
    // this unfortunately means manually parsing that file, oh well

    {
        SGPath presets = globals->get_fg_root() / "location-presets.xml";
        CPPUNIT_ASSERT(presets.exists());
        SGPropertyNode_ptr props(new SGPropertyNode);
        readProperties(presets, props);

        std::string icao = props->getStringValue("/sim/presets/airport-id");
        FGAirportRef defaultAirport = FGAirport::getByIdent(icao);
        checkClosestAirport(icao);
        checkPosition(defaultAirport->geod(), 10000.0);
    }
}

void PosInitTests::testAirportOnlyStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF"};
        opts->init(2, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    initPosition();

    checkClosestAirport(std::string("EDDF"));
    checkPosition(FGAirport::getByIdent("EDDF")->geod(), 10000.0);
}

void PosInitTests::testAirportAltitudeOffsetStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--altitude=1000", "--offset-azimuth=90", "--offset-distance=5", "--heading=45"};
        opts->init(6, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    initPosition();

    SGGeod expectedPos = SGGeodesy::direct(FGAirport::getByIdent("EDDF")->geod(), 270, 5 * SG_NM_TO_METER);
    checkPosition(expectedPos, 5000.0);
    checkAlt(1000.0);
    checkHeading(45.0);
}

void PosInitTests::testAirportAndRunwayStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--runway=25C"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/runway-requested"));
    checkRunway(std::string("25C"));
    initPosition();

    checkClosestAirport(std::string("EDDF"));
    checkPosition(FGAirport::getByIdent("EDDF")->geod(), 10000.0);
    checkHeading(250.0);
}

void PosInitTests::simulateFinalizePosition()
{
    auto subSyMgr = globals->get_subsystem_mgr();
    subSyMgr->bind();
    subSyMgr->init();
    subSyMgr->postinit();
    
    finalizePosition();
}

void PosInitTests::testAirportAndParkingStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--parkpos=V266"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }
    
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));

    checkStringProp("/sim/presets/parkpos", "V266");
    initPosition();

    simulateFinalizePosition();
    
    auto apt = FGAirport::getByIdent("EDDF");
    auto parking = apt->groundNetwork()->findParkingByName("V266");
    CPPUNIT_ASSERT(parking != nullptr);
    
    checkClosestAirport(std::string("EDDF"));
    checkPosition(parking->geod(), 20);
}

void PosInitTests::testAirportAndAvailableParkingStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--parkpos=AVAILABLE"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }
    
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/parkpos") == std::string("AVAILABLE"));
    initPosition();

    simulateFinalizePosition();
    
    auto assignedParking = string{globals->get_props()->getStringValue("/sim/presets/parkpos")};
    CPPUNIT_ASSERT(assignedParking != "AVAILABLE");
    
    auto dynamics =  FGAirport::getByIdent("EDDF");
    auto parking = dynamics->groundNetwork()->findParkingByName(assignedParking);
    
    checkClosestAirport(std::string("EDDF"));
    // Anywhere around EDDF will do!
    checkPosition(FGAirport::getByIdent("EDDF")->geod(), 10000.0);
    checkPosition(parking->geod(), 20.0);
}

void PosInitTests::testAirportAndMetarStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);
        const char* args[] = {"dummypath", "--airport=LOWI", "--metar=XXXX 271320Z 08007KT 030V130 CAVOK 17/02 Q1020 NOSIG"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }

    initPosition();

    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/airport/closest-airport-id") == std::string("LOWI"));
    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       FGAirport::getByIdent("LOWI")->geod());
    CPPUNIT_ASSERT(dist < 10000);
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("sim/atc/runway") == std::string("26"));
}

void PosInitTests::testAirportRunwayOffsetGlideslopeStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--runway=25C", "--offset-distance=5", "--glideslope=3.5"};
        opts->init(5, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/runway-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/runway") == std::string("25C"));
    CPPUNIT_ASSERT(globals->get_props()->getIntValue("/sim/presets/offset-distance-nm") == 5);
    initPosition();

    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/airport/closest-airport-id") == std::string("EDDF"));

    double crs = FGAirport::getByIdent("EDDF")->getRunwayByIdent("25C")->headingDeg() -180.0;

    SGGeod expectedPos = SGGeodesy::direct(FGAirport::getByIdent("EDDF")->getRunwayByIdent("25C")->geod(), crs, 5 * SG_NM_TO_METER);

    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       expectedPos);
    CPPUNIT_ASSERT(dist < 500);

    // Check altitude based on glideslope.  5nm = 30380ft. so altitude should be 2213ft.
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/position/altitude-ft", globals->get_props()->getDoubleValue("/position/altitude-ft"), 2213.0, 50.0);
}

void PosInitTests::testAirportRunwayOffsetAltitudeStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--runway=25C", "--offset-distance=5", "--altitude=3000"};
        opts->init(5, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/runway-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/runway") == std::string("25C"));
    CPPUNIT_ASSERT(globals->get_props()->getIntValue("/sim/presets/offset-distance-nm") == 5);
    initPosition();

    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/airport/closest-airport-id") == std::string("EDDF"));

    double crs = FGAirport::getByIdent("EDDF")->getRunwayByIdent("25C")->headingDeg() -180.0;

    SGGeod expectedPos = SGGeodesy::direct(FGAirport::getByIdent("EDDF")->getRunwayByIdent("25C")->geod(), crs, 5 * SG_NM_TO_METER);

    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       expectedPos);
    CPPUNIT_ASSERT(dist < 500);

    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/position/altitude-ft", globals->get_props()->getDoubleValue("/position/altitude-ft"), 3000.0, 10.0);
}

void PosInitTests::testVOROnlyStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--vor=JFK", "--vor-frequency=115.9"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/vor-id") == std::string("JFK"));
    initPosition();

    FGNavList::TypeFilter filter(FGPositioned::Type::VOR);
    const nav_list_type navlist = FGNavList::findByIdentAndFreq( "JFK", 0.0, &filter );

    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       navlist[0]->geod());

    CPPUNIT_ASSERT(dist < 500);
}

void PosInitTests::testVOROffsetAltitudeHeadingStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--vor=JFK", "--offset-distance=5", "--vor-frequency=115.9", "--offset-azimuth=78", "--altitude=1000", "--heading=45", "--vc=250"};
        opts->init(7, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getIntValue("/sim/presets/offset-distance-nm") == 5);
    initPosition();


    FGNavList::TypeFilter filter(FGPositioned::Type::VOR);
    const nav_list_type navlist = FGNavList::findByIdentAndFreq( "JFK", 115.9, &filter );

    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       SGGeodesy::direct(navlist[0]->geod(), 180 + 78, 5 * SG_NM_TO_METER));

    CPPUNIT_ASSERT(dist < 1000);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/position/altitude-ft", globals->get_props()->getDoubleValue("/position/altitude-ft"), 1000.0, 10.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/orientation/heading-deg incorrect", globals->get_props()->getDoubleValue("/orientation/heading-deg"), 45.0, 1.0);
}

void PosInitTests::testFixOnlyStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--fix=FOLER"};
        opts->init(2, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/fix") == std::string("FOLER"));
    initPosition();

    FGNavList::TypeFilter filter(FGPositioned::Type::FIX);
    const nav_list_type navlist = FGNavList::findByIdentAndFreq( "FOLER", 0.0, &filter );

    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       navlist[0]->geod());

    CPPUNIT_ASSERT(dist < 500);
}

void PosInitTests::testFixOffsetAltitudeHeadingStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--fix=FOLER", "--offset-distance=5", "--offset-azimuth=78", "--altitude=1000", "--heading=45", "--vc=250"};
        opts->init(6, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/fix") == std::string("FOLER"));
    CPPUNIT_ASSERT(globals->get_props()->getIntValue("/sim/presets/offset-distance-nm") == 5);
    initPosition();


    FGNavList::TypeFilter filter(FGPositioned::Type::FIX);
    const nav_list_type navlist = FGNavList::findByIdentAndFreq( "FOLER", 0.0, &filter );

    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       SGGeodesy::direct(navlist[0]->geod(), 180 + 78, 5 * SG_NM_TO_METER));

    CPPUNIT_ASSERT(dist < 1000);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/position/altitude-ft", globals->get_props()->getDoubleValue("/position/altitude-ft"), 1000.0, 10.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/orientation/heading-deg incorrect", globals->get_props()->getDoubleValue("/orientation/heading-deg"), 45.0, 1.0);
}

void PosInitTests::testNDBOnlyStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--ndb=HHI", "--ndb-frequency=373"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/ndb-id") == std::string("HHI"));
    initPosition();

    FGNavList::TypeFilter filter(FGPositioned::Type::NDB);
    const nav_list_type navlist = FGNavList::findByIdentAndFreq( "HHI", 373, &filter );

    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       navlist[0]->geod());

    CPPUNIT_ASSERT(dist < 500);
}

void PosInitTests::testNDBOffsetAltitudeHeadingStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--ndb=HHI", "--ndb-frequency=373", "--offset-distance=5", "--offset-azimuth=78", "--altitude=1000", "--heading=45", "--vc=250"};
        opts->init(8, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/ndb-id") == std::string("HHI"));
    //CPPUNIT_ASSERT(globals->get_props()->getIntValue("/sim/presets/offset-distance-nm") == 5);
    initPosition();

    FGNavList::TypeFilter filter(FGPositioned::Type::NDB);
    const nav_list_type navlist = FGNavList::findByIdentAndFreq( "HHI", 373, &filter );

    double dist = SGGeodesy::distanceM(globals->get_aircraft_position(),
                                       SGGeodesy::direct(navlist[0]->geod(), 180 + 78, 5 * SG_NM_TO_METER));

    std::cerr << "Navlist size : " << navlist.size() << " dist " << dist << "\n";
    CPPUNIT_ASSERT(dist < 1000);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/position/altitude-ft", 1000.0, globals->get_props()->getDoubleValue("/position/altitude-ft"), 10.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("/orientation/heading-deg incorrect", 45.0, globals->get_props()->getDoubleValue("/orientation/heading-deg"), 1.0);
}

void PosInitTests::testLatLonStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--lat=55.9", "--lon=-3.355", "--altitude=1000"};
        opts->init(4, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));

    initPosition();

    SGGeod pos = SGGeod::fromDeg(-3.355, 55.9);
    checkPosition(pos, 100.0);
    checkAlt(1000.0);
}

void PosInitTests::testLatLonOffsetStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--lat=55.9", "--lon=-3.355", "--altitude=1000", "--offset-distance=5", "--offset-azimuth=90", "--heading=45"};
        opts->init(7, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));

    initPosition();

    SGGeod pos = SGGeodesy::direct(SGGeod::fromDeg(-3.355, 55.9), 270, 5 * SG_NM_TO_METER);
    checkPosition(pos, 1000.0);
    checkAlt(1000.0);
    checkHeading(45.0);
}

void PosInitTests::testCarrierStartup()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--carrier=Nimitz"};
        opts->init(2, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/airport-requested"));
    initPosition();

    checkPosition(SGGeod::fromDeg(-122.6, 37.8), 100.0);
    checkOnGround();
}

void PosInitTests::testAirportRepositionAirport()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--runway=25C"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }

    std::cout << "Preset heading " << fgGetDouble("/sim/presets/heading-deg") << "\n";

    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/runway-requested"));
    checkRunway(std::string("25C"));
    initPosition();

    checkClosestAirport(std::string("EDDF"));
    checkPosition(FGAirport::getByIdent("EDDF")->geod(), 10000.0);
    checkHeading(250.0);

    std::cout << "Preset heading " << fgGetDouble("/sim/presets/heading-deg") << "\n";

    // Now re-position to KSFO runway
    // Reset the Lat/Lon as these will be used in preference to the airport ID
    fgSetDouble("/sim/presets/longitude-deg", -9990.00);
    fgSetDouble("/sim/presets/latitude-deg",  -9990.00);
    fgSetString("/sim/presets/airport-id", "KHAF");
    fgSetDouble("/sim/presets/heading-deg",  9990.00);
    fgSetString("/sim/presets/runway", "12");
    initPosition();

    checkClosestAirport(std::string("KHAF"));
    checkPosition(FGAirport::getByIdent("KHAF")->geod(), 5000.0);
    checkHeading(137.0); // Lots of magnetic variation in SF Bay area!
    checkOnGround();

   
}

// this simulates what the C172 preflight tutorial does,
// where the user is likely at a random airport (on a runway), and starts
// the preflight tutorial, which repositions them to a ramp location
// at PHTO
void PosInitTests::testAirportRunwayRepositionAirport()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);
        const char* args[] = {"dummypath", "--airport=EDDS", "--runway=07"};
        opts->init(3, (char**)args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/runway-requested"));
    checkRunway(std::string("07"));
    initPosition();
    simulateFinalizePosition();

    checkClosestAirport(std::string("EDDS"));
    checkPosition(FGAirport::getByIdent("EDDS")->geod(), 10000.0);

    // Now re-position to PHTO runway
    // Reset the Lat/Lon as these will be used in preference to the airport ID
    fgSetDouble("/sim/presets/longitude-deg", -155.0597483);
    fgSetDouble("/sim/presets/latitude-deg", 19.71731272);
    fgSetString("/sim/presets/airport-id", "PHTO");
    fgSetDouble("/sim/presets/heading-deg", 125);
    fgSetBool("/sim/presets/on-ground", true);
    fgSetBool("/sim/presets/runway-requested", true);
    fgSetBool("/sim/presets/airport-requested", true);
    fgSetBool("/sim/presets/parking-requested", true);

    simulateStartReposition();
    finalizePosition();

    FGTestApi::runForTime(1.0);

    checkClosestAirport(std::string("PHTO"));
    checkPosition(FGAirport::getByIdent("PHTO")->geod(), 5000.0);
    checkHeading(125);
    checkOnGround();

    FGTestApi::runForTime(1.0);
}

void PosInitTests::testParkInvalid()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);
        const char* args[] = {"dummypath", "--airport=EDDF", "--parkpos=foobar"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }
    
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));

    checkStringProp("/sim/presets/parkpos", "foobar");
    initPosition();
    
    auto apt = FGAirport::getByIdent("EDDF");
    

    fgSetDouble("/environment/metar/base-wind-dir-deg",  350.0);
    fgSetBool("/environment/metar/valid", true);
    
    simulateFinalizePosition();
    checkClosestAirport(std::string("EDDF"));
    // we should be on the best runway, let's see
    auto runway = apt->getRunwayByIdent("36");
    checkPosition(runway->threshold());
}

void PosInitTests::testParkAtOccupied()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--parkpos=V266"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }
    
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));

    checkStringProp("/sim/presets/parkpos", "V266");
    initPosition();
    
    auto apt = FGAirport::getByIdent("EDDF");
    auto parking = apt->groundNetwork()->findParkingByName("V266");
    
    // now mark the parking as occupied
    auto dynamics = apt->getDynamics();
    dynamics->setParkingAvailable(parking, false);
    fgSetDouble("/environment/metar/base-wind-dir-deg",  350.0);
    fgSetBool("/environment/metar/valid", true);
    
    simulateFinalizePosition();
    
    checkClosestAirport(std::string("EDDF"));
    

    // we should be on the best runway, let's see
    
    auto runway = apt->getRunwayByIdent("36");
    checkPosition(runway->threshold());
}

void PosInitTests::simulateStartReposition()
{
    initPosition();
    
    auto atcManager = globals->get_subsystem<FGATCManager>();
    if (atcManager) {
        atcManager->reposition();
    }
}

void PosInitTests::testRepositionAtParking()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=KSFO", "--runway=28L"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }
    
    initPosition();
    simulateFinalizePosition();
    
    fgSetDouble("/sim/presets/longitude-deg", -9990.00);
    fgSetDouble("/sim/presets/latitude-deg",  -9990.00);
    fgSetString("/sim/presets/airport-id", "EDDF");
    fgSetDouble("/sim/presets/heading-deg",  9990.00);
    fgSetString("/sim/presets/parkpos", "V266");
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/parkpos") == std::string("V266"));
    
    simulateStartReposition();
    finalizePosition();
    
   auto apt = FGAirport::getByIdent("EDDF");
   auto parking = apt->groundNetwork()->findParkingByName("V266");

   checkClosestAirport(std::string("EDDF"));
   checkPosition(parking->geod(), 10.0);
   checkOnGround();

// now checking switching back to 'AVAILABLE'
    
    
    fgSetDouble("/sim/presets/longitude-deg", -9990.00);
    fgSetDouble("/sim/presets/latitude-deg",  -9990.00);
    fgSetString("/sim/presets/airport-id", "EDDF");
    fgSetDouble("/sim/presets/heading-deg",  9990.00);
    fgSetString("/sim/presets/parkpos", "AVAILABLE");
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(globals->get_props()->getStringValue("/sim/presets/parkpos") == std::string("AVAILABLE"));
    
    simulateStartReposition();
    finalizePosition();
    
    auto assignedParking = string{globals->get_props()->getStringValue("/sim/presets/parkpos")};
    CPPUNIT_ASSERT(assignedParking != "AVAILABLE");

    parking = apt->groundNetwork()->findParkingByName(assignedParking);

    checkClosestAirport(std::string("EDDF"));
    checkPosition(parking->geod(), 20.0);
    
}

void PosInitTests::testRepositionAtSameParking()
{
    {
          Options* opts = Options::sharedInstance();
          opts->setShouldLoadDefaultConfig(false);

          const char* args[] = {"dummypath", "--airport=EDDF", "--parkpos=V266"};
          opts->init(3, (char**) args, SGPath());
          opts->processOptions();
      }
      
      CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
      CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));

      checkStringProp("/sim/presets/parkpos", "V266");
      initPosition();

      simulateFinalizePosition();
      
      auto apt = FGAirport::getByIdent("EDDF");
      auto parking = apt->groundNetwork()->findParkingByName("V266");
      CPPUNIT_ASSERT(parking != nullptr);
      
      checkClosestAirport(std::string("EDDF"));
      checkPosition(parking->geod(), 20);
/////////
    fgSetDouble("/sim/presets/longitude-deg", -9990.00);
    fgSetDouble("/sim/presets/latitude-deg",  -9990.00);
    fgSetString("/sim/presets/airport-id", "EDDF");
    fgSetDouble("/sim/presets/heading-deg",  9990.00);
    fgSetString("/sim/presets/parkpos", "V266");
    
    
    simulateStartReposition();
    finalizePosition();
    
    checkPosition(parking->geod(), 20);
    
}


void PosInitTests::testRepositionAtOccupied()
{
    {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--parkpos=F235"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }
    
    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));

    checkStringProp("/sim/presets/parkpos", "F235");
    initPosition();
    
    auto apt = FGAirport::getByIdent("EDDF");
    auto parking1 = apt->groundNetwork()->findParkingByName("F235");
         
  
    fgSetDouble("/environment/metar/base-wind-dir-deg",  350.0);
    fgSetBool("/environment/metar/valid", true);
    
    simulateFinalizePosition();
    
    checkClosestAirport(std::string("EDDF"));
    checkPosition(parking1->geod(), 20);

//////////
    fgSetDouble("/sim/presets/longitude-deg", -9990.00);
    fgSetDouble("/sim/presets/latitude-deg",  -9990.00);
    fgSetString("/sim/presets/airport-id", "EDDF");
    fgSetDouble("/sim/presets/heading-deg",  9990.00);
    fgSetString("/sim/presets/parkpos", "V266");
    
    auto parking2 = apt->groundNetwork()->findParkingByName("V266");
    // now mark the parking as occupied
    auto dynamics = apt->getDynamics();
    dynamics->setParkingAvailable(parking2, false);
    
    simulateStartReposition();
    finalizePosition();
        
    // we should be on the best runway, let's see
    auto runway = apt->getRunwayByIdent("36");
    checkPosition(runway->threshold());
}


void PosInitTests::testRepositionAtInvalid()
{
     {
        Options* opts = Options::sharedInstance();
        opts->setShouldLoadDefaultConfig(false);

        const char* args[] = {"dummypath", "--airport=EDDF", "--parkpos=F235"};
        opts->init(3, (char**) args, SGPath());
        opts->processOptions();
    }

    CPPUNIT_ASSERT(fgGetBool("/sim/presets/airport-requested"));
    CPPUNIT_ASSERT(! fgGetBool("/sim/presets/runway-requested"));

    checkStringProp("/sim/presets/parkpos", "F235");
    initPosition();

    auto apt = FGAirport::getByIdent("EDDF");
    auto parking1 = apt->groundNetwork()->findParkingByName("F235");
         

    fgSetDouble("/environment/metar/base-wind-dir-deg",  350.0);
    fgSetBool("/environment/metar/valid", true);

    simulateFinalizePosition();

    checkClosestAirport(std::string("EDDF"));
    checkPosition(parking1->geod(), 20);

    //////////
    fgSetDouble("/sim/presets/longitude-deg", -9990.00);
    fgSetDouble("/sim/presets/latitude-deg",  -9990.00);
    fgSetString("/sim/presets/airport-id", "EDDF");
    fgSetDouble("/sim/presets/heading-deg",  9990.00);
    fgSetString("/sim/presets/parkpos", "foobarzot");

    simulateStartReposition();
    finalizePosition();
        
    // we should be on the best runway, let's see
    auto runway = apt->getRunwayByIdent("36");
    checkPosition(runway->threshold());
}
