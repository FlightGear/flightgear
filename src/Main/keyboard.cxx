// keyboard.cxx -- handle GLUT keyboard events
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997 - 1999  Curtis L. Olson  - curt@flightgear.org
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

#include <GL/glut.h>
#include <simgear/xgl/xgl.h>

#if defined(FX) && defined(XMESA)
#include <GL/xmesa.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include STL_FSTREAM

#include <plib/pu.h>		// plib include

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgpath.hxx>

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

#include "bfi.hxx"
#include "globals.hxx"
#include "keyboard.hxx"
#include "options.hxx"
#include "save.hxx"
#include "views.hxx"

				// From main.cxx
extern void fgReshape( int width, int height );


// Handle keyboard events
void GLUTkey(unsigned char k, int x, int y) {
    FGInterface *f;
    SGTime *t;
    FGView *v;
    float fov, tmp;
    static bool winding_ccw = true;
    int speed;

    f = current_aircraft.fdm_state;
    v = &current_view;

    FG_LOG( FG_INPUT, FG_DEBUG, "Key hit = " << k );
    if ( puKeyboard(k, PU_DOWN) ) {
	return;
    }

    if ( GLUT_ACTIVE_ALT && glutGetModifiers() ) {
	FG_LOG( FG_INPUT, FG_DEBUG, " SHIFTED" );
	switch (k) {
	case 1: // Ctrl-A key
	    current_autopilot->set_AltitudeMode( 
                  FGAutopilot::FG_ALTITUDE_LOCK );
	    current_autopilot->set_AltitudeEnabled(
		  ! current_autopilot->get_AltitudeEnabled()
	        );
	    return;
	case 7: // Ctrl-G key
	    current_autopilot->set_AltitudeMode( 
                  FGAutopilot::FG_ALTITUDE_GS1 );
	    current_autopilot->set_AltitudeEnabled(
		  ! current_autopilot->get_AltitudeEnabled()
	        );
	    return;
	case 8: // Ctrl-H key
	    current_autopilot->set_HeadingMode( 
                  FGAutopilot::FG_HEADING_LOCK );
	    current_autopilot->set_HeadingEnabled(
		  ! current_autopilot->get_HeadingEnabled()
	        );
	    return;
	case 14: // Ctrl-N key
	    current_autopilot->set_HeadingMode( 
                  FGAutopilot::FG_HEADING_NAV1 );
	    current_autopilot->set_HeadingEnabled(
		  ! current_autopilot->get_HeadingEnabled()
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
	case 21: // Ctrl-U key
	    // add 1000' of emergency altitude.  Possibly good for 
	    // unflipping yourself :-)
	    {
		double alt = cur_fdm_state->get_Altitude() + 1000;
		fgFDMForceAltitude( current_options.get_flight_model(), 
				    alt * FEET_TO_METER );
	    }
	    return;
	case 49: // numeric keypad 1
	    v->set_goal_view_offset( FG_PI * 0.75 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(-25.0, 25.0, 1.0);
	      v->set_view_offset( FG_PI * 0.75 );
	    }
	    return;
	case 50: // numeric keypad 2
	    v->set_goal_view_offset( FG_PI );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(-25.0, 0.0, 1.0);
	      v->set_view_offset( FG_PI );
	    }
	    return;
	case 51: // numeric keypad 3
	    v->set_goal_view_offset( FG_PI * 1.25 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(-25.0, -25.0, 1.0);
	      v->set_view_offset( FG_PI * 1.25 );
	    }
	    return;
	case 52: // numeric keypad 4
	    v->set_goal_view_offset( FG_PI * 0.50 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(0.0, 25.0, 1.0);
	      v->set_view_offset( FG_PI * 0.50 );
	    }
	    return;
	case 54: // numeric keypad 6
	    v->set_goal_view_offset( FG_PI * 1.50 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(0.0, -25.0, 1.0);
	      v->set_view_offset( FG_PI * 1.50 );
	    }
	    return;
	case 55: // numeric keypad 7
	    v->set_goal_view_offset( FG_PI * 0.25 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(25.0, 25.0, 1.0);
	      v->set_view_offset( FG_PI * 0.25 );
	    }
	    return;
	case 56: // numeric keypad 8
	    v->set_goal_view_offset( 0.00 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(25.0, 0.0, 1.0);
	      v->set_view_offset( 0.00 );
	    }
	    return;
	case 57: // numeric keypad 9
	    v->set_goal_view_offset( FG_PI * 1.75 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(25.0, -25.0, 1.0);
	      v->set_view_offset( FG_PI * 1.75 );
	    }
	    return;
	case 65: // A key
	    speed = current_options.get_speed_up();
	    speed--;
	    if ( speed < 1 ) {
		speed = 1;
	    }
	    current_options.set_speed_up( speed );
	    return;
	case 72: // H key
	    // status = current_options.get_hud_status();
	    // current_options.set_hud_status(!status);
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
	case 80: // P key
	    current_options.toggle_panel();
	    break;
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
	    fov = current_options.get_fov();
	    fov *= 1.05;
	    if ( fov > FG_FOV_MAX ) {
		fov = FG_FOV_MAX;
	    }
	    current_options.set_fov(fov);
	    v->force_update_fov_math();
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
	}
    } else {
	FG_LOG( FG_INPUT, FG_DEBUG, "" );
	switch (k) {
	case 50: // numeric keypad 2
	    if ( current_autopilot->get_AltitudeEnabled() ) {
		current_autopilot->AltitudeAdjust( 100 );
	    } else {
		controls.move_elevator(-0.05);
	    }
	    return;
	case 56: // numeric keypad 8
	    if ( current_autopilot->get_AltitudeEnabled() ) {
		current_autopilot->AltitudeAdjust( -100 );
	    } else {
		controls.move_elevator(0.05);
	    }
	    return;
	case 49: // numeric keypad 1
	    controls.move_elevator_trim(-0.001);
	    return;
	case 55: // numeric keypad 7
	    controls.move_elevator_trim(0.001);
	    return;
	case 52: // numeric keypad 4
	    controls.move_aileron(-0.05);
	    return;
	case 54: // numeric keypad 6
	    controls.move_aileron(0.05);
	    return;
	case 48: // numeric keypad Ins
	    if ( current_autopilot->get_HeadingEnabled() ) {
		current_autopilot->HeadingAdjust( -1 );
	    } else {
		controls.move_rudder(-0.05);
	    }
	    return;
	case 13: // numeric keypad Enter
	    if ( current_autopilot->get_HeadingEnabled() ) {
		current_autopilot->HeadingAdjust( 1 );
	    } else {
		controls.move_rudder(0.05);
	    }
	    return;
	case 53: // numeric keypad 5
	    controls.set_aileron(0.0);
	    controls.set_elevator(0.0);
	    controls.set_rudder(0.0);
	    return;
	case 57: // numeric keypad 9 (Pg Up)
	    if ( current_autopilot->get_AutoThrottleEnabled() ) {
		current_autopilot->AutoThrottleAdjust( 5 );
	    } else {
		controls.move_throttle( FGControls::ALL_ENGINES, 0.01 );
	    }
	    return;
	case 51: // numeric keypad 3 (Pg Dn)
	    if ( current_autopilot->get_AutoThrottleEnabled() ) {
		current_autopilot->AutoThrottleAdjust( -5 );
	    } else {
		controls.move_throttle( FGControls::ALL_ENGINES, -0.01 );
	    }
	    return;
 	case 91: // [ key
 	    controls.move_flaps(-0.34);
 	    FG_LOG( FG_INPUT, FG_INFO,
		    "Set flaps to " << controls.get_flaps() );
 	    return;
 	case 93: // ] key
 	    controls.move_flaps(0.34);
 	    FG_LOG( FG_INPUT, FG_INFO,
		    "Set flaps to " << controls.get_flaps() );
 	    return;
	case 97: // a key
	    speed = current_options.get_speed_up();
	    speed++;
	    current_options.set_speed_up( speed );
	    return;
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
		FGBucket p( f->get_Longitude() * RAD_TO_DEG,
			    f->get_Latitude() * RAD_TO_DEG );
		FGPath tile_path( current_options.get_fg_root() );
		tile_path.append( "Scenery" );
		tile_path.append( p.gen_base_path() );
		tile_path.append( p.gen_index_str() );

		// printf position and attitude information
		FG_LOG( FG_INPUT, FG_INFO,
			"Lon = " << f->get_Longitude() * RAD_TO_DEG
			<< "  Lat = " << f->get_Latitude() * RAD_TO_DEG
			<< "  Altitude = " << f->get_Altitude() * FEET_TO_METER
			);
		FG_LOG( FG_INPUT, FG_INFO,
			"Heading = " << f->get_Psi() * RAD_TO_DEG 
			<< "  Roll = " << f->get_Phi() * RAD_TO_DEG
			<< "  Pitch = " << f->get_Theta() * RAD_TO_DEG );
		FG_LOG( FG_INPUT, FG_INFO, tile_path.c_str());
	    }
	    return;
	case 116: // t key
	    globals->inc_warp_delta( 30 );
	    fgUpdateSkyAndLightingParams();
	    return;
	case 118: // v key
// 	    current_options.cycle_view_mode();
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      current_options.set_view_mode(fgOPTIONS::FG_VIEW_PILOT);
	      v->set_goal_view_offset( 0.0 );
	      v->set_view_offset( 0.0 );
	    } else if (current_options.get_view_mode() ==
		       fgOPTIONS::FG_VIEW_PILOT) {
	      current_options.set_view_mode(fgOPTIONS::FG_VIEW_FOLLOW);
	      v->set_goal_view_offset( FG_PI * 1.75 );
	      v->set_view_offset( FG_PI * 1.75 );
	      pilot_view.set_pilot_offset(25.0, -25.0, 1.0);
	    }
	    fgReshape( current_view.get_winWidth(),
		       current_view.get_winHeight() );
	    return;
	case 120: // x key
	    fov = current_options.get_fov();
	    fov /= 1.05;
	    if ( fov < FG_FOV_MIN ) {
		fov = FG_FOV_MIN;
	    }
	    current_options.set_fov(fov);
	    v->force_update_fov_math();
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
	    FG_LOG( FG_INPUT, FG_ALERT, 
		    "Program exit requested." );
	    ConfirmExitDialog();
	    return;
	}
    }
}


