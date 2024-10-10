/*
 * SPDX-FileCopyrightText: (C) 2024 James Turner <james@flightgear.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */


#include "test_headingIndicator.hxx"

#include <cstring>
#include <memory>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Airports/airport.hxx>
#include <Navaids/NavDataCache.hxx>

#include <Instrumentation/heading_indicator_dg.hxx>
#include <Main/fg_props.hxx>
#include <Main/locale.hxx>

// Set up function for each test.
void HeadingIndicatorTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("heading-indicator-dg");
    FGTestApi::setUp::initNavDataCache();

    // otherwise ATCSPeech will call locale functions and assert
    globals->get_locale()->selectLanguage({});
}


// Clean up after each test.
void HeadingIndicatorTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


// std::string NavRadioTests::formatFrequency(double f)
// {
//     char buf[16];
//     ::snprintf(buf, 16, "%3.2f", f);
//     return buf;
// }

SGSubsystemRef HeadingIndicatorTests::setupInstrument(const std::string& name, int index)
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", name);
    configNode->setIntValue("number", index);
    configNode->setBoolValue("new-default-power-path", true);

    auto r = new HeadingIndicatorDG(configNode);

    r->bind();
    r->init();

    globals->get_subsystem_mgr()->add("heading-indicator-dg", r);

    auto elecOuts = fgGetNode("/systems/electrical/outputs/", true);
    elecOuts->getChild("heading-indicator-dg", index, true)->setDoubleValue(12.0);

    fgSetDouble("/accelerations/pilot-g", 1.0);

    return r;
}

