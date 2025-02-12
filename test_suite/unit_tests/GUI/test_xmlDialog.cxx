/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 James Turner <james@flightgear.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "test_xmlDialog.hxx"
#include "config.h"


#include "cppunit/TestAssert.h"
#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <simgear/canvas/Canvas.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/fg_commands.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include <Canvas/canvas_mgr.hxx>
#include <Canvas/gui_mgr.hxx>

#include "test_suite/FGTestApi/DummyCanvasSystemAdapter.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <GUI/FGPUICompatDialog.hxx>
#include <GUI/PUICompatObject.hxx>
#include <GUI/new_gui.hxx>

using namespace std::string_literals;
using namespace flightgear;

extern bool global_nasalMinimalInit;


void XMLDialogTests::setUp()
{
    global_nasalMinimalInit = false;

    FGTestApi::setUp::initTestGlobals("xmlui");
    FGTestApi::setUp::initNavDataCache(); // dialog loader uses the cache

    fgSetBool("/sim/menubar/enable", false);

    // Canvas needs loadxml command
    fgInitCommands();

    simgear::canvas::Canvas::setSystemAdapter(
        simgear::canvas::SystemAdapterPtr(new canvas::DummyCanvasSystemAdapter));

    auto sm = globals->get_subsystem_mgr();
    sm->add<CanvasMgr>();
    sm->add<NewGUI>();
    auto canvasGui = new GUIMgr;
    sm->add("CanvasGUI", canvasGui, SGSubsystemMgr::DISPLAY);


    sm->bind();
    sm->init();

    FGTestApi::setUp::initStandardNasal(true /* withCanvas */);
    sm->postinit();
}

void XMLDialogTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void XMLDialogTests::testParseVersion1()
{
    const auto dialogPath = SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "gui" / "dialog1.xml";

    SGPropertyNode_ptr props = new SGPropertyNode;
    readProperties(dialogPath, props);

    SGSharedPtr<FGPUICompatDialog> dlg(new FGPUICompatDialog(props));

    CPPUNIT_ASSERT(dlg->init());

    auto hbox = dlg->widgetByName("main-hbox");
    CPPUNIT_ASSERT_EQUAL(hbox->type(), "group"s);

    auto cb = dlg->widgetByName("cancel-button");
    CPPUNIT_ASSERT(cb);
}

void XMLDialogTests::testParseVersion2()
{
    const auto dialogPath = SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "gui" / "dialog2.xml";

    SGPropertyNode_ptr props = new SGPropertyNode;
    readProperties(dialogPath, props);

    SGSharedPtr<FGPUICompatDialog> dlg(new FGPUICompatDialog(props));

    CPPUNIT_ASSERT(dlg->init());

    auto rb = dlg->widgetByName("radio1");

    CPPUNIT_ASSERT_EQUAL(rb->radioGroupIdent(), "myGroupA"s);
}

void XMLDialogTests::testNasalAPI()
{
    bool ok = FGTestApi::executeNasal(R"(
        
    )");
    CPPUNIT_ASSERT(ok);
}