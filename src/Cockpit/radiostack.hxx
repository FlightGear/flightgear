// radiostack.hxx -- class to manage an instance of the radio stack
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_RADIOSTACK_HXX
#define _FG_RADIOSTACK_HXX


#include <Main/fgfs.hxx>
#include <Main/fg_props.hxx>

#include <simgear/compiler.h>

#include <simgear/math/interpolater.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Navaids/ilslist.hxx>
#include <Navaids/navlist.hxx>
#include <Sound/beacon.hxx>
#include <Sound/morse.hxx>

#include "kr_87.hxx"            // ADF
#include "kt_70.hxx"            // Transponder

class FGRadioStack : public FGSubsystem
{
    FGBeacon beacon;
    FGMorse morse;

    SGInterpTable *term_tbl;
    SGInterpTable *low_tbl;
    SGInterpTable *high_tbl;

    SGPropertyNode *lon_node;
    SGPropertyNode *lat_node;
    SGPropertyNode *alt_node;

    bool need_update;

    string comm1_ident;
    bool comm1_valid;
    bool comm1_inrange;
    double comm1_freq;
    double comm1_alt_freq;
    double comm1_vol_btn;
    bool comm1_ident_btn;
    double comm1_x;
    double comm1_y;
    double comm1_z;
    double comm1_dist;
    double comm1_elev;
    double comm1_range;
    double comm1_effective_range;

    string comm2_ident;
    bool comm2_valid;
    bool comm2_inrange;
    double comm2_freq;
    double comm2_alt_freq;
    double comm2_vol_btn;
    bool comm2_ident_btn;
    double comm2_x;
    double comm2_y;
    double comm2_z;
    double comm2_dist;
    double comm2_elev;
    double comm2_range;
    double comm2_effective_range;

    string nav1_ident;
    string nav1_trans_ident;
    bool nav1_valid;
    bool nav1_inrange;
    bool nav1_has_dme;
    bool nav1_has_gs;
    bool nav1_loc;
    double nav1_freq;
    double nav1_alt_freq;
    double nav1_radial;
    double nav1_sel_radial;
    double nav1_loclon;
    double nav1_loclat;
    double nav1_x;
    double nav1_y;
    double nav1_z;
    double nav1_loc_dist;
    double nav1_gslon;
    double nav1_gslat;
    double nav1_gs_x;
    double nav1_gs_y;
    double nav1_gs_z;
    double nav1_gs_dist;
    SGTimeStamp prev_time;
    SGTimeStamp curr_time;
    double nav1_elev;
    double nav1_range;
    double nav1_effective_range;
    double nav1_heading;
    double nav1_target_gs;
    double nav1_magvar;
    double nav1_vol_btn;
    bool nav1_ident_btn;

    string nav2_ident;
    string nav2_trans_ident;
    bool nav2_valid;
    bool nav2_inrange;
    bool nav2_has_dme;
    bool nav2_has_gs;
    bool nav2_loc;
    double nav2_freq;
    double nav2_alt_freq;
    double nav2_radial;
    double nav2_sel_radial;
    double nav2_loclon;
    double nav2_loclat;
    double nav2_x;
    double nav2_y;
    double nav2_z;
    double nav2_loc_dist;
    double nav2_gslon;
    double nav2_gslat;
    double nav2_gs_x;
    double nav2_gs_y;
    double nav2_gs_z;
    double nav2_gs_dist;
    double nav2_elev;
    double nav2_range;
    double nav2_effective_range;
    double nav2_heading;
    double nav2_target_gs;
    double nav2_magvar;
    double nav2_vol_btn;
    bool nav2_ident_btn;

    bool dme_valid;
    bool dme_inrange;
    double dme_freq;
    double dme_lon;
    double dme_lat;
    double dme_elev;
    double dme_range;
    double dme_effective_range;
    double dme_x;
    double dme_y;
    double dme_z;
    double dme_dist;
    double dme_prev_dist;
    double dme_spd;
    double dme_ete;
    SGTimeStamp dme_last_time;

    bool outer_marker;
    bool middle_marker;
    bool inner_marker;