// Handle "special" keyboard events
void GLUTspecialkey(int k, int x, int y) {
    FGView *v;

    v = &current_view;

    FG_LOG( FG_INPUT, FG_DEBUG, "Special key hit = " << k );

    if ( puKeyboard(k + PU_KEY_GLUT_SPECIAL_OFFSET, PU_DOWN) ) {
	return;
    }

    if ( GLUT_ACTIVE_SHIFT && glutGetModifiers() ) {
	FG_LOG( FG_INPUT, FG_DEBUG, " SHIFTED" );
	switch (k) {
 	case GLUT_KEY_F1: {
 	    ifstream input("fgfs.sav");
 	    if (input.good() && fgLoadFlight(input)) {
   	        input.close();
 		FG_LOG(FG_INPUT, FG_INFO, "Restored flight from fgfs.sav");
 	    } else {
 	        FG_LOG(FG_INPUT, FG_ALERT, "Cannot load flight from fgfs.sav");
 	    }
 	    return;
 	}
 	case GLUT_KEY_F2: {
 	    FG_LOG(FG_INPUT, FG_INFO, "Saving flight");
 	    ofstream output("fgfs.sav");
 	    if (output.good() && fgSaveFlight(output)) {
 		output.close();
 		FG_LOG(FG_INPUT, FG_INFO, "Saved flight to fgfs.sav");
 	    } else {
 	        FG_LOG(FG_INPUT, FG_ALERT, "Cannot save flight to fgfs.sav");
 	    }
 	    return;
 	}
	case GLUT_KEY_F3: {
	  string panel_path =
	    current_properties.getStringValue("/sim/panel/path",
					      "Panels/Default/default.xml");
	  FGPanel * new_panel = fgReadPanel(panel_path);
	  if (new_panel == 0) {
	    FG_LOG(FG_INPUT, FG_ALERT,
		   "Error reading new panel from " << panel_path);
	    return;
	  }
	  FG_LOG(FG_INPUT, FG_INFO, "Loaded new panel from " << panel_path);
	  delete current_panel;
	  current_panel = new_panel;
	  return;
	}
	case GLUT_KEY_F4: {
	  FGPath props_path(current_options.get_fg_root());
	  props_path.append("preferences.xml");
	  FG_LOG(FG_INPUT, FG_INFO, "Rereading global preferences");
	  if (!readPropertyList(props_path.str(), &current_properties)) {
	    FG_LOG(FG_INPUT, FG_ALERT,
		   "Failed to reread global preferences from "
		   << props_path.str());
	  } else {
	    FG_LOG(FG_INPUT, FG_INFO, "Finished Reading global preferences");
	  }
	  return;
	}
	case GLUT_KEY_F5: {
	  current_panel->setYOffset(current_panel->getYOffset() - 5);
	  fgReshape(current_view.get_winWidth(),
		    current_view.get_winHeight());
	  return;
	}
	case GLUT_KEY_F6: {
	  current_panel->setYOffset(current_panel->getYOffset() + 5);
	  fgReshape(current_view.get_winWidth(),
		    current_view.get_winHeight());
	  return;
	}
	case GLUT_KEY_F7: {
	  current_panel->setXOffset(current_panel->getXOffset() - 5);
	  return;
	}
	case GLUT_KEY_F8: {
	  current_panel->setXOffset(current_panel->getXOffset() + 5);
	  return;
	}
	case GLUT_KEY_END: // numeric keypad 1
	    v->set_goal_view_offset( FG_PI * 0.75 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(-25.0, 25.0, 1.0);
	      v->set_view_offset( FG_PI * 0.75 );
	    }
	    return;
	case GLUT_KEY_DOWN: // numeric keypad 2
	    v->set_goal_view_offset( FG_PI );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(-25.0, 0.0, 1.0);
	      v->set_view_offset( FG_PI );
	    }
	    return;
	case GLUT_KEY_PAGE_DOWN: // numeric keypad 3
	    v->set_goal_view_offset( FG_PI * 1.25 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(-25.0, -25.0, 1.0);
	      v->set_view_offset( FG_PI * 1.25 );
	    }
	    return;
	case GLUT_KEY_LEFT: // numeric keypad 4
	    v->set_goal_view_offset( FG_PI * 0.50 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(0.0, 25.0, 1.0);
	      v->set_view_offset( FG_PI * 0.50 );
	    }
	    return;
	case GLUT_KEY_RIGHT: // numeric keypad 6
	    v->set_goal_view_offset( FG_PI * 1.50 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(0.0, -25.0, 1.0);
	      v->set_view_offset( FG_PI * 1.50 );
	    }
	    return;
	case GLUT_KEY_HOME: // numeric keypad 7
	    v->set_goal_view_offset( FG_PI * 0.25 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(25.0, 25.0, 1.0);
	      v->set_view_offset( FG_PI * 0.25 );
	    }
	    return;
	case GLUT_KEY_UP: // numeric keypad 8
	    v->set_goal_view_offset( 0.00 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(25.0, 0.0, 1.0);
	      v->set_view_offset( 0.00 );
	    }
	    return;
	case GLUT_KEY_PAGE_UP: // numeric keypad 9
	    v->set_goal_view_offset( FG_PI * 1.75 );
	    if (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_FOLLOW) {
	      pilot_view.set_pilot_offset(25.0, -25.0, 1.0);
	      v->set_view_offset( FG_PI * 1.75 );
	    }
	    return;
	}
    } else {
        FG_LOG( FG_INPUT, FG_DEBUG, "" );
	switch (k) {
	case GLUT_KEY_F2: // F2 Reload Tile Cache...
	    {
		bool freeze;
		FG_LOG(FG_INPUT, FG_INFO, "ReIniting TileCache");
		if ( !freeze ) 
		    globals->set_freeze( true );
		BusyCursor(0);
		if ( global_tile_mgr.init() ) {
		    // Load the local scenery data
		    global_tile_mgr.update( 
		        cur_fdm_state->get_Longitude() * RAD_TO_DEG,
			cur_fdm_state->get_Latitude() * RAD_TO_DEG );
		} else {
		    FG_LOG( FG_GENERAL, FG_ALERT, 
			    "Error in Tile Manager initialization!" );
		    exit(-1);
		}
		BusyCursor(1);
		if ( !freeze )
		   globals->set_freeze( false );
		return;
	    }
	case GLUT_KEY_F3: // F3 Take a screen shot
	    fgDumpSnapShot();
	    return;
        case GLUT_KEY_F6: // F6 toggles Autopilot target location
	    if ( current_autopilot->get_HeadingMode() !=
		 FGAutopilot::FG_HEADING_WAYPOINT ) {
		current_autopilot->set_HeadingMode(
		    FGAutopilot::FG_HEADING_WAYPOINT );
		current_autopilot->set_HeadingEnabled( true );
	    } else {
		current_autopilot->set_HeadingMode(
		    FGAutopilot::FG_HEADING_LOCK );
	    }
	    return;
	case GLUT_KEY_F8: // F8 toggles fog ... off fastest nicest...
	    current_options.cycle_fog();
	
	    if ( current_options.get_fog() == fgOPTIONS::FG_FOG_DISABLED ) {
		FG_LOG( FG_INPUT, FG_INFO, "Fog disabled" );
	    } else if ( current_options.get_fog() == 
			fgOPTIONS::FG_FOG_FASTEST )
	    {
		FG_LOG( FG_INPUT, FG_INFO, 
			"Fog enabled, hint set to fastest" );
	    } else if ( current_options.get_fog() ==
			fgOPTIONS::FG_FOG_NICEST )
	    {
		FG_LOG( FG_INPUT, FG_INFO,
			"Fog enabled, hint set to nicest" );
	    }

 	    return;
 	case GLUT_KEY_F9: // F9 toggles textures on and off...
	    FG_LOG( FG_INPUT, FG_INFO, "Toggling texture" );
	    if ( current_options.get_textures() ) {
		current_options.set_textures( false );
		material_lib.set_step( 1 );
	    } else {
		current_options.set_textures( true );
		material_lib.set_step( 0 );
	    }
 	    return;
	case GLUT_KEY_F10: // F10 toggles menu on and off...
	    FG_LOG(FG_INPUT, FG_INFO, "Invoking call back function");
	    guiToggleMenu();
	    return;
	case GLUT_KEY_F11: // F11 Altitude Dialog.
	    FG_LOG(FG_INPUT, FG_INFO, "Invoking Altitude call back function");
	    NewAltitude( NULL );
	    return;
	case GLUT_KEY_F12: // F12 Heading Dialog...
	    FG_LOG(FG_INPUT, FG_INFO, "Invoking Heading call back function");
	    NewHeading( NULL );
	    return;
	case GLUT_KEY_UP:
	    if ( current_autopilot->get_AltitudeEnabled() ) {
		current_autopilot->AltitudeAdjust( -100 );
	    } else {
		controls.move_elevator(0.05);
	    }
	    return;
	case GLUT_KEY_DOWN:
	    if ( current_autopilot->get_AltitudeEnabled() ) {
		current_autopilot->AltitudeAdjust( 100 );
	    } else {
		controls.move_elevator(-0.05);
	    }
	    return;
	case GLUT_KEY_LEFT:
	    controls.move_aileron(-0.05);
	    return;
	case GLUT_KEY_RIGHT:
	    controls.move_aileron(0.05);
	    return;
	case GLUT_KEY_HOME: // numeric keypad 1
	    controls.move_elevator_trim(0.001);
	    return;
	case GLUT_KEY_END: // numeric keypad 7
	    controls.move_elevator_trim(-0.001);
	    return;
	case GLUT_KEY_INSERT: // numeric keypad Ins
	    if ( current_autopilot->get_HeadingEnabled() ) {
		current_autopilot->HeadingAdjust( -1 );
	    } else {
		controls.move_rudder(-0.05);
	    }
	    return;
	case 13: // numeric keypad Enter
	    if ( current_autopilot->get_HeadingEnabled() ) {
		current_autopilot->HeadingAdjust( 1 );
	    } else {
		controls.move_rudder(0.05);
	    }
	    return;
	case 53: // numeric keypad 5
	    controls.set_aileron(0.0);
	    controls.set_elevator(0.0);
	    controls.set_rudder(0.0);
	    return;
	case GLUT_KEY_PAGE_UP: // numeric keypad 9 (Pg Up)
	    if ( current_autopilot->get_AutoThrottleEnabled() ) {
		current_autopilot->AutoThrottleAdjust( 5 );
	    } else {
		controls.move_throttle( FGControls::ALL_ENGINES, 0.01 );
	    }
	    return;
	case GLUT_KEY_PAGE_DOWN: // numeric keypad 3 (Pg Dn)
	    if ( current_autopilot->get_AutoThrottleEnabled() ) {
		current_autopilot->AutoThrottleAdjust( -5 );
	    } else {
		controls.move_throttle( FGControls::ALL_ENGINES, -0.01 );
	    }
	    return;
	}
    }
}


