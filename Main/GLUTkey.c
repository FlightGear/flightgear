/**************************************************************************
 * tkglkey.c -- handle tkgl keyboard events
 *
 * Written by Curtis Olson, started May 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifdef WIN32
#  include <windows.h>                     
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>
#include <stdio.h>
#include <stdlib.h>

#include <Main/GLUTkey.h>
#include <Main/fg_debug.h>
#include <Main/views.h>

#include <Include/fg_constants.h>

#include <Aircraft/aircraft.h>
#include <Weather/weather.h>


extern int show_hud;             /* HUD state */
extern int displayInstruments;


/* Handle keyboard events */
void GLUTkey(unsigned char k, int x, int y) {
    struct fgCONTROLS *c;
    struct fgTIME *t;
    struct fgVIEW *v;
    struct fgWEATHER *w;

    c = &current_aircraft.controls;
    t = &cur_time_params;
    v = &current_view;
    w = &current_weather;

    fgPrintf( FG_INPUT, FG_DEBUG, "Key hit = %d", k);

    if ( GLUT_ACTIVE_ALT && glutGetModifiers() ) {
	fgPrintf( FG_INPUT, FG_DEBUG, " SHIFTED\n");
	switch (k) {
	case 49: /* numeric keypad 1 */
	    v->goal_view_offset = FG_PI * 0.75;
	    return;
	case 50: /* numeric keypad 2 */
	    v->goal_view_offset = FG_PI;
	    return;
	case 51: /* numeric keypad 3 */
	    v->goal_view_offset = FG_PI * 1.25;
	    return;
	case 52: /* numeric keypad 4 */
	    v->goal_view_offset = FG_PI * 0.50;
	    return;
	case 54: /* numeric keypad 6 */
	    v->goal_view_offset = FG_PI * 1.50;
	    return;
	case 55: /* numeric keypad 7 */
	    v->goal_view_offset = FG_PI * 0.25;
	    return;
	case 56: /* numeric keypad 8 */
	    v->goal_view_offset = 0.00;
	    return;
	case 57: /* numeric keypad 9 */
	    v->goal_view_offset = FG_PI * 1.75;
	    return;
	case 72: /* H key */
	    show_hud = !show_hud;
	    return;
	case 77: /* M key */
	    t->warp -= 60;
	    return;
	case 84: /* T key */
	    t->warp_delta -= 30;
	    return;
	case 87: /* W key */
	    displayInstruments = !displayInstruments;
	    return;
	case 90: /* Z key */
	    w->visibility /= 1.10;
	    xglFogf(GL_FOG_END, w->visibility);
	    fgPrintf( FG_INPUT, FG_DEBUG, "Fog density = %.4f\n", w->visibility);
	    return;
	}
    } else {
	fgPrintf( FG_INPUT, FG_DEBUG, "\n");
	switch (k) {
	case 50: /* numeric keypad 2 */
	    fgElevMove(-0.05);
	    return;
	case 56: /* numeric keypad 8 */
	    fgElevMove(0.05);
	    return;
	case 49: /* numeric keypad 1 */
	    fgElevTrimMove(-0.001);
	    return;
	case 55: /* numeric keypad 7 */
	    fgElevTrimMove(0.001);
	    return;
	case 52: /* numeric keypad 4 */
	    fgAileronMove(-0.05);
	    return;
	case 54: /* numeric keypad 6 */
	    fgAileronMove(0.05);
	    return;
	case 48: /* numeric keypad Ins */
	    fgRudderMove(-0.05);
	    return;
	case 13: /* numeric keypad Enter */
	    fgRudderMove(0.05);
	    return;
	case 53: /* numeric keypad 5 */
	    fgAileronSet(0.0);
	    fgElevSet(0.0);
	    fgRudderSet(0.0);
	    return;
	case 57: /* numeric keypad 9 (Pg Up) */
	    fgThrottleMove(0, 0.01);
	    return;
	case 51: /* numeric keypad 3 (Pg Dn) */
	    fgThrottleMove(0, -0.01);
	    return;
	case 109: /* m key */
	    t->warp += 60;
	    return;
	case 116: /* t key */
	    t->warp_delta += 30;
	    return;
	case 122: /* z key */
	    w->visibility *= 1.10;
	    xglFogf(GL_FOG_END, w->visibility);
	    fgPrintf( FG_INPUT, FG_DEBUG, "Fog density = %.4f\n", w->visibility);
	    return;
	case 27: /* ESC */
	    exit(0);
	}
    }

}


