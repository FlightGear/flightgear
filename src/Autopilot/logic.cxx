// logic.cxx - Base class for logic components
//
// Written by Torsten Dreyer
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
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

// for some obscure reason, MSVC needs this to compile
#ifdef _MSC_VER
#ifndef HAVE_CONFIG_H
#  include <config.h>
#endif
#endif

#include "logic.hxx"

using namespace FGXMLAutopilot;

bool Logic::get_input() const
{
  // return state of first configured condition
  InputMap::const_iterator it = _input.begin();
  if( it == _input.end() ) return false; // no inputs?
  return (*it).second->test();
}

void Logic::set_output( bool value )
{
  // respect global inverted flag
  if( _inverted ) value = !value;

  // set all outputs to the given value
  for( OutputMap::iterator it = _output.begin(); it != _output.end(); ++it )
    (*it).second->setValue( value );
}

bool Logic::get_output() const
{
  OutputMap::const_iterator it = _output.begin();
  bool q = it != _output.end() ? (*it).second->getValue() : false;
  return _inverted ? !q : q;
}

void Logic::update( bool firstTime, double dt )
{
  set_output( get_input() );
}

