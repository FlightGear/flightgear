/**************************************************************************
 * cockpit.cxx -- routines to draw a cockpit (initial draft)
 *
 * Written by Michele America, started September 1997.
 *
 * Copyright (C) 1997  Michele F. America  - nomimarketing@mail.telepac.pt
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 * (Log is kept at end of this file)
 **************************************************************************/


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

#include <Aircraft/aircraft.h>
#include <Debug/fg_debug.h>
#include <Include/fg_constants.h>
#include <Include/general.h>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Math/fg_random.h>
#include <Math/mat3.h>
#include <Math/polar3d.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>
#include <Weather/weather.h>

#include "cockpit.hxx"


// This is a structure that contains all data related to
// cockpit/panel/hud system

static pCockpit ac_cockpit;

// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

double get_latitude( void )
{
	fgFLIGHT *f;
	f = current_aircraft.flight;

//	return( toDM(FG_Latitude * RAD_TO_DEG) );
	return((double)((int)( FG_Latitude * RAD_TO_DEG)) );
}
double get_lat_min( void )
{
	fgFLIGHT *f;
	double      a, d;

	f = current_aircraft.flight;
	
	a = FG_Latitude * RAD_TO_DEG;	
	if (a < 0.0) {
		a = -a;
	}
	d = (double) ( (int) a);
	return( (a - d) * 60.0);
}


double get_longitude( void )
{
	fgFLIGHT *f;
	f = current_aircraft.flight;

//	return( toDM(FG_Longitude * RAD_TO_DEG) );
	return((double)((int) (FG_Longitude * RAD_TO_DEG)) );
}
double get_long_min( void )
{
	fgFLIGHT *f;
	double  a, d;

	f = current_aircraft.flight;
	
	a = FG_Longitude * RAD_TO_DEG;	
	if (a < 0.0) {
		a = -a;
	}
	d = (double) ( (int) a);
	return( (a - d) * 60.0);
}

double get_throttleval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->throttle[0];     // Hack limiting to one engine
}

double get_aileronval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->aileron;
}

double get_elevatorval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->elevator;
}

double get_elev_trimval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->elevator_trim;
}

double get_rudderval( void )
{
	fgCONTROLS *pcontrols;

  pcontrols = current_aircraft.controls;
  return pcontrols->rudder;
}

double get_speed( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;
	return( FG_V_equiv_kts );    // Make an explicit function call.
}

double get_aoa( void )
{
	fgFLIGHT *f;
              
	f = current_aircraft.flight;
	return( FG_Gamma_vert_rad * RAD_TO_DEG );
}

double get_roll( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;
	return( FG_Phi );
}

double get_pitch( void )
{
	fgFLIGHT *f;
              
	f = current_aircraft.flight;
	return( FG_Theta );
}

double get_heading( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;
	return( FG_Psi * RAD_TO_DEG );
}

double get_altitude( void )
{
	fgFLIGHT *f;
	// double rough_elev;

	f = current_aircraft.flight;
	// rough_elev = mesh_altitude(FG_Longitude * RAD_TO_ARCSEC,
	//		                   FG_Latitude  * RAD_TO_ARCSEC);

	return( FG_Altitude * FEET_TO_METER /* -rough_elev */ );
}

double get_agl( void )
{
        fgFLIGHT *f;
        double agl;

        f = current_aircraft.flight;
        agl = FG_Altitude * FEET_TO_METER - scenery.cur_elev;

        return( agl );
}

double get_sideslip( void )
{
        fgFLIGHT *f;
        
        f = current_aircraft.flight;
        
        return( FG_Beta );
}

double get_frame_rate( void )
{
    fgGENERAL *pgeneral;
 
    pgeneral = &general;                     
 
    return pgeneral->frame_rate;
}

double get_fov( void )
{
    return (current_options.get_fov()); 
}

double get_vfc_ratio( void )
{
    fgVIEW *pview;
 
    pview = &current_view;
 
    return pview->vfc_ratio;
}

double get_vfc_tris_drawn   ( void )
{
    return current_view.tris_rendered;
}

double get_climb_rate( void )
{
	fgFLIGHT *f;

	f = current_aircraft.flight;

	return( FG_Climb_Rate * FEET_TO_METER * 60.0 );
}


bool fgCockpitInit( fgAIRCRAFT *cur_aircraft )
{
    fgPrintf( FG_COCKPIT, FG_INFO, "Initializing cockpit subsystem\n");

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
	fgPanelInit();
    }

    fgPrintf( FG_COCKPIT, FG_INFO,
	      "  Code %d  Status %d\n",
	      ac_cockpit->code(), ac_cockpit->status() );
    
    return true;
}


