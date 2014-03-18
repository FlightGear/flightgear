// Expose HTTP module to Nasal
//
// Copyright (C) 2013 Thomas Geymayer
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "NasalHTTP.hxx"
#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Network/HTTPClient.hxx>

#include <simgear/io/HTTPFileRequest.hxx>
#include <simgear/io/HTTPMemoryRequest.hxx>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

typedef nasal::Ghost<simgear::HTTP::Request_ptr> NasalRequest;
typedef nasal::Ghost<simgear::HTTP::FileRequestRef> NasalFileRequest;
typedef nasal::Ghost<simgear::HTTP::MemoryRequestRef> NasalMemoryRequest;

FGHTTPClient& requireHTTPClient(naContext c)
{
  FGHTTPClient* http =
    static_cast<FGHTTPClient*>(globals->get_subsystem("http"));
  if( !http )
    naRuntimeError(c, "Failed to get HTTP subsystem");

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
  const std::string validated_path = fgValidatePath(filename, true);

  if( validated_path.empty() )
    naRuntimeError( ctx.c,
                    "Access denied: can not write to %s",
                    filename.c_str() );

  return ctx.to_nasal
  (
    requireHTTPClient(ctx.c).client()->save(url, validated_path)
  );
}

/**
 * http.load(url)
 */
static naRef f_http_load(const nasal::CallContext& ctx)
{
  const std::string url = ctx.requireArg<std::string>(0);
  return ctx.to_nasal( requireHTTPClient(ctx.c).client()->load(url) );
}

//------------------------------------------------------------------------------
naRef initNasalHTTP(naRef globals, naContext c)
{
  using simgear::HTTP::Request;
  typedef Request* (Request::*HTTPCallback)(const Request::Callback&);
  NasalRequest::init("http.Request")
    .member("url", &Request::url)
    .member("method", &Request::method)
    .member("scheme", &Request::scheme)
    .member("path", &Request::path)
    .member("host", &Request::host)
    .member("port", &Request::port)
    .member("query", &Request::query)
    .member("status", &Request::responseCode)
    .member("reason", &Request::responseReason)
    .member("readyState", &Request::readyState)
    .method("abort", static_cast<void (Request::*)()>(&Request::abort))
    .method("done", static_cast<HTTPCallback>(&Request::done))
    .method("fail", static_cast<HTTPCallback>(&Request::fail))
    .method("always", static_cast<HTTPCallback>(&Request::always));

  using simgear::HTTP::FileRequest;
  NasalFileRequest::init("http.FileRequest")
    .bases<NasalRequest>();

  using simgear::HTTP::MemoryRequest;
  NasalMemoryRequest::init("http.MemoryRequest")
    .bases<NasalRequest>()
    .member("response", &MemoryRequest::responseBody);
    
  nasal::Hash globals_module(globals, c),
              http = globals_module.createHash("http");

  http.set("save", f_http_save);
  http.set("load", f_http_load);

  return naNil();
}
