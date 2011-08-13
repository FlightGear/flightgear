// native_ctrls.cxx -- FGFS "Native" controls I/O class
//
// Written by Curtis Olson, started July 2001.
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
#include <simgear/io/iochannel.hxx>
#include <simgear/io/lowlevel.hxx> // endian tests

#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>	// ground elevation

#include "native_ctrls.hxx"

// FreeBSD works better with this included last ... (?)
#if defined( _MSC_VER )
#  include <windows.h>
#elif defined( __MINGW32__ )
#  include <winsock2.h>
#else
#  include <netinet/in.h>	// htonl() ntohl()
#endif


FGNativeCtrls::FGNativeCtrls() {
}

FGNativeCtrls::~FGNativeCtrls() {
}


// open hailing frequencies
bool FGNativeCtrls::open() {
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

    return true;
}


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


// Populate the FGNetCtrls structure from the property tree.
void FGProps2NetCtrls( FGNetCtrls *net, bool honor_freezes,
                       bool net_byte_order )
{
    int i;
    SGPropertyNode *node;
    SGPropertyNode *fuelpump;
    SGPropertyNode *tempnode;

    // fill in values
    node  = fgGetNode("/controls/flight", true);
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
        = fgGetDouble( "/systems/electrical/outputs/flaps", 1.0 ) >= 1.0;
    net->flap_motor_ok = node->getBoolValue( "flaps-serviceable" );

    net->num_engines = FGNetCtrls::FG_MAX_ENGINES;
    for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {
        // Controls
        node = fgGetNode("/controls/engines/engine", i );
        fuelpump = fgGetNode("/systems/electrical/outputs/fuel-pump", i );

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
        node = fgGetNode("/controls/fuel/tank", i);
        if ( node->getChild("fuel_selector") != 0 ) {
            net->fuel_selector[i]
                = node->getChild("fuel_selector")->getBoolValue();
        } else {
            net->fuel_selector[i] = false;
        }
    }
    node = fgGetNode("/controls/gear", true);
    net->brake_left = node->getChild("brake-left")->getDoubleValue();
    net->brake_right = node->getChild("brake-right")->getDoubleValue();
    net->copilot_brake_left
        = node->getChild("copilot-brake-left")->getDoubleValue();
    net->copilot_brake_right
        = node->getChild("copilot-brake-right")->getDoubleValue();
    net->brake_parking = node->getChild("brake-parking")->getDoubleValue();

    net->gear_handle = fgGetBool( "/controls/gear/gear-down" );

    net->master_avionics = fgGetBool("/controls/switches/master-avionics");

    net->wind_speed_kt = fgGetDouble("/environment/wind-speed-kt");
    net->wind_dir_deg = fgGetDouble("/environment/wind-from-heading-deg");
    net->turbulence_norm =
        fgGetDouble("/environment/turbulence/magnitude-norm");

    net->temp_c = fgGetDouble("/environment/temperature-degc");
    net->press_inhg = fgGetDouble("/environment/pressure-sea-level-inhg");

    net->hground = fgGetDouble("/position/ground-elev-m");
    net->magvar = fgGetDouble("/environment/magnetic-variation-deg");

    net->icing = fgGetBool("/hazards/icing/wing");

    net->speedup = fgGetInt("/sim/speed-up");
    net->freeze = 0;
    if ( honor_freezes ) {
        if ( fgGetBool("/sim/freeze/master") ) {
            net->freeze |= 0x01;
        }
        if ( fgGetBool("/sim/freeze/position") ) {
            net->freeze |= 0x02;
        }
        if ( fgGetBool("/sim/freeze/fuel") ) {
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
void FGNetCtrls2Props( FGNetCtrls *net, bool honor_freezes,
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
    node = fgGetNode("/controls/flight", true);
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
    fgSetBool( "/systems/electrical/outputs/flaps", net->flaps_power );
    node->setBoolValue( "flaps-serviceable", net->flap_motor_ok );

    for ( i = 0; i < FGNetCtrls::FG_MAX_ENGINES; ++i ) {
        // Controls
        node = fgGetNode("/controls/engines/engine", i);
        node->getChild( "throttle" )->setDoubleValue( net->throttle[i] );
        node->getChild( "mixture" )->setDoubleValue( net->mixture[i] );
        node->getChild( "propeller-pitch" )
            ->setDoubleValue( net->prop_advance[i] );
        node->getChild( "condition" )
            ->setDoubleValue( net->condition[i] );
        node->getChild( "magnetos" )->setDoubleValue( net->magnetos[i] );
        node->getChild( "starter" )->setDoubleValue( net->starter_power[i] );
        node->getChild( "feed_tank" )->setIntValue( net->feed_tank_to[i] );
        node->getChild( "reverser" )->setBoolValue( net->reverse[i] );
	// Faults
	SGPropertyNode *faults = node->getNode( "faults", true );
	faults->setBoolValue( "serviceable", net->engine_ok[i] );
	faults->setBoolValue( "left-magneto-serviceable",
			      net->mag_left_ok[i] );
	faults->setBoolValue( "right-magneto-serviceable",
			      net->mag_right_ok[i]);
	faults->setBoolValue( "spark-plugs-serviceable",
			      net->spark_plugs_ok[i] );
	faults->setIntValue( "oil-pressure-status", net->oil_press_status[i] );
	faults->setBoolValue( "fuel-pump-serviceable", net->fuel_pump_ok[i] );
    }

    fgSetBool( "/systems/electrical/outputs/fuel-pump",
               net->fuel_pump_power[0] );

    for ( i = 0; i < FGNetCtrls::FG_MAX_TANKS; ++i ) {
        node = fgGetNode( "/controls/fuel/tank", i );
        node->getChild( "fuel_selector" )
            ->setBoolValue( net->fuel_selector[i] );
//        node->getChild( "to_tank" )->xfer_tank( i, net->xfer_to[i] );
    }
    node = fgGetNode( "/controls/gear" );
    if ( node != NULL ) {
        node->getChild( "brake-left" )->setDoubleValue( net->brake_left );
        node->getChild( "brake-right" )->setDoubleValue( net->brake_right );
        node->getChild( "copilot-brake-left" )
            ->setDoubleValue( net->copilot_brake_left );
        node->getChild( "copilot-brake-right" )
            ->setDoubleValue( net->copilot_brake_right );
        node->getChild( "brake-parking" )->setDoubleValue( net->brake_parking );
    }

    node = fgGetNode( "/controls/gear", true );
    node->setBoolValue( "gear-down", net->gear_handle );
//    node->setDoubleValue( "brake-parking", net->brake_parking );
//    node->setDoubleValue( net->brake_left );
//    node->setDoubleValue( net->brake_right );

    node = fgGetNode( "/controls/switches", true );
    node->setBoolValue( "master-bat", net->master_bat );
    node->setBoolValue( "master-alt", net->master_alt );
    node->setBoolValue( "master-avionics", net->master_avionics );
    
    node = fgGetNode( "/environment", true );
    node->setDoubleValue( "wind-speed-kt", net->wind_speed_kt );
    node->setDoubleValue( "wind-from-heading-deg", net->wind_dir_deg );
    node->setDoubleValue( "turbulence/magnitude-norm", net->turbulence_norm );
    node->setDoubleValue( "magnetic-variation-deg", net->magvar );

    node->setDoubleValue( "/environment/temperature-degc",
			  net->temp_c );
    node->setDoubleValue( "/environment/pressure-sea-level-inhg",
			  net->press_inhg );

    // ground elevation ???

    fgSetDouble("/hazards/icing/wing", net->icing);
    
    node = fgGetNode( "/radios", true );
    node->setDoubleValue( "comm/frequencies/selected-mhz[0]", net->comm_1 );
    node->setDoubleValue( "nav/frequencies/selected-mhz[0]", net->nav_1 );
    node->setDoubleValue( "nav[1]/frequencies/selected-mhz[0]", net->nav_2 );

    fgSetInt( "/sim/speed-up", net->speedup );

    if ( honor_freezes ) {
        node = fgGetNode( "/sim/freeze", true );
        node->setBoolValue( "master", net->freeze & 0x01 );
        node->setBoolValue( "position", net->freeze & 0x02 );
        node->setBoolValue( "fuel", net->freeze & 0x04 );
    }
}


// process work for this port
bool FGNativeCtrls::process() {
    SGIOChannel *io = get_io_channel();
    int length = sizeof(FGNetCtrls);

    if ( get_direction() == SG_IO_OUT ) {

	FGProps2NetCtrls( &net_ctrls, true, true );

	if ( ! io->write( (char *)(& net_ctrls), length ) ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& net_ctrls), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		FGNetCtrls2Props( &net_ctrls, true, true );
	    }
	} else {
	    while ( io->read( (char *)(& net_ctrls), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		FGNetCtrls2Props( &net_ctrls, true, true );
	    }
	}
    }

    return true;
}


// close the channel
bool FGNativeCtrls::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}
