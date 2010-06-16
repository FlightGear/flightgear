// opengc.cxx - Network interface program to send data to display processor over LAN
//
// Created by: 	J. Wojnaroski  -- castle@mminternet.com
// Date:		21 Nov 2001 
//
// Extended from original network code developed by C. Olson
//
//  Modified 12/02/01 - Update engine structure for multi-engine models
//		          - Added data preamble to id msg types
//
//  Modified 01/23/02 - Converted portions of the Engine and Gear accesssors to properties
//			    - Removed data from navigation functions. OpenGC provides own nav functions
//
//  Modified 03/05/03 - Modified to reduce need to search property tree for each node per frame
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>

#include <vector>

#include "opengc.hxx"
#include <FDM/flightProperties.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

using std::vector;

FGOpenGC::FGOpenGC() : 
	press_node(fgGetNode("/environment/pressure-inhg[0]", true)),
	temp_node(fgGetNode("/environment/temperature-degc[0]", true)),
	wind_dir_node(fgGetNode("/environment/wind-from-heading-deg[0]", true)),
	wind_speed_node(fgGetNode("/environment/wind-speed-kt[0]", true)),
	magvar_node(fgGetNode("/environment/magnetic-variation-deg[0]", true)),
	p_latitude(fgGetNode("/position/latitude-deg", true)),	
	p_longitude(fgGetNode("/position/longitude-deg", true)),
	p_elev_node(fgGetNode("/position/altitude-ft", true)),
	p_altitude_agl(fgGetNode("/position/altitude-agl-ft", true)),
	p_pitch(fgGetNode("/orientation/pitch-deg[0]", true)),
	p_bank(fgGetNode("/orientation/roll-deg[0]", true)),
	p_heading(fgGetNode("/orientation/heading-magnetic-deg[0]", true)),
	p_yaw(fgGetNode("/fdm/jsbsim/aero/beta-rad[0]", true)),
	p_yaw_rate(fgGetNode("/fdm/jsbsim/aero/betadot-rad_sec[0]", true)),
	vel_kcas(fgGetNode("/velocities/airspeed-kt[0]", true)),
	p_vvi(fgGetNode("/velocities/vertical-speed-fps[0]", true )),
	p_mach(fgGetNode("/velocities/mach[0]", true )),
	p_left_aileron(fgGetNode("surface-positions/left-aileron-pos-norm", true)),
	p_right_aileron(fgGetNode("surface-positions/right-aileron-pos-norm", true)),
	p_elevator(fgGetNode("surface-positions/elevator-pos-norm", true)),
	p_elevator_trim(fgGetNode("surface-positions/elevator_trim-pos-norm", true)),
	p_rudder(fgGetNode("surface-positions/rudder-pos-norm", true)),
	p_flaps(fgGetNode("surface-positions/flap-pos-norm", true)),
	p_flaps_cmd(fgGetNode("/controls/flight/flaps", true)),
	p_park_brake(fgGetNode("/controls/gear/brake-parking", true)),	
	egt0_node(fgGetNode("/engines/engine/EGT_degC[0]", true)),
	egt1_node(fgGetNode("/engines/engine[1]/EGT_degC[0]", true)),
	egt2_node(fgGetNode("/engines/engine[2]/EGT_degC[0]", true)),
	egt3_node(fgGetNode("/engines/engine[3]/EGT_degC[0]", true)),
	epr0_node(fgGetNode("/engines/engine/EPR[0]", true)),
	epr1_node(fgGetNode("/engines/engine[1]/EPR[0]", true)),
	epr2_node(fgGetNode("/engines/engine[2]/EPR[0]", true)),
	epr3_node(fgGetNode("/engines/engine[3]/EPR[0]", true)),
	n10_node(fgGetNode("/engines/engine/N1[0]", true)),
	n11_node(fgGetNode("/engines/engine[1]/N1[0]", true)),
	n12_node(fgGetNode("/engines/engine[2]/N1[0]", true)),
	n13_node(fgGetNode("/engines/engine[3]/N1[0]", true)),
	n20_node(fgGetNode("/engines/engine/N2[0]", true)),
	n21_node(fgGetNode("/engines/engine[1]/N2[0]", true)),
	n22_node(fgGetNode("/engines/engine[2]/N2[0]", true)),
	n23_node(fgGetNode("/engines/engine[3]/N2[0]", true)),
	oil_temp0(fgGetNode("engines/engine/oil-temp-degF[0]", true)),
	oil_temp1(fgGetNode("engines/engine[1]/oil-temp-degF[0]", true)),
	oil_temp2(fgGetNode("engines/engine[2]/oil-temp-degF[0]", true)),
	oil_temp3(fgGetNode("engines/engine[3]/oil-temp-degF[0]", true)),
	tank0_node(fgGetNode("/consumables/fuel/tank/level-lb[0]", true)),
	tank1_node(fgGetNode("/consumables/fuel/tank[1]/level-lb[0]", true)),
	tank2_node(fgGetNode("/consumables/fuel/tank[2]/level-lb[0]", true)),
	tank3_node(fgGetNode("/consumables/fuel/tank[3]/level-lb[0]", true)),
	tank4_node(fgGetNode("/consumables/fuel/tank[4]/level-lb[0]", true)),
	tank5_node(fgGetNode("/consumables/fuel/tank[5]/level-lb[0]", true)),
	tank6_node(fgGetNode("/consumables/fuel/tank[6]/level-lb[0]", true)),
	tank7_node(fgGetNode("/consumables/fuel/tank[7]/level-lb[0]", true)),
	p_alphadot(fgGetNode("/fdm/jsbsim/aero/alphadot-rad_sec[0]", true)),
	p_betadot(fgGetNode("/fdm/jsbsim/aero/betadot-rad_sec[0]", true))
{
  fdm = new FlightProperties;
}

