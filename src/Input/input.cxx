// input.cxx -- handle user input from various sources.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <simgear/compiler.h>

#include <math.h>
#include <ctype.h>

#include STL_FSTREAM
#include STL_STRING
#include <vector>

#include <GL/glut.h>

#include <plib/pu.h>

#include <simgear/compiler.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/props.hxx>

#include <Aircraft/aircraft.hxx>
#include <Autopilot/auto_gui.hxx>
#include <Autopilot/newauto.hxx>
#include <Cockpit/hud.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include <GUI/gui.h>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "input.hxx"

#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(ifstream);
#endif
SG_USING_STD(string);
SG_USING_STD(vector);



////////////////////////////////////////////////////////////////////////
// Local variables.
////////////////////////////////////////////////////////////////////////

static FGInput * default_input = 0;



////////////////////////////////////////////////////////////////////////
// Implementation of FGBinding.
////////////////////////////////////////////////////////////////////////

FGBinding::FGBinding ()
  : _command(0),
    _arg(new SGPropertyNode),
    _setting(0)
{
}

FGBinding::FGBinding (const SGPropertyNode * node)
  : _command(0),
    _arg(new SGPropertyNode),
    _setting(0)
{
  read(node);
}

FGBinding::~FGBinding ()
{
//   delete _arg;			// Delete the saved arguments
//   delete _command_state;	// Delete the saved command state
}

void
FGBinding::read (const SGPropertyNode * node)
{
  const SGPropertyNode * conditionNode = node->getChild("condition");
  if (conditionNode != 0) {
    cerr << "Adding condition to binding" << endl;
    setCondition(fgReadCondition(conditionNode));
  }

  _command_name = node->getStringValue("command", "");
  if (_command_name.empty()) {
    SG_LOG(SG_INPUT, SG_WARN, "No command supplied for binding.");
    _command = 0;
    return;
  }

  _command = globals->get_commands()->getCommand(_command_name);
  if (_command == 0) {
    SG_LOG(SG_INPUT, SG_ALERT, "Command " << _command_name << " is undefined");
    _arg = 0;
    return;
  }

  delete _arg;
  _arg = new SGPropertyNode;
  _setting = 0;
  copyProperties(node, _arg);  // FIXME: don't use whole node!!!
}

void
FGBinding::fire () const
{
  if (test()) {
    if (_command == 0) {
      SG_LOG(SG_INPUT, SG_WARN, "No command attached to binding");
    } else if (!(*_command)(_arg)) {
      SG_LOG(SG_INPUT, SG_ALERT, "Failed to execute command "
	     << _command_name);
    }
  }
}

void
FGBinding::fire (double offset, double max) const
{
  if (test()) {
    _arg->setDoubleValue("offset", offset/max);
    fire();
  }
}

