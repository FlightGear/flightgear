/*
 * airports_fwd.hxx
 *
 *  Created on: 04.03.2013
 *      Author: tom
 */

#ifndef AIRPORTS_FWD_HXX_
#define AIRPORTS_FWD_HXX_

#include <simgear/structure/SGSharedPtr.hxx>

#include <list>
#include <map>
#include <vector>
#include <string>

// forward decls
class FGAirport;
class FGAirportDynamics;
class FGRunway;
class FGHelipad;
class FGTaxiway;
class FGPavement;

class FGNavRecord;

class Block;
class FGTaxiNode;
class FGParking;
class FGTaxiSegment;
class FGTaxiRoute;
class FGGroundNetwork;

class RunwayList;
class RunwayGroup;
class FGRunwayPreference;

class FGSidStar;

class SGPropertyNode;

namespace flightgear {
  class SID;
  class STAR;
  class Approach;
  class Waypt;
  class CommStation;

  typedef std::vector<flightgear::SID*> SIDList;
  typedef std::vector<STAR*> STARList;
  typedef std::vector<Approach*> ApproachList;

  typedef SGSharedPtr<Waypt> WayptRef;
  typedef std::vector<WayptRef> WayptVec;

  typedef SGSharedPtr<CommStation> CommStationRef;
  typedef std::vector<CommStation*> CommStationList;
  typedef std::map<std::string, FGAirport*> AirportCache;
}

typedef std::vector<FGRunway*> FGRunwayList;
typedef std::map<std::string, FGRunway*> FGRunwayMap;
typedef std::map<std::string, FGHelipad*> FGHelipadMap;

typedef std::vector<FGTaxiway*> FGTaxiwayList;
typedef std::vector<FGPavement*> FGPavementList;

typedef std::vector<FGTaxiSegment*>  FGTaxiSegmentVector;
typedef FGTaxiSegmentVector::iterator FGTaxiSegmentVectorIterator;

typedef SGSharedPtr<FGTaxiNode> FGTaxiNodeRef;
typedef std::vector<FGTaxiNodeRef> FGTaxiNodeVector;
typedef FGTaxiNodeVector::iterator FGTaxiNodeVectorIterator;
typedef std::map<int, FGTaxiNodeRef> IndexTaxiNodeMap;

typedef std::vector<Block> BlockList;
typedef BlockList::iterator BlockListIterator;

typedef std::vector<time_t> TimeVector;
typedef std::vector<time_t>::iterator TimeVectorIterator;

typedef std::vector<FGTaxiRoute> TaxiRouteVector;
typedef std::vector<FGTaxiRoute>::iterator TaxiRouteVectorIterator;

typedef std::vector<FGParking*> FGParkingVec;
typedef FGParkingVec::iterator FGParkingVecIterator;
typedef FGParkingVec::const_iterator FGParkingVecConstIterator;

typedef std::vector<RunwayList> RunwayListVec;
typedef std::vector<RunwayList>::iterator RunwayListVectorIterator;
typedef std::vector<RunwayList>::const_iterator RunwayListVecConstIterator;

typedef std::vector<RunwayGroup> PreferenceList;
typedef std::vector<RunwayGroup>::iterator PreferenceListIterator;
typedef std::vector<RunwayGroup>::const_iterator PreferenceListConstIterator;

#endif /* AIRPORTS_FWD_HXX_ */
