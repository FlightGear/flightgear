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

#include <simgear/compiler.h>

#include <ctype.h>

#include STL_FSTREAM
#include STL_STRING

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



////////////////////////////////////////////////////////////////////////
// Implementation of FGBinding.
////////////////////////////////////////////////////////////////////////

FGBinding::FGBinding ()
    : _action(ACTION_NONE), _node(0), _adjust_step(0), _assign_value(0)
{
}

FGBinding::FGBinding (const SGPropertyNode * node)
    : _action(ACTION_NONE), _node(0), _adjust_step(0), _assign_value(0)
{
    read(node);
}

FGBinding::~FGBinding ()
{
    // no op
}

void
FGBinding::setAction (Action action)
{
    _action = action;
}

void
FGBinding::setProperty (SGPropertyNode * node)
{
    _node = node;
}

void
FGBinding::setAdjustStep (const SGValue * step)
{
    _adjust_step = step;
}

void
FGBinding::setAssignValue (const SGValue * value)
{
    _assign_value = value;
}

void
FGBinding::read (const SGPropertyNode * node)
{
    if (node->hasValue("action")) {
        string action = node->getStringValue("action");
        if (action == "none")
            _action = ACTION_NONE;
        else if (action == "switch")
            _action = ACTION_SWITCH;
        else if (action == "adjust")
            _action = ACTION_ADJUST;
        else if (action == "assign")
            _action = ACTION_ASSIGN;
        else
            SG_LOG(SG_INPUT, SG_ALERT, "Ignoring unrecognized action type "
                   << action);
    }

    if (node->hasValue("control"))
        _node = fgGetNode(node->getStringValue("control"), true);

    if (node->hasValue("step"))
        _adjust_step = node->getChild("step")->getValue();

    if (node->hasValue("value"))
        _assign_value = node->getChild("value")->getValue();
}

