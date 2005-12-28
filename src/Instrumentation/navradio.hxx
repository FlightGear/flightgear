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
    SGPropertyNode *bus_power_node;

    // property inputs
    SGPropertyNode *power_btn_node;
    SGPropertyNode *freq_node;       // primary freq
    SGPropertyNode *alt_freq_node;   // standby freq
    SGPropertyNode *sel_radial_node; // selected radial
    SGPropertyNode *vol_btn_node;
    SGPropertyNode *ident_btn_node;
    SGPropertyNode *audio_btn_node;
    SGPropertyNode *nav_serviceable_node;
    SGPropertyNode *cdi_serviceable_node;
    SGPropertyNode *gs_serviceable_node;
    SGPropertyNode *tofrom_serviceable_node;

    // property outputs
    SGPropertyNode *fmt_freq_node;     // formated frequency
    SGPropertyNode *fmt_alt_freq_node; // formated alternate frequency
    SGPropertyNode *heading_node;      // true heading to nav station
    SGPropertyNode *radial_node;       // current radial we are on (taking
                                       // into consideration the vor station
                                       // alignment which likely doesn't
                                       // match the magnetic alignment
                                       // exactly.)
    SGPropertyNode *recip_radial_node; // radial_node(val) + 180 (for
                                       // convenience)
    SGPropertyNode *target_radial_true_node;
                                       // true heading of selected radial
    SGPropertyNode *target_auto_hdg_node;
    SGPropertyNode *to_flag_node;
    SGPropertyNode *from_flag_node;
    SGPropertyNode *inrange_node;
    SGPropertyNode *cdi_deflection_node;
    SGPropertyNode *cdi_xtrack_error_node;
    SGPropertyNode *has_gs_node;
    SGPropertyNode *loc_node;
    SGPropertyNode *loc_dist_node;
    SGPropertyNode *gs_deflection_node;
    SGPropertyNode *gs_rate_of_climb_node;
    SGPropertyNode *gs_dist_node;
    SGPropertyNode *nav_id_node;
    SGPropertyNode *id_c1_node;
    SGPropertyNode *id_c2_node;
    SGPropertyNode *id_c3_node;
    SGPropertyNode *id_c4_node;

    // gps slaving support
    SGPropertyNode *nav_slaved_to_gps_node;
    SGPropertyNode *gps_cdi_deflection_node;
    SGPropertyNode *gps_to_flag_node;
    SGPropertyNode *gps_from_flag_node;

    // internal (private) values

    string last_nav_id;
    bool last_nav_vor;
    int play_count;
    time_t last_time;

    int index;                  // used for property binding
    string nav_fx_name;
    string dme_fx_name;

    string trans_ident;
    bool is_valid;
    bool has_dme;
    double radial;
    double target_radial;
    double loc_lon;
    double loc_lat;
    double nav_x;
    double nav_y;
    double nav_z;
    double gs_lon;
    double gs_lat;
    double nav_elev;            // use gs elev if available
    double gs_x;
    double gs_y;
    double gs_z;
    sgdVec3 gs_base_vec;
    double gs_dist_signed;
    SGTimeStamp prev_time;
    SGTimeStamp curr_time;
    double range;
    double effective_range;
    double target_gs;
    double twist;
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

    // NavCom Accessors
    inline bool has_power() const {
        return power_btn_node->getBoolValue()
            && (bus_power_node->getDoubleValue() > 1.0);
    }
};


#endif // _FG_NAVRADIO_HXX
