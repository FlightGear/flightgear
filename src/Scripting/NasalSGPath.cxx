// Expose SGPath module to Nasal
//
// Copyright (C) 2013 The FlightGear Community
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

#include "NasalSGPath.hxx"
#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <simgear/misc/sg_path.hxx>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>


typedef boost::shared_ptr<SGPath> SGPathRef;
typedef nasal::Ghost<SGPathRef> NasalSGPath;

SGPath::Permissions checkIORules(const SGPath& path)
{
  SGPath::Permissions perm;

  perm.read  = !fgValidatePath(path.str(), false).empty();
  perm.write = !fgValidatePath(path.str(), true ).empty();

  return perm;
}

// TODO make exposing such function easier...
static naRef validatedPathToNasal( const nasal::CallContext& ctx,
                                   const SGPath& p )
{
  return ctx.to_nasal( SGPathRef(new SGPath(p.str(), &checkIORules)) );
}

/**
 * os.path.new()
 */
static naRef f_new_path(const nasal::CallContext& ctx)
{
  return validatedPathToNasal(ctx, SGPath(ctx.getArg<std::string>(0)));
}

static int f_path_create_dir(SGPath& p, const nasal::CallContext& ctx)
{
  // limit setable access rights for Nasal
  return p.create_dir(ctx.getArg<mode_t>(0, 0755) & 0775);
}

/**
 * os.path.desktop()
 */
static naRef f_desktop(const nasal::CallContext& ctx)
{
  return validatedPathToNasal(ctx, SGPath::desktop(SGPath(&checkIORules)));
}

/**
 * os.path.standardLocation(type)
 */
static naRef f_standardLocation(const nasal::CallContext& ctx)
{
  const std::string type_str = ctx.requireArg<std::string>(0);
  SGPath::StandardLocation type = SGPath::HOME;
  if(      type_str == "DESKTOP" )
    type = SGPath::DESKTOP;
  else if( type_str == "DOWNLOADS" )
    type = SGPath::DOWNLOADS;
  else if( type_str == "DOCUMENTS" )
    type = SGPath::DOCUMENTS;
  else if( type_str == "PICTURES" )
    type = SGPath::PICTURES;
  else if( type_str != "HOME" )
    naRuntimeError
    (
      ctx.c,
      "os.path.standardLocation: unknown type %s", type_str.c_str()
    );

  return validatedPathToNasal(ctx, SGPath::standardLocation(type));
}

//------------------------------------------------------------------------------
naRef initNasalSGPath(naRef globals, naContext c)
{
  // This wraps most of the SGPath APIs for use by Nasal
  // See: http://docs.freeflightsim.org/simgear/classSGPath.html

  NasalSGPath::init("os.path")
    .method("set", &SGPath::set)
    .method("append", &SGPath::append)
    .method("add", &SGPath::add)
    .method("concat", &SGPath::concat)

    .member("realpath", &SGPath::realpath)
    .member("file", &SGPath::file)
    .member("dir", &SGPath::dir)
    .member("base", &SGPath::base)
    .member("file_base", &SGPath::file_base)
    .member("extension", &SGPath::extension)
    .member("lower_extension", &SGPath::lower_extension)
    .member("complete_lower_extension", &SGPath::complete_lower_extension)
    .member("str", &SGPath::str)
    .member("str_native", &SGPath::str_native)
    .member("mtime", &SGPath::modTime)

    .method("exists", &SGPath::exists)
    .method("canRead", &SGPath::canRead)
    .method("canWrite", &SGPath::canWrite)
    .method("isFile", &SGPath::isFile)
    .method("isDir", &SGPath::isDir)
    .method("isRelative", &SGPath::isRelative)
    .method("isAbsolute", &SGPath::isAbsolute)
    .method("isNull", &SGPath::isNull)

    .method("create_dir", &f_path_create_dir)
    .method("remove", &SGPath::remove)
    .method("rename", &SGPath::rename);

  nasal::Hash globals_module(globals, c),
              path = globals_module.createHash("os")
                                   .createHash("path");

  path.set("new", f_new_path);
  path.set("desktop", &f_desktop);
  path.set("standardLocation", &f_standardLocation);

  return naNil();
}
