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
#include <Include/fg_constants.h>
#include <Math/fg_geodesy.h>


fgFLIGHT cur_flight_params;


/* Initialize the flight model parameters */
int fgFlightModelInit(int model, fgFLIGHT *f, double dt) {
    double save_alt = 0.0;
    int result;

    fgPrintf(FG_FLIGHT,FG_INFO,"Initializing flight model\n");

    if ( model == FG_SLEW ) {
	// fgSlewInit(dt);
    } else if ( model == FG_LARCSIM ) {
	/* lets try to avoid really screwing up the LaRCsim model */
	if ( FG_Altitude < -9000 ) {
	    save_alt = FG_Altitude;
	    FG_Altitude = 0;
	}

	fgFlight_2_LaRCsim(f);  /* translate FG to LaRCsim structure */
	fgLaRCsimInit(dt);
	fgPrintf( FG_FLIGHT, FG_INFO, "FG pos = %.2f\n", FG_Latitude );
	fgLaRCsim_2_Flight(f);  /* translate LaRCsim back to FG	structure */

	/* but lets restore our original bogus altitude when we are done */
	if ( save_alt < -9000 ) {
	    FG_Altitude = save_alt;
	}
    } else {
	fgPrintf( FG_FLIGHT, FG_WARN,
		  "Unimplemented flight model == %d\n", model );
    }

    result = 1;

    return(result);
}


/* Run multiloop iterations of the flight model */
int fgFlightModelUpdate(int model, fgFLIGHT *f, int multiloop) {
    int result;

    // printf("Altitude = %.2f\n", FG_Altitude * 0.3048);

    if ( model == FG_SLEW ) {
	// fgSlewUpdate(f, multiloop);
    } else if ( model == FG_LARCSIM ) {
	fgLaRCsimUpdate(f, multiloop);
    } else {
	fgPrintf( FG_FLIGHT, FG_WARN,
		  "Unimplemented flight model == %d\n", model );
    }

    result = 1;

    return(result);
}


/* Set the altitude (force) */
int fgFlightModelSetAltitude(int model, fgFLIGHT *f, double alt_meters) {
    double sea_level_radius_meters;
    double lat_geoc;
    // Set the FG variables first
    fgGeodToGeoc( FG_Latitude, alt_meters, 
		  &sea_level_radius_meters, &lat_geoc);

    FG_Altitude = alt_meters * METER_TO_FEET;
    FG_Radius_to_vehicle = FG_Altitude + 
	(sea_level_radius_meters * METER_TO_FEET);

    /* additional work needed for some flight models */
    if ( model == FG_LARCSIM ) {
	ls_ForceAltitude(FG_Altitude);
    }

}


/* $Log$
/* Revision 1.17  1998/08/24 20:09:07  curt
/* .
/*
 * Revision 1.16  1998/08/22  14:49:55  curt
 * Attempting to iron out seg faults and crashes.
 * Did some shuffling to fix a initialization order problem between view
 * position, scenery elevation.
 *
 * Revision 1.15  1998/07/30 23:44:36  curt
 * Beginning to add support for multiple flight models.
 *
 * Revision 1.14  1998/07/12 03:08:27  curt
 * Added fgFlightModelSetAltitude() to force the altitude to something
 * other than the current altitude.  LaRCsim doesn't let you do this by just
 * changing FG_Altitude.
 *
 * Revision 1.13  1998/04/25 22:06:28  curt
 * Edited cvs log messages in source files ... bad bad bad!
 *
 * Revision 1.12  1998/04/21 16:59:33  curt
 * Integrated autopilot.
 * Prepairing for C++ integration.
 *
 * Revision 1.11  1998/04/18 04:14:04  curt
 * Moved fg_debug.c to it's own library.
 *
 * Revision 1.10  1998/02/07 15:29:37  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.9  1998/01/27 00:47:53  curt
 * Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
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
