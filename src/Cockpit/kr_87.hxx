// kr-87.hxx -- class to impliment the King KR 87 Digital ADF
//
// Written by Curtis Olson, started June 2002.
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


#ifndef _FG_KR_87_HXX
#define _FG_KR_87_HXX


#include <Main/fgfs.hxx>
#include <Main/fg_props.hxx>

#include <simgear/compiler.h>

#include <simgear/math/interpolater.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Navaids/navlist.hxx>
#include <Sound/beacon.hxx>
#include <Sound/morse.hxx>


class FGKR_87 : public FGSubsystem
{
    FGBeacon beacon;
    FGMorse morse;

    SGInterpTable *term_tbl;
    SGInterpTable *low_tbl;
    SGInterpTable *high_tbl;

    SGPropertyNode *lon_node;
    SGPropertyNode *lat_node;
    SGPropertyNode *alt_node;

    bool need_update;

    // internal values
    string ident;
    string trans_ident;
    bool valid;
    bool inrange;
    double stn_lon;
    double stn_lat;
    double stn_elev;
    double range;
    double effective_range;
    double dist;
    double heading;
    double x;
    double y;
    double z;
    double goal_needle_deg;
    double et_flash_time;

    // modes
    int ant_mode;               // 0 = ADF mode (needle active), 1 = ANT mode
                                // (needle turned to 90, improved audio rcpt)
    int stby_mode;              // 0 = show stby freq, 1 = show timer
    int timer_mode;             // 0 = flt, 1 = et
    int count_mode;             // 0 = count up, 1 = count down, 2 = set et
                                // count down

    // input and buttons
    double rotation;            // compass faceplace rotation
    double on_off_vol_btn;
    bool adf_btn;               // 0 = normal, 1 = depressed
    bool bfo_btn;               // 0 = normal, 1 = depressed
    bool frq_btn;               // 0 = normal, 1 = depressed
    bool last_frq_btn;
    bool flt_et_btn;            // 0 = normal, 1 = depressed
    bool last_flt_et_btn;
    bool set_rst_btn;           // 0 = normal, 1 = depressed
    bool last_set_rst_btn;      // 0 = normal, 1 = depressed
    bool ident_btn;             // ???

    // outputs
    double freq;
    double stby_freq;
    double needle_deg;
    double flight_timer;
    double elapsed_timer;
    double tmp_timer;

    // annunciators
    bool ant_ann;
    bool adf_ann;
    bool bfo_ann;
    bool frq_ann;
    bool flt_ann;
    bool et_ann;

public:

    FGKR_87();
    ~FGKR_87();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Update nav/adf radios based on current postition
    void search ();

    // internal values
    inline bool get_inrange() const { return inrange; }
    inline double get_stn_lon() const { return stn_lon; }
    inline double get_stn_lat() const { return stn_lat; }
    inline double get_heading() const { return heading; }

    // modes
    inline int get_ant_mode() const { return ant_mode; }
    inline int get_stby_mode() const { return stby_mode; }
    inline int get_timer_mode() const { return timer_mode; }
    inline int get_count_mode() const { return count_mode; }

    // input and buttons
    inline double get_rotation () const { return rotation; }
    inline void set_rotation( double rot ) { rotation = rot; }
    inline double get_on_off_vol_btn() const { return on_off_vol_btn; }
    inline void set_on_off_vol_btn( double val ) {
	if ( val < 0.0 ) val = 0.0;
	if ( val > 1.0 ) val = 1.0;
	on_off_vol_btn = val;
    }
    inline bool get_adf_btn() const { return adf_btn; }
    inline void set_adf_btn( bool val ) { adf_btn = val; }
    inline bool get_bfo_btn() const { return bfo_btn; }
    inline void set_bfo_btn( bool val ) { bfo_btn = val; }
    inline bool get_frq_btn() const { return frq_btn; }
    inline void set_frq_btn( bool val ) { frq_btn = val; }
    inline bool get_flt_et_btn() const { return flt_et_btn; }
    inline void set_flt_et_btn( bool val ) { flt_et_btn = val; }
    inline bool get_set_rst_btn() const { return set_rst_btn; }
    inline void set_set_rst_btn( bool val ) { set_rst_btn = val; }
    inline bool get_ident_btn() const { return ident_btn; }
    inline void set_ident_btn( bool val ) { ident_btn = val; }

    // outputs
    inline double get_freq () const { return freq; }
    inline void set_freq( double f ) {
	freq = f;
        need_update = true;
    }
    double get_stby_freq () const;
    inline void set_stby_freq( double freq ) { stby_freq = freq; }
    inline double get_needle_deg() const { return needle_deg; }
    inline double get_flight_timer() const { return flight_timer; }
    inline double get_elapsed_timer() const { return elapsed_timer; }
    inline void set_elapsed_timer( double val ) { elapsed_timer = val; }

    // annunciators
    inline bool get_ant_ann() const { return ant_ann; }
    inline bool get_adf_ann() const { return adf_ann; }
    inline bool get_bfo_ann() const { return bfo_ann; }
    inline bool get_frq_ann() const { return frq_ann; }
    inline bool get_flt_ann() const { return flt_ann; }
    inline bool get_et_ann() const { return et_ann; }
};


#endif // _FG_KR_87_HXX
