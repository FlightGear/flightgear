/**************************************************************************
 * slew.h -- the "slew" flight model
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef SLEW_H
#define SLEW_H


#include "flight.h"


/* reset flight params to a specific position */ 

void slew_init(double pos_x, double pos_y, double pos_z, double heading);

/* update position based on inputs, positions, velocities, etc. */
void slew_update();


#endif SLEW_H


/* $Log$
/* Revision 1.1  1997/05/16 16:04:46  curt
/* Initial revision.
/*
 */
