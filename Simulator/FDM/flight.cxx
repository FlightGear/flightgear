// flight.c -- a general interface to the various flight models
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

#include <Debug/logstream.hxx>
#include <FDM/External/external.hxx>
#include <FDM/LaRCsim/ls_interface.h>
#include <Include/fg_constants.h>
#include <Math/fg_geodesy.hxx>
#include <Time/timestamp.hxx>

#include "flight.hxx"
#include "JSBsim.hxx"
#include "LaRCsim.hxx"


// base_fdm_state is the internal state that is updated in integer
// multiples of "dt".  This leads to "jitter" with respect to the real
// world time, so we introduce cur_fdm_state which is extrapolated by
// the difference between sim time and real world time

FGInterface cur_fdm_state;
FGInterface base_fdm_state;


// Extrapolate fdm based on time_offset (in usec)
void FGInterface::extrapolate( int time_offset ) {
    double dt = time_offset / 1000000.0;
    cout << "extrapolating FDM by dt = " << dt << endl;

    double lat = geodetic_position_v[0] + geocentric_rates_v[0] * dt;
    double lat_geoc = geocentric_position_v[0] + geocentric_rates_v[0] * dt;

    double lon = geodetic_position_v[1] + geocentric_rates_v[1] * dt;
    double lon_geoc = geocentric_position_v[1] + geocentric_rates_v[1] * dt;

    double alt = geodetic_position_v[2] + geocentric_rates_v[2] * dt;
    double radius = geocentric_position_v[2] + geocentric_rates_v[2] * dt;

    geodetic_position_v[0] = lat;
    geocentric_position_v[0] = lat_geoc;

    geodetic_position_v[1] = lon;
    geocentric_position_v[1] = lon_geoc;

    geodetic_position_v[2] = alt;
    geocentric_position_v[2] = radius;
}


// Initialize the flight model parameters
int fgFDMInit(int model, FGInterface& f, double dt) {
    double save_alt = 0.0;

    FG_LOG( FG_FLIGHT ,FG_INFO, "Initializing flight model" );

    base_fdm_state = f;

    if ( model == FGInterface::FG_SLEW ) {
	// fgSlewInit(dt);
    } else if ( model == FGInterface::FG_JSBSIM ) {
	fgJSBsimInit(dt);
	fgJSBsim_2_FGInterface(base_fdm_state);
    } else if ( model == FGInterface::FG_LARCSIM ) {
	// lets try to avoid really screwing up the LaRCsim model
	if ( base_fdm_state.get_Altitude() < -9000.0 ) {
	    save_alt = base_fdm_state.get_Altitude();
	    base_fdm_state.set_Altitude( 0.0 );
	}

	// translate FG to LaRCsim structure
	FGInterface_2_LaRCsim(base_fdm_state);

	// initialize LaRCsim
	fgLaRCsimInit(dt);

	FG_LOG( FG_FLIGHT, FG_INFO, "FG pos = " << 
		base_fdm_state.get_Latitude() );

	// translate LaRCsim back to FG structure
	fgLaRCsim_2_FGInterface(base_fdm_state);

	// but lets restore our original bogus altitude when we are done
	if ( save_alt < -9000.0 ) {
	    base_fdm_state.set_Altitude( save_alt );
	}
    } else if ( model == FGInterface::FG_EXTERNAL ) {
	fgExternalInit(base_fdm_state);
    } else {
	FG_LOG( FG_FLIGHT, FG_WARN,
		"Unimplemented flight model == " << model );
    }

    // set valid time for this record
    base_fdm_state.stamp_time();
	
    f = base_fdm_state;

    return 1;
}


// Run multiloop iterations of the flight model
int fgFDMUpdate(int model, FGInterface& f, int multiloop, int time_offset) {
    double time_step, start_elev, end_elev;

    // printf("Altitude = %.2f\n", FG_Altitude * 0.3048);

    // set valid time for this record
    base_fdm_state.stamp_time();

    time_step = (1.0 / DEFAULT_MODEL_HZ) * multiloop;
    start_elev = base_fdm_state.get_Altitude();

    if ( model == FGInterface::FG_SLEW ) {
	// fgSlewUpdate(f, multiloop);
    } else if ( model == FGInterface::FG_JSBSIM ) {
	fgJSBsimUpdate(base_fdm_state, multiloop);
	f = base_fdm_state;
    } else if ( model == FGInterface::FG_LARCSIM ) {
	fgLaRCsimUpdate(base_fdm_state, multiloop);
	// extrapolate position based on actual time
	// f = extrapolate_fdm( base_fdm_state, time_offset );
	f = base_fdm_state;
    } else if ( model == FGInterface::FG_EXTERNAL ) {
	// fgExternalUpdate(f, multiloop);
	FGTimeStamp current;
	current.stamp();
	f = base_fdm_state;
	f.extrapolate( current - base_fdm_state.get_time_stamp() );
    } else {
	FG_LOG( FG_FLIGHT, FG_WARN,
		"Unimplemented flight model == " <<  model );
    }

    end_elev = base_fdm_state.get_Altitude();

    if ( time_step > 0.0 ) {
	// feet per second
	base_fdm_state.set_Climb_Rate( (end_elev - start_elev) / time_step );
    }

    return 1;
}


// Set the altitude (force)
void fgFDMForceAltitude(int model, double alt_meters) {
    double sea_level_radius_meters;
    double lat_geoc;

    // Set the FG variables first
    fgGeodToGeoc( base_fdm_state.get_Latitude(), alt_meters, 
		  &sea_level_radius_meters, &lat_geoc);

    base_fdm_state.set_Altitude( alt_meters * METER_TO_FEET );
    base_fdm_state.set_Radius_to_vehicle( base_fdm_state.get_Altitude() + 
					  (sea_level_radius_meters * 
					   METER_TO_FEET) );

    // additional work needed for some flight models
    if ( model == FGInterface::FG_LARCSIM ) {
	ls_ForceAltitude( base_fdm_state.get_Altitude() );
    }
}