    SGTimeStamp blink;
    bool outer_blink;
    bool middle_blink;
    bool inner_blink;

    FGKR_87 adf;                // King KR 87 Digital ADF model
    FGKT_70 xponder;            // Bendix/King KT 70 Panel-Mounted Transponder

    // model standard VOR/DME/TACAN service volumes as per AIM 1-1-8
    double adjustNavRange( double stationElev, double aircraftElev,
			   double nominalRange );

    // model standard ILS service volumes as per AIM 1-1-9
    double adjustILSRange( double stationElev, double aircraftElev,
			   double offsetDegrees, double distance );

public:

    FGRadioStack();
    ~FGRadioStack();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Update nav/adf radios based on current postition
    void search ();

    // COMM1 Setters
    inline void set_comm1_freq( double freq ) {
	comm1_freq = freq; need_update = true;
    }
    inline void set_comm1_alt_freq( double freq ) { comm1_alt_freq = freq; }
    inline void set_comm1_vol_btn( double val ) {
	if ( val < 0.0 ) val = 0.0;
	if ( val > 1.0 ) val = 1.0;
	comm1_vol_btn = val;
    }
    inline void set_comm1_ident_btn( bool val ) { comm1_ident_btn = val; }

    // COMM2 Setters
    inline void set_comm2_freq( double freq ) {
	comm2_freq = freq; need_update = true;
    }
    inline void set_comm2_alt_freq( double freq ) { comm2_alt_freq = freq; }
    inline void set_comm2_vol_btn( double val ) {
	if ( val < 0.0 ) val = 0.0;
	if ( val > 1.0 ) val = 1.0;
	comm2_vol_btn = val;
    }
    inline void set_comm2_ident_btn( bool val ) { comm2_ident_btn = val; }

    // NAV1 Setters
    inline void set_nav1_freq( double freq ) {
	nav1_freq = freq; need_update = true;
    }
    inline void set_nav1_alt_freq( double freq ) { nav1_alt_freq = freq; }
    inline void set_nav1_sel_radial( double radial ) {
	nav1_sel_radial = radial; need_update = true;
    }
    inline void set_nav1_vol_btn( double val ) {
	if ( val < 0.0 ) val = 0.0;
	if ( val > 1.0 ) val = 1.0;
	nav1_vol_btn = val;
    }
    inline void set_nav1_ident_btn( bool val ) { nav1_ident_btn = val; }

    // NAV2 Setters
    inline void set_nav2_freq( double freq ) {
	nav2_freq = freq; need_update = true;
    }
    inline void set_nav2_alt_freq( double freq ) { nav2_alt_freq = freq; }
    inline void set_nav2_sel_radial( double radial ) {
	nav2_sel_radial = radial; need_update = true;
    }
    inline void set_nav2_vol_btn( double val ) {
	if ( val < 0.0 ) val = 0.0;
	if ( val > 1.0 ) val = 1.0;
	nav2_vol_btn = val;
    }
    inline void set_nav2_ident_btn( bool val ) { nav2_ident_btn = val; }

    // DME Setters
    inline void set_dme_freq (double freq) {
        dme_freq = freq; need_update = true;
    }

    // COMM1 Accessors
    inline double get_comm1_freq () const { return comm1_freq; }
    inline double get_comm1_alt_freq () const { return comm1_alt_freq; }

    // COMM2 Accessors
    inline double get_comm2_freq () const { return comm2_freq; }
    inline double get_comm2_alt_freq () const { return comm2_alt_freq; }

    // NAV1 Accessors
    inline double get_nav1_freq () const { return nav1_freq; }
    inline double get_nav1_alt_freq () const { return nav1_alt_freq; }
    inline double get_nav1_sel_radial() const { return nav1_sel_radial; }

    // NAV2 Accessors
    inline double get_nav2_freq () const { return nav2_freq; }
    inline double get_nav2_alt_freq () const { return nav2_alt_freq; }
    inline double get_nav2_sel_radial() const { return nav2_sel_radial; }

    // DME Accessors
    inline double get_dme_freq () const { return dme_freq; }

