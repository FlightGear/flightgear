// input.cxx -- handle user input from various sources.
//
// Written by David Megginson, started May 2001.
// Major redesign by Torsten Dreyer, started August 2009
//
// Copyright (C) 2001 David Megginson, david@megginson.com
// Copyright (C) 2009 Torsten Dreyer, Torsten (at) t3r _dot_ de
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

#include "input.hxx"

#include <simgear/compiler.h>

#include <Main/fg_props.hxx>
#include "FGMouseInput.hxx"
#include "FGKeyboardInput.hxx"
#include "FGJoystickInput.hxx"

#ifdef WITH_EVENTINPUT
#if defined( SG_WINDOWS )
//to be developed
//#include "FGDirectXEventInput.hxx"
//#define INPUTEVENT_CLASS FGDirectXEventInput
#elif defined ( SG_MAC )
#include "FGMacOSXEventInput.hxx"
#define INPUTEVENT_CLASS FGMacOSXEventInput
#else
#include "FGLinuxEventInput.hxx"
#define INPUTEVENT_CLASS FGLinuxEventInput
#endif

#endif

////////////////////////////////////////////////////////////////////////
// Implementation of FGInput.
////////////////////////////////////////////////////////////////////////


FGInput::FGInput ()
{
  if( fgGetBool("/sim/input/no-mouse-input",false) ) {
    SG_LOG(SG_INPUT,SG_ALERT,"Mouse input disabled!");
  } else {
    set_subsystem( "input-mouse", new FGMouseInput() );
  }

  if( fgGetBool("/sim/input/no-keyboard-input",false) ) {
    SG_LOG(SG_INPUT,SG_ALERT,"Keyboard input disabled!");
  } else {
    set_subsystem( "input-keyboard", new FGKeyboardInput() );
  }

  if( fgGetBool("/sim/input/no-joystick-input",false) ) {
    SG_LOG(SG_INPUT,SG_ALERT,"Joystick input disabled!");
  } else {
    set_subsystem( "input-joystick", new FGJoystickInput() );
  }
#ifdef INPUTEVENT_CLASS
  if( fgGetBool("/sim/input/no-event-input",false) ) {
    SG_LOG(SG_INPUT,SG_ALERT,"Event input disabled!");
  } else {
    set_subsystem( "input-event", new INPUTEVENT_CLASS() );
  }
#endif
}

FGInput::~FGInput ()
{
  // SGSubsystemGroup deletes all subsystem in it's destructor
}

