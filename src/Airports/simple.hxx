// simple.hxx -- a really simplistic class to manage airport ID,
//                 lat, lon of the center of one of it's runways, and 
//                 elevation in feet.
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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


#ifndef _SIMPLE_HXX
#define _SIMPLE_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef SG_HAVE_STD_INCLUDES
#  include <istream>
#elif defined( SG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#elif defined( __BORLANDC__ )
#  include <iostream>
#else
#  include <istream.h>
#endif

#include STL_STRING
#include <set>

#ifndef _MSC_VER
#   define NDEBUG			// she don't work without it.
#endif
#include <mk4.h>
#include <mk4str.h>
#ifndef _MSC_VER
#  undef NDEBUG
#endif

SG_USING_STD(string);
SG_USING_STD(set);

#if ! defined( SG_HAVE_NATIVE_SGI_COMPILERS )
SG_USING_STD(istream);
#endif


class FGAirport {

public:

    FGAirport( const string& name = "",
	       double lon = 0.0,
	       double lat = 0.0,
	       double ele = 0.0 )
	: id(name), longitude(lon), latitude(lat), elevation(ele) {}

    bool operator < ( const FGAirport& a ) const {
	return id < a.id;
    }

public:

    string id;
    double longitude;
    double latitude;
    double elevation;

};

inline istream&
operator >> ( istream& in, FGAirport& a )
{
    return in >> a.id >> a.latitude >> a.longitude >> a.elevation;
}


class FGAirports {

private:

    c4_Storage *storage;
    c4_View *vAirport;

public:

    // Constructor
    FGAirports( const string& file );

    // Destructor
    ~FGAirports();

    // search for the specified id.
    // Returns true if successful, otherwise returns false.
    // On success, airport data is returned thru "airport" pointer.
    // "airport" is not changed if "apt" is not found.
    bool search( const string& id, FGAirport* airport ) const;
    FGAirport search( const string& id ) const;
};


class FGAirportsUtil {
public:
#ifdef SG_NO_DEFAULT_TEMPLATE_ARGS
    typedef set< FGAirport, less< FGAirport > > container;
#else
    typedef set< FGAirport > container;
#endif
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;

private:
    container airports;

public:

    // Constructor
    FGAirportsUtil();

    // Destructor
    ~FGAirportsUtil();

    // load the data
    int load( const string& file );

    // save the data in metakit format
    bool dump_mk4( const string& file );

    // search for the specified id.
    // Returns true if successful, otherwise returns false.
    // On success, airport data is returned thru "airport" pointer.
    // "airport" is not changed if "id" is not found.
    bool search( const string& id, FGAirport* airport ) const;
    FGAirport search( const string& id ) const;
};


#endif // _SIMPLE_HXX


