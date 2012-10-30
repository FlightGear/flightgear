/**
 * NavDataCache.hxx - defines a unified binary cache for navigation
 * data, parsed from various text / XML sources.
 */
 
// Written by James Turner, started 2012.
//
// Copyright (C) 2012 James Turner
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

#ifndef FG_NAVDATACACHE_HXX
#define FG_NAVDATACACHE_HXX

#include <memory>

#include <simgear/misc/strutils.hxx> // for string_list
#include <Navaids/positioned.hxx>
    
class SGPath;

namespace flightgear
{
  
/// a pair of airport ID, runway ID
typedef std::pair<PositionedID, PositionedID> AirportRunwayPair;
  
typedef std::pair<FGPositioned::Type, PositionedID> TypedPositioned;
typedef std::vector<TypedPositioned> TypedPositionedVec;

// pair of airway ID, destination node ID
typedef std::pair<int, PositionedID> AirwayEdge;
typedef std::vector<AirwayEdge> AirwayEdgeVec;
  
namespace Octree {
  class Node;
  class Branch;
}
  
class NavDataCache
{
public:
    ~NavDataCache();
    
// singleton accessor
    static NavDataCache* instance();
          
  /**
   * predicate - check if the cache needs to be rebuilt.
   * This can happen is the cache file is missing or damaged, or one of the
   ** global input files is changed.
   */
  bool isRebuildRequired();
  
  /**
   * run the cache rebuild - returns true if rebuild is complete,
   * otherwise keep going.
   */
  bool rebuild();
  
  bool isCachedFileModified(const SGPath& path) const;
  void stampCacheFile(const SGPath& path);
  
  int readIntProperty(const std::string& key);
  double readDoubleProperty(const std::string& key);
  std::string readStringProperty(const std::string& key);
  
  void writeIntProperty(const std::string& key, int value);
  void writeStringProperty(const std::string& key, const std::string& value);
  void writeDoubleProperty(const std::string& key, const double& value);
  
  string_list readStringListProperty(const std::string& key);
  void writeStringListProperty(const std::string& key, const string_list& values);
  
  FGPositioned* loadById(PositionedID guid);
  
  PositionedID insertAirport(FGPositioned::Type ty, const std::string& ident,
                             const std::string& name);
  void insertTower(PositionedID airportId, const SGGeod& pos);
  PositionedID insertRunway(FGPositioned::Type ty, const std::string& ident,
                          const SGGeod& pos, PositionedID apt,
                          double heading, double length, double width, double displacedThreshold,
                          double stopway, int surfaceCode);
  void setRunwayReciprocal(PositionedID runway, PositionedID recip);
  void setRunwayILS(PositionedID runway, PositionedID ils);
  
  void updateRunwayThreshold(PositionedID runwayID, const SGGeod &aThreshold,
                    double aHeading, double aDisplacedThreshold,
                    double aStopway);
  
  PositionedID insertNavaid(FGPositioned::Type ty, const std::string& ident,
                            const std::string& name, const SGGeod& pos, int freq, int range, double multiuse,
                            PositionedID apt, PositionedID runway);
  void updateILS(PositionedID ils, const SGGeod& newPos, double aHdg);
  
  PositionedID insertCommStation(FGPositioned::Type ty,
                                 const std::string& name, const SGGeod& pos, int freq, int range,
                                PositionedID apt);
  PositionedID insertFix(const std::string& ident, const SGGeod& aPos);
  
  PositionedID createUserWaypoint(const std::string& ident, const SGGeod& aPos);
  
  void setAirportMetar(const std::string& icao, bool hasMetar);
  
  void updatePosition(PositionedID item, const SGGeod &pos);
  
  FGPositioned::List findAllWithIdent(const std::string& ident,
                                      FGPositioned::Filter* filter, bool exact);
  FGPositioned::List findAllWithName(const std::string& ident,
                                      FGPositioned::Filter* filter, bool exact);
  
  FGPositionedRef findClosestWithIdent(const std::string& aIdent,
                                      const SGGeod& aPos, FGPositioned::Filter* aFilter);
  

  char** searchAirportNamesAndIdents(const std::string& aFilter);
  
  FGPositionedRef findCommByFreq(int freqKhz, const SGGeod& pos, FGPositioned::Filter* filt);
  
  PositionedIDVec airportItemsOfType(PositionedID apt, FGPositioned::Type ty,
                                     FGPositioned::Type maxTy = FGPositioned::INVALID);
    
  PositionedID airportItemWithIdent(PositionedID apt, FGPositioned::Type ty, const std::string& ident);
  
  PositionedIDVec findNavaidsByFreq(int freqKhz, const SGGeod& pos, FGPositioned::Filter* filt);
  
  /// overload version of the above that does not consider positioned when
  /// returning results. Only used by TACAN carrier search
  PositionedIDVec findNavaidsByFreq(int freqKhz, FGPositioned::Filter* filt);
  
  /**
   * Given a runway and type, find the corresponding navaid (ILS / GS / OM)
   */
  PositionedID findNavaidForRunway(PositionedID runway, FGPositioned::Type ty);
  
  /**
   * given a navaid name (or similar) from apt.dat / nav.dat, find the
   * corresponding airport and runway IDs.
   * Such names look like: 'LHBP 31L DME-ILS' or 'UEEE 23L MM'
   */
  AirportRunwayPair findAirportRunway(const std::string& name);
  
  /**
   * Given an aiport, and runway and ILS identifier, find the corresponding cache
   * entry. This matches the data we get in the ils.xml files for airports.
   */
  PositionedID findILS(PositionedID airport, const std::string& runway, const std::string& navIdent);
  
  /**
   * Given an Octree node ID, return a bit-mask defining which of the child
   * nodes exist. In practice this means an 8-bit value be sufficent, but
   * an int works fine too.
   */
  int getOctreeBranchChildren(int64_t octreeNodeId);
  
  void defineOctreeNode(Octree::Branch* pr, Octree::Node* nd);
    
  /**
   * given an octree leaf, return all its child positioned items and their types
   */
  TypedPositionedVec getOctreeLeafChildren(int64_t octreeNodeId);
  
// airways
  int findAirway(int network, const std::string& aName);
  
  void insertEdge(int network, int airwayID, PositionedID from, PositionedID to);
  
  bool isInAirwayNetwork(int network, PositionedID pos);
  
  /**
   * retrive all the destination points reachcable from a positioned
   * in an airway
   */
  AirwayEdgeVec airwayEdgesFrom(int network, PositionedID pos);
private:
  NavDataCache();
  
  friend class RebuildThread;
  void doRebuild();
  
  class NavDataCachePrivate;
  std::auto_ptr<NavDataCachePrivate> d;      
};
  
} // of namespace flightgear

#endif // of FG_NAVDATACACHE_HXX