/* Handle "special" keyboard events */
void GLUTspecialkey(int k, int x, int y) {
    struct fgCONTROLS *c;
    struct fgVIEW *v;

    c = &current_aircraft.controls;
    v = &current_view;

    fgPrintf( FG_INPUT, FG_DEBUG, "Special key hit = %d", k);

    if ( GLUT_ACTIVE_SHIFT && glutGetModifiers() ) {
	fgPrintf( FG_INPUT, FG_DEBUG, " SHIFTED\n");
	switch (k) {
	case GLUT_KEY_END: /* numeric keypad 1 */
	    v->goal_view_offset = FG_PI * 0.75;
	    return;
	case GLUT_KEY_DOWN: /* numeric keypad 2 */
	    v->goal_view_offset = FG_PI;
	    return;
	case GLUT_KEY_PAGE_DOWN: /* numeric keypad 3 */
	    v->goal_view_offset = FG_PI * 1.25;
	    return;
	case GLUT_KEY_LEFT: /* numeric keypad 4 */
	    v->goal_view_offset = FG_PI * 0.50;
	    return;
	case GLUT_KEY_RIGHT: /* numeric keypad 6 */
	    v->goal_view_offset = FG_PI * 1.50;
	    return;
	case GLUT_KEY_HOME: /* numeric keypad 7 */
	    v->goal_view_offset = FG_PI * 0.25;
	    return;
	case GLUT_KEY_UP: /* numeric keypad 8 */
	    v->goal_view_offset = 0.00;
	    return;
	case GLUT_KEY_PAGE_UP: /* numeric keypad 9 */
	    v->goal_view_offset = FG_PI * 1.75;
	    return;
	}
    } else {
        fgPrintf( FG_INPUT, FG_DEBUG, "\n");
	switch (k) {
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
	case GLUT_KEY_HOME: /* numeric keypad 1 */
	    fgElevTrimMove(-0.001);
	    return;
	case GLUT_KEY_END: /* numeric keypad 7 */
	    fgElevTrimMove(0.001);
	    return;
	case GLUT_KEY_INSERT: /* numeric keypad Ins */
	    fgRudderMove(-0.05);
	    return;
	case 13: /* numeric keypad Enter */
	    fgRudderMove(0.05);
	    return;
	case 53: /* numeric keypad 5 */
	    fgAileronSet(0.0);
	    fgElevSet(0.0);
	    fgRudderSet(0.0);
	    return;
	case GLUT_KEY_PAGE_UP: /* numeric keypad 9 (Pg Up) */
	    fgThrottleMove(0, 0.01);
	    return;
	case GLUT_KEY_PAGE_DOWN: /* numeric keypad 3 (Pg Dn) */
	    fgThrottleMove(0, -0.01);
	    return;
	}
    }
}


/* $Log$
/* Revision 1.28  1998/02/03 23:20:23  curt
/* Lots of little tweaks to fix various consistency problems discovered by
/* Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
/* passed arguments along to the real printf().  Also incorporated HUD changes
/* by Michele America.
/*
 * Revision 1.27  1998/01/27 00:47:55  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.26  1998/01/19 19:27:07  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.25  1998/01/05 18:44:34  curt
 * Add an option to advance/decrease time from keyboard.
 *
 * Revision 1.24  1997/12/30 16:36:46  curt
 * Merged in Durk's changes ...
 *
 * Revision 1.23  1997/12/15 23:54:44  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.22  1997/12/10 22:37:45  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.21  1997/08/27 21:32:23  curt
 * Restructured view calculation code.  Added stars.
 *
 * Revision 1.20  1997/08/27 03:30:13  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.19  1997/08/25 20:27:21  curt
 * Merged in initial HUD and Joystick code.
 *
 * Revision 1.18  1997/08/22 21:34:38  curt
 * Doing a bit of reorganizing and house cleaning.
 *
 * Revision 1.17  1997/07/19 22:34:02  curt
 * Moved PI definitions to ../constants.h
 * Moved random() stuff to ../Utils/ and renamed fg_random()
 *
 * Revision 1.16  1997/07/18 23:41:24  curt
 * Tweaks for building with Cygnus Win32 compiler.
 *
 * Revision 1.15  1997/07/16 20:04:47  curt
 * Minor tweaks to aid Win32 port.
 *
 * Revision 1.14  1997/07/12 03:50:20  curt
 * Added an #include <Windows32/Base.h> to help compiling for Win32
 *
 * Revision 1.13  1997/06/25 15:39:46  curt
 * Minor changes to compile with rsxnt/win32.
 *
 * Revision 1.12  1997/06/21 17:12:52  curt
 * Capitalized subdirectory names.
 *
 * Revision 1.11  1997/06/18 04:10:31  curt
 * A couple more runway tweaks ...
 *
 * Revision 1.10  1997/06/18 02:21:23  curt
 * Hacked in a runway
 *
 * Revision 1.9  1997/06/02 03:40:06  curt
 * A tiny bit more view tweaking.
 *
 * Revision 1.8  1997/06/02 03:01:38  curt
 * Working on views (side, front, back, transitions, etc.)
 *
 * Revision 1.7  1997/05/31 19:16:25  curt
 * Elevator trim added.
 *
 * Revision 1.6  1997/05/31 04:13:52  curt
 * WE CAN NOW FLY!!!
 *
 * Continuing work on the LaRCsim flight model integration.
 * Added some MSFS-like keyboard input handling.
 *
 * Revision 1.5  1997/05/30 23:26:19  curt
 * Added elevator/aileron controls.
 *
 * Revision 1.4  1997/05/27 17:44:31  curt
 * Renamed & rearranged variables and routines.   Added some initial simple
 * timer/alarm routines so the flight model can be updated on a regular interval.
 *
 * Revision 1.3  1997/05/23 15:40:25  curt
 * Added GNU copyright headers.
 * Fog now works!
 *
 * Revision 1.2  1997/05/23 00:35:12  curt
 * Trying to get fog to work ...
 *
 * Revision 1.1  1997/05/21 15:57:50  curt
 * Renamed due to added GLUT support.
 *
 * Revision 1.2  1997/05/19 18:22:41  curt
 * Parameter tweaking ... starting to stub in fog support.
 *
 * Revision 1.1  1997/05/16 16:05:51  curt
 * Initial revision.
 *
 */
