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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iochannel.hxx>

#include <vector>

#include "opengc.hxx"
#include <FDM/flight.hxx>
#include <Main/globals.hxx>
#include <Controls/controls.hxx>
#include <Main/fg_props.hxx>

SG_USING_STD(vector);

FGOpenGC::FGOpenGC() : 
	press_node(fgGetNode("/environment/pressure-inhg", true)),
	temp_node(fgGetNode("/environment/temperature-degc", true)),
	wind_dir_node(fgGetNode("/environment/wind-from-heading-deg", true)),
	wind_speed_node(fgGetNode("/environment/wind-speed-kt", true)),
	mag_var_node(fgGetNode("/environment/magnetic-variation-deg", true)),
	p_latitude(fgGetNode("/position/latitude-deg", true)),
	p_longitude(fgGetNode("/position/longitude-deg", true)),
	p_alt_node(fgGetNode("/position/altitude-ft", true)),
	p_altitude(fgGetNode("/steam/altitude-ft", true)),
	p_altitude_agl(fgGetNode("/position/altitude-agl-ft", true)),
	egt0_node(fgGetNode("/engines/engine/egt-degf", true)),
	egt1_node(fgGetNode("/engines/engine[1]/egt-degf", true)),
	egt2_node(fgGetNode("/engines/engine[2]/egt-degf", true)),
	egt3_node(fgGetNode("/engines/engine[3]/egt-degf", true)),
	p_left_aileron(fgGetNode("surface-positions/left-aileron-pos-norm", true)),
	p_right_aileron(fgGetNode("surface-positions/right-aileron-pos-norm", true)),
	p_elevator(fgGetNode("surface-positions/elevator-pos-norm", true)),
	p_elevator_trim(fgGetNode("surface-positions/elevator_trim-pos-norm", true)),
	p_rudder(fgGetNode("surface-positions/rudder-pos-norm", true)),
	p_flaps(fgGetNode("surface-positions/flap-pos-norm", true)),
	p_flaps_cmd(fgGetNode("/controls/flaps", true)),
	p_alphadot(fgGetNode("/fdm/jsbsim/aero/alphadot-radsec", true)),
	p_betadot(fgGetNode("/fdm/jsbsim/aero/betadot-radsec", true))
	
{
}

