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
    SGPropertyNode *nav_serviceable;
    SGPropertyNode *cdi_serviceable, *gs_serviceable, *tofrom_serviceable;

    string last_nav_id;
    bool last_nav_vor;
    int nav_play_count;
    time_t nav_last_time;

    int index;                  // used for property binding
    string nav_fx_name;
    string dme_fx_name;

    bool need_update;

    bool power_btn;
    bool audio_btn;

    string nav_id;
    string nav_trans_ident;
    bool nav_valid;
    bool nav_inrange;
    bool nav_has_dme;
    bool nav_has_gs;
    bool nav_loc;
    double nav_freq;
    double nav_alt_freq;
    double nav_heading;         // true heading to nav station
    double nav_radial;          // current radial we are on (taking
                                // into consideration the vor station
                                // alignment which likely doesn't
                                // match the magnetic alignment
                                // exactly.)
    double nav_sel_radial;
    double nav_target_radial;
    double nav_target_radial_true;
    double nav_target_auto_hdg;
    double nav_loclon;
    double nav_loclat;
    double nav_x;
    double nav_y;
    double nav_z;
    double nav_loc_dist;
    double nav_gslon;
    double nav_gslat;
    double nav_elev;            // use gs elev if available
    double nav_gs_x;
    double nav_gs_y;
    double nav_gs_z;
    sgdVec3 gs_base_vec;
    double nav_gs_dist;
    double nav_gs_dist_signed;
    double nav_gs_rate_of_climb;
    SGTimeStamp prev_time;
    SGTimeStamp curr_time;
    double nav_range;
    double nav_effective_range;
    double nav_target_gs;
    double nav_twist;
    double nav_vol_btn;
    bool nav_ident_btn;
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

    // NavCom Setters
    inline void set_power_btn( bool val ) { power_btn = val; }
    inline void set_audio_btn( bool val ) { audio_btn = val; }

    // NAV Setters
    inline void set_nav_freq( double freq ) {
	nav_freq = freq; need_update = true;
    }
    inline void set_nav_alt_freq( double freq ) { nav_alt_freq = freq; }
    inline void set_nav_sel_radial( double radial ) {
	nav_sel_radial = radial; need_update = true;
    }
    inline void set_nav_vol_btn( double val ) {
	if ( val < 0.0 ) val = 0.0;
	if ( val > 1.0 ) val = 1.0;
	nav_vol_btn = val;
    }
    inline void set_nav_ident_btn( bool val ) { nav_ident_btn = val; }

    // NavCom Accessors
    inline bool has_power() const {
        return power_btn && (bus_power->getDoubleValue() > 1.0);
    }
    inline bool get_power_btn() const { return power_btn; }
    inline bool get_audio_btn() const { return audio_btn; }

   // NAV Accessors
    inline double get_nav_freq () const { return nav_freq; }
    inline double get_nav_alt_freq () const { return nav_alt_freq; }
    inline double get_nav_sel_radial() const { return nav_sel_radial; }
    inline double get_nav_target_radial() const { return nav_target_radial; }
    inline double get_nav_target_radial_true() const {
        return nav_target_radial_true;
    }
    inline double get_nav_target_auto_hdg() const {
        return nav_target_auto_hdg;
    }

    // Calculated values.
    inline bool get_nav_inrange() const { return nav_inrange; }
    bool get_nav_to_flag () const;
    bool get_nav_from_flag () const;
    inline bool get_nav_has_dme() const { return nav_has_dme; }
    inline bool get_nav_dme_inrange () const {
	return nav_inrange && nav_has_dme;
    }
    inline bool get_nav_has_gs() const { return nav_has_gs; }
    inline bool get_nav_loc() const { return nav_loc; }
    inline double get_nav_loclon() const { return nav_loclon; }
    inline double get_nav_loclat() const { return nav_loclat; }
    inline double get_nav_loc_dist() const { return nav_loc_dist; }
    inline double get_nav_gslon() const { return nav_gslon; }
    inline double get_nav_gslat() const { return nav_gslat; }
    inline double get_nav_gs_dist() const { return nav_gs_dist; }
    inline double get_nav_gs_dist_signed() const { return nav_gs_dist_signed; }
    inline double get_nav_gs_rate_of_climb() const {
        return nav_gs_rate_of_climb;
    }
    inline double get_nav_elev() const { return nav_elev; }
    double get_nav_heading() const;
    double get_nav_radial() const;
    double get_nav_reciprocal_radial() const;
    inline double get_nav_target_gs() const { return nav_target_gs; }
    inline double get_nav_twist() const { return nav_twist; }
    double get_nav_cdi_deflection() const;
    double get_nav_cdi_xtrack_error() const;
    double get_nav_gs_deflection() const;
    inline double get_nav_vol_btn() const { return nav_vol_btn; }
    inline bool get_nav_ident_btn() const { return nav_ident_btn; }
    inline const char * get_nav_id() const { return nav_id.c_str(); }
    inline int get_nav_id_c1() const { return nav_id.c_str()[0]; }
    inline int get_nav_id_c2() const { return nav_id.c_str()[1]; }
    inline int get_nav_id_c3() const { return nav_id.c_str()[2]; }
    inline int get_nav_id_c4() const { return nav_id.c_str()[3]; }
};


#endif // _FG_NAVRADIO_HXX
