// radiostack.hxx -- class to manage an instance of the radio stack
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


#ifndef _FG_RADIOSTACK_HXX
#define _FG_RADIOSTACK_HXX


#include <simgear/compiler.h>


class FGRadioStack {

    bool need_update;

    bool nav1_inrange;
    bool nav1_loc;
    double nav1_freq;
    double nav1_radial;
    double nav1_lon;
    double nav1_lat;
    double nav1_elev;
    double nav1_dist;
    double nav1_heading;

    bool nav2_inrange;
    double nav2_freq;
    double nav2_radial;
    double nav2_lon;
    double nav2_lat;
    double nav2_elev;
    double nav2_dist;
    double nav2_heading;

    bool adf_inrange;
    double adf_freq;
    double adf_lon;
    double adf_lat;
    double adf_elev;
    double adf_heading;

public:

    FGRadioStack();
    ~FGRadioStack();

    // Update nav/adf radios based on current postition
    void update( double lon, double lat, double elev );

    inline void set_nav1_freq( double freq ) {
	nav1_freq = freq; need_update = true;
    }
    inline void set_nav1_radial( double radial ) {
	nav1_radial = radial; need_update = true;
    }

    inline void set_nav2_freq( double freq ) {
	nav2_freq = freq; need_update = true;
    }

    inline void set_nav2_radial( double radial ) {
	nav2_radial = radial; need_update = true;
    }

    inline void set_adf_freq( double freq ) {
	adf_freq = freq; need_update = true;
    }

    inline bool get_nav1_inrange() const { return nav1_inrange; }
    inline bool get_nav1_loc() const { return nav1_loc; }
    inline double get_nav1_radial() const { return nav1_radial; }
    inline double get_nav1_lon() const { return nav1_lon; }
    inline double get_nav1_lat() const { return nav1_lat; }
    inline double get_nav1_elev() const { return nav1_elev; }
    inline double get_nav1_dist() const { return nav1_dist; }
    inline double get_nav1_heading() const { return nav1_heading; }

    inline bool get_nav2_inrange() const { return nav2_inrange; }
    inline double get_nav2_radial() const { return nav2_radial; }
    inline double get_nav2_lon() const { return nav2_lon; }
    inline double get_nav2_lat() const { return nav2_lat; }
    inline double get_nav2_elev() const { return nav2_elev; }
    inline double get_nav2_dist() const { return nav2_dist; }
    inline double get_nav2_heading() const { return nav2_heading; }

    inline bool get_adf_inrange() const { return adf_inrange; }
    inline double get_adf_lon() const { return adf_lon; }
    inline double get_adf_lat() const { return adf_lat; }
    inline double get_adf_heading() const { return adf_heading; }
};


#endif // _FG_RADIOSTACK_HXX
