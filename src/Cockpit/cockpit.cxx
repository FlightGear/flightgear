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

#include <GL/glu.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Aircraft/aircraft.hxx>
#include <Include/general.hxx>
#include <FDM/ADA.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/viewmgr.hxx>
#include <Scenery/scenery.hxx>
#include <Time/fg_timer.hxx>
#include <GUI/gui.h>

#include "cockpit.hxx"
#include "hud.hxx"


// This is a structure that contains all data related to
// cockpit/panel/hud system

static pCockpit ac_cockpit;
// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

float get_latitude( void )
{
    return current_aircraft.fdm_state->get_Latitude() * SGD_RADIANS_TO_DEGREES;
}

float get_lat_min( void )
{
    double a, d;

    a = current_aircraft.fdm_state->get_Latitude() * SGD_RADIANS_TO_DEGREES;    
    if (a < 0.0) {
        a = -a;
    }
    d = (double) ( (int) a);
    float lat_min = (a - d) * 60.0;

    return lat_min;
}


float get_longitude( void )
{
    return current_aircraft.fdm_state->get_Longitude() * SGD_RADIANS_TO_DEGREES;
}


char*
get_formated_gmt_time( void )
{
    static char buf[32];
    const struct tm *p = globals->get_time_params()->getGmt();
    sprintf( buf, "%d/%d/%4d %d:%02d:%02d", 
         p->tm_mon+1, p->tm_mday, 1900 + p->tm_year,
         p->tm_hour, p->tm_min, p->tm_sec);

    return buf;
}


float get_long_min( void )
{
    double  a, d;
    a = current_aircraft.fdm_state->get_Longitude() * SGD_RADIANS_TO_DEGREES;   
    if (a < 0.0) {
        a = -a;
    }
    d = (double) ( (int) a);
    float lon_min = (a - d) * 60.0; 

    return lon_min;
}

float get_throttleval( void )
{
    // Hack limiting to one engine
    return globals->get_controls()->get_throttle( 0 );
}

float get_aileronval( void )
{
    return globals->get_controls()->get_aileron();
}

float get_elevatorval( void )
{
    return globals->get_controls()->get_elevator();
}

float get_elev_trimval( void )
{
    return globals->get_controls()->get_elevator_trim();
}

float get_rudderval( void )
{
    return globals->get_controls()->get_rudder();
}

float get_speed( void )
{
    static const SGPropertyNode * speedup_node = fgGetNode("/sim/speed-up");

    float speed = current_aircraft.fdm_state->get_V_calibrated_kts()
	* speedup_node->getIntValue();

    return speed;
}

float get_mach(void)
{
    return current_aircraft.fdm_state->get_Mach_number();
}	

float get_aoa( void )
{
    return current_aircraft.fdm_state->get_Alpha() * SGD_RADIANS_TO_DEGREES;
}

float get_roll( void )
{
    return current_aircraft.fdm_state->get_Phi();
}

float get_pitch( void )
{
    return current_aircraft.fdm_state->get_Theta();
}

float get_heading( void )
{
    return current_aircraft.fdm_state->get_Psi() * SGD_RADIANS_TO_DEGREES;
}

float get_altitude( void )
{
    static const SGPropertyNode *startup_units_node
        = fgGetNode("/sim/startup/units");

    float altitude;

    if ( !strcmp(startup_units_node->getStringValue(), "feet") ) {
        altitude = current_aircraft.fdm_state->get_Altitude();
    } else {
        altitude = (current_aircraft.fdm_state->get_Altitude()
                    * SG_FEET_TO_METER);
    }

    return altitude;
}

float get_agl( void )
{
    static const SGPropertyNode *startup_units_node
        = fgGetNode("/sim/startup/units");

    float agl;

    if ( !strcmp(startup_units_node->getStringValue(), "feet") ) {
        agl = (current_aircraft.fdm_state->get_Altitude()
               - globals->get_scenery()->get_cur_elev() * SG_METER_TO_FEET);
    } else {
        agl = (current_aircraft.fdm_state->get_Altitude() * SG_FEET_TO_METER
               - globals->get_scenery()->get_cur_elev());
    }

    return agl;
}

