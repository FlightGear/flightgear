// airport.hxx -- a really simplistic class to manage airport ID,
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
#include <map>

#include <Navaids/positioned.hxx>
#include <Navaids/procedure.hxx>

#include "airports_fwd.hxx"
#include "runways.hxx"

/***************************************************************************************
 *
 **************************************************************************************/
class FGAirport : public FGPositioned
{
  public:
    FGAirport(PositionedID aGuid, const std::string& id, const SGGeod& location,
            const std::string& name, bool has_metar, Type aType);
    ~FGAirport();

    const std::string& getId() const { return ident(); }
    const std::string& getName() const { return _name; }
    std::string toString() const { return "an airport " + ident(); }

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

    /**
     * reload the ILS data from XML if required.
     */
    void validateILSData();

    SGGeod getTowerLocation() const;

    void setMetar(bool value) { _has_metar = value; }

    FGRunwayRef getActiveRunwayForUsage() const;

    FGAirportDynamics *getDynamics();
    
    unsigned int numRunways() const;
    unsigned int numHelipads() const;
    FGRunwayRef getRunwayByIndex(unsigned int aIndex) const;
    FGHelipadRef getHelipadByIndex(unsigned int aIndex) const;
    FGRunwayMap getRunwayMap() const;
    FGHelipadMap getHelipadMap() const;

    bool hasRunwayWithIdent(const std::string& aIdent) const;
    bool hasHelipadWithIdent(const std::string& aIdent) const;
    FGRunwayRef getRunwayByIdent(const std::string& aIdent) const;
    FGHelipadRef getHelipadByIdent(const std::string& aIdent) const;

    struct FindBestRunwayForHeadingParams {
      FindBestRunwayForHeadingParams() {
        lengthWeight =  0.01;
        widthWeight =  0.01;
        surfaceWeight =  10;
        deviationWeight =  1;
        ilsWeight = 0;
      }
      double lengthWeight;
      double widthWeight;
      double surfaceWeight;
      double deviationWeight;
      double ilsWeight;
    };
    FGRunwayRef findBestRunwayForHeading(double aHeading, struct FindBestRunwayForHeadingParams * parms = NULL ) const;
    
    /**
     * return the most likely target runway based on a position.
     * Specifically, return the runway for which the course from aPos
     * to the runway end, mostly closely matches the runway heading.
     * This is a good approximation of which runway the position is on or
     * aiming towards.
     */
    FGRunwayRef findBestRunwayForPos(const SGGeod& aPos) const;
    
    /**
     * Retrieve all runways at the airport, but excluding the reciprocal
     * runways. For example at KSFO this might return 1L, 1R, 28L and 28R,
     * but would not then include 19L/R or 10L/R.
     *
     * Exactly which runways you get, is undefined (i.e, dont assumes it's
     * runways with heading < 180 degrees) - it depends on order in apt.dat.
     *
     * This is useful for code that wants to process each piece of tarmac at
     * an airport *once*, not *twice* - eg mapping and nav-display code.
     */
    FGRunwayList getRunwaysWithoutReciprocals() const;
    
     /**
     * Useful predicate for FMS/GPS/NAV displays and similar - check if this
     * aiport has a hard-surfaced runway of at least the specified length.
     */
    bool hasHardRunwayOfLengthFt(double aLengthFt) const;

    unsigned int numTaxiways() const;
    FGTaxiwayRef getTaxiwayByIndex(unsigned int aIndex) const;
    FGTaxiwayList getTaxiways() const;

    unsigned int numPavements() const;
    FGPavementRef getPavementByIndex(unsigned int aIndex) const;
    FGPavementList getPavements() const;
    
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
     
