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

    string adf_ident;
    string adf_trans_ident;
    bool adf_valid;
    bool adf_inrange;
    double adf_freq;
    double adf_alt_freq;
    double adf_rotation;
    double adf_lon;
    double adf_lat;
    double adf_elev;
    double adf_range;
    double adf_effective_range;
    double adf_dist;
    double adf_heading;
    double adf_x;
    double adf_y;
    double adf_z;
    double adf_vol_btn;
    bool adf_ident_btn;

public:

    FGKR_87();
    ~FGKR_87();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Update nav/adf radios based on current postition
    void search ();

    // ADF Setters
    inline void set_adf_freq( double freq ) {
	adf_freq = freq; need_update = true;
    }
    inline void set_adf_alt_freq( double freq ) { adf_alt_freq = freq; }
    inline void set_adf_rotation( double rot ) { adf_rotation = rot; }
    inline void set_adf_vol_btn( double val ) {
	if ( val < 0.0 ) val = 0.0;
	if ( val > 1.0 ) val = 1.0;
	adf_vol_btn = val;
    }
    inline void set_adf_ident_btn( bool val ) { adf_ident_btn = val; }

    // ADF Accessors
    inline double get_adf_freq () const { return adf_freq; }
    inline double get_adf_alt_freq () const { return adf_alt_freq; }
    inline double get_adf_rotation () const { return adf_rotation; }

    // Calculated values
    inline bool get_adf_inrange() const { return adf_inrange; }
    inline double get_adf_lon() const { return adf_lon; }
    inline double get_adf_lat() const { return adf_lat; }
    inline double get_adf_heading() const { return adf_heading; }
    inline double get_adf_vol_btn() const { return adf_vol_btn; }
    inline bool get_adf_ident_btn() const { return adf_ident_btn; }
};


#endif // _FG_KR_87_HXX