void
FGBinding::fire () const
{
    if (_node == 0) {
        SG_LOG(SG_INPUT, SG_ALERT, "No control property attached to binding");
        return;
    }

    switch (_action) {

    case ACTION_NONE:
        break;

    case ACTION_SWITCH:
        _node->setBoolValue(!_node->getBoolValue());
        break;

    case ACTION_ADJUST:
        if  (_adjust_step == 0) {
            SG_LOG(SG_INPUT, SG_ALERT, "No step provided for adjust binding");
            break;
        }
        switch (_node->getType()) {
        case SGValue::BOOL:
            if (_adjust_step->getBoolValue())
                _node->setBoolValue(!_node->getBoolValue());
            break;
        case SGValue::INT:
            _node->setIntValue(_node->getIntValue() + _adjust_step->getIntValue());
            break;
        case SGValue::LONG:
            _node->setLongValue(_node->getLongValue() + _adjust_step->getLongValue());
            break;
        case SGValue::FLOAT:
            _node->setFloatValue(_node->getFloatValue()
                                 + _adjust_step->getFloatValue());
            break;
        case SGValue::DOUBLE:
        case SGValue::UNKNOWN:	// force to double
            _node->setDoubleValue(_node->getDoubleValue()
                                  + _adjust_step->getDoubleValue());
            break;
        case SGValue::STRING:
            SG_LOG(SG_INPUT, SG_ALERT, "Cannot increment or decrement string value"
                   << _node->getStringValue());
            break;
        }
        break;

    case ACTION_ASSIGN:
        if  (_assign_value == 0) {
            SG_LOG(SG_INPUT, SG_ALERT, "No value provided for assign binding");
            break;
        }
        switch (_node->getType()) {
        case SGValue::BOOL:
            _node->setBoolValue(_assign_value->getBoolValue());
            break;
        case SGValue::INT:
            _node->setIntValue(_assign_value->getIntValue());
            break;
        case SGValue::LONG:
            _node->setLongValue(_assign_value->getLongValue());
            break;
        case SGValue::FLOAT:
            _node->setFloatValue(_assign_value->getFloatValue());
            break;
        case SGValue::DOUBLE:
            _node->setDoubleValue(_assign_value->getDoubleValue());
            break;
        case SGValue::STRING:
            _node->setStringValue(_assign_value->getStringValue());
            break;
        case SGValue::UNKNOWN:
            _node->setUnknownValue(_assign_value->getStringValue());
            break;
        }
        break;
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
    // Read the keyboard bindings.
    // TODO: zero the old bindings first.
    const SGPropertyNode * keyboard =
        globals->get_props()->getNode("/input/keyboard", true);
    vector<const SGPropertyNode *> keys = keyboard->getChildren("key");

    for (unsigned int i = 0; i < keys.size(); i++) {
        int code = keys[i]->getIndex();
        int modifiers = FG_MOD_NONE;
        if (keys[i]->getBoolValue("mod-shift"))
            modifiers |= FG_MOD_SHIFT;
        if (keys[i]->getBoolValue("mod-ctrl"))
            modifiers |= FG_MOD_CTRL;
        if (keys[i]->getBoolValue("mod-alt"))
            modifiers |= FG_MOD_ALT;

        if (code < 0) {
            SG_LOG(SG_INPUT, SG_ALERT, "Key stroke not bound = "
                   << keys[i]->getStringValue("name", "[unnamed]"));
        } else {
            SG_LOG(SG_INPUT, SG_INFO, "Binding key " << code
                   << " with modifiers " << modifiers);
            vector<const SGPropertyNode *> bindings =
                keys[i]->getChildren("binding");
            for (unsigned int j = 0; j < bindings.size(); j++) {
                SG_LOG(SG_INPUT, SG_INFO, "  Adding binding " << j);
                _key_bindings[modifiers][code].push_back(FGBinding(bindings[j]));
            }
        }
    }
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
    // we'll do something here with the joystick
}

void
FGInput::doKey (int k, int modifiers, int x, int y)
{
    float fov, tmp;
    static bool winding_ccw = true;
    int speed;

    SG_LOG(SG_INPUT, SG_INFO, "User pressed key " << k
           << " with modifiers " << modifiers);

    if (_key_bindings[modifiers].find(k) != _key_bindings[modifiers].end()) {
        const vector<FGBinding> &bindings = _key_bindings[modifiers][k];
        for (unsigned int i = 0; i < bindings.size(); i++) {
            bindings[i].fire();
        }
        return;
    }

    SG_LOG(SG_INPUT, SG_INFO, "(No user binding.)");

    // Use the old, default actions.
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
	case 19: // Ctrl-S key
	    current_autopilot->set_AutoThrottleEnabled(
                                                       ! current_autopilot->get_AutoThrottleEnabled()
                                                       );
	    return;
	case 20: // Ctrl-T key
	    current_autopilot->set_AltitudeMode( 
                                                FGAutopilot::FG_ALTITUDE_TERRAIN );
	    current_autopilot->set_AltitudeEnabled(
                                                   ! current_autopilot->get_AltitudeEnabled()
                                                   );
	    return;
	case 49: // numeric keypad 1
	    v->set_goal_view_offset( SGD_PI * 0.75 );
	    return;
	case 50: // numeric keypad 2
	    v->set_goal_view_offset( SGD_PI );
	    return;
	case 51: // numeric keypad 3
	    v->set_goal_view_offset( SGD_PI * 1.25 );
	    return;
	case 52: // numeric keypad 4
	    v->set_goal_view_offset( SGD_PI * 0.50 );
	    return;
	case 54: // numeric keypad 6
	    v->set_goal_view_offset( SGD_PI * 1.50 );
	    return;
	case 55: // numeric keypad 7
	    v->set_goal_view_offset( SGD_PI * 0.25 );
	    return;
	case 56: // numeric keypad 8
	    v->set_goal_view_offset( 0.00 );
	    return;
	case 57: // numeric keypad 9
	    v->set_goal_view_offset( SGD_PI * 1.75 );
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
        // case 256+GLUT_KEY_F9: {
        //     return;
        // }
        case 256+GLUT_KEY_F10: {
            fgToggleFDMdataLogging();
            return;
        }
        // case 256+GLUT_KEY_F11: {
        //     return;
        // }
        // case 256+GLUT_KEY_F12: {
        //     return;
        // }
	case 256+GLUT_KEY_END: // numeric keypad 1
	    v->set_goal_view_offset( SGD_PI * 0.75 );
	    return;
	case 256+GLUT_KEY_DOWN: // numeric keypad 2
	    v->set_goal_view_offset( SGD_PI );
	    return;
	case 256+GLUT_KEY_PAGE_DOWN: // numeric keypad 3
	    v->set_goal_view_offset( SGD_PI * 1.25 );
	    return;
	case 256+GLUT_KEY_LEFT: // numeric keypad 4
	    v->set_goal_view_offset( SGD_PI * 0.50 );
	    return;
	case 256+GLUT_KEY_RIGHT: // numeric keypad 6
	    v->set_goal_view_offset( SGD_PI * 1.50 );
	    return;
	case 256+GLUT_KEY_HOME: // numeric keypad 7
	    v->set_goal_view_offset( SGD_PI * 0.25 );
	    return;
	case 256+GLUT_KEY_UP: // numeric keypad 8
	    v->set_goal_view_offset( 0.00 );
	    return;
	case 256+GLUT_KEY_PAGE_UP: // numeric keypad 9
	    v->set_goal_view_offset( SGD_PI * 1.75 );
	    return;

            // END SPECIALS

	}


    } else {
	SG_LOG( SG_INPUT, SG_DEBUG, "" );
	switch (k) {
	case 98: // b key
	    int b_ret;
	    double b_set;
	    b_ret = int( controls.get_brake( 0 ) );
	    b_set = double(!b_ret);
	    controls.set_brake( FGControls::ALL_WHEELS, b_set);
	    return;
	case 44: // , key
	    if (controls.get_brake(0) > 0.0) {
	        controls.set_brake(0, 0.0);
	    } else {
	        controls.set_brake(0, 1.0);
	    }
	    return;
	case 46: // . key
	    if (controls.get_brake(1) > 0.0) {
	        controls.set_brake(1, 0.0);
	    } else {
	        controls.set_brake(1, 1.0);
	    }
	    return;
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
	case 118: // v key
	    // handles GUI state as well as Viewer LookAt Direction
	    CenterView();
	    globals->set_current_view( globals->get_viewmgr()->next_view() );
	    fgReshape( fgGetInt("/sim/startup/xsize"),
		       fgGetInt("/sim/startup/ysize") );
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
	case 256+GLUT_KEY_F3: // F3 Take a screen shot
	    fgDumpSnapShot();
	    return;
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
FGInput::action (const SGPropertyNode * binding)
{
    const string &action = binding->getStringValue("action", "");
    const string &control = binding->getStringValue("control", "");
    bool repeatable = binding->getBoolValue("repeatable", false);
    int step = binding->getIntValue("step", 0.0);

    if (control == "") {
        SG_LOG(SG_INPUT, SG_ALERT, "No control specified for key "
               << binding->getIndex());
        return;
    }

    else if (action == "") {
        SG_LOG(SG_INPUT, SG_ALERT, "No action specified for key "
               << binding->getIndex());
        return;
    }

    else if (action == "switch") {
        fgSetBool(control, !fgGetBool(control));
    }

    else if (action == "adjust") {
        const SGValue * step = binding->getValue("step");
        if (step == 0) {
            SG_LOG(SG_INPUT, SG_ALERT, "No step supplied for adjust action for key "
                   << binding->getIndex());
            return;
        }
        SGValue * target = fgGetValue(control, true);
        // Use the target's type...
        switch (target->getType()) {
        case SGValue::BOOL:
        case SGValue::INT:
            target->setIntValue(target->getIntValue() + step->getIntValue());
            break;
        case SGValue::LONG:
            target->setLongValue(target->getLongValue() + step->getLongValue());
            break;
        case SGValue::FLOAT:
            target->setFloatValue(target->getFloatValue() + step->getFloatValue());
            break;
        case SGValue::DOUBLE:
        case SGValue::UNKNOWN:	// treat unknown as a double
            target->setDoubleValue(target->getDoubleValue()
                                   + step->getDoubleValue());
            break;
        case SGValue::STRING:
            SG_LOG(SG_INPUT, SG_ALERT, "Failed attempt to adjust string property "
                   << control);
            break;
        }
    }

    else if (action == "assign") {
        const SGValue * value = binding->getValue("value");
        if (value == 0) {
            SG_LOG(SG_INPUT, SG_ALERT, "No value supplied for assign action for key "
                   << binding->getIndex());
            return;
        }
        SGValue * target = fgGetValue(control, true);
        // Use the target's type...
        switch (target->getType()) {
        case SGValue::BOOL:
            target->setBoolValue(value->getBoolValue());
            break;
        case SGValue::INT:
            target->setIntValue(value->getIntValue());
            break;
        case SGValue::LONG:
            target->setLongValue(value->getLongValue());
            break;
        case SGValue::FLOAT:
            target->setFloatValue(value->getFloatValue());
            break;
        case SGValue::DOUBLE:
            target->setDoubleValue(value->getDoubleValue());
            break;
        case SGValue::STRING:
            target->setStringValue(value->getStringValue());
            break;
        case SGValue::UNKNOWN:
            target->setUnknownValue(value->getStringValue());
            break;
        }
    }

    else {
        SG_LOG(SG_INPUT, SG_ALERT, "Unknown action " << action
               << " for key " << binding->getIndex());
    }
}


#if 0
FGViewer *v = globals->get_current_view();

SG_LOG( SG_INPUT, SG_DEBUG, "Special key hit = " << k );

if ( GLUT_ACTIVE_SHIFT && glutGetModifiers() ) {
    SG_LOG( SG_INPUT, SG_DEBUG, " SHIFTED" );
    switch (k)
        } else
        }

#endif


// end of input.cxx
