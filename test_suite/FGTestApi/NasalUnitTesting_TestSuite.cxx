// Unit-test API for nasal
//
// There are two versions of this module, and we load one or the other
// depending on if we're running the test_suite (using CppUnit) or
// the normal simulator. The logic is that aircraft-developers and
// people hacking Nasal likely don't have a way to run the test-suite,
// whereas core-developers and Jenksin want a way to run all tests 
// through the standard CppUnit mechanim. So we have a consistent
// Nasal API, but different implement in fgfs_test_suite vs
// normal fgfs executable.
//
// Copyright (C) 2020 James Turner
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

#include <Main/globals.hxx>
#include <Main/util.hxx>

#include <Scripting/NasalSys.hxx>
#include <Scripting/NasalSys_private.hxx>

#include <cppunit/TestAssert.h>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

using std::string;

static CppUnit::SourceLine nasalSourceLine(const nasal::CallContext& ctx)
{
    const string fileName = ctx.from_nasal<string>(naGetSourceFile(ctx.c_ctx(), 0));
    const int lineNumber = naGetLine(ctx.c_ctx(), 0);
    return CppUnit::SourceLine(fileName, lineNumber);
}

static naRef f_assert(const nasal::CallContext& ctx )
{
    bool pass = ctx.requireArg<bool>(0);
    auto msg = ctx.getArg<string>(1, "assert failed:");

    CppUnit::Asserter::failIf(!pass, "assertion failed:" + msg, nasalSourceLine(ctx));
    return naNil();
}

static naRef f_fail(const nasal::CallContext& ctx )
{
    auto msg = ctx.getArg<string>(0);

    CppUnit::Asserter::fail("assertion failed:" + msg,
                            nasalSourceLine(ctx));
    return naNil();
}

static naRef f_assert_equal(const nasal::CallContext& ctx )
{
    naRef argA = ctx.requireArg<naRef>(0);
    naRef argB = ctx.requireArg<naRef>(1);
    auto msg = ctx.getArg<string>(2, "assert_equal failed");

    bool same = nasalStructEqual(ctx.c_ctx(), argA, argB);
    if (!same) {

        string aStr = ctx.from_nasal<string>(argA);
        string bStr = ctx.from_nasal<string>(argB);
        msg += "; expected:" + aStr + ", actual:" + bStr;

        CppUnit::Asserter::fail(msg, nasalSourceLine(ctx));
    }
    
    return naNil();
}

static naRef f_equal(const nasal::CallContext& ctx)
{
    naRef argA = ctx.requireArg<naRef>(0);
    naRef argB = ctx.requireArg<naRef>(1);

    bool same = nasalStructEqual(ctx.c_ctx(), argA, argB);
    return naNum(same);
}

static naRef f_assert_doubles_equal(const nasal::CallContext& ctx )
{
    double argA = ctx.requireArg<double>(0);
    double argB = ctx.requireArg<double>(1);
    double tolerance = ctx.requireArg<double>(2);

    auto msg = ctx.getArg<string>(3, "assert_doubles_equal failed");

    const bool same = fabs(argA - argB) < tolerance;
    if (!same) {
        msg += "; expected:" + std::to_string(argA) + ", actual:" + std::to_string(argB);
        CppUnit::Asserter::fail(msg, nasalSourceLine(ctx));
    }
    
    return naNil();
}

//------------------------------------------------------------------------------
naRef initNasalUnitTestCppUnit(naRef nasalGlobals, naContext c)
{
    nasal::Hash globals_module(nasalGlobals, c),
            unitTest = globals_module.createHash("unitTest");

    unitTest.set("assert", f_assert);
    unitTest.set("fail", f_fail);
    unitTest.set("assert_equal", f_assert_equal);
    unitTest.set("equal", f_equal);
    unitTest.set("assert_doubles_equal", f_assert_doubles_equal);
    
  return naNil();
}

void shutdownNasalUnitTestInSim()
{
    // stub
}