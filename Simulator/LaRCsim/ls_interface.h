/**************************************************************************
 * ls_interface.h -- interface to the "LaRCsim" flight model
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


#ifndef _LS_INTERFACE_H
#define _LS_INTERFACE_H


#ifdef __cplusplus
extern "C" {
#endif


#include "ls_types.h"


/* reset flight params to a specific position */ 
int ls_toplevel_init(double dt);

/* update position based on inputs, positions, velocities, etc. */
int ls_update(int multiloop);

#if 0
/* Convert from the fgFLIGHT struct to the LaRCsim generic_ struct */
int fgFlight_2_LaRCsim (fgFLIGHT *f);

/* Convert from the LaRCsim generic_ struct to the fgFLIGHT struct */
int fgLaRCsim_2_Flight (fgFLIGHT *f);

void ls_loop( SCALAR dt, int initialize );
#endif

/* Set the altitude (force) */
int ls_ForceAltitude(double alt_feet);


#endif /* _LS_INTERFACE_H */


#ifdef __cplusplus
}
#endif


// $Log$
// Revision 1.11  1998/10/17 01:34:15  curt
// C++ ifying ...
//
// Revision 1.10  1998/10/16 23:27:45  curt
// C++-ifying.
//
// Revision 1.9  1998/07/12 03:11:04  curt
// Removed some printf()'s.
// Fixed the autopilot integration so it should be able to update it's control
//   positions every time the internal flight model loop is run, and not just
//   once per rendered frame.
// Added a routine to do the necessary stuff to force an arbitrary altitude
//   change.
// Gave the Navion engine just a tad more power.
//
// Revision 1.8  1998/04/21 16:59:39  curt
// Integrated autopilot.
// Prepairing for C++ integration.
//
// Revision 1.7  1998/02/07 15:29:39  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.6  1998/02/03 23:20:17  curt
// Lots of little tweaks to fix various consistency problems discovered by
// Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
// passed arguments along to the real printf().  Also incorporated HUD changes
// by Michele America.
//
// Revision 1.5  1998/01/19 19:27:05  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.4  1998/01/19 18:40:27  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.3  1997/07/23 21:52:20  curt
// Put comments around the text after an #endif for increased portability.
//
// Revision 1.2  1997/05/29 22:39:59  curt
// Working on incorporating the LaRCsim flight model.
//
// Revision 1.1  1997/05/29 00:09:58  curt
// Initial Flight Gear revision.
//