float get_sideslip( void )
{
    return current_aircraft.fdm_state->get_Beta();
}

float get_frame_rate( void )
{
    return general.get_frame_rate();
}

float get_fov( void )
{
    return globals->get_current_view()->get_fov(); 
}

float get_vfc_ratio( void )
{
    // float vfc = current_view.get_vfc_ratio();
    // return (vfc);
    return 0.0;
}

float get_vfc_tris_drawn   ( void )
{
    // float rendered = current_view.get_tris_rendered();
    // return (rendered);
    return 0.0;
}

float get_vfc_tris_culled   ( void )
{
    // float culled = current_view.get_tris_culled();
    // return (culled);
    return 0.0;
}

float get_climb_rate( void )
{
    static const SGPropertyNode *startup_units_node
        = fgGetNode("/sim/startup/units");

    float climb_rate;
    if ( !strcmp(startup_units_node->getStringValue(), "feet") ) {
        climb_rate = current_aircraft.fdm_state->get_Climb_Rate() * 60.0;
    } else {
        climb_rate = current_aircraft.fdm_state->get_Climb_Rate() * SG_FEET_TO_METER * 60.0;
    }

    return climb_rate;
}


float get_view_direction( void )
{
    double view_off = SGD_2PI - globals->get_current_view()->getHeadingOffset_deg() * SGD_DEGREES_TO_RADIANS;
    double view = ( current_aircraft.fdm_state->get_Psi() + view_off)
	* SGD_RADIANS_TO_DEGREES;
    
    if(view > 360.)
        view -= 360.;
    else if(view<0.)
        view += 360.;
    
    return view;
}

// Added by Markus Hof on 5. Jan 2004
float get_dme( void )
{
    static const SGPropertyNode * dme_node =
        fgGetNode("/radios/dme/distance-nm");

    return dme_node->getFloatValue();
}

// $$$ begin - added, VS Renganathan 13 Oct 2K
// #ifdef FIGHTER_HUD
float get_Vx   ( void )
{
    // Curt dont comment this and return zero. - Ranga
    // Please remove comments from get_V_..() function in flight.hxx
    float Vxx = current_aircraft.fdm_state->get_V_north_rel_ground();
    return Vxx;
}

float get_Vy   ( void )
{
    // Curt dont comment this and return zero. - Ranga
    // Please remove comments from get_V_..() function in flight.hxx
    float Vyy = current_aircraft.fdm_state->get_V_east_rel_ground();
    return Vyy;
}

float get_Vz   ( void )
{
    // Curt dont comment this and return zero. - Ranga
    // Please remove comments from get_V_..() function in flight.hxx
    float Vzz = current_aircraft.fdm_state->get_V_down_rel_ground();
    return Vzz;
}

float get_Ax   ( void )
{
    float Ax = current_aircraft.fdm_state->get_V_dot_north();
    return Ax;
}

float get_Ay   ( void )
{
    float Ay = current_aircraft.fdm_state->get_V_dot_east();
    return Ay;
}

float get_Az   ( void )
{
    float Az = current_aircraft.fdm_state->get_V_dot_down();
    return Az;
}

float get_anzg   ( void )
{
    float anzg = current_aircraft.fdm_state->get_N_Z_cg();
    return anzg;
}

int get_iaux1 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(1);
}

int get_iaux2 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(2);
}

int get_iaux3 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(3);
}

int get_iaux4 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(4);
}

int get_iaux5 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(5);
}

int get_iaux6 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(6);
}

int get_iaux7 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(7);
}

int get_iaux8 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(8);
}

int get_iaux9 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(9);
}

int get_iaux10 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(10);
}

