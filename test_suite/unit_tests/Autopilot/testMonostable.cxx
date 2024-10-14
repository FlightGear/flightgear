/*
 * SPDX-FileName: testMonostable.cxx
 * SPDX-FileComment: Unit tests for monostable autopilot element
 * SPDX-FileCopyrightText: Copyright (C) 2023 Huntley Palmer
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "testMonostable.hxx"

#include <sstream>

#include "test_suite/FGTestApi/testGlobals.hxx"


#include <Autopilot/autopilot.hxx>
#include <Autopilot/digitalfilter.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>


#include <simgear/math/sg_random.hxx>
#include <simgear/props/props_io.hxx>

// Set up function for each test.
void MonostableTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("ap-monostable");
}


// Clean up after each test.
void MonostableTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


SGPropertyNode_ptr MonostableTests::configFromString(const std::string& s)
{
    SGPropertyNode_ptr config = new SGPropertyNode;

    std::istringstream iss(s);
    readProperties(iss, config);
    return config;
}

void MonostableTests::testMonostable()
{
    sg_srandom(999);
    //
    // Simple monostable            from wiki:
    //   monostable stable state is false,  true when Set, until timeout
    //   ( S == False, test/S == 0  ) :  O/P  = 0
    //   ( S == True , test/S == 1  ) :  O/P  = 1
    //   ( S == False, test/S == 0  ) :  Timer running,  O/P = 1
    //   ( S == False, test/S == 0  ) :  Timed out    ,  O/P = 0
    //
    //
    auto config = configFromString(R"(<?xml version="1.0" encoding="UTF-8"?>
                                    <PropertyList>
                                    <flipflop>
                                        <type>monostable</type>
                                        <S> 
                                          <property>/test/S</property>
                                        </S>
                                        <time>
                                            <value>0.50</value>
                                        </time>
                                        <output>/test/Q</output>
                                    </flipflop>
                                    </PropertyList>
                                    )");

    auto ap = new FGXMLAutopilot::Autopilot(globals->get_props(), config);

    globals->get_subsystem_mgr()->add("ap", ap);
    ap->bind();
    ap->init();

    // Initially        test/S is 0  O/P s.b    stable state == 0
    fgSetBool("/test/S", false);
    ap->update(0.01);
    CPPUNIT_ASSERT_EQUAL(false, fgGetBool("/test/Q"));

    // steady input            test/S  = 0  s.b. no change
    ap->update(0.24);
    CPPUNIT_ASSERT_EQUAL(false, fgGetBool("/test/Q"));

    // steady           test/S  = 0  s.b. no change during timer
    ap->update(0.25);
    CPPUNIT_ASSERT_EQUAL(false, fgGetBool("/test/Q"));

    // steady           test/S  = 0  s.b. no change after timeout
    ap->update(0.25);
    CPPUNIT_ASSERT_EQUAL(false, fgGetBool("/test/Q"));

    // Set S input true == 1   O/P should immediately = 1, true
    fgSetBool("/test/S", true);
    ap->update(0.01);
    CPPUNIT_ASSERT_EQUAL(true, fgGetBool("/test/Q"));

    // steady           test/S  = 1  s.b. no change during timer
    ap->update(0.24);
    CPPUNIT_ASSERT_EQUAL(true, fgGetBool("/test/Q"));

    // steady           test/S  = 1  s.b. no change after timeout
    ap->update(0.25);
    CPPUNIT_ASSERT_EQUAL(true, fgGetBool("/test/Q"));

    // Reset Input               test/S  = 0  s.b. no change
    fgSetBool("/test/S", false);
    ap->update(0.01);
    CPPUNIT_ASSERT_EQUAL(true, fgGetBool("/test/Q"));

    // steady           test/S  = 0  s.b. no change during timer
    ap->update(0.24);
    CPPUNIT_ASSERT_EQUAL(true, fgGetBool("/test/Q"));

    // steady           test/S  = 0  O/P resets = 0 after timeout
    ap->update(0.25);
    CPPUNIT_ASSERT_EQUAL(false, fgGetBool("/test/Q"));
}
