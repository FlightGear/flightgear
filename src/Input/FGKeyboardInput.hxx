// FGKeyboardInput.hxx -- handle user input from keyboard devices
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

#ifndef _FGKEYBOARDINPUT_HXX
#define _FGKEYBOARDINPUT_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include "FGCommonInput.hxx"
#include "FGButton.hxx"
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>

////////////////////////////////////////////////////////////////////////
// The Keyboard Input Class
////////////////////////////////////////////////////////////////////////
class FGKeyboardInput : public SGSubsystem,FGCommonInput {
public:
  FGKeyboardInput();
  virtual ~FGKeyboardInput();

  virtual void init();
  virtual void postinit();
  virtual void bind();
  virtual void unbind();
  virtual void update( double dt );

  static const int MAX_KEYS = 1024;

private:
  const binding_list_t& _find_key_bindings (unsigned int k, int modifiers);
  void doKey (int k, int modifiers, int x, int y);

  static void keyHandler(int key, int keymod, int mousex, int mousey);
  static FGKeyboardInput * keyboardInput;
  FGButton bindings[MAX_KEYS];
  SGPropertyNode_ptr _key_event;
  int  _key_code;
  int  _key_modifiers;
  bool _key_pressed;
  bool _key_shift;
  bool _key_ctrl;
  bool _key_alt;
  bool _key_meta;
  bool _key_super;
  bool _key_hyper;
  simgear::TiedPropertyList _tiedProperties;
};

#endif
