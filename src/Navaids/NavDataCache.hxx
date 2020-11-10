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
#include <cstddef>                   // for std::size_t
#include <functional>

#include <simgear/misc/strutils.hxx> // for string_list
#include <Navaids/positioned.hxx>
#include <Main/globals.hxx>          // for PathList

class SGPath;
class FGRunway;

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

    class Airway;
    using AirwayRef = SGSharedPtr<Airway>;

class NavDataCache
{
public:
    ~NavDataCache();

// singleton accessor
    static NavDataCache* instance();

// static creator
    static NavDataCache* createInstance();

    SGPath path() const;

    enum DatFileType {
      DATFILETYPE_APT = 0,
      DATFILETYPE_METAR,
      DATFILETYPE_AWY,
      DATFILETYPE_NAV,
      DATFILETYPE_FIX,
      DATFILETYPE_POI,
      DATFILETYPE_CARRIER,
      DATFILETYPE_TACAN_FREQ,
      DATFILETYPE_LAST
    };

    struct DatFilesGroupInfo {
      DatFileType datFileType;      // for instance, DATFILETYPE_APT
      PathList paths;               // SGPath instances
      std::size_t totalSize;        // total size of all these files, in bytes
    };

    // datTypeStr[DATFILETYPE_APT] = std::string("apt"), etc. This gives,
    // among other things, the subdirectory of $scenery_path/NavData where
    // each type of dat file is looked for.
    static const std::string datTypeStr[];
    // defaultDatFile[DATFILETYPE_APT] = std::string("Airports/apt.dat.gz"),
    // etc. This tells where to find the historical dat files: those under
    // $FG_ROOT.
    static const std::string defaultDatFile[];

    // Update d->datFilesInfo and legacy d->metarDatPath, d->poiDatPath,
    // etc. by looking into $scenery_path/NavData for each scenery path.
    void updateListsOfDatFiles();
    // Returns datFilesInfo for the given type.
    const DatFilesGroupInfo& getDatFilesInfo(DatFileType datFileType) const;

  /**
   * predicate - check if the cache needs to be rebuilt.
   * This can happen is the cache file is missing or damaged, or one of the
   ** global input files is changed.
   */
    bool isRebuildRequired();

    static bool isAnotherProcessRebuilding();

    enum RebuildPhase
    {
        REBUILD_UNKNOWN = 0,
        REBUILD_READING_APT_DAT_FILES,
        REBUILD_LOADING_AIRPORTS,
        REBUILD_NAVAIDS,
        REBUILD_FIXES,
        REBUILD_POIS,
        REBUILD_DONE
    };

  /**
   * run the cache rebuild - returns the current phase or 'done'
   */
  RebuildPhase rebuild();

  unsigned int rebuildPhaseCompletionPercentage() const;
  void setRebuildPhaseProgress(RebuildPhase ph, unsigned int percent = 0);

  bool isCachedFileModified(const SGPath& path) const;
  void stampCacheFile(const SGPath& path);

  int readIntProperty(const std::string& key);
  double readDoubleProperty(const std::string& key);
  std::string readStringProperty(const std::string& key);

  void writeIntProperty(const std::string& key, int value);
  void writeStringProperty(const std::string& key, const std::string& value);
  void writeDoubleProperty(const std::string& key, const double& value);

  // Warning: order is not necessarily preserved upon write-read cycles!
  string_list readStringListProperty(const std::string& key);
  void writeStringListProperty(const std::string& key, const string_list& values);

  // These ones guarantee the same order after each write-read cycle.
  string_list readOrderedStringListProperty(const std::string& key,
                                            const char* separator);
  void writeOrderedStringListProperty(const std::string& key,
                                      const string_list& values,
                                      const char* separator);

  /**
   * retrieve an FGPositioned from the cache.
   * This may be trivial if the object is previously loaded, or require actual
   * disk IO.
   */
  FGPositionedRef loadById(PositionedID guid);

  PositionedID insertAirport(FGPositioned::Type ty, const std::string& ident,
                             const std::string& name);
  void insertTower(PositionedID airportId, const SGGeod& pos);


  PositionedID insertRunway(FGPositioned::Type ty, const string& ident,
                             const SGGeod& pos, PositionedID apt,
                             double heading, double length, double width, double displacedThreshold,
                             double stopway, int markings, int approach, int tdz, int reil,
                             int surfaceCode, int shoulder_code, float smoothness, int center_lights,
                             int edge_lights, int distance_remaining);

  PositionedID insertRunway(FGPositioned::Type ty, const string& ident,
                            const SGGeod& pos, PositionedID apt,
                            double heading, double length, double width, double displacedThreshold,
                            double stopway, int surfaceCode);

