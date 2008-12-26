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

#include <simgear/compiler.h>

#include <string>
#include <map>
#include <vector>

#include "Navaids/positioned.hxx"

// forward decls
class FGAirportDynamics;
class FGRunway;

typedef SGSharedPtr<FGRunway> FGRunwayPtr;

/***************************************************************************************
 *
 **************************************************************************************/
class FGAirport : public FGPositioned
{
private:
    SGGeod _tower_location;
    std::string _name;
    bool _has_metar;
    FGAirportDynamics *_dynamics;

public:
    FGAirport(const std::string& id, const SGGeod& location, const SGGeod& tower, 
            const std::string& name, bool has_metar, Type aType);
    ~FGAirport();

    const std::string& getId() const { return ident(); }
    const std::string& getName() const { return _name; }
    double getLongitude() const { return longitude(); }
    // Returns degrees
    double getLatitude()  const { return latitude(); }
    // Returns ft
    double getElevation() const { return elevation(); }
    bool   getMetar()     const { return _has_metar; }
    bool   isAirport()    const;
    bool   isSeaport()    const;
    bool   isHeliport()   const;

    virtual const std::string& name() const
    { return _name; }

    const SGGeod& getTowerLocation() const { return _tower_location; }

    void setMetar(bool value) { _has_metar = value; }

    FGRunway* getActiveRunwayForUsage() const;

    FGAirportDynamics *getDynamics();
    
    unsigned int numRunways() const;
    FGRunway* getRunwayByIndex(unsigned int aIndex) const;

    bool hasRunwayWithIdent(const std::string& aIdent) const;
    FGRunway* getRunwayByIdent(const std::string& aIdent) const;
    FGRunway* findBestRunwayForHeading(double aHeading) const;
    
     /**
     * Useful predicate for FMS/GPS/NAV displays and similar - check if this
     * aiport has a hard-surfaced runway of at least the specified length.
     */
    bool hasHardRunwayOfLengthFt(double aLengthFt) const;
    
    unsigned int numTaxiways() const;
    FGRunway* getTaxiwayByIndex(unsigned int aIndex) const;
    
    void addRunway(FGRunway* aRunway);
    
    class AirportFilter : public Filter
     {
     public:
       virtual bool pass(FGPositioned* aPos) const { 
         Type ty(aPos->type());
         return (ty >= AIRPORT) && (ty <= SEAPORT);
       }
     };
     
     class HardSurfaceFilter : public Filter
     {
     public:
       HardSurfaceFilter(double minLengthFt);
       
       virtual bool pass(FGPositioned* aPos) const;
     private:
       double mMinLengthFt;
     };
     
     /**
      * Syntactic wrapper around FGPositioned::findClosest - find the closest
      * match for filter, and return it cast to FGAirport. The default filter
      * passes all airports, including seaports and heliports.
      */
     static FGAirport* findClosest(const SGGeod& aPos, double aCuttofNm, Filter* filter = NULL);
private:
    typedef std::vector<FGRunwayPtr>::const_iterator Runway_iterator;
    /**
     * Helper to locate a runway by ident
     */
    Runway_iterator getIteratorForRunwayIdent(const std::string& aIdent) const;

    FGAirport operator=(FGAirport &other);
    FGAirport(const FGAirport&);
    
    std::vector<FGRunwayPtr> mRunways;
    std::vector<FGRunwayPtr> mTaxiways;
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
              const std::string& name, bool has_metar, FGPositioned::Type aType);

    // search for the specified id.
    // Returns NULL if unsucessfull.
    FGAirport* search( const std::string& id );

    /**
     * Return the number of airports in the list.
     */
    int size() const;

    /**
     * Return a specific airport, by position.
     */
    const FGAirport *getAirport( unsigned int index ) const;

};

// find basic airport location info from airport database
const FGAirport *fgFindAirportID( const std::string& id);

// get airport elevation
double fgGetAirportElev( const std::string& id );

#endif // _FG_SIMPLE_HXX


