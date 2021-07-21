// Written by James Turner, started 2021.
//
// Copyright (C) 2021  James Turner
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

#include "test_timeManager.hxx"

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/timing/sg_time.hxx>

#include "Main/fg_props.hxx"
#include "Main/globals.hxx"
#include <Airports/airport.hxx>
#include <Time/TimeManager.hxx>

using namespace flightgear;


// Set up function for each test.
void TimeManagerTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("timeManager");
    FGTestApi::setUp::initNavDataCache();
}


// Clean up after each test.
void TimeManagerTests::tearDown()
{
   FGTestApi::tearDown::shutdownTestGlobals();
}


void TimeManagerTests::testBasic()
{
    auto timeManager = globals->get_subsystem<TimeManager>();

    // set standard values
    fgSetBool("/sim/freeze", false);
    fgSetBool("/sim/sceneryloaded", true);
    fgSetDouble("/sim/model-hz", 120.0);

    timeManager->bind();
    timeManager->init();
    timeManager->postinit();

    double simDt, realDt;
    // first run: values are zero
    timeManager->computeTimeDeltas(simDt, realDt);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, simDt, 1.0e-6);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, realDt, 1.0e-6);

    // manually modify the 'last time' to check delta computation
    timeManager->_lastStamp = SGTimeStamp::now() - SGTimeStamp::fromMSec(25);
    timeManager->computeTimeDeltas(simDt, realDt);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.025, simDt, 1.0e-3);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.025, realDt, 1.0e-3);

    timeManager->update(simDt);
}

void TimeManagerTests::testFreezeUnfreeze()
{
    auto timeManager = globals->get_subsystem<TimeManager>();

    // set standard values
    fgSetBool("/sim/freeze/clock", false);
    fgSetBool("/sim/sceneryloaded", true);
    fgSetDouble("/sim/model-hz", 120.0);

    timeManager->postinit();

    double simDt, realDt;
    // first run: values are zero
    timeManager->computeTimeDeltas(simDt, realDt);

    // test hack: force dtRemainder to zero so we aren't affected by the
    // system -> stread clock offset in our test assertions. Without this,
    // depending on exaclty when thetest suite is run, we would get different
    // results
    timeManager->_dtRemainder = 0.0;

    SGTimeStamp n;

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, simDt, 1.0e-6);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, realDt, 1.0e-6);

    timeManager->_lastStamp = SGTimeStamp::now() - SGTimeStamp::fromMSec(15);
    timeManager->computeTimeDeltas(simDt, realDt);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.008333, simDt, 1.0e-5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.008333, realDt, 1.0e-5);

    fgSetBool("/sim/freeze/clock", true);
    timeManager->_lastStamp = SGTimeStamp::now() - SGTimeStamp::fromMSec(20);
    timeManager->computeTimeDeltas(simDt, realDt);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, simDt, 1.0e-5); // sim time should not advance
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.025, realDt, 1.0e-5);
}

void TimeManagerTests::testTimeZones()
{
    auto timeManager = globals->get_subsystem<TimeManager>();

    auto vabb = fgFindAirportID("VABB");
    FGTestApi::setPositionAndStabilise(vabb->geod());


    // set standard values
    fgSetBool("/sim/freeze", false);
    fgSetBool("/sim/sceneryloaded", true);
    fgSetDouble("/sim/model-hz", 120.0);

    timeManager->postinit();

    // fake Unix time by setting this; it will then
    // set the 'current unix time' passed to SGTime
    const auto testDate = 314611200L;
    fgSetInt("/sim/time/cur-time-override", testDate);

    timeManager->update(0.0);

    CPPUNIT_ASSERT_EQUAL((time_t)19800, globals->get_time_params()->get_local_offset());

    auto gmt = globals->get_time_params()->getGmt();
    CPPUNIT_ASSERT_EQUAL(79, gmt->tm_year);
    CPPUNIT_ASSERT_EQUAL(11, gmt->tm_mon);
    CPPUNIT_ASSERT_EQUAL(21, gmt->tm_mday);


    // relocate to somewhere, check the time values update

    auto eddf = FGAirport::getByIdent("EDDF");
    FGTestApi::setPositionAndStabilise(eddf->geod());
    timeManager->reposition();
    timeManager->update(0.0);

    CPPUNIT_ASSERT_EQUAL((time_t)3600, globals->get_time_params()->get_local_offset());


    auto zbaa = FGAirport::getByIdent("ZBAA");
    FGTestApi::setPositionAndStabilise(zbaa->geod());
    timeManager->reposition();
    timeManager->update(0.0);

    CPPUNIT_ASSERT_EQUAL((time_t)28800, globals->get_time_params()->get_local_offset());
}

void TimeManagerTests::testETCTimeZones()
{
    auto timeManager = globals->get_subsystem<TimeManager>();

    auto phto = fgFindAirportID("PHTO");
    FGTestApi::setPositionAndStabilise(phto->geod());

    timeManager->postinit();

    // fake Unix time by setting this; it will then
    // set the 'current unix time' passed to SGTime
    const auto testDate = 314611200L;
    fgSetInt("/sim/time/cur-time-override", testDate);

    FGTestApi::setPositionAndStabilise(phto->geod());
    timeManager->reposition();
    timeManager->update(0.0);

    SGPropertyNode_ptr tzNameNode = fgGetNode("/sim/time/local-timezone", true);

    CPPUNIT_ASSERT_EQUAL((time_t)-36000, globals->get_time_params()->get_local_offset());
    CPPUNIT_ASSERT_EQUAL("Pacific/Honolulu"s, string{tzNameNode->getStringValue()});

    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->setSpeedKts(1000);
    pilot->setCourseTrue(320.0);

    bool ok = FGTestApi::runForTimeWithCheck(600.0, [tzNameNode]() {
        const string tz = tzNameNode->getStringValue();
        return tz == "Etc/GMT+10"s;
    });
    CPPUNIT_ASSERT(ok);

    ok = FGTestApi::runForTimeWithCheck(600.0, [tzNameNode]() {
        const string tz = tzNameNode->getStringValue();
        return tz == "Pacific/Honolulu"s;
    });
    CPPUNIT_ASSERT(ok);
}

void TimeManagerTests::testSpecifyTimeOffset()
{
    // disabled for now since this code depends on epehmeris as well
    // to define sun position
    return;

    auto timeManager = globals->get_subsystem<TimeManager>();

    // set standard values
    fgSetBool("/sim/freeze", false);
    fgSetBool("/sim/sceneryloaded", true);
    fgSetDouble("/sim/model-hz", 120.0);

    timeManager->postinit();

    const auto testDate = 314611200L;
    fgSetInt("/sim/time/cur-time-override", testDate);

    auto uudd = fgFindAirportID("UUDD");
    FGTestApi::setPositionAndStabilise(uudd->geod());

    timeManager->setTimeOffset("dawn", 0);

    timeManager->update(0.0);

    auto localTime = globals->get_time_params()->get_cur_time();

    CPPUNIT_ASSERT_EQUAL((time_t) 0, localTime);
}
