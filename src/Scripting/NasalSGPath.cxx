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

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
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
/**
 * os.path.new()
 */
static naRef f_new_path(const nasal::CallContext& ctx)
{
  return ctx.to_nasal
  (
    SGPathRef(new SGPath(ctx.getArg<std::string>(0), &checkIORules))
  );
}

/**
 * os.path.desktop()
 */
static naRef f_desktop(const nasal::CallContext& ctx)
{
  return ctx.to_nasal
  (
    SGPathRef(new SGPath(SGPath::desktop(SGPath(&checkIORules))))
  );
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

    .method("create_dir", &SGPath::create_dir)
    .method("remove", &SGPath::remove)
    .method("rename", &SGPath::rename);
 
  nasal::Hash globals_module(globals, c),
              path = globals_module.createHash("os")
                                   .createHash("path");

  path.set("new", f_new_path);
  path.set("desktop", &f_desktop);

  return naNil();
}
