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
    bool operator()(const string& s1, const string& s2) const {
        return s1 < s2;
    }
};


struct FGRunway {

    string _id;
    string _rwy_no;
    string _type;                // runway / taxiway

    double _lon;
    double _lat;
    double _heading;
    double _length;
    double _width;
    double _displ_thresh1;
    double _displ_thresh2;
    double _stopway1;
    double _stopway2;

    string _lighting_flags;
    int _surface_code;
    string _shoulder_code;
    int _marking_code;
    double _smoothness;
    bool   _dist_remaining;
};

typedef multimap < string, FGRunway, ltstr > runway_map;
typedef runway_map::iterator runway_map_iterator;
typedef runway_map::const_iterator const_runway_map_iterator;

class FGRunwayList {

private:

    runway_map runways;
    runway_map_iterator current;

public:

    // Constructor (new)
    FGRunwayList() {}

    // Destructor
    ~FGRunwayList();

    // add an entry to the list
    void add( const string& id, const string& rwy_no,
              const double longitude, const double latitude,
              const double heading, const double length, const double width,
              const double displ_thresh1, const double displ_thresh2,
              const double stopway1, const double stopway2,
              const string& lighting_flags, const int surface_code,
              const string& shoulder_code, const int marking_code,
              const double smoothness, const bool dist_remaining );

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
