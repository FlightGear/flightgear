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

#include <GL/glut.h>
#include <XGL/xgl.h>

#if defined(FX) && defined(XMESA)
#include <GL/xmesa.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <pu.h>			// plib include

#include <Debug/logstream.hxx>
#include <Aircraft/aircraft.hxx>
#include <Astro/solarsystem.hxx>
#include <Astro/sky.hxx>
#include <Autopilot/autopilot.hxx>
#include <Cockpit/hud.hxx>
#include <GUI/gui.h>
#include <Include/fg_constants.h>
#include <Misc/fgpath.hxx>
#include <Scenery/tilemgr.hxx>
#include <Objects/materialmgr.hxx>
#include <Time/fg_time.hxx>
#include <Time/light.hxx>

#ifdef FG_NEW_WEATHER
#  include <WeatherCM/FGLocalWeatherDatabase.h>
#else
#  include <Weather/weather.hxx>
#endif

#include "keyboard.hxx"
#include "options.hxx"
#include "views.hxx"

extern void NewAltitude( puObject *cb );
extern void NewHeading( puObject *cb );

// Force an update of the sky and lighting parameters
static void local_update_sky_and_lighting_params( void ) {
    // fgSunInit();
    SolarSystem::theSolarSystem->rebuild();
    cur_light_params.Update();
    fgSkyColorsInit();
}


