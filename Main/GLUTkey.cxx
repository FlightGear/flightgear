// GLUTkey.cxx -- handle GLUT keyboard events
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@me.umn.edu
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
// (Log is kept at end of this file)


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>
#include <stdio.h>
#include <stdlib.h>

#include <Aircraft/aircraft.h>
#include <Astro/solarsystem.hxx>
#include <Astro/sky.hxx>
#include <Autopilot/autopilot.hxx>
#include <Cockpit/hud.hxx>
#include <Debug/fg_debug.h>
#include <GUI/gui.h>
#include <Include/fg_constants.h>
#include <Objects/material.hxx>
#include <PUI/pu.h>
#include <Time/light.hxx>
#include <Weather/weather.h>

#include "GLUTkey.hxx"
#include "options.hxx"
#include "views.hxx"

#if defined(FX) && defined(XMESA)
#  include <GL/xmesa.h>
   static int fullscreen = 1;
#endif


// Force an update of the sky and lighting parameters
static void local_update_sky_and_lighting_params( void ) {
    // fgSunInit();
    SolarSystem::theSolarSystem->rebuild();
    cur_light_params.Update();
    fgSkyColorsInit();
}


// Handle keyboard events
void GLUTkey(unsigned char k, int x, int y) {
    fgCONTROLS *c;
    fgFLIGHT *f;
    fgTIME *t;
    fgVIEW *v;
    struct fgWEATHER *w;
    float fov, tmp;

    c = current_aircraft.controls;
    f = current_aircraft.flight;
    t = &cur_time_params;
    v = &current_view;
    w = &current_weather;

    fgPrintf( FG_INPUT, FG_DEBUG, "Key hit = %d", k);
    puKeyboard(k, PU_DOWN );

    if ( GLUT_ACTIVE_ALT && glutGetModifiers() ) {
	fgPrintf( FG_INPUT, FG_DEBUG, " SHIFTED\n");
	switch (k) {
	case 1: // Ctrl-A key
	    fgAPToggleAltitude();
	    return;
	case 8: // Ctrl-H key
	    fgAPToggleHeading();
	    return;
	case 19: // Ctrl-S key
	    fgAPToggleAutoThrottle();
	    return;
	case 20: // Ctrl-T key
	    fgAPToggleTerrainFollow();
	    return;
	case 49: // numeric keypad 1
	    v->goal_view_offset = FG_PI * 0.75;
	    return;
	case 50: // numeric keypad 2
	    v->goal_view_offset = FG_PI;
	    return;
	case 51: // numeric keypad 3
	    v->goal_view_offset = FG_PI * 1.25;
	    return;
	case 52: // numeric keypad 4
	    v->goal_view_offset = FG_PI * 0.50;
	    return;
	case 54: // numeric keypad 6
	    v->goal_view_offset = FG_PI * 1.50;
	    return;
	case 55: // numeric keypad 7
	    v->goal_view_offset = FG_PI * 0.25;
	    return;
	case 56: // numeric keypad 8
	    v->goal_view_offset = 0.00;
	    return;
	case 57: // numeric keypad 9
	    v->goal_view_offset = FG_PI * 1.75;
	    return;
	case 72: // H key
	    // status = current_options.get_hud_status();
	    // current_options.set_hud_status(!status);
	    HUD_brightkey( true );
	    return;
	case 73: // i key
	    // Minimal Hud
	    fgHUDInit2(&current_aircraft);
	    return;
	case 77: // M key
	    t->warp -= 60;
	    local_update_sky_and_lighting_params();
	    return;
	case 84: // T key
	    t->warp_delta -= 30;
	    local_update_sky_and_lighting_params();
	    return;
	case 87: // W key
#if defined(FX) && !defined(WIN32)
	    fullscreen = ( !fullscreen );
#if defined(XMESA_FX_FULLSCREEN) && defined(XMESA_FX_WINDOW)
	    XMesaSetFXmode(fullscreen ? XMESA_FX_FULLSCREEN : XMESA_FX_WINDOW);
#endif
#endif
	    return;
	case 88: // X key
	    fov = current_options.get_fov();
	    fov *= 1.05;
	    if ( fov > FG_FOV_MAX ) {
		fov = FG_FOV_MAX;
	    }
	    current_options.set_fov(fov);
	    v->update_fov = TRUE;
	    return;
	case 90: // Z key
	    tmp = fgWeatherGetVisibility();   // in meters
	    tmp /= 1.10;
	    fgWeatherSetVisibility( tmp );
	    return;
	}
    } else {
	fgPrintf( FG_INPUT, FG_DEBUG, "\n");
	switch (k) {
	case 50: // numeric keypad 2
	    fgElevMove(-0.05);
	    return;
	case 56: // numeric keypad 8
	    fgElevMove(0.05);
	    return;
	case 49: // numeric keypad 1
	    fgElevTrimMove(-0.001);
	    return;
	case 55: // numeric keypad 7
	    fgElevTrimMove(0.001);
	    return;
	case 52: // numeric keypad 4
	    fgAileronMove(-0.05);
	    return;
	case 54: // numeric keypad 6
	    fgAileronMove(0.05);
	    return;
	case 48: // numeric keypad Ins
	    fgRudderMove(-0.05);
	    return;
	case 13: // numeric keypad Enter
	    fgRudderMove(0.05);
	    return;
	case 53: // numeric keypad 5
	    fgAileronSet(0.0);
	    fgElevSet(0.0);
	    fgRudderSet(0.0);
	    return;
	case 57: // numeric keypad 9 (Pg Up)
	    fgThrottleMove(0, 0.01);
	    return;
	case 51: // numeric keypad 3 (Pg Dn)
	    fgThrottleMove(0, -0.01);
	    return;
	case 98: // b key
	    int b_ret;
	    double b_set;
	    b_ret = int( fgBrakeGet() );
	    b_set = double(!b_ret);
	    fgBrakeSet(b_set);
	    return;
	case 104: // h key
	    HUD_brightkey( false );
	    return;
	case 105: // i key
	    fgHUDInit(&current_aircraft);  // normal HUD
	    return;
	case 109: // m key
	    t->warp += 60;
	    local_update_sky_and_lighting_params();
	    return;
	case 112: // p key
	    t->pause = !t->pause;
	    // printf position and attitude information
	    fgPrintf( FG_INPUT, FG_INFO,
		      "Lon = %.4f  Lat = %.4f  Altitude = %.1f\n", 
		      FG_Longitude * RAD_TO_DEG,
		      FG_Latitude * RAD_TO_DEG,
		      FG_Altitude * FEET_TO_METER);
	    fgPrintf( FG_INPUT, FG_INFO,
		      "Heading = %.2f  Roll = %.2f  Pitch = %.2f\n", 
		      FG_Psi * RAD_TO_DEG,
		      FG_Phi * RAD_TO_DEG,
		      FG_Theta * RAD_TO_DEG);
	    return;
	case 116: // t key
	    t->warp_delta += 30;
	    local_update_sky_and_lighting_params();
	    return;
	case 120: // X key
	    fov = current_options.get_fov();
	    fov /= 1.05;
	    if ( fov < FG_FOV_MIN ) {
		fov = FG_FOV_MIN;
	    }
	    current_options.set_fov(fov);
	    v->update_fov = TRUE;
	    return;
	case 122: // z key
	    tmp = fgWeatherGetVisibility();   // in meters
	    tmp *= 1.10;
	    fgWeatherSetVisibility( tmp );
	    return;
	case 27: // ESC
	    // if( fg_DebugOutput ) {
	    //   fclose( fg_DebugOutput );
	    // }
	    fgPrintf( FG_INPUT, FG_EXIT, 
		      "Program exiting normally at user request.\n");
	}
    }
}


