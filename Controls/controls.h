/**************************************************************************
 * controls.h -- defines a standard interface to all flight sim controls
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


#ifndef CONTROLS_H
#define CONTROLS_H


#include "../Include/limits.h"


/* Define a structure containing the control parameters */

struct fgCONTROLS {
    double aileron;
    double elevator;
    double elevator_trim;
    double rudder;
    double throttle[FG_MAX_ENGINES];
};


#define FG_Elevator     c->elevator
#define FG_Aileron      c->aileron
#define FG_Rudder       c->rudder
#define FG_Throttle     c->throttle
#define FG_Throttle_All -1
#define FG_Elev_Trim    c->elevator_trim

/* 
#define Left_button     cockpit_.left_pb_on_stick
#define Right_button    cockpit_.right_pb_on_stick
#define First_trigger   cockpit_.trig_pos_1
#define Second_trigger  cockpit_.trig_pos_2
#define Left_trim       cockpit_.left_trim
#define Right_trim      cockpit_.right_trim
#define SB_extend       cockpit_.sb_extend
#define SB_retract      cockpit_.sb_retract
#define Gear_sel_up     cockpit_.gear_sel_up 
*/


void fgControlsInit();

void fgElevMove(double amt);
void fgElevSet(double pos);
void fgElevTrimMove(double amt);
void fgElevTrimSet(double pos);
void fgAileronMove(double amt);
void fgAileronSet(double pos);
void fgRudderMove(double amt);
void fgRudderSet(double pos);
void fgThrottleMove(int engine, double amt);
void fgThrottleSet(int engine, double pos);



#endif /* CONTROLS_H */


/* $Log$
/* Revision 1.7  1997/12/15 23:54:36  curt
/* Add xgl wrappers for debugging.
/* Generate terrain normals on the fly.
/*
 * Revision 1.6  1997/12/10 22:37:41  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.5  1997/08/27 03:30:02  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.4  1997/07/23 21:52:18  curt
 * Put comments around the text after an #endif for increased portability.
 *
 * Revision 1.3  1997/05/31 19:16:27  curt
 * Elevator trim added.
 *
 * Revision 1.2  1997/05/23 15:40:33  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 15:59:48  curt
 * Initial revision.
 *
 */
