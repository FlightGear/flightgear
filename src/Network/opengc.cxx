// opengc.cxx - Network interface program to send sim data onto a LAN
//
// Created by: 	J. Wojnaroski  -- castle@mminternet.com
// Date:		21 Nov 2001 
//
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
    //static void collect_data( ogcFGData *data ) {

    data->version_id = 0x0011;

    data->latitude = fdm->get_Longitude_deg();   
    data->longitude = cur_fdm_state->get_Latitude_deg();

    data->pitch = cur_fdm_state->get_Theta_deg();
    data->bank = cur_fdm_state->get_Phi_deg();
    data->heading = cur_fdm_state->get_Psi_deg();
    data->altitude = cur_fdm_state->get_Altitude();
    data->v_kcas = cur_fdm_state->get_V_calibrated_kts();
    data->vvi = cur_fdm_state->get_Climb_Rate();

    data->magvar = globals->get_mag()->get_magvar();

    data->rpm[0] = fgGetDouble("/engines/engine[0]/rpm");
    data->rpm[1] = fgGetDouble("/engines/engine[1]/rpm");

    data->epr[0] = fgGetDouble("/engines/engine[0]/mp-osi");
    data->epr[1] = fgGetDouble("/engines/engine[1]/mp-osi");

    data->egt[0] = fgGetDouble("/engines/engine[0]/egt-degf");
    data->egt[1] = fgGetDouble("/engines/engine[1]/egt-degf");

    data->oil_pressure[0] = fgGetDouble("/engines/engine[0]/oil-pressure-psi");
    data->oil_pressure[1] = fgGetDouble("/engines/engine[1]/oil-pressure-psi");


// navigation data
// Once OPenGC develops a comparable navaids database some of this will not be required

    //data->nav1_ident = current_radiostack->get_nav1_ident();
    data->nav1_freq = current_radiostack->get_nav1_freq();
    data->nav1_radial = current_radiostack->get_nav1_sel_radial();
    data->nav1_course_dev = current_radiostack->get_nav1_heading_needle_deflection(); 

    //data->nav1_ident = current_radiostack->get_nav1_ident();
    data->nav2_freq = current_radiostack->get_nav2_freq();
    data->nav2_radial = current_radiostack->get_nav2_sel_radial();
    data->nav2_course_dev = current_radiostack->get_nav2_heading_needle_deflection(); 

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