// Handle "special" keyboard events
void GLUTspecialkey(int k, int x, int y) {
    fgCONTROLS *c;
    fgVIEW *v;

    c = current_aircraft.controls;
    v = &current_view;

    fgPrintf( FG_INPUT, FG_DEBUG, "Special key hit = %d", k);
    puKeyboard(k + PU_KEY_GLUT_SPECIAL_OFFSET, PU_DOWN);

    if ( GLUT_ACTIVE_SHIFT && glutGetModifiers() ) {
	fgPrintf( FG_INPUT, FG_DEBUG, " SHIFTED\n");
	switch (k) {
	case GLUT_KEY_END: // numeric keypad 1
	    v->goal_view_offset = FG_PI * 0.75;
	    return;
	case GLUT_KEY_DOWN: // numeric keypad 2
	    v->goal_view_offset = FG_PI;
	    return;
	case GLUT_KEY_PAGE_DOWN: // numeric keypad 3
	    v->goal_view_offset = FG_PI * 1.25;
	    return;
	case GLUT_KEY_LEFT: // numeric keypad 4
	    v->goal_view_offset = FG_PI * 0.50;
	    return;
	case GLUT_KEY_RIGHT: // numeric keypad 6
	    v->goal_view_offset = FG_PI * 1.50;
	    return;
	case GLUT_KEY_HOME: // numeric keypad 7
	    v->goal_view_offset = FG_PI * 0.25;
	    return;
	case GLUT_KEY_UP: // numeric keypad 8
	    v->goal_view_offset = 0.00;
	    return;
	case GLUT_KEY_PAGE_UP: // numeric keypad 9
	    v->goal_view_offset = FG_PI * 1.75;
	    return;
	}
    } else {
        fgPrintf( FG_INPUT, FG_DEBUG, "\n");
	switch (k) {
 	case GLUT_KEY_F8: // F8 toggles fog ... off fastest nicest...
	    current_options.cycle_fog();
	
	    if ( current_options.get_fog() == fgOPTIONS::FG_FOG_DISABLED ) {
		fgPrintf( FG_INPUT, FG_INFO, "Fog disabled\n" );
	    } else if ( current_options.get_fog() == 
			fgOPTIONS::FG_FOG_FASTEST )
	    {
		fgPrintf( FG_INPUT, FG_INFO, 
			  "Fog enabled, hint set to fastest\n" );
	    } else if ( current_options.get_fog() ==
			fgOPTIONS::FG_FOG_NICEST )
	    {
		fgPrintf( FG_INPUT, FG_INFO,
			  "Fog enabled, hint set to nicest\n" );
	    }

 	    return;
 	case GLUT_KEY_F9: // F9 toggles textures on and off...
	    if ( material_mgr.get_textures_loaded() ) {
		current_options.get_textures() ?
		    current_options.set_textures(false) :
		    current_options.set_textures(true);
		fgPrintf( FG_INPUT, FG_INFO, "Toggling texture\n" );
	    } else {
		fgPrintf( FG_INPUT, FG_INFO, 
			  "No textures loaded, cannot toggle\n" );
	    }
 	    return;
	case GLUT_KEY_F10: // F10 toggles menu on and off...
	    fgPrintf(FG_INPUT, FG_INFO, "Invoking call back function");
	    hideMenuButton -> 
		setValue ((int) !(hideMenuButton -> getValue() ) );
	    hideMenuButton -> invokeCallback();
	    //exit(1);
	    return;
	case GLUT_KEY_UP:
	    fgElevMove(0.05);
	    return;
	case GLUT_KEY_DOWN:
	    fgElevMove(-0.05);
	    return;
	case GLUT_KEY_LEFT:
	    fgAileronMove(-0.05);
	    return;
	case GLUT_KEY_RIGHT:
	    fgAileronMove(0.05);
	    return;
	case GLUT_KEY_HOME: // numeric keypad 1
	    fgElevTrimMove(0.001);
	    return;
	case GLUT_KEY_END: // numeric keypad 7
	    fgElevTrimMove(-0.001);
	    return;
	case GLUT_KEY_INSERT: // numeric keypad Ins
	    fgRudderMove(-0.05);
	    return;
	case 13: // numeric keypad Enter
	    fgRudderMove(0.05);
	    return;
	case 53: // numeric keypad 5
	    fgAileronSet(0.0);
	    fgElevSet(0.0);
	    fgRudderSet(0.0);
	    return;
	case GLUT_KEY_PAGE_UP: // numeric keypad 9 (Pg Up)
	    fgThrottleMove(0, 0.01);
	    return;
	case GLUT_KEY_PAGE_DOWN: // numeric keypad 3 (Pg Dn)
	    fgThrottleMove(0, -0.01);
	    return;
	}
    }
}


