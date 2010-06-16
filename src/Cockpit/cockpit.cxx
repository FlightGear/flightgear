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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Aircraft/aircraft.hxx>
#include <Include/general.hxx>
#ifdef ENABLE_SP_FDM
#include <FDM/SP/ADA.hxx>
#endif
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/viewmgr.hxx>
#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>
#include <GUI/gui.h>

#include "cockpit.hxx"
#include "hud.hxx"

// ugly hack, make the raw FDM available here, to support some
// legacy accessor functions
extern FGInterface* evil_global_fdm_state;

// The following routines obtain information concerntin the aircraft's
// current state and return it to calling instrument display routines.
// They should eventually be member functions of the aircraft.
//

float get_latitude( void )
{
    return fgGetDouble("/position/latitude-deg");
}

float get_lat_min( void )
{
    double a, d;

    a = fgGetDouble("/position/latitude-deg");
    if (a < 0.0) {
        a = -a;
    }
    d = (double) ( (int) a);
    float lat_min = (a - d) * 60.0;

    return lat_min;
}


float get_longitude( void )
{
    return fgGetDouble("/position/longitude-deg");
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
    a = fgGetDouble("/position/longitude-deg");
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

    float speed = fgGetDouble("/velocities/airspeed-kt")
        * speedup_node->getIntValue();

    return speed;
}

float get_mach(void)
{
    return fgGetDouble("/velocities/mach");
}

float get_aoa( void )
{
    return fgGetDouble("/orientation/alpha-deg");
}

float get_roll( void )
{
    return fgGetDouble("/orientation/roll-deg") * SG_DEGREES_TO_RADIANS;
}

float get_pitch( void )
{
    return fgGetDouble("/orientation/pitch-deg") * SG_DEGREES_TO_RADIANS;
}

float get_heading( void )
{
    return fgGetDouble("/orientation/heading-deg");
}

float get_altitude( void )
{
    static const SGPropertyNode *startup_units_node
        = fgGetNode("/sim/startup/units");

    if ( !strcmp(startup_units_node->getStringValue(), "feet") ) {
        return fgGetDouble("/position/altitude-ft");
    } else {
        return fgGetDouble("/position/altitude-ft") * SG_FEET_TO_METER;
    }
}

float get_agl( void )
{
    static const SGPropertyNode *startup_units_node
        = fgGetNode("/sim/startup/units");

    if ( !strcmp(startup_units_node->getStringValue(), "feet") ) {
        return fgGetDouble("/position/altitude-agl-ft");
    } else {
        return fgGetDouble("/position/altitude-agl-ft") * SG_FEET_TO_METER;
    }
}

float get_sideslip( void )
{
    return fgGetDouble("/orientation/side-slip-rad");
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

    float climb_rate = fgGetDouble("/velocities/vertical-speed-fps", 0.0);
    if ( !strcmp(startup_units_node->getStringValue(), "feet") ) {
        climb_rate *= 60.0;
    } else {
        climb_rate *= SG_FEET_TO_METER * 60.0;
    }

    return climb_rate;
}


float get_view_direction( void )
{
    double view_off = 360.0 - globals->get_current_view()->getHeadingOffset_deg();
    double view = SGMiscd::normalizeAngle(fgGetDouble("/orientation/heading-deg") + view_off);
    return view;
}

// Added by Markus Hof on 5. Jan 2004
float get_dme( void )
{
    static const SGPropertyNode * dme_node =
        fgGetNode("/instrumentation/dme/indicated-distance-nm");

    return dme_node->getFloatValue();
}

// $$$ begin - added, VS Renganathan 13 Oct 2K
// #ifdef FIGHTER_HUD
float get_Vx   ( void )
{
    // Curt dont comment this and return zero. - Ranga
    // Please remove comments from get_V_..() function in flight.hxx
    float Vxx = evil_global_fdm_state->get_V_north_rel_ground();
    return Vxx;
}

float get_Vy   ( void )
{
    // Curt dont comment this and return zero. - Ranga
    // Please remove comments from get_V_..() function in flight.hxx
    float Vyy = evil_global_fdm_state->get_V_east_rel_ground();
    return Vyy;
}

float get_Vz   ( void )
{
    // Curt dont comment this and return zero. - Ranga
    // Please remove comments from get_V_..() function in flight.hxx
    float Vzz = evil_global_fdm_state->get_V_down_rel_ground();
    return Vzz;
}

float get_Ax   ( void )
{
    float Ax = evil_global_fdm_state->get_V_dot_north();
    return Ax;
}

