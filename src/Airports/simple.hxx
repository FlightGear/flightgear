// simple.hxx -- a really simplistic class to manage airport ID,
//                 lat, lon of the center of one of it's runways, and 
//                 elevation in feet.
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _FG_SIMPLE_HXX
#define _FG_SIMPLE_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include STL_STRING
#include <map>
#include <vector>

SG_USING_STD(string);
SG_USING_STD(map);
SG_USING_STD(vector);


struct FGAirport {
    string id;
    double longitude;
    double latitude;
    double elevation;
    string code;
    string name;
    bool has_metar;
};

typedef map < string, FGAirport > airport_map;
typedef airport_map::iterator airport_map_iterator;
typedef airport_map::const_iterator const_airport_map_iterator;

typedef vector < FGAirport * > airport_list;


class FGAirportList {

private:

    airport_map airports_by_id;
    airport_list airports_array;

public:

    // Constructor
    FGAirportList( const string &airport_file, const string &metar_file );

    // Destructor
    ~FGAirportList();

    // search for the specified id.
    // Returns true if successful, otherwise returns false.
    // On success, airport data is returned thru "airport" pointer.
    // "airport" is not changed if "apt" is not found.
    FGAirport search( const string& id );

    // search for the airport closest to the specified position
    // (currently a linear inefficient search so it's probably not
    // best to use this at runtime.)  If with_metar is true, then only
    // return station id's marked as having metar data.
    FGAirport search( double lon_deg, double lat_deg, bool with_metar );


    /**
     * Return the number of airports in the list.
     */
    int size() const;


    /**
     * Return a specific airport, by position.
     */
    const FGAirport *getAirport( int index ) const;


    /**
     * Mark the specified airport record as not having metar
     */
    void no_metar( const string &id );

};


#endif // _FG_SIMPLE_HXX