FGOpenGC::~FGOpenGC() {
  delete fdm;
}

// open hailing frequencies
bool FGOpenGC::open() {
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

//static void collect_data( const FGInterface *fdm, ogcFGData *data ) {
void FGOpenGC::collect_data(ogcFGData *data ) {	
											
    data->version_id = OGC_VERSION;
    
    data->longitude = p_longitude->getDoubleValue();   
    data->latitude = p_latitude->getDoubleValue();
    data->elevation = p_elev_node->getDoubleValue();
    data->magvar = magvar_node->getDoubleValue();
   
    data->pitch = p_pitch->getDoubleValue();
    data->bank = p_bank->getDoubleValue();
    data->heading = p_heading->getDoubleValue();
    //data->altitude = p_altitude->getDoubleValue();
    data->altitude_agl = p_altitude_agl->getDoubleValue();
   
    data->vvi = p_vvi->getDoubleValue();
    data->mach = p_mach->getDoubleValue();
    data->groundspeed = fdm->get_V_ground_speed();
    data->v_keas = fdm->get_V_equiv_kts();
    data->v_kcas = vel_kcas->getDoubleValue();
 

    data->phi_dot = fdm->get_Phi_dot();
    data->theta_dot = fdm->get_Theta_dot();
    data->psi_dot = fdm->get_Psi_dot();

    data->alpha = fdm->get_Alpha();
    data->beta = p_yaw->getDoubleValue();
    data->alpha_dot = p_alphadot->getDoubleValue();
    data->beta_dot = p_yaw_rate->getDoubleValue();
      
  //data->rudder_trim = p_Controls->get_rudder_trim();
    data->parking_brake = p_park_brake->getDoubleValue();
    
    data->left_aileron = p_left_aileron->getDoubleValue();
    data->right_aileron = p_right_aileron->getDoubleValue();
    data->elevator = p_elevator->getDoubleValue();
    data->elevator_trim = p_elevator_trim->getDoubleValue();
    data->rudder = p_rudder->getDoubleValue();
    data->flaps = p_flaps->getDoubleValue();
    data->flaps_cmd = p_flaps_cmd->getDoubleValue();

    data->gear_nose = fgGetDouble("gear/gear[0]/position-norm[0]");
    data->gear_left = fgGetDouble("gear/gear[1]/position-norm[0]");
    data->gear_right = fgGetDouble("gear/gear[2]/position-norm[0]");
	  data->gear_left_rear = fgGetDouble("gear/gear[3]/position-norm[0]");
	  data->gear_right_rear = fgGetDouble("gear/gear[4]/position-norm[0]");
	  data->parking_brake = p_park_brake->getDoubleValue();
	  data->wow_main = fgGetBool("gear/gear[1]/wow[0]") && fgGetBool("gear/gear[2]/wow[0]");
	  data->wow_nose = fgGetBool("gear/gear[0]/wow[0]");
	  //cout << "Nose " << data->wow_nose << " Main " << wow_main << endl;
	  
    //data->rpm[0] = fgGetDouble("/engines/engine[0]/rpm");
    //data->rpm[1] = fgGetDouble("/engines/engine[1]/rpm");   

    data->epr[0] = epr0_node->getDoubleValue();
    data->epr[1] = epr1_node->getDoubleValue();
    data->epr[2] = epr2_node->getDoubleValue();
    data->epr[3] = epr3_node->getDoubleValue();
    
    data->egt[0] = egt0_node->getDoubleValue();
    data->egt[1] = egt1_node->getDoubleValue();
    data->egt[2] = egt2_node->getDoubleValue();
    data->egt[3] = egt3_node->getDoubleValue();

    data->n2_turbine[0] = n20_node->getDoubleValue();
    data->n2_turbine[1] = n21_node->getDoubleValue();
    data->n2_turbine[2] = n22_node->getDoubleValue();
    data->n2_turbine[3] = n23_node->getDoubleValue();

    data->n1_turbine[0] = n10_node->getDoubleValue();
    data->n1_turbine[1] = n11_node->getDoubleValue();
    data->n1_turbine[2] = n12_node->getDoubleValue();
    data->n1_turbine[3] = n13_node->getDoubleValue();
    
// This is really lbs/hr (pph), converted in FGSimTurbine
    data->fuel_flow[0] = fgGetDouble("/engines/engine[0]/fuel-flow-gph");
    data->fuel_flow[1] = fgGetDouble("/engines/engine[1]/fuel-flow-gph");
    data->fuel_flow[2] = fgGetDouble("/engines/engine[2]/fuel-flow-gph");
    data->fuel_flow[3] = fgGetDouble("/engines/engine[3]/fuel-flow-gph");

    data->oil_pressure[0] = fgGetDouble("/engines/engine[0]/oil-pressure-psi");
    data->oil_pressure[1] = fgGetDouble("/engines/engine[1]/oil-pressure-psi");
    data->oil_pressure[2] = fgGetDouble("/engines/engine[2]/oil-pressure-psi");
    data->oil_pressure[3] = fgGetDouble("/engines/engine[3]/oil-pressure-psi");
    
    //data->oil_temp[0] = fgGetDouble("/engines/engine[0]/oil-temperature-degf");
    //data->oil_temp[1] = fgGetDouble("/engines/engine[1]/oil-temperature-degf");
    //data->oil_temp[2] = fgGetDouble("/engines/engine[2]/oil-temperature-degf");
    //data->oil_temp[3] = fgGetDouble("/engines/engine[3]/oil-temperature-degf");
    
    data->oil_temp[0] = oil_temp0->getDoubleValue();
    data->oil_temp[1] = oil_temp1->getDoubleValue();
    data->oil_temp[2] = oil_temp2->getDoubleValue();
    data->oil_temp[3] = oil_temp3->getDoubleValue();    
    
		data->hyd_pressure[0] = fgGetDouble("/engines/engine[0]/hyd-pressure-psi");
		data->hyd_pressure[1] = fgGetDouble("/engines/engine[1]/hyd-pressure-psi");
		data->hyd_pressure[2] = fgGetDouble("/engines/engine[2]/hyd-pressure-psi");
		data->hyd_pressure[3] = fgGetDouble("/engines/engine[3]/hyd-pressure-psi");
		// just a temp thing until htdraulic model built
//		data->hyd_pressure[0] = 2750.0;
//		data->hyd_pressure[1] = 2755.0;
//		data->hyd_pressure[2] = 2747.0;
//		data->hyd_pressure[3] = 2851.0;


    //data->man_pressure[0] = fgGetDouble("/engines/engine[0]/mp-osi");
    //data->man_pressure[1] = fgGetDouble("/engines/engine[1]/mp-osi"); 
    
    data->fuel_tank[0] = tank0_node->getDoubleValue() * 0.001;
    data->fuel_tank[1] = tank1_node->getDoubleValue() * 0.001;
    data->fuel_tank[2] = tank2_node->getDoubleValue() * 0.001;
    data->fuel_tank[3] = tank3_node->getDoubleValue() * 0.001;
    data->fuel_tank[4] = tank4_node->getDoubleValue() * 0.001;
    data->fuel_tank[5] = tank5_node->getDoubleValue() * 0.001; // 747 stab tank
    data->fuel_tank[6] = tank6_node->getDoubleValue() * 0.001; // 747 res2 tank
    data->fuel_tank[7] = tank7_node->getDoubleValue() * 0.001; // 747 res3 tank
    double total_fuel = 0.0;
    for ( int t= 0; t<8; t++ ) total_fuel += data->fuel_tank[t];
    data->fuel_tank[8] = total_fuel;
    //data->fuel_tank[8] = 386000.0; // total fuel load
		//cout << endl << "Send " << data->fuel_tank[0] << endl;
/*********		
		data->boost_pumps[0] = boost1_node->getBoolValue();
		data->boost_pumps[1] = boost2_node->getBoolValue();
		data->boost_pumps[2] = boost3_node->getBoolValue();
		data->boost_pumps[3] = boost4_node->getBoolValue();
		data->boost_pumps[4] = boost5_node->getBoolValue();
		data->boost_pumps[5] = boost6_node->getBoolValue();
		data->boost_pumps[6] = boost7_node->getBoolValue();
		data->boost_pumps[7] = boost8_node->getBoolValue();
		
		data->over_ride_pumps[0] = ovride0_node->getBoolValue();
		data->over_ride_pumps[1] = ovride1_node->getBoolValue();
		data->over_ride_pumps[2] = ovride2_node->getBoolValue();
		data->over_ride_pumps[3] = ovride3_node->getBoolValue();
		data->over_ride_pumps[4] = ovride4_node->getBoolValue();
		data->over_ride_pumps[5] = ovride5_node->getBoolValue();	
		
		data->x_feed_valve[0] = x_feed0_node->getBoolValue(); 
		data->x_feed_valve[1] = x_feed1_node->getBoolValue(); 		
		data->x_feed_valve[2] = x_feed2_node->getBoolValue(); 
		data->x_feed_valve[3] = x_feed3_node->getBoolValue(); 
		**********/			
    data->total_temperature = fdm->get_Total_temperature();
    data->total_pressure = fdm->get_Total_pressure();
    data->dynamic_pressure = fdm->get_Dynamic_pressure();
    
    data->static_pressure = press_node->getDoubleValue();
    data->static_temperature = temp_node->getDoubleValue();
    data->wind = wind_speed_node->getDoubleValue();
    data->wind_dir =  wind_dir_node->getDoubleValue();
    data->sea_level_pressure = fgGetDouble("/environment/sea-level-pressure-inhg");
}

static void distribute_data( const ogcFGData *data ) {
    // just a place holder until the CDU is developed
	
}

// process work for this port
bool FGOpenGC::process() {
    SGIOChannel *io = get_io_channel();
    int length = sizeof(buf);

    if ( get_direction() == SG_IO_OUT ) {
        collect_data( &buf );
	//collect_data( &buf );
	if ( ! io->write( (char *)(& buf), length ) ) {
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		distribute_data( &buf );
	    }
	} else {
	    while ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		distribute_data( &buf );
	    }
	}
    }

    return true;
}


// close the channel
bool FGOpenGC::close() {
    SGIOChannel *io = get_io_channel();

    set_enabled( false );

    if ( ! io->close() ) {
	return false;
    }

    return true;
}

