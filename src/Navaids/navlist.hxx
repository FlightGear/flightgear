// navlist.hxx -- navaids management class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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


#ifndef _FG_NAVLIST_HXX
#define _FG_NAVLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/SGReferenced.hxx>

#include <map>
#include <vector>
#include <string>

#include "navrecord.hxx"

using std::map;
using std::vector;
using std::string;



// FGNavList ------------------------------------------------------------------

typedef SGSharedPtr<FGNavRecord> nav_rec_ptr;
typedef vector < nav_rec_ptr > nav_list_type;
typedef nav_list_type::iterator nav_list_iterator;
typedef nav_list_type::const_iterator nav_list_const_iterator;

typedef map < int, nav_list_type > nav_map_type;
typedef nav_map_type::iterator nav_map_iterator;
typedef nav_map_type::const_iterator nav_map_const_iterator;

typedef map < string, nav_list_type > nav_ident_map_type;
typedef nav_ident_map_type::iterator nav_ident_map_iterator;


class FGNavList {

    nav_list_type carrierlist;
    nav_map_type navaids;
    nav_map_type navaids_by_tile;
    nav_ident_map_type ident_navaids;

    // Given a point and a list of stations, return the closest one to
    // the specified point.
    FGNavRecord *findNavFromList( const SGVec3d &aircraft,
                                  const nav_list_type &stations );

public:

    FGNavList();
    ~FGNavList();

    // initialize the nav list
    bool init();

    // add an entry
    bool add( FGNavRecord *n );

    /** Query the database for the specified station.  It is assumed
      * that there will be multiple stations with matching frequencies
      * so a position must be specified.  Lon and lat are in radians,
      * elev is in meters.
      */
    FGNavRecord *findByFreq( double freq, double lon, double lat, double elev );

    // locate closest item in the DB matching the requested ident
    FGNavRecord *findByIdent( const char* ident, const double lon, const double lat );

    // Find items of requested type with closest exact or subsequent ident
    // (by ASCII code value) to that supplied.
    // Supplying true for exact forces only exact matches to be returned (similar to above function)
    // Returns an empty list if no match found - calling function should check for this!
    nav_list_type findFirstByIdent( const string& ident, FGPositioned::Type type, bool exact = false );

    // Given an Ident and optional freqency, return the first matching
    // station.
    FGNavRecord *findByIdentAndFreq( const char* ident,
                                     const double freq = 0.0 );

    // returns the closest entry to the give lon/lat/elev
    FGNavRecord *findClosest( double lon_rad, double lat_rad, double elev_m, 
      FGPositioned::Type type = FG_NAV_ANY );

    // given a frequency returns the first matching entry
    FGNavRecord *findStationByFreq( double frequency );

    inline const nav_map_type& get_navaids() const { return navaids; }
};




// FGTACANList ----------------------------------------------------------------


typedef SGSharedPtr<FGTACANRecord> tacan_rec_ptr;
typedef vector < tacan_rec_ptr > tacan_list_type;
typedef map < int, tacan_list_type > tacan_map_type;
typedef map < string, tacan_list_type > tacan_ident_map_type;


class FGTACANList {

    tacan_list_type channellist;
    tacan_map_type channels;
    tacan_ident_map_type ident_channels;

public:

    FGTACANList();
    ~FGTACANList();

    // initialize the TACAN list
    bool init();

    // add an entry
    bool add( FGTACANRecord *r );

    // Given a TACAN Channel, return the appropriate frequency.
    FGTACANRecord *findByChannel( const string& channel );


};
#endif // _FG_NAVLIST_HXX
