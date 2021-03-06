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
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/io/lowlevel.hxx> // endian tests
#include <simgear/io/iochannel.hxx>
#include <simgear/timing/sg_time.hxx>

#if FG_HAVE_DDS
#include <simgear/io/SGDataDistributionService.hxx>
#endif

#include <FDM/flightProperties.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>

#include "native_fdm.hxx"

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
void FGProps2FDM<FGNetFDM>( FGNetFDM *net, bool net_byte_order ) {
    unsigned int i;

    FlightProperties fdm_state;

    // Version sanity checking
    net->version = FG_NET_FDM_VERSION;

    // Aero parameters
    net->longitude = fdm_state.get_Longitude();
    net->latitude = fdm_state.get_Latitude();
    net->altitude = fdm_state.get_Altitude() * SG_FEET_TO_METER;
    net->agl = fdm_state.get_Altitude_AGL() * SG_FEET_TO_METER;
    net->phi = fdm_state.get_Phi();
    net->theta = fdm_state.get_Theta();
    net->psi = fdm_state.get_Psi();
    net->alpha = fdm_state.get_Alpha();
    net->beta = fdm_state.get_Beta();
    net->phidot = fdm_state.get_Phi_dot_degps() * SG_DEGREES_TO_RADIANS;
    net->thetadot = fdm_state.get_Theta_dot_degps()
        * SG_DEGREES_TO_RADIANS;
    net->psidot = fdm_state.get_Psi_dot_degps() * SG_DEGREES_TO_RADIANS;

    net->vcas = fdm_state.get_V_calibrated_kts();
    net->climb_rate = fdm_state.get_Climb_Rate();

    net->v_north = fdm_state.get_V_north();
    net->v_east = fdm_state.get_V_east();
    net->v_down = fdm_state.get_V_down();
    net->v_body_u = fdm_state.get_uBody();
    net->v_body_v = fdm_state.get_vBody();
    net->v_body_w = fdm_state.get_wBody();

    net->A_X_pilot = fdm_state.get_A_X_pilot();
    net->A_Y_pilot = fdm_state.get_A_Y_pilot();
    net->A_Z_pilot = fdm_state.get_A_Z_pilot();

    net->stall_warning = fgGetDouble("/sim/alarms/stall-warning", 0.0);
    net->slip_deg
      = fgGetDouble("/instrumentation/slip-skid-ball/indicated-slip-skid");

    // Engine parameters
    net->num_engines = FGNetFDM::FG_MAX_ENGINES;
    for ( i = 0; i < net->num_engines; ++i ) {
        SGPropertyNode *node = fgGetNode("engines/engine", i, true);
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
        SGPropertyNode *node = fgGetNode("/consumables/fuel/tank", i, true);
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
        SGPropertyNode *node = fgGetNode("/gear/gear", i, true);
        net->wow[i] = node->getIntValue("wow");
        net->gear_pos[i] = node->getDoubleValue("position-norm");
        net->gear_steer[i] = node->getDoubleValue("steering-norm");
        net->gear_compression[i] = node->getDoubleValue("compression-norm");
    }

    // the following really aren't used in this context
    net->cur_time = globals->get_time_params()->get_cur_time();
    net->warp = globals->get_warp();
    net->visibility = fgGetDouble("/environment/visibility-m");

    // Control surface positions
    SGPropertyNode *node = fgGetNode("/surface-positions", true);
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
void FGFDM2Props<FGNetFDM>( FGNetFDM *net, bool net_byte_order ) {
    unsigned int i;
    FlightProperties fdm_state;
    
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
                                      
      fdm_state.set_Latitude(net->latitude);
        fdm_state.set_Longitude(net->longitude);
        fdm_state.set_Altitude(net->altitude * SG_METER_TO_FEET);
        
	if ( net->agl > -9000 ) {
	    fdm_state.set_Altitude_AGL( net->agl * SG_METER_TO_FEET );
	} else {
	    double agl_m = net->altitude
              - fdm_state.get_Runway_altitude_m();
	    fdm_state.set_Altitude_AGL( agl_m * SG_METER_TO_FEET );
	}
        fdm_state.set_Euler_Angles( net->phi,
                                          net->theta,
                                          net->psi );
        fdm_state.set_Alpha( net->alpha );
        fdm_state.set_Beta( net->beta );
        fdm_state.set_Euler_Rates( net->phidot,
					 net->thetadot,
					 net->psidot );
        fdm_state.set_V_calibrated_kts( net->vcas );
        fdm_state.set_Climb_Rate( net->climb_rate );
        fdm_state.set_Velocities_Local( net->v_north,
                                              net->v_east,
                                              net->v_down );
        fdm_state.set_Velocities_Body( net->v_body_u,
                                                  net->v_body_v,
                                                  net->v_body_w );

        fdm_state.set_Accels_Pilot_Body( net->A_X_pilot,
					       net->A_Y_pilot,
					       net->A_Z_pilot );

        fgSetDouble( "/sim/alarms/stall-warning", net->stall_warning );
	fgSetDouble( "/instrumentation/slip-skid-ball/indicated-slip-skid",
		     net->slip_deg );
	fgSetBool( "/instrumentation/slip-skid-ball/override", true );

	for ( i = 0; i < net->num_engines; ++i ) {
	    SGPropertyNode *node = fgGetNode( "engines/engine", i, true );
	    
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
		= fgGetNode("/consumables/fuel/tank", i, true);
        node->setDoubleValue("level-gal_us", net->fuel_quantity[i]);
        node->setBoolValue("selected", net->tank_selected[i] > 0);
        node->setDoubleValue("capacity-m3", net->capacity_m3[i]);
        node->setDoubleValue("unusable-m3", net->unusable_m3[i]);
        node->setDoubleValue("density-kgpm3", net->density_kgpm3[i]);
        node->setDoubleValue("level-m3", net->level_m3[i]);
    }

	for (i = 0; i < net->num_wheels; ++i ) {
	    SGPropertyNode * node = fgGetNode("/gear/gear", i, true);
	    node->setDoubleValue("wow", net->wow[i] );
	    node->setDoubleValue("position-norm", net->gear_pos[i] );
	    node->setDoubleValue("steering-norm", net->gear_steer[i] );
	    node->setDoubleValue("compression-norm", net->gear_compression[i] );
	}

	/* these are ignored for now  ... */
	/*
	if ( net->cur_time ) {
	    fgSetLong("/sim/time/cur-time-override", net->cur_time);
	}

        globals->set_warp( net->warp );
        last_warp = net->warp;
	*/

        SGPropertyNode *node = fgGetNode("/surface-positions", true);
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
                "Error: version mismatch in FGNetFDM2Props()" );
	SG_LOG( SG_IO, SG_ALERT,
		"\tread " << net->version << " need " << FG_NET_FDM_VERSION );
	SG_LOG( SG_IO, SG_ALERT,
		"\tNeeds to upgrade net_fdm.hxx and recompile." );
    }
}