int get_iaux11 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_iaux(11);
}

int get_iaux12 (void)
{
     FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
     return fdm->get_iaux(12);
}

float get_aux1 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_daux(1);
}

float get_aux2 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_daux(2);
}

float get_aux3 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_daux(3);
}

float get_aux4 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_daux(4);
}

float get_aux5 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_daux(5);
}

float get_aux6 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_daux(6);
}

float get_aux7 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_daux(7);
}

float get_aux8 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_daux(8);
}

float get_aux9 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(1);
}

float get_aux10 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(2);
}

float get_aux11 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(3);
}

float get_aux12 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(4);
}

float get_aux13 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(5);
}

float get_aux14 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(6);
}

float get_aux15 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(7);
}

float get_aux16 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(8);
}

float get_aux17 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(9);
}

float get_aux18 (void)
{
    FGADA *fdm = (FGADA *)current_aircraft.fdm_state;
    return fdm->get_faux(10);
}
// #endif
// $$$ end - added, VS Renganathan 13 Oct 2K


#ifdef NOT_USED
/****************************************************************************/
/* Convert degrees to dd mm'ss.s" (DMS-Format)                              */
/****************************************************************************/
char *dmshh_format(double degrees)
{
    static char buf[16];    
    int deg_part;
    int min_part;
    double sec_part;
    
    if (degrees < 0)
      degrees = -degrees;

    deg_part = degrees;
    min_part = 60.0 * (degrees - deg_part);
    sec_part = 3600.0 * (degrees - deg_part - min_part / 60.0);

    /* Round off hundredths */
    if (sec_part + 0.005 >= 60.0)
      sec_part -= 60.0, min_part += 1;
    if (min_part >= 60)
      min_part -= 60, deg_part += 1;

    sprintf(buf,"%02d*%02d %05.2f",deg_part,min_part,sec_part);

    return buf;
}
#endif // 0


/************************************************************************
 Convert degrees to dd mm.mmm' (DMM-Format)
 Description: Converts using a round-off factor tailored to the required
 precision of the minutes field (three decimal places).  Round-off
 prevents function from returning a minutes value of 60.

 Input arguments: Coordinate value in decimal degrees

************************************************************************/
static char *toDM(float dd)
{
    static char  dm[16];
    double tempdd;
    double mn;
    double sign = 1;
    int deg;

    if (dd < 0) {
	sign = -1;
    }
    /* round for minutes expressed to three decimal places */
    tempdd = fabs(dd) + (5.0E-4 / 60.0);
    deg = (int)tempdd;
    mn = fabs( (tempdd - (double)(deg)) * 60.0 - 4.999E-4 );
    deg *= (int)sign;
    sprintf(dm, "%d*%06.3f", deg, mn);
    return dm;
}


/************************************************************************
 Convert degrees to dd mm'ss.s'' (DMS-Format)
 Description: Converts using a round-off factor tailored to the required
 precision of the seconds field (one decimal place).  Round-off
 prevents function from returning a seconds value of 60.

 Input arguments: Coordinate value in decimal degrees

************************************************************************/
static char *toDMS(float dd)
{
    static char  dms[16];
    double tempdd, tempmin;
    int deg;
    int mn;
    double sec;
    double sign = 1;

    if(dd < 0) {
	sign = -1;
    }
    /* round up for seconds expressed to one decimal place */
    tempdd = fabs(dd) + (0.05 / 3600.0);
    deg = (int)tempdd;
    tempmin =  (tempdd - (double)(deg)) * 60.0;
    mn = (int)tempmin;
    sec = fabs( (tempmin - (double)(mn)) * 60.0 - 0.049 );
    deg *= (int)sign;
    sprintf(dms, "%d*%02d %04.1f", deg, mn, sec);
    return dms;
}


// Have to set the LatLon display type
//static char *(*fgLatLonFormat)(float) = toDM;
static char *(*fgLatLonFormat)(float);

