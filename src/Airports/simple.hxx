// simple.hxx -- a really simplistic class to manage airport ID,
//                 lat, lon of the center of one of it's runways, and
//                 elevation in feet.
//
// Written by Curtis Olson, started April 1998.
// Updated by Durk Talsma, started December 2004.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _FG_SIMPLE_HXX
#define _FG_SIMPLE_HXX

#include <simgear/math/point3d.hxx>

#include <simgear/compiler.h>

#include <string>
#include <map>
#include <set>
#include <vector>

#include "Airports/runways.hxx"

// forward decls
class FGAirportDynamics;

/***************************************************************************************
 *
 **************************************************************************************/
class FGAirport {
private:
    std::string _id;
    SGGeod _location;
    SGGeod _tower_location;
    std::string _name;
    bool _has_metar;
    bool _is_airport;
    bool _is_seaport;
    bool _is_heliport;
    FGAirportDynamics *_dynamics;

public:
    FGAirport();
    // FGAirport(const FGAirport &other);
    FGAirport(const std::string& id, const SGGeod& location, const SGGeod& tower, 
            const std::string& name,
            bool has_metar, bool is_airport, bool is_seaport, bool is_heliport);
    ~FGAirport();

    const std::string& getId() const { return _id; }
    const std::string& getName() const { return _name; }
    double getLongitude() const { return _location.getLongitudeDeg(); }
    // Returns degrees
    double getLatitude()  const { return _location.getLatitudeDeg(); }
    // Returns ft
    double getElevation() const { return _location.getElevationFt(); }
    bool   getMetar()     const { return _has_metar; }
    bool   isAirport()    const { return _is_airport; }
    bool   isSeaport()    const { return _is_seaport; }
    bool   isHeliport()   const { return _is_heliport; }

    const SGGeod& getTowerLocation() const { return _tower_location; }

    void setId(const std::string& id) { _id = id; }
    void setMetar(bool value) { _has_metar = value; }

    FGRunway getActiveRunwayForUsage() const;

    FGAirportDynamics *getDynamics();
    
    unsigned int numRunways() const;
    FGRunway getRunwayByIndex(unsigned int aIndex) const;

    bool hasRunwayWithIdent(const std::string& aIdent) const;
    FGRunway getRunwayByIdent(const std::string& aIdent) const;
    FGRunway findBestRunwayForHeading(double aHeading) const;
    
    unsigned int numTaxiways() const;
    FGRunway getTaxiwayByIndex(unsigned int aIndex) const;
    
    void addRunway(const FGRunway& aRunway);
private:
    /**
     * Helper to locate a runway by ident
     */
    FGRunwayVector::const_iterator getIteratorForRunwayIdent(const std::string& aIdent, bool& aReversed) const;

    FGAirport operator=(FGAirport &other);
    FGAirport(const FGAirport&);
    
    std::vector<FGRunway> mRunways;
    std::vector<FGRunway> mTaxiways;
};


class FGAirportSearchFilter {
public:
    virtual ~FGAirportSearchFilter() {}
    // all airports pass the filter by default
    virtual bool pass(FGAirport*) { return true; }
};


typedef std::map < std::string, FGAirport* > airport_map;
typedef airport_map::iterator airport_map_iterator;
typedef airport_map::const_iterator const_airport_map_iterator;

typedef std::vector < FGAirport * > airport_list;
typedef airport_list::iterator airport_list_iterator;
typedef airport_list::const_iterator const_airport_list_iterator;



class FGAirportList {
private:

    airport_map airports_by_id;
    airport_list airports_array;

public:
    // Constructor (new)
    FGAirportList();

    // Destructor
    ~FGAirportList();

    // add an entry to the list
    FGAirport* add( const std::string& id, const SGGeod& location, const SGGeod& tower,
              const std::string& name, bool has_metar, bool is_airport,
              bool is_seaport, bool is_heliport );

    // search for the specified id.
    // Returns NULL if unsucessfull.
    FGAirport* search( const std::string& id );

    // Search for the next airport in ASCII sequence to the supplied id.
    // eg. id = "KDC" or "KDCA" would both return "KDCA".
    // If exact = true then only exact matches are returned.
    // NOTE: Numbers come prior to A-Z in ASCII sequence so id = "LD" would return "LD57", not "LDDP"
    // Implementation assumes airport codes are unique.
    // Returns NULL if unsucessfull.
    const FGAirport* findFirstById( const std::string& id, bool exact = false );

    // search for the airport closest to the specified position
    // (currently a linear inefficient search so it's probably not
    // best to use this at runtime.)  An FGAirportSearchFilter class
    // can be used to restrict the possible choices to a subtype.
    // max_range limits search - specified as an arc distance, in degrees
    FGAirport* search( double lon_deg, double lat_deg, double max_range );
    FGAirport* search( double lon_deg, double lat_deg, double max_range, FGAirportSearchFilter& );

    /**
     * Return the number of airports in the list.
     */
    int size() const;

    /**
     * Return a specific airport, by position.
     */
    const FGAirport *getAirport( unsigned int index ) const;

    /**
     * Return a pointer to the raw airport list
     */
    inline const airport_list* getAirportList() { return (&airports_array); }

    /**
     * Mark the specified airport record as not having metar
     */
    void no_metar( const std::string &id );

    /**
     * Mark the specified airport record as (yes) having metar
     */
    void has_metar( const std::string &id );

};

// find basic airport location info from airport database
const FGAirport *fgFindAirportID( const std::string& id);

// get airport elevation
double fgGetAirportElev( const std::string& id );

// get airport position
Point3D fgGetAirportPos( const std::string& id );

#endif // _FG_SIMPLE_HXX