void fgCockpitUpdate( void ) {
    fgVIEW *pview;

    pview = &current_view;

    fgPrintf( FG_COCKPIT, FG_DEBUG,
	      "Cockpit: code %d   status %d\n",
	      ac_cockpit->code(), ac_cockpit->status() );

    if ( current_options.get_hud_status() ) {
	// This will check the global hud linked list pointer.
	// If these is anything to draw it will.
	fgUpdateHUD();
    }

    if ( current_options.get_panel_status() && 
	 (fabs(pview->view_offset) < 0.2) ) {
	fgPanelUpdate();
    }
}


/* $Log$
/* Revision 1.16  1998/09/29 02:01:06  curt
/* Added a "rate of climb" indicator.
/*
 * Revision 1.15  1998/08/28 18:14:39  curt
 * Added new cockpit code from Friedemann Reinhard
 * <mpt218@faupt212.physik.uni-erlangen.de>
 *
 * Revision 1.14  1998/08/24 20:05:15  curt
 * Added a second minimalistic HUD.
 * Added code to display the number of triangles rendered.
 *
 * Revision 1.13  1998/08/22 01:19:27  curt
 * Omit panel code because it's texture loading overruns array bounds.
 *
 * Revision 1.12  1998/07/13 21:28:00  curt
 * Converted the aoa scale to a radio altimeter.
 *
 * Revision 1.11  1998/07/13 21:00:45  curt
 * Integrated Charlies latest HUD updates.
 * Wrote access functions for current fgOPTIONS.
 *
 * Revision 1.10  1998/07/08 14:41:08  curt
 * Renamed polar3d.h to polar3d.hxx
 *
 * Revision 1.9  1998/06/27 16:47:53  curt
 * Incorporated Friedemann Reinhard's <mpt218@faupt212.physik.uni-erlangen.de>
 * first pass at an isntrument panel.
 *
 * Revision 1.8  1998/05/17 16:58:12  curt
 * Added a View Frustum Culling ratio display to the hud.
 *
 * Revision 1.7  1998/05/16 13:04:13  curt
 * New updates from Charlie Hotchkiss.
 *
 * Revision 1.6  1998/05/13 18:27:53  curt
 * Added an fov to hud display.
 *
 * Revision 1.5  1998/05/11 18:13:10  curt
 * Complete C++ rewrite of all cockpit code by Charlie Hotchkiss.
 *
 * Revision 1.4  1998/05/03 00:46:45  curt
 * polar.h -> polar3d.h
 *
 * Revision 1.3  1998/04/30 12:36:02  curt
 * C++-ifying a couple source files.
 *
 * Revision 1.2  1998/04/25 22:06:26  curt
 * Edited cvs log messages in source files ... bad bad bad!
 *
 * Revision 1.1  1998/04/24 00:45:54  curt
 * C++-ifing the code a bit.
 *
 * Revision 1.13  1998/04/18 04:14:01  curt
 * Moved fg_debug.c to it's own library.
 *
 * Revision 1.12  1998/04/14 02:23:09  curt
 * Code reorganizations.  Added a Lib/ directory for more general libraries.
 *
 * Revision 1.11  1998/03/14 00:32:13  curt
 * Changed a printf() to a fgPrintf().
 *
 * Revision 1.10  1998/02/07 15:29:33  curt
 * Incorporated HUD changes and struct/typedef changes from Charlie Hotchkiss
 * <chotchkiss@namg.us.anritsu.com>
 *
 * Revision 1.9  1998/02/03 23:20:14  curt
 * Lots of little tweaks to fix various consistency problems discovered by
 * Solaris' CC.  Fixed a bug in fg_debug.c with how the fgPrintf() wrapper
 * passed arguments along to the real printf().  Also incorporated HUD changes
 * by Michele America.
 *
 * Revision 1.8  1998/01/31 00:43:03  curt
 * Added MetroWorks patches from Carmen Volpe.
 *
 * Revision 1.7  1998/01/27 00:47:51  curt
 * Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
 * system and commandline/config file processing code.
 *
 * Revision 1.6  1998/01/19 19:27:01  curt
 * Merged in make system changes from Bob Kuehne <rpk@sgi.com>
 * This should simplify things tremendously.
 *
 * Revision 1.5  1998/01/19 18:40:19  curt
 * Tons of little changes to clean up the code and to remove fatal errors
 * when building with the c++ compiler.
 *
 * Revision 1.4  1997/12/30 20:47:34  curt
 * Integrated new event manager with subsystem initializations.
 *
 * Revision 1.3  1997/12/15 23:54:33  curt
 * Add xgl wrappers for debugging.
 * Generate terrain normals on the fly.
 *
 * Revision 1.2  1997/12/10 22:37:38  curt
 * Prepended "fg" on the name of all global structures that didn't have it yet.
 * i.e. "struct WEATHER {}" became "struct fgWEATHER {}"
 *
 * Revision 1.1  1997/08/29 18:03:20  curt
 * Initial revision.
 *
 */