void HeadingIndicatorTests::testBasic()
{
    auto r = setupInstrument("hi", 2);
    FGAirportRef apt = FGAirport::getByIdent("EDDM");
    FGTestApi::setPositionAndStabilise(apt->geod());

    SGPropertyNode_ptr n = globals->get_props()->getNode("instrumentation/hi[2]");
    auto indicatedHeadingNode = n->getChild("indicated-heading-deg");

    fgSetDouble("/orientation/heading-deg", 77.0);
    FGTestApi::runForTime(6.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, n->getDoubleValue("spin"), 1e-2);

    n->setDoubleValue("offset-deg", 2.0);
    r->update(0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(79.0, indicatedHeadingNode->getDoubleValue(), 0.1);

// setup a wrap condition around 360.0 / 0.0
    fgSetDouble("/orientation/heading-deg", 358.0);
    n->setDoubleValue("offset-deg", -2);
    FGTestApi::runForTime(6.0);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(356.0, indicatedHeadingNode->getDoubleValue(), 0.1);

    fgSetDouble("/orientation/heading-deg", 5.0);
    for (int i=0; i<10; ++i) {
        r->update(0.01);
        auto indicated = n->getDoubleValue("indicated-heading-deg");
        CPPUNIT_ASSERT((indicated >= 356) || (indicated < 3.0));
    }

    FGTestApi::runForTime(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, indicatedHeadingNode->getDoubleValue(), 0.1);

// setup a wrap condition around 180.0
    fgSetDouble("/orientation/heading-deg", 182.0);
    n->setDoubleValue("offset-deg", 2);
    FGTestApi::runForTime(6.0);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(184.0, indicatedHeadingNode->getDoubleValue(), 0.1);

    fgSetDouble("/orientation/heading-deg", 175.0);
    for (int i=0; i<10; ++i) {
        r->update(0.01);
        auto indicated = indicatedHeadingNode->getDoubleValue();
        CPPUNIT_ASSERT((indicated >= 176.0) && (indicated < 184.0));
    }

    FGTestApi::runForTime(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(177, indicatedHeadingNode->getDoubleValue(), 0.1);

    // one more wrap condition
    fgSetDouble("/orientation/heading-deg", 270.0);
    n->setDoubleValue("offset-deg", 87);
    FGTestApi::runForTime(6.0); // stabilize

    CPPUNIT_ASSERT_DOUBLES_EQUAL(357.0, indicatedHeadingNode->getDoubleValue(), 0.1);

    fgSetDouble("/orientation/heading-deg", 250.0);
    for (int i = 0; i < 10; ++i) {
        r->update(0.01);
        auto indicated = indicatedHeadingNode->getDoubleValue();

        CPPUNIT_ASSERT((indicated < 357.0) && (indicated > 336.0));
    }

    FGTestApi::runForTime(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(337.0, indicatedHeadingNode->getDoubleValue(), 0.1);

    // one more wrap condition
    fgSetDouble("/orientation/heading-deg", 270.0);
    n->setDoubleValue("offset-deg", 97);
    FGTestApi::runForTime(2.0); // stabilize

    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.0, indicatedHeadingNode->getDoubleValue(), 0.1);

    fgSetDouble("/orientation/heading-deg", 250.0);
    for (int i = 0; i < 10; ++i) {
        r->update(0.01);
        auto indicated = indicatedHeadingNode->getDoubleValue();
        std::cerr << "Indicated:" << indicated << std::endl;
        CPPUNIT_ASSERT((indicated < 7.0) || (indicated > 346.0));
    }

    FGTestApi::runForTime(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(347.0, indicatedHeadingNode->getDoubleValue(), 0.1);

    // test alignment adjustment
    fgSetDouble("/orientation/heading-deg", 182.0);
    n->setDoubleValue("offset-deg", 0.0);
    n->setDoubleValue("align-deg", 42.0);
    FGTestApi::runForTime(1.0);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(224.0, indicatedHeadingNode->getDoubleValue(), 0.1);

    // test error adjustment
    n->setDoubleValue("align-deg", 10.0);
    n->setDoubleValue("error-deg", 13.0);
    r->update(0.01);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(205.0, indicatedHeadingNode->getDoubleValue(), 0.1);
}

void HeadingIndicatorTests::testTumble()
{
    auto r = setupInstrument("hi", 2);
    SGPropertyNode_ptr n = globals->get_props()->getNode("instrumentation/hi[2]");

    FGAirportRef apt = FGAirport::getByIdent("EDDM");
    FGTestApi::setPositionAndStabilise(apt->geod());

    // spin-up
    FGTestApi::runForTime(6.0);
    CPPUNIT_ASSERT(!n->getBoolValue("is-caged"));

    // not tumbled
    CPPUNIT_ASSERT(!n->getBoolValue("tumble-flag"));
    fgSetDouble("/accelerations/pilot-g", 1.4);
    r->update(0.1);
    CPPUNIT_ASSERT(!n->getBoolValue("tumble-flag"));

    fgSetDouble("/accelerations/pilot-g", 2.5);
    FGTestApi::runForTime(1.0);
    CPPUNIT_ASSERT(n->getBoolValue("tumble-flag"));

    // back to normal Gs, should stay tumbled
    fgSetDouble("/accelerations/pilot-g", 1.0);
    FGTestApi::runForTime(3.0);
    CPPUNIT_ASSERT(n->getBoolValue("tumble-flag"));
}


void HeadingIndicatorTests::testLatitudeNut()
{
    auto r = setupInstrument("hi", 2);

    FGAirportRef apt = FGAirport::getByIdent("EDDM");
    FGTestApi::setPositionAndStabilise(apt->geod());

    SGPropertyNode_ptr n = globals->get_props()->getNode("instrumentation/hi[2]");
    fgSetDouble("/orientation/heading-deg", 39.0);
    FGTestApi::runForTime(6.0);

    r->update(0.1);
    n->setDoubleValue("offset-deg", 0.0); // remove spin-up offset
                                          // check wander
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-10.6, n->getDoubleValue("drift-per-hour-deg"), 0.1);
    n->setDoubleValue("latitude-nut-setting", apt->latitude());

    // set nut, should negate the drift
    FGTestApi::runForTime(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, n->getDoubleValue("drift-per-hour-deg"), 0.1);
}

void HeadingIndicatorTests::testVacuumGyro()
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "hi-dg");
    configNode->setIntValue("number", 0);
    configNode->setStringValue("suction", "/test/suction");
    configNode->setDoubleValue("minimum-vacuum", 4.0);
    auto r = new HeadingIndicatorDG(configNode);

    r->bind();
    r->init();

    globals->get_subsystem_mgr()->add("heading-indicator-dg", r);
    SGPropertyNode_ptr n = globals->get_props()->getNode("instrumentation/hi-dg[0]");

    fgSetDouble("/test/suction", 3.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, n->getDoubleValue("spin"), 0.1);

    fgSetDouble("/test/suction", 6.0);
    FGTestApi::runForTime(2.0);

    // should still be spinning up
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.75, n->getDoubleValue("spin"), 0.1);

    FGTestApi::runForTime(6.0);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, n->getDoubleValue("spin"), 0.1);
}