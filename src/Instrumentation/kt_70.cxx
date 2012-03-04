// kt-70.cxx -- class to impliment the King KT 70 panel-m transponder
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <iostream>
#include <string>
#include <sstream>

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>

#include "kt_70.hxx"


// Constructor
FGKT_70::FGKT_70(SGPropertyNode *node) :
    r_flash_time(0.0),
    ident_mode(false),
    ident_btn(false),
    last_ident_btn(false),
    digit1(1), digit2(2), digit3(0), digit4(0),
    func_knob(4),
    id_code(1200),
    flight_level(0),
    fl_ann(0),
    alt_ann(0),
    gnd_ann(0),
    on_ann(0),
    sby_ann(0),
    reply_ann(0),
    name("kt-70"),
    num(0)
{
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_INSTR, SG_WARN, 
                    "Error in kt-70 config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }

}


// Destructor
FGKT_70::~FGKT_70() { }


void FGKT_70::init ()
{
    string branch;
    branch = "/instrumentation/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    // Inputs
    lon_node = fgGetNode("/position/longitude-deg", true);
    lat_node = fgGetNode("/position/latitude-deg", true);
    alt_node = fgGetNode("/position/altitude-ft", true);
    bus_power = fgGetNode("/systems/electrical/outputs/transponder", true);
    serviceable_node = (node->getChild("inputs", 0, true))
                        ->getChild("serviceable", 0, true);
    serviceable_node->setBoolValue(true);
}


void FGKT_70::bind ()
{
    std::ostringstream temp;
    string branch;
    temp << num;
    branch = "/instrumentation/" + name + "[" + temp.str() + "]";
    _tiedProperties.setRoot(fgGetNode(branch, true));

    // internal values

    // modes
    // input and buttons
    _tiedProperties.Tie("inputs/ident-btn", this,
                        &FGKT_70::get_ident_btn, &FGKT_70::set_ident_btn);
    fgSetArchivable((branch + "/inputs/rotation-deg").c_str());
    _tiedProperties.Tie("inputs/digit1", this,
                        &FGKT_70::get_digit1, &FGKT_70::set_digit1);
    fgSetArchivable((branch + "/inputs/digit1").c_str());
    _tiedProperties.Tie("inputs/digit2", this,
                        &FGKT_70::get_digit2, &FGKT_70::set_digit2);
    fgSetArchivable((branch + "/inputs/digit2").c_str());
    _tiedProperties.Tie("inputs/digit3", this,
                        &FGKT_70::get_digit3, &FGKT_70::set_digit3);
    fgSetArchivable((branch + "/inputs/digit3").c_str());
    _tiedProperties.Tie("inputs/digit4", this,
                        &FGKT_70::get_digit4, &FGKT_70::set_digit4);
    fgSetArchivable((branch + "/inputs/digit4").c_str());
    _tiedProperties.Tie("inputs/func-knob", this,
                        &FGKT_70::get_func_knob, &FGKT_70::set_func_knob);
    fgSetArchivable((branch + "/inputs/func-knob").c_str());

    // outputs
    _tiedProperties.Tie("outputs/id-code", this,
                        &FGKT_70::get_id_code, &FGKT_70::set_id_code);
    fgSetArchivable((branch + "/outputs/id-code").c_str());
    _tiedProperties.Tie("outputs/flight-level", this,
                        &FGKT_70::get_flight_level);

    // annunciators
    _tiedProperties.Tie("annunciators/fl", this,
                        &FGKT_70::get_fl_ann );
    _tiedProperties.Tie("annunciators/alt", this,
                        &FGKT_70::get_alt_ann );
    _tiedProperties.Tie("annunciators/gnd", this,
                         &FGKT_70::get_gnd_ann );
    _tiedProperties.Tie("annunciators/on", this,
                        &FGKT_70::get_on_ann );
    _tiedProperties.Tie("annunciators/sby", this,
                        &FGKT_70::get_sby_ann );
    _tiedProperties.Tie("annunciators/reply", this,
                        &FGKT_70::get_reply_ann );
}


void FGKT_70::unbind () {
    _tiedProperties.Untie();
}


// Update the various nav values based on position and valid tuned in navs
void FGKT_70::update( double dt ) {
    // start with all annunciators off (reply ann is handled
    // separately) and then turn on the ones we want
    fl_ann = false;
    alt_ann = false;
    gnd_ann = false;
    on_ann = false;
    sby_ann = false;
    reply_ann = false;

    if ( has_power() && serviceable_node->getBoolValue() ) {
        // sanity checks
        if ( digit1 < 0 ) { digit1 = 0; }
        if ( digit1 > 7 ) { digit1 = 7; }
        if ( digit2 < 0 ) { digit2 = 0; }
        if ( digit2 > 7 ) { digit2 = 7; }
        if ( digit3 < 0 ) { digit3 = 0; }
        if ( digit3 > 7 ) { digit3 = 7; }
        if ( digit4 < 0 ) { digit4 = 0; }
        if ( digit4 > 7 ) { digit4 = 7; }

        id_code = digit1 * 1000 + digit2 * 100 + digit3 * 10 + digit4;

        // flight level computation

        // FIXME!!!! This needs to be computed relative to 29.92 inHg,
        // but for the moment, until I figure out how to do that, I'll
        // just use true altitude.
        flight_level = (int)( (alt_node->getDoubleValue() + 50.0) / 100.0);

        // ident button
        if ( ident_btn && !last_ident_btn ) {
            // ident button depressed
            r_flash_time = 0.0;
            ident_mode = true;
        }
        r_flash_time += dt;
        if ( r_flash_time > 18.0 ) {
            ident_mode = false;
        }
    
        if ( ident_mode ) {
            reply_ann = true;
        } else {
            reply_ann = false;
        }

        if ( func_knob == 1 ) {
            sby_ann = true;
        } else if ( func_knob == 2 ) { // selftest
            fl_ann = true;
            alt_ann = true;
            gnd_ann = true;
            on_ann = true;
            sby_ann = true;
            reply_ann = true;
            id_code = 8888;
            flight_level = 888;
        } else if ( func_knob == 3 ) {
            fl_ann = true;
            gnd_ann = true;
        } else if ( func_knob == 4 ) {
            on_ann = true;
        } else if ( func_knob == 5 ) {
            fl_ann = true;
            alt_ann = true;
        }
    }
}
