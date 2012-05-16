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
#include <vector>

#include <Navaids/positioned.hxx>

// forward decls
class FGAirportDynamics;
class FGRunway;
class FGTaxiway;
class FGPavement;
class SGPropertyNode;

typedef SGSharedPtr<FGRunway> FGRunwayPtr;
typedef SGSharedPtr<FGTaxiway> FGTaxiwayPtr;
typedef SGSharedPtr<FGPavement> FGPavementPtr;

namespace flightgear {
  class SID;
  class STAR;
  class Approach;
  class Waypt;
  class CommStation;

  typedef SGSharedPtr<Waypt> WayptRef;
  typedef std::vector<WayptRef> WayptVec;
  
  typedef std::vector<CommStation*> CommStationList;
}



/***************************************************************************************
 *
 **************************************************************************************/
class FGAirport : public FGPositioned
{
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

    static bool isAirportType(FGPositioned* pos);
    
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
     * return the most likely target runway based on a position.
     * Specifically, return the runway for which the course from aPos
     * to the runway end, mostly closely matches the runway heading.
     * This is a good approximation of which runway the position is on or
     * aiming towards.
     */
    FGRunway* findBestRunwayForPos(const SGGeod& aPos) const;
    
     /**
     * Useful predicate for FMS/GPS/NAV displays and similar - check if this
     * aiport has a hard-surfaced runway of at least the specified length.
     */
    bool hasHardRunwayOfLengthFt(double aLengthFt) const;

    unsigned int numTaxiways() const;
    FGTaxiway* getTaxiwayByIndex(unsigned int aIndex) const;

    unsigned int numPavements() const;
    FGPavement* getPavementByIndex(unsigned int aIndex) const;

    void setRunwaysAndTaxiways(std::vector<FGRunwayPtr>& rwys,
      std::vector<FGTaxiwayPtr>& txwys,
      std::vector<FGPavementPtr>& pvts);
    
    class AirportFilter : public Filter
     {
     public:
       virtual bool pass(FGPositioned* aPos) const { 
         return passAirport(static_cast<FGAirport*>(aPos));
       }
       
       virtual Type minType() const {
         return AIRPORT;
       }
       
       virtual Type maxType() const {
         return AIRPORT;
       }
       
       virtual bool passAirport(FGAirport* aApt) const {
         return true;
       }
     };
     
     /**
      * Filter which passes heliports and seaports in addition to airports
      */
     class PortsFilter : public AirportFilter
     {
     public:
       virtual Type maxType() const {
         return SEAPORT;
       }
     };
     
     class HardSurfaceFilter : public AirportFilter
     {
     public:
       HardSurfaceFilter(double minLengthFt = -1);
       
       virtual bool passAirport(FGAirport* aApt) const;
       
     private:
       double mMinLengthFt;
     };
     
     
     void setProcedures(const std::vector<flightgear::SID*>& aSids,
      const std::vector<flightgear::STAR*>& aStars,
      const std::vector<flightgear::Approach*>& aApproaches);
     
     void addSID(flightgear::SID* aSid);
      void addSTAR(flightgear::STAR* aStar);
      void addApproach(flightgear::Approach* aApp);

      unsigned int numSIDs() const;
      flightgear::SID* getSIDByIndex(unsigned int aIndex) const;
      flightgear::SID* findSIDWithIdent(const std::string& aIdent) const;
      
      unsigned int numSTARs() const;
      flightgear::STAR* getSTARByIndex(unsigned int aIndex) const;
      flightgear::STAR* findSTARWithIdent(const std::string& aIdent) const;
      
      unsigned int numApproaches() const;
      flightgear::Approach* getApproachByIndex(unsigned int aIndex) const;
      flightgear::Approach* findApproachWithIdent(const std::string& aIdent) const;
  
     /**
      * Syntactic wrapper around FGPositioned::findClosest - find the closest
      * match for filter, and return it cast to FGAirport. The default filter
      * passes airports, but not seaports or heliports
      */
     static FGAirport* findClosest(const SGGeod& aPos, double aCuttofNm, Filter* filter = NULL);
     
     /**
      * Helper to look up an FGAirport instance by unique ident. Throws an 
      * exception if the airport could not be found - so callers can assume
      * the result is non-NULL.
      */
     static FGAirport* getByIdent(const std::string& aIdent);
     
     /**
      * Helper to look up an FGAirport instance by unique ident. Returns NULL
      * if the airport could not be found.
      */
     static FGAirport* findByIdent(const std::string& aIdent);
     
     /**
      * Specialised helper to implement the AirportList dialog. Performs a
      * case-insensitive search on airport names and ICAO codes, and returns
      * matches in a format suitable for use by a puaList. 
      */
     static char** searchNamesAndIdents(const std::string& aFilter);
     
    void setCommStations(flightgear::CommStationList& comms);
    
    flightgear::CommStationList commStationsOfType(FGPositioned::Type aTy) const;
    
    const flightgear::CommStationList& commStations() const
        { return mCommStations; }
private:
    typedef std::vector<FGRunwayPtr>::const_iterator Runway_iterator;
    /**
     * Helper to locate a runway by ident
     */
    Runway_iterator getIteratorForRunwayIdent(const std::string& aIdent) const;

    // disable these
    FGAirport operator=(FGAirport &other);
    FGAirport(const FGAirport&);
  
    /**
     * helper to read airport data from the scenery XML files.
     */
    void loadSceneryDefinitions() const;
    
    /**
     * Helpers to process property data loaded from an ICAO.threshold.xml file
     */
    void readThresholdData(SGPropertyNode* aRoot);
    void processThreshold(SGPropertyNode* aThreshold);
    
    /**
     * Helper to parse property data loaded from an ICAO.twr.xml filke
     */
    void readTowerData(SGPropertyNode* aRoot);
    
    SGGeod _tower_location;
    std::string _name;
    bool _has_metar;
    FGAirportDynamics *_dynamics;

    void loadRunways() const;
    void loadTaxiways() const;
    void loadProcedures() const;
    
    mutable bool mRunwaysLoaded;
    mutable bool mTaxiwaysLoaded;
    mutable bool mProceduresLoaded;
    
    std::vector<FGRunwayPtr> mRunways;
    std::vector<FGTaxiwayPtr> mTaxiways;
    std::vector<FGPavementPtr> mPavements;
    
    std::vector<flightgear::SID*> mSIDs;
    std::vector<flightgear::STAR*> mSTARs;
    std::vector<flightgear::Approach*> mApproaches;
    
    flightgear::CommStationList mCommStations;
};

// find basic airport location info from airport database
const FGAirport *fgFindAirportID( const std::string& id);

// get airport elevation
double fgGetAirportElev( const std::string& id );

#endif // _FG_SIMPLE_HXX


