/**************************************************************************
 * controls.h -- defines a standard interface to all flight sim controls
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef CONTROLS_H
#define CONTROLS_H


#include "../limits.h"


/* Define a structure containing the control parameters */

struct control_params {
    double aileron;
    double elev;
    double rudder;
    double throttle[MAX_ENGINES];
};


#endif CONTROLS_H


/* $Log$
/* Revision 1.1  1997/05/16 15:59:48  curt
/* Initial revision.
/*
 */
