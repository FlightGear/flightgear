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


#include <math.h>
#include <stdio.h>

#include "aircraft.h"

#define FG_RAD_2_DEG(RAD) ((RAD) * 180.0 / M_PI)

/* Display various parameters to stdout */
void aircraft_debug(int type) {
    struct flight_params *f;
    struct control_params *c;

    f = &current_aircraft.flight;
    c = &current_aircraft.controls;

    printf("Pos = (%.2f,%.2f,%.2f)  (Phi,Theta,Psi)=(%.2f,%.2f,%.2f)\n",
	   FG_RAD_2_DEG(FG_Longitude) * 3600.0, 
           FG_RAD_2_DEG(FG_Latitude) * 3600.0, 
	   FG_Altitude, FG_Phi, FG_Theta, FG_Psi);
    printf("Mach = %.2f  Elev = %.2f, Aileron = %.2f, Rudder = %.2f\n", 
	   FG_Mach_number, c->elev, c->aileron, c->rudder);
}


/* $Log$
/* Revision 1.5  1997/05/30 19:30:14  curt
/* The LaRCsim flight model is starting to look like it is working.
/*
 * Revision 1.4  1997/05/30 03:54:11  curt
 * Made a bit more progress towards integrating the LaRCsim flight model.
 *
 * Revision 1.3  1997/05/29 22:39:56  curt
 * Working on incorporating the LaRCsim flight model.
 *
 * Revision 1.2  1997/05/23 15:40:29  curt
 * Added GNU copyright headers.
 *
 * Revision 1.1  1997/05/16 15:58:24  curt
 * Initial revision.
 *
 */
