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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/SGReferenced.hxx>

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#elif defined( __BORLANDC__ ) || (__APPLE__)
#  include <iostream>
#else
#  include <istream.h>
#endif

#define FG_NAV_DEFAULT_RANGE 50 // nm
#define FG_LOC_DEFAULT_RANGE 18 // nm
#define FG_DME_DEFAULT_RANGE 50 // nm
#define FG_NAV_MAX_RANGE 300    // nm

// Shield the rest of FG from possibly changing details of Robins navaid type numbering system.
// Currently only the GPS code uses this - extra types (LOC, GS etc) may need to be added
// should other FG code choose to use this. 
enum fg_nav_types {
    FG_NAV_VOR,
    FG_NAV_NDB,
    FG_NAV_ILS,
    FG_NAV_ANY
};

class FGNavRecord : public SGReferenced {

    int type;
    SGGeod pos;                // location in geodetic coords (degrees)
    SGVec3d cart;              // location in cartesian coords (earth centered)
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
    inline fg_nav_types get_fg_type() const;
    inline double get_lon() const { return pos.getLongitudeDeg(); } // degrees
    inline void set_lon( double l ) { pos.setLongitudeDeg(l); } // degrees
    inline double get_lat() const { return pos.getLatitudeDeg(); } // degrees
    inline void set_lat( double l ) { pos.setLatitudeDeg(l); } // degrees
    inline double get_elev_ft() const { return pos.getElevationFt(); }
    inline void set_elev_ft( double e ) { pos.setElevationFt(e); }
    const SGGeod& get_pos() const { return pos; }
    const SGVec3d& get_cart() const { return cart; }
    inline int get_freq() const { return freq; }
    inline int get_range() const { return range; }
    inline double get_multiuse() const { return multiuse; }
    inline void set_multiuse( double m ) { multiuse = m; }
    inline const char *get_ident() const { return ident.c_str(); }
    inline const string& get_name() const { return name; }
    inline const string& get_apt_id() const { return apt_id; }
    inline bool get_serviceable() const { return serviceable; }
    inline const char *get_trans_ident() const { return trans_ident.c_str(); }

    friend std::istream& operator>> ( std::istream&, FGNavRecord& );
};


inline
FGNavRecord::FGNavRecord(void) :
    type(0),
    pos(SGGeod::fromDeg(0, 0)),
    cart(0, 0, 0),
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


inline fg_nav_types FGNavRecord::get_fg_type() const {
    switch(type) {
    case 2: return(FG_NAV_NDB);
    case 3: return(FG_NAV_VOR);
    case 4: return(FG_NAV_ILS);
    default: return(FG_NAV_ANY);
    }
}


inline std::istream&
operator >> ( std::istream& in, FGNavRecord& n )
{
    in >> n.type;
    
    if ( n.type == 99 ) {
        return in >> skipeol;
    }

    double lat, lon, elev_ft;
    in >> lat >> lon >> elev_ft >> n.freq >> n.range >> n.multiuse
       >> n.ident;
    n.pos.setLatitudeDeg(lat);
    n.pos.setLongitudeDeg(lon);
    n.pos.setElevationFt(elev_ft);
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

    // Ranges are included with the latest data format, no need to
    // assign our own defaults, unless the range is not set for some
    // reason.

    if ( n.range < 0.1 ) {
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
    }

    // transmitted ident (same as ident unless modeling a fault)
    n.trans_ident = n.ident;

    // generate cartesian coordinates
    n.cart = SGVec3d::fromGeod(n.pos);

    return in;
}

class FGTACANRecord : public SGReferenced {

    string channel;		
    int freq;
     
public:
    
    inline FGTACANRecord(void);
    inline ~FGTACANRecord(void) {}

    inline const string& get_channel() const { return channel; }
    inline int get_freq() const { return freq; }
    friend std::istream& operator>> ( std::istream&, FGTACANRecord& );
    };


inline
FGTACANRecord::FGTACANRecord(void) :
    channel(""),
    freq(0)
    
{
}

inline std::istream&
operator >> ( std::istream& in, FGTACANRecord& n )
{
    in >> n.channel >> n.freq ;
    //getline( in, n.name );

    return in;
}
#endif // _FG_NAVRECORD_HXX
