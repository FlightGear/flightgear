/*
 * Copyright (C) 2021 Keith Paterson
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

#include "test_TrafficMgr.hxx"

#include <cstring>
#include <memory>
#include <unistd.h>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Traffic/TrafficMgr.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

// Set up function for each test.
void TrafficMgrTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("TrafficMgr");
    FGTestApi::setUp::initNavDataCache();
    auto props = globals->get_props();
    props->setBoolValue("sim/ai/enabled", true);
    props->setBoolValue("sim/traffic-manager/enabled", true);
    fgSetBool("/environment/realwx/enabled", false);
    fgSetBool("/environment/metar/valid", false);
    //Otherwise TrafficMgr won't load
    props->setBoolValue("sim/signals/fdm-initialized", true);
    globals->set_fg_root(SGPath::fromUtf8(FG_TEST_SUITE_DATA));
}

// Clean up after each test.
void TrafficMgrTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


void TrafficMgrTests::testParse() {
    globals->add_new_subsystem<FGTrafficManager>();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();

    FGTrafficManager *tmgr = (FGTrafficManager *) globals->get_subsystem("traffic-manager");
    FGScheduledFlightVecIterator fltBegin, fltEnd;

    for (size_t i = 0; i < 1000000; i++)
    {
        FGTestApi::runForTime(10.0);
        // We have to wait for async parser
        fltBegin = tmgr->getFirstFlight("TST_BN_2");
        fltEnd   = tmgr->getLastFlight("TST_BN_2");
        if (fltBegin != fltEnd) {
            break;
        }
    }

    int counter = 0;
    for (FGScheduledFlightVecIterator i = fltBegin; i != fltEnd; i++) {
        cout << (*i)->getCallSign() << counter++ << endl;
    }
    CPPUNIT_ASSERT_EQUAL(2, counter);
}
