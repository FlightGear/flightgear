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



////////////////////////////////////////////////////////////////////////
// General binding support.
////////////////////////////////////////////////////////////////////////


/**
 * An input binding of some sort.
 *
 * <p>This class represents a binding that can be assigned to a
 * keyboard key, a joystick button or axis, or even a panel
 * instrument.</p>
 */
class FGBinding : public FGConditional
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
   * Fire a binding with a scaled movement (rather than absolute position).
   */
  virtual void fire (double offset, double max) const;


  /**
   * Fire a binding with a setting (i.e. joystick axis).
   *
   * A double 'setting' property will be added to the arguments.
   *
   * @param setting The input setting, usually between -1.0 and 1.0.
   */
  virtual void fire (double setting) const;


private:
  string _command_name;
  SGCommandMgr::command_t _command;
  mutable SGPropertyNode * _arg;
  mutable SGPropertyNode * _setting;
  mutable SGCommandState * _command_state;
};



////////////////////////////////////////////////////////////////////////
// General input mapping support.
////////////////////////////////////////////////////////////////////////


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


  /**
   * Default constructor.
   */
  FGInput ();


  /**
   * Destructor.
   */
  virtual ~FGInput();

  //
  // Implementation of FGSubsystem.
  //
  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (double dt);


  /**
   * Control whether this is the default module to receive events.
   *
   * The first input module created will set itself as the default
   * automatically.
   *
   * @param status true if this should be the default module for
   * events, false otherwise.
   */
  virtual void makeDefault (bool status = true);


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
   * Handle a mouse click event.
   *
   * @param button The mouse button selected.
   * @param updown Button status.
   * @param x The X position of the mouse event, in screen coordinates.
   * @param y The Y position of the mouse event, in screen coordinates.
   */
  virtual void doMouseClick (int button, int updown, int x, int y);


  /**
   * Handle mouse motion.
   *
   * @param x The new mouse x position, in screen coordinates.
   * @param y The new mouse y position, in screen coordinates.
   */
  virtual void doMouseMotion (int x, int y);


private:
				// Constants
  enum 
  {
    MAX_KEYS = 1024,

  #ifdef WIN32
    MAX_JOYSTICKS = 2,
  #else
    MAX_JOYSTICKS = 10,
  #endif
    MAX_JOYSTICK_AXES = _JS_MAX_AXES,
    MAX_JOYSTICK_BUTTONS = 32,

    MAX_MICE = 1,
    MAX_MOUSE_BUTTONS = 8
  };
  struct mouse;
  friend struct mouse;

  typedef vector<FGBinding *> binding_list_t;

  /**
   * Settings for a key or button.
   */
  struct button {
    button ();
    virtual ~button ();
    bool is_repeatable;
    int last_state;
    binding_list_t bindings[FG_MOD_MAX];
  };


  /**
   * Settings for a single joystick axis.
   */
  struct axis {
    axis ();
    virtual ~axis ();
    float last_value;
    float tolerance;
    binding_list_t bindings[FG_MOD_MAX];
    float low_threshold;
    float high_threshold;
    struct button low;
    struct button high;
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
    button * buttons;
  };


  /**
   * Settings for a mouse mode.
   */
  struct mouse_mode {
    mouse_mode ();
    virtual ~mouse_mode ();
    int cursor;
    bool constrained;
    bool pass_through;
    button * buttons;
    binding_list_t x_bindings[FG_MOD_MAX];
    binding_list_t y_bindings[FG_MOD_MAX];
  };


  /**
   * Settings for a mouse.
   */
  struct mouse {
    mouse ();
    virtual ~mouse ();
    int x;
    int y;
    SGPropertyNode * mode_node;
    SGPropertyNode * mouse_button_nodes[MAX_MOUSE_BUTTONS];
    int nModes;
    int current_mode;
    mouse_mode * modes;
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
   * Initialize mouse bindings.
   */
  void _init_mouse ();


  /**
   * Initialize a single button.
   */
  inline void _init_button (const SGPropertyNode * node,
			    button &b,
			    const string name);


  /**
   * Update the keyboard.
   */
  void _update_keyboard ();


  /**
   * Update the joystick.
   */
  void _update_joystick ();


  /**
   * Update the mouse.
   */
  void _update_mouse ();


  /**
   * Update a single button.
   */
  inline void _update_button (button &b, int modifiers, bool pressed,
			      int x, int y);


  /**
   * Read bindings and modifiers.
   */
  void _read_bindings (const SGPropertyNode * node,
		       binding_list_t * binding_list,
		       int modifiers);

  /**
   * Look up the bindings for a key code.
   */
  const vector<FGBinding *> &_find_key_bindings (unsigned int k,
						 int modifiers);

  button _key_bindings[MAX_KEYS];
  joystick _joystick_bindings[MAX_JOYSTICKS];
  mouse _mouse_bindings[MAX_MICE];

};



////////////////////////////////////////////////////////////////////////
// GLUT callbacks.
////////////////////////////////////////////////////////////////////////

// Handle GLUT events.
extern "C" {

/**
 * Key-down event handler for Glut.
 *
 * <p>Pass the value on to the FGInput module unless PUI wants it.</p>
 *
 * @param k The integer value for the key pressed.
 * @param x (unused)
 * @param y (unused)
 */
void GLUTkey (unsigned char k, int x, int y);


/**
 * Key-up event handler for GLUT.
 *
 * <p>PUI doesn't use this, so always pass it to the input manager.</p>
 *
 * @param k The integer value for the key pressed.
 * @param x (unused)
 * @param y (unused)
 */
void GLUTkeyup (unsigned char k, int x, int y);


/**
 * Special key-down handler for Glut.
 *
 * <p>Pass the value on to the FGInput module unless PUI wants it.
 * The key value will have 256 added to it.</p>
 *
 * @param k The integer value for the key pressed (will have 256 added
 * to it).
 * @param x (unused)
 * @param y (unused)
 */
void GLUTspecialkey (int k, int x, int y);


/**
 * Special key-up handler for Glut.
 *
 * @param k The integer value for the key pressed (will have 256 added
 * to it).
 * @param x (unused)
 * @param y (unused)
 */
void GLUTspecialkeyup (int k, int x, int y);


/**
 * Mouse click handler for Glut.
 *
 * @param button The mouse button pressed.
 * @param updown Press or release flag.
 * @param x The x-location of the click.
 * @param y The y-location of the click.
 */
void GLUTmouse (int button, int updown, int x, int y);


/**
 * Mouse motion handler for Glut.
 *
 * @param x The new x-location of the mouse.
 * @param y The new y-location of the mouse.
 */
void GLUTmotion (int x, int y);

} // extern "C"

#endif // _INPUT_HXX
