// dme.hxx -- class to manage an instance of the DME
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


#ifndef _FG_DME_HXX
#define _FG_DME_HXX


#include <Main/fg_props.hxx>

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
// #include <simgear/math/interpolater.hxx>
#include <simgear/timing/timestamp.hxx>

// #include <Navaids/ilslist.hxx>
#include <Navaids/navlist.hxx>
// #include <Sound/beacon.hxx>
#include <Sound/morse.hxx>

// #include "kr_87.hxx"            // ADF
// #include "kt_70.hxx"            // Transponder
// #include "navcom.hxx"

class FGDME : public SGSubsystem
{
    SGPropertyNode *lon_node;
    SGPropertyNode *lat_node;
    SGPropertyNode *alt_node;
    SGPropertyNode *bus_power;
    SGPropertyNode *navcom1_bus_power, *navcom2_bus_power;
    SGPropertyNode *navcom1_power_btn, *navcom2_power_btn;
    SGPropertyNode *navcom1_freq, *navcom2_freq;

    bool need_update;

    bool valid;
    int switch_pos;
    bool inrange;
    double freq;
    double lon;
    double lat;
    double elev;
    double range;
    double effective_range;
    double x;
    double y;
    double z;
    double bias;
    double dist;
    double prev_dist;
    double spd;
    double ete;
    SGTimeStamp last_time;

public:

    FGDME();
    ~FGDME();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Update dme based on current postition
    void search ();

    // DME Setters
    inline void set_freq (double freq) {
        freq = freq; need_update = true;
    }


    // DME Accessors
    inline bool has_power() const {
        return (switch_pos > 0)
            && (bus_power->getDoubleValue() > 1.0);
    }
    inline bool navcom1_on() const {
        return (navcom1_bus_power->getDoubleValue() > 1.0)
            && navcom1_power_btn->getBoolValue();
    }
    inline bool navcom2_on() const {
        return (navcom2_bus_power->getDoubleValue() > 1.0)
            && navcom2_power_btn->getBoolValue();
    }
    inline double get_freq () const { return freq; }

    // Calculated values.
    inline bool get_inrange () const { return inrange; }
    inline double get_dist () const { return dist; }
    inline double get_spd () const { return spd; }
    inline double get_ete () const { return ete; }
};


#endif // _FG_DME_HXX
