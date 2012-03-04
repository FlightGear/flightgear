// kt-70.hxx -- class to impliment the King KT 70 panel-m transponder
//
// Written by Curtis Olson, started July 2002.
//
// Copyright (C) 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_KT_70_HXX
#define _FG_KT_70_HXX


#include <Main/fg_props.hxx>

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Navaids/navlist.hxx>
#include <Sound/beacon.hxx>
#include <Sound/morse.hxx>


class FGKT_70 : public SGSubsystem
{
private:
    SGPropertyNode_ptr lon_node;
    SGPropertyNode_ptr lat_node;
    SGPropertyNode_ptr alt_node;
    SGPropertyNode_ptr bus_power;
    SGPropertyNode_ptr serviceable_node;

    // internal values
    double r_flash_time;

    // modes
    bool ident_mode;             // false = normal, true = ident/squawk

    // input and buttons
    bool ident_btn;
    bool last_ident_btn;
    int digit1, digit2, digit3, digit4;
    int func_knob;              // 0 = OFF, 1 = SBY, 2 = TST, 3 = GND, 4 = ON,
                                // 5 = ALT

    // outputs
    int id_code;
    int flight_level;

    // annunciators
    bool fl_ann;                // flight level
    bool alt_ann;               // altitude
    bool gnd_ann;               // ground
    bool on_ann;                // on
    bool sby_ann;               // standby
    bool reply_ann;             // reply

    string name;
    int num;
    simgear::TiedPropertyList _tiedProperties;

public:

    FGKT_70(SGPropertyNode *node);
    ~FGKT_70();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);
    void search () { /* empty placeholder */ }

    // internal values
    inline bool has_power() const {
        return (func_knob > 0) && (bus_power->getDoubleValue() > 1.0);
    }

    // input and buttons
    inline bool get_ident_btn() const { return ident_btn; }
    inline void set_ident_btn( bool val ) { ident_btn = val; }
    inline int get_digit1() const { return digit1; }
    inline void set_digit1( int val ) { digit1 = val; }
    inline int get_digit2() const { return digit2; }
    inline void set_digit2( int val ) { digit2 = val; }
    inline int get_digit3() const { return digit3; }
    inline void set_digit3( int val ) { digit3 = val; }
    inline int get_digit4() const { return digit4; }
    inline void set_digit4( int val ) { digit4 = val; }
    inline int get_func_knob() const { return func_knob; }
    inline void set_func_knob( int val ) { func_knob = val; }

    // outputs
    inline int get_id_code () const { return id_code; }
    inline void set_id_code( int c ) { id_code = c; }
    inline int get_flight_level () const { return flight_level; }

    // annunciators
    inline bool get_fl_ann() const { return fl_ann; }
    inline bool get_alt_ann() const { return alt_ann; }
    inline bool get_gnd_ann() const { return gnd_ann; }
    inline bool get_on_ann() const { return on_ann; }
    inline bool get_sby_ann() const { return sby_ann; }
    inline bool get_reply_ann() const { return reply_ann; }
};


#endif // _FG_KT_70_HXX
