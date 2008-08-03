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

#include <iosfwd>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>

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

    std::string ident;		// navaid ident
    std::string name;                // verbose name in nav database
    std::string apt_id;              // corresponding airport id


    bool serviceable;		// for failure modeling
    std::string trans_ident;         // for failure modeling

public:
    FGNavRecord(void);
    inline ~FGNavRecord(void) {}

    FGNavRecord(int type, const std::string& ident, const std::string& name, const std::string& airport,
      double lat, double lon, int freq, int range, double multiuse);

    inline int get_type() const { return type; }
    
    fg_nav_types get_fg_type() const;
    
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
    inline const std::string& get_name() const { return name; }
    inline const std::string& get_apt_id() const { return apt_id; }
    inline bool get_serviceable() const { return serviceable; }
    inline const char *get_trans_ident() const { return trans_ident.c_str(); }

    friend std::istream& operator>> ( std::istream&, FGNavRecord& );
};

class FGTACANRecord : public SGReferenced {

    std::string channel;		
    int freq;
     
public:
    
     FGTACANRecord(void);
    inline ~FGTACANRecord(void) {}

    inline const std::string& get_channel() const { return channel; }
    inline int get_freq() const { return freq; }
    friend std::istream& operator>> ( std::istream&, FGTACANRecord& );
    };

#endif // _FG_NAVRECORD_HXX