     /**
      * Filter which passes specified port type and in case of airport checks
      * if a runway larger the /sim/navdb/min-runway-lenght-ft exists.
      */
     class TypeRunwayFilter:
       public AirportFilter
     {
       public:
         TypeRunwayFilter();

         /**
          * Construct from string containing type (airport, seaport or heliport)
          */
         bool fromTypeString(const std::string& type);

         virtual FGPositioned::Type minType() const { return _type; }
         virtual FGPositioned::Type maxType() const { return _type; }
         virtual bool pass(FGPositioned* pos) const;

       protected:
         FGPositioned::Type _type;
         double _min_runway_length_ft;
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
      flightgear::SIDList getSIDs() const;
      
      unsigned int numSTARs() const;
      flightgear::STAR* getSTARByIndex(unsigned int aIndex) const;
      flightgear::STAR* findSTARWithIdent(const std::string& aIdent) const;
      flightgear::STARList getSTARs() const;
      
      unsigned int numApproaches() const;
      flightgear::Approach* getApproachByIndex(unsigned int aIndex) const;
      flightgear::Approach* findApproachWithIdent(const std::string& aIdent) const;
      flightgear::ApproachList getApproaches
      (
        flightgear::ProcedureType type = flightgear::PROCEDURE_INVALID
      ) const;
  
     /**
      * Syntactic wrapper around FGPositioned::findClosest - find the closest
      * match for filter, and return it cast to FGAirport. The default filter
      * passes airports, but not seaports or heliports
      */
     static FGAirportRef findClosest(const SGGeod& aPos, double aCuttofNm, Filter* filter = NULL);
     
     /**
      * Helper to look up an FGAirport instance by unique ident. Throws an 
      * exception if the airport could not be found - so callers can assume
      * the result is non-NULL.
      */
     static FGAirportRef getByIdent(const std::string& aIdent);
     
     /**
      * Helper to look up an FGAirport instance by unique ident. Returns NULL
      * if the airport could not be found.
      */
     static FGAirportRef findByIdent(const std::string& aIdent);
     
     /**
      * Specialised helper to implement the AirportList dialog. Performs a
      * case-insensitive search on airport names and ICAO codes, and returns
      * matches in a format suitable for use by a puaList. 
      */
     static char** searchNamesAndIdents(const std::string& aFilter);
    
    
    /**
     * Sort an FGPositionedList of airports by size (number of runways + length)
     * this is meant to prioritise more important airports.
     */
    static void sortBySize(FGPositionedList&);
    
    flightgear::CommStationList commStationsOfType(FGPositioned::Type aTy) const;
    
    flightgear::CommStationList commStations() const;
private:
    static flightgear::AirportCache airportCache;

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
      
    void readILSData(SGPropertyNode* aRoot);
  
    void validateTowerData() const;
    
    /**
     * Helper to parse property data loaded from an ICAO.twr.xml file
     */
    void readTowerData(SGPropertyNode* aRoot);
  
    PositionedIDVec itemsOfType(FGPositioned::Type ty) const;
  
    std::string _name;
    bool _has_metar;
    FGAirportDynamics *_dynamics;

    void loadRunways() const;
    void loadHelipads() const;
    void loadTaxiways() const;
    void loadProcedures() const;
    
    mutable bool mTowerDataLoaded;
    mutable SGGeod mTowerPosition;
  
    mutable bool mRunwaysLoaded;
    mutable bool mHelipadsLoaded;
    mutable bool mTaxiwaysLoaded;
    mutable bool mProceduresLoaded;
  
    mutable bool mThresholdDataLoaded;
    bool mILSDataLoaded;

    mutable std::vector<FGRunwayRef> mRunways;
  
    mutable PositionedIDVec mHelipads;
    mutable PositionedIDVec mTaxiways;
    PositionedIDVec mPavements;
    
    typedef SGSharedPtr<flightgear::SID> SIDRef;
    typedef SGSharedPtr<flightgear::STAR> STARRef;
    typedef SGSharedPtr<flightgear::Approach> ApproachRef;
    
    std::vector<SIDRef> mSIDs;
    std::vector<STARRef> mSTARs;
    std::vector<ApproachRef> mApproaches;
  };

// find basic airport location info from airport database
const FGAirport *fgFindAirportID( const std::string& id);

// get airport elevation
double fgGetAirportElev( const std::string& id );

#endif // _FG_SIMPLE_HXX


