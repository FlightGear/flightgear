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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>	// snprintf

#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>

#include <Aircraft/aircraft.hxx>

#include "kt_70.hxx"


// Constructor
FGKT_70::FGKT_70() :
    lon_node(fgGetNode("/position/longitude-deg", true)),
    lat_node(fgGetNode("/position/latitude-deg", true)),
    alt_node(fgGetNode("/position/altitude-ft", true)),
    bus_power(fgGetNode("/systems/electrical/outputs/transponder", true)),
    serviceable_node(fgGetNode("/radios/kt-70/inputs/serviceable", true)),
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
    reply_ann(0)
{
}


// Destructor
FGKT_70::~FGKT_70() { }


void FGKT_70::init () {
}


void FGKT_70::bind () {
    // internal values

    // modes

    // input and buttons
    fgTie("/radios/kt-70/inputs/ident-btn", this,
	  &FGKT_70::get_ident_btn, &FGKT_70::set_ident_btn);
    fgSetArchivable("/radios/kt-70/inputs/rotation-deg");
    fgTie("/radios/kt-70/inputs/digit1", this,
	  &FGKT_70::get_digit1, &FGKT_70::set_digit1);
    fgSetArchivable("/radios/kt-70/inputs/digit1");
    fgTie("/radios/kt-70/inputs/digit2", this,
	  &FGKT_70::get_digit2, &FGKT_70::set_digit2);
    fgSetArchivable("/radios/kt-70/inputs/digit2");
    fgTie("/radios/kt-70/inputs/digit3", this,
	  &FGKT_70::get_digit3, &FGKT_70::set_digit3);
    fgSetArchivable("/radios/kt-70/inputs/digit3");
    fgTie("/radios/kt-70/inputs/digit4", this,
	  &FGKT_70::get_digit4, &FGKT_70::set_digit4);
    fgSetArchivable("/radios/kt-70/inputs/digit4");
    fgTie("/radios/kt-70/inputs/func-knob", this,
	  &FGKT_70::get_func_knob, &FGKT_70::set_func_knob);
    fgSetArchivable("/radios/kt-70/inputs/func-knob");

    // outputs
    fgTie("/radios/kt-70/outputs/id-code", this,
	  &FGKT_70::get_id_code, &FGKT_70::set_id_code);
    fgSetArchivable("/radios/kt-70/outputs/id-code");
    fgTie("/radios/kt-70/outputs/flight-level", this,
          &FGKT_70::get_flight_level);

    // annunciators
    fgTie("/radios/kt-70/annunciators/fl", this, &FGKT_70::get_fl_ann );
    fgTie("/radios/kt-70/annunciators/alt", this, &FGKT_70::get_alt_ann );
    fgTie("/radios/kt-70/annunciators/gnd", this, &FGKT_70::get_gnd_ann );
    fgTie("/radios/kt-70/annunciators/on", this, &FGKT_70::get_on_ann );
    fgTie("/radios/kt-70/annunciators/sby", this, &FGKT_70::get_sby_ann );
    fgTie("/radios/kt-70/annunciators/reply", this, &FGKT_70::get_reply_ann );
}


void FGKT_70::unbind () {
    // internal values

    // modes

    // input and buttons
    fgUntie("/radios/kt-70/inputs/ident-btn");
    fgUntie("/radios/kt-70/inputs/digit1");
    fgUntie("/radios/kt-70/inputs/digit2");
    fgUntie("/radios/kt-70/inputs/digit3");
    fgUntie("/radios/kt-70/inputs/digit4");
    fgUntie("/radios/kt-70/inputs/func-knob");

    // outputs
    fgUntie("/radios/kt-70/outputs/id-code");
    fgUntie("/radios/kt-70/outputs/flight-level");

    // annunciators
    fgUntie("/radios/kt-70/annunciators/fl");
    fgUntie("/radios/kt-70/annunciators/alt");
    fgUntie("/radios/kt-70/annunciators/gnd");
    fgUntie("/radios/kt-70/annunciators/on");
    fgUntie("/radios/kt-70/annunciators/sby");
    fgUntie("/radios/kt-70/annunciators/reply");
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
        } else if ( func_knob == 2 ) {
            fl_ann = true;
            alt_ann = true;
            gnd_ann = true;
            on_ann = true;
            sby_ann = true;
            reply_ann = true;
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