// Set the local ground elevation
void fgFDMSetGroundElevation(int model, double ground_meters) {
    base_fdm_state.set_Runway_altitude( ground_meters * METER_TO_FEET );
    cur_fdm_state.set_Runway_altitude( ground_meters * METER_TO_FEET );
}


// $Log$
// Revision 1.1  1999/04/05 21:32:44  curt
// Initial revision
//
// Revision 1.17  1999/04/03 04:20:03  curt
// Optimizations (tm) by Norman Vine.
//
// Revision 1.16  1999/02/26 22:09:12  curt
// Added initial support for native SGI compilers.
// Integrated Jon's next version of JSBsim.
//
// Revision 1.15  1999/02/05 21:29:01  curt
// Modifications to incorporate Jon S. Berndts flight model code.
//
// Revision 1.14  1999/02/01 21:33:31  curt
// Renamed FlightGear/Simulator/Flight to FlightGear/Simulator/FDM since
// Jon accepted my offer to do this and thought it was a good idea.
//
// Revision 1.13  1999/01/27 04:48:39  curt
// Set the runway height in cur_fdm_state as well as base_fdm_state.
//
// Revision 1.12  1999/01/20 13:42:22  curt
// Tweaked FDM interface.
// Testing check sum support for NMEA serial output.
//
// Revision 1.11  1999/01/19 17:52:06  curt
// Working on being able to extrapolate a new position and orientation
// based on a position, orientation, and time offset.
//
// Revision 1.10  1999/01/09 13:37:32  curt
// Convert fgTIMESTAMP to FGTimeStamp which holds usec instead of ms.
//
// Revision 1.9  1999/01/08 19:27:37  curt
// Fixed AOA reading on HUD.
// Continued work on time jitter compensation.
//
// Revision 1.8  1999/01/08 03:23:51  curt
// Beginning work on compensating for sim time vs. real world time "jitter".
//
// Revision 1.7  1998/12/18 23:37:07  curt
// Collapsed out the FGState variables not currently needed.  They are just
// commented out and can be readded easily at any time.  The point of this
// exersize is to determine which variables were or were not currently being
// used.
//
// Revision 1.6  1998/12/05 15:54:11  curt
// Renamed class fgFLIGHT to class FGState as per request by JSB.
//
// Revision 1.5  1998/12/04 01:29:39  curt
// Stubbed in a new flight model called "External" which is expected to be driven
// from some external source.
//
// Revision 1.4  1998/12/03 01:16:40  curt
// Converted fgFLIGHT to a class.
//
// Revision 1.3  1998/11/06 21:18:03  curt
// Converted to new logstream debugging facility.  This allows release
// builds with no messages at all (and no performance impact) by using
// the -DFG_NDEBUGNDEBUG flag.
//
// Revision 1.2  1998/10/16 23:27:40  curt
// C++-ifying.
//
// Revision 1.1  1998/10/16 20:16:41  curt
// Renamed flight.[ch] to flight.[ch]xx
//
// Revision 1.19  1998/09/29 14:57:38  curt
// c++-ified comments.
//
// Revision 1.18  1998/09/29 02:02:40  curt
// Added a rate of climb calculation.
//
// Revision 1.17  1998/08/24 20:09:07  curt
// .
//
// Revision 1.16  1998/08/22  14:49:55  curt
// Attempting to iron out seg faults and crashes.
// Did some shuffling to fix a initialization order problem between view
// position, scenery elevation.
//
// Revision 1.15  1998/07/30 23:44:36  curt
// Beginning to add support for multiple flight models.
//
// Revision 1.14  1998/07/12 03:08:27  curt
// Added fgFlightModelSetAltitude() to force the altitude to something
// other than the current altitude.  LaRCsim doesn't let you do this by just
// changing FG_Altitude.
//
// Revision 1.13  1998/04/25 22:06:28  curt
// Edited cvs log messages in source files ... bad bad bad!
//
// Revision 1.12  1998/04/21 16:59:33  curt
// Integrated autopilot.
// Prepairing for C++ integration.
//
// Revision 1.11  1998/04/18 04:14:04  curt
// Moved fg_debug.c to it's own library.
//
// Revision 1.10  1998/02/07 15:29:37  curt
// Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
// <chotchkiss@namg.us.anritsu.com>
//
// Revision 1.9  1998/01/27 00:47:53  curt
// Incorporated Paul Bleisch's <pbleisch@acm.org> new debug message
// system and commandline/config file processing code.
//
// Revision 1.8  1998/01/19 19:27:03  curt
// Merged in make system changes from Bob Kuehne <rpk@sgi.com>
// This should simplify things tremendously.
//
// Revision 1.7  1998/01/19 18:40:23  curt
// Tons of little changes to clean up the code and to remove fatal errors
// when building with the c++ compiler.
//
// Revision 1.6  1998/01/19 18:35:43  curt
// Minor tweaks and fixes for cygwin32.
//
// Revision 1.5  1997/12/30 20:47:37  curt
// Integrated new event manager with subsystem initializations.
//
// Revision 1.4  1997/12/10 22:37:42  curt
// Prepended "fg" on the name of all global structures that didn't have it yet.
// i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
//
// Revision 1.3  1997/08/27 03:30:04  curt
// Changed naming scheme of basic shared structures.
//
// Revision 1.2  1997/05/29 22:39:57  curt
// Working on incorporating the LaRCsim flight model.
//
// Revision 1.1  1997/05/29 02:35:04  curt
// Initial revision.
//
