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


#include <stdio.h>

#include <Aircraft/aircraft.h>
#include <Include/fg_constants.h>
#include <Main/fg_debug.h>

/* This is a record containing all the info for the aircraft currently
   being operated */
struct fgAIRCRAFT current_aircraft;


/* Display various parameters to stdout */
void fgAircraftOutputCurrent(struct fgAIRCRAFT *a) {
    struct fgFLIGHT *f;
    struct fgCONTROLS *c;

    f = &a->flight;
    c = &a->controls;

    fgPrintf( FG_FLIGHT, FG_DEBUG,
	      "Pos = (%.2f,%.2f,%.2f)  (Phi,Theta,Psi)=(%.2f,%.2f,%.2f)\n",
	      FG_Longitude * 3600.0 * RAD_TO_DEG, 
	      FG_Latitude  * 3600.0 * RAD_TO_DEG,
	      FG_Altitude, FG_Phi, FG_Theta, FG_Psi);
    fgPrintf( FG_FLIGHT, FG_DEBUG,
	      "Kts = %.0f  Elev = %.2f, Aileron = %.2f, Rudder = %.2f  Power = %.2f\n", 
	      FG_V_equiv_kts, FG_Elevator, FG_Aileron, FG_Rudder, FG_Throttle[0]);
}


/* $Log$
/* Revision 1.15  1998/01/27 00:47:46  curt
/* Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
/* system and commandline/config file processing code.
/*
 * Revision 1.14  1998/01/19 19:26:56  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.13  1997/12/15 23:54:30  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.12  1997/12/10 22:37:37  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.11  1997/09/13 02:00:05  curt
 * Mostly working on stars and generating sidereal time for accurate star
 * placement.
 *
 * Revision 1.10  1997/08/27 03:29:56  curt
 * Changed naming scheme of basic shared structures.
 *
 * Revision 1.9  1997/07/19 22:39:08  curt
 * Moved PI to ../constants.h
 *
 * Revision 1.8  1997/06/25 15:39:45  curt
 * Minor changes to compile with rsxnt/win32.
 *
 * Revision 1.7  1997/06/02 03:01:39  curt
 * Working on views (side, front, back, transitions, etc.)
 *
 * Revision 1.6  1997/05/31 19:16:26  curt
 * Elevator trim added.
 *
 * Revision 1.5  1997/05/30 19:30:14  curt
 * The LaRCsim flight model is starting to look like it is working.
 *
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
