/**************************************************************************
 * ls_interface.h -- interface to the "LaRCsim" flight model
 *
 * Written by Curtis Olson, started May 1997.
 *
 * Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
int ls_toplevel_init(double dt, char * aircraft);

/* update position based on inputs, positions, velocities, etc. */
int ls_update(int multiloop);

void ls_set_model_dt(double dt);

#if 0
/* Convert from the fgFLIGHT struct to the LaRCsim generic_ struct */
int fgFlight_2_LaRCsim (fgFLIGHT *f);

/* Convert from the LaRCsim generic_ struct to the fgFLIGHT struct */
int fgLaRCsim_2_Flight (fgFLIGHT *f);
#endif

void ls_loop( SCALAR dt, int initialize );

/* Set the altitude (force) */
int ls_ForceAltitude(double alt_feet);


#ifdef __cplusplus
}
#endif

#endif /* _LS_INTERFACE_H */


// $Log$
// Revision 1.3  2006/02/21 17:45:03  mfranz
// new FSF address (see http://www.gnu.org/licenses/gpl.html)
//
// Revision 1.2  2004-11-19 22:10:42  curt
// Fix my mailing address by replacing it with my web page.
//
// Revision 1.1.1.1  2002/09/10 01:14:02  curt
// Initial revision of FlightGear-0.9.0
//
// Revision 1.5  2001/03/24 05:03:12  curt
// SG-ified logstream.
//
// Revision 1.4  2000/10/23 22:34:54  curt
// I tested:
// LaRCsim c172 on-ground and in-air starts, reset: all work
// UIUC Cessna172 on-ground and in-air starts work as expected, reset
// results in an aircraft that is upside down but does not crash FG.   I
// don't know what it was like before, so it may well be no change.
// JSBSim c172 and X15 in-air starts work fine, resets now work (and are
// trimmed), on-ground starts do not -- the c172 ends up on its back.  I
// suspect this is no worse than before.
//
// I did not test:
// Balloon (the weather code returns nan's for the atmosphere data --this
// is in the weather module and apparently is a linux only bug)
// ADA (don't know how)
// MagicCarpet  (needs work yet)
// External (don't know how)
//
// known to be broken:
// LaRCsim c172 on-ground starts with a negative terrain altitude (this
// happens at KPAO when the scenery is not present).   The FDM inits to
// about 50 feet AGL and the model falls to the ground.  It does stay
// upright, however, and seems to be fine once it settles out, FWIW.
//
// To do:
// --implement set_Model on the bus
// --bring Christian's weather data into JSBSim
// -- add default method to bus for updating things like the sin and cos of
// latitude (for Balloon, MagicCarpet)
// -- lots of cleanup
//
// The files:
// src/FDM/flight.cxx
// src/FDM/flight.hxx
// -- all data members now declared protected instead of private.
// -- eliminated all but a small set of 'setters', no change to getters.
// -- that small set is declared virtual, the default implementation
// provided preserves the old behavior
// -- all of the vector data members are now initialized.
// -- added busdump() method -- SG_LOG's  all the bus data when called,
// useful for diagnostics.
//
// src/FDM/ADA.cxx
// -- bus data members now directly assigned to
//
// src/FDM/Balloon.cxx
// -- bus data members now directly assigned to
// -- changed V_equiv_kts to V_calibrated_kts
//
// src/FDM/JSBSim.cxx
// src/FDM/JSBSim.hxx
// -- bus data members now directly assigned to
// -- implemented the FGInterface virtual setters with JSBSim specific
// logic
// -- changed the static FDMExec to a dynamic fdmex (needed so that the
// JSBSim object can be deleted when a model change is called for)
// -- implemented constructor and destructor, moved some of the logic
// formerly in init() to constructor
// -- added logic to bring up FGEngInterface objects and set the RPM and
// throttle values.
//
// src/FDM/LaRCsim.cxx
// src/FDM/LaRCsim.hxx
// -- bus data members now directly assigned to
// -- implemented the FGInterface virtual setters with LaRCsim specific
// logic, uses LaRCsimIC
// -- implemented constructor and destructor, moved some of the logic
// formerly in init() to constructor
// -- moved default inertias to here from fg_init.cxx
// -- eliminated the climb rate calculation.  The equivalent, climb_rate =
// -1*vdown, is now in copy_from_LaRCsim().
//
// src/FDM/LaRCsimIC.cxx
// src/FDM/LaRCsimIC.hxx
// -- similar to FGInitialCondition, this class has all the logic needed to
// turn data like Vc and Mach into the more fundamental quantities LaRCsim
// needs to initialize.
// -- put it in src/FDM since it is a class
//
// src/FDM/MagicCarpet.cxx
//  -- bus data members now directly assigned to
//
// src/FDM/Makefile.am
// -- adds LaRCsimIC.hxx and cxx
//
// src/FDM/JSBSim/FGAtmosphere.h
// src/FDM/JSBSim/FGDefs.h
// src/FDM/JSBSim/FGInitialCondition.cpp
// src/FDM/JSBSim/FGInitialCondition.h
// src/FDM/JSBSim/JSBSim.cpp
// -- changes to accomodate the new bus
//
// src/FDM/LaRCsim/atmos_62.h
// src/FDM/LaRCsim/ls_geodesy.h
// -- surrounded prototypes with #ifdef __cplusplus ... #endif , functions
// here are needed in LaRCsimIC
//
// src/FDM/LaRCsim/c172_main.c
// src/FDM/LaRCsim/cherokee_aero.c
// src/FDM/LaRCsim/ls_aux.c
// src/FDM/LaRCsim/ls_constants.h
// src/FDM/LaRCsim/ls_geodesy.c
// src/FDM/LaRCsim/ls_geodesy.h
// src/FDM/LaRCsim/ls_step.c
// src/FDM/UIUCModel/uiuc_betaprobe.cpp
// -- changed PI to LS_PI, eliminates preprocessor naming conflict with
// weather module
//
// src/FDM/LaRCsim/ls_interface.c
// src/FDM/LaRCsim/ls_interface.h
// -- added function ls_set_model_dt()
//
// src/Main/bfi.cxx
// -- eliminated calls that set the NED speeds to body components.  They
// are no longer needed and confuse the new bus.
//
// src/Main/fg_init.cxx
// -- eliminated calls that just brought the bus data up-to-date (e.g.
// set_sin_cos_latitude). or set default values.   The bus now handles the
// defaults and updates itself when the setters are called (for LaRCsim and
// JSBSim).  A default method for doing this needs to be added to the bus.
// -- added fgVelocityInit() to set the speed the user asked for.  Both
// JSBSim and LaRCsim can now be initialized using any of:
// vc,mach, NED components, UVW components.
//
// src/Main/main.cxx
// --eliminated call to fgFDMSetGroundElevation, this data is now 'pulled'
// onto the bus every update()
//
// src/Main/options.cxx
// src/Main/options.hxx
// -- added enum to keep track of the speed requested by the user
// -- eliminated calls to set NED velocity properties to body speeds, they
// are no longer needed.
// -- added options for the NED components.
//
// src/Network/garmin.cxx
// src/Network/nmea.cxx
// --eliminated calls that just brought the bus data up-to-date (e.g.
// set_sin_cos_latitude).  The bus now updates itself when the setters are
// called (for LaRCsim and JSBSim).  A default method for doing this needs
// to be added to the bus.
// -- changed set_V_equiv_kts to set_V_calibrated_kts.  set_V_equiv_kts no
// longer exists ( get_V_equiv_kts still does, though)
//
// src/WeatherCM/FGLocalWeatherDatabase.cpp
// -- commented out the code to put the weather data on the bus, a
// different scheme for this is needed.
//
// Revision 1.3  2000/04/27 19:57:08  curt
// MacOS build updates.
//
// Revision 1.2  2000/04/10 18:09:41  curt
// David Megginson made a few (mostly minor) mods to the LaRCsim files, and
// it's now possible to choose the LaRCsim model at runtime, as in
//
//   fgfs --aircraft=c172
//
// or
//
//   fgfs --aircraft=uiuc --aircraft-dir=Aircraft-uiuc/Boeing747
//
// I did this so that I could play with the UIUC stuff without losing
// Tony's C172 with its flaps, etc.  I did my best to respect the design
// of the LaRCsim code by staying in C, making only minimal changes, and
// not introducing any dependencies on the rest of FlightGear.  The
// modified files are attached.
//
// Revision 1.1.1.1  1999/06/17 18:07:33  curt
// Start of 0.7.x branch
//
// Revision 1.1.1.1  1999/04/05 21:32:45  curt
// Start of 0.6.x branch.
//
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
