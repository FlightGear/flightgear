// atc610x.hxx -- FGFS interface to ATC 610x hardware
//
// Written by Curtis Olson, started January 2002.
//
// Copyright (C) 2002  Curtis L. Olson - curt@flightgear.org
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


#ifndef _FG_ATC610X_HXX
#define _FG_ATC610X_HXX

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/netChat.h>

#include <Main/fg_props.hxx>

#include "protocol.hxx"


#define ATC_ANAL_IN_VALUES 32
#define ATC_ANAL_IN_BYTES (2 * ATC_ANAL_IN_VALUES)
#define ATC_COMPASS_CH 5
#define ATC_STEPPER_HOME 0xC0
#define ATC_RADIO_DISPLAY_BYTES 48
#define ATC_RADIO_SWITCH_BYTES 32
#define ATC_SWITCH_BYTES 16
#define ATC_NUM_COLS 8


class FGATC610x : public FGProtocol {

    int board;

    int lock_fd;
    int analog_in_fd;
    int lamps_fd;
    int radios_fd;
    int stepper_fd;
    int switches_fd;

    char lock_file[256];
    char analog_in_file[256];
    char lamps_file[256];
    char radios_file[256];
    char stepper_file[256];
    char switches_file[256];

    unsigned char analog_in_bytes[ATC_ANAL_IN_BYTES];
    int analog_in_data[ATC_ANAL_IN_VALUES];
    unsigned char radio_display_data[ATC_RADIO_DISPLAY_BYTES];
    unsigned char radio_switch_data[ATC_RADIO_SWITCH_BYTES];
    unsigned char switch_data[ATC_SWITCH_BYTES];

    float compass_position;
    SGPropertyNode *mag_compass;
    SGPropertyNode *dme_min, *dme_kt, *dme_nm;
    SGPropertyNode *com1_freq, *com1_stby_freq;
    SGPropertyNode *com2_freq, *com2_stby_freq;
    SGPropertyNode *nav1_freq, *nav1_stby_freq;
    SGPropertyNode *nav2_freq, *nav2_stby_freq;
    SGPropertyNode *adf_on_off_vol;
    SGPropertyNode *adf_freq, *adf_stby_freq, *adf_stby_mode, *adf_timer_mode;
    SGPropertyNode *adf_count_mode, *adf_flight_timer, *adf_elapsed_timer;
    SGPropertyNode *inner, *middle, *outer;

    int dme_switch;

    bool do_analog_in();
    bool do_lights();
    bool do_radio_switches();
    bool do_radio_display();
    bool do_steppers();
    bool do_switches();

public:

    FGATC610x() { }

    ~FGATC610x() { }

    bool open();

    bool process();

    bool close();
};


#endif // _FG_ATC610X_HXX
