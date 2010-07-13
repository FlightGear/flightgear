// native_gui.cxx -- FGFS external gui data export class
//
// Written by Curtis Olson, started January 2002.
//
// Copyright (C) 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/io/lowlevel.hxx> // endian tests
#include <simgear/io/iochannel.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>
#include <FDM/flightProperties.hxx>

#include "native_gui.hxx"

// FreeBSD works better with this included last ... (?)
#if defined( _MSC_VER )
#  include <windows.h>
#elif defined( __MINGW32__ )
#  include <winsock2.h>
#else
#  include <netinet/in.h>	// htonl() ntohl()
#endif


// #define FG_USE_NETWORK_BYTE_ORDER
#if defined( FG_USE_NETWORK_BYTE_ORDER )

// The function htond is defined this way due to the way some
// processors and OSes treat floating point values.  Some will raise
// an exception whenever a "bad" floating point value is loaded into a
// floating point register.  Solaris is notorious for this, but then
// so is LynxOS on the PowerPC.  By translating the data in place,
// there is no need to load a FP register with the "corruped" floating
// point value.  By doing the BIG_ENDIAN test, I can optimize the
// routine for big-endian processors so it can be as efficient as
// possible
static void htond (double &x)	
{
    if ( sgIsLittleEndian() ) {
        int    *Double_Overlay;
        int     Holding_Buffer;
    
        Double_Overlay = (int *) &x;
        Holding_Buffer = Double_Overlay [0];
    
        Double_Overlay [0] = htonl (Double_Overlay [1]);
        Double_Overlay [1] = htonl (Holding_Buffer);
    } else {
        return;
    }
}
static void htonf (float &x)	
{
    if ( sgIsLittleEndian() ) {
        int    *Float_Overlay;
        int     Holding_Buffer;
    
        Float_Overlay = (int *) &x;
        Holding_Buffer = Float_Overlay [0];
    
        Float_Overlay [0] = htonl (Holding_Buffer);
    } else {
        return;
    }
}
#endif


FGNativeGUI::FGNativeGUI() {
}

FGNativeGUI::~FGNativeGUI() {
}


// open hailing frequencies
bool FGNativeGUI::open() {
    if ( is_enabled() ) {
	SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel " 
		<< "is already in use, ignoring" );
	return false;
    }

    SGIOChannel *io = get_io_channel();

    if ( ! io->open( get_direction() ) ) {
	SG_LOG( SG_IO, SG_ALERT, "Error opening channel communication layer." );
	return false;
    }

    set_enabled( true );

    fgSetDouble("/position/sea-level-radius-ft", SG_EQUATORIAL_RADIUS_FT);
    return true;
}


void FGProps2NetGUI( FGNetGUI *net ) {
    static SGPropertyNode *nav_freq
	= fgGetNode("/instrumentation/nav/frequencies/selected-mhz", true);
    static SGPropertyNode *nav_target_radial
	= fgGetNode("/instrumentation/nav/radials/target-radial-deg", true);
    static SGPropertyNode *nav_inrange
	= fgGetNode("/instrumentation/nav/in-range", true);
    static SGPropertyNode *nav_loc
	= fgGetNode("/instrumentation/nav/nav-loc", true);
    static SGPropertyNode *nav_gs_dist_signed
	= fgGetNode("/instrumentation/nav/gs-distance", true);
    static SGPropertyNode *nav_loc_dist
	= fgGetNode("/instrumentation/nav/nav-distance", true);
    static SGPropertyNode *nav_reciprocal_radial
	= fgGetNode("/instrumentation/nav/radials/reciprocal-radial-deg", true);
    static SGPropertyNode *nav_gs_deflection
	= fgGetNode("/instrumentation/nav/gs-needle-deflection", true);
    unsigned int i;

    static FlightProperties* fdm_state = new FlightProperties;

    // Version sanity checking
    net->version = FG_NET_GUI_VERSION;

    // Aero parameters
    net->longitude = fdm_state->get_Longitude();
    net->latitude = fdm_state->get_Latitude();
    net->altitude = fdm_state->get_Altitude() * SG_FEET_TO_METER;
    net->phi = fdm_state->get_Phi();
    net->theta = fdm_state->get_Theta();
    net->psi = fdm_state->get_Psi();

    // Velocities
    net->vcas = fdm_state->get_V_calibrated_kts();
    net->climb_rate = fdm_state->get_Climb_Rate();

    // Consumables
    net->num_tanks = FGNetGUI::FG_MAX_TANKS;
    for ( i = 0; i < net->num_tanks; ++i ) {
        SGPropertyNode *node = fgGetNode("/consumables/fuel/tank", i, true);
        net->fuel_quantity[i] = node->getDoubleValue("level-gal_us");
    }

    // Environment
    net->cur_time = globals->get_time_params()->get_cur_time();
    net->warp = globals->get_warp();
    net->ground_elev = fdm_state->get_Runway_altitude_m();

    // Approach
    net->tuned_freq = nav_freq->getDoubleValue();
    net->nav_radial = nav_target_radial->getDoubleValue();
    net->in_range = nav_inrange->getBoolValue();

    if ( nav_loc->getBoolValue() ) {
        // is an ILS
        net->dist_nm
            = nav_gs_dist_signed->getDoubleValue()
              * SG_METER_TO_NM;
    } else {
        // is a VOR
        net->dist_nm = nav_loc_dist->getDoubleValue()
            * SG_METER_TO_NM;
    }

    net->course_deviation_deg
        = nav_reciprocal_radial->getDoubleValue()
        - nav_target_radial->getDoubleValue();

    if ( net->course_deviation_deg < -1000.0 
         || net->course_deviation_deg > 1000.0 )
    {
        // Sanity check ...
        net->course_deviation_deg = 0.0;
    }
    while ( net->course_deviation_deg >  180.0 ) {
        net->course_deviation_deg -= 360.0;
    }
    while ( net->course_deviation_deg < -180.0 ) {
        net->course_deviation_deg += 360.0;
    }
    if ( fabs(net->course_deviation_deg) > 90.0 )
        net->course_deviation_deg
            = ( net->course_deviation_deg<0.0
                ? -net->course_deviation_deg - 180.0
                : -net->course_deviation_deg + 180.0 );

    if ( nav_loc->getBoolValue() ) {
        // is an ILS
        net->gs_deviation_deg
            = nav_gs_deflection->getDoubleValue()
            / 5.0;
    } else {
        // is an ILS
        net->gs_deviation_deg = -9999.0;
    }

#if defined( FG_USE_NETWORK_BYTE_ORDER )
    // Convert the net buffer to network format
    net->version = htonl(net->version);

    htond(net->longitude);
    htond(net->latitude);
    htonf(net->altitude);
    htonf(net->phi);
    htonf(net->theta);
    htonf(net->psi);
    htonf(net->vcas);
    htonf(net->climb_rate);

    for ( i = 0; i < net->num_tanks; ++i ) {
        htonf(net->fuel_quantity[i]);
    }
    net->num_tanks = htonl(net->num_tanks);

    net->cur_time = htonl( net->cur_time );
    net->warp = htonl( net->warp );
    net->ground_elev = htonl( net->ground_elev );

    htonf(net->tuned_freq);
    htonf(net->nav_radial);
    net->in_range = htonl( net->in_range );
    htonf(net->dist_nm);
    htonf(net->course_deviation_deg);
    htonf(net->gs_deviation_deg);
#endif
}


