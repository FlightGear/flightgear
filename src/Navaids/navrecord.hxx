// navrecord.hxx -- generic vor/dme/ndb class
//
// Written by Curtis Olson, started May 2004.
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _FG_NAVRECORD_HXX
#define _FG_NAVRECORD_HXX

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


#define FG_NAV_DEFAULT_RANGE 50 // nm
#define FG_LOC_DEFAULT_RANGE 18 // nm
#define FG_DME_DEFAULT_RANGE 50 // nm
#define FG_NAV_MAX_RANGE 300    // nm


class FGNavRecord {

    int type;
    double lon, lat;            // location in geodetic coords
    double elev_ft;
    double x, y, z;             // location in cartesian coords (earth centered)
    int freq;
    int range;
    double multiuse;            // can be slaved variation of VOR
                                // (degrees) or localizer heading
                                // (degrees) or dme bias (nm)

    string ident;		// navaid ident
    string name;                // verbose name in nav database
    string apt_id;              // corresponding airport id


    bool serviceable;		// for failure modeling
    string trans_ident;         // for failure modeling

public:

    inline FGNavRecord(void);
    inline ~FGNavRecord(void) {}

    inline int get_type() const { return type; }
    inline double get_lon() const { return lon; }
    inline void set_lon( double l ) { lon = l; }
    inline double get_lat() const { return lat; }
    inline void set_lat( double l ) { lat = l; }
    inline double get_elev_ft() const { return elev_ft; }
    inline void set_elev_ft( double e ) { elev_ft = e; }
    inline double get_x() const { return x; }
    inline double get_y() const { return y; }
    inline double get_z() const { return z; }
    inline int get_freq() const { return freq; }
    inline int get_range() const { return range; }
    inline double get_multiuse() const { return multiuse; }
    inline void set_multiuse( double m ) { multiuse = m; }
    inline const char *get_ident() { return ident.c_str(); }
    inline string get_name() { return name; }
    inline string get_apt_id() { return apt_id; }
    inline bool get_serviceable() { return serviceable; }
    inline const char *get_trans_ident() { return trans_ident.c_str(); }

    friend istream& operator>> ( istream&, FGNavRecord& );
};


inline
FGNavRecord::FGNavRecord(void) :
    type(0),
    lon(0.0), lat(0.0),
    elev_ft(0.0),
    x(0.0), y(0.0), z(0.0),
    freq(0),
    range(0),
    multiuse(0.0),
    ident(""),
    name(""),
    apt_id(""),
    serviceable(true),
    trans_ident("")
{
}


inline istream&
operator >> ( istream& in, FGNavRecord& n )
{
    in >> n.type;
    
    if ( n.type == 99 ) {
        return in >> skipeol;
    }

    in >> n.lat >> n.lon >> n.elev_ft >> n.freq >> n.multiuse
       >> n.ident;
    getline( in, n.name );

    // silently multiply adf frequencies by 100 so that adf
    // vs. nav/loc frequency lookups can use the same code.
    if ( n.type == 2 ) {
        n.freq *= 100;
    }

    // Remove any leading spaces before the name
    while ( n.name.substr(0,1) == " " ) {
        n.name = n.name.erase(0,1);
    }

    if ( n.type >= 4 && n.type <= 9 ) {
        // these types are always associated with an airport id
        string::size_type pos = n.name.find(" ");
        n.apt_id = n.name.substr(0, pos);
    }

    // assign default ranges
    if ( n.type == 2 || n.type == 3 ) {
        n.range = FG_NAV_DEFAULT_RANGE;
    } else if ( n.type == 4 || n.type == 5 || n.type == 6 ) {
        n.range = FG_LOC_DEFAULT_RANGE;
    } else if ( n.type == 12 ) {
        n.range = FG_DME_DEFAULT_RANGE;
    } else {
        n.range = FG_LOC_DEFAULT_RANGE;
    }

    // transmitted ident (same as ident unless modeling a fault)
    n.trans_ident = n.ident;

    // generate cartesian coordinates
    Point3D geod( n.lon * SGD_DEGREES_TO_RADIANS,
                  n.lat * SGD_DEGREES_TO_RADIANS,
                  n.elev_ft * SG_FEET_TO_METER );
    Point3D cart = sgGeodToCart( geod );
    n.x = cart.x();
    n.y = cart.y();
    n.z = cart.z();

    return in;
}


#endif // _FG_NAVRECORD_HXX
