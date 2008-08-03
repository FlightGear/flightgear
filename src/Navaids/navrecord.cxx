// navrecord.cxx -- generic vor/dme/ndb class
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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <istream>
#include <simgear/misc/sgstream.hxx>
#include "Navaids/navrecord.hxx"

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

FGNavRecord::FGNavRecord(int aTy, const std::string& aIdent, 
  const std::string& aName, const std::string& aAirport,
  double lat, double lon, int aFreq, int aRange, double aMultiuse) :
  type(aTy),
  freq(aFreq),
  range(aRange),
  multiuse(aMultiuse),
  ident(aIdent),
  name(aName),
  apt_id(aAirport),
  serviceable(true),
  trans_ident(aIdent)
{
  pos = SGGeod::fromDeg(lon, lat);
  cart = SGVec3d::fromGeod(pos);
}

fg_nav_types FGNavRecord::get_fg_type() const {
    switch(type) {
    case 2: return(FG_NAV_NDB);
    case 3: return(FG_NAV_VOR);
    case 4: return(FG_NAV_ILS);
    default: return(FG_NAV_ANY);
    }
}


std::istream& operator>>( std::istream& in, FGNavRecord& n )
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
        std::string::size_type pos = n.name.find(" ");
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


FGTACANRecord::FGTACANRecord(void) :
    channel(""),
    freq(0)
    
{
}

std::istream&
operator >> ( std::istream& in, FGTACANRecord& n )
{
    in >> n.channel >> n.freq ;
    //getline( in, n.name );

    return in;
}
