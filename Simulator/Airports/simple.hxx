//
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


#ifndef _AIRPORTS_HXX
#define _AIRPORTS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <Include/compiler.h>
#ifdef FG_HAVE_STD_INCLUDES
#  include <istream>
#elif defined( FG_HAVE_NATIVE_SGI_COMPILERS )
#  include <iostream.h>
#else
#  include <istream.h>
#endif

#include STL_STRING
#include <set>

FG_USING_STD(string);
FG_USING_STD(set);

#if ! defined( FG_HAVE_NATIVE_SGI_COMPILERS )
FG_USING_STD(istream);
#endif


class fgAIRPORT {
public:
    fgAIRPORT( const string& name = "",
	       double lon = 0.0,
	       double lat = 0.0,
	       double ele = 0.0 )
	: id(name), longitude(lon), latitude(lat), elevation(ele) {}

    bool operator < ( const fgAIRPORT& a ) const {
	return id < a.id;
    }

public:
    string id;
    double longitude;
    double latitude;
    double elevation;
};

inline istream&
operator >> ( istream& in, fgAIRPORT& a )
{
    return in >> a.id >> a.longitude >> a.latitude >> a.elevation;
}

class fgAIRPORTS {
public:
#ifdef FG_NO_DEFAULT_TEMPLATE_ARGS
    typedef set< fgAIRPORT, less< fgAIRPORT > > container;
#else
    typedef set< fgAIRPORT > container;
#endif
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;

private:
    container airports;

public:

    // Constructor
    fgAIRPORTS();

    // Destructor
    ~fgAIRPORTS();

    // load the data
    int load( const string& file );

    // search for the specified id.
    // Returns true if successful, otherwise returns false.
    // On success, airport data is returned thru "airport" pointer.
    // "airport" is not changed if "id" is not found.
    bool search( const string& id, fgAIRPORT* airport ) const;
    fgAIRPORT search( const string& id ) const;
};


#endif /* _AIRPORTS_HXX */


