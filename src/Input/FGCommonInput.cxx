// FGCommonInput.cxx -- common functions for all Input subsystems
//
// Written by Torsten Dreyer, started August 2009
// Based on work from David Megginson, started May 2001.
//
// Copyright (C) 2009 Torsten Dreyer, Torsten (at) t3r _dot_ de
// Copyright (C) 2001 David Megginson, david@megginson.com
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
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "FGCommonInput.hxx"
#include <Main/globals.hxx>
#include <Main/fg_os.hxx>

using simgear::PropertyList;
using std::string;

void FGCommonInput::read_bindings (const SGPropertyNode * node, binding_list_t * binding_list, int modifiers, const string & module )
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Reading all bindings");
  PropertyList bindings = node->getChildren("binding");
  static string nasal = "nasal";
  for (unsigned int i = 0; i < bindings.size(); i++) {
    const char *cmd = bindings[i]->getStringValue("command");
    SG_LOG(SG_INPUT, SG_DEBUG, "Reading binding " << cmd);
    if (nasal.compare(cmd) == 0 && !module.empty())
      bindings[i]->setStringValue("module", module.c_str());
    binding_list[modifiers].push_back(new SGBinding(bindings[i], globals->get_props()));
  }

                                // Read nested bindings for modifiers
  if (node->getChild("mod-up") != 0)
    read_bindings(node->getChild("mod-up"), binding_list,
                   modifiers|KEYMOD_RELEASED, module);

  if (node->getChild("mod-shift") != 0)
    read_bindings(node->getChild("mod-shift"), binding_list,
                   modifiers|KEYMOD_SHIFT, module);

  if (node->getChild("mod-ctrl") != 0)
    read_bindings(node->getChild("mod-ctrl"), binding_list,
                   modifiers|KEYMOD_CTRL, module);

  if (node->getChild("mod-alt") != 0)
    read_bindings(node->getChild("mod-alt"), binding_list,
                   modifiers|KEYMOD_ALT, module);

  if (node->getChild("mod-meta") != 0)
    read_bindings(node->getChild("mod-meta"), binding_list,
                   modifiers|KEYMOD_META, module);

  if (node->getChild("mod-super") != 0)
    read_bindings(node->getChild("mod-super"), binding_list,
                   modifiers|KEYMOD_SUPER, module);

  if (node->getChild("mod-hyper") != 0)
    read_bindings(node->getChild("mod-hyper"), binding_list,
                   modifiers|KEYMOD_HYPER, module);
}