float get_Ay   ( void )
{
    float Ay = evil_global_fdm_state->get_V_dot_east();
    return Ay;
}

float get_Az   ( void )
{
    float Az = evil_global_fdm_state->get_V_dot_down();
    return Az;
}

float get_anzg   ( void )
{
    float anzg = evil_global_fdm_state->get_N_Z_cg();
    return anzg;
}

#ifdef ENABLE_SP_FDM
int get_iaux1 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(1);
}

int get_iaux2 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(2);
}

int get_iaux3 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(3);
}

int get_iaux4 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(4);
}

int get_iaux5 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(5);
}

int get_iaux6 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(6);
}

int get_iaux7 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(7);
}

int get_iaux8 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(8);
}

int get_iaux9 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(9);
}

int get_iaux10 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(10);
}

int get_iaux11 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_iaux(11);
}

int get_iaux12 (void)
{
     FGADA *fdm = (FGADA *)evil_global_fdm_state;
     return fdm->get_iaux(12);
}

float get_aux1 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_daux(1);
}

float get_aux2 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_daux(2);
}

float get_aux3 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_daux(3);
}

float get_aux4 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_daux(4);
}

float get_aux5 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_daux(5);
}

float get_aux6 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_daux(6);
}

float get_aux7 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_daux(7);
}

float get_aux8 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_daux(8);
}

float get_aux9 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(1);
}

float get_aux10 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(2);
}

float get_aux11 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(3);
}

float get_aux12 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(4);
}

float get_aux13 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(5);
}

float get_aux14 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(6);
}

float get_aux15 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(7);
}

float get_aux16 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(8);
}

float get_aux17 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(9);
}

float get_aux18 (void)
{
    FGADA *fdm = (FGADA *)evil_global_fdm_state;
    return fdm->get_faux(10);
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

    return true;
}

void fgCockpitUpdate( osg::State* state ) {

    static const SGPropertyNode * xsize_node = fgGetNode("/sim/startup/xsize");
    static const SGPropertyNode * ysize_node = fgGetNode("/sim/startup/ysize");
    static const SGPropertyNode * hud_visibility_node
        = fgGetNode("/sim/hud/visibility");

    int iwidth   = xsize_node->getIntValue();
    int iheight  = ysize_node->getIntValue();

                                // FIXME: inefficient
    if ( hud_visibility_node->getBoolValue() ) {
        // This will check the global hud linked list pointer.
        // If there is anything to draw it will.
        fgUpdateHUD( state );
    }

    glViewport( 0, 0, iwidth, iheight );
}





struct FuncTable {
    const char *name;
    FLTFNPTR func;
} fn_table[] = {
    { "agl", get_agl },
    { "aileronval", get_aileronval },
    { "altitude", get_altitude },
    { "anzg", get_anzg },
    { "aoa", get_aoa },
    { "ax", get_Ax },
    { "climb", get_climb_rate },
    { "elevatortrimval", get_elev_trimval },
    { "elevatorval", get_elevatorval },
    { "fov", get_fov },
    { "framerate", get_frame_rate },
    { "heading", get_heading },
    { "latitude", get_latitude },
    { "longitude", get_longitude },
    { "mach", get_mach },
    { "rudderval", get_rudderval },
    { "speed", get_speed },
    { "throttleval", get_throttleval },
    { "view_direction", get_view_direction },
    { "vfc_tris_culled", get_vfc_tris_culled },
    { "vfc_tris_drawn", get_vfc_tris_drawn },
#ifdef ENABLE_SP_FDM
    { "aux1", get_aux1 },
    { "aux2", get_aux2 },
    { "aux3", get_aux3 },
    { "aux4", get_aux4 },
    { "aux5", get_aux5 },
    { "aux6", get_aux6 },
    { "aux7", get_aux7 },
    { "aux8", get_aux8 },
    { "aux9", get_aux9 },
    { "aux10", get_aux10 },
    { "aux11", get_aux11 },
    { "aux12", get_aux12 },
    { "aux13", get_aux13 },
    { "aux14", get_aux14 },
    { "aux15", get_aux15 },
    { "aux16", get_aux16 },
    { "aux17", get_aux17 },
    { "aux18", get_aux18 },
#endif
    { 0, 0 },
};


FLTFNPTR get_func(const char *name)
{
    for (int i = 0; fn_table[i].name; i++)
        if (!strcmp(fn_table[i].name, name))
            return fn_table[i].func;

    return 0;
}


