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

#include <string.h>

#include "NasalCondition.hxx"
#include "NasalSys.hxx"
#include <Main/globals.hxx>

#include <simgear/sg_inlines.h>
#include <simgear/props/condition.hxx>

static void conditionGhostDestroy(void* g);

naGhostType ConditionGhostType = { conditionGhostDestroy, "condition" };

static void hashset(naContext c, naRef hash, const char* key, naRef val)
{
  naRef s = naNewString(c);
  naStr_fromdata(s, (char*)key, strlen(key));
  naHash_set(hash, s, val);
}

SGCondition* conditionGhost(naRef r)
{
  if ((naGhost_type(r) == &ConditionGhostType))
  {
    return (SGCondition*) naGhost_ptr(r);
  }
  
  return 0;
}

static void conditionGhostDestroy(void* g)
{
  SGCondition* cond = (SGCondition*)g;
  if (!SGCondition::put(cond)) // unref
    delete cond;
}

static naRef conditionPrototype;

naRef ghostForCondition(naContext c, const SGCondition* cond)
{
  if (!cond) {
    return naNil();
  }
  
  SGCondition::get(cond); // take a ref
  return naNewGhost(c, &ConditionGhostType, (void*) cond);
}

static naRef f_condition_test(naContext c, naRef me, int argc, naRef* args)
{
  SGCondition* cond = conditionGhost(me);
  if (!cond) {
    naRuntimeError(c, "condition.test called on non-condition object");
  }
  
  return naNum(cond->test());
}

static naRef f_createCondition(naContext c, naRef me, int argc, naRef* args)
{
  SGPropertyNode* node = ghostToPropNode(args[0]);
  SGPropertyNode* root = globals->get_props();
  if (argc > 1) {
    root = ghostToPropNode(args[1]);
  }
  
  SGCondition* cond = sgReadCondition(root, node);
  return ghostForCondition(c, cond);
}

// Table of extension functions.  Terminate with zeros.
static struct { const char* name; naCFunction func; } funcs[] = {
  { "_createCondition", f_createCondition },
  { 0, 0 }
};


naRef initNasalCondition(naRef globals, naContext c)
{
  conditionPrototype = naNewHash(c);
  naSave(c, conditionPrototype);
  
  hashset(c, conditionPrototype, "test", naNewFunc(c, naNewCCode(c, f_condition_test)));  
  for(int i=0; funcs[i].name; i++) {
    hashset(c, globals, funcs[i].name,
            naNewFunc(c, naNewCCode(c, funcs[i].func)));
  }
  
  return naNil();
}
