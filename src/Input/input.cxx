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
#include <Scenery/tilemgr.hxx>
#include <Objects/matlib.hxx>
#include <Time/light.hxx>
#include <Time/tmp.hxx>

#ifndef FG_OLD_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include <Main/bfi.hxx>
#include <Main/globals.hxx>
#include <Main/keyboard.hxx>
#include <Main/fg_props.hxx>
#include <Main/options.hxx>

#include "input.hxx"

SG_USING_STD(ifstream);
SG_USING_STD(string);
SG_USING_STD(vector);



////////////////////////////////////////////////////////////////////////
// Local data structures.
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// Implementation of FGBinding.
////////////////////////////////////////////////////////////////////////

FGBinding::FGBinding ()
  : _command(0), _arg(0)
{
}

FGBinding::FGBinding (const SGPropertyNode * node)
  : _command(0), _arg(0)
{
  read(node);
}

FGBinding::~FGBinding ()
{
  // no op
}

void
FGBinding::read (const SGPropertyNode * node)
{
  _command_name = node->getStringValue("command", "");
  if (_command_name == "") {
    SG_LOG(SG_INPUT, SG_ALERT, "No command supplied for binding.");
    _command = 0;
    return;
  }

  _command = globals->get_commands()->getCommand(_command_name);
  if (_command == 0) {
    SG_LOG(SG_INPUT, SG_ALERT, "Command " << _command_name << " is undefined");
    _arg = 0;
    return;
  }
  _arg = node;			// FIXME: don't use whole node!!!
}

void
FGBinding::fire () const
{
  _fire(_arg);
}

void
FGBinding::fire (double setting) const
{
  SGPropertyNode arg;
  copyProperties(_arg, &arg);
  arg.setDoubleValue("setting", setting);
  _fire(&arg);
}

