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

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Airports/airport.hxx>
#include <Traffic/TrafficMgr.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

// Set up function for each test.
void TrafficMgrTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("TrafficMgr");
    FGTestApi::setUp::initNavDataCache();
    fgSetBool("sim/ai/enabled", true);
    fgSetBool("sim/traffic-manager/enabled", true);
    fgSetBool("/environment/realwx/enabled", false);
    fgSetBool("/environment/metar/valid", false);
    //Otherwise TrafficMgr won't load
    fgSetBool("sim/signals/fdm-initialized", true);
    globals->set_fg_root(SGPath::fromUtf8(FG_TEST_SUITE_DATA));
}

// Clean up after each test.
void TrafficMgrTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void TrafficMgrTests::testParse() {
    globals->get_subsystem_mgr()->add<FGTrafficManager>();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();

    auto tmgr = globals->get_subsystem<FGTrafficManager>();
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
        std::cout << (*i)->getCallSign() << counter++ << std::endl;
    }
    CPPUNIT_ASSERT_EQUAL(2, counter);
}

void TrafficMgrTests::testTrafficManager()
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

    auto tmgr = globals->get_subsystem_mgr()->add<FGTrafficManager>();

    tmgr->bind();
    tmgr->init();

    for( int i = 0; i < 30; i++) {
        bool active = fgGetBool("/sim/traffic-manager/inited");
        // std::cout << "Inited " << "\t" << i << "\t" << active << "\r\n";
        FGTestApi::runForTime(5.0);
        if(active) {
            break;
        }
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

    FGTestApi::runForTime(360.0);

    FGScheduledFlightVecIterator fltBegin, fltEnd;
    fltBegin = tmgr->getFirstFlight("HBR_BN_2");
    fltEnd = tmgr->getLastFlight("HBR_BN_2");

    if (fltBegin == fltEnd) {
        CPPUNIT_FAIL("No Traffic found");
    }

    int counter = 0;
    for (FGScheduledFlightVecIterator i = fltBegin; i != fltEnd; i++) {
        std::cout << (*i)->getDepartureAirport()->getId() << "\t" << (*i)->getArrivalAirport()->getId() << "\t" << (*i)->getDepartureTime() << "\n";
        counter++;
    }
   CPPUNIT_ASSERT_EQUAL(25, counter);
}
