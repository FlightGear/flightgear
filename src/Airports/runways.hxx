// runways.hxx -- a simple class to manage airport runway info
//
// Written by Curtis Olson, started August 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _RUNWAYS_HXX
#define _RUNWAYS_HXX


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
#include <vector>

#ifndef _MSC_VER
#define NDEBUG			// she don't work without it.
#endif // !_MSC_VER

#include <mk4.h>
#include <mk4str.h>

#ifndef _MSC_VER
#undef NDEBUG
#endif // !_MSC_VER

SG_USING_STD(string);
SG_USING_STD(vector);

#if ! defined( SG_HAVE_NATIVE_SGI_COMPILERS )
SG_USING_STD(istream);
#endif


class FGRunway {

public:

    string id;
    string rwy_no;

    double lon;
    double lat;
    double heading;
    double length;
    double width;
    
    string surface_flags;
    string end1_flags;
    string end2_flags;

public:

    FGRunway();
    ~FGRunway();

};

inline istream&
operator >> ( istream& in, FGRunway& a )
{
    int tmp;

    return in >> a.rwy_no >> a.lat >> a.lon >> a.heading >> a.length >> a.width
	      >> a.surface_flags >> a.end1_flags >> tmp >> tmp >> a.end2_flags
	      >> tmp >> tmp;
}


class FGRunways {

private:

    c4_Storage *storage;
    c4_View *vRunway;
    int next_index;

public:

    // Constructor
    FGRunways( const string& file );

    // Destructor
    ~FGRunways();

    // search for the specified apt id.
    // Returns true if successful, otherwise returns false.
    // On success, runway data is returned thru "runway" pointer.
    // "runway" is not changed if "apt" is not found.
    bool search( const string& aptid, FGRunway* runway );
    bool search( const string& aptid, const string& rwyno, FGRunway* runway );

    // DCL - search for runway closest to desired heading in degrees
    bool search( const string& aptid, const int hdg, FGRunway* runway );

    // Return the runway number of the runway closest to a given heading
    string search( const string& aptid, const int tgt_hdg );

    FGRunway search( const string& aptid );
    bool next( FGRunway* runway );
    FGRunway next();
};


class FGRunwaysUtil {
public:
    typedef vector< FGRunway > container;
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;

private:
    container runways;

public:

    // Constructor
    FGRunwaysUtil();

    // Destructor
    ~FGRunwaysUtil();

    // load the data
    int load( const string& file );

    // save the data in metakit format
    bool dump_mk4( const string& file );

    // search for the specified id.
    // Returns true if successful, otherwise returns false.
    // On success, runway data is returned thru "runway" pointer.
    // "runway" is not changed if "id" is not found.
    // bool search( const string& id, FGRunway* runway ) const;
    // FGRunway search( const string& id ) const;
};


#endif // _RUNWAYS_HXX
