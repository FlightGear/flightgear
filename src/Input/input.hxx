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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _INPUT_HXX
#define _INPUT_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif

#include <plib/js.h>
#include <plib/ul.h>

#include <simgear/compiler.h>

#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>

#include <Main/fg_os.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include <map>
#include <vector>

SG_USING_STD(map);
SG_USING_STD(vector);




#if defined( UL_WIN32 )
#define TGT_PLATFORM	"windows"
#elif defined ( UL_MAC_OSX )
#define TGT_PLATFORM    "mac"
#else
#define TGT_PLATFORM	"unix"
#endif



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
class FGBinding : public SGConditional
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
                                // just to be safe.
  FGBinding (const FGBinding &binding);

  string _command_name;
  mutable SGCommandMgr::command_t _command;
  mutable SGPropertyNode_ptr _arg;
  mutable SGPropertyNode_ptr _setting;
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
class FGInput : public SGSubsystem
{
public:
  /**
   * Default constructor.
   */
  FGInput ();

  /**
   * Destructor.
   */
  virtual ~FGInput();

  //
  // Implementation of SGSubsystem.
  //
  virtual void init ();
  virtual void reinit ();
  virtual void postinit ();
  virtual void bind ();
  virtual void unbind ();
  virtual void update (double dt);
  virtual void suspend ();
  virtual void resume ();
  virtual bool is_suspended () const;

virtual void printTimingInformation();

     void stamp(string name);


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
   * @param k The integer key code, see Main/fg_os.hxx
   * @param modifiers Modifier keys pressed (bitfield).
   * @param x The mouse x position at the time of keypress.
   * @param y The mouse y position at the time of keypress.
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

   eventTimeVec timingInfo;
				// Constants
  enum 
  {
    MAX_KEYS = 1024,
    MAX_JOYSTICKS = 10,
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
    float interval_sec;
    float last_dt;
    int last_state;
    binding_list_t bindings[KEYMOD_MAX];
  };


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
    struct button low;
    struct button high;
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
    binding_list_t x_bindings[KEYMOD_MAX];
    binding_list_t y_bindings[KEYMOD_MAX];
  };


  /**
   * Settings for a mouse.
   */
  struct mouse {
    mouse ();
    virtual ~mouse ();
    int x;
    int y;
    SGPropertyNode_ptr mode_node;
    SGPropertyNode_ptr mouse_button_nodes[MAX_MOUSE_BUTTONS];
    int nModes;
    int current_mode;
    double timeout;
    int save_x;
    int save_y;
    mouse_mode * modes;
  };


  /**
   * Initialize joystick bindings.
   */
  void _init_joystick ();


  /**
   * Scan directory recursively for "named joystick" configuration files,
   * and read them into /input/joysticks/js-named[index]++.
   */
  void _scan_joystick_dir (SGPath *path, SGPropertyNode* node, int *index);


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
   * Initialize key bindings, as well as those joystick parts that
   * depend on a working Nasal subsystem.
   */
  void _postinit_keyboard ();
  void _postinit_joystick ();

  /**
   * Update the keyboard.
   */
  void _update_keyboard ();


  /**
   * Update the joystick.
   */
  void _update_joystick (double dt);


  /**
   * Update the mouse.
   */
  void _update_mouse (double dt);


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

  /**
   * Nasal module name/namespace.
   */
  string _module;
};

#endif // _INPUT_HXX
