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

#include "positioned.hxx"

#define FG_NAV_DEFAULT_RANGE 50 // nm
#define FG_LOC_DEFAULT_RANGE 18 // nm
#define FG_DME_DEFAULT_RANGE 50 // nm
#define FG_NAV_MAX_RANGE 300    // nm

// FIXME - get rid of these, and use the real enum directly
#define FG_NAV_VOR FGPositioned::VOR
#define FG_NAV_NDB FGPositioned::NDB
#define FG_NAV_ILS FGPositioned::ILS
#define FG_NAV_ANY FGPositioned::INVALID

typedef FGPositioned::Type fg_nav_types;

// forward decls
class FGRunway;

class FGNavRecord : public FGPositioned 
{

    int freq;
    int range;
    double multiuse;            // can be slaved variation of VOR
                                // (degrees) or localizer heading
                                // (degrees) or dme bias (nm)

    std::string name;                // verbose name in nav database
    std::string apt_id;              // corresponding airport id


    bool serviceable;		// for failure modeling
    std::string trans_ident;         // for failure modeling

  /**
   * Helper to init data when a navrecord is associated with an airport
   */
  void initAirportRelation();
  
  void alignLocaliserWithRunway(FGRunway* aRunway, double aThreshold);
public:
  inline ~FGNavRecord(void) {}

  static FGNavRecord* createFromStream(std::istream& aStream);

    FGNavRecord(Type type, const std::string& ident, const std::string& name,
      double lat, double lon, double aElevFt,
      int freq, int range, double multiuse);
    
    inline double get_lon() const { return longitude(); } // degrees
    inline double get_lat() const { return latitude(); } // degrees
    inline double get_elev_ft() const { return elevation(); }

    const SGGeod& get_pos() const { return geod(); }
    SGVec3d get_cart() const;
    
    Type get_fg_type() const { return type(); }
    
    inline int get_freq() const { return freq; }
    inline int get_range() const { return range; }
    inline double get_multiuse() const { return multiuse; }
    inline void set_multiuse( double m ) { multiuse = m; }
    inline const char *get_ident() const { return ident().c_str(); }
    inline const std::string& get_name() const { return name; }
    inline const std::string& get_apt_id() const { return apt_id; }
    inline bool get_serviceable() const { return serviceable; }
    inline const char *get_trans_ident() const { return trans_ident.c_str(); }

  
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
