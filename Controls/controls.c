/**************************************************************************
 * controls.c -- defines a standard interface to all flight sim controls
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


#include "controls.h"
#include "../aircraft/aircraft.h"


void fgControlsInit() {
    int i;
    struct control_params *c;
    c = &current_aircraft.controls;

    FG_Elevator = 0.0;
    FG_Elev_Trim = 1.969572E-03;
    FG_Aileron  = 0.0;
    FG_Rudder   = 0.0;

    for ( i = 0; i < FG_MAX_ENGINES; i++ ) {
	FG_Throttle[i] = 0.0;
    }

}


void fgElevMove(double amt) {
    struct control_params *c;
    c = &current_aircraft.controls;

    FG_Elevator += amt;

    if ( FG_Elevator < -1.0 ) FG_Elevator = -1.0;
    if ( FG_Elevator >  1.0 ) FG_Elevator =  1.0;
}

void fgElevSet(double pos) {
    struct control_params *c;
    c = &current_aircraft.controls;

    FG_Elevator = pos;

    if ( FG_Elevator < -1.0 ) FG_Elevator = -1.0;
    if ( FG_Elevator >  1.0 ) FG_Elevator =  1.0;
}

void fgElevTrimMove(double amt) {
    struct control_params *c;
    c = &current_aircraft.controls;

    FG_Elev_Trim += amt;

    if ( FG_Elev_Trim < -1.0 ) FG_Elev_Trim = -1.0;
    if ( FG_Elev_Trim >  1.0 ) FG_Elev_Trim =  1.0;
}

void fgElevTrimSet(double pos) {
    struct control_params *c;
    c = &current_aircraft.controls;

    FG_Elev_Trim = pos;

    if ( FG_Elev_Trim < -1.0 ) FG_Elev_Trim = -1.0;
    if ( FG_Elev_Trim >  1.0 ) FG_Elev_Trim =  1.0;
}

void fgAileronMove(double amt) {
    struct control_params *c;
    c = &current_aircraft.controls;

    FG_Aileron += amt;

    if ( FG_Aileron < -1.0 ) FG_Aileron = -1.0;
    if ( FG_Aileron >  1.0 ) FG_Aileron =  1.0;
}

void fgAileronSet(double pos) {
    struct control_params *c;
    c = &current_aircraft.controls;

    FG_Aileron = pos;

    if ( FG_Aileron < -1.0 ) FG_Aileron = -1.0;
    if ( FG_Aileron >  1.0 ) FG_Aileron =  1.0;
}

void fgRudderMove(double amt) {
    struct control_params *c;
    c = &current_aircraft.controls;

    FG_Rudder += amt;

    if ( FG_Rudder < -1.0 ) FG_Rudder = -1.0;
    if ( FG_Rudder >  1.0 ) FG_Rudder =  1.0;
}

void fgRudderSet(double pos) {
    struct control_params *c;
    c = &current_aircraft.controls;

    FG_Rudder = pos;

    if ( FG_Rudder < -1.0 ) FG_Rudder = -1.0;
    if ( FG_Rudder >  1.0 ) FG_Rudder =  1.0;
}

void fgThrottleMove(int engine, double amt) {
    int i;
    struct control_params *c;
    c = &current_aircraft.controls;

    if ( engine == FG_Throttle_All ) {
	for ( i = 0; i < FG_MAX_ENGINES; i++ ) {
	    FG_Throttle[i] += amt;
	    if ( FG_Throttle[i] < 0.0 ) FG_Throttle[i] = 0.0;
	    if ( FG_Throttle[i] > 1.0 ) FG_Throttle[i] = 1.0;
	}
    } else {
	if ( (engine >= 0) && (engine < FG_MAX_ENGINES) ) {
	    FG_Throttle[engine] += amt;
	    if ( FG_Throttle[engine] < 0.0 ) FG_Throttle[engine] = 0.0;
	    if ( FG_Throttle[engine] > 1.0 ) FG_Throttle[engine] = 1.0;
	}
    }
}

void fgThrottleSet(int engine, double pos) {
    int i;
    struct control_params *c;
    c = &current_aircraft.controls;

    if ( engine == FG_Throttle_All ) {
	for ( i = 0; i < FG_MAX_ENGINES; i++ ) {
	    FG_Throttle[i] = pos;
	    if ( FG_Throttle[i] < 0.0 ) FG_Throttle[i] = 0.0;
	    if ( FG_Throttle[i] > 1.0 ) FG_Throttle[i] = 1.0;
	}
    } else {
	if ( (engine >= 0) && (engine < FG_MAX_ENGINES) ) {
	    FG_Throttle[engine] = pos;
	    if ( FG_Throttle[engine] < 0.0 ) FG_Throttle[engine] = 0.0;
	    if ( FG_Throttle[engine] > 1.0 ) FG_Throttle[engine] = 1.0;
	}
    }
}


/* $Log$
/* Revision 1.1  1997/05/31 19:24:04  curt
/* Initial revision.
/*
 */