// $Log$
// Revision 1.27  1998/10/02 12:46:46  curt
// Added an "auto throttle"
//
// Revision 1.26  1998/10/01 00:38:04  curt
// More altitude hold tweaks.
//
// Revision 1.25  1998/09/29 02:03:36  curt
// Autopilot mods.
//
// Revision 1.24  1998/09/26 13:16:44  curt
// C++-ified the comments.
//
// Revision 1.23  1998/09/17 18:35:30  curt
// Added F8 to toggle fog and F9 to toggle texturing.
//
// Revision 1.22  1998/09/15 04:27:27  curt
// Changes for new Astro code.
//
// Revision 1.21  1998/08/29 13:09:25  curt
// Changes to event manager from Bernie Bright.
//
// Revision 1.20  1998/08/24 20:11:12  curt
// Added i/I to toggle full vs. minimal HUD.
// Added a --hud-tris vs --hud-culled option.
// Moved options accessor funtions to options.hxx.
//
// Revision 1.19  1998/08/05 00:19:33  curt
// Added a local routine to update lighting params every frame when time is
// accelerated.
//
// Revision 1.18  1998/07/30 23:48:24  curt
// Output position & orientation when pausing.
// Eliminated libtool use.
// Added options to specify initial position and orientation.
// Changed default fov to 55 degrees.
// Added command line option to start in paused or unpaused state.
//
// Revision 1.17  1998/07/27 18:41:23  curt
// Added a pause command "p"
// Fixed some initialization order problems between pui and glut.
// Added an --enable/disable-sound option.
//
// Revision 1.16  1998/07/16 17:33:34  curt
// "H" / "h" now control hud brightness as well with off being one of the
//   states.
// Better checking for xmesa/fx 3dfx fullscreen/window support for deciding
//   whether or not to build in the feature.
// Translucent menu support.
// HAVE_AUDIO_SUPPORT -> ENABLE_AUDIO_SUPPORT
// Use fork() / wait() for playing mp3 init music in background under unix.
// Changed default tile diameter to 5.
//
// Revision 1.15  1998/07/13 21:01:34  curt
// Wrote access functions for current fgOPTIONS.
//
// Revision 1.14  1998/07/06 02:42:02  curt
// Added support for switching between fullscreen and window mode for
// Mesa/3dfx/glide.
//
// Added a basic splash screen.  Restructured the main loop and top level
// initialization routines to do this.
//
// Hacked in some support for playing a startup mp3 sound file while rest
// of sim initializes.  Currently only works in Unix using the mpg123 player.
// Waits for the mpg123 player to finish before initializing internal
// sound drivers.
//
// Revision 1.13  1998/06/27 16:54:32  curt
// Replaced "extern displayInstruments" with a entry in fgOPTIONS.
// Don't change the view port when displaying the panel.
//
// Revision 1.12  1998/06/12 14:27:26  curt
// Pui -> PUI, Gui -> GUI.
//
// Revision 1.11  1998/06/12 00:57:38  curt
// Added support for Pui/Gui.
// Converted fog to GL_FOG_EXP2.
// Link to static simulator parts.
// Update runfg.bat to try to be a little smarter.
//
// Revision 1.10  1998/05/27 02:24:05  curt
// View optimizations by Norman Vine.
//
// Revision 1.9  1998/05/16 13:05:21  curt
// Added limits to fov.
//
// Revision 1.8  1998/05/13 18:29:56  curt
// Added a keyboard binding to dynamically adjust field of view.
// Added a command line option to specify fov.
// Adjusted terrain color.
// Root path info moved to fgOPTIONS.
// Added ability to parse options out of a config file.
//
// Revision 1.7  1998/05/07 23:14:14  curt
// Added "D" key binding to set autopilot heading.
// Made frame rate calculation average out over last 10 frames.
// Borland C++ floating point exception workaround.
// Added a --tile-radius=n option.
//
// Revision 1.6  1998/04/28 01:20:20  curt
// Type-ified fgTIME and fgVIEW.
// Added a command line option to disable textures.
//
// Revision 1.5  1998/04/25 22:06:29  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.4  1998/04/25 20:24:00  curt
// Cleaned up initialization sequence to eliminate interdependencies
// between sun position, lighting, and view position.  This creates a
// valid single pass initialization path.
//
// Revision 1.3  1998/04/24 14:19:29  curt
// Fog tweaks.
//
// Revision 1.2  1998/04/24 00:49:17  curt
// Wrapped "#include <config.h>" in "#ifdef HAVE_CONFIG_H"
// Trying out some different option parsing code.
// Some code reorganization.
//
// Revision 1.1  1998/04/22 13:25:40  curt
// C++ - ifing the code.
// Starting a bit of reorganization of lighting code.
//
// Revision 1.33  1998/04/18 04:11:25  curt
// Moved fg_debug to it's own library, added zlib support.
//
// Revision 1.32  1998/04/14 02:21:01  curt
// Incorporated autopilot heading hold contributed by:  Jeff Goeke-Smith
// <jgoeke@voyager.net>
//
// Revision 1.31  1998/04/08 23:34:05  curt
// Patch from Durk to fix trim reversal with numlock key active.
//
// Revision 1.30  1998/04/03 22:09:02  curt
// Converting to Gnu autoconf system.
//
// Revision 1.29  1998/02/07 15:29:40  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.28  1998/02/03 23:20:23  curt
// Lots of little tweaks to fix various consistency problems discovered by
// Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
// passed arguments along to the real printf().  Also incorporated HUD changes
// by Michele America.
//
// Revision 1.27  1998/01/27 00:47:55  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.26  1998/01/19 19:27:07  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.25  1998/01/05 18:44:34  curt
// Add an option to advance/decrease time from keyboard.
//
// Revision 1.24  1997/12/30 16:36:46  curt
// Merged in Durk's changes ...
//
// Revision 1.23  1997/12/15 23:54:44  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.22  1997/12/10 22:37:45  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.21  1997/08/27 21:32:23  curt
// Restructured view calculation code.  Added stars.
//
// Revision 1.20  1997/08/27 03:30:13  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.19  1997/08/25 20:27:21  curt
// Merged in initial HUD and Joystick code.
//
// Revision 1.18  1997/08/22 21:34:38  curt
// Doing a bit of reorganizing and house cleaning.
//
// Revision 1.17  1997/07/19 22:34:02  curt
// Moved PI definitions to ../constants.h
// Moved random() stuff to ../Utils/ and renamed fg_random()
//
// Revision 1.16  1997/07/18 23:41:24  curt
// Tweaks for building with Cygnus Win32 compiler.
//
// Revision 1.15  1997/07/16 20:04:47  curt
// Minor tweaks to aid Win32 port.
//
// Revision 1.14  1997/07/12 03:50:20  curt
// Added an #include <Windows32/Base.h> to help compiling for Win32
//
// Revision 1.13  1997/06/25 15:39:46  curt
// Minor changes to compile with rsxnt/win32.
//
// Revision 1.12  1997/06/21 17:12:52  curt
// Capitalized subdirectory names.
//
// Revision 1.11  1997/06/18 04:10:31  curt
// A couple more runway tweaks ...
//
// Revision 1.10  1997/06/18 02:21:23  curt
// Hacked in a runway
//
// Revision 1.9  1997/06/02 03:40:06  curt
// A tiny bit more view tweaking.
//
// Revision 1.8  1997/06/02 03:01:38  curt
// Working on views (side, front, back, transitions, etc.)
//
// Revision 1.7  1997/05/31 19:16:25  curt
// Elevator trim added.
//
// Revision 1.6  1997/05/31 04:13:52  curt
// WE CAN NOW FLY!!!
//
// Continuing work on the LaRCsim flight model integration.
// Added some MSFS-like keyboard input handling.
//
// Revision 1.5  1997/05/30 23:26:19  curt
// Added elevator/aileron controls.
//
// Revision 1.4  1997/05/27 17:44:31  curt
// Renamed & rearranged variables and routines.   Added some initial simple
// timer/alarm routines so the flight model can be updated on a regular 
// interval.
//
// Revision 1.3  1997/05/23 15:40:25  curt
// Added GNU copyright headers.
// Fog now works!
//
// Revision 1.2  1997/05/23 00:35:12  curt
// Trying to get fog to work ...
//
// Revision 1.1  1997/05/21 15:57:50  curt
// Renamed due to added GLUT support.
//
// Revision 1.2  1997/05/19 18:22:41  curt
// Parameter tweaking ... starting to stub in fog support.
//
// Revision 1.1  1997/05/16 16:05:51  curt
// Initial revision.

