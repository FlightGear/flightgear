// NavDataCache.cxx - defines a unified binary cache for navigation
// data, parsed from various text / XML sources.

// Written by James Turner, started 2012.
//
// Copyright (C) 2012  James Turner
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

// to ensure compatability between sqlite3_int64 and PositionedID,
// force the type used by sqlite to match PositionedID explicitly
#define SQLITE_INT64_TYPE int64_t
#define SQLITE_UINT64_TYPE uint64_t

#include "NavDataCache.hxx"

// std
#include <map>
#include <cassert>
#include <stdint.h> // for int64_t
// boost
#include <boost/foreach.hpp>

#include "sqlite3.h"

// SimGear
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/threads/SGGuard.hxx>

#include <Main/globals.hxx>
#include "markerbeacon.hxx"
#include "navrecord.hxx"
#include <Airports/simple.hxx>
#include <Airports/runways.hxx>
#include <ATC/CommStation.hxx>
#include "fix.hxx"
#include <Navaids/fixlist.hxx>
#include <Navaids/navdb.hxx>
#include "PositionedOctree.hxx"
#include <Airports/apt_loader.hxx>
#include <Navaids/airways.hxx>
#include <Airports/parking.hxx>
#include <Airports/gnnode.hxx>

using std::string;

#define SG_NAVCACHE SG_GENERAL
//#define LAZY_OCTREE_UPDATES 1

namespace {

const int SCHEMA_VERSION = 5;

// bind a std::string to a sqlite statement. The std::string must live the
// entire duration of the statement execution - do not pass a temporary
// std::string, or the compiler may delete it, freeing the C-string storage,
// and causing subtle memory corruption bugs!
void sqlite_bind_stdstring(sqlite3_stmt* stmt, int value, const std::string& s)
{
  sqlite3_bind_text(stmt, value, s.c_str(), s.length(), SQLITE_STATIC);
}

// variant of the above, which does not care about the lifetime of the
// passed std::string
void sqlite_bind_temp_stdstring(sqlite3_stmt* stmt, int value, const std::string& s)
{
  sqlite3_bind_text(stmt, value, s.c_str(), s.length(), SQLITE_TRANSIENT);
}
  
typedef sqlite3_stmt* sqlite3_stmt_ptr;

void f_distanceCartSqrFunction(sqlite3_context* ctx, int argc, sqlite3_value* argv[])
{
  if (argc != 6) {
    return;
  }
  
  SGVec3d posA(sqlite3_value_double(argv[0]),
               sqlite3_value_double(argv[1]),
               sqlite3_value_double(argv[2]));
  
  SGVec3d posB(sqlite3_value_double(argv[3]),
               sqlite3_value_double(argv[4]),
               sqlite3_value_double(argv[5]));
  sqlite3_result_double(ctx, distSqr(posA, posB));
}
  
  
static string cleanRunwayNo(const string& aRwyNo)
{
  if (aRwyNo[0] == 'x') {
    return string(); // no ident for taxiways
  }
  
  string result(aRwyNo);
  // canonicalise runway ident
  if ((aRwyNo.size() == 1) || !isdigit(aRwyNo[1])) {
    result = "0" + aRwyNo;
  }
  
  // trim off trailing garbage
  if (result.size() > 2) {
    char suffix = toupper(result[2]);
    if (suffix == 'X') {
      result = result.substr(0, 2);
    }
  }
  
  return result;
}
  
} // anonymous namespace

namespace flightgear
{

/**
 * Thread encapsulating a cache rebuild. This is not used to parallelise
 * the rebuild - we must still wait until completion before doing other
 * startup, since many things rely on a complete cache. The thread is used
 * so we don't block the main event loop for an unacceptable duration,
 * which causes 'not responding' / spinning beachballs on Windows & Mac
 */
class RebuildThread : public SGThread
{
public:
  RebuildThread(NavDataCache* cache) :
  _cache(cache),
  _isFinished(false)
  {
    
  }
  
  bool isFinished() const
  {
    SGGuard<SGMutex> g(_lock);
    return _isFinished;
  }
  
  virtual void run()
  {
    SGTimeStamp st;
    st.stamp();
    _cache->doRebuild();
    SG_LOG(SG_GENERAL, SG_INFO, "cache rebuild took:" << st.elapsedMSec() << "msec");
    
    SGGuard<SGMutex> g(_lock);
    _isFinished = true;
  }
private:
  NavDataCache* _cache;
  mutable SGMutex _lock;
  bool _isFinished;
};

////////////////////////////////////////////////////////////////////////////
  
typedef std::map<PositionedID, FGPositionedRef> PositionedCache;
  
class AirportTower : public FGPositioned
{
public:
  AirportTower(PositionedID& guid, PositionedID airport,
               const string& ident, const SGGeod& pos) :
    FGPositioned(guid, FGPositioned::TOWER, ident, pos)
  {
  }
};

class NavDataCache::NavDataCachePrivate
{
public:
  NavDataCachePrivate(const SGPath& p, NavDataCache* o) :
    outer(o),
    db(NULL),
    path(p),
    cacheHits(0),
    cacheMisses(0)
  {
  }
  
  ~NavDataCachePrivate()
  {
    BOOST_FOREACH(sqlite3_stmt_ptr stmt, prepared) {
      sqlite3_finalize(stmt);
    }
    prepared.clear();
    
    sqlite3_close(db);
  }
  
