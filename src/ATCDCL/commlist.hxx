// commlist.hxx -- comm frequency lookup class
//
// Written by David Luff and Alexander Kappes, started Jan 2003.
// Based on navlist.hxx by Curtis Olson, started April 2000.
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

/*****************************************************************
*
* FGCommList is used to store communication frequency information
* for the ATC and AI subsystems.  Two maps are maintained - one
* searchable by location and one searchable by frequency.  The
* data structure returned from the search is the ATCData struct
* defined in ATC.hxx, containing location, frequency, name, range
* and type of the returned station.
*
******************************************************************/

#ifndef _FG_COMMLIST_HXX
#define _FG_COMMLIST_HXX


#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>

#include <map>
#include <list>
#include <string>

#include "ATC.hxx"
#include "atis.hxx"

using std::list;
using std::map;
using std::vector;
using std::string;

// A list of ATC stations
typedef list < ATCData > comm_list_type;
typedef comm_list_type::iterator comm_list_iterator;
typedef comm_list_type::const_iterator comm_list_const_iterator;

// A map of ATC station lists
typedef map < int, comm_list_type > comm_map_type;
typedef comm_map_type::iterator comm_map_iterator;
typedef comm_map_type::const_iterator comm_map_const_iterator;


class FGCommList {
    
public:

    FGCommList();
    ~FGCommList();

    // load all comm frequencies and build the map
    bool init( const SGPath& path );

    // query the database for the specified frequency, lon and lat are
    // in degrees, elev is in meters.
    // If no atc_type is specified, it returns true if any non-invalid type is found.
    // If atc_type is specifed, returns true only if the specified type is found.
    // Returns the station closest to the supplied position.
    // The data found is written into the passed-in ATCData structure.
    bool FindByFreq( double lon, double lat, double elev, double freq, ATCData* ad, atc_type tp = INVALID );
    
    // query the database by location, lon and lat are in degrees, elev is in meters, range is in nautical miles.
    // Returns the number of stations of the specified atc_type tp that are in range of the position defined by 
    // lon, lat and elev, and pushes them into stations.
    // If no atc_type is specifed, returns the number of all stations in range, and pushes them into stations
    // ** stations is erased before use **
    int FindByPos( double lon, double lat, double elev, double range, comm_list_type* stations, atc_type tp = INVALID );
    
    // Returns the distance in meters to the closest station of a given type,
    // with the details written into ATCData& ad.  If no type is specifed simply
    // returns the distance to the closest station of any type.
    // Returns -9999 if no stations found within max_range in nautical miles (default 100 miles).
    // Note that the search algorithm starts at 10 miles and multiplies by 10 thereafter, so if
    // say 300 miles is specifed 10, then 100, then 1000 will be searched, breaking at first result 
    // and giving up after 1000.
    // !!!Be warned that searching anything over 100 miles will pause the sim unacceptably!!!
    //  (The ability to search longer ranges should be used during init only).
    double FindClosest( double lon, double lat, double elev, ATCData& ad, atc_type tp = INVALID, double max_range = 100.0 );
    
    // Find by Airport code.
    bool FindByCode( const string& ICAO, ATCData& ad, atc_type tp = INVALID );

    // Return the sequence letter for an ATIS transmission given transmission time and airport id
    // This maybe should get moved somewhere else!!
    int GetAtisSequence( const string& apt_id, const double tstamp, 
                    const int interval, const int flush=0);
    
    // Comm stations mapped by frequency
    comm_map_type commlist_freq;    
    
    // Comm stations mapped by bucket
    comm_map_type commlist_bck;

    // Load comms from a specified path (which must include the filename)   
private:

    bool LoadComms(const SGPath& path);

//----------- This stuff is left over from atislist.[ch]xx and maybe should move somewhere else
    // Add structure and map for storing a log of atis transmissions
    // made in this session of FlightGear.  This allows the callsign
    // to be allocated correctly wrt time.
    typedef struct {
            double tstamp;
        int sequence;
    } atis_transmission_type;

    typedef map < string, atis_transmission_type > atis_log_type;
    typedef atis_log_type::iterator atis_log_iterator;
    typedef atis_log_type::const_iterator atis_log_const_iterator;

    atis_log_type atislog;
//-----------------------------------------------------------------------------------------------

};


extern FGCommList *current_commlist;

#endif // _FG_COMMLIST_HXX


