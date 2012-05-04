// FGButton.cxx -- a simple button/key wrapper class
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

#include "FGButton.hxx"


FGButton::FGButton ()
  : is_repeatable(false),
    interval_sec(0),
    last_dt(0),
    last_state(0)
{
}

FGButton::~FGButton ()
{
  // bindings is a list of SGSharedPtr<SGBindings>
  // no cleanup required
}


void FGButton::init( const SGPropertyNode * node, const std::string & name, std::string & module )
{
  if (node == 0) {
    SG_LOG(SG_INPUT, SG_DEBUG, "No bindings for button " << name);
  } else {
    is_repeatable = node->getBoolValue("repeatable", is_repeatable);
    // Get the bindings for the button
    read_bindings( node, bindings, KEYMOD_NONE, module );
  }
}

void FGButton::update( int modifiers, bool pressed, int x, int y)
{
  if (pressed) {
    // The press event may be repeated.
    if (!last_state || is_repeatable) {
      SG_LOG( SG_INPUT, SG_DEBUG, "Button has been pressed" );
      for (unsigned int k = 0; k < bindings[modifiers].size(); k++) {
        bindings[modifiers][k]->fire(x, y);
      }
    }
  } else {
    // The release event is never repeated.
    if (last_state) {
      SG_LOG( SG_INPUT, SG_DEBUG, "Button has been released" );
      for (unsigned int k = 0; k < bindings[modifiers|KEYMOD_RELEASED].size(); k++)
        bindings[modifiers|KEYMOD_RELEASED][k]->fire(x, y);
    }
  }
          
  last_state = pressed;
}  


