// input.hxx -- handle user input from various sources.
//
// Written by David Megginson, started May 2001.
//
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _INPUT_HXX
#define _INPUT_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <simgear/misc/props.hxx>

#include <Main/fgfs.hxx>
#include <Main/globals.hxx>

#include <map>
#include <vector>

SG_USING_STD(map);
SG_USING_STD(vector);

/**
 * An input binding of some sort.
 *
 * <p>This class represents a binding that can be assigned to a
 * keyboard key, a joystick button or axis, or even a panel
 * instrument.</p>
 */
class FGBinding
{
public:

  enum Action {
    ACTION_NONE,
    ACTION_SWITCH,
    ACTION_ADJUST,
    ACTION_ASSIGN
  };

  FGBinding ();
  FGBinding (const SGPropertyNode * node);
  virtual ~FGBinding ();

  virtual Action getAction () const { return _action; }
  virtual SGPropertyNode * getProperty () { return _node; }
  virtual const SGPropertyNode * getProperty () const { return _node; }
  virtual const SGValue * getAdjustStep () const { return _adjust_step; }
  virtual const SGValue * getAssignValue () const { return _assign_value; }
  
  virtual void setAction (Action action);
  virtual void setProperty (SGPropertyNode * node);
  virtual void setAdjustStep (const SGValue * step);
  virtual void setAssignValue (const SGValue * value);

  virtual void read (const SGPropertyNode * node);

  virtual void fire () const;
//   virtual void fire (double value);
//   virtual void fire (int xdelta, int ydelta);

private:
  Action _action;
  SGPropertyNode * _node;
  const SGValue * _adjust_step;
  const SGValue * _assign_value;
};


/**
 * Generic input module.
 *
 * <p>This module is designed to handle input from multiple sources --
 * keyboard, joystick, mouse, or even panel switches -- in a consistent
 * way, and to allow users to rebind any of the actions at runtime.</p>
 */
class FGInput : public FGSubsystem
{
public:

  enum {
    FG_MOD_NONE = 0,
    FG_MOD_SHIFT = 1,
    FG_MOD_CTRL = 2,
    FG_MOD_ALT = 4,
    FG_MOD_MAX = 8			// one past all modifiers
  };

  FGInput();
  virtual ~FGInput();

  //
  // Implementation of FGSubsystem.
  //
  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update ();


  /**
   * Handle a single keystroke.
   *
   * <p>Note: for special keys, the integer key code will be the Glut
   * code + 256.</p>
   *
   * @param k The integer key code, as returned by glut.
   * @param modifiers Modifier keys pressed (bitfield).
   * @param x The mouse x position at the time of keypress.
   * @param y The mouse y position at the time of keypress.
   * @see #FG_MOD_SHIFT
   * @see #FG_MOD_CTRL
   * @see #FG_MOD_ALT
   */
  virtual void doKey (int k, int modifiers, int x, int y);


  /**
   * Fire off a single-trigger action.
   *
   * <p>This method fires an action triggered by a key or button
   * press, with no additional quantity information.</p> 
   *
   * @param binding The property node with the binding.
   */
  virtual void action (const SGPropertyNode * binding);


  /**
   * Fire off a quantity action.
   *
   * <p>This method fires an action triggered by a change in value,
   * such as a slider or axis.</p>
   *
   * @param action The property node with the binding.
   * @param value The new value.
   */
//   virtual void action (const SGPropertyNode * binding, double value);


  /**
   * Fire off a movement action.
   *
   * <p>This method fires an action triggered by relative movement
   * rather than an absolute value; it is especially applicable to
   * mouse-movement bindings.</p>
   *
   * @param binding The property node containing the binding.
   * @param xdelta The amount of X movement.
   * @param ydelta The amount of Y movement.
   */
//   virtual void action (const SGPropertyNode * binding, int xdelta, int ydelta);

private:

  typedef map<int,vector<FGBinding> > keyboard_map;
  keyboard_map _key_bindings[FG_MOD_MAX];

};

extern FGInput current_input;

#endif // _CONTROLS_HXX
