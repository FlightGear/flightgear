// cockpit.cxx -- routines to draw a cockpit (initial draft)
//
// Written by Michele America, started September 1997.
//
// Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <XGL/xgl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <Aircraft/aircraft.hxx>
#include <Debug/logstream.hxx>
#include <Include/fg_constants.h>
#include <Include/general.hxx>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>

#include "cockpit.hxx"
#include "panel.hxx"


// This is a structure that contains all data related to
// cockpit/panel/hud system

static pCockpit ac_cockpit;

// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.

double get_latitude( void )
{
    return((double)((int)( current_aircraft.fdm_state->get_Latitude() 
			   * RAD_TO_DEG)) );
}

double get_lat_min( void )
{
    double a, d;

    a = current_aircraft.fdm_state->get_Latitude() * RAD_TO_DEG;	
    if (a < 0.0) {
	a = -a;
    }
    d = (double) ( (int) a);
    return( (a - d) * 60.0);
}

double get_longitude( void )
{
    return( (double)((int) (current_aircraft.fdm_state->get_Longitude()
			    * RAD_TO_DEG)) );
}

double get_long_min( void )
{
    double  a, d;
    
    a = current_aircraft.fdm_state->get_Longitude() * RAD_TO_DEG;	
    if (a < 0.0) {
	a = -a;
    }
    d = (double) ( (int) a);
    return( (a - d) * 60.0);
}

double get_throttleval( void )
{
    return controls.get_throttle( 0 );     // Hack limiting to one engine
}

double get_aileronval( void )
{
    return controls.get_aileron();
}

double get_elevatorval( void )
{
    return controls.get_elevator();
}

double get_elev_trimval( void )
{
    return controls.get_elevator_trim();
}

double get_rudderval( void )
{
    return controls.get_rudder();
}

double get_speed( void )
{
    return( current_aircraft.fdm_state->get_V_equiv_kts() );
}

double get_aoa( void )
{
    return( current_aircraft.fdm_state->get_Alpha() * RAD_TO_DEG );
}

double get_roll( void )
{
    return( current_aircraft.fdm_state->get_Phi() );
}

double get_pitch( void )
{
    return( current_aircraft.fdm_state->get_Theta() );
}

double get_heading( void )
{
    return( current_aircraft.fdm_state->get_Psi() * RAD_TO_DEG );
}

double get_altitude( void )
{
    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
	return current_aircraft.fdm_state->get_Altitude();
    } else {
	return current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER;
    }
}

double get_agl( void )
{
    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
	return current_aircraft.fdm_state->get_Altitude()
	    - scenery.cur_elev * METER_TO_FEET;
    } else {
	return current_aircraft.fdm_state->get_Altitude() * FEET_TO_METER
	    - scenery.cur_elev;
    }
}

double get_sideslip( void )
{
    return( current_aircraft.fdm_state->get_Beta() );
}

double get_frame_rate( void )
{
    return (double) general.get_frame_rate();
}

double get_fov( void )
{
    return (current_options.get_fov()); 
}

double get_vfc_ratio( void )
{
    return current_view.get_vfc_ratio();
}

double get_vfc_tris_drawn   ( void )
{
    return current_view.get_tris_rendered();
}

double get_climb_rate( void )
{
    if ( current_options.get_units() == fgOPTIONS::FG_UNITS_FEET ) {
	return current_aircraft.fdm_state->get_Climb_Rate() * 60.0;
    } else {
	return current_aircraft.fdm_state->get_Climb_Rate()
	    * FEET_TO_METER * 60.0;
    }
}


bool fgCockpitInit( fgAIRCRAFT *cur_aircraft )
{
    FG_LOG( FG_COCKPIT, FG_INFO, "Initializing cockpit subsystem" );

    //	cockpit->code = 1;	/* It will be aircraft dependent */
    //	cockpit->status = 0;

    // If aircraft has HUD specified we will get the specs from its def
    // file. For now we will depend upon hard coding in hud?
    
    // We must insure that the existing instrument link is purged.
    // This is done by deleting the links in the list.
    
    // HI_Head is now a null pointer so we can generate a new list from the
    // current aircraft.

    fgHUDInit( cur_aircraft );
    ac_cockpit = new fg_Cockpit();
    
    if ( current_options.get_panel_status() ) {
        new FGPanel;
    }

    FG_LOG( FG_COCKPIT, FG_INFO,
	    "  Code " << ac_cockpit->code() << " Status " 
	    << ac_cockpit->status() );
    
    return true;
}


void fgCockpitUpdate( void ) {
    FG_LOG( FG_COCKPIT, FG_DEBUG,
	    "Cockpit: code " << ac_cockpit->code() << " status " 
	    << ac_cockpit->status() );

    if ( current_options.get_hud_status() ) {
	// This will check the global hud linked list pointer.
	// If these is anything to draw it will.
	fgUpdateHUD();
    }

    if ( current_options.get_panel_status() && 
	 (fabs( current_view.get_view_offset() ) < 0.2) )
    {
	xglViewport( 0, 0, 
		     current_view.get_winWidth(), 
		     current_view.get_winHeight() );
	FGPanel::OurPanel->Update();
    }
}


