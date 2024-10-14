/*
 * SPDX-FileCopyrightText: (C) 2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "test_swiftAircraftManager.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"

#include "Network/Swift/SwiftAircraftManager.h"
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

void SwiftAircraftManagerTest::setUp()
{
    FGTestApi::setUp::initTestGlobals("SwiftService");
}

void SwiftAircraftManagerTest::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


std::vector<SGSharedPtr<FGAIBase>> SwiftAircraftManagerTest::getAIList()
{
    return globals->get_subsystem<FGAIManager>()->get_ai_list();
}

void SwiftAircraftManagerTest::testAircraftManager()
{
    globals->get_subsystem_mgr()->add<FGAIManager>();
    globals->get_subsystem<FGAIManager>()->bind();
    globals->get_subsystem<FGAIManager>()->init();

    flightgear::swift::FGSwiftAircraftManager acm;
    acm.addPlane("BER123", "PATH_TO_MODEL");
    CPPUNIT_ASSERT_EQUAL(globals->get_subsystem<FGAIManager>()->get_ai_list().size(), (size_t)1);

    acm.addPlane("BAW123", "PATH_TO_MODEL");
    CPPUNIT_ASSERT_EQUAL(globals->get_subsystem<FGAIManager>()->get_ai_list().size(), (size_t)2);

    for (auto& aircraft : getAIList()) {
        CPPUNIT_ASSERT(!aircraft->getDie());
    }

    acm.removeAllPlanes();
    for (auto& aircraft : getAIList()) {
        CPPUNIT_ASSERT(aircraft->getDie());
    }

    acm.addPlane("BER123", "PATH_TO_MODEL");
    CPPUNIT_ASSERT(!getAIList()[2]->getDie());
    acm.removePlane("BER123");
    CPPUNIT_ASSERT(getAIList()[2]->getDie());

    // Test position updates
    acm.addPlane("SAS123", "PATH_TO_MODEL");
    SGGeod position;
    position.setLatitudeDeg(50.0);
    position.setLongitudeDeg(6.0);
    position.setElevationM(1024);

    acm.updatePlanes({{"SAS123", position, SGVec3d(1.0, 2.0, 3.0), 200, false}});
    CPPUNIT_ASSERT_EQUAL(fgGetString("/ai/models/swift[3]/callsign"), std::string("SAS123"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/orientation/pitch-deg"), 1.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/orientation/roll-deg"), 2.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/orientation/true-heading-deg"), 3.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/position/latitude-deg"), 50.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/position/longitude-deg"), 6.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/velocities/true-airspeed-kt"), 200, 0.1);

    position.setLatitudeDeg(20.0);
    position.setLongitudeDeg(4.0);
    acm.updatePlanes({{"SAS123", position, SGVec3d(5.0, 6.0, 7.0), 400, false}});
    CPPUNIT_ASSERT_EQUAL(fgGetString("/ai/models/swift[3]/callsign"), std::string("SAS123"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/orientation/pitch-deg"), 5.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/orientation/roll-deg"), 6.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/orientation/true-heading-deg"), 7.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/position/latitude-deg"), 20.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/position/longitude-deg"), 4.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/velocities/true-airspeed-kt"), 400, 0.1);


    // Update another aircraft
    acm.addPlane("DAL123", "PATH_TO_MODEL");
    position.setLatitudeDeg(-20.0);
    position.setLongitudeDeg(5.0);
    acm.updatePlanes({{"DAL123", position, SGVec3d(1.0, 1.0, 1.0), 250, false}});
    CPPUNIT_ASSERT_EQUAL(fgGetString("/ai/models/swift[4]/callsign"), std::string("DAL123"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[4]/orientation/pitch-deg"), 1.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[4]/orientation/roll-deg"), 1.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[4]/orientation/true-heading-deg"), 1.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[4]/position/latitude-deg"), -20.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[4]/position/longitude-deg"), 5.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[4]/velocities/true-airspeed-kt"), 250, 0.1);

    CPPUNIT_ASSERT_EQUAL(fgGetString("/ai/models/swift[3]/callsign"), std::string("SAS123"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/orientation/pitch-deg"), 5.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/orientation/roll-deg"), 6.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/orientation/true-heading-deg"), 7.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/position/latitude-deg"), 20.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/position/longitude-deg"), 4.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fgGetDouble("/ai/models/swift[3]/velocities/true-airspeed-kt"), 400, 0.1);
}