  void setRunwayReciprocal(PositionedID runway, PositionedID recip);
  void setRunwayILS(PositionedID runway, PositionedID ils);

  PositionedID insertNavaid(FGPositioned::Type ty, const std::string& ident,
                            const std::string& name, const SGGeod& pos, int freq, int range, double multiuse,
                            PositionedID apt, PositionedID runway);

  // Assign colocated DME to a navaid
  void setNavaidColocated(PositionedID navaid, PositionedID colocatedDME);

  PositionedID insertCommStation(FGPositioned::Type ty,
                                 const std::string& name, const SGGeod& pos, int freq, int range,
                                PositionedID apt);
  PositionedID insertFix(const std::string& ident, const SGGeod& aPos);

  PositionedID createPOI(FGPositioned::Type ty, const std::string& ident, const SGGeod& aPos);

  bool removePOI(FGPositioned::Type ty, const std::string& aIdent);

  /// update the metar flag associated with an airport
  void setAirportMetar(const std::string& icao, bool hasMetar);

  /**
   * Modify the position of an existing item.
   */
  void updatePosition(PositionedID item, const SGGeod &pos);

  FGPositionedList findAllWithIdent( const std::string& ident,
                                     FGPositioned::Filter* filter,
                                     bool exact );
  FGPositionedList findAllWithName( const std::string& ident,
                                    FGPositioned::Filter* filter,
                                    bool exact );

  FGPositionedRef findClosestWithIdent( const std::string& aIdent,
                                        const SGGeod& aPos,
                                        FGPositioned::Filter* aFilter );


  /**
   * Helper to implement the AirportSearch widget. Optimised text search of
   * airport names and idents, returning a list suitable for passing directly
   * to PLIB.
   */
  char** searchAirportNamesAndIdents(const std::string& aFilter);

  /**
   * Find the closest matching comm-station on a frequency, to a position.
   * The filter with be used for both type ranging and to validate the result
   * candidates.
   */
  FGPositionedRef findCommByFreq(int freqKhz, const SGGeod& pos, FGPositioned::Filter* filt);

  /**
   * find all items of a specified type (or range of types) at an airport
   */
  PositionedIDVec airportItemsOfType(PositionedID apt, FGPositioned::Type ty,
                                     FGPositioned::Type maxTy = FGPositioned::INVALID);

  /**
   * find the first match item of the specified type and ident, at an airport
   */
  PositionedID airportItemWithIdent(PositionedID apt, FGPositioned::Type ty, const std::string& ident);

  /**
   * Find all navaids matching a particular frequency, sorted by range from the
   * supplied position. Type-range will be determined from the filter
   */
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
  int findAirway(int network, const std::string& aName, bool create);

  int findAirway(const std::string& aName);

  /**
   * insert an edge between two positioned nodes, into the network.
   * The airway identifier will be set accordingly. No reverse edge is created
   * by this method - edges are directional so a reverses must be explicitly
   * created.
   */
  void insertEdge(int network, int airwayID, PositionedID from, PositionedID to);

  /// is the specified positioned a node on the network?
  bool isInAirwayNetwork(int network, PositionedID pos);

  /**
   * retrive all the destination points reachable from a positioned
   * in an airway
   */
  AirwayEdgeVec airwayEdgesFrom(int network, PositionedID pos);

    AirwayRef loadAirway(int airwayID);

    /**
     * Waypoints on the airway
     */
  PositionedIDVec airwayWaypts(int id);

    class Transaction
    {
    public:
        Transaction(NavDataCache* cache);
        ~Transaction();

        void commit();
    private:
        NavDataCache* _instance;
        bool _committed;
    };

    bool isReadOnly() const;

    class ThreadedGUISearch
    {
    public:
        ThreadedGUISearch(const std::string& term, bool onlyAirports);
        ~ThreadedGUISearch();

        PositionedIDVec results() const;

        bool isComplete() const;
    private:
        class ThreadedGUISearchPrivate;
        std::unique_ptr<ThreadedGUISearchPrivate> d;
    };

    void clearDynamicPositioneds();

private:
  NavDataCache();

  friend class RebuildThread;

  // A generic function for loading all navigation data files of the
  // specified type (apt/fix/nav etc.) using the passed type-specific loader.
  void loadDatFiles(DatFileType type,
                    std::function<void(const SGPath&, std::size_t, std::size_t)> loader);

  void doRebuild();

  friend class Transaction;

    void beginTransaction();
    void commitTransaction();
    void abortTransaction();

  class NavDataCachePrivate;
  std::unique_ptr<NavDataCachePrivate> d;

  bool rebuildInProgress = false;
};

} // of namespace flightgear

#endif // of FG_NAVDATACACHE_HXX
