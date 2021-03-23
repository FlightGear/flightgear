// native_fdm.cxx -- FGFS "Native" flight dynamics protocal class
//
// Written by Curtis Olson, started September 2001.
//
// Copyright (C) 2001  Curtis L. Olson - http://www.flightgear.org/~curt
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
#include <config.h>
#endif

#include <simgear/io/lowlevel.hxx>	// endian tests
#include <simgear/timing/sg_time.hxx>

#include <Network/net_ctrls.hxx>
#include <Network/net_fdm.hxx>
#include <Network/net_gui.hxx>
#include <Main/fg_props.hxx>

#include "native_structs.hxx"


// FreeBSD works better with this included last ... (?)
#if defined( _MSC_VER )
#  include <windows.h>
#elif defined( __MINGW32__ )
#  include <winsock2.h>
#else
#  include <netinet/in.h>	// htonl() ntohl()
#endif

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

// Float version
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

template<>
void FGProps2FDM<FGNetFDM>( SGPropertyNode *props, FGNetFDM *net, bool net_byte_order ) {
    unsigned int i;

    // Version sanity checking
    net->version = FG_NET_FDM_VERSION;

    // Aero parameters
    net->longitude = props->getDoubleValue("position/longitude-deg")
                       * SG_DEGREES_TO_RADIANS;
    net->latitude = props->getDoubleValue("position/latitude-deg")
                      * SG_DEGREES_TO_RADIANS;
    net->altitude = props->getDoubleValue("position/altitude-ft")
                      * SG_FEET_TO_METER;
    net->agl = props->getDoubleValue("position/altitude-agl-ft")
                 * SG_FEET_TO_METER;
    net->phi = SGMiscd::deg2rad( props->getDoubleValue("orientation/roll-deg") );
    net->theta = SGMiscd::deg2rad( props->getDoubleValue("orientation/pitch-deg") );
    net->psi = SGMiscd::deg2rad( props->getDoubleValue("orientation/heading-deg") );
    net->alpha = props->getDoubleValue("orientation/alpha-deg")
                   * SG_DEGREES_TO_RADIANS;
    net->beta = props->getDoubleValue("orientation/beta-deg")
                  * SG_DEGREES_TO_RADIANS;
    net->phidot = props->getDoubleValue("orientation/roll-rate-degps")
                    * SG_DEGREES_TO_RADIANS;
    net->thetadot = props->getDoubleValue("orientation/pitch-rate-degps")
                      * SG_DEGREES_TO_RADIANS;
    net->psidot = props->getDoubleValue("orientation/yaw-rate-degps")
                    * SG_DEGREES_TO_RADIANS;

    net->vcas = props->getDoubleValue("velocities/airspeed-kt");
    net->climb_rate = props->getDoubleValue("velocities/vertical-speed-fps");

    net->v_north = props->getDoubleValue("velocities/speed-north-fps", 0.0);
    net->v_east = props->getDoubleValue("velocities/speed-east-fps", 0.0);
    net->v_down = props->getDoubleValue("velocities/speed-down-fps", 0.0);
    net->v_body_u = props->getDoubleValue("velocities/uBody-fps", 0.0);
    net->v_body_v = props->getDoubleValue("velocities/vBody-fps", 0.0);
    net->v_body_w = props->getDoubleValue("velocities/wBody-fps", 0.0);

    net->A_X_pilot = props->getDoubleValue("accelerations/pilot/x-accel-fps_sec", 0.0);
    net->A_Y_pilot = props->getDoubleValue("/accelerations/pilot/y-accel-fps_sec", 0.0);
    net->A_Z_pilot = props->getDoubleValue("/accelerations/pilot/z-accel-fps_sec", 0.0);

    net->stall_warning = props->getDoubleValue("/sim/alarms/stall-warning", 0.0);
    net->slip_deg
      = props->getDoubleValue("/instrumentation/slip-skid-ball/indicated-slip-skid");

    // Engine parameters
    net->num_engines = FGNetFDM::FG_MAX_ENGINES;
    for ( i = 0; i < net->num_engines; ++i ) {
        SGPropertyNode *node = props->getNode("engines/engine", i, true);
        if ( node->getBoolValue( "running" ) ) {
            net->eng_state[i] = 2;
        } else if ( node->getBoolValue( "cranking" ) ) {
            net->eng_state[i] = 1;
        } else {
            net->eng_state[i] = 0;
        }
        net->rpm[i] = node->getDoubleValue( "rpm" );
        net->fuel_flow[i] = node->getDoubleValue( "fuel-flow-gph" );
        net->fuel_px[i] = node->getDoubleValue( "fuel-px-psi" );
        net->egt[i] = node->getDoubleValue( "egt-degf" );
        // cout << "egt = " << aero->EGT << endl;
        net->cht[i] = node->getDoubleValue( "cht-degf" );
        net->mp_osi[i] = node->getDoubleValue( "mp-osi" );
        net->tit[i] = node->getDoubleValue( "tit" );
        net->oil_temp[i] = node->getDoubleValue( "oil-temperature-degf" );
        net->oil_px[i] = node->getDoubleValue( "oil-pressure-psi" );
    }

    // Consumables
    net->num_tanks = FGNetFDM::FG_MAX_TANKS;
    for ( i = 0; i < net->num_tanks; ++i ) {
        SGPropertyNode *node = props->getNode("/consumables/fuel/tank", i, true);
        net->fuel_quantity[i] = node->getDoubleValue("level-gal_us");
        net->tank_selected[i] = node->getBoolValue("selected");
        net->capacity_m3[i] = node->getDoubleValue("capacity-m3");
        net->unusable_m3[i] = node->getDoubleValue("unusable-m3");
        net->density_kgpm3[i] = node->getDoubleValue("density-kgpm3");
        net->level_m3[i] = node->getDoubleValue("level-m3");
    }

    // Gear and flaps
    net->num_wheels = FGNetFDM::FG_MAX_WHEELS;
    for (i = 0; i < net->num_wheels; ++i ) {
        SGPropertyNode *node = props->getNode("/gear/gear", i, true);
        net->wow[i] = node->getIntValue("wow");
        net->gear_pos[i] = node->getDoubleValue("position-norm");
        net->gear_steer[i] = node->getDoubleValue("steering-norm");
        net->gear_compression[i] = node->getDoubleValue("compression-norm");
    }

    // the following really aren't used in this context
    SGTime time;
    net->cur_time = time.get_cur_time();
    net->warp = props->getIntValue("/sim/time/warp");
    net->visibility = props->getDoubleValue("/environment/visibility-m");

    // Control surface positions
    SGPropertyNode *node = props->getNode("/surface-positions", true);
    net->elevator = node->getDoubleValue( "elevator-pos-norm" );
    net->elevator_trim_tab
        = node->getDoubleValue( "elevator-trim-tab-pos-norm" );
    // FIXME: CLO 10/28/04 - This really should be separated out into 2 values
    net->left_flap = node->getDoubleValue( "flap-pos-norm" );
    net->right_flap = node->getDoubleValue( "flap-pos-norm" );
    net->left_aileron = node->getDoubleValue( "left-aileron-pos-norm" );
    net->right_aileron = node->getDoubleValue( "right-aileron-pos-norm" );
    net->rudder = node->getDoubleValue( "rudder-pos-norm" );
    net->nose_wheel = node->getDoubleValue( "nose-wheel-pos-norm" );
    net->speedbrake = node->getDoubleValue( "speedbrake-pos-norm" );
    net->spoilers = node->getDoubleValue( "spoilers-pos-norm" );

    if ( net_byte_order ) {
        // Convert the net buffer to network format
        net->version = htonl(net->version);

        htond(net->longitude);
        htond(net->latitude);
        htond(net->altitude);
        htonf(net->agl);
        htonf(net->phi);
        htonf(net->theta);
        htonf(net->psi);
        htonf(net->alpha);
        htonf(net->beta);

        htonf(net->phidot);
        htonf(net->thetadot);
        htonf(net->psidot);
        htonf(net->vcas);
        htonf(net->climb_rate);
        htonf(net->v_north);
        htonf(net->v_east);
        htonf(net->v_down);
        htonf(net->v_body_u);
        htonf(net->v_body_v);
        htonf(net->v_body_w);

        htonf(net->A_X_pilot);
        htonf(net->A_Y_pilot);
        htonf(net->A_Z_pilot);

        htonf(net->stall_warning);
        htonf(net->slip_deg);

        for ( i = 0; i < net->num_engines; ++i ) {
            net->eng_state[i] = htonl(net->eng_state[i]);
            htonf(net->rpm[i]);
            htonf(net->fuel_flow[i]);
            htonf(net->fuel_px[i]);
            htonf(net->egt[i]);
            htonf(net->cht[i]);
            htonf(net->mp_osi[i]);
            htonf(net->tit[i]);
            htonf(net->oil_temp[i]);
            htonf(net->oil_px[i]);
        }
        net->num_engines = htonl(net->num_engines);

        for ( i = 0; i < net->num_tanks; ++i ) {
            htonf(net->fuel_quantity[i]);
            htonl(net->tank_selected[i]);
            htond(net->capacity_m3[i]);
            htond(net->unusable_m3[i]);
            htond(net->density_kgpm3[i]);
            htond(net->level_m3[i]);
        }
        net->num_tanks = htonl(net->num_tanks);

        for ( i = 0; i < net->num_wheels; ++i ) {
            net->wow[i] = htonl(net->wow[i]);
            htonf(net->gear_pos[i]);
            htonf(net->gear_steer[i]);
            htonf(net->gear_compression[i]);
        }
        net->num_wheels = htonl(net->num_wheels);

        net->cur_time = htonl( net->cur_time );
        net->warp = htonl( net->warp );
        htonf(net->visibility);

        htonf(net->elevator);
        htonf(net->elevator_trim_tab);
        htonf(net->left_flap);
        htonf(net->right_flap);
        htonf(net->left_aileron);
        htonf(net->right_aileron);
        htonf(net->rudder);
        htonf(net->nose_wheel);
        htonf(net->speedbrake);
        htonf(net->spoilers);
    }
}