void FGNetGUI2Props( FGNetGUI *net ) {
    unsigned int i;

#if defined( FG_USE_NETWORK_BYTE_ORDER )
    // Convert to the net buffer from network format
    net->version = ntohl(net->version);

    htond(net->longitude);
    htond(net->latitude);
    htonf(net->altitude);
    htonf(net->phi);
    htonf(net->theta);
    htonf(net->psi);
    htonf(net->vcas);
    htonf(net->climb_rate);

    net->num_tanks = htonl(net->num_tanks);
    for ( i = 0; i < net->num_tanks; ++i ) {
	htonf(net->fuel_quantity[i]);
    }

    net->cur_time = ntohl(net->cur_time);
    net->warp = ntohl(net->warp);
    net->ground_elev = htonl( net->ground_elev );

    htonf(net->tuned_freq);
    htonf(net->nav_radial);
    net->in_range = htonl( net->in_range );
    htonf(net->dist_nm);
    htonf(net->course_deviation_deg);
    htonf(net->gs_deviation_deg);
#endif

    if ( net->version == FG_NET_GUI_VERSION ) {
        FlightProperties fdm_state;
        
        // cout << "pos = " << net->longitude << " " << net->latitude << endl;
        // cout << "sea level rad = " << fdm_state->get_Sea_level_radius()
	//      << endl;
  
        fdm_state.set_Latitude(net->latitude);
        fdm_state.set_Longitude(net->longitude);
        fdm_state.set_Altitude(net->altitude * SG_METER_TO_FEET);

        fdm_state.set_Euler_Angles( net->phi,
                                          net->theta,
                                          net->psi );

        fdm_state.set_V_calibrated_kts( net->vcas );
        fdm_state.set_Climb_Rate( net->climb_rate );

	for (i = 0; i < net->num_tanks; ++i ) {
	    SGPropertyNode * node
		= fgGetNode("/consumables/fuel/tank", i, true);
	    node->setDoubleValue("level-gal_us", net->fuel_quantity[i] );
	}

	if ( net->cur_time ) {
	    fgSetLong("/sim/time/cur-time-override", net->cur_time);
	}

        globals->set_warp( net->warp );

        // Approach
        fgSetDouble( "/instrumentation/nav[0]/frequencies/selected-mhz",
                     net->tuned_freq );
        fgSetBool( "/instrumentation/nav[0]/in-range", net->in_range );
        fgSetDouble( "/instrumentation/dme/indicated-distance-nm", net->dist_nm );
        fgSetDouble( "/instrumentation/nav[0]/heading-needle-deflection",
                     net->course_deviation_deg );
        fgSetDouble( "/instrumentation/nav[0]/gs-needle-deflection",
                     net->gs_deviation_deg );
    } else {
	SG_LOG( SG_IO, SG_ALERT,
                "Error: version mismatch in FGNetNativeGUI2Props()" );
	SG_LOG( SG_IO, SG_ALERT,
		"\tread " << net->version << " need " << FG_NET_GUI_VERSION );
	SG_LOG( SG_IO, SG_ALERT,
		"\tNeed to upgrade net_fdm.hxx and recompile." );
    }
}


// process work for this port
bool FGNativeGUI::process() {
    SGIOChannel *io = get_io_channel();
    int length = sizeof(buf);

    if ( get_direction() == SG_IO_OUT ) {
	// cout << "size of fdm_state = " << length << endl;
	FGProps2NetGUI( &buf );
	if ( ! io->write( (char *)(& buf), length ) ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		FGNetGUI2Props( &buf );
	    }
	} else {
	    while ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		FGNetGUI2Props( &buf );
	    }
	}
    }

    return true;
}


// close the channel
bool FGNativeGUI::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