char *coord_format_lat(float latitude)
{
    static char buf[16];

    sprintf(buf,"%s%c",
//      dmshh_format(latitude),
//      toDMS(latitude),
//      toDM(latitude),
        fgLatLonFormat(latitude),           
        latitude > 0 ? 'N' : 'S');
    return buf;
}

char *coord_format_lon(float longitude)
{
    static char buf[80];

    sprintf(buf,"%s%c",
//      dmshh_format(longitude),
//      toDMS(longitude),
//      toDM(longitude),
        fgLatLonFormat(longitude),
        longitude > 0 ? 'E' : 'W');
    return buf;
}

void fgLatLonFormatToggle( puObject *)
{
    static int toggle = 0;

    if ( toggle ) 
        fgLatLonFormat = toDM;
    else
        fgLatLonFormat = toDMS;
    
    toggle = ~toggle;
}

#ifdef NOT_USED
char *coord_format_latlon(double latitude, double longitude)
{
    static char buf[1024];

    sprintf(buf,"%s%c %s%c",
        dmshh_format(latitude),
        latitude > 0 ? 'N' : 'S',
        dmshh_format(longitude),
        longitude > 0 ? 'E' : 'W');
    return buf;
}
#endif


bool fgCockpitInit( fgAIRCRAFT *cur_aircraft )
{
    SG_LOG( SG_COCKPIT, SG_INFO, "Initializing cockpit subsystem" );

    //  cockpit->code = 1;  /* It will be aircraft dependent */
    //  cockpit->status = 0;

    // If aircraft has HUD specified we will get the specs from its def
    // file. For now we will depend upon hard coding in hud?
    
    // We must insure that the existing instrument link is purged.
    // This is done by deleting the links in the list.
    
    // HI_Head is now a null pointer so we can generate a new list from the
    // current aircraft.

    fgHUDInit( cur_aircraft );
    ac_cockpit = new fg_Cockpit();
    
    // Have to set the LatLon display type
    fgLatLonFormat = toDM;
    
    SG_LOG( SG_COCKPIT, SG_INFO,
        "  Code " << ac_cockpit->code() << " Status " 
        << ac_cockpit->status() );

	return true;
}

void fgCockpitUpdate( void ) {

    SG_LOG( SG_COCKPIT, SG_DEBUG,
            "Cockpit: code " << ac_cockpit->code() << " status " 
            << ac_cockpit->status() );

    static const SGPropertyNode * xsize_node = fgGetNode("/sim/startup/xsize");
    static const SGPropertyNode * ysize_node = fgGetNode("/sim/startup/ysize");
    static const SGPropertyNode * hud_visibility_node
        = fgGetNode("/sim/hud/visibility");

    int iwidth   = xsize_node->getIntValue();
    int iheight  = ysize_node->getIntValue();
    float width  = iwidth;
    // float height = iheight;

				// FIXME: inefficient
    if ( hud_visibility_node->getBoolValue() ) {
        // This will check the global hud linked list pointer.
        // If these is anything to draw it will.
        fgUpdateHUD();
    }

    if ( fgGetBool( "/sim/hud/draw-fps", false ) ) {
        char buf[64];
        float fps = get_frame_rate();
        sprintf(buf,"%-5.1f", fps);

        glMatrixMode( GL_PROJECTION );
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D( 0, iwidth, 0, iheight );
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();
        glLoadIdentity();

        glDisable( GL_DEPTH_TEST );
        glDisable( GL_LIGHTING );
        
        glColor3f( 0.9, 0.4, 0.2 );

        guiFnt.drawString( buf,
			   int(width - guiFnt.getStringWidth(buf) - 10),
                           10 );
        glEnable( GL_DEPTH_TEST );
        glEnable( GL_LIGHTING );
        glMatrixMode( GL_PROJECTION );
        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );
        glPopMatrix();
    }
    
    glViewport( 0, 0, iwidth, iheight );
}

