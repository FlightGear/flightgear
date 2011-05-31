// FGJoystickInput.hxx -- handle user input from joystick devices
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

#ifndef _FGJOYSTICKINPUT_HXX
#define _FGJOYSTICKINPUT_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include "FGCommonInput.hxx"
#include "FGButton.hxx"
#include <simgear/structure/subsystem_mgr.hxx>
#include <plib/js.h>

////////////////////////////////////////////////////////////////////////
// The Joystick Input Class
////////////////////////////////////////////////////////////////////////
class FGJoystickInput : public SGSubsystem,FGCommonInput {
public:
  FGJoystickInput();
  virtual ~FGJoystickInput();

  virtual void init();
  virtual void postinit();
  virtual void reinit();
  virtual void update( double dt );

  static const int MAX_JOYSTICKS        = 10;
  static const int MAX_JOYSTICK_AXES    = _JS_MAX_AXES;
  static const int MAX_JOYSTICK_BUTTONS = 32;

private:
   void _remove(bool all);

  /**
   * Settings for a single joystick axis.
   */
  struct axis {
    axis ();
    virtual ~axis ();
    float last_value;
    float tolerance;
    binding_list_t bindings[KEYMOD_MAX];
    float low_threshold;
    float high_threshold;
    FGButton low;
    FGButton high;
    float interval_sec;
    double last_dt;
  };

  /**
   * Settings for a joystick.
   */
  struct joystick {
    joystick ();
    virtual ~joystick ();
    int jsnum;
    jsJoystick * js;
    int naxes;
    int nbuttons;
    axis * axes;
    FGButton * buttons;
    bool predefined;
  };
  joystick bindings[MAX_JOYSTICKS];

};

#endif
