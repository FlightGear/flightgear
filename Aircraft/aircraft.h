/**************************************************************************
 * aircraft.h -- define shared aircraft parameters
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


#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include "../flight/flight.h"
#include "../controls/controls.h"


/* Define a structure containing all the parameters for an aircraft */
struct aircraft_params {
    struct flight_params flight;
    struct control_params controls;
};


/* current_aircraft contains all the parameters of the aircraft
   currently being operated. */
extern struct aircraft_params current_aircraft;


/* Display various parameters to stdout */
void aircraft_debug(int type);


#endif AIRCRAFT_H


/* $Log$
/* Revision 1.2  1997/05/23 15:40:30  curt
/* Added GNU copyright headers.
/*
 * Revision 1.1  1997/05/16 15:58:25  curt
 * Initial revision.
 *
 */
