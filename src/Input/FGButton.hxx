// FGButton.hxx -- a simple button/key wrapper class
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

#ifndef FGBUTTON_H
#define FGBUTTON_H

#include "FGCommonInput.hxx"
#include <Main/fg_os.hxx>

class FGButton : public FGCommonInput {
public:
  FGButton();
  virtual ~FGButton();
  void init( const SGPropertyNode * node, const std::string & name, std::string & module );
  void update( int modifiers, bool pressed, int x = -1, int y = -1);
  bool is_repeatable;
  float interval_sec;
  float last_dt;
  int last_state;
  binding_list_t bindings[KEYMOD_MAX];
};

#endif