FGOpenGC::~FGOpenGC() {
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
void FGOpenGC::collect_data( const FGInterface *fdm, ogcFGData *data ) {	
											
    data->version_id = OGC_VERSION;

    data->longitude = p_longitude->getDoubleValue();   
    data->latitude = p_latitude->getDoubleValue();
    data->elevation = p_alt_node->getDoubleValue();
    data->magvar = mag_var_node->getDoubleValue();
    
    //cout << "ID: " << OGC_VERSION  << " Lon: " << data->longitude << " Lat: " << data->latitude << endl;
   
    data->pitch = cur_fdm_state->get_Theta_deg();
    data->bank = cur_fdm_state->get_Phi_deg();
    data->heading = cur_fdm_state->get_Psi_deg();
    data->altitude = p_altitude->getDoubleValue();
    data->altitude_agl = p_altitude_agl->getDoubleValue();
    data->v_kcas = cur_fdm_state->get_V_calibrated_kts();
    data->vvi = cur_fdm_state->get_Climb_Rate();
    data->mach = cur_fdm_state->get_Mach_number();
    data->groundspeed = cur_fdm_state->get_V_ground_speed();
    data->v_keas = cur_fdm_state->get_V_equiv_kts();

    data->phi_dot = cur_fdm_state->get_Phi_dot();
    data->theta_dot = cur_fdm_state->get_Theta_dot();
    data->psi_dot = cur_fdm_state->get_Psi_dot();

    data->alpha = cur_fdm_state->get_Alpha();
    data->beta = cur_fdm_state->get_Beta();
    data->alpha_dot = p_alphadot->getDoubleValue();
    data->beta_dot = p_betadot->getDoubleValue();
    
    //data->rudder_trim = p_Controls->get_rudder_trim();
    
    data->left_aileron = p_left_aileron->getDoubleValue();
    data->right_aileron = p_right_aileron->getDoubleValue();
    data->elevator = p_elevator->getDoubleValue();
    data->elevator_trim = p_elevator_trim->getDoubleValue();
    data->rudder = p_rudder->getDoubleValue();
    data->flaps = p_flaps->getDoubleValue();
    data->flaps_cmd = p_flaps_cmd->getDoubleValue();

    data->gear_nose = fgGetDouble("gear/gear[0]/position-norm");
    data->gear_left = fgGetDouble("gear/gear[1]/position-norm");
    data->gear_right = fgGetDouble("gear/gear[2]/position-norm");
	  data->gear_left_rear = fgGetDouble("gear/gear[3]/position-norm");
	  data->gear_right_rear = fgGetDouble("gear/gear[4]/position-norm");
	  
    data->rpm[0] = fgGetDouble("/engines/engine[0]/rpm");
    data->rpm[1] = fgGetDouble("/engines/engine[1]/rpm");   

    data->epr[0] = fgGetDouble("/engines/engine[0]/epr");
    data->epr[1] = fgGetDouble("/engines/engine[1]/epr");
    data->epr[2] = fgGetDouble("/engines/engine[2]/epr");
    data->epr[3] = fgGetDouble("/engines/engine[3]/epr");
    
    data->egt[0] = (egt0_node->getDoubleValue() - 32.0) * 0.555;
    data->egt[1] = (egt1_node->getDoubleValue() - 32.0) * 0.555;
    data->egt[2] = (egt2_node->getDoubleValue() - 32.0) * 0.555;
    data->egt[3] = (egt3_node->getDoubleValue() - 32.0) * 0.555;

    data->n2_turbine[0] = fgGetDouble("/engines/engine[0]/n2");
    data->n2_turbine[1] = fgGetDouble("/engines/engine[1]/n2");
    data->n2_turbine[2] = fgGetDouble("/engines/engine[2]/n2");
    data->n2_turbine[3] = fgGetDouble("/engines/engine[3]/n2");

    data->n1_turbine[0] = fgGetDouble("/engines/engine[0]/n1");
    data->n1_turbine[1] = fgGetDouble("/engines/engine[1]/n1");
    data->n1_turbine[2] = fgGetDouble("/engines/engine[2]/n1");
    data->n1_turbine[3] = fgGetDouble("/engines/engine[3]/n1");
// the units for turbine engines is lbs/hr divide by 6.5 to convert to gph if piston type
    data->fuel_flow[0] = fgGetDouble("/engines/engine[0]/fuel-flow-gph");
    data->fuel_flow[1] = fgGetDouble("/engines/engine[1]/fuel-flow-gph");
    data->fuel_flow[2] = fgGetDouble("/engines/engine[2]/fuel-flow-gph");
    data->fuel_flow[3] = fgGetDouble("/engines/engine[3]/fuel-flow-gph");

    data->oil_pressure[0] = fgGetDouble("/engines/engine[0]/oil-pressure-psi");
    data->oil_pressure[1] = fgGetDouble("/engines/engine[1]/oil-pressure-psi");
    data->oil_pressure[2] = fgGetDouble("/engines/engine[2]/oil-pressure-psi");
    data->oil_pressure[3] = fgGetDouble("/engines/engine[3]/oil-pressure-psi");
    
    data->oil_temp[0] = fgGetDouble("/engines/engine[0]/OilTemp_degK");
    data->oil_temp[1] = fgGetDouble("/engines/engine[1]/OilTemp_degK");
    data->oil_temp[2] = fgGetDouble("/engines/engine[2]/OilTemp_degK");
    data->oil_temp[3] = fgGetDouble("/engines/engine[3]/OilTemp_degK");
    //  dummy value to fill data packet
    data->oil_quantity[0] = fgGetDouble("/engines/engine[0]/oil-quantity");
    data->oil_quantity[1] = fgGetDouble("/engines/engine[1]/oil-quantity");
    data->oil_quantity[2] = fgGetDouble("/engines/engine[2]/oil-quantity");
    data->oil_quantity[3] = fgGetDouble("/engines/engine[3]/oil-quantity");
    data->oil_quantity[0] = 40.3; data->oil_quantity[1] = 44.6;   
    
    data->hyd_pressure[0] = fgGetDouble("/engines/engine[0]/hyd-pressure-psi");
    data->hyd_pressure[1] = fgGetDouble("/engines/engine[1]/hyd-pressure-psi");
    data->hyd_pressure[2] = fgGetDouble("/engines/engine[2]/hyd-pressure-psi");
    data->hyd_pressure[3] = fgGetDouble("/engines/engine[3]/hyd-pressure-psi");
    data->hyd_pressure[0] = 3215.0; data->hyd_pressure[1] = 3756.0;
    
    data->man_pressure[0] = fgGetDouble("/engines/engine[0]/mp-osi");
    data->man_pressure[1] = fgGetDouble("/engines/engine[1]/mp-osi");
    
    data->tank[0] = fgGetDouble("/consumables/fuel/tank[2]/level-gal_us") * 6.6;
    data->tank[1] = fgGetDouble("/consumables/fuel/tank/level-gal_us") * 6.6;
    data->tank[2] = fgGetDouble("/consumables/fuel/tank[1]/level-gal_us") * 6.6;
    //data->tank[0] = 3000.0;
    //data->tank[1] = 9800.0;
    //data->tank[2] = 9800.0;

    data->total_temperature = cur_fdm_state->get_Total_temperature();
    data->total_pressure = cur_fdm_state->get_Total_pressure();
    data->dynamic_pressure = cur_fdm_state->get_Dynamic_pressure();
    
    data->static_pressure = press_node->getDoubleValue();
    data->static_temperature = temp_node->getDoubleValue();
    data->wind = wind_speed_node->getDoubleValue();
    data->wind_dir =  wind_dir_node->getDoubleValue();
}

static void distribute_data( const ogcFGData *data, FGInterface *chunk ) {
    // just a place holder until the CDU is developed
	
}

// process work for this port
bool FGOpenGC::process() {
    SGIOChannel *io = get_io_channel();
    int length = sizeof(buf);

    if ( get_direction() == SG_IO_OUT ) {
        collect_data( cur_fdm_state, &buf );
	//collect_data( &buf );
	if ( ! io->write( (char *)(& buf), length ) ) {
	    SG_LOG( SG_IO, SG_WARN, "Error writing data." );
	    return false;
	}
    } else if ( get_direction() == SG_IO_IN ) {
	if ( io->get_type() == sgFileType ) {
	    if ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		distribute_data( &buf, cur_fdm_state );
	    }
	} else {
	    while ( io->read( (char *)(& buf), length ) == length ) {
		SG_LOG( SG_IO, SG_DEBUG, "Success reading data." );
		distribute_data( &buf, cur_fdm_state );
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

