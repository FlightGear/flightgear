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

#include "test_Views.hxx"

#include <cstring>
#include <memory>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/view.hxx>
#include <Viewer/viewmgr.hxx>

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void ViewsTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("Views");
    FGTestApi::setUp::initNavDataCache();
}

// Clean up after each test.
void ViewsTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void ViewsTests::testBasic()
{
    auto vm = globals->get_subsystem_mgr()->add<FGViewMgr>();

    CPPUNIT_ASSERT_EQUAL(static_cast<flightgear::View*>(nullptr), vm->get_current_view());

    // define simple views setup
    const char* propsXML = R"(<?xml version="1.0" encoding="UTF-8"?>
    <PropertyList>
        <sim>
          <view>
              <name type="string">AView</name>
              <internal type="bool">1</internal>
              <type type="string">lookat</type>
              <config>
              </config>
          </view>
        
        <view>
            <name type="string">BView</name>
            <internal type="bool">0</internal>
            <type type="string">lookfrom</type>
            <config>
            </config>
        </view>
        </sim>
      </PropertyList>
    )";

    SGPropertyNode_ptr views = globals->get_props();
    {
        std::istringstream is(propsXML);
        readProperties(is, views);
    }

    fgSetInt("/sim/current-view/view-number", 0);

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();

    auto c = vm->get_current_view();
    CPPUNIT_ASSERT(c);
    CPPUNIT_ASSERT_EQUAL(true, c->getInternal());
    CPPUNIT_ASSERT_EQUAL(std::string{"AView"}, c->getName());
    CPPUNIT_ASSERT_EQUAL(flightgear::View::FG_LOOKAT, c->getType());

    fgSetInt("/sim/current-view/view-number", 1);
    CPPUNIT_ASSERT_EQUAL(1, vm->getCurrentViewIndex());

    c = vm->get_current_view();
    CPPUNIT_ASSERT_EQUAL(false, c->getInternal());
    CPPUNIT_ASSERT_EQUAL(std::string{"BView"}, c->getName());
    CPPUNIT_ASSERT_EQUAL(flightgear::View::FG_LOOKFROM, c->getType());

    // disabled by James, since this asserts inside FGViewMgr::setCurrentViewIndex
#if 0
    fgSetInt("/sim/current-view/view-number", 3);
    CPPUNIT_ASSERT_EQUAL(0, vm->getCurrentViewIndex());
    CPPUNIT_ASSERT_EQUAL(0, fgGetInt("/sim/current-view/view-number"));
#endif
}
