/**************************************************************************
 * hud.h -- hud defines and prototypes (initial draft)
 *
 * Written by Michele America, started September 1997.
 *
 * Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
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


#ifndef _HUD_H
#define _HUD_H


#include <Aircraft/aircraft.h>
#include <Flight/flight.h>
#include <Controls/controls.h>

// View mode definitions

enum VIEW_MODES { HUD_VIEW, PANEL_VIEW, CHASE_VIEW, TOWER_VIEW };

// Hud general constants
#define DAY                1
#define NIGHT              2
#define BRT_DARK           3
#define BRT_MEDIUM         4
#define BRT_LIGHT          5
#define SIZE_SMALL         6
#define SIZE_LARGE         7

// Instrument types
#define ARTIFICIAL_HORIZON	1
#define SCALE              2
#define LADDER             3
#define LABEL              4

// Scale constants
#define HORIZONTAL         1
#define TOP                2
#define BOTTOM             3
#define VERTICAL           4
#define LEFT               5
#define RIGHT              6
#define LIMIT              7
#define NOLIMIT            8
#define ROUNDROB           9

// Label constants
#define SMALL              1
#define LARGE              2
#define BLINK              3
#define NOBLINK            4
#define LEFT_JUST          5
#define CENTER_JUST        6
#define RIGHT_JUST         7

// Ladder constants
#define NONE               1
#define UPPER_LEFT         2
#define UPPER_CENTER       3
#define UPPER_RIGHT        4
#define CENTER_RIGHT       5
#define LOWER_RIGHT        6
#define LOWER_CENTER       7
#define LOWER_LEFT         8
#define CENTER_LEFT        9
#define SOLID_LINES       10
#define DASHED_LINES      11
#define DASHED_NEG_LINES  12

// Ladder orientaion
// #define HUD_VERTICAL        1
// #define HUD_HORIZONTAL		2
// #define HUD_FREEFLOAT		3

// Ladder orientation modes
// #define HUD_LEFT    		1
// #define HUD_RIGHT         	2
// #define HUD_TOP           	1
// #define HUD_BOTTOM        	2
// #define HUD_V_LEFT    		1
// #define HUD_V_RIGHT         	2
// #define HUD_H_TOP           	1
// #define HUD_H_BOTTOM        	2


// Ladder sub-types
// #define HUD_LIM				1
// #define HUD_NOLIM			2
// #define HUD_CIRC			3

// #define HUD_INSTR_LADDER	1
// #define HUD_INSTR_CLADDER	2
// #define HUD_INSTR_HORIZON	3
// #define HUD_INSTR_LABEL		4

// The following structs will become classes with a derivation from
// an ABC instrument_pack. Eventually the instruments may well become
// dll's. This would open the instrumentation issue to all commers.
//
// Methods Needed:
//    Constructor()
//    Initialization();  // For dynamic scenario settups?
//    Update();          // Follow the data changes.
//    Repaint();         // Respond to uncover/panel repaints.
//    Break();           // Show a frown.
//    Fix();             // Return to normal appearance and function.
//    Night_Day();       // Illumination changes appearance/bitmaps.
//

typedef struct  {
  int type;
  int sub_type;
  int scr_pos;
  int scr_min;
  int scr_max;
  int div_min;
  int div_max;
  int orientation;
  int minimum_value;
  int maximum_value;
  int width_units;
  double (*load_value)( void );
}HUD_scale,  *pHUDscale;

typedef struct  {
	int type;
	int scr_pos;
	int scr_min;
	int scr_max;
	int div_min;
	int div_max;
	int orientation;
	int label_position;
	int width_units;
	double (*load_value)( void );
}HUD_circular_scale, *pHUD_circscale;

typedef struct  {
	int type;
	int x_pos;
	int y_pos;
	int scr_width;
	int scr_height;
	int scr_hole;
	int div_units;
	int label_position;
	int width_units;
	double (*load_roll)( void );
	double (*load_pitch)( void );
}HUD_ladder, *pHUDladder;

typedef struct {
	int scr_min;
	int scr_max;
	int div_min;
	int div_max;
	int orientation;
	int label_position;
	int width_units;
	double (*load_value)( void );
} HUD_circular_ladder, *pHUDcircladder;

#define HORIZON_FIXED	1
#define HORIZON_MOVING	2

typedef struct{
	int type;
	int x_pos;
	int y_pos;
	int scr_width;
	int scr_hole;
	int tee_height;
	double (*load_roll)( void );
	double (*load_sideslip)( void );
} HUD_horizon, *pHUDhorizon;

typedef struct {
  int x_pos;
  int y_pos;
  double(*load_value)(void);
} HUD_control_surfaces, *pHUDControlSurfaces;

typedef struct {
  int ctrl_x;
  int ctrl_y;
  int ctrl_length;
  int orientation;
  int alignment;
  int min_value;
  int max_value;
  int width_units;
  double (*load_value)(void);
} HUD_control, *pHUDControl;
#define LABEL_COUNTER	1
#define LABEL_WARNING	2

typedef struct {
	int type;
	int x_pos;
	int y_pos;
	int size;
	int blink;
	int justify;
	char *pre_str;
	char *post_str;
	char *format;
	double (*load_value)( void ); // pointer to routine to get the data
} HUD_label, *pHUDlabel;

// Removed union HUD_instr_data to evolve this to oop code.

typedef enum{ HUDno_instr,
                   HUDscale,
                   HUDcirc_scale,
                   HUDladder,
                   HUDcirc_ladder,
                   HUDhorizon,
                   HUDlabel,
                   HUDcontrol_surfaces,
                   HUDcontrol
                   } hudinstype;

typedef struct HUD_INSTR_STRUCT{
  hudinstype  type;
  int sub_type;
  int orientation;
  void *instr;   // For now we will cast this pointer accoring to the value
                 // of the type member.
  struct HUD_INSTR_STRUCT *next;
} HUD_instr, *HIptr;

typedef struct  {
	int code;
	HIptr instruments;
	int status;
	int time_of_day;
	int brightness;
	int size; // possibly another name for this ? (michele)
}HUD, *Hptr;

Hptr fgHUDInit      ( fgAIRCRAFT *cur_aircraft );

void fgHUDSetTimeMode( Hptr hud, int time_of_day );
void fgHUDSetBrightness( Hptr hud, int brightness );

Hptr fgHUDAddHorizon( Hptr hud,
                      int x_pos,
                      int y_pos,
                      int length,
                      int hole_len,
                      int tee_height,
                      double (*load_roll)( void ),
                      double (*load_sideslip)( void ) );

Hptr fgHUDAddScale  ( Hptr hud,                    \
                      int type,                    \
                      int subtype,                 \
                      int scr_pos,                 \
                      int scr_min,                 \
                      int scr_max,                 \
                      int div_min,                 \
                      int div_max,                 \
                      int orientation,             \
                      int min_value,               \
                      int max_value,               \
                      int width_units,             \
                      double (*load_value)( void ) );

Hptr fgHUDAddLabel  ( Hptr hud,                    \
                      int x_pos,                   \
                      int y_pos,                   \
                      int size,                    \
                      int blink,                   \
                      int justify,                 \
                      char *pre_str,               \
                      char *post_str,              \
                      char *format,                \
                      double (*load_value)( void ) );

Hptr fgHUDAddLadder ( Hptr hud,                    \
                      int x_pos,                   \
                      int y_pos,                   \
                      int scr_width,               \
                      int scr_height,              \
                      int hole_len,                \
                      int div_units,               \
                      int label_pos,               \
                      int max_value,               \
                      double (*load_roll)( void ), \
                      double (*load_pitch)( void ) );

Hptr fgHUDAddControlSurfaces( Hptr hud,                    \
                              int x_pos,                   \
                              int y_pos,                   \
                              double (*load_value)( void) );

Hptr fgHUDAddControl( Hptr hud,                    \
                      int ctrl_x,                  \
                      int ctrl_y,                  \
                      int ctrl_length,             \
                      int orientation,             \
                      int alignment,               \
                      int min_value,               \
                      int max_value,               \
                      int width_units,             \
                      double (*load_value)( void) );

/*
Hptr fgHUDAddLadder ( Hptr hud,
                      int scr_min,
                      int scr_max,
                      int div_min,
                      int div_max, \
					            int orientation,
                      int max_value,
                      double *(load_value);

Hptr fgHUDAddCircularLadder( Hptr hud,
                             int scr_min,
                             int scr_max,
                             int div_min,
                             int div_max, \
					                   int max_value,
                             double *(load_value) );

Hptr fgHUDAddNumDisp( Hptr hud,
                      int x_pos,
                      int y_pos,
                      int size,
                      int blink, \
					            char *pre_str,
                      char *post_str,
                      double *(load_value) );
*/

void fgUpdateHUD ( Hptr hud );
void fgUpdateHUD2( Hptr hud ); // Future use?

#endif // _HUD_H  

/* $Log$
/* Revision 1.13  1998/02/20 00:16:22  curt
/* Thursday's tweaks.
/*
 * Revision 1.12  1998/02/19 13:05:52  curt
 * Incorporated some HUD tweaks from Michelle America.
 * Tweaked the sky's sunset/rise colors.
 * Other misc. tweaks.
 *
 * Revision 1.11  1998/02/16 13:38:42  curt
 * Integrated changes from Charlie Hotchkiss.
 *
 * Revision 1.10  1998/02/12 21:59:42  curt
 * Incorporated code changes contributed by Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.8  1998/02/07 15:29:35  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.7  1998/02/03 23:20:15  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.6  1998/01/22 02:59:30  curt
 * Changed #ifdef FILE_H to #ifdef _FILE_H
 *
 * Revision 1.5  1998/01/19 19:27:01  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.4  1998/01/19 18:40:21  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.3  1997/12/30 16:36:41  curt
 * Merged in Durk's changes ...
 *
 * Revision 1.2  1997/12/10 22:37:40  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.1  1997/08/29 18:03:22  curt
 * Initial revision.
 *
 */
