// atcutils.hxx 
//
// This file contains a collection of classes from David Luff's
// AI/ATC code that are shared by non-AI parts of FlightGear.
// more specifcially, it contains implementations of FGCommList and
// FGATCAlign
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

#ifndef _FG_ATCUTILS_HXX
#define _FG_ATCUTILS_HXX



#include <simgear/compiler.h>

#include <map>
#include <list>
#include <string>

//#include "ATC.hxx"
//#include "atis.hxx"

#if !ENABLE_ATCDCL

class SGPath;
class ATCData;

// Possible types of ATC type that the radios may be tuned to.
// INVALID implies not tuned in to anything.
enum atc_type {
	AWOS,
	ATIS,
	GROUND,
	TOWER,
	APPROACH,
	DEPARTURE,
	ENROUTE,
  INVALID	 /* must be last element;  see ATC_NUM_TYPES */
};


// A list of ATC stations
typedef std::list < ATCData > comm_list_type;
typedef comm_list_type::iterator comm_list_iterator;
typedef comm_list_type::const_iterator comm_list_const_iterator;

// A map of ATC station lists
typedef std::map < int, comm_list_type > comm_map_type;
typedef comm_map_type::iterator comm_map_iterator;
typedef comm_map_type::const_iterator comm_map_const_iterator;


class FGCommList {
    
public:

    FGCommList();
    ~FGCommList();

    // load all comm frequencies and build the map
    bool init( const SGPath& path );

    // query the database for the specified frequency, lon and lat are
    // If no atc_type is specified, it returns true if any non-invalid type is found.
    // If atc_type is specifed, returns true only if the specified type is found.
    // Returns the station closest to the supplied position.
    // The data found is written into the passed-in ATCData structure.
    bool FindByFreq(const SGGeod& aPos, double freq, ATCData* ad, atc_type tp = INVALID );
    
    // query the database by location, lon and lat are in degrees, elev is in meters, range is in nautical miles.
    // Returns the number of stations of the specified atc_type tp that are in range of the position defined by 
    // lon, lat and elev, and pushes them into stations.
    // If no atc_type is specifed, returns the number of all stations in range, and pushes them into stations
    // ** stations is erased before use **
    int FindByPos(const SGGeod& aPos, double range, comm_list_type* stations, atc_type tp = INVALID );
    
    // Returns the distance in meters to the closest station of a given type,
    // with the details written into ATCData& ad.  If no type is specifed simply
    // returns the distance to the closest station of any type.
    // Returns -9999 if no stations found within max_range in nautical miles (default 100 miles).
    // Note that the search algorithm starts at 10 miles and multiplies by 10 thereafter, so if
    // say 300 miles is specifed 10, then 100, then 1000 will be searched, breaking at first result 
    // and giving up after 1000.
    // !!!Be warned that searching anything over 100 miles will pause the sim unacceptably!!!
    //  (The ability to search longer ranges should be used during init only).
    double FindClosest(const SGGeod& aPos, ATCData& ad, atc_type tp = INVALID, double max_range = 100.0 );
    
    // Find by Airport code.
    bool FindByCode( const std::string& ICAO, ATCData& ad, atc_type tp = INVALID );

    // Return the sequence letter for an ATIS transmission given transmission time and airport id
    // This maybe should get moved somewhere else!!
    int GetAtisSequence( const std::string& apt_id, const double tstamp, 
                    const int interval, const int flush=0);
    
    // Comm stations mapped by frequency
    //comm_map_type commlist_freq;    
    
    // Comm stations mapped by bucket
    //comm_map_type commlist_bck;

    // Load comms from a specified path (which must include the filename)   
private:

    bool LoadComms(const SGPath& path);

//----------- This stuff is left over from atislist.[ch]xx and maybe should move somewhere else
    // Add structure and map for storing a log of atis transmissions
    // made in this session of FlightGear.  This allows the callsign
    // to be allocated correctly wrt time.
    //typedef struct {
    //        double tstamp;
    //    int sequence;
    //} atis_transmission_type;

    //typedef std::map < std::string, atis_transmission_type > atis_log_type;
    //typedef atis_log_type::iterator atis_log_iterator;
    //typedef atis_log_type::const_iterator atis_log_const_iterator;

    //atis_log_type atislog;
//-----------------------------------------------------------------------------------------------

};


extern FGCommList *current_commlist;

// FGATCAlignedProjection - a class to project an area local to a runway onto an orthogonal co-ordinate system
// with the origin at the threshold and the runway aligned with the y axis.
class FGKln89AlignedProjection {

public:
    FGKln89AlignedProjection();
    FGKln89AlignedProjection(const SGGeod& centre, double heading);
    ~FGKln89AlignedProjection();

    void Init(const SGGeod& centre, double heading);

    // Convert a lat/lon co-ordinate (degrees) to the local projection (meters)
    SGVec3d ConvertToLocal(const SGGeod& pt);

    // Convert a local projection co-ordinate (meters) to lat/lon (degrees)
    SGGeod ConvertFromLocal(const SGVec3d& pt);

private:
    SGGeod _origin;	// lat/lon of local area origin (the threshold)
    double _theta;	// the rotation angle for alignment in radians
    double _correction_factor;	// Reduction in surface distance per degree of longitude due to latitude.  Saves having to do a cos() every call.

};

#endif // #if ENABLE_ATCDCL

#endif // _FG_ATCUTILS_HXX


