/**************************************************************************
 * slew.c -- the "slew" flight model
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <math.h>

#include "slew.h"
#include "flight.h"
#include "../aircraft/aircraft.h"
#include "../controls/controls.h"


#ifndef PI2                                               
#define PI2  (M_PI + M_PI)                      
#endif        


/* reset flight params to a specific position */
void slew_init(double pos_x, double pos_y, double pos_z, double heading) {
    struct flight_params *f;

    f = &current_aircraft.flight;

    f->pos_x = pos_x;
    f->pos_y = pos_y;
    f->pos_z = pos_z;

    f->vel_x = 0.0;
    f->vel_y = 0.0;
    f->vel_z = 0.0;

    f->Phi = 0.0;
    f->Theta = 0.0;
    f->Psi = 0.0;

    f->vel_Phi = 0.0;
    f->vel_Theta = 0.0;
    f->vel_Psi = 0.0;

    f->Psi = heading;
}


/* update position based on inputs, positions, velocities, etc. */
void slew_update() {
    struct flight_params *f;
    struct control_params *c;

    f = &current_aircraft.flight;
    c = &current_aircraft.controls;

    f->Psi += c->aileron;
    if ( f->Psi > PI2 ) {
	f->Psi -= PI2;
    } else if ( f->Psi < 0 ) {
	f->Psi += PI2;
    }

    f->vel_x = -c->elev;

    f->pos_x = f->pos_x + (cos(f->Psi) * f->vel_x);
    f->pos_y = f->pos_y + (sin(f->Psi) * f->vel_x);
}


/* $Log$
/* Revision 1.1  1997/05/16 16:04:45  curt
/* Initial revision.
/*
 */
