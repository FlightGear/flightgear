/**************************************************************************
 * aircraft.c -- various aircraft routines
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#include <stdio.h>

#include "aircraft.h"


/* Display various parameters to stdout */
void aircraft_debug(int type) {
    struct flight_params *f;
    struct control_params *c;

    f = &current_aircraft.flight;
    c = &current_aircraft.controls;

    printf("Pos = (%.2f,%.2f,%.2f)  Dir = %.2f\n", 
           f->pos_x, f->pos_y, f->pos_z, f->Psi);
    printf("Elev = %.2f, Aileron = %.2f, Rudder = %.2f\n", 
	   c->elev, c->aileron, c->rudder);
}


/* $Log$
/* Revision 1.1  1997/05/16 15:58:24  curt
/* Initial revision.
/*
 */
