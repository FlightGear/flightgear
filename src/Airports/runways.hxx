// runways.hxx -- a simple class to manage airport runway info
//
// Written by Curtis Olson, started August 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _FG_RUNWAYS_HXX
#define _FG_RUNWAYS_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include STL_STRING
#include <map>

SG_USING_STD(string);
SG_USING_STD(multimap);


struct ltstr {
    bool operator()(const string s1, const string s2) const {
        return s1 < s2;
    }
};


struct FGRunway {

    string type;
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

    double end1_displaced_threshold;
    double end2_displaced_threshold;

    double end1_stopway;
    double end2_stopway;

};

typedef multimap < string, FGRunway, ltstr > runway_map;
typedef runway_map::iterator runway_map_iterator;
typedef runway_map::const_iterator const_runway_map_iterator;

class FGRunwayList {

private:

    runway_map runways;
    runway_map_iterator current;

public:

    // Constructor
    FGRunwayList( const string& file );

    // Destructor
    ~FGRunwayList();

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


#endif // _FG_RUNWAYS_HXX
