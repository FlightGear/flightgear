/**************************************************************************
 * aircraft.c -- various aircraft routines
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
/* Revision 1.2  1997/05/23 15:40:29  curt
/* Added GNU copyright headers.
/*
 * Revision 1.1  1997/05/16 15:58:24  curt
 * Initial revision.
 *
 */