void
FGBinding::_fire(const SGPropertyNode * arg) const
{
  if (_command == 0) {
    SG_LOG(SG_INPUT, SG_ALERT, "No command attached to binding");
  } else if (!(*_command)(arg)) {
    SG_LOG(SG_INPUT, SG_ALERT, "Failed to execute command " << _command_name);
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput.
////////////////////////////////////////////////////////////////////////

				// From main.cxx
extern void fgReshape( int width, int height );

FGInput current_input;


FGInput::FGInput ()
{
  // no op
}

FGInput::~FGInput ()
{
  // no op
}

void
FGInput::init ()
{
  _init_keyboard();
  _init_joystick();
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
FGInput::update ()
{
  _update_keyboard();
  _update_joystick();
}

void
FGInput::doKey (int k, int modifiers, int x, int y)
{
				// Sanity check.
  if (k < 0 || k >= MAX_KEYS) {
    SG_LOG(SG_INPUT, SG_ALERT, "Key value " << k << " out of range");
    return;
  }

  button &b = _key_bindings[k];

				// Key pressed.
  if (modifiers&FG_MOD_UP == 0) {
    // SG_LOG(SG_INPUT, SG_INFO, "User pressed key " << k
    //        << " with modifiers " << modifiers);
    if (!b.last_state || b.is_repeatable) {
      const binding_list_t &bindings =
	_find_key_bindings(k, modifiers);
      int max = bindings.size();
      if (max > 0) {
	for (int i = 0; i < max; i++)
	  bindings[i].fire();
	return;
      }
    }
  }

				// Key released.
  else {
    // SG_LOG(SG_INPUT, SG_INFO, "User released key " << k
    //        << " with modifiers " << modifiers);
    if (b.last_state) {
      const binding_list_t &bindings =
	_find_key_bindings(k, modifiers);
      int max = bindings.size();
      if (max > 0) {
	for (int i = 0; i < max; i++)
	  bindings[i].fire();
	return;
      }
    }
  }


				// Use the old, default actions.
  SG_LOG(SG_INPUT, SG_INFO, "(No user binding.)");
  float fov, tmp;
  static bool winding_ccw = true;
  int speed;
  FGInterface *f = current_aircraft.fdm_state;
  FGViewer *v = globals->get_current_view();
  
  // everything after here will be removed sooner or later...

  if (modifiers & FG_MOD_SHIFT) {

	switch (k) {
	case 7: // Ctrl-G key
	    current_autopilot->set_AltitudeMode( 
                  FGAutopilot::FG_ALTITUDE_GS1 );
	    current_autopilot->set_AltitudeEnabled(
		  ! current_autopilot->get_AltitudeEnabled()
	        );
	    return;
	case 18: // Ctrl-R key
	    // temporary
	    winding_ccw = !winding_ccw;
	    if ( winding_ccw ) {
		glFrontFace ( GL_CCW );
	    } else {
		glFrontFace ( GL_CW );
	    }
	    return;
	case 20: // Ctrl-T key
	    current_autopilot->set_AltitudeMode( 
                  FGAutopilot::FG_ALTITUDE_TERRAIN );
	    current_autopilot->set_AltitudeEnabled(
		  ! current_autopilot->get_AltitudeEnabled()
	        );
	    return;
	case 72: // H key
	    HUD_brightkey( true );
	    return;
	case 73: // I key
	    // Minimal Hud
	    fgHUDInit2(&current_aircraft);
	    return;
	case 77: // M key
	    globals->inc_warp( -60 );
	    fgUpdateSkyAndLightingParams();
	    return;
	case 84: // T key
	    globals->inc_warp_delta( -30 );
	    fgUpdateSkyAndLightingParams();
	    return;
	case 87: // W key
#if defined(FX) && !defined(WIN32)
	    global_fullscreen = ( !global_fullscreen );
#  if defined(XMESA_FX_FULLSCREEN) && defined(XMESA_FX_WINDOW)
	    XMesaSetFXmode( global_fullscreen ? 
			    XMESA_FX_FULLSCREEN : XMESA_FX_WINDOW );
#  endif
#endif
	    return;
	case 88: // X key
	    fov = globals->get_current_view()->get_fov();
	    fov *= 1.05;
	    if ( fov > FG_FOV_MAX ) {
		fov = FG_FOV_MAX;
	    }
	    globals->get_current_view()->set_fov(fov);
	    // v->force_update_fov_math();
	    return;
	case 90: // Z key
#ifndef FG_OLD_WEATHER
	    tmp = WeatherDatabase->getWeatherVisibility();
	    tmp /= 1.10;
	    WeatherDatabase->setWeatherVisibility( tmp );
#else
	    tmp = current_weather.get_visibility();   // in meters
	    tmp /= 1.10;
	    current_weather.set_visibility( tmp );
#endif
	    return;

// START SPECIALS

 	case 256+GLUT_KEY_F1: {
 	    ifstream input("fgfs.sav");
 	    if (input.good() && fgLoadFlight(input)) {
   	        input.close();
 		SG_LOG(SG_INPUT, SG_INFO, "Restored flight from fgfs.sav");
 	    } else {
 	        SG_LOG(SG_INPUT, SG_ALERT, "Cannot load flight from fgfs.sav");
 	    }
 	    return;
 	}
 	case 256+GLUT_KEY_F2: {
 	    SG_LOG(SG_INPUT, SG_INFO, "Saving flight");
 	    ofstream output("fgfs.sav");
 	    if (output.good() && fgSaveFlight(output)) {
 		output.close();
 		SG_LOG(SG_INPUT, SG_INFO, "Saved flight to fgfs.sav");
 	    } else {
 	        SG_LOG(SG_INPUT, SG_ALERT, "Cannot save flight to fgfs.sav");
 	    }
 	    return;
 	}
	case 256+GLUT_KEY_F3: {
            string panel_path =
                fgGetString("/sim/panel/path", "Panels/Default/default.xml");
            FGPanel * new_panel = fgReadPanel(panel_path);
            if (new_panel == 0) {
                SG_LOG(SG_INPUT, SG_ALERT,
                       "Error reading new panel from " << panel_path);
                return;
            }
            SG_LOG(SG_INPUT, SG_INFO, "Loaded new panel from " << panel_path);
            current_panel->unbind();
            delete current_panel;
            current_panel = new_panel;
	    current_panel->bind();
            return;
	}
	case 256+GLUT_KEY_F4: {
            SGPath props_path(globals->get_fg_root());
            props_path.append("preferences.xml");
            SG_LOG(SG_INPUT, SG_INFO, "Rereading global preferences");
            if (!readProperties(props_path.str(), globals->get_props())) {
                SG_LOG(SG_INPUT, SG_ALERT,
                       "Failed to reread global preferences from "
                       << props_path.str());
            } else {
                SG_LOG(SG_INPUT, SG_INFO, "Finished Reading global preferences");
            }
            return;
	}
	case 256+GLUT_KEY_F5: {
            current_panel->setYOffset(current_panel->getYOffset() - 5);
            fgReshape(fgGetInt("/sim/startup/xsize"),
                      fgGetInt("/sim/startup/ysize"));
            return;
	}
	case 256+GLUT_KEY_F6: {
            current_panel->setYOffset(current_panel->getYOffset() + 5);
            fgReshape(fgGetInt("/sim/startup/xsize"),
                      fgGetInt("/sim/startup/ysize"));
            return;
	}
	case 256+GLUT_KEY_F7: {
            current_panel->setXOffset(current_panel->getXOffset() - 5);
            return;
	}
	case 256+GLUT_KEY_F8: {
            current_panel->setXOffset(current_panel->getXOffset() + 5);
            return;
	}
        case 256+GLUT_KEY_F10: {
            fgToggleFDMdataLogging();
            return;
        }

// END SPECIALS

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
	case 109: // m key
	    globals->inc_warp( 60 );
	    fgUpdateSkyAndLightingParams();
	    return;
	case 112: // p key
	    globals->set_freeze( ! globals->get_freeze() );

	    {
		SGBucket p( f->get_Longitude() * SGD_RADIANS_TO_DEGREES,
			    f->get_Latitude() * SGD_RADIANS_TO_DEGREES );
		SGPath tile_path( globals->get_fg_root() );
		tile_path.append( "Scenery" );
		tile_path.append( p.gen_base_path() );
		tile_path.append( p.gen_index_str() );

		// printf position and attitude information
		SG_LOG( SG_INPUT, SG_INFO,
			"Lon = " << f->get_Longitude() * SGD_RADIANS_TO_DEGREES
			<< "  Lat = " << f->get_Latitude() * SGD_RADIANS_TO_DEGREES
			<< "  Altitude = " << f->get_Altitude() * SG_FEET_TO_METER
			);
		SG_LOG( SG_INPUT, SG_INFO,
			"Heading = " << f->get_Psi() * SGD_RADIANS_TO_DEGREES 
			<< "  Roll = " << f->get_Phi() * SGD_RADIANS_TO_DEGREES
			<< "  Pitch = " << f->get_Theta() * SGD_RADIANS_TO_DEGREES );
		SG_LOG( SG_INPUT, SG_INFO, tile_path.c_str());
	    }
	    return;
	case 116: // t key
	    globals->inc_warp_delta( 30 );
	    fgUpdateSkyAndLightingParams();
	    return;
	case 120: // x key
	    fov = globals->get_current_view()->get_fov();
	    fov /= 1.05;
	    if ( fov < FG_FOV_MIN ) {
		fov = FG_FOV_MIN;
	    }
	    globals->get_current_view()->set_fov(fov);
	    // v->force_update_fov_math();
	    return;
	case 122: // z key
#ifndef FG_OLD_WEATHER
	    tmp = WeatherDatabase->getWeatherVisibility();
	    tmp *= 1.10;
	    WeatherDatabase->setWeatherVisibility( tmp );
#else
	    tmp = current_weather.get_visibility();   // in meters
	    tmp *= 1.10;
	    current_weather.set_visibility( tmp );
#endif
	    return;
	case 27: // ESC
	    // if( fg_DebugOutput ) {
	    //   fclose( fg_DebugOutput );
	    // }
	    SG_LOG( SG_INPUT, SG_ALERT, 
		    "Program exit requested." );
	    ConfirmExitDialog();
	    return;

// START SPECIALS

	case 256+GLUT_KEY_F2: // F2 Reload Tile Cache...
	    {
		bool freeze = globals->get_freeze();
		SG_LOG(SG_INPUT, SG_INFO, "ReIniting TileCache");
		if ( !freeze ) 
		    globals->set_freeze( true );
		BusyCursor(0);
		if ( global_tile_mgr.init() ) {
		    // Load the local scenery data
		    global_tile_mgr.update( 
		        cur_fdm_state->get_Longitude() * SGD_RADIANS_TO_DEGREES,
			cur_fdm_state->get_Latitude() * SGD_RADIANS_TO_DEGREES );
		} else {
		    SG_LOG( SG_GENERAL, SG_ALERT, 
			    "Error in Tile Manager initialization!" );
		    exit(-1);
		}
		BusyCursor(1);
		if ( !freeze )
		   globals->set_freeze( false );
		return;
	    }
        case 256+GLUT_KEY_F4: // F4 Update lighting manually
	    fgUpdateSkyAndLightingParams();
            return;
        case 256+GLUT_KEY_F6: // F6 toggles Autopilot target location
	    if ( current_autopilot->get_HeadingMode() !=
		 FGAutopilot::FG_HEADING_WAYPOINT ) {
		current_autopilot->set_HeadingMode(
		    FGAutopilot::FG_HEADING_WAYPOINT );
		current_autopilot->set_HeadingEnabled( true );
	    } else {
		current_autopilot->set_HeadingMode(
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
 	case 256+GLUT_KEY_F9: // F9 toggles textures on and off...
	    SG_LOG( SG_INPUT, SG_INFO, "Toggling texture" );
	    if ( fgGetBool("/sim/rendering/textures")) {
		fgSetBool("/sim/rendering/textures", false);
		material_lib.set_step( 1 );
	    } else {
		fgSetBool("/sim/rendering/textures", true);
		material_lib.set_step( 0 );
	    }
 	    return;
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
FGInput::_init_keyboard ()
{
				// TODO: zero the old bindings first.
  SG_LOG(SG_INPUT, SG_INFO, "Initializing key bindings");
  SGPropertyNode * key_nodes = fgGetNode("/input/keyboard");
  if (key_nodes == 0) {
    SG_LOG(SG_INPUT, SG_ALERT, "No key bindings (/input/keyboard)!!");
    return;
  }
  
  vector<SGPropertyNode *> keys = key_nodes->getChildren("key");
  for (unsigned int i = 0; i < keys.size(); i++) {
    int index = keys[i]->getIndex();
    SG_LOG(SG_INPUT, SG_INFO, "Binding key " << index);
    _key_bindings[index].is_repeatable = keys[i]->getBoolValue("repeatable");
    _read_bindings(keys[i], _key_bindings[index].bindings, FG_MOD_NONE);
  }
}


void
FGInput::_init_joystick ()
{
				// TODO: zero the old bindings first.
  SG_LOG(SG_INPUT, SG_INFO, "Initializing joystick bindings");
  SGPropertyNode * js_nodes = fgGetNode("/input/joysticks");
  if (js_nodes == 0) {
    SG_LOG(SG_INPUT, SG_ALERT, "No joystick bindings (/input/joysticks)!!");
    return;
  }

  for (int i = 0; i < MAX_JOYSTICKS; i++) {
    const SGPropertyNode * js_node = js_nodes->getChild("js", i);
    if (js_node == 0) {
      SG_LOG(SG_INPUT, SG_ALERT, "No bindings for joystick " << i);
      continue;
    }
    jsJoystick * js = new jsJoystick(i);
    _joystick_bindings[i].js = js;
    if (js->notWorking()) {
      SG_LOG(SG_INPUT, SG_INFO, "Joystick " << i << " not found");
      continue;
    }
#ifdef WIN32
    JOYCAPS jsCaps ;
    joyGetDevCaps( i, &jsCaps, sizeof(jsCaps) );
    int nbuttons = jsCaps.wNumButtons;
    if (nbuttons > MAX_BUTTONS) nbuttons = MAX_BUTTONS;
#else
    int nbuttons = MAX_BUTTONS;
#endif
	
    int naxes = js->getNumAxes();
    if (naxes > MAX_AXES) naxes = MAX_AXES;
    _joystick_bindings[i].naxes = naxes;
    _joystick_bindings[i].nbuttons = nbuttons;

    SG_LOG(SG_INPUT, SG_INFO, "Initializing joystick " << i);

				// Set up range arrays
    float minRange[naxes];
    float maxRange[naxes];
    float center[naxes];

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
    for (int j = 0; j < naxes; j++) {
      const SGPropertyNode * axis_node = js_node->getChild("axis", j);
      if (axis_node == 0) {
	SG_LOG(SG_INPUT, SG_INFO, "No bindings for axis " << j);
	continue;
      }
      
      axis &a = _joystick_bindings[i].axes[j];

      js->setDeadBand(j, axis_node->getDoubleValue("dead-band", 0.0));

      a.tolerance = axis_node->getDoubleValue("tolerance", 0.002);
      minRange[j] = axis_node->getDoubleValue("min-range", minRange[j]);
      maxRange[j] = axis_node->getDoubleValue("max-range", maxRange[j]);
      center[j] = axis_node->getDoubleValue("center", center[j]);

      _read_bindings(axis_node, a.bindings, FG_MOD_NONE);
    }

    //
    // Initialize the buttons.
    //
    for (int j = 0; j < nbuttons; j++) {
      const SGPropertyNode * button_node = js_node->getChild("button", j);
      if (button_node == 0) {
	SG_LOG(SG_INPUT, SG_INFO, "No bindings for button " << j);
	continue;
      }

      button &b = _joystick_bindings[i].buttons[j];
      
      b.is_repeatable =
	button_node->getBoolValue("repeatable", b.is_repeatable);

				// Get the bindings for the button
      _read_bindings(button_node, b.bindings, FG_MOD_NONE);
    }

    js->setMinRange(minRange);
    js->setMaxRange(maxRange);
    js->setCenter(center);
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
  float js_val, diff;
  float axis_values[MAX_AXES];

  for (int i = 0; i < MAX_JOYSTICKS; i++) {

    jsJoystick * js = _joystick_bindings[i].js;
    if (js == 0 || js->notWorking())
      continue;

    js->read(&buttons, axis_values);


				// Fire bindings for the axes.
    for (int j = 0; j < _joystick_bindings[i].naxes; j++) {
      axis &a = _joystick_bindings[i].axes[j];
      
				// Do nothing if the axis position
				// is unchanged; only a change in
				// position fires the bindings.
      if (fabs(axis_values[j] - a.last_value) > a.tolerance) {
// 	SG_LOG(SG_INPUT, SG_INFO, "Axis " << j << " has moved");
	SGPropertyNode node;
	a.last_value = axis_values[j];
// 	SG_LOG(SG_INPUT, SG_INFO, "There are "
// 	       << a.bindings[modifiers].size() << " bindings");
	for (unsigned int k = 0; k < a.bindings[modifiers].size(); k++)
	  a.bindings[modifiers][k].fire(axis_values[j]);
      }
    }

				// Fire bindings for the buttons.
    for (int j = 0; j < _joystick_bindings[i].nbuttons; j++) {
      bool pressed = ((buttons & (1 << j)) > 0);
      button &b = _joystick_bindings[i].buttons[j];

      if (pressed) {
				// The press event may be repeated.
	if (!b.last_state || b.is_repeatable) {
// 	  SG_LOG(SG_INPUT, SG_INFO, "Button " << j << " has been pressed");
	  for (unsigned int k = 0; k < b.bindings[modifiers].size(); k++)
	    b.bindings[modifiers][k].fire();
	}
      } else {
				// The release event is never repeated.
	if (b.last_state)
// 	  SG_LOG(SG_INPUT, SG_INFO, "Button " << j << " has been released");
	  for (int k = 0; k < b.bindings[modifiers|FG_MOD_UP].size(); k++)
	    b.bindings[modifiers|FG_MOD_UP][k].fire();
      }
	  
      b.last_state = pressed;
    }
  }
}


void
FGInput::_read_bindings (const SGPropertyNode * node, 
			 binding_list_t * binding_list,
			 int modifiers)
{
  vector<const SGPropertyNode *> bindings = node->getChildren("binding");
  for (unsigned int i = 0; i < bindings.size(); i++) {
    SG_LOG(SG_INPUT, SG_INFO, "Reading binding "
	   << bindings[i]->getStringValue("command"));
    binding_list[modifiers].push_back(FGBinding(bindings[i]));
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


const vector<FGBinding> &
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

// end of input.cxx