#if FG_HAVE_DDS
template<>
void FGProps2FDM<FG_DDS_FDM>( FG_DDS_FDM *dds, bool net_byte_order ) {
    unsigned int i;

    FlightProperties fdm_state;

    // Version sanity checking
    dds->version = FG_DDS_FDM_VERSION;

    // Aero parameters
    dds->longitude = fdm_state.get_Longitude();
    dds->latitude = fdm_state.get_Latitude();
    dds->altitude = fdm_state.get_Altitude() * SG_FEET_TO_METER;
    dds->agl = fdm_state.get_Altitude_AGL() * SG_FEET_TO_METER;
    dds->phi = fdm_state.get_Phi();
    dds->theta = fdm_state.get_Theta();
    dds->psi = fdm_state.get_Psi();
    dds->alpha = fdm_state.get_Alpha();
    dds->beta = fdm_state.get_Beta();
    dds->phidot = fdm_state.get_Phi_dot_degps() * SG_DEGREES_TO_RADIANS;
    dds->thetadot = fdm_state.get_Theta_dot_degps() * SG_DEGREES_TO_RADIANS;
    dds->psidot = fdm_state.get_Psi_dot_degps() * SG_DEGREES_TO_RADIANS;

    dds->vcas = fdm_state.get_V_calibrated_kts();
    dds->climb_rate = fdm_state.get_Climb_Rate();

    dds->v_north = fdm_state.get_V_north();
    dds->v_east = fdm_state.get_V_east();
    dds->v_down = fdm_state.get_V_down();
    dds->v_body_u = fdm_state.get_uBody();
    dds->v_body_v = fdm_state.get_vBody();
    dds->v_body_w = fdm_state.get_wBody();

    dds->A_X_pilot = fdm_state.get_A_X_pilot();
    dds->A_Y_pilot = fdm_state.get_A_Y_pilot();
    dds->A_Z_pilot = fdm_state.get_A_Z_pilot();

    dds->stall_warning = fgGetDouble("/sim/alarms/stall-warning", 0.0);
    dds->slip_deg
      = fgGetDouble("/instrumentation/slip-skid-ball/indicated-slip-skid");

    // Engine parameters
    dds->num_engines = FGNetFDM::FG_MAX_ENGINES;
    for ( i = 0; i < dds->num_engines; ++i ) {
        SGPropertyNode *node = fgGetNode("engines/engine", i, true);
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
        SGPropertyNode *node = fgGetNode("/consumables/fuel/tank", i, true);
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
        SGPropertyNode *node = fgGetNode("/gear/gear", i, true);
        dds->wow[i] = node->getIntValue("wow");
        dds->gear_pos[i] = node->getDoubleValue("position-norm");
        dds->gear_steer[i] = node->getDoubleValue("steering-norm");
        dds->gear_compression[i] = node->getDoubleValue("compression-norm");
    }

    // the following really aren't used in this context
    dds->cur_time = globals->get_time_params()->get_cur_time();
    dds->warp = globals->get_warp();
    dds->visibility = fgGetDouble("/environment/visibility-m");

    // Control surface positions
    SGPropertyNode *node = fgGetNode("/surface-positions", true);
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
void FGFDM2Props<FG_DDS_FDM>( FG_DDS_FDM *dds, bool net_byte_order ) {
    unsigned int i;
    FlightProperties fdm_state;

    if ( dds->version == FG_DDS_FDM_VERSION ) {
        // cout << "pos = " << dds->longitude << " " << dds->latitude << endl;
        // cout << "sea level rad = " << fdm_state.get_Sea_level_radius()
        //      << endl;

        fdm_state.set_Latitude(dds->latitude);
        fdm_state.set_Longitude(dds->longitude);
        fdm_state.set_Altitude(dds->altitude * SG_METER_TO_FEET);

        if ( dds->agl > -9000 ) {
            fdm_state.set_Altitude_AGL( dds->agl * SG_METER_TO_FEET );
        } else {
            double agl_m = dds->altitude
              - fdm_state.get_Runway_altitude_m();
            fdm_state.set_Altitude_AGL( agl_m * SG_METER_TO_FEET );
        }
        fdm_state.set_Euler_Angles( dds->phi,
                                          dds->theta,
                                          dds->psi );
        fdm_state.set_Alpha( dds->alpha );
        fdm_state.set_Beta( dds->beta );
        fdm_state.set_Euler_Rates( dds->phidot,
                                         dds->thetadot,
                                         dds->psidot );
        fdm_state.set_V_calibrated_kts( dds->vcas );
        fdm_state.set_Climb_Rate( dds->climb_rate );
        fdm_state.set_Velocities_Local( dds->v_north,
                                              dds->v_east,
                                              dds->v_down );
        fdm_state.set_Velocities_Body( dds->v_body_u,
                                                  dds->v_body_v,
                                                  dds->v_body_w );

        fdm_state.set_Accels_Pilot_Body( dds->A_X_pilot,
                                               dds->A_Y_pilot,
                                               dds->A_Z_pilot );

        fgSetDouble( "/sim/alarms/stall-warning", dds->stall_warning );
        fgSetDouble( "/instrumentation/slip-skid-ball/indicated-slip-skid",
                     dds->slip_deg );
        fgSetBool( "/instrumentation/slip-skid-ball/override", true );

        for ( i = 0; i < dds->num_engines; ++i ) {
            SGPropertyNode *node = fgGetNode( "engines/engine", i, true );

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
                = fgGetNode("/consumables/fuel/tank", i, true);
            node->setDoubleValue("level-gal_us", dds->fuel_quantity[i]);
            node->setBoolValue("selected", dds->tank_selected[i] > 0);
            node->setDoubleValue("capacity-m3", dds->capacity_m3[i]);
            node->setDoubleValue("unusable-m3", dds->unusable_m3[i]);
            node->setDoubleValue("density-kgpm3", dds->density_kgpm3[i]);
            node->setDoubleValue("level-m3", dds->level_m3[i]);
        }

        for (i = 0; i < dds->num_wheels; ++i ) {
            SGPropertyNode * node = fgGetNode("/gear/gear", i, true);
            node->setDoubleValue("wow", dds->wow[i] );
            node->setDoubleValue("position-norm", dds->gear_pos[i] );
            node->setDoubleValue("steering-norm", dds->gear_steer[i] );
            node->setDoubleValue("compression-norm", dds->gear_compression[i] );
        }

        /* these are ignored for now  ... */
        /*
        if ( dds->cur_time ) {
            fgSetLong("/sim/time/cur-time-override", dds->cur_time);
        }

        globals->set_warp( dds->warp );
        last_warp = dds->warp;
        */
        SGPropertyNode *node = fgGetNode("/surface-positions", true);
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
                "Error: version mismatch in FGNetFDM2Props()" );
        SG_LOG( SG_IO, SG_ALERT,
                "\tread " << dds->version << " need " << FG_DDS_FDM_VERSION );
        SG_LOG( SG_IO, SG_ALERT,
                "\tNeeds to upgrade DDS/dds_fdm.hxx and recompile." );
    }
}
#endif

