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
#include "navcom.hxx"

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
    SGPropertyNode *dme_bus_power;

    bool need_update;

    bool dme_valid;
    int dme_switch_pos;
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
    FGNavCom navcom1;
    FGNavCom navcom2;

public:

    FGRadioStack();
    ~FGRadioStack();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Update nav/adf radios based on current postition
    void search ();

    inline FGNavCom *get_navcom1() { return &navcom1; }
    inline FGNavCom *get_navcom2() { return &navcom2; }

    // DME Setters
    inline void set_dme_freq (double freq) {
        dme_freq = freq; need_update = true;
    }


    // DME Accessors
    inline bool dme_has_power() const {
        return (dme_switch_pos == 1 || dme_switch_pos == 3)
            && (dme_bus_power->getDoubleValue() > 1.0);
    }
    inline double get_dme_freq () const { return dme_freq; }

    // Marker Beacon Accessors
    inline bool get_inner_blink () const { return inner_blink; }
    inline bool get_middle_blink () const { return middle_blink; }
    inline bool get_outer_blink () const { return outer_blink; }

    // Calculated values.
    inline bool get_dme_inrange () const { return dme_inrange; }
    inline double get_dme_dist () const { return dme_dist; }
    inline double get_dme_spd () const { return dme_spd; }
    inline double get_dme_ete () const { return dme_ete; }
};


extern FGRadioStack *current_radiostack;

#endif // _FG_RADIOSTACK_HXX
