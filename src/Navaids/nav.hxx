// nav.hxx -- vor/dme/ndb class
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


#ifndef _FG_NAV_HXX
#define _FG_NAV_HXX

#include <stdio.h>

#include <simgear/compiler.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#elif defined( __BORLANDC__ ) || (__APPLE__)
#  include <iostream>
#else
#  include <istream.h>
#endif

SG_USING_STD(istream);


class FGNav {
    int type;
    double lon, lat;
    double elev_ft;
    double x, y, z;
    int freq;
    double range;
    string ident;
    double magvar;		// magvar from true north (negative = W)
    string name;

    // for failure modeling
    bool serviceable;
    string trans_ident;		// transmitted ident

public:

    inline FGNav();
    inline FGNav( int _type, double _lat, double _lon, double _elev_ft,
                  int _freq, double _range, double _magvar,
                  string _ident, string _name,
                  bool _serviceable );
    inline ~FGNav() {}

    inline int get_type() const { return type; }
    inline double get_lon() const { return lon; }
    inline double get_lat() const { return lat; }
    inline double get_elev_ft() const { return elev_ft; }
    inline double get_x() const { return x; }
    inline double get_y() const { return y; }
    inline double get_z() const { return z; }
    inline int get_freq() const { return freq; }
    inline double get_range() const { return range; }
    // inline bool get_has_dme() const { return has_dme; }
    inline const char *get_ident() { return ident.c_str(); }
    inline string get_trans_ident() { return trans_ident; }
    inline double get_magvar() const { return magvar; }
    inline string get_name() { return name; }
};


inline
FGNav::FGNav() :
    type(0),
    lon(0.0), lat(0.0),
    elev_ft(0.0),
    x(0.0), y(0.0), z(0.0),
    freq(0),
    range(0.0),
    // has_dme(false),
    ident(""),
    magvar(0.0),
    name(""),
    serviceable(true),
    trans_ident("")
{
}


inline
FGNav::FGNav( int _type, double _lat, double _lon, double _elev_ft,
              int _freq, double _range, double _magvar,
              string _ident, string _name,
              bool _serviceable ) :
    type(0),
    lon(0.0), lat(0.0),
    elev_ft(0.0),
    x(0.0), y(0.0), z(0.0),
    freq(0),
    range(0.0),
    // has_dme(false),
    ident(""),
    magvar(0.0),
    name(""),
    serviceable(true),
    trans_ident("")
{
    type = _type;
    lat = _lat;
    lon = _lon;
    elev_ft = _elev_ft;
    freq = _freq;
    range = _range;
    magvar = _magvar;
    ident = _ident;
    name = _name;
    trans_ident = _ident;
    serviceable = _serviceable;

    // generate cartesian coordinates
    Point3D geod( lon * SGD_DEGREES_TO_RADIANS,
                  lat * SGD_DEGREES_TO_RADIANS,
                  elev_ft * SG_FEET_TO_METER );
    Point3D cart = sgGeodToCart( geod );
    x = cart.x();
    y = cart.y();
    z = cart.z();
}


#endif // _FG_NAV_HXX
