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

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

#if 0
typedef nasal::Ghost<simgear::HTTP::Request_ptr> NasalRequest;
typedef nasal::Ghost<simgear::HTTP::FileRequestRef> NasalFileRequest;
typedef nasal::Ghost<simgear::HTTP::MemoryRequestRef> NasalMemoryRequest;

FGHTTPClient& requireHTTPClient(const nasal::ContextWrapper& ctx)
{
  FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();
  if( !http )
    ctx.runtimeError("Failed to get HTTP subsystem");

  return *http;
}

/**
 * http.save(url, filename)
 */
static naRef f_http_save(const nasal::CallContext& ctx)
{
  const std::string url = ctx.requireArg<std::string>(0);

  // Check for write access to target file
  const std::string filename = ctx.requireArg<std::string>(1);
  const SGPath validated_path = fgValidatePath(filename, true);

  if( validated_path.isNull() )
    ctx.runtimeError("Access denied: can not write to %s", filename.c_str());

  return ctx.to_nasal
  (
    requireHTTPClient(ctx).client()->save(url, validated_path.utf8Str())
  );
}

/**
 * http.load(url)
 */
static naRef f_http_load(const nasal::CallContext& ctx)
{
  const std::string url = ctx.requireArg<std::string>(0);
  return ctx.to_nasal( requireHTTPClient(ctx).client()->load(url) );
}

static naRef f_request_abort( simgear::HTTP::Request&,
                              const nasal::CallContext& ctx )
{
    // we need a request_ptr for cancel, not a reference. So extract
    // the me object from the context directly.
    simgear::HTTP::Request_ptr req = ctx.from_nasal<simgear::HTTP::Request_ptr>(ctx.me);
    requireHTTPClient(ctx).client()->cancelRequest(req);
    return naNil();
}

#endif

//------------------------------------------------------------------------------
naRef initNasalUnitTestCppUnit(naRef globals, naContext c)
{
 

  nasal::Hash globals_module(globals, c),
              unitTest = globals_module.createHash("unitTest");

//  http.set("save", f_http_save);
//  http.set("load", f_http_load);

  return naNil();
}
