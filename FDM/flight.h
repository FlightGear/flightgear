/**************************************************************************
 * flight.h -- define shared flight model parameters
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
/* Revision 1.2  1997/05/23 15:40:37  curt
/* Added GNU copyright headers.
/*
 * Revision 1.1  1997/05/16 16:04:45  curt
 * Initial revision.
 *
 */