template<>
void FGFDM2Props<FGNetFDM>( SGPropertyNode *props, FGNetFDM *net, bool net_byte_order ) {
    unsigned int i;
    
    if ( net_byte_order ) {
        // Convert to the net buffer from network format
        net->version = ntohl(net->version);

        htond(net->longitude);
        htond(net->latitude);
        htond(net->altitude);
        htonf(net->agl);
        htonf(net->phi);
        htonf(net->theta);
        htonf(net->psi);
        htonf(net->alpha);
        htonf(net->beta);

        htonf(net->phidot);
        htonf(net->thetadot);
        htonf(net->psidot);
        htonf(net->vcas);
        htonf(net->climb_rate);
        htonf(net->v_north);
        htonf(net->v_east);
        htonf(net->v_down);
        htonf(net->v_body_u);
        htonf(net->v_body_v);
        htonf(net->v_body_w);

        htonf(net->A_X_pilot);
        htonf(net->A_Y_pilot);
        htonf(net->A_Z_pilot);

        htonf(net->stall_warning);
        htonf(net->slip_deg);

        net->num_engines = htonl(net->num_engines);
        for ( i = 0; i < net->num_engines; ++i ) {
            net->eng_state[i] = htonl(net->eng_state[i]);
            htonf(net->rpm[i]);
            htonf(net->fuel_flow[i]);
            htonf(net->fuel_px[i]);
            htonf(net->egt[i]);
            htonf(net->cht[i]);
            htonf(net->mp_osi[i]);
            htonf(net->tit[i]);
            htonf(net->oil_temp[i]);
            htonf(net->oil_px[i]);
        }

        net->num_tanks = htonl(net->num_tanks);
        for ( i = 0; i < net->num_tanks; ++i ) {
            htonf(net->fuel_quantity[i]);
            htonl(net->tank_selected[i]);
            htond(net->capacity_m3[i]);
            htond(net->unusable_m3[i]);
            htond(net->density_kgpm3[i]);
            htond(net->level_m3[i]);
        }

        net->num_wheels = htonl(net->num_wheels);
        for ( i = 0; i < net->num_wheels; ++i ) {
            net->wow[i] = htonl(net->wow[i]);
            htonf(net->gear_pos[i]);
            htonf(net->gear_steer[i]);
            htonf(net->gear_compression[i]);
        }

        net->cur_time = htonl(net->cur_time);
        net->warp = ntohl(net->warp);
        htonf(net->visibility);

        htonf(net->elevator);
        htonf(net->elevator_trim_tab);
        htonf(net->left_flap);
        htonf(net->right_flap);
        htonf(net->left_aileron);
        htonf(net->right_aileron);
        htonf(net->rudder);
        htonf(net->nose_wheel);
        htonf(net->speedbrake);
        htonf(net->spoilers);
    }

    if ( net->version == FG_NET_FDM_VERSION ) {
        // cout << "pos = " << net->longitude << " " << net->latitude << endl;
        // cout << "sea level rad = " << fdm_state.get_Sea_level_radius()
	//      << endl;
                                      
        props->setDoubleValue("position/latitude-deg",
                              net->latitude * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("position/longitude-deg",
                              net->longitude* SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("position/altitude-ft",
                              net->altitude * SG_METER_TO_FEET);
        
	if ( net->agl > -9000 ) {
	    props->setDoubleValue("position/altitude-agl-ft",
                                  net->agl * SG_METER_TO_FEET );
	} else {
	    double agl_m = net->altitude
              - props->getDoubleValue("environment/ground-elevation-m");
	    props->setDoubleValue("position/altitude-agl-ft",
                                  agl_m * SG_METER_TO_FEET );
	}
        props->setDoubleValue("orientation/roll-deg",
                              net->phi * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/pitch-deg",
                              net->theta * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/heading-deg",
                              net->psi * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/alpha-deg",
                              net->alpha * SG_RADIANS_TO_DEGREES );
        props->setDoubleValue("orientation/side-slip-rad",
                              net->beta * SG_RADIANS_TO_DEGREES );
        props->setDoubleValue("orientation/roll-rate-degps",
                              net->phidot * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/pitch-rate-degps",
                              net->thetadot * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/yaw-rate-degps",
                              net->psidot * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("velocities/airspeed-kt", net->vcas);
        props->setDoubleValue("velocities/vertical-speed-fps", net->climb_rate);
        props->setDoubleValue("velocities/speed-north-fps", net->v_north);
        props->setDoubleValue("velocities/speed-east-fps", net->v_east);
        props->setDoubleValue("velocities/speed-down-fps", net->v_down);
        props->setDoubleValue("velocities/uBody-fps", net->v_body_u);
        props->setDoubleValue("velocities/vBody-fps", net->v_body_v);
        props->setDoubleValue("velocities/wBody-fps", net->v_body_w);
        props->setDoubleValue("accelerations/pilot/x-accel-fps_sec",
                              net->A_X_pilot);
        props->setDoubleValue("accelerations/pilot/y-accel-fps_sec",
                              net->A_Y_pilot);
        props->setDoubleValue("accelerations/pilot/z-accel-fps_sec",
                              net->A_Z_pilot);

        props->setDoubleValue( "/sim/alarms/stall-warning", net->stall_warning );
	props->setDoubleValue( "/instrumentation/slip-skid-ball/indicated-slip-skid",
		     net->slip_deg );
	props->setBoolValue( "/instrumentation/slip-skid-ball/override", true );

	for ( i = 0; i < net->num_engines; ++i ) {
	    SGPropertyNode *node = props->getNode( "engines/engine", i, true );
	    
	    // node->setBoolValue("running", t->isRunning());
	    // node->setBoolValue("cranking", t->isCranking());
	    
	    // cout << net->eng_state[i] << endl;
	    if ( net->eng_state[i] == 0 ) {
		node->setBoolValue( "cranking", false );
		node->setBoolValue( "running", false );
	    } else if ( net->eng_state[i] == 1 ) {
		node->setBoolValue( "cranking", true );
		node->setBoolValue( "running", false );
	    } else if ( net->eng_state[i] == 2 ) {
		node->setBoolValue( "cranking", false );
		node->setBoolValue( "running", true );
	    }

	    node->setDoubleValue( "rpm", net->rpm[i] );
	    node->setDoubleValue( "fuel-flow-gph", net->fuel_flow[i] );
	    node->setDoubleValue( "fuel-px-psi", net->fuel_px[i] );
	    node->setDoubleValue( "egt-degf", net->egt[i] );
	    node->setDoubleValue( "cht-degf", net->cht[i] );
	    node->setDoubleValue( "mp-osi", net->mp_osi[i] );
	    node->setDoubleValue( "tit", net->tit[i] );
	    node->setDoubleValue( "oil-temperature-degf", net->oil_temp[i] );
	    node->setDoubleValue( "oil-pressure-psi", net->oil_px[i] );		
	}

	for (i = 0; i < net->num_tanks; ++i ) {
	    SGPropertyNode * node
		= props->getNode("/consumables/fuel/tank", i, true);
        node->setDoubleValue("level-gal_us", net->fuel_quantity[i]);
        node->setBoolValue("selected", net->tank_selected[i] > 0);
        node->setDoubleValue("capacity-m3", net->capacity_m3[i]);
        node->setDoubleValue("unusable-m3", net->unusable_m3[i]);
        node->setDoubleValue("density-kgpm3", net->density_kgpm3[i]);
        node->setDoubleValue("level-m3", net->level_m3[i]);
    }

	for (i = 0; i < net->num_wheels; ++i ) {
	    SGPropertyNode * node = props->getNode("/gear/gear", i, true);
	    node->setDoubleValue("wow", net->wow[i] );
	    node->setDoubleValue("position-norm", net->gear_pos[i] );
	    node->setDoubleValue("steering-norm", net->gear_steer[i] );
	    node->setDoubleValue("compression-norm", net->gear_compression[i] );
	}

	/* these are ignored for now  ... */
	/*
	if ( net->cur_time ) {
	    props->setLongValue("/sim/time/cur-time-override", net->cur_time);
	}

        props->setIntValue("/sim/time/warp", net->warp);
        last_warp = net->warp;
	*/

        SGPropertyNode *node = props->getNode("/surface-positions", true);
        node->setDoubleValue("elevator-pos-norm", net->elevator);
        node->setDoubleValue("elevator-trim-tab-pos-norm",
                             net->elevator_trim_tab);
	// FIXME: CLO 10/28/04 - This really should be separated out
	// into 2 values
        node->setDoubleValue("flap-pos-norm", net->left_flap);
        node->setDoubleValue("flap-pos-norm", net->right_flap);
        node->setDoubleValue("left-aileron-pos-norm", net->left_aileron);
        node->setDoubleValue("right-aileron-pos-norm", net->right_aileron);
        node->setDoubleValue("rudder-pos-norm", net->rudder);
        node->setDoubleValue("nose-wheel-pos-norm", net->nose_wheel);
        node->setDoubleValue("speedbrake-pos-norm", net->speedbrake);
        node->setDoubleValue("spoilers-pos-norm", net->spoilers);
    } else {
	SG_LOG( SG_IO, SG_ALERT,
                "Error: version mismatch in Net FGFDM2Props()" );
	SG_LOG( SG_IO, SG_ALERT,
		"\tread " << net->version << " need " << FG_NET_FDM_VERSION );
	SG_LOG( SG_IO, SG_ALERT,
		"\tNeeds to upgrade net_fdm.hxx and recompile." );
    }
}

#if FG_HAVE_DDS
#include "DDS/dds_fdm.h"
#include "DDS/dds_gui.h"
#include "DDS/dds_ctrls.h"

template<>
void FGProps2FDM<FG_DDS_FDM>( SGPropertyNode *props, FG_DDS_FDM *dds, bool net_byte_order ) {
    unsigned int i;

    // Version sanity checking
    dds->version = FG_DDS_FDM_VERSION;

    // Aero parameters
    dds->longitude = props->getDoubleValue("position/longitude-deg")
                       * SG_DEGREES_TO_RADIANS;
    dds->latitude = props->getDoubleValue("position/latitude-deg")
                      * SG_DEGREES_TO_RADIANS;
    dds->altitude = props->getDoubleValue("position/altitude-ft")
                      * SG_FEET_TO_METER;
    dds->agl = props->getDoubleValue("position/altitude-agl-ft")
                 * SG_FEET_TO_METER;
    dds->phi = SGMiscd::deg2rad( props->getDoubleValue("orientation/roll-deg") );
    dds->theta = SGMiscd::deg2rad( props->getDoubleValue("orientation/pitch-deg") );
    dds->psi = SGMiscd::deg2rad( props->getDoubleValue("orientation/heading-deg") );
    dds->alpha = props->getDoubleValue("orientation/alpha-deg") 
                   * SG_DEGREES_TO_RADIANS;
    dds->beta = props->getDoubleValue("orientation/beta-deg") 
                  * SG_DEGREES_TO_RADIANS;
    dds->phidot = props->getDoubleValue("orientation/roll-rate-degps") 
                    * SG_DEGREES_TO_RADIANS;
    dds->thetadot = props->getDoubleValue("orientation/pitch-rate-degps")
                      * SG_DEGREES_TO_RADIANS;
    dds->psidot = props->getDoubleValue("orientation/yaw-rate-degps")
                    * SG_DEGREES_TO_RADIANS;

    dds->vcas = props->getDoubleValue("velocities/airspeed-kt");
    dds->climb_rate = props->getDoubleValue("velocities/vertical-speed-fps");

    dds->v_north = props->getDoubleValue("velocities/speed-north-fps", 0.0);
    dds->v_east = props->getDoubleValue("velocities/speed-east-fps", 0.0);
    dds->v_down = props->getDoubleValue("velocities/speed-down-fps", 0.0);
    dds->v_body_u = props->getDoubleValue("velocities/uBody-fps", 0.0);
    dds->v_body_v = props->getDoubleValue("velocities/vBody-fps", 0.0);
    dds->v_body_w = props->getDoubleValue("velocities/wBody-fps", 0.0);

    dds->A_X_pilot = props->getDoubleValue("accelerations/pilot/x-accel-fps_sec", 0.0);
    dds->A_Y_pilot = props->getDoubleValue("/accelerations/pilot/y-accel-fps_sec", 0.0);
    dds->A_Z_pilot = props->getDoubleValue("/accelerations/pilot/z-accel-fps_sec", 0.0);

    dds->stall_warning = props->getDoubleValue("/sim/alarms/stall-warning", 0.0);
    dds->slip_deg
      = props->getDoubleValue("/instrumentation/slip-skid-ball/indicated-slip-skid");

    // Engine parameters
    dds->num_engines = FGNetFDM::FG_MAX_ENGINES;
    for ( i = 0; i < dds->num_engines; ++i ) {
        SGPropertyNode *node = props->getNode("engines/engine", i, true);
        if ( node->getBoolValue( "running" ) ) {
            dds->eng_state[i] = 2;
        } else if ( node->getBoolValue( "cranking" ) ) {
            dds->eng_state[i] = 1;
        } else {
            dds->eng_state[i] = 0;
        }
        dds->rpm[i] = node->getDoubleValue( "rpm" );
        dds->fuel_flow[i] = node->getDoubleValue( "fuel-flow-gph" );
        dds->fuel_px[i] = node->getDoubleValue( "fuel-px-psi" );
        dds->egt[i] = node->getDoubleValue( "egt-degf" );
        // cout << "egt = " << aero->EGT << endl;
        dds->cht[i] = node->getDoubleValue( "cht-degf" );
        dds->mp_osi[i] = node->getDoubleValue( "mp-osi" );
        dds->tit[i] = node->getDoubleValue( "tit" );
        dds->oil_temp[i] = node->getDoubleValue( "oil-temperature-degf" );
        dds->oil_px[i] = node->getDoubleValue( "oil-pressure-psi" );
    }

    // Consumables
    dds->num_tanks = FGNetFDM::FG_MAX_TANKS;
    for ( i = 0; i < dds->num_tanks; ++i ) {
        SGPropertyNode *node = props->getNode("/consumables/fuel/tank", i, true);
        dds->fuel_quantity[i] = node->getDoubleValue("level-gal_us");
        dds->tank_selected[i] = node->getBoolValue("selected");
        dds->capacity_m3[i] = node->getDoubleValue("capacity-m3");
        dds->unusable_m3[i] = node->getDoubleValue("unusable-m3");
        dds->density_kgpm3[i] = node->getDoubleValue("density-kgpm3");
        dds->level_m3[i] = node->getDoubleValue("level-m3");
    }

    // Gear and flaps
    dds->num_wheels = FGNetFDM::FG_MAX_WHEELS;
    for (i = 0; i < dds->num_wheels; ++i ) {
        SGPropertyNode *node = props->getNode("/gear/gear", i, true);
        dds->wow[i] = node->getIntValue("wow");
        dds->gear_pos[i] = node->getDoubleValue("position-norm");
        dds->gear_steer[i] = node->getDoubleValue("steering-norm");
        dds->gear_compression[i] = node->getDoubleValue("compression-norm");
    }

    // the following really aren't used in this context
    SGTime time;
    dds->cur_time = time.get_cur_time();
    dds->warp = props->getIntValue("/sim/time/warp");
    dds->visibility = props->getDoubleValue("/environment/visibility-m");

    // Control surface positions
    SGPropertyNode *node = props->getNode("/surface-positions", true);
    dds->elevator = node->getDoubleValue( "elevator-pos-norm" );
    dds->elevator_trim_tab
        = node->getDoubleValue( "elevator-trim-tab-pos-norm" );
    // FIXME: CLO 10/28/04 - This really should be separated out into 2 values
    dds->left_flap = node->getDoubleValue( "flap-pos-norm" );
    dds->right_flap = node->getDoubleValue( "flap-pos-norm" );
    dds->left_aileron = node->getDoubleValue( "left-aileron-pos-norm" );
    dds->right_aileron = node->getDoubleValue( "right-aileron-pos-norm" );
    dds->rudder = node->getDoubleValue( "rudder-pos-norm" );
    dds->nose_wheel = node->getDoubleValue( "nose-wheel-pos-norm" );
    dds->speedbrake = node->getDoubleValue( "speedbrake-pos-norm" );
    dds->spoilers = node->getDoubleValue( "spoilers-pos-norm" );
}

template<>
void FGFDM2Props<FG_DDS_FDM>( SGPropertyNode *props, FG_DDS_FDM *dds, bool net_byte_order ) {
    unsigned int i;

    if ( dds->version == FG_DDS_FDM_VERSION ) {
        // cout << "pos = " << dds->longitude << " " << dds->latitude << endl;
        // cout << "sea level rad = " << fdm_state.get_Sea_level_radius()
        //      << endl;

        props->setDoubleValue("position/latitude-deg",
                              dds->latitude * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("position/longitude-deg",
                              dds->longitude* SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("position/altitude-ft",
                              dds->altitude * SG_METER_TO_FEET);

        if ( dds->agl > -9000 ) {
            props->setDoubleValue("position/altitude-agl-ft",
                                  dds->agl * SG_METER_TO_FEET );
        } else {
            double agl_m = dds->altitude
              - props->getDoubleValue("environment/ground-elevation-m");
            props->setDoubleValue("position/altitude-agl-ft",
                                  agl_m * SG_METER_TO_FEET );
        }
        props->setDoubleValue("orientation/roll-deg",
                              dds->phi * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/pitch-deg",
                              dds->theta * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/heading-deg",
                              dds->psi * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/alpha-deg",
                              dds->alpha * SG_RADIANS_TO_DEGREES );
        props->setDoubleValue("orientation/side-slip-rad",
                              dds->beta * SG_RADIANS_TO_DEGREES );
        props->setDoubleValue("orientation/roll-rate-degps",
                              dds->phidot * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/pitch-rate-degps",
                              dds->thetadot * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/yaw-rate-degps",
                              dds->psidot * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("velocities/airspeed-kt", dds->vcas);
        props->setDoubleValue("velocities/vertical-speed-fps", dds->climb_rate);
        props->setDoubleValue("velocities/speed-north-fps", dds->v_north);
        props->setDoubleValue("velocities/speed-east-fps", dds->v_east);
        props->setDoubleValue("velocities/speed-down-fps", dds->v_down);
        props->setDoubleValue("velocities/uBody-fps", dds->v_body_u);
        props->setDoubleValue("velocities/vBody-fps", dds->v_body_v);
        props->setDoubleValue("velocities/wBody-fps", dds->v_body_w);
        props->setDoubleValue("accelerations/pilot/x-accel-fps_sec",
                              dds->A_X_pilot);
        props->setDoubleValue("accelerations/pilot/y-accel-fps_sec",
                              dds->A_Y_pilot);
        props->setDoubleValue("accelerations/pilot/z-accel-fps_sec",
                              dds->A_Z_pilot);

        props->setDoubleValue( "/sim/alarms/stall-warning", dds->stall_warning );
        props->setDoubleValue( "/instrumentation/slip-skid-ball/indicated-slip-skid",
                     dds->slip_deg );
        props->setBoolValue( "/instrumentation/slip-skid-ball/override", true );

        for ( i = 0; i < dds->num_engines; ++i ) {
            SGPropertyNode *node = props->getNode( "engines/engine", i, true );

            // node->setBoolValue("running", t->isRunning());
            // node->setBoolValue("cranking", t->isCranking());
            // cout << dds->eng_state[i] << endl;
            if ( dds->eng_state[i] == 0 ) {
                node->setBoolValue( "cranking", false );
                node->setBoolValue( "running", false );
            } else if ( dds->eng_state[i] == 1 ) {
                node->setBoolValue( "cranking", true );
                node->setBoolValue( "running", false );
            } else if ( dds->eng_state[i] == 2 ) {
                node->setBoolValue( "cranking", false );
                node->setBoolValue( "running", true );
            }

            node->setDoubleValue( "rpm", dds->rpm[i] );
            node->setDoubleValue( "fuel-flow-gph", dds->fuel_flow[i] );
            node->setDoubleValue( "fuel-px-psi", dds->fuel_px[i] );
            node->setDoubleValue( "egt-degf", dds->egt[i] );
            node->setDoubleValue( "cht-degf", dds->cht[i] );
            node->setDoubleValue( "mp-osi", dds->mp_osi[i] );
            node->setDoubleValue( "tit", dds->tit[i] );
            node->setDoubleValue( "oil-temperature-degf", dds->oil_temp[i] );
            node->setDoubleValue( "oil-pressure-psi", dds->oil_px[i] );
        }

        for (i = 0; i < dds->num_tanks; ++i ) {
            SGPropertyNode * node
                = props->getNode("/consumables/fuel/tank", i, true);
            node->setDoubleValue("level-gal_us", dds->fuel_quantity[i]);
            node->setBoolValue("selected", dds->tank_selected[i] > 0);
            node->setDoubleValue("capacity-m3", dds->capacity_m3[i]);
            node->setDoubleValue("unusable-m3", dds->unusable_m3[i]);
            node->setDoubleValue("density-kgpm3", dds->density_kgpm3[i]);
            node->setDoubleValue("level-m3", dds->level_m3[i]);
        }

        for (i = 0; i < dds->num_wheels; ++i ) {
            SGPropertyNode * node = props->getNode("/gear/gear", i, true);
            node->setDoubleValue("wow", dds->wow[i] );
            node->setDoubleValue("position-norm", dds->gear_pos[i] );
            node->setDoubleValue("steering-norm", dds->gear_steer[i] );
            node->setDoubleValue("compression-norm", dds->gear_compression[i] );
        }

        /* these are ignored for now  ... */
        /*
        if ( dds->cur_time ) {
            props->setLongValue("/sim/time/cur-time-override", dds->cur_time);
        }

        props->setIntValue("/sim/time/warp", dds->warp);
        last_warp = dds->warp;
        */
        SGPropertyNode *node = props->getNode("/surface-positions", true);
        node->setDoubleValue("elevator-pos-norm", dds->elevator);
        node->setDoubleValue("elevator-trim-tab-pos-norm",
                             dds->elevator_trim_tab);
        // FIXME: CLO 10/28/04 - This really should be separated out
        // into 2 values
        node->setDoubleValue("flap-pos-norm", dds->left_flap);
        node->setDoubleValue("flap-pos-norm", dds->right_flap);
        node->setDoubleValue("left-aileron-pos-norm", dds->left_aileron);
        node->setDoubleValue("right-aileron-pos-norm", dds->right_aileron);
        node->setDoubleValue("rudder-pos-norm", dds->rudder);
        node->setDoubleValue("nose-wheel-pos-norm", dds->nose_wheel);
        node->setDoubleValue("speedbrake-pos-norm", dds->speedbrake);
        node->setDoubleValue("spoilers-pos-norm", dds->spoilers);
    } else {
        SG_LOG( SG_IO, SG_ALERT,
                "Error: version mismatch in DDS FGFDM2Props()" );
        SG_LOG( SG_IO, SG_ALERT,
                "\tread " << dds->version << " need " << FG_DDS_FDM_VERSION );
        SG_LOG( SG_IO, SG_ALERT,
                "\tNeeds to upgrade DDS/dds_fdm.hxx and recompile." );
    }
}
#endif





template<>
void FGProps2GUI<FGNetGUI>( SGPropertyNode *props, FGNetGUI *net ) {
    static SGPropertyNode *nav_freq
	= props->getNode("/instrumentation/nav/frequencies/selected-mhz", true);
    static SGPropertyNode *nav_target_radial
	= props->getNode("/instrumentation/nav/radials/target-radial-deg", true);
    static SGPropertyNode *nav_inrange
	= props->getNode("/instrumentation/nav/in-range", true);
    static SGPropertyNode *nav_loc
	= props->getNode("/instrumentation/nav/nav-loc", true);
    static SGPropertyNode *nav_gs_dist_signed
	= props->getNode("/instrumentation/nav/gs-distance", true);
    static SGPropertyNode *nav_loc_dist
	= props->getNode("/instrumentation/nav/nav-distance", true);
    static SGPropertyNode *nav_reciprocal_radial
	= props->getNode("/instrumentation/nav/radials/reciprocal-radial-deg", true);
    static SGPropertyNode *nav_gs_deflection
	= props->getNode("/instrumentation/nav/gs-needle-deflection", true);
    unsigned int i;

    // Version sanity checking
    net->version = FG_NET_GUI_VERSION;

    // Aero parameters
    net->longitude = props->getDoubleValue("position/longitude-deg")
                       * SG_DEGREES_TO_RADIANS;
    net->latitude = props->getDoubleValue("position/latitude-deg")
                      * SG_DEGREES_TO_RADIANS;
    net->altitude = props->getDoubleValue("position/altitude-ft")
                      * SG_FEET_TO_METER;
    net->phi = SGMiscd::deg2rad( props->getDoubleValue("orientation/roll-deg") );
    net->theta = SGMiscd::deg2rad( props->getDoubleValue("orientation/pitch-deg") );
    net->psi = SGMiscd::deg2rad( props->getDoubleValue("orientation/heading-deg") );

    // Velocities
    net->vcas = props->getDoubleValue("velocities/airspeed-kt");
    net->climb_rate = props->getDoubleValue("velocities/vertical-speed-fps");

    // Consumables
    net->num_tanks = FGNetGUI::FG_MAX_TANKS;
    for ( i = 0; i < net->num_tanks; ++i ) {
        SGPropertyNode *node = props->getNode("/consumables/fuel/tank", i, true);
        net->fuel_quantity[i] = node->getDoubleValue("level-gal_us");
    }

    // Environment
    SGTime time;
    net->cur_time = time.get_cur_time();
    net->warp = props->getIntValue("/sim/time/warp");
    net->ground_elev = props->getDoubleValue("environment/ground-elevation-m");

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

template<>
void FGGUI2Props<FGNetGUI>( SGPropertyNode *props, FGNetGUI *net ) {
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
        
        // cout << "pos = " << net->longitude << " " << net->latitude << endl;
        // cout << "sea level rad = " << fdm_state->get_Sea_level_radius()
	//      << endl;
  
        props->setDoubleValue("position/latitude-deg",
                              net->latitude * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("position/longitude-deg",
                              net->longitude* SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("position/altitude-ft",
                              net->altitude * SG_METER_TO_FEET);

        props->setDoubleValue("orientation/roll-deg",
                              net->phi * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/pitch-deg",
                              net->theta * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/heading-deg",
                              net->psi * SG_RADIANS_TO_DEGREES);

        props->setDoubleValue("velocities/airspeed-kt", net->vcas);
        props->setDoubleValue("velocities/vertical-speed-fps", net->climb_rate);

	for (i = 0; i < net->num_tanks; ++i ) {
	    SGPropertyNode * node
		= props->getNode("/consumables/fuel/tank", i, true);
	    node->setDoubleValue("level-gal_us", net->fuel_quantity[i] );
	}

	if ( net->cur_time ) {
	    props->setLongValue("/sim/time/cur-time-override", net->cur_time);
	}

        props->setIntValue("/sim/time/warp", net->warp);

        // Approach
        props->setDoubleValue( "/instrumentation/nav[0]/frequencies/selected-mhz",
                     net->tuned_freq );
        props->setBoolValue( "/instrumentation/nav[0]/in-range", net->in_range > 0);
        props->setDoubleValue( "/instrumentation/dme/indicated-distance-nm", net->dist_nm );
        props->setDoubleValue( "/instrumentation/nav[0]/heading-needle-deflection",
                     net->course_deviation_deg );
        props->setDoubleValue( "/instrumentation/nav[0]/gs-needle-deflection",
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

#if FG_HAVE_DDS
template<>
void FGProps2GUI<FG_DDS_GUI>( SGPropertyNode *props, FG_DDS_GUI *dds ) {
    static SGPropertyNode *nav_freq
        = props->getNode("/instrumentation/nav/frequencies/selected-mhz", true);
    static SGPropertyNode *nav_target_radial
        = props->getNode("/instrumentation/nav/radials/target-radial-deg", true);
    static SGPropertyNode *nav_inrange
        = props->getNode("/instrumentation/nav/in-range", true);
    static SGPropertyNode *nav_loc
        = props->getNode("/instrumentation/nav/nav-loc", true);
    static SGPropertyNode *nav_gs_dist_signed
        = props->getNode("/instrumentation/nav/gs-distance", true);
    static SGPropertyNode *nav_loc_dist
        = props->getNode("/instrumentation/nav/nav-distance", true);
    static SGPropertyNode *nav_reciprocal_radial
        = props->getNode("/instrumentation/nav/radials/reciprocal-radial-deg", true);
    static SGPropertyNode *nav_gs_deflection
        = props->getNode("/instrumentation/nav/gs-needle-deflection", true);
    unsigned int i;

    // Version sanity checking
    dds->version = FG_DDS_GUI_VERSION;

    // Aero parameters
    dds->longitude = props->getDoubleValue("position/longitude-deg")
                       * SG_DEGREES_TO_RADIANS;
    dds->latitude = props->getDoubleValue("position/latitude-deg")
                      * SG_DEGREES_TO_RADIANS;
    dds->altitude = props->getDoubleValue("position/altitude-ft")
                      * SG_FEET_TO_METER;
    dds->phi = SGMiscd::deg2rad( props->getDoubleValue("orientation/roll-deg") );
    dds->theta = SGMiscd::deg2rad( props->getDoubleValue("orientation/pitch-deg") );
    dds->psi = SGMiscd::deg2rad( props->getDoubleValue("orientation/heading-deg") );

    // Velocities
    dds->vcas = props->getDoubleValue("velocities/airspeed-kt");
    dds->climb_rate = props->getDoubleValue("velocities/vertical-speed-fps");

    // Consumables
    dds->num_tanks = FGNetGUI::FG_MAX_TANKS;
    for ( i = 0; i < dds->num_tanks; ++i ) {
        SGPropertyNode *node = props->getNode("/consumables/fuel/tank", i, true);
        dds->fuel_quantity[i] = node->getDoubleValue("level-gal_us");
    }

    // Environment
    SGTime time;
    dds->cur_time = time.get_cur_time();
    dds->warp = props->getIntValue("/sim/time/warp");
    dds->ground_elev = props->getDoubleValue("environment/ground-elevation-m");

    // Approach
    dds->tuned_freq = nav_freq->getDoubleValue();
    dds->nav_radial = nav_target_radial->getDoubleValue();
    dds->in_range = nav_inrange->getBoolValue();

    if ( nav_loc->getBoolValue() ) {
        // is an ILS
        dds->dist_nm
            = nav_gs_dist_signed->getDoubleValue()
              * SG_METER_TO_NM;
    } else {
        // is a VOR
        dds->dist_nm = nav_loc_dist->getDoubleValue()
            * SG_METER_TO_NM;
    }

    dds->course_deviation_deg
        = nav_reciprocal_radial->getDoubleValue()
        - nav_target_radial->getDoubleValue();

    if ( dds->course_deviation_deg < -1000.0
         || dds->course_deviation_deg > 1000.0 )
    {
        // Sanity check ...
        dds->course_deviation_deg = 0.0;
    }
    while ( dds->course_deviation_deg >  180.0 ) {
        dds->course_deviation_deg -= 360.0;
    }
    while ( dds->course_deviation_deg >  180.0 ) {
        dds->course_deviation_deg -= 360.0;
    }
    while ( dds->course_deviation_deg < -180.0 ) {
        dds->course_deviation_deg += 360.0;
    }
    if ( fabs(dds->course_deviation_deg) > 90.0 )
        dds->course_deviation_deg
            = ( dds->course_deviation_deg<0.0
                ? -dds->course_deviation_deg - 180.0
                : -dds->course_deviation_deg + 180.0 );

    if ( nav_loc->getBoolValue() ) {
        // is an ILS
        dds->gs_deviation_deg
            = nav_gs_deflection->getDoubleValue()
            / 5.0;
    } else {
        // is an ILS
        dds->gs_deviation_deg = -9999.0;
    }
}

template<>
void FGGUI2Props<FG_DDS_GUI>( SGPropertyNode *props, FG_DDS_GUI *dds ) {
    unsigned int i;

    if ( dds->version == FG_DDS_GUI_VERSION ) {

        // cout << "pos = " << dds->longitude << " " << dds->latitude << endl;
        // cout << "sea level rad = " << fdm_state->get_Sea_level_radius()
        //      << endl;

        props->setDoubleValue("position/latitude-deg",
                              dds->latitude * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("position/longitude-deg",
                              dds->longitude* SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("position/altitude-ft",
                              dds->altitude * SG_METER_TO_FEET);

        props->setDoubleValue("orientation/roll-deg",
                              dds->phi * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/pitch-deg",
                              dds->theta * SG_RADIANS_TO_DEGREES);
        props->setDoubleValue("orientation/heading-deg",
                              dds->psi * SG_RADIANS_TO_DEGREES);

        props->setDoubleValue("velocities/airspeed-kt", dds->vcas);
        props->setDoubleValue("velocities/vertical-speed-fps", dds->climb_rate);

        for (i = 0; i < dds->num_tanks; ++i ) {
            SGPropertyNode * node
                = props->getNode("/consumables/fuel/tank", i, true);
            node->setDoubleValue("level-gal_us", dds->fuel_quantity[i] );
        }

        if ( dds->cur_time ) {
            props->setLongValue("/sim/time/cur-time-override", dds->cur_time);
        }

        props->setIntValue("/sim/time/warp", dds->warp );

        // Approach
        props->setDoubleValue( "/instrumentation/nav[0]/frequencies/selected-mhz",
                     dds->tuned_freq );
        props->setBoolValue( "/instrumentation/nav[0]/in-range", dds->in_range > 0);
        props->setDoubleValue( "/instrumentation/dme/indicated-distance-nm", dds->dist_nm );
        props->setDoubleValue( "/instrumentation/nav[0]/heading-needle-deflection",
                     dds->course_deviation_deg );
        props->setDoubleValue( "/instrumentation/nav[0]/gs-needle-deflection",
                     dds->gs_deviation_deg );
    } else {
        SG_LOG( SG_IO, SG_ALERT,
                "Error: version mismatch in FGNetNativeGUI2Props()" );
        SG_LOG( SG_IO, SG_ALERT,
                "\tread " << dds->version << " need " << FG_DDS_GUI_VERSION );
        SG_LOG( SG_IO, SG_ALERT,
                "\tNeed to upgrade net_fdm.hxx and recompile." );
    }
}
#endif


// Populate the FGNetCtrls structure from the property tree.
template<>
void FGProps2Ctrls<FGNetCtrls>( SGPropertyNode *props, FGNetCtrls *net, bool honor_freezes,
                                bool net_byte_order )
{
    int i;
    SGPropertyNode *node;
    SGPropertyNode *fuelpump;
    SGPropertyNode *tempnode;

    // fill in values
    node  = props->getNode("/controls/flight", true);
    net->version = FG_NET_CTRLS_VERSION;
    net->aileron = node->getDoubleValue( "aileron" );
    net->elevator = node->getDoubleValue( "elevator" );
    net->rudder = node->getDoubleValue( "rudder" );
    net->aileron_trim = node->getDoubleValue( "aileron-trim" );
    net->elevator_trim = node->getDoubleValue( "elevator-trim" );
    net->rudder_trim = node->getDoubleValue( "rudder-trim" );
    net->flaps = node->getDoubleValue( "flaps" );
    net->speedbrake = node->getDoubleValue( "speedbrake" );
    net->spoilers = node->getDoubleValue( "spoilers" );
    net->flaps_power
        = props->getDoubleValue( "/systems/electrical/outputs/flaps", 1.0 ) >= 1.0;
    net->flap_motor_ok = node->getBoolValue( "flaps-serviceable" );

    net->num_engines = FGNetCtrls::FG_MAX_ENGINES;
    for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {
        // Controls
        node = props->getNode("/controls/engines/engine", i );
        fuelpump = props->getNode("/systems/electrical/outputs/fuel-pump", i );

        tempnode = node->getChild("starter");
        if ( tempnode != NULL ) {
            net->starter_power[i] = ( tempnode->getDoubleValue() >= 1.0 );
        }
        tempnode = node->getChild("master-bat");
        if ( tempnode != NULL ) {
            net->master_bat[i] = tempnode->getBoolValue();
        }
        tempnode = node->getChild("master-alt");
        if ( tempnode != NULL ) {
            net->master_alt[i] = tempnode->getBoolValue();
        }

        net->throttle[i] = node->getDoubleValue( "throttle", 0.0 );
	net->mixture[i] = node->getDoubleValue( "mixture", 0.0 );
	net->prop_advance[i] = node->getDoubleValue( "propeller-pitch", 0.0 );
	net->condition[i] = node->getDoubleValue( "condition", 0.0 );
	net->magnetos[i] = node->getIntValue( "magnetos", 0 );
	if ( i == 0 ) {
	  // cout << "Magnetos -> " << node->getIntValue( "magnetos", 0 );
	}
	if ( i == 0 ) {
	  // cout << "Starter -> " << node->getIntValue( "starter", false )
	  //      << endl;
	}

        if ( fuelpump != NULL ) {
            net->fuel_pump_power[i] = ( fuelpump->getDoubleValue() >= 1.0 );
        } else {
            net->fuel_pump_power[i] = 0;
        }

	// Faults
	SGPropertyNode *faults = node->getChild( "faults", 0, true );
	net->engine_ok[i] = faults->getBoolValue( "serviceable", true );
	net->mag_left_ok[i]
	  = faults->getBoolValue( "left-magneto-serviceable", true );
	net->mag_right_ok[i]
	  = faults->getBoolValue( "right-magneto-serviceable", true);
	net->spark_plugs_ok[i]
	  = faults->getBoolValue( "spark-plugs-serviceable", true );
	net->oil_press_status[i]
	  = faults->getIntValue( "oil-pressure-status", 0 );
	net->fuel_pump_ok[i]
	  = faults->getBoolValue( "fuel-pump-serviceable", true );
    }
    net->num_tanks = FGNetCtrls::FG_MAX_TANKS;
    for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
        node = props->getNode("/controls/fuel/tank", i);
        if ( node->getChild("fuel_selector") != 0 ) {
            net->fuel_selector[i]
                = node->getChild("fuel_selector")->getBoolValue();
        } else {
            net->fuel_selector[i] = false;
        }
    }
    node = props->getNode("/controls/gear", true);
    net->brake_left = node->getChild("brake-left")->getDoubleValue();
    net->brake_right = node->getChild("brake-right")->getDoubleValue();
    net->copilot_brake_left
        = node->getChild("copilot-brake-left")->getDoubleValue();
    net->copilot_brake_right
        = node->getChild("copilot-brake-right")->getDoubleValue();
    net->brake_parking = node->getChild("brake-parking")->getDoubleValue();

    net->gear_handle = props->getBoolValue( "/controls/gear/gear-down" );

    net->master_avionics = props->getBoolValue("/controls/switches/master-avionics");

    net->wind_speed_kt = props->getDoubleValue("/environment/wind-speed-kt");
    net->wind_dir_deg = props->getDoubleValue("/environment/wind-from-heading-deg");
    net->turbulence_norm =
        props->getDoubleValue("/environment/turbulence/magnitude-norm");

    net->temp_c = props->getDoubleValue("/environment/temperature-degc");
    net->press_inhg = props->getDoubleValue("/environment/pressure-sea-level-inhg");

    net->hground = props->getDoubleValue("/position/ground-elev-m");
    net->magvar = props->getDoubleValue("/environment/magnetic-variation-deg");

    net->icing = props->getBoolValue("/hazards/icing/wing");

    net->speedup = props->getIntValue("/sim/speed-up");
    net->freeze = 0;
    if ( honor_freezes ) {
        if ( props->getBoolValue("/sim/freeze/master") ) {
            net->freeze |= 0x01;
        }
        if ( props->getBoolValue("/sim/freeze/position") ) {
            net->freeze |= 0x02;
        }
        if ( props->getBoolValue("/sim/freeze/fuel") ) {
            net->freeze |= 0x04;
        }
    }

    if ( net_byte_order ) {
        // convert to network byte order
        net->version = htonl(net->version);
        htond(net->aileron);
        htond(net->elevator);
        htond(net->rudder);
        htond(net->aileron_trim);
        htond(net->elevator_trim);
        htond(net->rudder_trim);
        htond(net->flaps);
        htond(net->speedbrake);
        htond(net->spoilers);
        net->flaps_power = htonl(net->flaps_power);
        net->flap_motor_ok = htonl(net->flap_motor_ok);

        net->num_engines = htonl(net->num_engines);
        for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {
            net->master_bat[i] = htonl(net->master_bat[i]);
            net->master_alt[i] = htonl(net->master_alt[i]);
            net->magnetos[i] = htonl(net->magnetos[i]);
            net->starter_power[i] = htonl(net->starter_power[i]);
            htond(net->throttle[i]);
            htond(net->mixture[i]);
            net->fuel_pump_power[i] = htonl(net->fuel_pump_power[i]);
            htond(net->prop_advance[i]);
            htond(net->condition[i]);
            net->engine_ok[i] = htonl(net->engine_ok[i]);
            net->mag_left_ok[i] = htonl(net->mag_left_ok[i]);
            net->mag_right_ok[i] = htonl(net->mag_right_ok[i]);
            net->spark_plugs_ok[i] = htonl(net->spark_plugs_ok[i]);
            net->oil_press_status[i] = htonl(net->oil_press_status[i]);
            net->fuel_pump_ok[i] = htonl(net->fuel_pump_ok[i]);
        }

        net->num_tanks = htonl(net->num_tanks);
        for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
            net->fuel_selector[i] = htonl(net->fuel_selector[i]);
        }

        net->cross_feed = htonl(net->cross_feed);
        htond(net->brake_left);
        htond(net->brake_right);
        htond(net->copilot_brake_left);
        htond(net->copilot_brake_right);
        htond(net->brake_parking);
        net->gear_handle = htonl(net->gear_handle);
        net->master_avionics = htonl(net->master_avionics);
        htond(net->wind_speed_kt);
        htond(net->wind_dir_deg);
        htond(net->turbulence_norm);
	htond(net->temp_c);
        htond(net->press_inhg);
	htond(net->hground);
        htond(net->magvar);
        net->icing = htonl(net->icing);
        net->speedup = htonl(net->speedup);
        net->freeze = htonl(net->freeze);
    }
}


// Update the property tree from the FGNetCtrls structure.
template<>
void FGCtrls2Props<FGNetCtrls>( SGPropertyNode *props, FGNetCtrls *net, bool honor_freezes,
                                bool net_byte_order )
{
    int i;

    SGPropertyNode * node;

    if ( net_byte_order ) {
        // convert from network byte order
        net->version = htonl(net->version);
        htond(net->aileron);
        htond(net->elevator);
        htond(net->rudder);
        htond(net->aileron_trim);
        htond(net->elevator_trim);
        htond(net->rudder_trim);
        htond(net->flaps);
        htond(net->speedbrake);
        htond(net->spoilers);
        net->flaps_power = htonl(net->flaps_power);
        net->flap_motor_ok = htonl(net->flap_motor_ok);

        net->num_engines = htonl(net->num_engines);
        for ( i = 0; i < (int)net->num_engines; ++i ) {
            net->master_bat[i] = htonl(net->master_bat[i]);
            net->master_alt[i] = htonl(net->master_alt[i]);
            net->magnetos[i] = htonl(net->magnetos[i]);
            net->starter_power[i] = htonl(net->starter_power[i]);
            htond(net->throttle[i]);
            htond(net->mixture[i]);
            net->fuel_pump_power[i] = htonl(net->fuel_pump_power[i]);
            htond(net->prop_advance[i]);
            htond(net->condition[i]);
            net->engine_ok[i] = htonl(net->engine_ok[i]);
            net->mag_left_ok[i] = htonl(net->mag_left_ok[i]);
            net->mag_right_ok[i] = htonl(net->mag_right_ok[i]);
            net->spark_plugs_ok[i] = htonl(net->spark_plugs_ok[i]);
            net->oil_press_status[i] = htonl(net->oil_press_status[i]);
            net->fuel_pump_ok[i] = htonl(net->fuel_pump_ok[i]);
        }

        net->num_tanks = htonl(net->num_tanks);
        for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
            net->fuel_selector[i] = htonl(net->fuel_selector[i]);
        }

        net->cross_feed = htonl(net->cross_feed);
        htond(net->brake_left);
        htond(net->brake_right);
        htond(net->copilot_brake_left);
        htond(net->copilot_brake_right);
        htond(net->brake_parking);
        net->gear_handle = htonl(net->gear_handle);
        net->master_avionics = htonl(net->master_avionics);
        htond(net->wind_speed_kt);
        htond(net->wind_dir_deg);
        htond(net->turbulence_norm);
	htond(net->temp_c);
        htond(net->press_inhg);
        htond(net->hground);
        htond(net->magvar);
        net->icing = htonl(net->icing);
        net->speedup = htonl(net->speedup);
        net->freeze = htonl(net->freeze);
    }

    if ( net->version != FG_NET_CTRLS_VERSION ) {
	SG_LOG( SG_IO, SG_ALERT,
                "Version mismatch with raw controls packet format." );
        SG_LOG( SG_IO, SG_ALERT,
                "FlightGear needs version = " << FG_NET_CTRLS_VERSION
                << " but is receiving version = "  << net->version );
    }
    node = props->getNode("/controls/flight", true);
    node->setDoubleValue( "aileron", net->aileron );
    node->setDoubleValue( "elevator", net->elevator );
    node->setDoubleValue( "rudder", net->rudder );
    node->setDoubleValue( "aileron-trim", net->aileron_trim );
    node->setDoubleValue( "elevator-trim", net->elevator_trim );
    node->setDoubleValue( "rudder-trim", net->rudder_trim );
    node->setDoubleValue( "flaps", net->flaps );
    node->setDoubleValue( "speedbrake", net->speedbrake );  //JWW
    // or
    node->setDoubleValue( "spoilers", net->spoilers );  //JWW
//    cout << "NET->Spoilers: " << net->spoilers << endl;
    props->setBoolValue( "/systems/electrical/outputs/flaps", net->flaps_power > 0 );
    node->setBoolValue( "flaps-serviceable", net->flap_motor_ok > 0 );

    for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {
        // Controls
        node = props->getNode("/controls/engines/engine", i);
        node->getChild( "throttle" )->setDoubleValue( net->throttle[i] );
        node->getChild( "mixture" )->setDoubleValue( net->mixture[i] );
        node->getChild( "propeller-pitch" )
            ->setDoubleValue( net->prop_advance[i] );
        node->getChild( "condition" )
            ->setDoubleValue( net->condition[i] );
        node->getChild( "magnetos" )->setDoubleValue( net->magnetos[i] );
        node->getChild( "starter" )->setDoubleValue( net->starter_power[i] );
        node->getChild( "feed_tank" )->setIntValue( net->feed_tank_to[i] );
        node->getChild( "reverser" )->setBoolValue( net->reverse[i] > 0 );
	// Faults
	SGPropertyNode *faults = node->getNode( "faults", true );
	faults->setBoolValue( "serviceable", net->engine_ok[i] > 0 );
	faults->setBoolValue( "left-magneto-serviceable",
			      net->mag_left_ok[i] > 0 );
	faults->setBoolValue( "right-magneto-serviceable",
			      net->mag_right_ok[i] > 0);
	faults->setBoolValue( "spark-plugs-serviceable",
			      net->spark_plugs_ok[i] > 0);
	faults->setIntValue( "oil-pressure-status", net->oil_press_status[i] );
	faults->setBoolValue( "fuel-pump-serviceable", net->fuel_pump_ok[i] > 0);
    }

    props->setBoolValue( "/systems/electrical/outputs/fuel-pump",
               net->fuel_pump_power[0] > 0);

    for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
        node = props->getNode( "/controls/fuel/tank", i );
        node->getChild( "fuel_selector" )
            ->setBoolValue( net->fuel_selector[i] > 0 );
//        node->getChild( "to_tank" )->xfer_tank( i, net->xfer_to[i] );
    }
    node = props->getNode( "/controls/gear" );
    if ( node != NULL ) {
        node->getChild( "brake-left" )->setDoubleValue( net->brake_left );
        node->getChild( "brake-right" )->setDoubleValue( net->brake_right );
        node->getChild( "copilot-brake-left" )
            ->setDoubleValue( net->copilot_brake_left );
        node->getChild( "copilot-brake-right" )
            ->setDoubleValue( net->copilot_brake_right );
        node->getChild( "brake-parking" )->setDoubleValue( net->brake_parking );
    }

    node = props->getNode( "/controls/gear", true );
    node->setBoolValue( "gear-down", net->gear_handle > 0 );
//    node->setDoubleValue( "brake-parking", net->brake_parking );
//    node->setDoubleValue( net->brake_left );
//    node->setDoubleValue( net->brake_right );

    node = props->getNode( "/controls/switches", true );
    node->setBoolValue( "master-bat", net->master_bat[0] != 0 );
    node->setBoolValue( "master-alt", net->master_alt[0] != 0 );
    node->setBoolValue( "master-avionics", net->master_avionics > 0 );

    node = props->getNode( "/environment", true );
    node->setDoubleValue( "wind-speed-kt", net->wind_speed_kt );
    node->setDoubleValue( "wind-from-heading-deg", net->wind_dir_deg );
    node->setDoubleValue( "turbulence/magnitude-norm", net->turbulence_norm );
    node->setDoubleValue( "magnetic-variation-deg", net->magvar );

    node->setDoubleValue( "/environment/temperature-degc",
			  net->temp_c );
    node->setDoubleValue( "/environment/pressure-sea-level-inhg",
			  net->press_inhg );

    // ground elevation ???

    props->setDoubleValue("/hazards/icing/wing", net->icing);
    
    node = props->getNode( "/radios", true );
    node->setDoubleValue( "comm/frequencies/selected-mhz[0]", net->comm_1 );
    node->setDoubleValue( "nav/frequencies/selected-mhz[0]", net->nav_1 );
    node->setDoubleValue( "nav[1]/frequencies/selected-mhz[0]", net->nav_2 );

    props->setDoubleValue( "/sim/speed-up", net->speedup );

    if ( honor_freezes ) {
        node = props->getNode( "/sim/freeze", true );
        node->setBoolValue( "master", (net->freeze & 0x01) > 0 );
        node->setBoolValue( "position", (net->freeze & 0x02) > 0 );
        node->setBoolValue( "fuel", (net->freeze & 0x04) > 0 );
    }
}

#if FG_HAVE_DDS
// Populate the FG_DDS_Ctrls structure from the property tree.
template<>
void FGProps2Ctrls<FG_DDS_Ctrls>( SGPropertyNode *props, FG_DDS_Ctrls *dds, bool honor_freezes, bool net_byte_order )
{
    int i;
    SGPropertyNode *node;
    SGPropertyNode *fuelpump;
    SGPropertyNode *tempnode;

    // fill in values
    node  = props->getNode("/controls/flight", true);
    dds->version = FG_DDS_CTRLS_VERSION;
    dds->aileron = node->getDoubleValue( "aileron" );
    dds->elevator = node->getDoubleValue( "elevator" );
    dds->rudder = node->getDoubleValue( "rudder" );
    dds->aileron_trim = node->getDoubleValue( "aileron-trim" );
    dds->elevator_trim = node->getDoubleValue( "elevator-trim" );
    dds->rudder_trim = node->getDoubleValue( "rudder-trim" );
    dds->flaps = node->getDoubleValue( "flaps" );
    dds->speedbrake = node->getDoubleValue( "speedbrake" );
    dds->spoilers = node->getDoubleValue( "spoilers" );
    dds->flaps_power
        = props->getDoubleValue( "/systems/electrical/outputs/flaps", 1.0 ) >= 1.0;
    dds->flap_motor_ok = node->getBoolValue( "flaps-serviceable" );

    dds->num_engines = FGNetCtrls::FG_MAX_ENGINES;
    for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {
        // Controls
        node = props->getNode("/controls/engines/engine", i );
        fuelpump = props->getNode("/systems/electrical/outputs/fuel-pump", i );

        tempnode = node->getChild("starter");
        if ( tempnode != NULL ) {
            dds->starter_power[i] = ( tempnode->getDoubleValue() >= 1.0 );
        }
        tempnode = node->getChild("master-bat");
        if ( tempnode != NULL ) {
            dds->master_bat[i] = tempnode->getBoolValue();
        }
        tempnode = node->getChild("master-alt");
        if ( tempnode != NULL ) {
            dds->master_alt[i] = tempnode->getBoolValue();
        }

        dds->throttle[i] = node->getDoubleValue( "throttle", 0.0 );
        dds->mixture[i] = node->getDoubleValue( "mixture", 0.0 );
        dds->prop_advance[i] = node->getDoubleValue( "propeller-pitch", 0.0 );
        dds->condition[i] = node->getDoubleValue( "condition", 0.0 );
        dds->magnetos[i] = node->getIntValue( "magnetos", 0 );
        if ( i == 0 ) {
          // cout << "Magnetos -> " << node->getIntValue( "magnetos", 0 );
        }
        if ( i == 0 ) {
          // cout << "Starter -> " << node->getIntValue( "starter", false )
          //      << endl;
        }

        if ( fuelpump != NULL ) {
            dds->fuel_pump_power[i] = ( fuelpump->getDoubleValue() >= 1.0 );
        } else {
            dds->fuel_pump_power[i] = 0;
        }

        // Faults
        SGPropertyNode *faults = node->getChild( "faults", 0, true );
        dds->engine_ok[i] = faults->getBoolValue( "serviceable", true );
        dds->mag_left_ok[i]
          = faults->getBoolValue( "left-magneto-serviceable", true );
        dds->mag_right_ok[i]
          = faults->getBoolValue( "right-magneto-serviceable", true);
        dds->spark_plugs_ok[i]
          = faults->getBoolValue( "spark-plugs-serviceable", true );
        dds->oil_press_status[i]
          = faults->getIntValue( "oil-pressure-status", 0 );
        dds->fuel_pump_ok[i]
          = faults->getBoolValue( "fuel-pump-serviceable", true );
    }
    dds->num_tanks = FGNetCtrls::FG_MAX_TANKS;
    for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
        node = props->getNode("/controls/fuel/tank", i);
        if ( node->getChild("fuel_selector") != 0 ) {
            dds->fuel_selector[i]
                = node->getChild("fuel_selector")->getBoolValue();
        } else {
            dds->fuel_selector[i] = false;
        }
    }
    node = props->getNode("/controls/gear", true);
    dds->brake_left = node->getChild("brake-left")->getDoubleValue();
    dds->brake_right = node->getChild("brake-right")->getDoubleValue();
    dds->copilot_brake_left
        = node->getChild("copilot-brake-left")->getDoubleValue();
    dds->copilot_brake_right
        = node->getChild("copilot-brake-right")->getDoubleValue();
    dds->brake_parking = node->getChild("brake-parking")->getDoubleValue();

    dds->gear_handle = props->getBoolValue( "/controls/gear/gear-down" );

    dds->master_avionics = props->getBoolValue("/controls/switches/master-avionics");

    dds->wind_speed_kt = props->getDoubleValue("/environment/wind-speed-kt");
    dds->wind_dir_deg = props->getDoubleValue("/environment/wind-from-heading-deg");
    dds->turbulence_norm =
        props->getDoubleValue("/environment/turbulence/magnitude-norm");

    dds->temp_c = props->getDoubleValue("/environment/temperature-degc");
    dds->press_inhg = props->getDoubleValue("/environment/pressure-sea-level-inhg");

    dds->hground = props->getDoubleValue("/position/ground-elev-m");
    dds->magvar = props->getDoubleValue("/environment/magnetic-variation-deg");

    dds->icing = props->getBoolValue("/hazards/icing/wing");

    dds->speedup = props->getIntValue("/sim/speed-up");
    dds->freeze = 0;
    if ( honor_freezes ) {
        if ( props->getBoolValue("/sim/freeze/master") ) {
            dds->freeze |= 0x01;
        }
        if ( props->getBoolValue("/sim/freeze/position") ) {
            dds->freeze |= 0x02;
        }
        if ( props->getBoolValue("/sim/freeze/fuel") ) {
            dds->freeze |= 0x04;
        }
    }
}

// Update the property tree from the FG_DDS_Ctrls structure.
template<>
void FGCtrls2Props<FG_DDS_Ctrls>( SGPropertyNode *props, FG_DDS_Ctrls *dds, bool honor_freezes, bool net_byte_order )
{
    int i;

    SGPropertyNode * node;

    if ( dds->version != FG_DDS_CTRLS_VERSION ) {
        SG_LOG( SG_IO, SG_ALERT,
                "Version mismatch with raw controls packet format." );
        SG_LOG( SG_IO, SG_ALERT,
                "FlightGear needs version = " << FG_DDS_CTRLS_VERSION
                << " but is receiving version = "  << dds->version );
    }
    node = props->getNode("/controls/flight", true);
    node->setDoubleValue( "aileron", dds->aileron );
    node->setDoubleValue( "elevator", dds->elevator );
    node->setDoubleValue( "rudder", dds->rudder );
    node->setDoubleValue( "aileron-trim", dds->aileron_trim );
    node->setDoubleValue( "elevator-trim", dds->elevator_trim );
    node->setDoubleValue( "rudder-trim", dds->rudder_trim );
    node->setDoubleValue( "flaps", dds->flaps );
    node->setDoubleValue( "speedbrake", dds->speedbrake );  //JWW
    // or
    node->setDoubleValue( "spoilers", dds->spoilers );  //JWW
//    cout << "NET->Spoilers: " << dds->spoilers << endl;
    props->setBoolValue( "/systems/electrical/outputs/flaps", dds->flaps_power > 0 );
    node->setBoolValue( "flaps-serviceable", dds->flap_motor_ok > 0 );

    for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {
        // Controls
        node = props->getNode("/controls/engines/engine", i);
        node->getChild( "throttle" )->setDoubleValue( dds->throttle[i] );
        node->getChild( "mixture" )->setDoubleValue( dds->mixture[i] );
        node->getChild( "propeller-pitch" )
            ->setDoubleValue( dds->prop_advance[i] );
        node->getChild( "condition" )
            ->setDoubleValue( dds->condition[i] );
        node->getChild( "magnetos" )->setDoubleValue( dds->magnetos[i] );
        node->getChild( "starter" )->setDoubleValue( dds->starter_power[i] );
        node->getChild( "feed_tank" )->setIntValue( dds->feed_tank_to[i] );
        node->getChild( "reverser" )->setBoolValue( dds->reverse[i] > 0 );
        // Faults
        SGPropertyNode *faults = node->getNode( "faults", true );
        faults->setBoolValue( "serviceable", dds->engine_ok[i] > 0 );
        faults->setBoolValue( "left-magneto-serviceable",
                              dds->mag_left_ok[i] > 0 );
        faults->setBoolValue( "right-magneto-serviceable",
                              dds->mag_right_ok[i] > 0);
        faults->setBoolValue( "spark-plugs-serviceable",
                              dds->spark_plugs_ok[i] > 0);
        faults->setIntValue( "oil-pressure-status", dds->oil_press_status[i] );
        faults->setBoolValue( "fuel-pump-serviceable", dds->fuel_pump_ok[i] > 0);
    }

    props->setBoolValue( "/systems/electrical/outputs/fuel-pump",
               dds->fuel_pump_power[0] > 0);

    for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
        node = props->getNode( "/controls/fuel/tank", i );
        node->getChild( "fuel_selector" )
            ->setBoolValue( dds->fuel_selector[i] > 0 );
//        node->getChild( "to_tank" )->xfer_tank( i, dds->xfer_to[i] );
    }
    node = props->getNode( "/controls/gear" );
    if ( node != NULL ) {
        node->getChild( "brake-left" )->setDoubleValue( dds->brake_left );
        node->getChild( "brake-right" )->setDoubleValue( dds->brake_right );
        node->getChild( "copilot-brake-left" )
            ->setDoubleValue( dds->copilot_brake_left );
        node->getChild( "copilot-brake-right" )
            ->setDoubleValue( dds->copilot_brake_right );
        node->getChild( "brake-parking" )->setDoubleValue( dds->brake_parking );
    }

    node = props->getNode( "/controls/gear", true );
    node->setBoolValue( "gear-down", dds->gear_handle > 0 );
//    node->setDoubleValue( "brake-parking", dds->brake_parking );
//    node->setDoubleValue( dds->brake_left );
//    node->setDoubleValue( dds->brake_right );

    node = props->getNode( "/controls/switches", true );
    node->setBoolValue( "master-bat", dds->master_bat[0] != 0 );
    node->setBoolValue( "master-alt", dds->master_alt[0] != 0 );
    node->setBoolValue( "master-avionics", dds->master_avionics > 0 );

    node = props->getNode( "/environment", true );
    node->setDoubleValue( "wind-speed-kt", dds->wind_speed_kt );
    node->setDoubleValue( "wind-from-heading-deg", dds->wind_dir_deg );
    node->setDoubleValue( "turbulence/magnitude-norm", dds->turbulence_norm );
    node->setDoubleValue( "magnetic-variation-deg", dds->magvar );

    node->setDoubleValue( "/environment/temperature-degc",
                          dds->temp_c );
    node->setDoubleValue( "/environment/pressure-sea-level-inhg",
                          dds->press_inhg );

    // ground elevation ???

    props->setDoubleValue("/hazards/icing/wing", dds->icing);

    node = props->getNode( "/radios", true );
    node->setDoubleValue( "comm/frequencies/selected-mhz[0]", dds->comm_1 );
    node->setDoubleValue( "nav/frequencies/selected-mhz[0]", dds->nav_1 );
    node->setDoubleValue( "nav[1]/frequencies/selected-mhz[0]", dds->nav_2 );

    props->setDoubleValue( "/sim/speed-up", dds->speedup );

    if ( honor_freezes ) {
        node = props->getNode( "/sim/freeze", true );
        node->setBoolValue( "master", (dds->freeze & 0x01) > 0 );
        node->setBoolValue( "position", (dds->freeze & 0x02) > 0 );
        node->setBoolValue( "fuel", (dds->freeze & 0x04) > 0 );
    }
}
#endif