void
FGBinding::fire (double setting) const
{
  if (test()) {
				// A value is automatically added to
				// the args
    if (_setting == 0)		// save the setting node for efficiency
      _setting = _arg->getChild("setting", 0, true);
    _setting->setDoubleValue(setting);
    fire();
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput.
////////////////////////////////////////////////////////////////////////

				// From main.cxx
extern void fgReshape( int width, int height );


FGInput::FGInput ()
{
    if (default_input == 0)
        default_input = this;
}

FGInput::~FGInput ()
{
    if (default_input == this)
        default_input = 0;
}

void
FGInput::init ()
{
  _init_keyboard();
  _init_joystick();
  _init_mouse();

  glutKeyboardFunc(GLUTkey);
  glutKeyboardUpFunc(GLUTkeyup);
  glutSpecialFunc(GLUTspecialkey);
  glutSpecialUpFunc(GLUTspecialkeyup);
  glutMouseFunc (GLUTmouse);
  glutMotionFunc (GLUTmotion);
  glutPassiveMotionFunc (GLUTmotion);
}

void
FGInput::bind ()
{
  // no op
}

void
FGInput::unbind ()
{
  // no op
}

void 
FGInput::update (double dt)
{
  _update_keyboard();
  _update_joystick();
  _update_mouse();
}

void
FGInput::makeDefault (bool status)
{
    if (status)
        default_input = this;
    else if (default_input == this)
        default_input = 0;
}

void
FGInput::doKey (int k, int modifiers, int x, int y)
{
  SG_LOG( SG_INPUT, SG_DEBUG, "User pressed key " << k
	  << " with modifiers " << modifiers );

				// Sanity check.
  if (k < 0 || k >= MAX_KEYS) {
    SG_LOG(SG_INPUT, SG_WARN, "Key value " << k << " out of range");
    return;
  }

  button &b = _key_bindings[k];

				// Key pressed.
  if (modifiers&FG_MOD_UP == 0) {
    SG_LOG( SG_INPUT, SG_DEBUG, "User pressed key " << k
	    << " with modifiers " << modifiers );
    if (!b.last_state || b.is_repeatable) {
      const binding_list_t &bindings =
	_find_key_bindings(k, modifiers);
      int max = bindings.size();
      if (max > 0) {
	for (int i = 0; i < max; i++)
	  bindings[i]->fire();
	return;
      }
    }
  }

				// Key released.
  else {
    SG_LOG(SG_INPUT, SG_DEBUG, "User released key " << k
	   << " with modifiers " << modifiers);
    if (b.last_state) {
      const binding_list_t &bindings =
	_find_key_bindings(k, modifiers);
      int max = bindings.size();
      if (max > 0) {
	for (int i = 0; i < max; i++)
	  bindings[i]->fire();
	return;
      }
    }
  }


				// Use the old, default actions.
  SG_LOG( SG_INPUT, SG_DEBUG, "(No user binding.)" );
  if (modifiers&FG_MOD_UP)
    return;

  // everything after here will be removed sooner or later...

  if (modifiers & FG_MOD_SHIFT) {

	switch (k) {
	case 72: // H key
	    HUD_brightkey( true );
	    return;
	case 73: // I key
	    // Minimal Hud
	    fgHUDInit2(&current_aircraft);
	    return;
	}


    } else {
	SG_LOG( SG_INPUT, SG_DEBUG, "" );
	switch (k) {
	case 104: // h key
	    HUD_masterswitch( true );
	    return;
	case 105: // i key
	    fgHUDInit(&current_aircraft);  // normal HUD
	    return;

// START SPECIALS

        case 256+GLUT_KEY_F6: // F6 toggles Autopilot target location
	    if ( globals->get_autopilot()->get_HeadingMode() !=
		 FGAutopilot::FG_HEADING_WAYPOINT ) {
		globals->get_autopilot()->set_HeadingMode(
		    FGAutopilot::FG_HEADING_WAYPOINT );
		globals->get_autopilot()->set_HeadingEnabled( true );
	    } else {
		globals->get_autopilot()->set_HeadingMode(
		    FGAutopilot::FG_TC_HEADING_LOCK );
	    }
	    return;
	case 256+GLUT_KEY_F8: {// F8 toggles fog ... off fastest nicest...
	    const string &fog = fgGetString("/sim/rendering/fog");
	    if (fog == "disabled") {
	      fgSetString("/sim/rendering/fog", "fastest");
	      SG_LOG(SG_INPUT, SG_INFO, "Fog enabled, hint=fastest");
	    } else if (fog == "fastest") {
	      fgSetString("/sim/rendering/fog", "nicest");
	      SG_LOG(SG_INPUT, SG_INFO, "Fog enabled, hint=nicest");
	    } else if (fog == "nicest") {
	      fgSetString("/sim/rendering/fog", "disabled");
	      SG_LOG(SG_INPUT, SG_INFO, "Fog disabled");
	    } else {
	      fgSetString("/sim/rendering/fog", "disabled");
	      SG_LOG(SG_INPUT, SG_ALERT, "Unrecognized fog type "
		     << fog << ", changed to 'disabled'");
	    }
 	    return;
	}
	case 256+GLUT_KEY_F10: // F10 toggles menu on and off...
	    SG_LOG(SG_INPUT, SG_INFO, "Invoking call back function");
	    guiToggleMenu();
	    return;
	case 256+GLUT_KEY_F11: // F11 Altitude Dialog.
	    SG_LOG(SG_INPUT, SG_INFO, "Invoking Altitude call back function");
	    NewAltitude( NULL );
	    return;
	case 256+GLUT_KEY_F12: // F12 Heading Dialog...
	    SG_LOG(SG_INPUT, SG_INFO, "Invoking Heading call back function");
	    NewHeading( NULL );
	    return;
	}

// END SPECIALS

    }
}

void
FGInput::doMouseClick (int b, int updown, int x, int y)
{
  int modifiers = FG_MOD_NONE;	// FIXME: any way to get the real ones?

  mouse &m = _mouse_bindings[0];
  mouse_mode &mode = m.modes[m.current_mode];

				// Let the property manager know.
  if (b >= 0 && b < MAX_MOUSE_BUTTONS)
    m.mouse_button_nodes[b]->setBoolValue(updown == GLUT_DOWN);

				// Pass on to PUI and the panel if
				// requested, and return if one of
				// them consumes the event.
  if (mode.pass_through) {
    if (puMouse(b, updown, x, y))
      return;
    else if ((current_panel != 0) &&
	     current_panel->doMouseAction(b, updown, x, y))
      return;
  }

				// OK, PUI and the panel didn't want the click
  if (b >= MAX_MOUSE_BUTTONS) {
    SG_LOG(SG_INPUT, SG_ALERT, "Mouse button " << b
	   << " where only " << MAX_MOUSE_BUTTONS << " expected");
    return;
  }

  _update_button(m.modes[m.current_mode].buttons[b], modifiers, 0 != updown, x, y);
}

void
FGInput::doMouseMotion (int x, int y)
{
  int modifiers = FG_MOD_NONE;	// FIXME: any way to get the real ones?

  int xsize = fgGetInt("/sim/startup/xsize", 800);
  int ysize = fgGetInt("/sim/startup/ysize", 600);
  mouse &m = _mouse_bindings[0];
  if (m.current_mode < 0 || m.current_mode >= m.nModes)
    return;
  mouse_mode &mode = m.modes[m.current_mode];

				// Pass on to PUI if requested, and return
				// if PUI consumed the event.
  if (mode.pass_through && puMouse(x, y))
    return;

				// OK, PUI didn't want the event,
				// so we can play with it.
  if (x != m.x) {
    int delta = x - m.x;
    for (unsigned int i = 0; i < mode.x_bindings[modifiers].size(); i++)
      mode.x_bindings[modifiers][i]->fire(double(delta), double(xsize));
  }
  if (y != m.y) {
    int delta = y - m.y;
    for (unsigned int i = 0; i < mode.y_bindings[modifiers].size(); i++)
      mode.y_bindings[modifiers][i]->fire(double(delta), double(ysize));
  }

				// Constrain the mouse if requested
  if (mode.constrained) {
    bool need_warp = false;
    if (x <= 0) {
      x = xsize - 2;
      need_warp = true;
    } else if (x >= (xsize-1)) {
      x = 1;
      need_warp = true;
    }

    if (y <= 0) {
      y = ysize - 2;
      need_warp = true;
    } else if (y >= (ysize-1)) {
      y = 1;
      need_warp = true;
    }

    if (need_warp)
      glutWarpPointer(x, y);
  }
  m.x = x;
  m.y = y;
}

void
FGInput::_init_keyboard ()
{
				// TODO: zero the old bindings first.
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing key bindings");
  SGPropertyNode * key_nodes = fgGetNode("/input/keyboard");
  if (key_nodes == 0) {
    SG_LOG(SG_INPUT, SG_WARN, "No key bindings (/input/keyboard)!!");
    key_nodes = fgGetNode("/input/keyboard", true);
  }
  
  vector<SGPropertyNode_ptr> keys = key_nodes->getChildren("key");
  for (unsigned int i = 0; i < keys.size(); i++) {
    int index = keys[i]->getIndex();
    SG_LOG(SG_INPUT, SG_DEBUG, "Binding key " << index);
    _key_bindings[index].is_repeatable = keys[i]->getBoolValue("repeatable");
    _read_bindings(keys[i], _key_bindings[index].bindings, FG_MOD_NONE);
  }
}


void
FGInput::_init_joystick ()
{
				// TODO: zero the old bindings first.
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing joystick bindings");
  SGPropertyNode * js_nodes = fgGetNode("/input/joysticks");
  if (js_nodes == 0) {
    SG_LOG(SG_INPUT, SG_WARN, "No joystick bindings (/input/joysticks)!!");
    js_nodes = fgGetNode("/input/joysticks", true);
  }

  for (int i = 0; i < MAX_JOYSTICKS; i++) {
    SGPropertyNode_ptr js_node = js_nodes->getChild("js", i);
    if (js_node == 0) {
      SG_LOG(SG_INPUT, SG_DEBUG, "No bindings for joystick " << i);
      js_node = js_nodes->getChild("js", i, true);
    }
    jsJoystick * js = new jsJoystick(i);
    _joystick_bindings[i].js = js;
    if (js->notWorking()) {
      SG_LOG(SG_INPUT, SG_WARN, "Joystick " << i << " not found");
      continue;
    } else {
#ifdef FG_PLIB_JOYSTICK_GETNAME
      bool found_js = false;
      const char * name = js->getName();
      SG_LOG(SG_INPUT, SG_INFO, "Looking for bindings for joystick \""
	     << name << '"');
      vector<SGPropertyNode_ptr> nodes = js_nodes->getChildren("js-named");
      for (unsigned int i = 0; i < nodes.size(); i++) {
	SGPropertyNode_ptr node = nodes[i];
        vector<SGPropertyNode_ptr> name_nodes = node->getChildren("name");
        for (unsigned int j = 0; j < name_nodes.size(); j++) {
            const char * js_name = name_nodes[j]->getStringValue();
            SG_LOG(SG_INPUT, SG_INFO, "  Trying \"" << js_name << '"');
            if (!strcmp(js_name, name)) {
                SG_LOG(SG_INPUT, SG_INFO, "  Found bindings");
                js_node = node;
                found_js = true;
                break;
            }
        }
        if (found_js)
            break;
      }
#endif
    }
#ifdef WIN32
    JOYCAPS jsCaps ;
    joyGetDevCaps( i, &jsCaps, sizeof(jsCaps) );
    int nbuttons = jsCaps.wNumButtons;
    if (nbuttons > MAX_JOYSTICK_BUTTONS) nbuttons = MAX_JOYSTICK_BUTTONS;
#else
    int nbuttons = MAX_JOYSTICK_BUTTONS;
#endif
	
    int naxes = js->getNumAxes();
    if (naxes > MAX_JOYSTICK_AXES) naxes = MAX_JOYSTICK_AXES;
    _joystick_bindings[i].naxes = naxes;
    _joystick_bindings[i].nbuttons = nbuttons;

    SG_LOG(SG_INPUT, SG_DEBUG, "Initializing joystick " << i);

				// Set up range arrays
    float minRange[MAX_JOYSTICK_AXES];
    float maxRange[MAX_JOYSTICK_AXES];
    float center[MAX_JOYSTICK_AXES];

				// Initialize with default values
    js->getMinRange(minRange);
    js->getMaxRange(maxRange);
    js->getCenter(center);

				// Allocate axes and buttons
    _joystick_bindings[i].axes = new axis[naxes];
    _joystick_bindings[i].buttons = new button[nbuttons];


    //
    // Initialize the axes.
    //
    int j;
    for (j = 0; j < naxes; j++) {
      const SGPropertyNode * axis_node = js_node->getChild("axis", j);
      if (axis_node == 0) {
	SG_LOG(SG_INPUT, SG_DEBUG, "No bindings for axis " << j);
	axis_node = js_node->getChild("axis", j, true);
      }
      
      axis &a = _joystick_bindings[i].axes[j];

      js->setDeadBand(j, axis_node->getDoubleValue("dead-band", 0.0));

      a.tolerance = axis_node->getDoubleValue("tolerance", 0.002);
      minRange[j] = axis_node->getDoubleValue("min-range", minRange[j]);
      maxRange[j] = axis_node->getDoubleValue("max-range", maxRange[j]);
      center[j] = axis_node->getDoubleValue("center", center[j]);

      _read_bindings(axis_node, a.bindings, FG_MOD_NONE);

      // Initialize the virtual axis buttons.
      _init_button(axis_node->getChild("low"), a.low, "low");
      a.low_threshold = axis_node->getDoubleValue("low-threshold", -0.9);
      
      _init_button(axis_node->getChild("high"), a.high, "high");
      a.high_threshold = axis_node->getDoubleValue("high-threshold", 0.9);
    }

    //
    // Initialize the buttons.
    //
    char buf[32];
    for (j = 0; j < nbuttons; j++) {
      sprintf(buf, "%d", j);
      SG_LOG(SG_INPUT, SG_DEBUG, "Initializing button " << j);
      _init_button(js_node->getChild("button", j),
		   _joystick_bindings[i].buttons[j],
		   buf);
		   
    }

    js->setMinRange(minRange);
    js->setMaxRange(maxRange);
    js->setCenter(center);
  }
}

// 
// Map of all known GLUT cursor names
//
struct {
  const char * name;
  int cursor;
} mouse_cursor_map[] = {
  { "right-arrow", GLUT_CURSOR_RIGHT_ARROW },
  { "left-arrow", GLUT_CURSOR_LEFT_ARROW },
  { "info", GLUT_CURSOR_INFO },
  { "destroy", GLUT_CURSOR_DESTROY },
  { "help", GLUT_CURSOR_HELP },
  { "cycle", GLUT_CURSOR_CYCLE },
  { "spray", GLUT_CURSOR_SPRAY },
  { "wait", GLUT_CURSOR_WAIT },
  { "text", GLUT_CURSOR_TEXT },
  { "crosshair", GLUT_CURSOR_CROSSHAIR },
  { "up-down", GLUT_CURSOR_UP_DOWN },
  { "left-right", GLUT_CURSOR_LEFT_RIGHT },
  { "top-side", GLUT_CURSOR_TOP_SIDE },
  { "bottom-side", GLUT_CURSOR_BOTTOM_SIDE },
  { "left-side", GLUT_CURSOR_LEFT_SIDE },
  { "right-side", GLUT_CURSOR_RIGHT_SIDE },
  { "top-left-corner", GLUT_CURSOR_TOP_LEFT_CORNER },
  { "top-right-corner", GLUT_CURSOR_TOP_RIGHT_CORNER },
  { "bottom-right-corner", GLUT_CURSOR_BOTTOM_RIGHT_CORNER },
  { "bottom-left-corner", GLUT_CURSOR_BOTTOM_LEFT_CORNER },
  { "inherit", GLUT_CURSOR_INHERIT },
  { "none", GLUT_CURSOR_NONE },
  { "full-crosshair", GLUT_CURSOR_FULL_CROSSHAIR },
  { 0, 0 }
};



void
FGInput::_init_mouse ()
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing mouse bindings");

  SGPropertyNode * mouse_nodes = fgGetNode("/input/mice");
  if (mouse_nodes == 0) {
    SG_LOG(SG_INPUT, SG_WARN, "No mouse bindings (/input/mice)!!");
    mouse_nodes = fgGetNode("/input/mice", true);
  }

  int j;
  for (int i = 0; i < MAX_MICE; i++) {
    SGPropertyNode * mouse_node = mouse_nodes->getChild("mouse", i, true);
    mouse &m = _mouse_bindings[i];

				// Grab node pointers
    char buf[64];
    sprintf(buf, "/devices/status/mice/mouse[%d]/mode", i);
    m.mode_node = fgGetNode(buf, true);
    m.mode_node->setIntValue(0);
    for (j = 0; j < MAX_MOUSE_BUTTONS; j++) {
      sprintf(buf, "/devices/status/mice/mouse[%d]/button[%d]", i, j);
      m.mouse_button_nodes[j] = fgGetNode(buf, true);
      m.mouse_button_nodes[j]->setBoolValue(false);
    }

				// Read all the modes
    m.nModes = mouse_node->getIntValue("mode-count", 1);
    m.modes = new mouse_mode[m.nModes];

    for (int j = 0; j < m.nModes; j++) {
      int k;

				// Read the mouse cursor for this mode
      SGPropertyNode * mode_node = mouse_node->getChild("mode", j, true);
      const char * cursor_name =
	mode_node->getStringValue("cursor", "inherit");
      m.modes[j].cursor = GLUT_CURSOR_INHERIT;
      for (k = 0; mouse_cursor_map[k].name != 0; k++) {
	if (!strcmp(mouse_cursor_map[k].name, cursor_name)) {
	  m.modes[j].cursor = mouse_cursor_map[k].cursor;
	  break;
	}
      }

				// Read other properties for this mode
      m.modes[j].constrained = mode_node->getBoolValue("constrained", false);
      m.modes[j].pass_through = mode_node->getBoolValue("pass-through", false);

				// Read the button bindings for this mode
      m.modes[j].buttons = new button[MAX_MOUSE_BUTTONS];
      char buf[32];
      for (k = 0; k < MAX_MOUSE_BUTTONS; k++) {
	sprintf(buf, "mouse button %d", k);
	SG_LOG(SG_INPUT, SG_DEBUG, "Initializing mouse button " << k);
	_init_button(mode_node->getChild("button", k),
		     m.modes[j].buttons[k],
		     buf);
      }

				// Read the axis bindings for this mode
      _read_bindings(mode_node->getChild("x-axis", 0, true),
		     m.modes[j].x_bindings,
		     FG_MOD_NONE);
      _read_bindings(mode_node->getChild("y-axis", 0, true),
		     m.modes[j].y_bindings,
		     FG_MOD_NONE);
    }
  }
}


void
FGInput::_init_button (const SGPropertyNode * node,
		       button &b,
		       const string name)
{	
  if (node == 0) {
    SG_LOG(SG_INPUT, SG_DEBUG, "No bindings for button " << name);
  } else {
    b.is_repeatable = node->getBoolValue("repeatable", b.is_repeatable);
    
    		// Get the bindings for the button
    _read_bindings(node, b.bindings, FG_MOD_NONE);
  }
}


void
FGInput::_update_keyboard ()
{
  // no-op
}


void
FGInput::_update_joystick ()
{
  int modifiers = FG_MOD_NONE;	// FIXME: any way to get the real ones?
  int buttons;
  // float js_val, diff;
  float axis_values[MAX_JOYSTICK_AXES];

  int i;
  int j;

  for ( i = 0; i < MAX_JOYSTICKS; i++) {

    jsJoystick * js = _joystick_bindings[i].js;
    if (js == 0 || js->notWorking())
      continue;

    js->read(&buttons, axis_values);


				// Fire bindings for the axes.
    for ( j = 0; j < _joystick_bindings[i].naxes; j++) {
      axis &a = _joystick_bindings[i].axes[j];
      
				// Do nothing if the axis position
				// is unchanged; only a change in
				// position fires the bindings.
      if (fabs(axis_values[j] - a.last_value) > a.tolerance) {
// 	SG_LOG(SG_INPUT, SG_DEBUG, "Axis " << j << " has moved");
	SGPropertyNode node;
	a.last_value = axis_values[j];
// 	SG_LOG(SG_INPUT, SG_DEBUG, "There are "
// 	       << a.bindings[modifiers].size() << " bindings");
	for (unsigned int k = 0; k < a.bindings[modifiers].size(); k++)
	  a.bindings[modifiers][k]->fire(axis_values[j]);
      }
     
				// do we have to emulate axis buttons?
      if (a.low.bindings[modifiers].size())
	_update_button(_joystick_bindings[i].axes[j].low,
		       modifiers,
		       axis_values[j] < a.low_threshold,
		       -1, -1);
      
      if (a.high.bindings[modifiers].size())
	_update_button(_joystick_bindings[i].axes[j].high,
		       modifiers,
		       axis_values[j] > a.high_threshold,
		       -1, -1);
    }

				// Fire bindings for the buttons.
    for (j = 0; j < _joystick_bindings[i].nbuttons; j++) {
      _update_button(_joystick_bindings[i].buttons[j],
		     modifiers,
		     (buttons & (1 << j)) > 0,
		     -1, -1);
    }
  }
}

void
FGInput::_update_mouse ()
{
  mouse &m = _mouse_bindings[0];
  int mode =  m.mode_node->getIntValue();
  if (mode != m.current_mode) {
    m.current_mode = mode;
    if (mode >= 0 && mode < m.nModes) {
      glutSetCursor(m.modes[mode].cursor);
      m.x = fgGetInt("/sim/startup/xsize", 800) / 2;
      m.y = fgGetInt("/sim/startup/ysize", 600) / 2;
      glutWarpPointer(m.x, m.y);
    } else {
      SG_LOG(SG_INPUT, SG_DEBUG, "Mouse mode " << mode << " out of range");
      glutSetCursor(GLUT_CURSOR_INHERIT);
    }
  }
}

void
FGInput::_update_button (button &b, int modifiers, bool pressed,
			 int x, int y)
{
  if (pressed) {
				// The press event may be repeated.
    if (!b.last_state || b.is_repeatable) {
      SG_LOG( SG_INPUT, SG_DEBUG, "Button has been pressed" );
      for (unsigned int k = 0; k < b.bindings[modifiers].size(); k++)
	b.bindings[modifiers][k]->fire(x, y);
    }
  } else {
				// The release event is never repeated.
    if (b.last_state) {
      SG_LOG( SG_INPUT, SG_DEBUG, "Button has been released" );
      for (unsigned int k = 0; k < b.bindings[modifiers|FG_MOD_UP].size(); k++)
	b.bindings[modifiers|FG_MOD_UP][k]->fire(x, y);
    }
  }
	  
  b.last_state = pressed;
}  


void
FGInput::_read_bindings (const SGPropertyNode * node, 
			 binding_list_t * binding_list,
			 int modifiers)
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Reading all bindings");
  vector<SGPropertyNode_ptr> bindings = node->getChildren("binding");
  for (unsigned int i = 0; i < bindings.size(); i++) {
    SG_LOG(SG_INPUT, SG_DEBUG, "Reading binding "
	   << bindings[i]->getStringValue("command"));
    binding_list[modifiers].push_back(new FGBinding(bindings[i]));
  }

				// Read nested bindings for modifiers
  if (node->getChild("mod-up") != 0)
    _read_bindings(node->getChild("mod-up"), binding_list,
		   modifiers|FG_MOD_UP);

  if (node->getChild("mod-shift") != 0)
    _read_bindings(node->getChild("mod-shift"), binding_list,
		   modifiers|FG_MOD_SHIFT);

  if (node->getChild("mod-ctrl") != 0)
    _read_bindings(node->getChild("mod-ctrl"), binding_list,
		   modifiers|FG_MOD_CTRL);

  if (node->getChild("mod-alt") != 0)
    _read_bindings(node->getChild("mod-alt"), binding_list,
		   modifiers|FG_MOD_ALT);
}


