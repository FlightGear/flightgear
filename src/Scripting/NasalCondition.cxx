// NasalCondition -- expose SGCondition to Nasal
//
// Written by James Turner, started 2012.
//
// Copyright (C) 2012 James Turner
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

#include "NasalCondition.hxx"
#include "NasalSys.hxx"
#include <Main/globals.hxx>

#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/props/condition.hxx>

typedef nasal::Ghost<SGConditionRef> NasalCondition;

//------------------------------------------------------------------------------
static naRef f_createCondition(naContext c, naRef me, int argc, naRef* args)
{
  SGPropertyNode* node = argc > 0
                       ? ghostToPropNode(args[0])
                       : NULL;
  SGPropertyNode* root = argc > 1
                       ? ghostToPropNode(args[1])
                       : globals->get_props();

  if( !node || !root )
    naRuntimeError(c, "createCondition: invalid argument(s)");

  try
  {
    return nasal::to_nasal(c, sgReadCondition(root, node));
  }
  catch(std::exception& ex)
  {
    naRuntimeError(c, "createCondition: %s", ex.what());
  }

  return naNil();
}

//------------------------------------------------------------------------------
naRef initNasalCondition(naRef globals, naContext c)
{
  nasal::Ghost<SGConditionRef>::init("Condition")
    .method("test", &SGCondition::test);

  nasal::Hash(globals, c).set("_createCondition", f_createCondition);

  return naNil();
}
