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

#include <plib/js.h>

#include <simgear/compiler.h>

#include <simgear/misc/commands.hxx>
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

  /**
   * Default constructor.
   */
  FGBinding ();


  /**
   * Convenience constructor.
   *
   * @param node The binding will be built from this node.
   */
  FGBinding (const SGPropertyNode * node);


  /**
   * Destructor.
   */
  virtual ~FGBinding ();


  /**
   * Get the command name.
   *
   * @return The string name of the command for this binding.
   */
  virtual const string &getCommandName () const { return _command_name; }


  /**
   * Get the command itself.
   *
   * @return The command associated with this binding, or 0 if none
   * is present.
   */
  virtual SGCommandMgr::command_t getCommand () const { return _command; }


  /**
   * Get the argument that will be passed to the command.
   *
   * @return A property node that will be passed to the command as its
   * argument, or 0 if none was supplied.
   */
  virtual const SGPropertyNode * getArg () { return _arg; }
  

  /**
   * Read a binding from a property node.
   *
   * @param node The property node containing the binding.
   */
  virtual void read (const SGPropertyNode * node);


  /**
   * Fire a binding.
   */
  virtual void fire () const;


  /**
   * Fire a binding with a setting (i.e. joystick axis).
   *
   * A double 'setting' property will be added to the arguments.
   *
   * @param setting The input setting, usually between -1.0 and 1.0.
   */
  virtual void fire (double setting) const;


private:
  void _fire (const SGPropertyNode *arg) const;
  string _command_name;
  SGCommandMgr::command_t _command;
  const SGPropertyNode * _arg;
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
    FG_MOD_UP = 1,		// key- or button-up
    FG_MOD_SHIFT = 2,
    FG_MOD_CTRL = 4,
    FG_MOD_ALT = 8,
    FG_MOD_MAX = 16		// enough to handle all combinations
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


private:

  typedef vector<FGBinding> binding_list_t;

				// Constants
  static const int MAX_KEYS = 1024;
  #ifdef WIN32
  static const int MAX_JOYSTICKS = 2;
  #else
  static const int MAX_JOYSTICKS = 10;
  #endif
  static const int MAX_AXES = _JS_MAX_AXES;
  static const int MAX_BUTTONS = 32;


  /**
   * Settings for a key or button.
   */
  struct button {
    button ()
      : is_repeatable(true),
	last_state(-1)
    {}
    bool is_repeatable;
    int last_state;
    binding_list_t bindings[FG_MOD_MAX];
  };


  /**
   * Settings for a single joystick axis.
   */
  struct axis {
    axis ()
      : last_value(9999999),
	tolerance(0.002)
    {}
    float last_value;
    float tolerance;
    binding_list_t bindings[FG_MOD_MAX];
  };


  /**
   * Settings for a joystick.
   */
  struct joystick {
    virtual ~joystick () {
      delete js;
      delete[] axes;
      delete[] buttons;
    }
    int naxes;
    int nbuttons;
    jsJoystick * js;
    axis * axes;
    button * buttons;
  };


  /**
   * Initialize key bindings.
   */
  void _init_keyboard ();


  /**
   * Initialize joystick bindings.
   */
  void _init_joystick ();


  /**
   * Update the keyboard.
   */
  void _update_keyboard ();


  /**
   * Update the joystick.
   */
  void _update_joystick ();


  /**
   * Read bindings and modifiers.
   */
  void _read_bindings (const SGPropertyNode * node,
		       binding_list_t * binding_list,
		       int modifiers);

  /**
   * Look up the bindings for a key code.
   */
  const vector<FGBinding> &_find_key_bindings (unsigned int k, int modifiers);

  button _key_bindings[MAX_KEYS];
  joystick _joystick_bindings[MAX_JOYSTICKS];

};

extern FGInput current_input;

#endif // _CONTROLS_HXX