    // Marker Beacon Accessors
    inline bool get_inner_blink () const { return inner_blink; }
    inline bool get_middle_blink () const { return middle_blink; }
    inline bool get_outer_blink () const { return outer_blink; }

    // Calculated values.
    inline bool get_comm1_inrange() const { return comm1_inrange; }
    inline double get_comm1_vol_btn() const { return comm1_vol_btn; }
    inline bool get_comm1_ident_btn() const { return comm1_ident_btn; }

    inline bool get_comm2_inrange() const { return comm2_inrange; }
    inline double get_comm2_vol_btn() const { return comm2_vol_btn; }
    inline bool get_comm2_ident_btn() const { return comm2_ident_btn; }

    inline bool get_nav1_inrange() const { return nav1_inrange; }
    bool get_nav1_to_flag () const;
    bool get_nav1_from_flag () const;
    inline bool get_nav1_has_dme() const { return nav1_has_dme; }
    inline bool get_nav1_dme_inrange () const {
	return nav1_inrange && nav1_has_dme;
    }
    inline bool get_nav1_has_gs() const { return nav1_has_gs; }
    inline bool get_nav1_loc() const { return nav1_loc; }
    inline double get_nav1_loclon() const { return nav1_loclon; }
    inline double get_nav1_loclat() const { return nav1_loclat; }
    inline double get_nav1_loc_dist() const { return nav1_loc_dist; }
    inline double get_nav1_gslon() const { return nav1_gslon; }
    inline double get_nav1_gslat() const { return nav1_gslat; }
    inline double get_nav1_gs_dist() const { return nav1_gs_dist; }
    inline double get_nav1_elev() const { return nav1_elev; }
    inline double get_nav1_heading() const { return nav1_heading; }
    inline double get_nav1_radial() const { return nav1_radial; }
    inline double get_nav1_target_gs() const { return nav1_target_gs; }
    inline double get_nav1_magvar() const { return nav1_magvar; }
    double get_nav1_heading_needle_deflection() const;
    double get_nav1_gs_needle_deflection() const;
    inline double get_nav1_vol_btn() const { return nav1_vol_btn; }
    inline bool get_nav1_ident_btn() const { return nav1_ident_btn; }

    inline bool get_nav2_inrange() const { return nav2_inrange; }
    bool get_nav2_to_flag () const;
    bool get_nav2_from_flag () const;
    inline bool get_nav2_has_dme() const { return nav2_has_dme; }
    inline bool get_nav2_dme_inrange () const {
	return nav2_inrange && nav2_has_dme;
    }
    inline bool get_nav2_has_gs() const { return nav2_has_gs; }
    inline bool get_nav2_loc() const { return nav2_loc; }
    inline double get_nav2_loclon() const { return nav2_loclon; }
    inline double get_nav2_loclat() const { return nav2_loclat; }
    inline double get_nav2_loc_dist() const { return nav2_loc_dist; }
    inline double get_nav2_gslon() const { return nav2_gslon; }
    inline double get_nav2_gslat() const { return nav2_gslat; }
    inline double get_nav2_gs_dist() const { return nav2_gs_dist; }
    inline double get_nav2_elev() const { return nav2_elev; }
    inline double get_nav2_heading() const { return nav2_heading; }
    inline double get_nav2_radial() const { return nav2_radial; }
    inline double get_nav2_target_gs() const { return nav2_target_gs; }
    inline double get_nav2_magvar() const { return nav2_magvar; }
    double get_nav2_heading_needle_deflection() const;
    double get_nav2_gs_needle_deflection() const;
    inline double get_nav2_vol_btn() const { return nav2_vol_btn; }
    inline bool get_nav2_ident_btn() const { return nav2_ident_btn; }

    inline bool get_dme_inrange () const { return dme_inrange; }
    inline double get_dme_dist () const { return dme_dist; }
    inline double get_dme_spd () const { return dme_spd; }
    inline double get_dme_ete () const { return dme_ete; }
};


extern FGRadioStack *current_radiostack;

#endif // _FG_RADIOSTACK_HXX
