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
#include <Cockpit/radiostack.hxx>
#include <Controls/controls.hxx>
#include <Main/fg_props.hxx>

SG_USING_STD(vector);

FGOpenGC::FGOpenGC() {
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


static void collect_data( const FGInterface *fdm, ogcFGData *data ) {
												
    data->version_id = 0x0012;

    data->longitude = fdm->get_Longitude_deg();   
    data->latitude = cur_fdm_state->get_Latitude_deg();
    data->magvar = globals->get_mag()->get_magvar();
   
    data->pitch = cur_fdm_state->get_Theta_deg();
    data->bank = cur_fdm_state->get_Phi_deg();
    data->heading = cur_fdm_state->get_Psi_deg();
    data->altitude = cur_fdm_state->get_Altitude();
    data->altitude_agl = cur_fdm_state->get_Altitude_AGL();
    data->v_kcas = cur_fdm_state->get_V_calibrated_kts();
    data->vvi = cur_fdm_state->get_Climb_Rate();
    data->mach = cur_fdm_state->get_Mach_number();
    data->groundspeed = cur_fdm_state->get_V_ground_speed();
    data->v_tas = cur_fdm_state->get_V_equiv_kts();

    data->alpha = cur_fdm_state->get_Alpha();
    data->beta = cur_fdm_state->get_Beta();
    data->alpha_dot = cur_fdm_state->get_Alpha_dot();
    data->beta_dot = cur_fdm_state->get_Beta_dot();

    data->static_temperature = cur_fdm_state->get_Static_temperature();
    data->total_temperature = cur_fdm_state->get_Total_temperature();

    data->static_pressure = cur_fdm_state->get_Static_pressure();
    data->total_pressure = cur_fdm_state->get_Total_pressure();
    data->dynamic_pressure = cur_fdm_state->get_Dynamic_pressure();

    data->rpm[0] = fgGetDouble("/engines/engine[0]/rpm");
    data->rpm[1] = fgGetDouble("/engines/engine[1]/rpm");   

    data->epr[0] = fgGetDouble("/engines/engine[0]/epr");
    data->epr[1] = fgGetDouble("/engines/engine[1]/epr");
    data->epr[2] = fgGetDouble("/engines/engine[2]/epr");
    data->epr[3] = fgGetDouble("/engines/engine[3]/epr");

    data->egt[0] = fgGetDouble("/engines/engine[0]/egt-degf");
    data->egt[1] = fgGetDouble("/engines/engine[1]/egt-degf");
    data->egt[2] = fgGetDouble("/engines/engine[2]/egt-degf");
    data->egt[3] = fgGetDouble("/engines/engine[3]/egt-degf");

    data->n2_turbine[0] = fgGetDouble("/engines/engine[0]/n2");
    data->n2_turbine[1] = fgGetDouble("/engines/engine[1]/n2");
    data->n2_turbine[2] = fgGetDouble("/engines/engine[2]/n2");
    data->n2_turbine[3] = fgGetDouble("/engines/engine[3]/n2");

    data->n1_turbine[0] = fgGetDouble("/engines/engine[0]/n1");
    data->n1_turbine[1] = fgGetDouble("/engines/engine[1]/n1");
    data->n1_turbine[2] = fgGetDouble("/engines/engine[2]/n1");
    data->n1_turbine[3] = fgGetDouble("/engines/engine[3]/n1");

    data->fuel_flow[0] = fgGetDouble("/engines/engine[0]/fuel-flow-gph");
    data->fuel_flow[1] = fgGetDouble("/engines/engine[1]/fuel-flow-gph");
    data->fuel_flow[1] = fgGetDouble("/engines/engine[1]/fuel-flow-gph");
    data->fuel_flow[2] = fgGetDouble("/engines/engine[2]/fuel-flow-gph");

    data->oil_pressure[0] = fgGetDouble("/engines/engine[0]/oil-pressure-psi");
    data->oil_pressure[1] = fgGetDouble("/engines/engine[1]/oil-pressure-psi");
    data->oil_pressure[2] = fgGetDouble("/engines/engine[2]/oil-pressure-psi");
    data->oil_pressure[3] = fgGetDouble("/engines/engine[3]/oil-pressure-psi");

    data->man_pressure[0] = fgGetDouble("/engines/engine[0]/mp-osi");
    data->man_pressure[1] = fgGetDouble("/engines/engine[1]/mp-osi");    

    //data->gear_nose = p_gear[0]->GetPosition();
    //data->gear_left = p_gear[1]->GetPosition();
    //data->gear_right = p_gear[2]->GetPosition();
    
    data->aileron = globals->get_controls()->get_aileron();
    //data->aileron_trim = p_Controls->get_aileron_trim();
    data->elevator = globals->get_controls()->get_elevator();
    data->elevator_trim = globals->get_controls()->get_elevator_trim();
    data->rudder = globals->get_controls()->get_rudder();
    //data->rudder_trim = p_Controls->get_rudder_trim();
    //data->flaps = fgGetDouble("/controls/flaps");
    data->flaps = globals->get_controls()->get_flaps();
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
	    SG_LOG( SG_IO, SG_ALERT, "Error writing data." );
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