  void init()
  {
    SG_LOG(SG_NAVCACHE, SG_INFO, "NavCache at:" << path);
    sqlite3_open_v2(path.c_str(), &db,
                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    
    
    sqlite3_stmt_ptr checkTables =
      prepare("SELECT count(*) FROM sqlite_master WHERE name='properties'");
    
    sqlite3_create_function(db, "distanceCartSqr", 6, SQLITE_ANY, NULL,
                            f_distanceCartSqrFunction, NULL, NULL);
    
    execSelect(checkTables);
    bool didCreate = false;
    if (sqlite3_column_int(checkTables, 0) == 0) {
      SG_LOG(SG_NAVCACHE, SG_INFO, "will create tables");
      initTables();
      didCreate = true;
    }
    
    readPropertyQuery = prepare("SELECT value FROM properties WHERE key=?");
    writePropertyQuery = prepare("INSERT OR REPLACE INTO properties "
                                 "(key, value) VALUES (?,?)");
    
    if (didCreate) {
      writeIntProperty("schema-version", SCHEMA_VERSION);
    } else {
      int schemaVersion = outer->readIntProperty("schema-version");
      if (schemaVersion != SCHEMA_VERSION) {
        SG_LOG(SG_NAVCACHE, SG_INFO, "Navcache schema mismatch, will rebuild");
        throw sg_exception("Navcache schema has changed");
      }
    }
    
    prepareQueries();
  }
  
  void checkCacheFile()
  {
    SG_LOG(SG_NAVCACHE, SG_INFO, "running DB integrity check");
    SGTimeStamp st;
    st.stamp();
    
    sqlite3_stmt_ptr stmt = prepare("PRAGMA integrity_check(1)");
    if (!execSelect(stmt)) {
      throw sg_exception("DB integrity check failed to run");
    }
    
    string v = (char*) sqlite3_column_text(stmt, 0);
    if (v != "ok") {
      throw sg_exception("DB integrity check returned:" + v);
    }
    
    SG_LOG(SG_NAVCACHE, SG_INFO, "NavDataCache integrity check took:" << st.elapsedMSec());
    finalize(stmt);
  }
  
  void callSqlite(int result, const string& sql)
  {
    if (result == SQLITE_OK)
      return; // all good
    
    string errMsg;
    if (result == SQLITE_MISUSE) {
      errMsg = "Sqlite API abuse";
      SG_LOG(SG_NAVCACHE, SG_ALERT, "Sqlite API abuse");
    } else {
      errMsg = sqlite3_errmsg(db);
      SG_LOG(SG_NAVCACHE, SG_ALERT, "Sqlite error:" << errMsg << " running:\n\t" << sql);
    }
    
    throw sg_exception("Sqlite error:" + errMsg, sql);
  }
  
  void runSQL(const string& sql)
  {
    sqlite3_stmt_ptr stmt;
    callSqlite(sqlite3_prepare_v2(db, sql.c_str(), sql.length(), &stmt, NULL), sql);
    
    try {
      execSelect(stmt);
    } catch (sg_exception&) {
      sqlite3_finalize(stmt);
      throw; // re-throw
    }
    
    sqlite3_finalize(stmt);
  }
  
  sqlite3_stmt_ptr prepare(const string& sql)
  {
    sqlite3_stmt_ptr stmt;
    callSqlite(sqlite3_prepare_v2(db, sql.c_str(), sql.length(), &stmt, NULL), sql);
    prepared.push_back(stmt);
    return stmt;
  }
  
  void finalize(sqlite3_stmt_ptr s)
  {
    StmtVec::iterator it = std::find(prepared.begin(), prepared.end(), s);
    if (it == prepared.end()) {
      throw sg_exception("Finalising statement that was not prepared");
    }
    
    prepared.erase(it);
    sqlite3_finalize(s);
  }
  
  void reset(sqlite3_stmt_ptr stmt)
  {
    assert(stmt);
    if (sqlite3_reset(stmt) != SQLITE_OK) {
      string errMsg = sqlite3_errmsg(db);
      SG_LOG(SG_NAVCACHE, SG_ALERT, "Sqlite error resetting:" << errMsg);
      throw sg_exception("Sqlite error resetting:" + errMsg, sqlite3_sql(stmt));
    }
  }
  
  bool execSelect(sqlite3_stmt_ptr stmt)
  {
    return stepSelect(stmt);
  }
  
  bool stepSelect(sqlite3_stmt_ptr stmt)
  {
    int result = sqlite3_step(stmt);
    if (result == SQLITE_ROW) {
      return true; // at least one result row
    }
    
    if (result == SQLITE_DONE) {
      return false; // no result rows
    }
    
    string errMsg;
    if (result == SQLITE_MISUSE) {
      errMsg = "Sqlite API abuse";
      SG_LOG(SG_NAVCACHE, SG_ALERT, "Sqlite API abuse");
    } else {
      errMsg = sqlite3_errmsg(db);
      SG_LOG(SG_NAVCACHE, SG_ALERT, "Sqlite error:" << errMsg
             << " while running:\n\t" << sqlite3_sql(stmt));
    }
    
    throw sg_exception("Sqlite error:" + errMsg, sqlite3_sql(stmt));
  }
  
  void execSelect1(sqlite3_stmt_ptr stmt)
  {
    if (!execSelect(stmt)) {
      SG_LOG(SG_NAVCACHE, SG_WARN, "empty SELECT running:\n\t" << sqlite3_sql(stmt));
      throw sg_exception("no results returned for select", sqlite3_sql(stmt));
    }
  }
  
  sqlite3_int64 execInsert(sqlite3_stmt_ptr stmt)
  {
    execSelect(stmt);
    return sqlite3_last_insert_rowid(db);
  }
  
  void execUpdate(sqlite3_stmt_ptr stmt)
  {
    execSelect(stmt);
  }
  
  void initTables()
  {
    runSQL("CREATE TABLE properties ("
           "key VARCHAR,"
           "value VARCHAR"
           ")");
    
    runSQL("CREATE TABLE stat_cache ("
           "path VARCHAR unique,"
           "stamp INT"
           ")");
    
    runSQL("CREATE TABLE positioned ("
           "type INT,"
           "ident VARCHAR collate nocase,"
           "name VARCHAR collate nocase,"
           "airport INT64,"
           "lon FLOAT,"
           "lat FLOAT,"
           "elev_m FLOAT,"
           "octree_node INT,"
           "cart_x FLOAT,"
           "cart_y FLOAT,"
           "cart_z FLOAT"
           ")");
    
    runSQL("CREATE INDEX pos_octree ON positioned(octree_node)");
    runSQL("CREATE INDEX pos_ident ON positioned(ident collate nocase)");
    runSQL("CREATE INDEX pos_name ON positioned(name collate nocase)");
    // allow efficient querying of 'all ATIS at this airport' or
    // 'all towers at this airport'
    runSQL("CREATE INDEX pos_apt_type ON positioned(airport, type)");
    
    runSQL("CREATE TABLE airport ("
           "has_metar BOOL"
           ")"
           );
    
    runSQL("CREATE TABLE comm ("
           "freq_khz INT,"
           "range_nm INT"
           ")"
           );
    
    runSQL("CREATE INDEX comm_freq ON comm(freq_khz)");
    
    runSQL("CREATE TABLE runway ("
           "heading FLOAT,"
           "length_ft FLOAT,"
           "width_m FLOAT,"
           "surface INT,"
           "displaced_threshold FLOAT,"
           "stopway FLOAT,"
           "reciprocal INT64,"
           "ils INT64"
           ")"
           );
    
    runSQL("CREATE TABLE navaid ("
           "freq INT,"
           "range_nm INT,"
           "multiuse FLOAT,"
           "runway INT64,"
           "colocated INT64"
           ")"
           );
    
    runSQL("CREATE INDEX navaid_freq ON navaid(freq)");
    
    runSQL("CREATE TABLE octree (children INT)");
    
    runSQL("CREATE TABLE airway ("
           "ident VARCHAR collate nocase,"
           "network INT" // high-level or low-level
           ")");
    
    runSQL("CREATE INDEX airway_ident ON airway(ident)");
    
    runSQL("CREATE TABLE airway_edge ("
           "network INT,"
           "airway INT64,"
           "a INT64,"
           "b INT64"
           ")");
    
    runSQL("CREATE INDEX airway_edge_from ON airway_edge(a)");
    
    runSQL("CREATE TABLE taxi_node ("
           "hold_type INT,"
           "on_runway BOOL,"
           "pushback BOOL"
           ")");
    
    runSQL("CREATE TABLE parking ("
           "heading FLOAT,"
           "radius INT,"
           "gate_type VARCHAR,"
           "airlines VARCHAR,"
           "pushback INT64"
           ")");
    
    runSQL("CREATE TABLE groundnet_edge ("
           "airport INT64,"
           "a INT64,"
           "b INT64"
           ")");
    
    runSQL("CREATE INDEX groundnet_edge_airport ON groundnet_edge(airport)");
    runSQL("CREATE INDEX groundnet_edge_from ON groundnet_edge(a)");
  }
  
  void prepareQueries()
  {
    clearProperty = prepare("DELETE FROM properties WHERE key=?1");
    writePropertyMulti = prepare("INSERT INTO properties (key, value) VALUES(?1,?2)");
    
#define POSITIONED_COLS "rowid, type, ident, name, airport, lon, lat, elev_m, octree_node"
#define AND_TYPED "AND type>=?2 AND type <=?3"
    statCacheCheck = prepare("SELECT stamp FROM stat_cache WHERE path=?");
    stampFileCache = prepare("INSERT OR REPLACE INTO stat_cache "
                             "(path, stamp) VALUES (?,?)");
    
    loadPositioned = prepare("SELECT " POSITIONED_COLS " FROM positioned WHERE rowid=?");
    loadAirportStmt = prepare("SELECT has_metar FROM airport WHERE rowid=?");
    loadNavaid = prepare("SELECT range_nm, freq, multiuse, runway, colocated FROM navaid WHERE rowid=?");
    loadCommStation = prepare("SELECT freq_khz, range_nm FROM comm WHERE rowid=?");
    loadRunwayStmt = prepare("SELECT heading, length_ft, width_m, surface, displaced_threshold,"
                             "stopway, reciprocal, ils FROM runway WHERE rowid=?1");
    
    getAirportItems = prepare("SELECT rowid FROM positioned WHERE airport=?1 " AND_TYPED);

    
    setAirportMetar = prepare("UPDATE airport SET has_metar=?2 WHERE rowid="
                              "(SELECT rowid FROM positioned WHERE ident=?1 AND type>=?3 AND type <=?4)");
    sqlite3_bind_int(setAirportMetar, 3, FGPositioned::AIRPORT);
    sqlite3_bind_int(setAirportMetar, 4, FGPositioned::SEAPORT);
    
    setRunwayReciprocal = prepare("UPDATE runway SET reciprocal=?2 WHERE rowid=?1");
    setRunwayILS = prepare("UPDATE runway SET ils=?2 WHERE rowid=?1");
    updateRunwayThreshold = prepare("UPDATE runway SET heading=?2, displaced_threshold=?3, stopway=?4 WHERE rowid=?1");
    
    insertPositionedQuery = prepare("INSERT INTO positioned "
                                    "(type, ident, name, airport, lon, lat, elev_m, octree_node, "
                                    "cart_x, cart_y, cart_z)"
                                    " VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)");
    
    setAirportPos = prepare("UPDATE positioned SET lon=?2, lat=?3, elev_m=?4, octree_node=?5, "
                            "cart_x=?6, cart_y=?7, cart_z=?8 WHERE rowid=?1");
    insertAirport = prepare("INSERT INTO airport (rowid, has_metar) VALUES (?, ?)");
    insertNavaid = prepare("INSERT INTO navaid (rowid, freq, range_nm, multiuse, runway, colocated)"
                           " VALUES (?1, ?2, ?3, ?4, ?5, ?6)");
    updateILS = prepare("UPDATE navaid SET multiuse=?2 WHERE rowid=?1");
    
    insertCommStation = prepare("INSERT INTO comm (rowid, freq_khz, range_nm)"
                                " VALUES (?, ?, ?)");
    insertRunway = prepare("INSERT INTO runway "
                           "(rowid, heading, length_ft, width_m, surface, displaced_threshold, stopway, reciprocal)"
                           " VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)");
    runwayLengthFtQuery = prepare("SELECT length_ft FROM runway WHERE rowid=?1");
    
  // query statement    
    findClosestWithIdent = prepare("SELECT rowid FROM positioned WHERE ident=?1 "
                                   AND_TYPED " ORDER BY distanceCartSqr(cart_x, cart_y, cart_z, ?4, ?5, ?6)");
    
    findCommByFreq = prepare("SELECT positioned.rowid FROM positioned, comm WHERE "
                             "positioned.rowid=comm.rowid AND freq_khz=?1 "
                             AND_TYPED " ORDER BY distanceCartSqr(cart_x, cart_y, cart_z, ?4, ?5, ?6)");
    
    findNavsByFreq = prepare("SELECT positioned.rowid FROM positioned, navaid WHERE "
                             "positioned.rowid=navaid.rowid "
                             "AND navaid.freq=?1 " AND_TYPED
                             " ORDER BY distanceCartSqr(cart_x, cart_y, cart_z, ?4, ?5, ?6)");
    
    findNavsByFreqNoPos = prepare("SELECT positioned.rowid FROM positioned, navaid WHERE "
                                  "positioned.rowid=navaid.rowid AND freq=?1 " AND_TYPED);
    
    findNavaidForRunway = prepare("SELECT positioned.rowid FROM positioned, navaid WHERE "
                                  "positioned.rowid=navaid.rowid AND runway=?1 AND type=?2");
    
  // for an octree branch, return the child octree nodes which exist,
  // described as a bit-mask
    getOctreeChildren = prepare("SELECT children FROM octree WHERE rowid=?1");
    
#ifdef LAZY_OCTREE_UPDATES
    updateOctreeChildren = prepare("UPDATE octree SET children=?2 WHERE rowid=?1");
#else
  // mask the new child value into the existing one
    updateOctreeChildren = prepare("UPDATE octree SET children=(?2 | children) WHERE rowid=?1");
#endif
    
  // define a new octree node (with no children)
    insertOctree = prepare("INSERT INTO octree (rowid, children) VALUES (?1, 0)");
    
    getOctreeLeafChildren = prepare("SELECT rowid, type FROM positioned WHERE octree_node=?1");
    
    searchAirports = prepare("SELECT ident, name FROM positioned WHERE (name LIKE ?1 OR ident LIKE ?1) " AND_TYPED);
    sqlite3_bind_int(searchAirports, 2, FGPositioned::AIRPORT);
    sqlite3_bind_int(searchAirports, 3, FGPositioned::SEAPORT);
    
    getAirportItemByIdent = prepare("SELECT rowid FROM positioned WHERE airport=?1 AND ident=?2 AND type=?3");
    
    findAirportRunway = prepare("SELECT airport, rowid FROM positioned WHERE ident=?2 AND type=?3 AND airport="
                                "(SELECT rowid FROM positioned WHERE type=?4 AND ident=?1)");
    sqlite3_bind_int(findAirportRunway, 3, FGPositioned::RUNWAY);
    sqlite3_bind_int(findAirportRunway, 4, FGPositioned::AIRPORT);
    
    // three-way join to get the navaid ident and runway ident in a single select.
    // we're joining positioned to itself by the navaid runway, with the complication
    // that we need to join the navaids table to get the runway ID.
    // we also need to filter by type to excluse glideslope (GS) matches
    findILS = prepare("SELECT nav.rowid FROM positioned AS nav, positioned AS rwy, navaid WHERE "
                      "nav.ident=?1 AND nav.airport=?2 AND rwy.ident=?3 "
                      "AND rwy.rowid = navaid.runway AND navaid.rowid=nav.rowid "
                      "AND (nav.type=?4 OR nav.type=?5)");

    sqlite3_bind_int(findILS, 4, FGPositioned::ILS);
    sqlite3_bind_int(findILS, 5, FGPositioned::LOC);
    
  // airways 
    findAirway = prepare("SELECT rowid FROM airway WHERE network=?1 AND ident=?2");
    insertAirway = prepare("INSERT INTO airway (ident, network) "
                           "VALUES (?1, ?2)");
    
    insertAirwayEdge = prepare("INSERT INTO airway_edge (network, airway, a, b) "
                               "VALUES (?1, ?2, ?3, ?4)");
    
    isPosInAirway = prepare("SELECT rowid FROM airway_edge WHERE network=?1 AND a=?2");
    
    airwayEdgesFrom = prepare("SELECT airway, b FROM airway_edge WHERE network=?1 AND a=?2");
    
  // parking / taxi-node graph
    insertTaxiNode = prepare("INSERT INTO taxi_node (rowid, hold_type, on_runway, pushback) VALUES(?1, ?2, ?3, 0)");
    insertParkingPos = prepare("INSERT INTO parking (rowid, heading, radius, gate_type, airlines) "
                               "VALUES (?1, ?2, ?3, ?4, ?5)");
    setParkingPushBack = prepare("UPDATE parking SET pushback=?2 WHERE rowid=?1");
    
    loadTaxiNodeStmt = prepare("SELECT hold_type, on_runway FROM taxi_node WHERE rowid=?1");
    loadParkingPos = prepare("SELECT heading, radius, gate_type, airlines, pushback FROM parking WHERE rowid=?1");
    taxiEdgesFrom = prepare("SELECT b FROM groundnet_edge WHERE a=?1");
    pushbackEdgesFrom = prepare("SELECT b FROM groundnet_edge, taxi_node WHERE "
                                "a=?1 AND groundnet_edge.b = taxi_node.rowid AND pushback=1");
    
    insertTaxiEdge = prepare("INSERT INTO groundnet_edge (airport, a,b) VALUES(?1, ?2, ?3)");
    
    markTaxiNodeAsPushback = prepare("UPDATE taxi_node SET pushback=1 WHERE rowid=?1");
    airportTaxiNodes = prepare("SELECT rowid FROM positioned WHERE (type=?2 OR type=?3) AND airport=?1");
    sqlite3_bind_int(airportTaxiNodes, 2, FGPositioned::PARKING);
    sqlite3_bind_int(airportTaxiNodes, 3, FGPositioned::TAXI_NODE);
    
    airportPushbackNodes = prepare("SELECT positioned.rowid FROM positioned, taxi_node WHERE "\
                                   "airport=?1 AND positioned.rowid=taxi_node.rowid AND pushback=1 "
                                   "AND (type=?2 OR type=?3)");
    sqlite3_bind_int(airportPushbackNodes, 2, FGPositioned::PARKING);
    sqlite3_bind_int(airportPushbackNodes, 3, FGPositioned::TAXI_NODE);
    
    findNearestTaxiNode = prepare("SELECT positioned.rowid FROM positioned, taxi_node WHERE "
                                  "positioned.rowid = taxi_node.rowid AND airport=?1 "
                                  "ORDER BY distanceCartSqr(cart_x, cart_y, cart_z, ?2, ?3, ?4) "
                                  "LIMIT 1");
    
    findNearestRunwayTaxiNode = prepare("SELECT positioned.rowid FROM positioned, taxi_node WHERE "
                                        "positioned.rowid = taxi_node.rowid AND airport=?1 "
                                        "AND on_runway=1 " 
                                        "ORDER BY distanceCartSqr(cart_x, cart_y, cart_z, ?2, ?3, ?4) ");
    
    findAirportParking = prepare("SELECT positioned.rowid FROM positioned, parking WHERE "
                                 "airport=?1 AND type=?4 AND "
                                 "radius >= ?2 AND gate_type = ?3 AND "
                                 "parking.rowid=positioned.rowid");
    sqlite3_bind_int(findAirportParking, 4, FGPositioned::PARKING);
  }
  
  void writeIntProperty(const string& key, int value)
  {
    sqlite_bind_stdstring(writePropertyQuery, 1, key);
    sqlite3_bind_int(writePropertyQuery, 2, value);
    execSelect(writePropertyQuery);
  }

  
  FGPositioned* loadFromStmt(sqlite3_stmt_ptr query);
  
  FGAirport* loadAirport(sqlite_int64 rowId,
                         FGPositioned::Type ty,
                         const string& id, const string& name, const SGGeod& pos)
  {
    reset(loadAirportStmt);
    sqlite3_bind_int64(loadAirportStmt, 1, rowId);
    execSelect1(loadAirportStmt);
    bool hasMetar = (sqlite3_column_int(loadAirportStmt, 0) > 0);
    return new FGAirport(rowId, id, pos, name, hasMetar, ty);
  }
  
  FGRunwayBase* loadRunway(sqlite3_int64 rowId, FGPositioned::Type ty,
                           const string& id, const SGGeod& pos, PositionedID apt)
  {
    reset(loadRunwayStmt);
    sqlite3_bind_int(loadRunwayStmt, 1, rowId);
    execSelect1(loadRunwayStmt);
    
    double heading = sqlite3_column_double(loadRunwayStmt, 0);
    double lengthM = sqlite3_column_int(loadRunwayStmt, 1);
    double widthM = sqlite3_column_double(loadRunwayStmt, 2);
    int surface = sqlite3_column_int(loadRunwayStmt, 3);
  
    if (ty == FGPositioned::TAXIWAY) {
      return new FGTaxiway(rowId, id, pos, heading, lengthM, widthM, surface);
    } else {
      double displacedThreshold = sqlite3_column_double(loadRunwayStmt, 4);
      double stopway = sqlite3_column_double(loadRunwayStmt, 5);
      PositionedID reciprocal = sqlite3_column_int64(loadRunwayStmt, 6);
      PositionedID ils = sqlite3_column_int64(loadRunwayStmt, 7);
      FGRunway* r = new FGRunway(rowId, apt, id, pos, heading, lengthM, widthM,
                          displacedThreshold, stopway, surface, false);
      
      if (reciprocal > 0) {
        r->setReciprocalRunway(reciprocal);
      }
      
      if (ils > 0) {
        r->setILS(ils);
      }
      
      return r;
    }
  }
  
  CommStation* loadComm(sqlite3_int64 rowId, FGPositioned::Type ty,
                        const string& id, const string& name,
                        const SGGeod& pos,
                        PositionedID airport)
  {
    reset(loadCommStation);
    sqlite3_bind_int64(loadCommStation, 1, rowId);
    execSelect1(loadCommStation);
    
    int range = sqlite3_column_int(loadCommStation, 0);
    int freqKhz = sqlite3_column_int(loadCommStation, 1);
    
    CommStation* c = new CommStation(rowId, name, ty, pos, freqKhz, range);
    c->setAirport(airport);
    return c;
  }
  
  FGPositioned* loadNav(sqlite3_int64 rowId,
                       FGPositioned::Type ty, const string& id,
                       const string& name, const SGGeod& pos)
  {
    reset(loadNavaid);
    sqlite3_bind_int64(loadNavaid, 1, rowId);
    execSelect1(loadNavaid);
    
    PositionedID runway = sqlite3_column_int64(loadNavaid, 3);
    // marker beacons are light-weight
    if ((ty == FGPositioned::OM) || (ty == FGPositioned::IM) ||
        (ty == FGPositioned::MM))
    {
      return new FGMarkerBeaconRecord(rowId, ty, runway, pos);
    }
    
    int rangeNm = sqlite3_column_int(loadNavaid, 0),
      freq = sqlite3_column_int(loadNavaid, 1);
    double mulituse = sqlite3_column_double(loadNavaid, 2);
    //sqlite3_int64 colocated = sqlite3_column_int64(loadNavaid, 4);
    
    return new FGNavRecord(rowId, ty, id, name, pos, freq, rangeNm, mulituse, runway);
  }
  
  FGPositioned* loadParking(sqlite3_int64 rowId,
                            const string& name, const SGGeod& pos,
                            PositionedID airport)
  {
    reset(loadParkingPos);
    sqlite3_bind_int64(loadParkingPos, 1, rowId);
    execSelect1(loadParkingPos);
    
    double heading = sqlite3_column_double(loadParkingPos, 0);
    int radius = sqlite3_column_int(loadParkingPos, 1);
    string aircraftType((char*) sqlite3_column_text(loadParkingPos, 2));
    string airlines((char*) sqlite3_column_text(loadParkingPos, 3));
    PositionedID pushBack = sqlite3_column_int64(loadParkingPos, 4);
    
    return new FGParking(rowId, pos, heading, radius, name, aircraftType, airlines, pushBack);
  }
  
  FGPositioned* loadTaxiNode(sqlite3_int64 rowId, const SGGeod& pos,
                             PositionedID airport)
  {
    reset(loadTaxiNodeStmt);
    sqlite3_bind_int64(loadTaxiNodeStmt, 1, rowId);
    execSelect1(loadTaxiNodeStmt);
    
    int hold_type = sqlite3_column_int(loadTaxiNodeStmt, 0);
    bool onRunway = sqlite3_column_int(loadTaxiNodeStmt, 1);
    return new FGTaxiNode(rowId, pos, onRunway, hold_type);
  }
  
  PositionedID insertPositioned(FGPositioned::Type ty, const string& ident,
                                const string& name, const SGGeod& pos, PositionedID apt,
                                bool spatialIndex)
  {
    SGVec3d cartPos(SGVec3d::fromGeod(pos));
    
    reset(insertPositionedQuery);
    sqlite3_bind_int(insertPositionedQuery, 1, ty);
    sqlite_bind_stdstring(insertPositionedQuery, 2, ident);
    sqlite_bind_stdstring(insertPositionedQuery, 3, name);
    sqlite3_bind_int64(insertPositionedQuery, 4, apt);
    sqlite3_bind_double(insertPositionedQuery, 5, pos.getLongitudeDeg());
    sqlite3_bind_double(insertPositionedQuery, 6, pos.getLatitudeDeg());
    sqlite3_bind_double(insertPositionedQuery, 7, pos.getElevationM());
    
    if (spatialIndex) {
      Octree::Leaf* octreeLeaf = Octree::global_spatialOctree->findLeafForPos(cartPos);
      assert(intersects(octreeLeaf->bbox(), cartPos));
      sqlite3_bind_int64(insertPositionedQuery, 8, octreeLeaf->guid());
    } else {
      sqlite3_bind_null(insertPositionedQuery, 8);
    }
    
    sqlite3_bind_double(insertPositionedQuery, 9, cartPos.x());
    sqlite3_bind_double(insertPositionedQuery, 10, cartPos.y());
    sqlite3_bind_double(insertPositionedQuery, 11, cartPos.z());
    
    PositionedID r = execInsert(insertPositionedQuery);
    return r;
  }
  
  FGPositioned::List findAllByString(const string& s, const string& column,
                                     FGPositioned::Filter* filter, bool exact)
  {
    string query = s;
    if (!exact) query += "%";
    
  // build up SQL query text
    string matchTerm = exact ? "=?1" : " LIKE ?1";
    string sql = "SELECT rowid FROM positioned WHERE " + column + matchTerm;
    if (filter) {
      sql += " " AND_TYPED;
    }

  // find or prepare a suitable statement frrm the SQL
    sqlite3_stmt_ptr stmt = findByStringDict[sql];
    if (!stmt) {
      stmt = prepare(sql);
      findByStringDict[sql] = stmt;
    }

    reset(stmt);
    sqlite_bind_stdstring(stmt, 1, query);
    if (filter) {
      sqlite3_bind_int(stmt, 2, filter->minType());
      sqlite3_bind_int(stmt, 3, filter->maxType());
    }
    
    FGPositioned::List result;
  // run the prepared SQL
    while (stepSelect(stmt))
    {
      FGPositioned* pos = outer->loadById(sqlite3_column_int64(stmt, 0));
      if (filter && !filter->pass(pos)) {
        continue;
      }
      
      result.push_back(pos);
    }
    
    return result;
  }
  
  PositionedIDVec selectIds(sqlite3_stmt_ptr query)
  {
    PositionedIDVec result;
    while (stepSelect(query)) {
      result.push_back(sqlite3_column_int64(query, 0));
    }
    return result;
  }
  
  double runwayLengthFt(PositionedID rwy)
  {
    reset(runwayLengthFtQuery);
    sqlite3_bind_int64(runwayLengthFtQuery, 1, rwy);
    execSelect1(runwayLengthFtQuery);
    return sqlite3_column_double(runwayLengthFtQuery, 0);
  }
  
  void flushDeferredOctreeUpdates()
  {
    BOOST_FOREACH(Octree::Branch* nd, deferredOctreeUpdates) {
      reset(updateOctreeChildren);
      sqlite3_bind_int64(updateOctreeChildren, 1, nd->guid());
      sqlite3_bind_int(updateOctreeChildren, 2, nd->childMask());
      execUpdate(updateOctreeChildren);
    }
    
    deferredOctreeUpdates.clear();
  }
  
  NavDataCache* outer;
  sqlite3* db;
  SGPath path;
  
  /// the actual cache of ID -> instances. This holds an owning reference,
  /// so once items are in the cache they will never be deleted until
  /// the cache drops its reference
  PositionedCache cache;
  unsigned int cacheHits, cacheMisses;
  
  SGPath aptDatPath, metarDatPath, navDatPath, fixDatPath,
  carrierDatPath, airwayDatPath;
  
  sqlite3_stmt_ptr readPropertyQuery, writePropertyQuery,
    stampFileCache, statCacheCheck,
    loadAirportStmt, loadCommStation, loadPositioned, loadNavaid,
    loadRunwayStmt;
  sqlite3_stmt_ptr writePropertyMulti, clearProperty;
  
  sqlite3_stmt_ptr insertPositionedQuery, insertAirport, insertTower, insertRunway,
  insertCommStation, insertNavaid;
  sqlite3_stmt_ptr setAirportMetar, setRunwayReciprocal, setRunwayILS,
    setAirportPos, updateRunwayThreshold, updateILS;
  
  sqlite3_stmt_ptr findClosestWithIdent;
// octree (spatial index) related queries
  sqlite3_stmt_ptr getOctreeChildren, insertOctree, updateOctreeChildren,
    getOctreeLeafChildren;

  sqlite3_stmt_ptr searchAirports;
  sqlite3_stmt_ptr findCommByFreq, findNavsByFreq,
  findNavsByFreqNoPos, findNavaidForRunway;
  sqlite3_stmt_ptr getAirportItems, getAirportItemByIdent;
  sqlite3_stmt_ptr findAirportRunway,
    findILS;
  
  sqlite3_stmt_ptr runwayLengthFtQuery;
  
// airways
  sqlite3_stmt_ptr findAirway, insertAirwayEdge, isPosInAirway, airwayEdgesFrom,
  insertAirway;
  
// groundnet (parking, taxi node graph)
  sqlite3_stmt_ptr loadTaxiNodeStmt, loadParkingPos, insertTaxiNode, insertParkingPos;
  sqlite3_stmt_ptr taxiEdgesFrom, pushbackEdgesFrom, insertTaxiEdge, markTaxiNodeAsPushback,
    airportTaxiNodes, airportPushbackNodes, findNearestTaxiNode, findAirportParking,
    setParkingPushBack, findNearestRunwayTaxiNode;
  
// since there's many permutations of ident/name queries, we create
// them programtically, but cache the exact query by its raw SQL once
// used.
  std::map<string, sqlite3_stmt_ptr> findByStringDict;
  
  typedef std::vector<sqlite3_stmt_ptr> StmtVec;
  StmtVec prepared;
  
  std::set<Octree::Branch*> deferredOctreeUpdates;
  
  // if we're performing a rebuild, the thread that is doing the work.
  // otherwise, NULL
  std::auto_ptr<RebuildThread> rebuilder;
};

