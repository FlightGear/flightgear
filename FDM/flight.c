/**************************************************************************
 * flight.c -- a general interface to the various flight models
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
#include "flight.h"


/* Initialize the flight model parameters */
int fgFlightModelInit(int model, struct fgFLIGHT *f, double dt) {
    int result;

    if ( model == FG_LARCSIM ) {
	fgFlight_2_LaRCsim(f);  /* translate FG to LaRCsim structure */
	fgLaRCsimInit(dt);
	printf("FG pos = %.2f\n", FG_Latitude);
	fgLaRCsim_2_Flight(f);  /* translate LaRCsim back to FG	structure */
    } else {
	printf("Unimplemented flight model == %d\n", model);
    }

    return(result);
}


/* Run multiloop iterations of the flight model */
int fgFlightModelUpdate(int model, struct fgFLIGHT *f, int multiloop) {
    int result;

    if ( model == FG_LARCSIM ) {
	fgFlight_2_LaRCsim(f);  /* translate FG to LaRCsim structure */
	fgLaRCsimUpdate(multiloop);
	fgLaRCsim_2_Flight(f);  /* translate LaRCsim back to FG	structure */
    } else {
	printf("Unimplemented flight model == %d\n", model);
    }

    return(result);
}


/* $Log$
/* Revision 1.4  1997/12/10 22:37:42  curt
/* Prepended "fg" on the name of all global structures that didn't have it yet.
/* i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
/*
 * Revision 1.3  1997/08/27 03:30:04  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.2  1997/05/29 22:39:57  curt
 * Working on incorporating the LaRCsim flight model.
 *
 * Revision 1.1  1997/05/29 02:35:04  curt
 * Initial revision.
 *
 */
