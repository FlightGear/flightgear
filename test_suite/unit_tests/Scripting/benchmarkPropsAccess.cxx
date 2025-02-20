
// SPDX-FileCopyrightText: (C) 2024 James Turner
// SPDX-License-Identifier: GPL-2.0-or-later

#include "benchmarkPropsAccess.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Scripting/NasalSys.hxx>

#include <Main/FGInterpolator.hxx>
#include <simgear/debug/debug_types.h>

// Set up function for each test.
void BenchmarkPropsAccess::setUp()
{
    FGTestApi::setUp::initTestGlobals("BenchmarkNasalProps");

    fgInitAllowedPaths();

    globals->get_subsystem_mgr()->add<FGInterpolator>();

    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();

    globals->get_subsystem_mgr()->add<FGNasalSys>();

    globals->get_subsystem_mgr()->postinit();
}


// Clean up after each test.
void BenchmarkPropsAccess::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


void BenchmarkPropsAccess::benchSetProp()
{
    SGTimeStamp s;
    s.stamp();

    bool ok = FGTestApi::executeNasal(R"(
        var iter = 4000;
        for (var i=0; i < iter; i += 1) {
            setprop('/foo/bar/v', i);
            setprop('/foo/bar/zot', 'apples');
            setprop('/foo/bar/w', 1.23456);
        }

    )");
    CPPUNIT_ASSERT(ok);
    SG_LOG(SG_GENERAL, SG_INFO, "set-prop took:" << s.elapsedUSec());
}

void BenchmarkPropsAccess::benchPropsNodeSet()
{
    SGTimeStamp s;
    s.stamp();

    bool ok = FGTestApi::executeNasal(R"(
        var iter = 4000;
        var node = props.globals.getNode('/foo/bar/v', 1);
        var node2 = props.globals.getNode('/foo/bar/zot', 1);
        var node3 = props.globals.getNode('/foo/bar/w', 1);

        for (var i=0; i < iter; i += 1) {
            node.setValue(i);
            node2.setValue('apples');
            node3.setValue(1.23456);
        }

    )");
    CPPUNIT_ASSERT(ok);
    SG_LOG(SG_GENERAL, SG_INFO, "props.Node set took:" << s.elapsedUSec());
}
