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

#include "ATC-Inputs.hxx"


#define ATC_COMPASS_CH 5
#define ATC_STEPPER_HOME 0xC0
#define ATC_RADIO_DISPLAY_BYTES 48


class FGATC610x : public FGProtocol {

    FGATCInput *input0;         // board0 input interface class
    FGATCInput *input1;         // board1 input interface class

    SGPath input0_path;
    SGPath input1_path;
    SGPath output0_path;
    SGPath output1_path;

    int board;

    int lock_fd;
    int lamps_fd;
    int radios_fd;
    int stepper_fd;

    char lock_file[256];
    char lamps_file[256];
    char radios_file[256];
    char stepper_file[256];

    unsigned char radio_display_data[ATC_RADIO_DISPLAY_BYTES];
    unsigned char radio_switch_data[ATC_RADIO_SWITCH_BYTES];

    float compass_position;

    // Electrical system state
    SGPropertyNode *adf_bus_power, *dme_bus_power, *xpdr_bus_power;
    SGPropertyNode *navcom1_bus_power, *navcom2_bus_power;

    // Property tree variables
    SGPropertyNode *mag_compass;
    SGPropertyNode *dme_min, *dme_kt, *dme_nm;
    SGPropertyNode *dme_in_range;
    SGPropertyNode *navcom1_power_btn, *navcom2_power_btn;
    SGPropertyNode *com1_freq, *com1_stby_freq;
    SGPropertyNode *com2_freq, *com2_stby_freq;
    SGPropertyNode *nav1_freq, *nav1_stby_freq, *nav1_obs;
    SGPropertyNode *nav2_freq, *nav2_stby_freq, *nav2_obs;
    SGPropertyNode *adf_adf_btn, *adf_bfo_btn;
    SGPropertyNode *adf_power_btn, *adf_vol;
    SGPropertyNode *adf_freq, *adf_stby_freq;
    SGPropertyNode *adf_stby_mode, *adf_timer_mode;
    SGPropertyNode *adf_count_mode, *adf_flight_timer, *adf_elapsed_timer;
    SGPropertyNode *adf_ant_ann, *adf_adf_ann, *adf_bfo_ann, *adf_frq_ann;
    SGPropertyNode *adf_flt_ann, *adf_et_ann, *adf_hdg, *hdg_bug;
    SGPropertyNode *inner, *middle, *outer;
    SGPropertyNode *xpdr_ident_btn;
    SGPropertyNode *xpdr_digit1, *xpdr_digit2, *xpdr_digit3, *xpdr_digit4;
    SGPropertyNode *xpdr_func_knob, *xpdr_id_code, *xpdr_flight_level;
    SGPropertyNode *xpdr_fl_ann, *xpdr_alt_ann, *xpdr_gnd_ann, *xpdr_on_ann;
    SGPropertyNode *xpdr_sby_ann, *xpdr_reply_ann;
    SGPropertyNode *ati_bird, *alt_press;

    // Faults
    SGPropertyNode *comm1_serviceable, *comm2_serviceable;
    SGPropertyNode *nav1_serviceable, *nav2_serviceable;
    SGPropertyNode *adf_serviceable, *xpdr_serviceable, *dme_serviceable;

    // Configuration values
    SGPropertyNode *elevator_center, *elevator_min, *elevator_max;
    SGPropertyNode *ailerons_center, *ailerons_min, *ailerons_max;
    SGPropertyNode *rudder_center, *rudder_min, *rudder_max;
    SGPropertyNode *brake_left_min, *brake_left_max;
    SGPropertyNode *brake_right_min, *brake_right_max;
    SGPropertyNode *throttle_min, *throttle_max;
    SGPropertyNode *mixture_min, *mixture_max;
    SGPropertyNode *trim_center, *trim_min, *trim_max;
    SGPropertyNode *nav1vol_min, *nav1vol_max;
    SGPropertyNode *nav2vol_min, *nav2vol_max;

    // raw switch positions
    SGPropertyNode *dme_selector;
    SGPropertyNode *ignore_flight_controls;

    int dme_switch;

    bool do_lights();
    bool do_radio_display();
    bool do_steppers();

    // convenience
    inline bool adf_has_power() const {
        return (adf_bus_power->getDoubleValue() > 1.0)
            && adf_power_btn->getBoolValue();
    }
    inline bool dme_has_power() const {
        return (dme_bus_power->getDoubleValue() > 1.0)
            && dme_switch;
    }
    inline bool navcom1_has_power() const {
        return (navcom1_bus_power->getDoubleValue() > 1.0)
            && navcom1_power_btn->getBoolValue();
    }
    inline bool navcom2_has_power() const {
        return (navcom2_bus_power->getDoubleValue() > 1.0)
            && navcom2_power_btn->getBoolValue();
    }
    inline bool xpdr_has_power() const {
        return (xpdr_bus_power->getDoubleValue() > 1.0)
            && (xpdr_func_knob->getIntValue() > 0);
    }

public:

    FGATC610x() :
        input0(NULL),
        input1(NULL),
        input0_path(""),
        input1_path(""),
        output0_path(""),
        output1_path("")
    { }

    ~FGATC610x() {
        delete input0;
        delete input1;
    }

    // Open and initialize ATC 610x hardware
    bool open();

    void init_config();

    bool process();

    bool close();

    inline void set_path_names( const SGPath &in0, const SGPath &in1,
                                const SGPath &out0, const SGPath &out1 )
    {
        input0_path = in0;
        input1_path = in1;
        output0_path = out0;
        output1_path = out1;
    }
};


#endif // _FG_ATC610X_HXX
