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

#include <Debug/fg_debug.h>
#include <Flight/flight.h>
#include <Flight/LaRCsim/ls_interface.h>


fgFLIGHT cur_flight_params;


/* Initialize the flight model parameters */
int fgFlightModelInit(int model, fgFLIGHT *f, double dt) {
    int result;

    fgPrintf(FG_FLIGHT,FG_INFO,"Initializing flight model\n");

    if ( model == FG_LARCSIM ) {
	fgFlight_2_LaRCsim(f);  /* translate FG to LaRCsim structure */
	fgLaRCsimInit(dt);
	fgPrintf(FG_FLIGHT,FG_INFO,"FG pos = %.2f\n", FG_Latitude);
	fgLaRCsim_2_Flight(f);  /* translate LaRCsim back to FG	structure */
    } else {
	fgPrintf(FG_FLIGHT,FG_WARN,"Unimplemented flight model == %d\n", model);
    }

    result = 1;

    return(result);
}


/* Run multiloop iterations of the flight model */
int fgFlightModelUpdate(int model, fgFLIGHT *f, int multiloop) {
    int result;

    if ( model == FG_LARCSIM ) {
	fgFlight_2_LaRCsim(f);  /* translate FG to LaRCsim structure */
	fgLaRCsimUpdate(multiloop);
	fgLaRCsim_2_Flight(f);  /* translate LaRCsim back to FG	structure */
    } else {
	fgPrintf(FG_FLIGHT,FG_WARN,"Unimplemented flight model == %d\n", model);
    }

    result = 1;

    return(result);
}


/* $Log$
/* Revision 1.11  1998/04/18 04:14:04  curt
/* Moved fg_debug.c to it's own library.
/*
 * Revision 1.10  1998/02/07 15:29:37  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.9  1998/01/27 00:47:53  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.8  1998/01/19 19:27:03  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.7  1998/01/19 18:40:23  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.6  1998/01/19 18:35:43  curt
 * Minor tweaks and fixes for cygwin32.
 *
 * Revision 1.5  1997/12/30 20:47:37  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.4  1997/12/10 22:37:42  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
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
