/*
 * SPDX-FileCopyrightText: (C) 2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "test_swiftService.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"

#include "Network/Swift/service.h"
#include <Main/fg_props.hxx>

void SwiftServiceTest::setUp()
{
    FGTestApi::setUp::initTestGlobals("SwiftService");

    // Setup properties
    fgSetBool("/sim/freeze/master", true);
    fgSetDouble("/position/latitude-deg", 50.12);
    fgSetDouble("/position/longitude-deg", 6.3);
    fgSetDouble("/position/altitude-ft", 12000.0);
    fgSetDouble("/position/altitude-agl-ft", 1020.0);
    fgSetDouble("/velocities/groundspeed-kt", 242.0);
    fgSetDouble("/orientation/pitch-deg", 3.0);
    fgSetDouble("/orientation/roll-deg", 1.0);
    fgSetDouble("/orientation/heading-deg", 230.0);
    fgSetBool("/gear/gear/wow", false);
    fgSetDouble("/instrumentation/comm/frequencies/selected-mhz", 122.8);
    fgSetDouble("/instrumentation/comm/frequencies/standby-mhz", 135.65);
    fgSetDouble("/instrumentation/comm[1]/frequencies/selected-mhz", 121.5);
    fgSetDouble("/instrumentation/comm[1]/frequencies/standby-mhz", 118.3);
    fgSetInt("/instrumentation/transponder/id-code", 1234);
    fgSetInt("/instrumentation/transponder/inputs/knob-mode", 1);
    fgSetBool("/instrumentation/transponder/ident", true);
    fgSetBool("/controls/lighting/beacon", true);
    fgSetBool("/controls/lighting/landing-lights", false);
    fgSetBool("/controls/lighting/nav-lights", true);
    fgSetBool("/controls/lighting/strobe", true);
    fgSetBool("/controls/lighting/taxi-light", false);
    fgSetBool("/instrumentation/altimeter/serviceable", true);
    fgSetDouble("/instrumentation/altimeter/pressure-alt-ft", 24000.0);
    fgSetDouble("/surface-positions/flap-pos-norm", 0.0);
    fgSetDouble("/gear/gear/position-norm", 0.7);
    fgSetDouble("/surface-positions/speedbrake-pos-norm", 0.4);
    fgSetString("/sim/aircraft", "glider");
    fgSetDouble("/position/ground-elev-m", 778.0);
    fgSetDouble("/velocities/speed-east-fps", 20.0);
    fgSetDouble("/velocities/speed-down-fps", -30.0);
    fgSetDouble("/velocities/speed-north-fps", -10.2);
    fgSetDouble("/orientation/roll-rate-degps", 1.0);
    fgSetDouble("/orientation/pitch-rate-degps", 0.0);
    fgSetDouble("/orientation/yaw-rate-degps", -2.0);
    fgSetDouble("/instrumentation/comm/volume", 42.0);
    fgSetDouble("/instrumentation/comm[1]/volume", 100.0);
}

void SwiftServiceTest::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void SwiftServiceTest::testService()
{
    flightgear::swift::CService service;
    CPPUNIT_ASSERT(service.isPaused());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getLatitude(), 50.12, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getLongitude(), 6.3, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getAltitudeMSL(), 12000.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getHeightAGL(), 1020.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getGroundSpeed(), 242.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getPitch(), 3.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getRoll(), 1.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getTrueHeading(), 230.0, 0.1);
    CPPUNIT_ASSERT(!service.getAllWheelsOnGround());
    CPPUNIT_ASSERT_EQUAL(service.getCom1Active(), 122800);
    CPPUNIT_ASSERT_EQUAL(service.getCom1Standby(), 135650);
    CPPUNIT_ASSERT_EQUAL(service.getCom2Active(), 121500);
    CPPUNIT_ASSERT_EQUAL(service.getCom2Standby(), 118300);

    CPPUNIT_ASSERT_EQUAL(service.getTransponderCode(), 1234);
    CPPUNIT_ASSERT_EQUAL(service.getTransponderMode(), 1);
    CPPUNIT_ASSERT(service.getTransponderIdent());
    CPPUNIT_ASSERT(service.getBeaconLightsOn());
    CPPUNIT_ASSERT(!service.getLandingLightsOn());
    CPPUNIT_ASSERT(service.getNavLightsOn());
    CPPUNIT_ASSERT(service.getStrobeLightsOn());
    CPPUNIT_ASSERT(!service.getTaxiLightsOn());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getFlapsDeployRatio(), 0.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getGearDeployRatio(), 0.7, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getSpeedBrakeRatio(), 0.4, 0.1);

    CPPUNIT_ASSERT_EQUAL(service.getAircraftName(), std::string("glider"));


    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getGroundElevation(), 778.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getVelocityX(), 20.0 * SG_FEET_TO_METER, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getVelocityY(), -30.0 * SG_FEET_TO_METER * -1, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getVelocityZ(), -10.2 * SG_FEET_TO_METER, 0.1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getRollRate(), 1.0 * SG_DEGREES_TO_RADIANS, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getPitchRate(), 0.0 * SG_DEGREES_TO_RADIANS, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getYawRate(), -2.0 * SG_DEGREES_TO_RADIANS, 0.1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getCom1Volume(), 42.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getCom2Volume(), 100.0, 0.1);


    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getPressAlt(), 24000.0, 0.1);
    fgSetBool("/instrumentation/altimeter/serviceable", false);
    // Fallback if altimeter is not serviceable
    CPPUNIT_ASSERT_DOUBLES_EQUAL(service.getPressAlt(), service.getAltitudeMSL(), 0.1);

    // Test setter
    service.setCom1Active(128550);
    CPPUNIT_ASSERT_EQUAL(service.getCom1Active(), 128550);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/instrumentation/comm/frequencies/selected-mhz"), 128.550, 0.1);

    service.setCom1Standby(128650);
    CPPUNIT_ASSERT_EQUAL(service.getCom1Standby(), 128650);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/instrumentation/comm/frequencies/standby-mhz"), 128.650, 0.1);


    service.setCom2Active(121900);
    CPPUNIT_ASSERT_EQUAL(service.getCom2Active(), 121900);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/instrumentation/comm[1]/frequencies/selected-mhz"), 121.900, 0.1);

    service.setCom2Standby(121600);
    CPPUNIT_ASSERT_EQUAL(service.getCom2Standby(), 121600);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/instrumentation/comm[1]/frequencies/standby-mhz"), 121.600, 0.1);

    service.setTransponderCode(2000);
    CPPUNIT_ASSERT_EQUAL(service.getTransponderCode(), 2000);
    CPPUNIT_ASSERT_EQUAL(fgGetInt("/instrumentation/transponder/id-code"), 2000);


    service.setTransponderMode(0);
    CPPUNIT_ASSERT_EQUAL(service.getTransponderMode(), 0);
    CPPUNIT_ASSERT_EQUAL(fgGetInt("/instrumentation/transponder/inputs/knob-mode"), 0);
}
