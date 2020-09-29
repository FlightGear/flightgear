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

#include <Scripting/NasalUnitTesting.hxx>

#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Scripting/NasalSys.hxx>
#include <Scripting/NasalSys_private.hxx>

#include <Main/fg_commands.hxx>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

#include <simgear/structure/commands.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_dir.hxx>

struct ActiveTest
{
    bool failure = false;
    std::string failureMessage;
    std::string failureFileName;
    int failLineNumber;
};

static std::unique_ptr<ActiveTest> static_activeTest;

static naRef f_assert(const nasal::CallContext& ctx )
{
    bool pass = ctx.requireArg<bool>(0);
    auto msg = ctx.getArg<string>(1);
    
    if (!pass) {
        if (!static_activeTest) {
            ctx.runtimeError("No active test in progress");
        }
        
        if (static_activeTest->failure) {
            ctx.runtimeError("Active test already failed");
        }
        
        static_activeTest->failure = true;
        static_activeTest->failureMessage = msg;
        static_activeTest->failureFileName = ctx.from_nasal<string>(naGetSourceFile(ctx.c_ctx(), 0));
        static_activeTest->failLineNumber = naGetLine(ctx.c_ctx(), 0);

        ctx.runtimeError("Test assert failed");
    }
    
    return naNil();
}

static naRef f_fail(const nasal::CallContext& ctx )
{
    auto msg = ctx.getArg<string>(0);
    
    if (!static_activeTest) {
       ctx.runtimeError("No active test in progress");
   }
   
   if (static_activeTest->failure) {
       ctx.runtimeError("Active test already failed");
   }
   
   static_activeTest->failure = true;
   static_activeTest->failureMessage = msg;
   static_activeTest->failureFileName = ctx.from_nasal<string>(naGetSourceFile(ctx.c_ctx(), 0));
   static_activeTest->failLineNumber = naGetLine(ctx.c_ctx(), 0);

   ctx.runtimeError("Test failed");

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
        static_activeTest->failure = true;
        static_activeTest->failureMessage = msg;
        static_activeTest->failureFileName = ctx.from_nasal<string>(naGetSourceFile(ctx.c_ctx(), 0));
        static_activeTest->failLineNumber = naGetLine(ctx.c_ctx(), 0);
        ctx.runtimeError(msg.c_str());
    }
    
    return naNil();
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
        static_activeTest->failure = true;
        static_activeTest->failureMessage = msg;
        static_activeTest->failureFileName = ctx.from_nasal<string>(naGetSourceFile(ctx.c_ctx(), 0));
        static_activeTest->failLineNumber = naGetLine(ctx.c_ctx(), 0);
        ctx.runtimeError(msg.c_str());
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

//------------------------------------------------------------------------------
// commands

bool command_executeNasalTest(const SGPropertyNode *arg, SGPropertyNode * root)
{
    SGPath p = SGPath::fromUtf8(arg->getStringValue("path"));
    if (p.isRelative()) {
        for (auto dp : globals->get_data_paths("Nasal")) {
            SGPath absPath = dp / p.utf8Str();
            if (absPath.exists()) {
                p = absPath;
                break;
            }
        }
    }
    if (!p.exists() || !p.isFile() || (p.lower_extension() != "nut")) {
        SG_LOG(SG_NASAL, SG_DEV_ALERT, "not a Nasal test file:" << p);
        return false;
    }

    return executeNasalTest(p);
}

bool command_executeNasalTestDir(const SGPropertyNode *arg, SGPropertyNode * root)
{
    SGPath p = SGPath::fromUtf8(arg->getStringValue("path"));
    if (!p.exists() || !p.isDir()) {
        SG_LOG(SG_NASAL, SG_DEV_ALERT, "no such directory:" << p);
        return false;
    }

    executeNasalTestsInDir(p);
    return true;
}

