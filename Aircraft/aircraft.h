/**************************************************************************
 * aircraft.h -- define shared aircraft parameters
 *
 * Written by Curtis Olson, started May 1997.
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
/* Revision 1.1  1997/05/16 15:58:25  curt
/* Initial revision.
/*
 */
