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


#include "../Aircraft/aircraft.h"
#include "../Flight/flight.h"
#include "../Controls/controls.h"


/* Instrument types */
#define ARTIFICIAL_HORIZON	1
#define SCALE				2
#define LADDER				3
#define LABEL				4

/* Scale constants */
#define HORIZONTAL			1
#define TOP					2
#define	BOTTOM				3
#define VERTICAL			4
#define LEFT				5
#define RIGHT				6
#define	LIMIT				7
#define NOLIMIT				8

/* Label constants */
#define SMALL				1
#define LARGE				2
#define BLINK				3
#define NOBLINK				4
#define LEFT_JUST			5
#define CENTER_JUST			6
#define RIGHT_JUST			7

/* Ladder constants */
#define NONE				1
#define UPPER_LEFT			2
#define UPPER_CENTER		3
#define UPPER_RIGHT			4
#define CENTER_RIGHT		5
#define LOWER_RIGHT			6
#define LOWER_CENTER		7
#define LOWER_LEFT			8
#define CENTER_LEFT			9
#define SOLID_LINES			10
#define DASHED_LINES		11
#define DASHED_NEG_LINES	12

/* Ladder orientaion */
// #define HUD_VERTICAL        1
// #define HUD_HORIZONTAL		2
// #define HUD_FREEFLOAT		3

/* Ladder orientation modes */
// #define HUD_LEFT    		1
// #define HUD_RIGHT         	2
// #define HUD_TOP           	1
// #define HUD_BOTTOM        	2
// #define HUD_V_LEFT    		1
// #define HUD_V_RIGHT         	2
// #define HUD_H_TOP           	1
// #define HUD_H_BOTTOM        	2


/* Ladder sub-types */
// #define HUD_LIM				1
// #define HUD_NOLIM			2
// #define HUD_CIRC			3

// #define HUD_INSTR_LADDER	1
// #define HUD_INSTR_CLADDER	2
// #define HUD_INSTR_HORIZON	3
// #define HUD_INSTR_LABEL		4

struct HUD_scale {
	int type;
	int scr_pos;
	int scr_min;
	int scr_max;
	int div_min;
	int div_max;
	int orientation;
	int with_minimum;
	int minimum_value;
	int width_units;
	double (*load_value)();
};

struct HUD_circular_scale {
	int type;
	int scr_pos;
	int scr_min;
	int scr_max;
	int div_min;
	int div_max;
	int orientation;
	int label_position;
	int width_units;
	double (*load_value)();
};

struct HUD_ladder {
	int type;
	int x_pos;
	int y_pos;
	int scr_width;
	int scr_height;
	int scr_hole;
	int div_units;
	int label_position;
	int width_units;
	double (*load_roll)();
	double (*load_pitch)();
};

struct HUD_circular_ladder {
	int scr_min;
	int scr_max;
	int div_min;
	int div_max;
	int orientation;
	int label_position;
	int width_units;
	double (*load_value)();
};

#define HORIZON_FIXED	1
#define HORIZON_MOVING	2

struct HUD_horizon {
	int type;
	int x_pos;
	int y_pos;
	int scr_width;
	int scr_hole;
	double (*load_value)();
};

#define LABEL_COUNTER	1
#define LABEL_WARNING	2

struct HUD_label {
	int type;
	int x_pos;
	int y_pos;
	int size;
	int blink;
	int justify;
	char *pre_str;
	char *post_str;
	char *format;
	double (*load_value)();
};

union HUD_instr_data {
	struct HUD_scale scale;
	struct HUD_circular_scale circ_scale;
	struct HUD_ladder ladder;
	struct HUD_circular_ladder circ_ladder;
	struct HUD_horizon horizon;
	struct HUD_label label;
};

typedef struct HUD_instr *HIptr;

struct HUD_instr {
	int type;
	int sub_type;
	int orientation;
	union HUD_instr_data instr;
	int color;
	HIptr next;
};

struct HUD {
	int code;
	// struct HUD_instr *instruments;
	HIptr instruments;
	int status;
};

typedef struct HUD *Hptr;

Hptr fgHUDInit( struct AIRCRAFT cur_aircraft, int color );
Hptr fgHUDAddHorizon( Hptr hud, int x_pos, int y_pos, int length, int hole_len, double (*load_value)() );
Hptr fgHUDAddScale( Hptr hud, int type, int scr_pos, int scr_min, int scr_max, int div_min, int div_max, \
					int orientation, int with_min, int min_value, int width_units, double (*load_value)() );
Hptr fgHUDAddLabel( Hptr hud, int x_pos, int y_pos, int size, int blink, int justify, \
					char *pre_str, char *post_str, char *format, double (*load_value)() );
Hptr fgHUDAddLadder( Hptr hud, int x_pos, int y_pos, int scr_width, int scr_height, \
					int hole_len, int div_units, int label_pos, int max_value, \
					double (*load_roll)(), double (*load_pitch)() );
					
					
					
/* struct HUD *fgHUDAddLadder( Hptr hud, int scr_min, int scr_max, int div_min, int div_max, \
					int orientation, int max_value, double *(load_value);
struct HUD *fgHUDAddCircularLadder( Hptr hud, int scr_min, int scr_max, int div_min, int div_max, \
					int max_value, double *(load_value) );
struct HUD *fgHUDAddNumDisp( Hptr hud, int x_pos, int y_pos, int size, int blink, \
					char *pre_str, char *post_str, double *(load_value) ); */
void fgUpdateHUD();
void fgUpdateHUD2( struct HUD *hud );


/* $Log$
/* Revision 1.1  1997/08/29 18:03:22  curt
/* Initial revision.
/*
 */