//------------------------------------------------------------------------------
naRef initNasalUnitTestInSim(naRef nasalGlobals, naContext c)
{
  nasal::Hash globals_module(nasalGlobals, c),
              unitTest = globals_module.createHash("unitTest");

    unitTest.set("assert", f_assert);
    unitTest.set("fail", f_fail);
    unitTest.set("assert_equal", f_assert_equal);
    unitTest.set("assert_doubles_equal", f_assert_doubles_equal);
    unitTest.set("equal", f_equal);

    globals->get_commands()->addCommand("nasal-test", &command_executeNasalTest);
    globals->get_commands()->addCommand("nasal-test-dir", &command_executeNasalTestDir);

  return naNil();
}

void executeNasalTestsInDir(const SGPath& path)
{
    simgear::Dir d(path);
    
    for (const auto& testFile : d.children(simgear::Dir::TYPE_FILE, "*.nut")) {
        SG_LOG(SG_NASAL, SG_INFO, "Processing test file " << testFile);
        
    } // of test files iteration
}

// variant on FGNasalSys parse,
static naRef parseTestFile(naContext ctx, const char* filename,
                        const char* buf, int len,
                        std::string& errors)
{
    int errLine = -1;
    naRef srcfile = naNewString(ctx);
    naStr_fromdata(srcfile, (char*)filename, strlen(filename));
    naRef code = naParseCode(ctx, srcfile, 1, (char*)buf, len, &errLine);
    if(naIsNil(code)) {
        std::ostringstream errorMessageStream;
        errorMessageStream << "Nasal Test parse error: " << naGetError(ctx) <<
            " in "<< filename <<", line " << errLine;
        errors = errorMessageStream.str();
        SG_LOG(SG_NASAL, SG_DEV_ALERT, errors);
        return naNil();
    }
    
    const auto nasalSys = globals->get_subsystem<FGNasalSys>();
    return naBindFunction(ctx, code, nasalSys->nasalGlobals());
}



bool executeNasalTest(const SGPath& path)
{
    naContext ctx = naNewContext();
    const auto nasalSys = globals->get_subsystem<FGNasalSys>();
    sg_ifstream file_in(path);
    const auto source = file_in.read_all();
    
    string errors;
    string fileName = path.utf8Str();
    naRef code = parseTestFile(ctx, fileName.c_str(),
                       source.c_str(),
                       source.size(), errors);
    if(naIsNil(code)) {
        naFreeContext(ctx);
        return false;
    }

    // create test context
    
    auto localNS = nasalSys->getGlobals().createHash("_test_" + path.utf8Str());
    nasalSys->callWithContext(ctx, code, 0, 0, localNS.get_naRef());
    
    
    auto setUpFunc = localNS.get("setUp");
    auto tearDown = localNS.get("tearDown");
       
   for (const auto value : localNS) {
       if (value.getKey().find("test_") == 0) {
           static_activeTest.reset(new ActiveTest);
           
           if (naIsFunc(setUpFunc)) {
               nasalSys->callWithContext(ctx, setUpFunc, 0, nullptr ,localNS.get_naRef());
           }

           const auto testName = value.getKey();
           auto testFunc = value.getValue<naRef>();
           if (!naIsFunc(testFunc)) {
               SG_LOG(SG_NAVAID, SG_DEV_WARN, "Skipping non-function test member:" << testName);
               continue;
           }

           nasalSys->callWithContext(ctx, testFunc, 0, nullptr, localNS.get_naRef());
           if (static_activeTest->failure) {
               SG_LOG(SG_NASAL, SG_ALERT, testName << ": Test failure:" << static_activeTest->failureMessage << "\n\tat: " << static_activeTest->failureFileName << ": " << static_activeTest->failLineNumber);
           } else {
               SG_LOG(SG_NASAL, SG_ALERT, testName << ": Test passed");
           }
           
           if (naIsFunc(tearDown)) {
               nasalSys->callWithContext(ctx, tearDown, 0, nullptr ,localNS.get_naRef());
           }
           
           static_activeTest.reset();
       }
   }
    
    // remvoe test hash/namespace

    naFreeContext(ctx);
    return true;
}


void shutdownNasalUnitTestInSim()
{
    globals->get_commands()->removeCommand("nasal-test");
    globals->get_commands()->removeCommand("nasal-test-dir");
}
