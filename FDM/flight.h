/**************************************************************************
 * flight.h -- define shared flight model parameters
 *
 * Written by Curtis Olson, started May 1997.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


#ifndef FLIGHT_H
#define FLIGHT_H


#include "slew.h"


/* Define a structure containing the shared flight model parameters */

struct flight_params {
    double pos_x, pos_y, pos_z;   /* temporary position variables */
    double vel_x, vel_y, vel_z;   /* temporary velocity variables */

    double Phi;    /* Roll angle */
    double Theta;  /* Pitch angle */
    double Psi;    /* Heading angle */
    double vel_Phi;
    double vel_Theta;
    double vel_Psi;
};


#endif FLIGHT_H


/* $Log$
/* Revision 1.1  1997/05/16 16:04:45  curt
/* Initial revision.
/*
 */
