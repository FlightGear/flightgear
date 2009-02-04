// ATC-Outputs.hxx -- Translate FGFS properties into ATC hardware outputs
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


#ifndef _FG_ATC_OUTPUTS_HXX
#define _FG_ATC_OUTPUTS_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/fg_props.hxx>

#define ATC_RADIO_DISPLAY_BYTES 48
#define ATC_ANALOG_OUT_CHANNELS 48
#define ATC_COMPASS_CH 5
#define ATC_STEPPER_HOME 0xC0


class FGATCOutput {

    int is_open;

    int board;
    SGPath config;

    int lock_fd;
    int analog_out_fd;
    int lamps_fd;
    int radio_display_fd;
    int stepper_fd;

    char analog_out_file[256];
    char lamps_file[256];
    char radio_display_file[256];
    char stepper_file[256];

    unsigned char analog_out_data[ATC_ANALOG_OUT_CHANNELS*2];
    unsigned char radio_display_data[ATC_RADIO_DISPLAY_BYTES];

    SGPropertyNode_ptr analog_out_node;
    SGPropertyNode_ptr lamps_out_node;
    SGPropertyNode_ptr radio_display_node;
    SGPropertyNode_ptr steppers_node;

    void init_config();
    bool do_analog_out();
    bool do_lamps();
    bool do_radio_display();
    bool do_steppers();

    // hardwired stepper motor code
    float compass_position;

public:

    // Constructor: The _board parameter specifies which board to
    // reference.  Possible values are 0 or 1.  The _config_file
    // parameter specifies the location of the output config file (xml)
    FGATCOutput( const int _board, const SGPath &_config_file );

    // Destructor
    ~FGATCOutput() { }

    // need to pass in atc hardware lock_fd so that the radios and
    // lamps can be blanked and so that the compass can be homed.
    bool open( int lock_fd );

    // process the hardware outputs.  This code assumes the calling
    // layer will lock the hardware.
    bool process();

    bool close();
};


#endif // _FG_ATC_OUTPUTS_HXX