// Handle keyboard events
void GLUTkey(unsigned char k, int x, int y) {
    FGInterface *f;
    FGTime *t;
    FGView *v;
    float fov, tmp;
    static bool winding_ccw = true;
    int speed;

    f = current_aircraft.fdm_state;
    t = FGTime::cur_time_params;
    v = &current_view;

    FG_LOG( FG_INPUT, FG_DEBUG, "Key hit = " << k );
    if ( puKeyboard(k, PU_DOWN) ) {
	return;
    }

    if ( GLUT_ACTIVE_ALT && glutGetModifiers() ) {
	FG_LOG( FG_INPUT, FG_DEBUG, " SHIFTED" );
	switch (k) {
	case 1: // Ctrl-A key
	    fgAPToggleAltitude();
	    return;
	case 8: // Ctrl-H key
	    fgAPToggleHeading();
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
	    fgAPToggleAutoThrottle();
	    return;
	case 20: // Ctrl-T key
	    fgAPToggleTerrainFollow();
	    return;
	case 49: // numeric keypad 1
	    v->set_goal_view_offset( FG_PI * 0.75 );
	    return;
	case 50: // numeric keypad 2
	    v->set_goal_view_offset( FG_PI );
	    return;
	case 51: // numeric keypad 3
	    v->set_goal_view_offset( FG_PI * 1.25 );
	    return;
	case 52: // numeric keypad 4
	    v->set_goal_view_offset( FG_PI * 0.50 );
	    return;
	case 54: // numeric keypad 6
	    v->set_goal_view_offset( FG_PI * 1.50 );
	    return;
	case 55: // numeric keypad 7
	    v->set_goal_view_offset( FG_PI * 0.25 );
	    return;
	case 56: // numeric keypad 8
	    v->set_goal_view_offset( 0.00 );
	    return;
	case 57: // numeric keypad 9
	    v->set_goal_view_offset( FG_PI * 1.75 );
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
	    t->adjust_warp(-60);
	    local_update_sky_and_lighting_params();
	    return;
	case 80: // P key
	    current_options.toggle_panel();
	    break;
	case 84: // T key
	    t->adjust_warp_delta(-30);
	    local_update_sky_and_lighting_params();
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
#ifdef FG_NEW_WEATHER
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
	    if( fgAPAltitudeEnabled() || fgAPTerrainFollowEnabled() ) {
		fgAPAltitudeAdjust( 100 );
	    } else {
		controls.move_elevator(-0.05);
	    }
	    return;
	case 56: // numeric keypad 8
	    if( fgAPAltitudeEnabled() || fgAPTerrainFollowEnabled() ) {
		fgAPAltitudeAdjust( -100 );
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
	    if( fgAPHeadingEnabled() ) {
		fgAPHeadingAdjust( -1 );
	    } else {
		controls.move_rudder(-0.05);
	    }
	    return;
	case 13: // numeric keypad Enter
	    if( fgAPHeadingEnabled() ) {
		fgAPHeadingAdjust( 1 );
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
	    if( fgAPAutoThrottleEnabled() ) {
		fgAPAutoThrottleAdjust( 5 );
	    } else {
		controls.move_throttle( FGControls::ALL_ENGINES, 0.01 );
	    }
	    return;
	case 51: // numeric keypad 3 (Pg Dn)
	    if( fgAPAutoThrottleEnabled() ) {
		fgAPAutoThrottleAdjust( -5 );
	    } else {
		controls.move_throttle( FGControls::ALL_ENGINES, -0.01 );
	    }
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
	case 104: // h key
	    HUD_brightkey( false );
	    return;
	case 105: // i key
	    fgHUDInit(&current_aircraft);  // normal HUD
	    return;
	case 109: // m key
	    t->adjust_warp (+60);
	    local_update_sky_and_lighting_params();
	    return;
	case 112: // p key
	    t->togglePauseMode();

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
	    t->adjust_warp_delta (+30);
	    local_update_sky_and_lighting_params();
	    return;
	case 118: // v key
	    current_options.cycle_view_mode();
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
#ifdef FG_NEW_WEATHER
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
		    "Program exiting normally at user request." );
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
	case GLUT_KEY_END: // numeric keypad 1
	    v->set_goal_view_offset( FG_PI * 0.75 );
	    return;
	case GLUT_KEY_DOWN: // numeric keypad 2
	    v->set_goal_view_offset( FG_PI );
	    return;
	case GLUT_KEY_PAGE_DOWN: // numeric keypad 3
	    v->set_goal_view_offset( FG_PI * 1.25 );
	    return;
	case GLUT_KEY_LEFT: // numeric keypad 4
	    v->set_goal_view_offset( FG_PI * 0.50 );
	    return;
	case GLUT_KEY_RIGHT: // numeric keypad 6
	    v->set_goal_view_offset( FG_PI * 1.50 );
	    return;
	case GLUT_KEY_HOME: // numeric keypad 7
	    v->set_goal_view_offset( FG_PI * 0.25 );
	    return;
	case GLUT_KEY_UP: // numeric keypad 8
	    v->set_goal_view_offset( 0.00 );
	    return;
	case GLUT_KEY_PAGE_UP: // numeric keypad 9
	    v->set_goal_view_offset( FG_PI * 1.75 );
	    return;
	}
    } else {
        FG_LOG( FG_INPUT, FG_DEBUG, "" );
	switch (k) {
	case GLUT_KEY_F2: // F2 Reload Tile Cache...
	    {
		int toggle_pause;
		FG_LOG(FG_INPUT, FG_INFO, "ReIniting TileCache");
		FGTime *t = FGTime::cur_time_params;
		if( (toggle_pause = !t->getPause()) )
		    t->togglePauseMode();
		BusyCursor(0);
		if( global_tile_mgr.init() ) {
		    // Load the local scenery data
		    global_tile_mgr.update();
		} else {
		    FG_LOG( FG_GENERAL, FG_ALERT, 
			    "Error in Tile Manager initialization!" );
		    exit(-1);
		}
		BusyCursor(1);
		if(toggle_pause)
		    t->togglePauseMode();
		return;
	    }
        case GLUT_KEY_F6: // F6 toggles Autopilot target location
                fgAPToggleWayPoint();
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
	    if ( material_mgr.loaded() ) {
 	        if (current_options.get_textures()) {
 		    current_options.set_textures(false);
 		    glDisable(GL_TEXTURE_2D);
 		    ssgOverrideTexture(true);
 		} else {
		    current_options.set_textures(true);
 		    glEnable(GL_TEXTURE_2D);
 		    ssgOverrideTexture(false);
		}
		FG_LOG( FG_INPUT, FG_INFO, "Toggling texture" );
	    } else {
		FG_LOG( FG_INPUT, FG_INFO, 
			"No textures loaded, cannot toggle" );
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
	    if( fgAPAltitudeEnabled() || fgAPTerrainFollowEnabled() ) {
		fgAPAltitudeAdjust( -100 );
	    } else {
		controls.move_elevator(0.05);
	    }
	    return;
	case GLUT_KEY_DOWN:
	    if( fgAPAltitudeEnabled() || fgAPTerrainFollowEnabled() ) {
		fgAPAltitudeAdjust( 100 );
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
	    if( fgAPHeadingEnabled() ) {
		fgAPHeadingAdjust( -1 );
	    } else {
		controls.move_rudder(-0.05);
	    }
	    return;
	case 13: // numeric keypad Enter
	    if( fgAPHeadingEnabled() ) {
		fgAPHeadingAdjust( 1 );
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
	    if( fgAPAutoThrottleEnabled() ) {
		fgAPAutoThrottleAdjust( 5 );
	    } else {
		controls.move_throttle( FGControls::ALL_ENGINES, 0.01 );
	    }
	    return;
	case GLUT_KEY_PAGE_DOWN: // numeric keypad 3 (Pg Dn)
	    if( fgAPAutoThrottleEnabled() ) {
		fgAPAutoThrottleAdjust( -5 );
	    } else {
		controls.move_throttle( FGControls::ALL_ENGINES, -0.01 );
	    }
	    return;
	}
    }
}


