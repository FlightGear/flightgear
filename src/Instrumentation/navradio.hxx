// navradio.hxx -- class to manage a nav radio instance
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000 - 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_NAVRADIO_HXX
#define _FG_NAVRADIO_HXX


#include <Main/fg_props.hxx>

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Navaids/navlist.hxx>
#include <Sound/morse.hxx>

class FGNavRadio : public SGSubsystem
{
    FGMorse morse;

    SGInterpTable *term_tbl;
    SGInterpTable *low_tbl;
    SGInterpTable *high_tbl;

    SGPropertyNode *lon_node;
    SGPropertyNode *lat_node;
    SGPropertyNode *alt_node;
    SGPropertyNode *bus_power;

    // inputs
    SGPropertyNode *power_btn;
    SGPropertyNode *nav_freq;       // primary freq
    SGPropertyNode *nav_alt_freq;   // standby freq
    SGPropertyNode *nav_sel_radial; // selected radial
    SGPropertyNode *nav_vol_btn;
    SGPropertyNode *nav_ident_btn;
    SGPropertyNode *audio_btn;

    // outputs
    SGPropertyNode *fmt_freq;     // formated frequency
    SGPropertyNode *fmt_alt_freq; // formated alternate frequency
    SGPropertyNode *nav_heading;  // true heading to nav station
    SGPropertyNode *nav_radial;   // current radial we are on (taking
                                  // into consideration the vor station
                                  // alignment which likely doesn't
                                  // match the magnetic alignment
                                  // exactly.)
    SGPropertyNode *reciprocal_radial;
                                  // nav_radial + 180 (convenience value)
    SGPropertyNode *nav_target_radial_true;
                                  // true heading of selected radial
    SGPropertyNode *nav_target_auto_hdg;
    SGPropertyNode *nav_to_flag;
    SGPropertyNode *nav_from_flag;
    SGPropertyNode *nav_inrange;
    SGPropertyNode *nav_cdi_deflection;
    SGPropertyNode *nav_cdi_xtrack_error;
    SGPropertyNode *nav_has_gs;
    SGPropertyNode *nav_loc;
    SGPropertyNode *nav_loc_dist;
    SGPropertyNode *nav_gs_deflection;
    SGPropertyNode *nav_gs_rate_of_climb;
    SGPropertyNode *nav_gs_dist;
    SGPropertyNode *nav_id;
    SGPropertyNode *nav_id_c1;
    SGPropertyNode *nav_id_c2;
    SGPropertyNode *nav_id_c3;
    SGPropertyNode *nav_id_c4;

    // unfiled
    SGPropertyNode *nav_serviceable;
    SGPropertyNode *cdi_serviceable, *gs_serviceable, *tofrom_serviceable;
    SGPropertyNode *nav_slaved_to_gps;
    SGPropertyNode *gps_cdi_deflection, *gps_to_flag, *gps_from_flag;

    string last_nav_id;
    bool last_nav_vor;
    int nav_play_count;
    time_t nav_last_time;

    int index;                  // used for property binding
    string nav_fx_name;
    string dme_fx_name;

    // internal (unexported) values
    string nav_trans_ident;
    bool nav_valid;
    bool nav_has_dme;
    double nav_target_radial;
    double nav_loclon;
    double nav_loclat;
    double nav_x;
    double nav_y;
    double nav_z;
    double nav_gslon;
    double nav_gslat;
    double nav_elev;            // use gs elev if available
    double nav_gs_x;
    double nav_gs_y;
    double nav_gs_z;
    sgdVec3 gs_base_vec;
    double nav_gs_dist_signed;
    SGTimeStamp prev_time;
    SGTimeStamp curr_time;
    double nav_range;
    double nav_effective_range;
    double nav_target_gs;
    double nav_twist;
    double horiz_vel;
    double last_x;

    string name;
    int num;

    // internal periodic station search timer
    double _time_before_search_sec;

    // model standard VOR/DME/TACAN service volumes as per AIM 1-1-8
    double adjustNavRange( double stationElev, double aircraftElev,
			   double nominalRange );

    // model standard ILS service volumes as per AIM 1-1-9
    double adjustILSRange( double stationElev, double aircraftElev,
			   double offsetDegrees, double distance );

public:

    FGNavRadio(SGPropertyNode *node);
    ~FGNavRadio();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Update nav/adf radios based on current postition
    void search ();
/*
    inline void set_bind_index( int i ) {
        index = i;
        sprintf( nav_fx_name, "nav%d-vor-ident", index );
        sprintf( dme_fx_name, "dme%d-vor-ident", index );
    }
*/

    // NavCom Accessors
    inline bool has_power() const {
        return power_btn->getBoolValue() && (bus_power->getDoubleValue() > 1.0);
    }

    // NAV Accessors
    inline double get_nav_target_radial() const { return nav_target_radial; }

    // Calculated values.
    bool get_nav_to_flag () const;
    inline bool get_nav_has_dme() const { return nav_has_dme; }
    inline bool get_nav_dme_inrange () const {
	return nav_inrange->getBoolValue() && nav_has_dme;
    }
    inline double get_nav_loclon() const { return nav_loclon; }
    inline double get_nav_loclat() const { return nav_loclat; }
    inline double get_nav_gslon() const { return nav_gslon; }
    inline double get_nav_gslat() const { return nav_gslat; }
    inline double get_nav_gs_dist_signed() const { return nav_gs_dist_signed; }
    inline double get_nav_elev() const { return nav_elev; }
    inline double get_nav_target_gs() const { return nav_target_gs; }
    inline double get_nav_twist() const { return nav_twist; }
    //inline const char * get_nav_id() const { return nav_id.c_str(); }
    //inline int get_nav_id_c1() const { return nav_id.c_str()[0]; }
    //inline int get_nav_id_c2() const { return nav_id.c_str()[1]; }
    //inline int get_nav_id_c3() const { return nav_id.c_str()[2]; }
    //inline int get_nav_id_c4() const { return nav_id.c_str()[3]; }
};


#endif // _FG_NAVRADIO_HXX
