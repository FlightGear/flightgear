// aircraft.cxx -- various aircraft routines
//
// Written by Curtis Olson, started May 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
// (Log is kept at end of this file)


#include <stdio.h>

#include "aircraft.hxx"
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>

// This is a record containing all the info for the aircraft currently
// being operated
fgAIRCRAFT current_aircraft;


// Initialize an Aircraft structure
void fgAircraftInit( void ) {
    FG_LOG( FG_AIRCRAFT, FG_INFO, "Initializing Aircraft structure" );

    current_aircraft.fdm_state   = &cur_fdm_state;
    current_aircraft.controls = &controls;
}


// Display various parameters to stdout
void fgAircraftOutputCurrent(fgAIRCRAFT *a) {
    FGInterface *f;

    f = a->fdm_state;

    FG_LOG( FG_FLIGHT, FG_DEBUG,
	    "Pos = ("
	    << (f->get_Longitude() * 3600.0 * RAD_TO_DEG) << "," 
	    << (f->get_Latitude()  * 3600.0 * RAD_TO_DEG) << ","
	    << f->get_Altitude() 
	    << ")  (Phi,Theta,Psi)=("
	    << f->get_Phi() << "," 
	    << f->get_Theta() << "," 
	    << f->get_Psi() << ")" );

    FG_LOG( FG_FLIGHT, FG_DEBUG,
	    "Kts = " << f->get_V_equiv_kts() 
	    << "  Elev = " << controls.get_elevator() 
	    << "  Aileron = " << controls.get_aileron() 
	    << "  Rudder = " << controls.get_rudder() 
	    << "  Power = " << controls.get_throttle( 0 ) );
}


// $Log$
// Revision 1.1  1999/04/05 21:32:49  curt
// Initial revision
//
// Revision 1.7  1999/02/05 21:28:09  curt
// Modifications to incorporate Jon S. Berndts flight model code.
//
// Revision 1.6  1998/12/05 15:53:59  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.5  1998/12/03 01:14:58  curt
// Converted fgFLIGHT to a class.
//
// Revision 1.4  1998/11/06 21:17:31  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUG flag.
//
// Revision 1.3  1998/10/25 14:08:37  curt
// Turned "struct fgCONTROLS" into a class, with inlined accessor functions.
//
// Revision 1.2  1998/10/17 01:33:52  curt
// C++ ifying ...
//
// Revision 1.1  1998/10/16 23:26:47  curt
// C++-ifying.
//
// Revision 1.19  1998/04/25 22:06:24  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.18  1998/04/18 04:13:56  curt
// Moved fg_debug.c to it's own library.
//
// Revision 1.17  1998/02/12 21:59:31  curt
// Incorporated code changes contributed by Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.16  1998/02/07 15:29:31  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.15  1998/01/27 00:47:46  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.14  1998/01/19 19:26:56  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.13  1997/12/15 23:54:30  curt
// Add xgl wrappers for debugging.
// Generate terrain normals on the fly.
//
// Revision 1.12  1997/12/10 22:37:37  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.11  1997/09/13 02:00:05  curt
// Mostly working on stars and generating sidereal time for accurate star
// placement.
//
// Revision 1.10  1997/08/27 03:29:56  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.9  1997/07/19 22:39:08  curt
// Moved PI to ../constants.h
//
// Revision 1.8  1997/06/25 15:39:45  curt
// Minor changes to compile with rsxnt/win32.
//
// Revision 1.7  1997/06/02 03:01:39  curt
// Working on views (side, front, back, transitions, etc.)
//
// Revision 1.6  1997/05/31 19:16:26  curt
// Elevator trim added.
//
// Revision 1.5  1997/05/30 19:30:14  curt
// The LaRCsim flight model is starting to look like it is working.
//
// Revision 1.4  1997/05/30 03:54:11  curt
// Made a bit more progress towards integrating the LaRCsim flight model.
//
// Revision 1.3  1997/05/29 22:39:56  curt
// Working on incorporating the LaRCsim flight model.
//
// Revision 1.2  1997/05/23 15:40:29  curt
// Added GNU copyright headers.
//
// Revision 1.1  1997/05/16 15:58:24  curt
// Initial revision.
//

