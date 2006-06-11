// ATC-Inputs.hxx -- Translate ATC hardware inputs to FGFS properties
//
// Written by Curtis Olson, started November 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_ATC_INPUTS_HXX
#define _FG_ATC_INPUTS_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/fg_props.hxx>

#define ATC_ANAL_IN_VALUES 32
#define ATC_ANAL_IN_BYTES (2 * ATC_ANAL_IN_VALUES)
#define ATC_RADIO_SWITCH_BYTES 32
#define ATC_SWITCH_BYTES 16
#define ATC_NUM_COLS 8


class FGATCInput {

    int is_open;

    int board;
    SGPath config;

    int analog_in_fd;
    int radios_fd;
    int switches_fd;

    char analog_in_file[256];
    char radios_file[256];
    char switches_file[256];

    unsigned char analog_in_bytes[ATC_ANAL_IN_BYTES];
    int analog_in_data[ATC_ANAL_IN_VALUES];
    unsigned char radio_switch_data[ATC_RADIO_SWITCH_BYTES];
    unsigned char switch_data[ATC_SWITCH_BYTES];

    SGPropertyNode_ptr ignore_flight_controls;
    SGPropertyNode_ptr ignore_pedal_controls;

    SGPropertyNode_ptr analog_in_node;
    SGPropertyNode_ptr radio_in_node;
    SGPropertyNode_ptr switches_node;

    void init_config();
    bool do_analog_in();
    bool do_radio_switches();
    bool do_switches();

public:

    // Constructor: The _board parameter specifies which board to
    // reference.  Possible values are 0 or 1.  The _config_file
    // parameter specifies the location of the input config file (xml)
    FGATCInput( const int _board, const SGPath &_config_file );

    // Destructor
    ~FGATCInput() { }

    bool open();

    // process the hardware inputs.  This code assumes the calling
    // layer will lock the hardware.
    bool process();

    bool close();
};


#endif // _FG_ATC_INPUTS_HXX
