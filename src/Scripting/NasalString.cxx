// Add (std::string) like methods to Nasal strings
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
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

#include "NasalString.hxx"

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/NasalString.hxx>

/**
 *  Compare (sub)string with other string
 *
 *  compare(s)
 *  compare(pos, len, s)
 */
static naRef f_compare(naContext c, naRef me, int argc, naRef* args)
{
  nasal::CallContext ctx(c, argc, args);
  nasal::String str = nasal::from_nasal<nasal::String>(c, me),
                rhs = ctx.requireArg<nasal::String>(argc > 1 ? 2 : 0);
  size_t pos = argc > 1 ? ctx.requireArg<int>(1) : 0;
  size_t len = argc > 1 ? ctx.requireArg<int>(2) : 0;

  if( len == 0 )
    len = nasal::String::npos;

  return naNum( str.compare(pos, len, rhs) );
}

/**
 *  Check whether string starts with other string
 */
static naRef f_starts_with(naContext c, naRef me, int argc, naRef* args)
{
  nasal::CallContext ctx(c, argc, args);
  nasal::String str = nasal::from_nasal<nasal::String>(c, me),
                rhs = ctx.requireArg<nasal::String>(0);

  return naNum( str.starts_with(rhs) );
}

/**
 *  Helper to convert size_t position/npos to Nasal conventions (-1 == npos)
 */
naRef pos_to_nasal(size_t pos)
{
  if( pos == nasal::String::npos )
    return naNum(-1);
  else
    return naNum(pos);
}

/**
 *  Find first occurrence of single character
 *
 *  find(c, pos = 0)
 */
static naRef f_find(naContext c, naRef me, int argc, naRef* args)
{
  nasal::CallContext ctx(c, argc, args);
  nasal::String str = nasal::from_nasal<nasal::String>(c, me),
                find = ctx.requireArg<nasal::String>(0);
  size_t pos = ctx.getArg<int>(1, 0);

  if( find.size() != 1 )
    naRuntimeError(c, "string::find: single character expected");

  return pos_to_nasal( str.find(*find.c_str(), pos) );
}

/**
 * Find first character of a string occurring in this string
 *
 * find_first_of(search, pos = 0)
 */
static naRef f_find_first_of(naContext c, naRef me, int argc, naRef* args)
{
  nasal::CallContext ctx(c, argc, args);
  nasal::String str = nasal::from_nasal<nasal::String>(c, me),
                find = ctx.requireArg<nasal::String>(0);
  size_t pos = ctx.getArg<int>(1, 0);

  return pos_to_nasal( str.find_first_of(find, pos) );
}

/**
 * Find first character of this string not occurring in the other string
 *
 * find_first_not_of(search, pos = 0)
 */
static naRef f_find_first_not_of(naContext c, naRef me, int argc, naRef* args)
{
  nasal::CallContext ctx(c, argc, args);
  nasal::String str = nasal::from_nasal<nasal::String>(c, me),
                find = ctx.requireArg<nasal::String>(0);
  size_t pos = ctx.getArg<int>(1, 0);

  return pos_to_nasal( str.find_first_not_of(find, pos) );
}

//------------------------------------------------------------------------------
naRef initNasalString(naRef globals, naRef string, naContext c, naRef gcSave)
{
  nasal::Hash string_module(string, c);

  string_module.set("compare", f_compare);
  string_module.set("starts_with", f_starts_with);
  string_module.set("find", f_find);
  string_module.set("find_first_of", f_find_first_of);
  string_module.set("find_first_not_of", f_find_first_not_of);

  return naNil();
}