const vector<FGBinding *> &
FGInput::_find_key_bindings (unsigned int k, int modifiers)
{
  button &b = _key_bindings[k];

				// Try it straight, first.
  if (b.bindings[modifiers].size() > 0)
    return b.bindings[modifiers];

				// Try removing the control modifier
				// for control keys.
  else if ((modifiers&FG_MOD_CTRL) && iscntrl(k))
    return _find_key_bindings(k, modifiers&~FG_MOD_CTRL);

				// Try removing shift modifier 
				// for upper case or any punctuation
				// (since different keyboards will
				// shift different punctuation types)
  else if ((modifiers&FG_MOD_SHIFT) && (isupper(k) || ispunct(k)))
    return _find_key_bindings(k, modifiers&~FG_MOD_SHIFT);

				// Try removing alt modifier for
				// high-bit characters.
  else if ((modifiers&FG_MOD_ALT) && k >= 128 && k < 256)
    return _find_key_bindings(k, modifiers&~FG_MOD_ALT);

				// Give up and return the empty vector.
  else
    return b.bindings[modifiers];
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::button.
////////////////////////////////////////////////////////////////////////

FGInput::button::button ()
  : is_repeatable(false),
    last_state(-1)
{
}

FGInput::button::~button ()
{
				// FIXME: memory leak
//   for (int i = 0; i < FG_MOD_MAX; i++)
//     for (int j = 0; i < bindings[i].size(); j++)
//       delete bindings[i][j];
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::axis.
////////////////////////////////////////////////////////////////////////

FGInput::axis::axis ()
  : last_value(9999999),
    tolerance(0.002),
    low_threshold(-0.9),
    high_threshold(0.9)
{
}

FGInput::axis::~axis ()
{
//   for (int i = 0; i < FG_MOD_MAX; i++)
//     for (int j = 0; i < bindings[i].size(); j++)
//       delete bindings[i][j];
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::joystick.
////////////////////////////////////////////////////////////////////////

FGInput::joystick::joystick ()
{
}

FGInput::joystick::~joystick ()
{
//   delete js;
  delete[] axes;
  delete[] buttons;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::mouse_mode
////////////////////////////////////////////////////////////////////////

FGInput::mouse_mode::mouse_mode ()
  : cursor(GLUT_CURSOR_INHERIT),
    constrained(false),
    pass_through(false),
    buttons(0)
{
}

FGInput::mouse_mode::~mouse_mode ()
{
				// FIXME: memory leak
//   for (int i = 0; i < FG_MOD_MAX; i++) {
//     int j;
//     for (j = 0; i < x_bindings[i].size(); j++)
//       delete bindings[i][j];
//     for (j = 0; j < y_bindings[i].size(); j++)
//       delete bindings[i][j];
//   }
  delete [] buttons;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::mouse
////////////////////////////////////////////////////////////////////////

FGInput::mouse::mouse ()
  : x(-1),
    y(-1),
    nModes(1),
    current_mode(0),
    modes(0)
{
}

FGInput::mouse::~mouse ()
{
  delete [] modes;
}



////////////////////////////////////////////////////////////////////////
// Implementation of GLUT callbacks.
////////////////////////////////////////////////////////////////////////


/**
 * Construct the modifiers.
 */
static inline int get_mods ()
{
  int glut_modifiers = glutGetModifiers();
  int modifiers = 0;

  if (glut_modifiers & GLUT_ACTIVE_SHIFT)
    modifiers |= FGInput::FG_MOD_SHIFT;
  if (glut_modifiers & GLUT_ACTIVE_CTRL)
    modifiers |= FGInput::FG_MOD_CTRL;
  if (glut_modifiers & GLUT_ACTIVE_ALT)
    modifiers |= FGInput::FG_MOD_ALT;

  return modifiers;
}



////////////////////////////////////////////////////////////////////////
// GLUT C callbacks.
////////////////////////////////////////////////////////////////////////

void
GLUTkey(unsigned char k, int x, int y)
{
				// Give PUI a chance to grab it first.
    if (!puKeyboard(k, PU_DOWN)) {
      if (default_input != 0)
          default_input->doKey(k, get_mods(), x, y);
    }
}

void
GLUTkeyup(unsigned char k, int x, int y)
{
    if (default_input != 0)
        default_input->doKey(k, get_mods()|FGInput::FG_MOD_UP, x, y);
}

void
GLUTspecialkey(int k, int x, int y)
{
				// Give PUI a chance to grab it first.
    if (!puKeyboard(k + PU_KEY_GLUT_SPECIAL_OFFSET, PU_DOWN)) {
        if (default_input != 0)
            default_input->doKey(k + 256, get_mods(), x, y);
    }
}

void
GLUTspecialkeyup(int k, int x, int y)
{
    if (default_input != 0)
        default_input->doKey(k + 256, get_mods()|FGInput::FG_MOD_UP, x, y);
}

void
GLUTmouse (int button, int updown, int x, int y)
{
    if (default_input != 0)
        default_input->doMouseClick(button, updown, x, y);
}

void
GLUTmotion (int x, int y)
{
    if (default_input != 0)
        default_input->doMouseMotion(x, y);
}

// end of input.cxx