  //////////////////////////////////////////////////////////////////////
  
FGPositioned* NavDataCache::NavDataCachePrivate::loadFromStmt(sqlite3_stmt_ptr query)
{
  execSelect1(query);
  sqlite3_int64 rowid = sqlite3_column_int64(query, 0);
  FGPositioned::Type ty = (FGPositioned::Type) sqlite3_column_int(query, 1);
  
  string ident = (char*) sqlite3_column_text(query, 2);
  string name = (char*) sqlite3_column_text(query, 3);
  sqlite3_int64 aptId = sqlite3_column_int64(query, 4);
  double lon = sqlite3_column_double(query, 5);
  double lat = sqlite3_column_double(query, 6);
  double elev = sqlite3_column_double(query, 7);
  SGGeod pos = SGGeod::fromDegM(lon, lat, elev);
  
  switch (ty) {
    case FGPositioned::AIRPORT:
    case FGPositioned::SEAPORT:
    case FGPositioned::HELIPORT:
      return loadAirport(rowid, ty, ident, name, pos);
      
    case FGPositioned::TOWER:
      return new AirportTower(rowid, aptId, ident, pos);
      
    case FGPositioned::RUNWAY:
    case FGPositioned::TAXIWAY:
      return loadRunway(rowid, ty, ident, pos, aptId);
      
    case FGPositioned::LOC:
    case FGPositioned::VOR:
    case FGPositioned::GS:
    case FGPositioned::ILS:
    case FGPositioned::NDB:
    case FGPositioned::OM:
    case FGPositioned::MM:
    case FGPositioned::IM:
    case FGPositioned::DME:
    case FGPositioned::TACAN:
    case FGPositioned::MOBILE_TACAN:
    {
      if (aptId > 0) {
        FGAirport* apt = (FGAirport*) outer->loadById(aptId);
        if (apt->validateILSData()) {
          SG_LOG(SG_NAVCACHE, SG_INFO, "re-loaded ILS data for " << apt->ident());
          // queried data above is probably invalid, force us to go around again
          // (the next time through, validateILSData will return false)
          return outer->loadById(rowid);
        }
      }
      
      return loadNav(rowid, ty, ident, name, pos);
    }
      
    case FGPositioned::FIX:
      return new FGFix(rowid, ident, pos);
      
    case FGPositioned::WAYPOINT:
    {
      FGPositioned* wpt = new FGPositioned(rowid, FGPositioned::WAYPOINT, ident, pos);
      return wpt;
    }
      
    case FGPositioned::FREQ_GROUND:
    case FGPositioned::FREQ_TOWER:
    case FGPositioned::FREQ_ATIS:
    case FGPositioned::FREQ_AWOS:
    case FGPositioned::FREQ_APP_DEP:
    case FGPositioned::FREQ_ENROUTE:
    case FGPositioned::FREQ_CLEARANCE:
    case FGPositioned::FREQ_UNICOM:
      return loadComm(rowid, ty, ident, name, pos, aptId);
      
    case FGPositioned::TAXI_NODE:
      return loadTaxiNode(rowid, pos, aptId);
      
    case FGPositioned::PARKING:
      return loadParking(rowid, ident, pos, aptId);
      
    default:
      return NULL;
  }
}

  
static NavDataCache* static_instance = NULL;
        
NavDataCache::NavDataCache()
{
  const int MAX_TRIES = 3;
  SGPath homePath(globals->get_fg_home());
  homePath.append("navdata.cache");
  
  for (int t=0; t < MAX_TRIES; ++t) {
    try {
      d.reset(new NavDataCachePrivate(homePath, this));
      d->init();
      //d->checkCacheFile();
    // reached this point with no exception, success
      break;
    } catch (sg_exception& e) {
      SG_LOG(SG_NAVCACHE, t == 0 ? SG_WARN : SG_ALERT, "NavCache: init failed:" << e.what()
             << " (attempt " << t << ")");
      d.reset();
      homePath.remove();
    }
  } // of retry loop
    
  double RADIUS_EARTH_M = 7000 * 1000.0; // 7000km is plenty
  SGVec3d earthExtent(RADIUS_EARTH_M, RADIUS_EARTH_M, RADIUS_EARTH_M);
  Octree::global_spatialOctree =
    new Octree::Branch(SGBox<double>(-earthExtent, earthExtent), 1);
  
  d->aptDatPath = SGPath(globals->get_fg_root());
  d->aptDatPath.append("Airports/apt.dat.gz");
  
  d->metarDatPath = SGPath(globals->get_fg_root());
  d->metarDatPath.append("Airports/metar.dat.gz");

  d->navDatPath = SGPath(globals->get_fg_root());  
  d->navDatPath.append("Navaids/nav.dat.gz");

  d->fixDatPath = SGPath(globals->get_fg_root());
  d->fixDatPath.append("Navaids/fix.dat.gz");
  
  d->carrierDatPath = SGPath(globals->get_fg_root());
  d->carrierDatPath.append("Navaids/carrier_nav.dat.gz");
  
  d->airwayDatPath = SGPath(globals->get_fg_root());
  d->airwayDatPath.append("Navaids/awy.dat.gz");
}
    
NavDataCache::~NavDataCache()
{
  assert(static_instance == this);
  static_instance = NULL;
  SG_LOG(SG_NAVCACHE, SG_INFO, "closing the navcache");
  d.reset();
}
    
NavDataCache* NavDataCache::instance()
{
  if (!static_instance) {
    static_instance = new NavDataCache;
  }
  
  return static_instance;
}
  
bool NavDataCache::isRebuildRequired()
{
  if (isCachedFileModified(d->aptDatPath) ||
      isCachedFileModified(d->metarDatPath) ||
      isCachedFileModified(d->navDatPath) ||
      isCachedFileModified(d->fixDatPath) ||
      isCachedFileModified(d->airwayDatPath))
  {
    SG_LOG(SG_NAVCACHE, SG_INFO, "NavCache: rebuild required");
    return true;
  }

  SG_LOG(SG_NAVCACHE, SG_INFO, "NavCache: no rebuild required");
  return false;
}
  
bool NavDataCache::rebuild()
{
  if (!d->rebuilder.get()) {
    d->rebuilder.reset(new RebuildThread(this));
    d->rebuilder->start();
  }
  
// poll the rebuild thread
  bool fin = d->rebuilder->isFinished();
  if (fin) {
    d->rebuilder.reset(); // all done!
  }
  return fin;
}
  
void NavDataCache::doRebuild()
{
  try {
    d->runSQL("BEGIN");
    d->runSQL("DELETE FROM positioned");
    d->runSQL("DELETE FROM airport");
    d->runSQL("DELETE FROM runway");
    d->runSQL("DELETE FROM navaid");
    d->runSQL("DELETE FROM comm");
    d->runSQL("DELETE FROM octree");
    d->runSQL("DELETE FROM airway");
    d->runSQL("DELETE FROM airway_edge");
    
  // initialise the root octree node
    d->runSQL("INSERT INTO octree (rowid, children) VALUES (1, 0)");
    
    SGTimeStamp st;
    st.stamp();
    
    airportDBLoad(d->aptDatPath);
    SG_LOG(SG_NAVCACHE, SG_INFO, "apt.dat load took:" << st.elapsedMSec());
    
    metarDataLoad(d->metarDatPath);
    stampCacheFile(d->aptDatPath);
    stampCacheFile(d->metarDatPath);
    
    st.stamp();
    loadFixes(d->fixDatPath);
    stampCacheFile(d->fixDatPath);
    SG_LOG(SG_NAVCACHE, SG_INFO, "fix.dat load took:" << st.elapsedMSec());
    
    st.stamp();
    navDBInit(d->navDatPath);
    stampCacheFile(d->navDatPath);
    SG_LOG(SG_NAVCACHE, SG_INFO, "nav.dat load took:" << st.elapsedMSec());
    
    loadCarrierNav(d->carrierDatPath);
    stampCacheFile(d->carrierDatPath);
    
    st.stamp();
    Airway::load(d->airwayDatPath);
    stampCacheFile(d->airwayDatPath);
    SG_LOG(SG_NAVCACHE, SG_INFO, "awy.dat load took:" << st.elapsedMSec());
    
    d->flushDeferredOctreeUpdates();
    
    d->runSQL("COMMIT");
  } catch (sg_exception& e) {
    SG_LOG(SG_NAVCACHE, SG_ALERT, "caught exception rebuilding navCache:" << e.what());
  // abandon the DB transation completely
    d->runSQL("ROLLBACK");
  }
}
  
int NavDataCache::readIntProperty(const string& key)
{
  d->reset(d->readPropertyQuery);
  sqlite_bind_stdstring(d->readPropertyQuery, 1, key);
  
  if (d->execSelect(d->readPropertyQuery)) {
    return sqlite3_column_int(d->readPropertyQuery, 0);
  } else {
    SG_LOG(SG_NAVCACHE, SG_WARN, "readIntProperty: unknown:" << key);
    return 0; // no such property
  }
}

double NavDataCache::readDoubleProperty(const string& key)
{
  d->reset(d->readPropertyQuery);
  sqlite_bind_stdstring(d->readPropertyQuery, 1, key);
  if (d->execSelect(d->readPropertyQuery)) {
    return sqlite3_column_double(d->readPropertyQuery, 0);
  } else {
    SG_LOG(SG_NAVCACHE, SG_WARN, "readDoubleProperty: unknown:" << key);
    return 0.0; // no such property
  }
}
  
string NavDataCache::readStringProperty(const string& key)
{
  d->reset(d->readPropertyQuery);
  sqlite_bind_stdstring(d->readPropertyQuery, 1, key);
  if (d->execSelect(d->readPropertyQuery)) {
    return (char*) sqlite3_column_text(d->readPropertyQuery, 0);
  } else {
    SG_LOG(SG_NAVCACHE, SG_WARN, "readStringProperty: unknown:" << key);
    return string(); // no such property
  }
}

void NavDataCache::writeIntProperty(const string& key, int value)
{
  d->writeIntProperty(key, value);
}

void NavDataCache::writeStringProperty(const string& key, const string& value)
{
  d->reset(d->writePropertyQuery);
  sqlite_bind_stdstring(d->writePropertyQuery, 1, key);
  sqlite_bind_stdstring(d->writePropertyQuery, 2, value);
  d->execSelect(d->writePropertyQuery);
}

void NavDataCache::writeDoubleProperty(const string& key, const double& value)
{
  d->reset(d->writePropertyQuery);
  sqlite_bind_stdstring(d->writePropertyQuery, 1, key);
  sqlite3_bind_double(d->writePropertyQuery, 2, value);
  d->execSelect(d->writePropertyQuery);
}

string_list NavDataCache::readStringListProperty(const string& key)
{
  d->reset(d->readPropertyQuery);
  sqlite_bind_stdstring(d->readPropertyQuery, 1, key);
  string_list result;
  while (d->stepSelect(d->readPropertyQuery)) {
    result.push_back((char*) sqlite3_column_text(d->readPropertyQuery, 0));
  }
  
  return result;
}
  
void NavDataCache::writeStringListProperty(const string& key, const string_list& values)
{
  d->reset(d->clearProperty);
  sqlite_bind_stdstring(d->clearProperty, 1, key);
  d->execUpdate(d->clearProperty);
  
  BOOST_FOREACH(string value, values) {
    d->reset(d->writePropertyMulti);
    sqlite_bind_stdstring(d->writePropertyMulti, 1, key);
    sqlite_bind_stdstring(d->writePropertyMulti, 2, value);
    d->execInsert(d->writePropertyMulti);
  }
}
  
bool NavDataCache::isCachedFileModified(const SGPath& path) const
{
  if (!path.exists()) {
    throw sg_io_exception("isCachedFileModified: Missing file:" + path.str());
  }
  
  d->reset(d->statCacheCheck);
  sqlite_bind_temp_stdstring(d->statCacheCheck, 1, path.str());
  if (d->execSelect(d->statCacheCheck)) {
    time_t modtime = sqlite3_column_int64(d->statCacheCheck, 0);
    return (modtime != path.modTime());
  } else {
    return true;
  }
}

void NavDataCache::stampCacheFile(const SGPath& path)
{
  d->reset(d->stampFileCache);
  sqlite_bind_temp_stdstring(d->stampFileCache, 1, path.str());
  sqlite3_bind_int64(d->stampFileCache, 2, path.modTime());
  d->execInsert(d->stampFileCache);
}

void NavDataCache::beginTransaction()
{
  d->runSQL("BEGIN");
}
  
void NavDataCache::commitTransaction()
{
  d->runSQL("COMMIT");
}
  
void NavDataCache::abortTransaction()
{
  d->runSQL("ROLLBACK");
}

FGPositioned* NavDataCache::loadById(PositionedID rowid)
{
  if (rowid == 0) {
    return NULL;
  }
 
  PositionedCache::iterator it = d->cache.find(rowid);
  if (it != d->cache.end()) {
    d->cacheHits++;
    return it->second; // cache it
  }
  
  d->reset(d->loadPositioned);
  sqlite3_bind_int64(d->loadPositioned, 1, rowid);
  FGPositioned* pos = d->loadFromStmt(d->loadPositioned);
  
  d->cache.insert(it, PositionedCache::value_type(rowid, pos));
  d->cacheMisses++;
  
  return pos;
}

PositionedID NavDataCache::insertAirport(FGPositioned::Type ty, const string& ident,
                                         const string& name)
{
  // airports have their pos computed based on the avergae runway centres
  // so the pos isn't available immediately. Pass a dummy pos and avoid
  // doing spatial indexing until later
  sqlite3_int64 rowId = d->insertPositioned(ty, ident, name, SGGeod(),
                                            0 /* airport */,
                                            false /* spatial index */);
  
  d->reset(d->insertAirport);
  sqlite3_bind_int64(d->insertAirport, 1, rowId);
  d->execInsert(d->insertAirport);
  
  return rowId;
}
  
void NavDataCache::updatePosition(PositionedID item, const SGGeod &pos)
{
  if (d->cache.find(item) != d->cache.end()) {
    SG_LOG(SG_GENERAL, SG_DEBUG, "updating position of an item in the cache");
    d->cache[item]->modifyPosition(pos);
  }
  
  SGVec3d cartPos(SGVec3d::fromGeod(pos));
  
  d->reset(d->setAirportPos);
  sqlite3_bind_int(d->setAirportPos, 1, item);
  sqlite3_bind_double(d->setAirportPos, 2, pos.getLongitudeDeg());
  sqlite3_bind_double(d->setAirportPos, 3, pos.getLatitudeDeg());
  sqlite3_bind_double(d->setAirportPos, 4, pos.getElevationM());
  
// bug 905; the octree leaf may change here, but the leaf may already be
// loaded, and caching its children. (Either the old or new leaf!). Worse,
// we may be called here as a result of loading one of those leaf's children.
// instead of dealing with all those possibilites, such as modifying
// the in-memory leaf's STL child container, we simply leave the runtime
// structures alone. This is fine providing items do no move very far, since
// all the spatial searches ultimately use the items' real cartesian position,
// which was updated above.
  Octree::Leaf* octreeLeaf = Octree::global_spatialOctree->findLeafForPos(cartPos);
  sqlite3_bind_int64(d->setAirportPos, 5, octreeLeaf->guid());
  
  sqlite3_bind_double(d->setAirportPos, 6, cartPos.x());
  sqlite3_bind_double(d->setAirportPos, 7, cartPos.y());
  sqlite3_bind_double(d->setAirportPos, 8, cartPos.z());

  
  d->execUpdate(d->setAirportPos);
}

void NavDataCache::insertTower(PositionedID airportId, const SGGeod& pos)
{
  d->insertPositioned(FGPositioned::TOWER, string(), string(),
                      pos, airportId, true /* spatial index */);
}

PositionedID
NavDataCache::insertRunway(FGPositioned::Type ty, const string& ident,
                           const SGGeod& pos, PositionedID apt,
                           double heading, double length, double width, double displacedThreshold,
                           double stopway, int surfaceCode)
{
  // only runways are spatially indexed; don't bother indexing taxiways
  // or pavements
  bool spatialIndex = (ty == FGPositioned::RUNWAY);
  
  sqlite3_int64 rowId = d->insertPositioned(ty, cleanRunwayNo(ident), "", pos, apt,
                                            spatialIndex);
  d->reset(d->insertRunway);
  sqlite3_bind_int64(d->insertRunway, 1, rowId);
  sqlite3_bind_double(d->insertRunway, 2, heading);
  sqlite3_bind_double(d->insertRunway, 3, length);
  sqlite3_bind_double(d->insertRunway, 4, width);
  sqlite3_bind_int(d->insertRunway, 5, surfaceCode);
  sqlite3_bind_double(d->insertRunway, 6, displacedThreshold);
  sqlite3_bind_double(d->insertRunway, 7, stopway);
  
  return d->execInsert(d->insertRunway);  
}

void NavDataCache::setRunwayReciprocal(PositionedID runway, PositionedID recip)
{
  d->reset(d->setRunwayReciprocal);
  sqlite3_bind_int64(d->setRunwayReciprocal, 1, runway);
  sqlite3_bind_int64(d->setRunwayReciprocal, 2, recip);
  d->execUpdate(d->setRunwayReciprocal);
  
// and the opposite direction too!
  d->reset(d->setRunwayReciprocal);
  sqlite3_bind_int64(d->setRunwayReciprocal, 2, runway);
  sqlite3_bind_int64(d->setRunwayReciprocal, 1, recip);
  d->execUpdate(d->setRunwayReciprocal);
}

void NavDataCache::setRunwayILS(PositionedID runway, PositionedID ils)
{
  d->reset(d->setRunwayILS);
  sqlite3_bind_int64(d->setRunwayILS, 1, runway);
  sqlite3_bind_int64(d->setRunwayILS, 2, ils);
  d->execUpdate(d->setRunwayILS);
}
  
void NavDataCache::updateRunwayThreshold(PositionedID runwayID, const SGGeod &aThreshold,
                                  double aHeading, double aDisplacedThreshold,
                                  double aStopway)
{
// update the runway information
  d->reset(d->updateRunwayThreshold);
  sqlite3_bind_int64(d->updateRunwayThreshold, 1, runwayID);
  sqlite3_bind_double(d->updateRunwayThreshold, 2, aHeading);
  sqlite3_bind_double(d->updateRunwayThreshold, 3, aDisplacedThreshold);
  sqlite3_bind_double(d->updateRunwayThreshold, 4, aStopway);
  d->execUpdate(d->updateRunwayThreshold);
      
// compute the new runway center, based on the threshold lat/lon and length,
  double offsetFt = (0.5 * d->runwayLengthFt(runwayID));
  SGGeod newCenter;
  double dummy;
  SGGeodesy::direct(aThreshold, aHeading, offsetFt * SG_FEET_TO_METER, newCenter, dummy);
    
// now update the positional data
  updatePosition(runwayID, newCenter);
}
  
PositionedID
NavDataCache::insertNavaid(FGPositioned::Type ty, const string& ident,
                          const string& name, const SGGeod& pos,
                           int freq, int range, double multiuse,
                           PositionedID apt, PositionedID runway)
{
  bool spatialIndex = true;
  if (ty == FGPositioned::MOBILE_TACAN) {
    spatialIndex = false;
  }
  
  sqlite3_int64 rowId = d->insertPositioned(ty, ident, name, pos, apt,
                                            spatialIndex);
  d->reset(d->insertNavaid);
  sqlite3_bind_int64(d->insertNavaid, 1, rowId);
  sqlite3_bind_int(d->insertNavaid, 2, freq);
  sqlite3_bind_int(d->insertNavaid, 3, range);
  sqlite3_bind_double(d->insertNavaid, 4, multiuse);
  sqlite3_bind_int64(d->insertNavaid, 5, runway);
  return d->execInsert(d->insertNavaid);
}

void NavDataCache::updateILS(PositionedID ils, const SGGeod& newPos, double aHdg)
{
  d->reset(d->updateILS);
  sqlite3_bind_int64(d->updateILS, 1, ils);
  sqlite3_bind_double(d->updateILS, 2, aHdg);
  d->execUpdate(d->updateILS);
  updatePosition(ils, newPos);
}
  
PositionedID NavDataCache::insertCommStation(FGPositioned::Type ty,
                                             const string& name, const SGGeod& pos, int freq, int range,
                                             PositionedID apt)
{
  sqlite3_int64 rowId = d->insertPositioned(ty, "", name, pos, apt, true);
  d->reset(d->insertCommStation);
  sqlite3_bind_int64(d->insertCommStation, 1, rowId);
  sqlite3_bind_int(d->insertCommStation, 2, freq);
  sqlite3_bind_int(d->insertCommStation, 3, range);
  return d->execInsert(d->insertCommStation);
}
  
PositionedID NavDataCache::insertFix(const std::string& ident, const SGGeod& aPos)
{
  return d->insertPositioned(FGPositioned::FIX, ident, string(), aPos, 0, true);
}

PositionedID NavDataCache::createUserWaypoint(const std::string& ident, const SGGeod& aPos)
{
  return d->insertPositioned(FGPositioned::WAYPOINT, ident, string(), aPos, 0,
                             true /* spatial index */);
}
  
void NavDataCache::setAirportMetar(const string& icao, bool hasMetar)
{
  d->reset(d->setAirportMetar);
  sqlite_bind_stdstring(d->setAirportMetar, 1, icao);
  sqlite3_bind_int(d->setAirportMetar, 2, hasMetar);
  d->execUpdate(d->setAirportMetar);
}

FGPositioned::List NavDataCache::findAllWithIdent(const string& s,
                                                  FGPositioned::Filter* filter, bool exact)
{
  return d->findAllByString(s, "ident", filter, exact);
}

FGPositioned::List NavDataCache::findAllWithName(const string& s,
                                                  FGPositioned::Filter* filter, bool exact)
{
  return d->findAllByString(s, "name", filter, exact);
}
  
FGPositionedRef NavDataCache::findClosestWithIdent(const string& aIdent,
                                                   const SGGeod& aPos, FGPositioned::Filter* aFilter)
{
  d->reset(d->findClosestWithIdent);
  sqlite_bind_stdstring(d->findClosestWithIdent, 1, aIdent);
  if (aFilter) {
    sqlite3_bind_int(d->findClosestWithIdent, 2, aFilter->minType());
    sqlite3_bind_int(d->findClosestWithIdent, 3, aFilter->maxType());
  } else { // full type range
    sqlite3_bind_int(d->findClosestWithIdent, 2, FGPositioned::INVALID);
    sqlite3_bind_int(d->findClosestWithIdent, 3, FGPositioned::LAST_TYPE);
  }
  
  SGVec3d cartPos(SGVec3d::fromGeod(aPos));
  sqlite3_bind_double(d->findClosestWithIdent, 4, cartPos.x());
  sqlite3_bind_double(d->findClosestWithIdent, 5, cartPos.y());
  sqlite3_bind_double(d->findClosestWithIdent, 6, cartPos.z());
  
  while (d->stepSelect(d->findClosestWithIdent)) {
    FGPositioned* pos = loadById(sqlite3_column_int64(d->findClosestWithIdent, 0));
    if (aFilter && !aFilter->pass(pos)) {
      continue;
    }
    
    return pos;
  }
  
  return NULL; // no matches at all
}

  
int NavDataCache::getOctreeBranchChildren(int64_t octreeNodeId)
{
  d->reset(d->getOctreeChildren);
  sqlite3_bind_int64(d->getOctreeChildren, 1, octreeNodeId);
  d->execSelect1(d->getOctreeChildren);
  return sqlite3_column_int(d->getOctreeChildren, 0);
}

void NavDataCache::defineOctreeNode(Octree::Branch* pr, Octree::Node* nd)
{
  d->reset(d->insertOctree);
  sqlite3_bind_int64(d->insertOctree, 1, nd->guid());
  d->execInsert(d->insertOctree);
  
#ifdef LAZY_OCTREE_UPDATES
  d->deferredOctreeUpdates.insert(pr);
#else
  // lowest three bits of node ID are 0..7 index of the child in the parent
  int childIndex = nd->guid() & 0x07;
  
  d->reset(d->updateOctreeChildren);
  sqlite3_bind_int64(d->updateOctreeChildren, 1, pr->guid());
// mask has bit N set where child N exists
  int childMask = 1 << childIndex;
  sqlite3_bind_int(d->updateOctreeChildren, 2, childMask);
  d->execUpdate(d->updateOctreeChildren);
#endif
}
  
TypedPositionedVec
NavDataCache::getOctreeLeafChildren(int64_t octreeNodeId)
{
  d->reset(d->getOctreeLeafChildren);
  sqlite3_bind_int64(d->getOctreeLeafChildren, 1, octreeNodeId);
  
  TypedPositionedVec r;
  while (d->stepSelect(d->getOctreeLeafChildren)) {
    FGPositioned::Type ty = static_cast<FGPositioned::Type>
      (sqlite3_column_int(d->getOctreeLeafChildren, 1));
    r.push_back(std::make_pair(ty,
                sqlite3_column_int64(d->getOctreeLeafChildren, 0)));
  }

  return r;
}

  
/**
 * A special purpose helper (used by FGAirport::searchNamesAndIdents) to
 * implement the AirportList dialog. It's unfortunate that it needs to reside
 * here, but for now it's least ugly solution.
 */
char** NavDataCache::searchAirportNamesAndIdents(const std::string& aFilter)
{
  d->reset(d->searchAirports);
  string s = "%" + aFilter + "%";
  sqlite_bind_stdstring(d->searchAirports, 1, s);
  
  unsigned int numMatches = 0, numAllocated = 16;
  char** result = (char**) malloc(sizeof(char*) * numAllocated);
  
  while (d->stepSelect(d->searchAirports)) {
    if ((numMatches + 1) >= numAllocated) {
      numAllocated <<= 1; // double in size!
    // reallocate results array
      char** nresult = (char**) malloc(sizeof(char*) * numAllocated);
      memcpy(nresult, result, sizeof(char*) * numMatches);
      free(result);
      result = nresult;
    }
    
    // nasty code to avoid excessive string copying and allocations.
    // We format results as follows (note whitespace!):
    //   ' name-of-airport-chars   (ident)'
    // so the total length is:
    //    1 + strlen(name) + 4 + strlen(icao) + 1 + 1 (for the null)
    // which gives a grand total of 7 + name-length + icao-length.
    // note the ident can be three letters (non-ICAO local strip), four
    // (default ICAO) or more (extended format ICAO)
    int nameLength = sqlite3_column_bytes(d->searchAirports, 1);
    int icaoLength = sqlite3_column_bytes(d->searchAirports, 0);
    char* entry = (char*) malloc(7 + nameLength + icaoLength);
    char* dst = entry;
    *dst++ = ' ';
    memcpy(dst, sqlite3_column_text(d->searchAirports, 1), nameLength);
    dst += nameLength;
    *dst++ = ' ';
    *dst++ = ' ';
    *dst++ = ' ';
    *dst++ = '(';
    memcpy(dst, sqlite3_column_text(d->searchAirports, 0), icaoLength);
    dst += icaoLength;
    *dst++ = ')';
    *dst++ = 0;

    result[numMatches++] = entry;
  }
  
  result[numMatches] = NULL; // end of list marker
  return result;
}
  
FGPositionedRef
NavDataCache::findCommByFreq(int freqKhz, const SGGeod& aPos, FGPositioned::Filter* aFilter)
{
  d->reset(d->findCommByFreq);
  sqlite3_bind_int(d->findCommByFreq, 1, freqKhz);
  if (aFilter) {
    sqlite3_bind_int(d->findCommByFreq, 2, aFilter->minType());
    sqlite3_bind_int(d->findCommByFreq, 3, aFilter->maxType());
  } else { // full type range
    sqlite3_bind_int(d->findCommByFreq, 2, FGPositioned::FREQ_GROUND);
    sqlite3_bind_int(d->findCommByFreq, 3, FGPositioned::FREQ_UNICOM);
  }
  
  SGVec3d cartPos(SGVec3d::fromGeod(aPos));
  sqlite3_bind_double(d->findCommByFreq, 4, cartPos.x());
  sqlite3_bind_double(d->findCommByFreq, 5, cartPos.y());
  sqlite3_bind_double(d->findCommByFreq, 6, cartPos.z());
  
  while (d->execSelect(d->findCommByFreq)) {
    FGPositioned* p = loadById(sqlite3_column_int64(d->findCommByFreq, 0));
    if (aFilter && !aFilter->pass(p)) {
      continue;
    }
    
    return p;
  }
  
  return NULL;
}
  
PositionedIDVec
NavDataCache::findNavaidsByFreq(int freqKhz, const SGGeod& aPos, FGPositioned::Filter* aFilter)
{
  d->reset(d->findNavsByFreq);
  sqlite3_bind_int(d->findNavsByFreq, 1, freqKhz);
  if (aFilter) {
    sqlite3_bind_int(d->findNavsByFreq, 2, aFilter->minType());
    sqlite3_bind_int(d->findNavsByFreq, 3, aFilter->maxType());
  } else { // full type range
    sqlite3_bind_int(d->findNavsByFreq, 2, FGPositioned::NDB);
    sqlite3_bind_int(d->findNavsByFreq, 3, FGPositioned::GS);
  }
  
  SGVec3d cartPos(SGVec3d::fromGeod(aPos));
  sqlite3_bind_double(d->findNavsByFreq, 4, cartPos.x());
  sqlite3_bind_double(d->findNavsByFreq, 5, cartPos.y());
  sqlite3_bind_double(d->findNavsByFreq, 6, cartPos.z());
  
  return d->selectIds(d->findNavsByFreq);
}

PositionedIDVec
NavDataCache::findNavaidsByFreq(int freqKhz, FGPositioned::Filter* aFilter)
{
  d->reset(d->findNavsByFreqNoPos);
  sqlite3_bind_int(d->findNavsByFreqNoPos, 1, freqKhz);
  if (aFilter) {
    sqlite3_bind_int(d->findNavsByFreqNoPos, 2, aFilter->minType());
    sqlite3_bind_int(d->findNavsByFreqNoPos, 3, aFilter->maxType());
  } else { // full type range
    sqlite3_bind_int(d->findNavsByFreqNoPos, 2, FGPositioned::NDB);
    sqlite3_bind_int(d->findNavsByFreqNoPos, 3, FGPositioned::GS);
  }
  
  return d->selectIds(d->findNavsByFreqNoPos);
}
  
PositionedIDVec
NavDataCache::airportItemsOfType(PositionedID apt,FGPositioned::Type ty,
                                 FGPositioned::Type maxTy)
{
  if (maxTy == FGPositioned::INVALID) {
    maxTy = ty; // single-type range
  }
  
  d->reset(d->getAirportItems);
  sqlite3_bind_int64(d->getAirportItems, 1, apt);
  sqlite3_bind_int(d->getAirportItems, 2, ty);
  sqlite3_bind_int(d->getAirportItems, 3, maxTy);
  
  return d->selectIds(d->getAirportItems);
}

PositionedID
NavDataCache::airportItemWithIdent(PositionedID apt, FGPositioned::Type ty,
                                   const std::string& ident)
{
  d->reset(d->getAirportItemByIdent);
  sqlite3_bind_int64(d->getAirportItemByIdent, 1, apt);
  sqlite_bind_stdstring(d->getAirportItemByIdent, 2, ident);
  sqlite3_bind_int(d->getAirportItemByIdent, 3, ty);
  
  if (!d->execSelect(d->getAirportItemByIdent)) {
    return 0;
  }
  
  return sqlite3_column_int64(d->getAirportItemByIdent, 0);
}
  
AirportRunwayPair
NavDataCache::findAirportRunway(const std::string& aName)
{
  if (aName.empty()) {
    return AirportRunwayPair();
  }
  
  string_list parts = simgear::strutils::split(aName);
  if (parts.size() < 2) {
    SG_LOG(SG_NAVCACHE, SG_WARN, "findAirportRunway: malformed name:" << aName);
    return AirportRunwayPair();
  }

  d->reset(d->findAirportRunway);
  sqlite_bind_stdstring(d->findAirportRunway, 1, parts[0]);
  sqlite_bind_stdstring(d->findAirportRunway, 2, parts[1]);
  if (!d->execSelect(d->findAirportRunway)) {
    SG_LOG(SG_NAVCACHE, SG_WARN, "findAirportRunway: unknown airport/runway:" << aName);
    return AirportRunwayPair();
  }

  // success, extract the IDs and continue
  return AirportRunwayPair(sqlite3_column_int64(d->findAirportRunway, 0),
                           sqlite3_column_int64(d->findAirportRunway, 1));
}
  
PositionedID
NavDataCache::findILS(PositionedID airport, const string& runway, const string& navIdent)
{
  d->reset(d->findILS);
  sqlite_bind_stdstring(d->findILS, 1, navIdent);
  sqlite3_bind_int64(d->findILS, 2, airport);
  sqlite_bind_stdstring(d->findILS, 3, runway);
  
  if (!d->execSelect(d->findILS)) {
    return 0;
  }
  
  return sqlite3_column_int64(d->findILS, 0);
}
  
int NavDataCache::findAirway(int network, const string& aName)
{
  d->reset(d->findAirway);
  sqlite3_bind_int(d->findAirway, 1, network);
  sqlite_bind_stdstring(d->findAirway, 2, aName);
  if (d->execSelect(d->findAirway)) {
    // already exists
    return sqlite3_column_int(d->findAirway, 0);
  }
  
  d->reset(d->insertAirway);
  sqlite_bind_stdstring(d->insertAirway, 1, aName);
  sqlite3_bind_int(d->insertAirway, 2, network);
  return d->execInsert(d->insertAirway);
}

void NavDataCache::insertEdge(int network, int airwayID, PositionedID from, PositionedID to)
{
  // assume all edges are bidirectional for the moment
  for (int i=0; i<2; ++i) {
    d->reset(d->insertAirwayEdge);
    sqlite3_bind_int(d->insertAirwayEdge, 1, network);
    sqlite3_bind_int(d->insertAirwayEdge, 2, airwayID);
    sqlite3_bind_int64(d->insertAirwayEdge, 3, from);
    sqlite3_bind_int64(d->insertAirwayEdge, 4, to);
    d->execInsert(d->insertAirwayEdge);
    
    std::swap(from, to);
  }
}
  
bool NavDataCache::isInAirwayNetwork(int network, PositionedID pos)
{
  d->reset(d->isPosInAirway);
  sqlite3_bind_int(d->isPosInAirway, 1, network);
  sqlite3_bind_int64(d->isPosInAirway, 2, pos);
  bool ok = d->execSelect(d->isPosInAirway);
  return ok;
}

AirwayEdgeVec NavDataCache::airwayEdgesFrom(int network, PositionedID pos)
{
  d->reset(d->airwayEdgesFrom);
  sqlite3_bind_int(d->airwayEdgesFrom, 1, network);
  sqlite3_bind_int64(d->airwayEdgesFrom, 2, pos);
  
  AirwayEdgeVec result;
  while (d->stepSelect(d->airwayEdgesFrom)) {
    result.push_back(AirwayEdge(
                     sqlite3_column_int(d->airwayEdgesFrom, 0),
                     sqlite3_column_int64(d->airwayEdgesFrom, 1)
                     ));
  }
  return result;
}

PositionedID NavDataCache::findNavaidForRunway(PositionedID runway, FGPositioned::Type ty)
{
  d->reset(d->findNavaidForRunway);
  sqlite3_bind_int64(d->findNavaidForRunway, 1, runway);
  sqlite3_bind_int(d->findNavaidForRunway, 2, ty);
  if (!d->execSelect(d->findNavaidForRunway)) {
    return 0;
  }
  
  return sqlite3_column_int64(d->findNavaidForRunway, 0);
}
  
PositionedID
NavDataCache::insertParking(const std::string& name, const SGGeod& aPos,
                            PositionedID aAirport,
                           double aHeading, int aRadius, const std::string& aAircraftType,
                           const std::string& aAirlines)
{
  sqlite3_int64 rowId = d->insertPositioned(FGPositioned::PARKING, name, "", aPos, aAirport, false);
  
// we need to insert a row into the taxi_node table, otherwise we can't maintain
// the appropriate pushback flag.
  d->reset(d->insertTaxiNode);
  sqlite3_bind_int64(d->insertTaxiNode, 1, rowId);
  sqlite3_bind_int(d->insertTaxiNode, 2, 0);
  sqlite3_bind_int(d->insertTaxiNode, 3, 0);
  d->execInsert(d->insertTaxiNode);
  
  d->reset(d->insertParkingPos);
  sqlite3_bind_int64(d->insertParkingPos, 1, rowId);
  sqlite3_bind_double(d->insertParkingPos, 2, aHeading);
  sqlite3_bind_int(d->insertParkingPos, 3, aRadius);
  sqlite_bind_stdstring(d->insertParkingPos, 4, aAircraftType);
  sqlite_bind_stdstring(d->insertParkingPos, 5, aAirlines);
  return d->execInsert(d->insertParkingPos);
}
  
void NavDataCache::setParkingPushBackRoute(PositionedID parking, PositionedID pushBackNode)
{
  d->reset(d->setParkingPushBack);
  sqlite3_bind_int64(d->setParkingPushBack, 1, parking);
  sqlite3_bind_int64(d->setParkingPushBack, 2, pushBackNode);
  d->execUpdate(d->setParkingPushBack);
}

PositionedID
NavDataCache::insertTaxiNode(const SGGeod& aPos, PositionedID aAirport, int aHoldType, bool aOnRunway)
{
  sqlite3_int64 rowId = d->insertPositioned(FGPositioned::TAXI_NODE, string(), string(), aPos, aAirport, false);
  d->reset(d->insertTaxiNode);
  sqlite3_bind_int64(d->insertTaxiNode, 1, rowId);
  sqlite3_bind_int(d->insertTaxiNode, 2, aHoldType);
  sqlite3_bind_int(d->insertTaxiNode, 3, aOnRunway);
  return d->execInsert(d->insertTaxiNode);
}
  
void NavDataCache::insertGroundnetEdge(PositionedID aAirport, PositionedID from, PositionedID to)
{
  d->reset(d->insertTaxiEdge);
  sqlite3_bind_int64(d->insertTaxiEdge, 1, aAirport);
  sqlite3_bind_int64(d->insertTaxiEdge, 2, from);
  sqlite3_bind_int64(d->insertTaxiEdge, 3, to);
  d->execInsert(d->insertTaxiEdge);
}
  
PositionedIDVec NavDataCache::groundNetNodes(PositionedID aAirport, bool onlyPushback)
{
  sqlite3_stmt_ptr q = onlyPushback ? d->airportPushbackNodes : d->airportTaxiNodes;
  d->reset(q);
  sqlite3_bind_int64(q, 1, aAirport);
  return d->selectIds(q);
}
  
void NavDataCache::markGroundnetAsPushback(PositionedID nodeId)
{
  d->reset(d->markTaxiNodeAsPushback);
  sqlite3_bind_int64(d->markTaxiNodeAsPushback, 1, nodeId);
  d->execUpdate(d->markTaxiNodeAsPushback);
}

static double headingDifferenceDeg(double crs1, double crs2)
{
  double diff =  crs2 - crs1;
  SG_NORMALIZE_RANGE(diff, -180.0, 180.0);
  return diff;
}
  
PositionedID NavDataCache::findGroundNetNode(PositionedID airport, const SGGeod& aPos,
                                             bool onRunway, FGRunway* aRunway)
{
  sqlite3_stmt_ptr q = onRunway ? d->findNearestRunwayTaxiNode : d->findNearestTaxiNode;
  d->reset(q);
  sqlite3_bind_int64(q, 1, airport);
  
  SGVec3d cartPos(SGVec3d::fromGeod(aPos));
  sqlite3_bind_double(q, 2, cartPos.x());
  sqlite3_bind_double(q, 3, cartPos.y());
  sqlite3_bind_double(q, 4, cartPos.z());
  
  while (d->execSelect(q)) {
    PositionedID id = sqlite3_column_int64(q, 0);
    if (!aRunway) {
      return id;
    }
    
  // ensure found node lies on the runway
    FGPositionedRef node = loadById(id);
    double course = SGGeodesy::courseDeg(node->geod(), aRunway->end());
    if (fabs(headingDifferenceDeg(course, aRunway->headingDeg())) < 3.0 ) {
      return id;
    }
  }
  
  return 0;
}
  
PositionedIDVec NavDataCache::groundNetEdgesFrom(PositionedID pos, bool onlyPushback)
{
  sqlite3_stmt_ptr q = onlyPushback ? d->pushbackEdgesFrom : d->taxiEdgesFrom;
  d->reset(q);
  sqlite3_bind_int64(q, 1, pos);
  return d->selectIds(q);
}

PositionedIDVec NavDataCache::findAirportParking(PositionedID airport, const std::string& flightType,
                                   int radius)
{
  d->reset(d->findAirportParking);
  sqlite3_bind_int64(d->findAirportParking, 1, airport);
  sqlite3_bind_int(d->findAirportParking, 2, radius);
  sqlite_bind_stdstring(d->findAirportParking, 3, flightType);
  
  return d->selectIds(d->findAirportParking);
}

} // of namespace flightgear