// open hailing frequencies
bool FGNativeFDM::open() {
    if ( is_enabled() ) {
        SG_LOG( SG_IO, SG_ALERT, "This shouldn't happen, but the channel "
                << "is already in use, ignoring" );
        return false;
    }

    SGIOChannel *io = get_io_channel();

#if FG_HAVE_DDS
    if ( io->get_type() == sgDDSType ) {
        SG_DDS *dds = static_cast<SG_DDS*>(io);
        dds->setup("FG_DDS_FDM" , &FG_DDS_FDM_desc, sizeof (FG_DDS_FDM));
    }
#endif

    if ( ! io->open( get_direction() ) ) {
        SG_LOG( SG_IO, SG_ALERT, "Error opening channel communication layer." );
        return false;
    }

    set_enabled( true );

    // Is this really needed here ????
    fgSetDouble("/position/sea-level-radius-ft", SG_EQUATORIAL_RADIUS_FT);

    return true;
}

// process work for this port
bool FGNativeFDM::process() {
    SGIOChannel *io = get_io_channel();
    int length;
    char *buf;

    if ( io->get_type() == sgDDSType ) {
        buf = reinterpret_cast<char*>(&fdm.dds);
        length = sizeof(FG_DDS_FDM);
    } else {
        buf = reinterpret_cast<char*>(&fdm.net);
        length = sizeof(FGNetFDM);
    }

    if ( get_direction() == SG_IO_OUT ) {

        if ( io->get_type() == sgDDSType ) {
            FGProps2FDM( &fdm.dds );
        } else {
            FGProps2FDM( &fdm.net );
        }

        if ( ! io->write( buf, length ) ) {
            SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
            return false;
        }
    } else if ( get_direction() == SG_IO_IN ) {
        if ( io->get_type() == sgFileType ) {
            if ( io->read( buf, length ) == length ) {
                SG_LOG( SG_IO, SG_INFO, "Success reading data." );
                if ( io->get_type() == sgDDSType ) {
                    FGFDM2Props( &fdm.dds );
                } else {
                    FGFDM2Props( &fdm.net );
                }
            }
        } else {
            while ( io->read( buf, length ) == length ) {
                SG_LOG( SG_IO, SG_INFO, "  Success reading data." );
                if ( io->get_type() == sgDDSType ) {
                    FGFDM2Props( &fdm.dds );
                } else {
                    FGFDM2Props( &fdm.net );
                }
            }
        }
    }

    return true;
}

// close the channel
bool FGNativeFDM::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
        return false;
    }

    return true;
}
