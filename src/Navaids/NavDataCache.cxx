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

#include "config.h"

#include "NavDataCache.hxx"

// std
#include <cstddef>  // for std::size_t
#include <map>
#include <cstring>  // for memcoy
#include <cassert>
#include <stdint.h> // for int64_t
#include <sstream>  // for std::ostringstream
#include <mutex>

#ifdef SYSTEM_SQLITE
// the standard sqlite3.h doesn't give a way to set SQLITE_UINT64_TYPE,
// so we have to hope sizeof(int64_t) matches sizeof(sqlite3_int64).
// otherwise things will go bad quickly.
  #include "sqlite3.h"
#else
// to ensure compatability between sqlite3_int64 and PositionedID,
// force the type used by sqlite to match PositionedID explicitly
#define SQLITE_INT64_TYPE int64_t
#define SQLITE_UINT64_TYPE uint64_t

  #include "fg_sqlite3.h"
#endif

#if defined(SG_WINDOWS)
#define WIN32_LEAN_AND_MEAN // less crap :)
#include <Windows.h>
#endif

// SimGear
#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/threads/SGThread.hxx>

#include "CacheSchema.h"
#include "PositionedOctree.hxx"
#include "fix.hxx"
#include "markerbeacon.hxx"
#include "navrecord.hxx"
#include "poidb.hxx"
#include <ATC/CommStation.hxx>
#include <Airports/airport.hxx>
#include <Airports/apt_loader.hxx>
#include <Airports/gnnode.hxx>
#include <Airports/parking.hxx>
#include <Airports/runways.hxx>
#include <GUI/MessageBox.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/options.hxx>
#include <Main/sentryIntegration.hxx>
#include <Navaids/airways.hxx>
#include <Navaids/fixlist.hxx>
#include <Navaids/navdb.hxx>

using std::string;

#define SG_NAVCACHE SG_NAVAID
//#define LAZY_OCTREE_UPDATES 1

namespace {

const int MAX_RETRIES = 10;

const int CACHE_SIZE_KBYTES= 32 * 1024;

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
  // canonicalise runway ident to two digits and one optional uppercase letter
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
    _phase(NavDataCache::REBUILD_UNKNOWN),
    _completionPercent(0),
  _isFinished(false)
  {

  }

  bool isFinished() const
  {
    std::lock_guard<std::mutex> g(_lock);
    return _isFinished;
  }

  virtual void run()
  {
    SGTimeStamp st;
    st.stamp();
    _cache->doRebuild();
    SG_LOG(SG_NAVCACHE, SG_INFO, "cache rebuild took:" << st.elapsedMSec() << "msec");

    std::lock_guard<std::mutex> g(_lock);
    _isFinished = true;
    _phase = NavDataCache::REBUILD_DONE;
  }

    NavDataCache::RebuildPhase currentPhase() const
    {
        NavDataCache::RebuildPhase ph;
        std::lock_guard<std::mutex> g(_lock);
        ph = _phase;
        return ph;
    }

    unsigned int completionPercent() const
    {
        unsigned int perc = 0;
        std::lock_guard<std::mutex> g(_lock);
        perc = _completionPercent;
        return perc;
    }

    void setProgress(NavDataCache::RebuildPhase ph, unsigned int percent)
    {
        std::lock_guard<std::mutex> g(_lock);
        _phase = ph;
        _completionPercent = percent;
    }

private:
  NavDataCache* _cache;
    NavDataCache::RebuildPhase _phase;
    unsigned int _completionPercent;
  mutable std::mutex _lock;
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
      SG_UNUSED(airport);
  }
};

// useful for debugging 'hanging' queries: look for a statement
// which starts but never completes
#if 0
int traceCallback(unsigned int traceCode, void* ctx, void* p, void* x)
{
    if (traceCode == SQLITE_TRACE_STMT) {
        SG_LOG(SG_NAVCACHE, SG_WARN, "step:" << p << " text=" << (char*)x);
    }
    else if (traceCode == SQLITE_TRACE_PROFILE) {
        int64_t* nanoSecs = (int64_t*)x;
        SG_LOG(SG_NAVCACHE, SG_WARN, "profile:" << p << " took " << *nanoSecs);
    }

    return 0;
}
#endif

class NavDataCache::NavDataCachePrivate
{
public:
  NavDataCachePrivate(const SGPath& p, NavDataCache* o) :
    outer(o),
    db(nullptr),
    path(p),
    readOnly(false),
    cacheHits(0),
    cacheMisses(0),
    transactionLevel(0),
    transactionAborted(false)
  {
  }

  ~NavDataCachePrivate()
  {
    close();
  }

  void init()
  {
      SG_LOG(SG_NAVCACHE, SG_INFO, "NavCache at:" << path);

      readOnly = fgGetBool("/sim/fghome-readonly", false);
      SG_LOG(SG_NAVCACHE, SG_INFO, "NavCache read-only flags is:" << readOnly);

      if (!readOnly && !path.canWrite()) {
          throw sg_exception("Nav-cache file is not writeable");
      }

      if (outer->isAnotherProcessRebuilding()) {
          SG_LOG(SG_NAVCACHE, SG_ALERT, "NavDataCache: init() while another processing is rebuilding the DB");
      }

      int openFlags = readOnly ? SQLITE_OPEN_READONLY :
        (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
      std::string pathUtf8 = path.utf8Str();
      int result = sqlite3_open_v2(pathUtf8.c_str(), &db, openFlags, NULL);
      if (result == SQLITE_MISUSE) {
          // docs state sqlite3_errmsg may not work in this case
          SG_LOG(SG_NAVCACHE, SG_WARN, "Failed to open DB at " << path << ": misuse of Sqlite API");
          throw sg_exception("Navcache failed to open: Sqlite API misuse");
      } else if (result != SQLITE_OK) {
          std::string errMsg = sqlite3_errmsg(db);
          SG_LOG(SG_NAVCACHE, SG_WARN, "Failed to open DB at " << path << " with error:\n\t" << errMsg);
          throw sg_exception("Navcache failed to open:" + errMsg);
      }

      if (!readOnly && (sqlite3_db_readonly(db, nullptr) != 0)) {
          throw sg_exception("Nav-cache file opened but is not writeable");
      }

  // enable tracing for debugging stuck queries
   //   sqlite3_trace_v2(db, SQLITE_TRACE_STMT | SQLITE_TRACE_PROFILE, traceCallback, this);

    sqlite3_stmt_ptr checkTables =
      prepare("SELECT count(*) FROM sqlite_master WHERE name='properties'");

    sqlite3_create_function(db, "distanceCartSqr", 6, SQLITE_ANY, NULL,
                            f_distanceCartSqrFunction, NULL, NULL);

    execSelect(checkTables);
    bool didCreate = false;
    if (!readOnly && (sqlite3_column_int(checkTables, 0) == 0)) {
      SG_LOG(SG_NAVCACHE, SG_INFO, "will create tables");
      initTables();
      didCreate = true;
    }

    reset(checkTables);

    readPropertyQuery = prepare("SELECT value FROM properties WHERE key=?");
    writePropertyQuery = prepare("INSERT INTO properties (key, value) VALUES (?,?)");
    clearProperty = prepare("DELETE FROM properties WHERE key=?1");

    if (didCreate) {
      writeIntProperty("schema-version", SCHEMA_VERSION);
    } else {
      int schemaVersion = outer->readIntProperty("schema-version");
      if (schemaVersion != SCHEMA_VERSION) {
        SG_LOG(SG_NAVCACHE, SG_INFO, "Navcache schema mismatch, will rebuild");
        throw sg_exception("Navcache schema has changed");
      }
    }

    // see http://www.sqlite.org/pragma.html#pragma_cache_size
    // for the details, small cache would cause thrashing.
    std::ostringstream q;
    q << "PRAGMA cache_size=-" << CACHE_SIZE_KBYTES << ";";
    runSQL(q.str());
    prepareQueries();
  }

  void close()
  {
    for (sqlite3_stmt_ptr stmt : prepared) {
      sqlite3_finalize(stmt);
    }
    prepared.clear();
    sqlite3_close(db);
  }

  void checkCacheFile()
  {
    SG_LOG(SG_NAVCACHE, SG_INFO, "running DB integrity check");
    SGTimeStamp st;
    st.stamp();

    sqlite3_stmt_ptr stmt = prepare("PRAGMA quick_check(1)");
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

  bool isCachedFileModified(const SGPath& path, bool verbose);
  void findDatFiles(NavDataCache::DatFileType datFileType);
  bool areDatFilesModified(
    NavDataCache::DatFileType datFileType,
    bool verbose);

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
    int retries = 0;
    int result;
    while (retries < MAX_RETRIES) {
      result = sqlite3_step(stmt);
      if (result == SQLITE_ROW) {
        return true; // at least one result row
      }

      if (result == SQLITE_DONE) {
        return false; // no result rows
      }

      if (result != SQLITE_BUSY) {
        break;
      }

      SG_LOG(SG_NAVCACHE, SG_ALERT, "NavCache contention on select, will retry:" << retries);
      SGTimeStamp::sleepForMSec(++retries * 10);
    } // of retry loop for DB locked

    if (retries >= MAX_RETRIES) {
      SG_LOG(SG_NAVCACHE, SG_ALERT, "exceeded maximum number of SQLITE_BUSY retries");
      return false;
    }

    string errMsg;
    if (result == SQLITE_MISUSE) {
      errMsg = "Sqlite API abuse";
      SG_LOG(SG_NAVCACHE, SG_ALERT, "Sqlite API abuse");
    } else {
      errMsg = sqlite3_errmsg(db);
      SG_LOG(SG_NAVCACHE, SG_ALERT, "Sqlite error:" << errMsg << " (" << result
             << ") while running:\n\t" << sqlite3_sql(stmt));
    }

    throw sg_exception("Sqlite error:" + errMsg, sqlite3_sql(stmt));
  }

  void execSelect1(sqlite3_stmt_ptr stmt)
  {
    if (!execSelect(stmt)) {
      SG_LOG(SG_NAVCACHE, SG_WARN, "empty SELECT running:\n\t" << sqlite3_sql(stmt));
      flightgear::sentryReportException("empty SELECT running:" + std::string{sqlite3_sql(stmt)});
      throw sg_exception("no results returned for select", sqlite3_sql(stmt));
    }
  }

  sqlite3_int64 execInsert(sqlite3_stmt_ptr stmt)
  {
    execSelect(stmt);
    sqlite3_int64 rowid = sqlite3_last_insert_rowid(db);
    reset(stmt);
    return rowid;
  }

  void execUpdate(sqlite3_stmt_ptr stmt)
  {
    execSelect(stmt);
    reset(stmt);
  }

  void initTables()
  {
      string_list commands = simgear::strutils::split(SCHEMA_SQL, ";");
      for (std::string sql : commands) {
          if (sql.empty()) {
              continue;
          }

          runSQL(sql);
      } // of commands in scheme loop
  }

  void prepareQueries()
  {
    writePropertyMulti = prepare("INSERT INTO properties (key, value) VALUES(?1,?2)");

    beginTransactionStmt = prepare("BEGIN");
    commitTransactionStmt = prepare("COMMIT");
    rollbackTransactionStmt = prepare("ROLLBACK");


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
    setNavaidColocated = prepare("UPDATE navaid SET colocated=?2 WHERE rowid=?1");

    insertPositionedQuery = prepare("INSERT INTO positioned "
                                    "(type, ident, name, airport, lon, lat, elev_m, octree_node, "
                                    "cart_x, cart_y, cart_z)"
                                    " VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)");

    setAirportPos = prepare("UPDATE positioned SET lon=?2, lat=?3, elev_m=?4, octree_node=?5, "
                            "cart_x=?6, cart_y=?7, cart_z=?8 WHERE rowid=?1");
    insertAirport = prepare("INSERT INTO airport (rowid, has_metar) VALUES (?, ?)");
    insertNavaid = prepare("INSERT INTO navaid (rowid, freq, range_nm, multiuse, runway, colocated)"
                           " VALUES (?1, ?2, ?3, ?4, ?5, ?6)");

    insertCommStation = prepare("INSERT INTO comm (rowid, freq_khz, range_nm)"
                                " VALUES (?, ?, ?)");
    insertRunway = prepare("INSERT INTO runway "
                           "(rowid, heading, length_ft, width_m, surface, displaced_threshold, stopway, reciprocal)"
                           " VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)");
    runwayLengthFtQuery = prepare("SELECT length_ft FROM runway WHERE rowid=?1");

    removePOIQuery = prepare("DELETE FROM positioned WHERE type=?1 AND ident=?2");

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

    getAllAirports = prepare("SELECT ident, name FROM positioned WHERE type>=?1 AND type <=?2");
    sqlite3_bind_int(getAllAirports, 1, FGPositioned::AIRPORT);
    sqlite3_bind_int(getAllAirports, 2, FGPositioned::SEAPORT);


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
    findAirwayNet = prepare("SELECT rowid FROM airway WHERE network=?1 AND ident=?2");
    findAirway = prepare("SELECT rowid FROM airway WHERE ident=?1");
    loadAirway = prepare("SELECT ident, network FROM airway WHERE rowid=?1");
    insertAirway = prepare("INSERT INTO airway (ident, network) "
                           "VALUES (?1, ?2)");

    insertAirwayEdge = prepare("INSERT INTO airway_edge (network, airway, a, b) "
                               "VALUES (?1, ?2, ?3, ?4)");

    isPosInAirway = prepare("SELECT rowid FROM airway_edge WHERE network=?1 AND (a=?2 OR b=?2)");

    airwayEdgesFrom = prepare("SELECT airway, b FROM airway_edge WHERE network=?1 AND a=?2");
    airwayEdgesTo = prepare("SELECT airway, a FROM airway_edge WHERE network=?1 AND b=?2");
    airwayEdges = prepare("SELECT a, b FROM airway_edge WHERE airway=?1");
  }

  void writeIntProperty(const string& key, int value)
  {
    if (!readOnly){
      sqlite_bind_stdstring(clearProperty, 1, key);
      execUpdate(clearProperty);

      sqlite_bind_stdstring(writePropertyQuery, 1, key);
      sqlite3_bind_int(writePropertyQuery, 2, value);
      execUpdate(writePropertyQuery);
    }
  }


  FGPositioned* loadById(sqlite_int64 rowId, sqlite3_int64& aptId);

  FGAirport* loadAirport(sqlite_int64 rowId,
                         FGPositioned::Type ty,
                         const string& id, const string& name, const SGGeod& pos)
  {
    sqlite3_bind_int64(loadAirportStmt, 1, rowId);
    execSelect1(loadAirportStmt);
    bool hasMetar = (sqlite3_column_int(loadAirportStmt, 0) > 0);
    reset(loadAirportStmt);

    return new FGAirport(rowId, id, pos, name, hasMetar, ty);
  }

  FGRunwayBase* loadRunway(sqlite3_int64 rowId, FGPositioned::Type ty,
                           const string& id, const SGGeod& pos, PositionedID apt)
  {
    sqlite3_bind_int(loadRunwayStmt, 1, rowId);
    execSelect1(loadRunwayStmt);

    double heading = sqlite3_column_double(loadRunwayStmt, 0);
    double lengthM = sqlite3_column_int(loadRunwayStmt, 1);
    double widthM = sqlite3_column_double(loadRunwayStmt, 2);
    int surface = sqlite3_column_int(loadRunwayStmt, 3);

    if (ty == FGPositioned::TAXIWAY) {
      reset(loadRunwayStmt);
      return new FGTaxiway(rowId, id, pos, heading, lengthM, widthM, surface);
    } else if (ty == FGPositioned::HELIPAD) {
        reset(loadRunwayStmt);
        return new FGHelipad(rowId, apt, id, pos, heading, lengthM, widthM, surface);
    } else {
      double displacedThreshold = sqlite3_column_double(loadRunwayStmt, 4);
      double stopway = sqlite3_column_double(loadRunwayStmt, 5);
      PositionedID reciprocal = sqlite3_column_int64(loadRunwayStmt, 6);
      PositionedID ils = sqlite3_column_int64(loadRunwayStmt, 7);
      FGRunway* r = new FGRunway(rowId, apt, id, pos, heading, lengthM, widthM,
                          displacedThreshold, stopway, surface);

      if (reciprocal > 0) {
        r->setReciprocalRunway(reciprocal);
      }

      if (ils > 0) {
        r->setILS(ils);
      }

      reset(loadRunwayStmt);
      return r;
    }
  }

  CommStation* loadComm(sqlite3_int64 rowId, FGPositioned::Type ty,
                        const string& id, const string& name,
                        const SGGeod& pos,
                        PositionedID airport)
  {
      SG_UNUSED(id);

      sqlite3_bind_int64(loadCommStation, 1, rowId);
      execSelect1(loadCommStation);

      int range = sqlite3_column_int(loadCommStation, 0);
      int freqKhz = sqlite3_column_int(loadCommStation, 1);
      reset(loadCommStation);

      CommStation* c = new CommStation(rowId, name, ty, pos, freqKhz, range);
      c->setAirport(airport);
      return c;
  }

  FGPositioned* loadNav(sqlite3_int64 rowId,
                       FGPositioned::Type ty, const string& id,
                       const string& name, const SGGeod& pos)
  {
    sqlite3_bind_int64(loadNavaid, 1, rowId);
    execSelect1(loadNavaid);

    PositionedID runway = sqlite3_column_int64(loadNavaid, 3);
    // marker beacons are light-weight
    if ((ty == FGPositioned::OM) || (ty == FGPositioned::IM) ||
        (ty == FGPositioned::MM))
    {
      reset(loadNavaid);
      return new FGMarkerBeaconRecord(rowId, ty, runway, pos);
    }

    int rangeNm = sqlite3_column_int(loadNavaid, 0),
    freq = sqlite3_column_int(loadNavaid, 1);
    double mulituse = sqlite3_column_double(loadNavaid, 2);
    PositionedID colocated = sqlite3_column_int64(loadNavaid, 4);
    reset(loadNavaid);

    FGNavRecord* n =
      (ty == FGPositioned::MOBILE_TACAN)
      ? new FGMobileNavRecord
            (rowId, ty, id, name, pos, freq, rangeNm, mulituse, runway)
      : new FGNavRecord
            (rowId, ty, id, name, pos, freq, rangeNm, mulituse, runway);

    if (colocated)
      n->setColocatedDME(colocated);

    return n;
  }

  PositionedID insertPositioned(FGPositioned::Type ty, const string& ident,
                                const string& name, const SGGeod& pos, PositionedID apt,
                                bool spatialIndex)
  {
    SGVec3d cartPos(SGVec3d::fromGeod(pos));

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

  FGPositionedList findAllByString(const string& s, const string& column,
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

    sqlite_bind_stdstring(stmt, 1, query);
    if (filter) {
      sqlite3_bind_int(stmt, 2, filter->minType());
      sqlite3_bind_int(stmt, 3, filter->maxType());
    }

    FGPositionedList result;
  // run the prepared SQL
    while (stepSelect(stmt))
    {
      FGPositioned* pos = outer->loadById(sqlite3_column_int64(stmt, 0));
      if (filter && !filter->pass(pos)) {
        continue;
      }

      result.push_back(pos);
    }

    reset(stmt);
    return result;
  }

  PositionedIDVec selectIds(sqlite3_stmt_ptr query)
  {
    PositionedIDVec result;
    while (stepSelect(query)) {
      result.push_back(sqlite3_column_int64(query, 0));
    }
    reset(query);
    return result;
  }

  double runwayLengthFt(PositionedID rwy)
  {
    sqlite3_bind_int64(runwayLengthFtQuery, 1, rwy);
    execSelect1(runwayLengthFtQuery);
    double length = sqlite3_column_double(runwayLengthFtQuery, 0);
    reset(runwayLengthFtQuery);
    return length;
  }

  void flushDeferredOctreeUpdates()
  {
    for (Octree::Branch* nd : deferredOctreeUpdates) {
      sqlite3_bind_int64(updateOctreeChildren, 1, nd->guid());
      sqlite3_bind_int(updateOctreeChildren, 2, nd->childMask());
      execUpdate(updateOctreeChildren);
    }

    deferredOctreeUpdates.clear();
  }

  void removePositionedWithIdent(FGPositioned::Type ty, const std::string& aIdent)
  {
    sqlite3_bind_int(removePOIQuery, 1, ty);
    sqlite_bind_stdstring(removePOIQuery, 2, aIdent);
    execUpdate(removePOIQuery);
    reset(removePOIQuery);
  }

  NavDataCache* outer;
  sqlite3* db;
  SGPath path;
    bool readOnly;

  /// the actual cache of ID -> instances. This holds an owning reference,
  /// so once items are in the cache they will never be deleted until
  /// the cache drops its reference
  PositionedCache cache;
  unsigned int cacheHits, cacheMisses;

  /**
   * record the levels of open transaction objects we have
   */
  unsigned int transactionLevel;
  bool transactionAborted;
  sqlite3_stmt_ptr beginTransactionStmt, commitTransactionStmt, rollbackTransactionStmt;

  std::map<DatFileType, NavDataCache::DatFilesGroupInfo> datFilesInfo;
  SGPath metarDatPath, poiDatPath,
  carrierDatPath, airwayDatPath;

  sqlite3_stmt_ptr readPropertyQuery, writePropertyQuery,
    stampFileCache, statCacheCheck,
    loadAirportStmt, loadCommStation, loadPositioned, loadNavaid,
    loadRunwayStmt;
  sqlite3_stmt_ptr writePropertyMulti, clearProperty;

  sqlite3_stmt_ptr insertPositionedQuery, insertAirport, insertTower, insertRunway,
  insertCommStation, insertNavaid;
  sqlite3_stmt_ptr setAirportMetar, setRunwayReciprocal, setRunwayILS, setNavaidColocated,
    setAirportPos;
  sqlite3_stmt_ptr removePOIQuery;

  sqlite3_stmt_ptr findClosestWithIdent;
// octree (spatial index) related queries
  sqlite3_stmt_ptr getOctreeChildren, insertOctree, updateOctreeChildren,
    getOctreeLeafChildren;

  sqlite3_stmt_ptr searchAirports, getAllAirports;
  sqlite3_stmt_ptr findCommByFreq, findNavsByFreq,
  findNavsByFreqNoPos, findNavaidForRunway;
  sqlite3_stmt_ptr getAirportItems, getAirportItemByIdent;
  sqlite3_stmt_ptr findAirportRunway,
    findILS;

  sqlite3_stmt_ptr runwayLengthFtQuery;

// airways
  sqlite3_stmt_ptr findAirway, findAirwayNet, insertAirwayEdge,
    isPosInAirway, airwayEdgesFrom, airwayEdgesTo,
    insertAirway, airwayEdges;
  sqlite3_stmt_ptr loadAirway;

// since there's many permutations of ident/name queries, we create
// them programtically, but cache the exact query by its raw SQL once
// used.
  std::map<string, sqlite3_stmt_ptr> findByStringDict;

  typedef std::vector<sqlite3_stmt_ptr> StmtVec;
  StmtVec prepared;

  std::set<Octree::Branch*> deferredOctreeUpdates;

  // if we're performing a rebuild, the thread that is doing the work.
  // otherwise, NULL
  std::unique_ptr<RebuildThread> rebuilder;
};

//////////////////////////////////////////////////////////////////////

FGPositioned* NavDataCache::NavDataCachePrivate::loadById(sqlite3_int64 rowid,
                                                          sqlite3_int64& aptId)
{

  sqlite3_bind_int64(loadPositioned, 1, rowid);
  execSelect1(loadPositioned);

  assert(rowid == sqlite3_column_int64(loadPositioned, 0));
  FGPositioned::Type ty = (FGPositioned::Type) sqlite3_column_int(loadPositioned, 1);

  PositionedID prowid = static_cast<PositionedID>(rowid);
  string ident = (char*) sqlite3_column_text(loadPositioned, 2);
  string name = (char*) sqlite3_column_text(loadPositioned, 3);
  aptId = sqlite3_column_int64(loadPositioned, 4);
  double lon = sqlite3_column_double(loadPositioned, 5);
  double lat = sqlite3_column_double(loadPositioned, 6);
  double elev = sqlite3_column_double(loadPositioned, 7);
  SGGeod pos = SGGeod::fromDegM(lon, lat, elev);

  reset(loadPositioned);

  switch (ty) {
    case FGPositioned::AIRPORT:
    case FGPositioned::SEAPORT:
    case FGPositioned::HELIPORT:
      return loadAirport(rowid, ty, ident, name, pos);

    case FGPositioned::TOWER:
      return new AirportTower(prowid, aptId, ident, pos);

    case FGPositioned::RUNWAY:
    case FGPositioned::HELIPAD:
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
      return loadNav(rowid, ty, ident, name, pos);

    case FGPositioned::FIX:
      return new FGFix(rowid, ident, pos);

    case FGPositioned::WAYPOINT:
    case FGPositioned::COUNTRY:
    case FGPositioned::CITY:
    case FGPositioned::TOWN:
    case FGPositioned::VILLAGE:
    {
        FGPositioned* wpt = new FGPositioned(rowid, ty, ident, pos);
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

    default:
      return NULL;
  }
}

bool NavDataCache::NavDataCachePrivate::isCachedFileModified(const SGPath& path, bool verbose)
{
  if (!path.exists()) {
    throw sg_exception(
      "NavCache: missing file '" + path.utf8Str() + "'",
      string("NavDataCache::NavDataCachePrivate::isCachedFileModified()"));
  }

  sqlite_bind_temp_stdstring(statCacheCheck, 1, path.realpath().utf8Str());
  bool isModified = true;
  sgDebugPriority logLevel = verbose ? SG_WARN : SG_DEBUG;
  if (execSelect(statCacheCheck)) {
    time_t modtime = sqlite3_column_int64(statCacheCheck, 0);
    time_t delta = std::labs(modtime - path.modTime());
    if (delta != 0)
    {
      SG_LOG(SG_NAVCACHE, logLevel, "NavCache: rebuild required for " << path <<
             ". Timestamps: " << modtime << " != " << path.modTime());
    }
    else
    {
      SG_LOG(SG_NAVCACHE, SG_DEBUG, "NavCache: no rebuild required for " << path);
    }

    isModified = (delta != 0);
  } else {
    SG_LOG(SG_NAVCACHE, logLevel, "NavCache: (re-)build required because '" <<
           path.utf8Str() << "' is not in the cache");
  }

  reset(statCacheCheck);
  return isModified;
}

// Find the list of $scenery_path/NavData/<type>/*.dat[.gz] files found
// inside scenery paths, plus the default file for the given type (e.g.,
// $FG_ROOT/Airports/apt.dat.gz for the 'apt' type).
// Also compute the total size of all these files (in bytes), which is useful
// for progress information.
// The result is stored in the datFilesInfo field.
void NavDataCache::NavDataCachePrivate::findDatFiles(
  NavDataCache::DatFileType datFileType)
{
  NavDataCache::DatFilesGroupInfo result;
  result.datFileType = datFileType;
  result.totalSize = 0;

  // we don't always add FG_ROOT/Scenery to the path list. But if it exists,
  // we want to pick up NavData files from it, since we have shipped
  // scenery (for BIKF) which uses newer data than the default files
  // in Airports/
  auto paths = globals->get_fg_scenery();
  const auto fgrootScenery = globals->get_fg_root() / "Scenery";
  if (fgrootScenery.exists()) {
    auto it = std::find(paths.begin(), paths.end(), fgrootScenery);
    if (it == paths.end()) {
       paths.push_back(fgrootScenery);
    }
  }

  for (const auto& path: paths) {
    if (! path.isDir()) {
      SG_LOG(SG_NAVCACHE, SG_WARN, path <<
             ": given as a scenery path, but is not a directory");
      continue;
    }

    const SGPath datFilesDir =
      path / "NavData" / NavDataCache::datTypeStr[datFileType];

    if (datFilesDir.isDir()) {
      // Deterministic because the return value of simgear::Dir::children() is
      // already sorted
      const PathList files = simgear::Dir(datFilesDir).children(
        simgear::Dir::TYPE_FILE | simgear::Dir::NO_DOT_OR_DOTDOT);

      for (const auto& f : files) {
        const std::string name = f.file();
        if (simgear::strutils::ends_with(name, ".dat") ||
            simgear::strutils::ends_with(name, ".dat.gz")) {
          result.paths.push_back(f);
          result.totalSize += f.sizeInBytes();
        }
      }
    }
  } // of loop over the list of scenery paths

  // Add the default file (e.g., $FG_ROOT/Airports/apt.dat.gz), at least for
  // now
  SGPath defaultDatFile(globals->get_fg_root());
  defaultDatFile.append(NavDataCache::defaultDatFile[datFileType]);
  if ((result.paths.empty() || result.paths.back() != defaultDatFile) &&
      defaultDatFile.isFile()) {
    result.paths.push_back(defaultDatFile);
    result.totalSize += defaultDatFile.sizeInBytes();
  }

  datFilesInfo[datFileType] = result;
}

// Compare:
//   - the list of dat files given by 'datFilesGroupInfo';
//   - the list obtained from the record with key '<type>.dat files' of the
//     NavDataCache 'properties' table, where <type> is one of 'apt', 'metar',
//     'fix', 'poi', etc..
//
// This comparison is sensitive to the number and order of the files,
// their respective SGPath::realpath() and SGPath::modTime().
bool NavDataCache::NavDataCachePrivate::areDatFilesModified(
  NavDataCache::DatFileType datFileType,
  bool verbose)
{
  // 'apt' or 'metar' or 'fix' or...
  const NavDataCache::DatFilesGroupInfo& datFilesGroupInfo =
        datFilesInfo[datFileType];
  const string datTypeStr =
                       NavDataCache::datTypeStr[datFileType];
  const string_list cachedFiles = outer->readOrderedStringListProperty(
    datTypeStr + ".dat files", SGPath::pathListSep);
  const PathList& datFiles = datFilesGroupInfo.paths;
  PathList::const_iterator datFilesIt = datFiles.begin();
  string_list::const_iterator cachedFilesIt = cachedFiles.begin();
  // Same logic as in NavDataCachePrivate::isCachedFileModified()
  sgDebugPriority logLevel = verbose ? SG_WARN : SG_DEBUG;

  if (cachedFilesIt == cachedFiles.end() && datFilesIt != datFiles.end()) {
     SG_LOG(SG_NAVCACHE, logLevel,
            "NavCache: rebuild required for " << datTypeStr << ".dat files "
            "(no file in cache, but " << datFiles.size() << " such file" <<
            (datFiles.size() > 1 ? "s" : "") << " found in scenery paths)");
     return true;
  }

  while (datFilesIt != datFiles.end()) {
    const SGPath& path = *(datFilesIt++);

    if (!path.exists()) {
      throw sg_exception(
        "NavCache: non-existent file '" + path.utf8Str() + "'",
        string("NavDataCache::NavDataCachePrivate::areDatFilesModified()"));
    }

    if (cachedFilesIt == cachedFiles.end()) {
      SG_LOG(SG_NAVCACHE, logLevel,
             "NavCache: rebuild required for " << datTypeStr << ".dat files "
             "(less files in cache than in scenery paths)");
      return true;
    } else {
      string cachedFile = *(cachedFilesIt++);
      string fileOnDisk = path.realpath().utf8Str();

      if (cachedFile != fileOnDisk || isCachedFileModified(path, verbose)) {
        // isCachedFileModified() does all the logging, so we only have to
        // tell what is happening in case that method was not called.
        if (cachedFile != fileOnDisk) {
          SG_LOG(SG_NAVCACHE, logLevel,
                 "NavCache: rebuild required because '" << cachedFile <<
                 "' (in cache) != '" << fileOnDisk << "' (on disk)");
        }
        return true;
      }
    }
  } // of loop over the elements of 'datFiles'

  if (cachedFilesIt != cachedFiles.end()) {
      SG_LOG(SG_NAVCACHE, logLevel,
             "NavCache: rebuild required for " << datTypeStr << ".dat files "
             "(more files in cache than in scenery paths)");
      return true;
  }

  SG_LOG(SG_NAVCACHE, SG_DEBUG,
         "NavCache: no rebuild required for " << datTypeStr << ".dat files");
  return false;
}


// NavDataCache's static member variables
static NavDataCache* static_instance = NULL;

const string NavDataCache::datTypeStr[] = {
    string("apt"),
    string("metar"),
    string("awy"),
    string("nav"),
    string("fix"),
    string("poi"),
    string("carrier"),
    string("TACAN_freq")
};

const string NavDataCache::defaultDatFile[] = {
    string("Airports/apt.dat.gz"),
    string("Airports/metar.dat.gz"),
    string("Navaids/awy.dat.gz"),
    string("Navaids/nav.dat.gz"),
    string("Navaids/fix.dat.gz"),
    string("Navaids/poi.dat.gz"),
    string("Navaids/carrier.dat.gz"),
    string("Navaids/TACAN_freq.dat.gz")
};

NavDataCache::NavDataCache()
{
    const int MAX_TRIES = 3;
    SGPath homePath(globals->get_fg_home());

    std::ostringstream os;
    string_list versionParts = simgear::strutils::split(VERSION, ".");
    if (versionParts.size() < 2) {
        os << "navdata.cache";
    } else {
        os << "navdata_" << versionParts[0] << "_" << versionParts[1] << ".cache";
    }

    // permit additional DB connections from the same process
    sqlite3_config(SQLITE_CONFIG_MULTITHREAD);

    for (int t=0; t < MAX_TRIES; ++t) {
        SGPath cachePath = homePath / os.str();
        try {
            d.reset(new NavDataCachePrivate(cachePath, this));
            d->init();
            //d->checkCacheFile();
            // reached this point with no exception, success
            break;
        } catch (sg_exception& e) {
            SG_LOG(SG_NAVCACHE, t == 0 ? SG_WARN : SG_ALERT, "NavCache: init failed:" << e.what()
                   << " (attempt " << t << ")");

            if (t == (MAX_TRIES - 1)) {
                // final attempt still failed, we are busted
                flightgear::fatalMessageBoxThenExit(
                  "Unable to open navigation cache",
                  std::string("The navigation data cache could not be opened:")
                  + e.getMessage(), e.getOrigin());
            }

            d.reset();

            // only wipe the existing if not readonly
            if (cachePath.exists() && !fgGetBool("/sim/fghome-readonly", false)) {
                bool ok = cachePath.remove();
                if (!ok) {
                    SG_LOG(SG_NAVCACHE, SG_ALERT, "NavCache: failed to remove previous cache file");
                    flightgear::fatalMessageBoxThenExit(
                      "Unable to re-create navigation cache",
                      "Attempting to remove the old cache failed.",
                      "Location: "  + cachePath.utf8Str());
                }
            }
        }
    } // of retry loop

    double RADIUS_EARTH_M = 7000 * 1000.0; // 7000km is plenty
    SGVec3d earthExtent(RADIUS_EARTH_M, RADIUS_EARTH_M, RADIUS_EARTH_M);
    Octree::global_spatialOctree =
    new Octree::Branch(SGBox<double>(-earthExtent, earthExtent), 1);

    // Update d->aptDatFilesInfo, d->metarDatPath, d->navDatPath, etc.
    updateListsOfDatFiles();
}

NavDataCache::~NavDataCache()
{
  assert(static_instance == this);
  static_instance = nullptr;
  d.reset();

// ensure we wip the airports cache too, or we'll get out
// of sync during tests
  FGAirport::clearAirportsCache();
}

NavDataCache* NavDataCache::createInstance()
{
    assert(static_instance == nullptr);
    static_instance = new NavDataCache;
    return static_instance;
}

NavDataCache* NavDataCache::instance()
{
  return static_instance;
}

// Update the lists of dat files used for NavCache freshness checking and
// rebuilding.
void NavDataCache::updateListsOfDatFiles() {
  // All $scenery_path/NavData/apt/*.dat[.gz] files found inside scenery
  // paths, plus $FG_ROOT/Airports/apt.dat.gz (order matters).
  d->findDatFiles(DATFILETYPE_APT);

  // All $scenery_path/NavData/{nav,fix}/*.dat[.gz] files found inside scenery
  // paths, plus $FG_ROOT/Navaids/{nav,fix}.dat.gz (order matters).
  d->findDatFiles(DATFILETYPE_NAV);
  d->findDatFiles(DATFILETYPE_FIX);

  // These ones are still managed the "old" way (no search through scenery
  // paths).
  d->metarDatPath = SGPath(globals->get_fg_root());
  d->metarDatPath.append("Airports/metar.dat.gz");

  d->poiDatPath = SGPath(globals->get_fg_root());
  d->poiDatPath.append("Navaids/poi.dat.gz");

  d->carrierDatPath = SGPath(globals->get_fg_root());
  d->carrierDatPath.append("Navaids/carrier_nav.dat.gz");

  d->airwayDatPath = SGPath(globals->get_fg_root());
  d->airwayDatPath.append("Navaids/awy.dat.gz");
}

const NavDataCache::DatFilesGroupInfo&
NavDataCache::getDatFilesInfo(DatFileType datFileType) const
{
  auto iter = d->datFilesInfo.find(datFileType);
  if (iter == d->datFilesInfo.end()) {
    throw sg_error("NavCache: requesting info about the list of " +
           datTypeStr[datFileType] + " dat files, however this is not "
           "implemented yet!");
  }
  return iter->second;
}

bool NavDataCache::isRebuildRequired()
{
    if (d->readOnly) {
        return false;
    }

    if (flightgear::Options::sharedInstance()->isOptionSet("restore-defaults")) {
        SG_LOG(SG_NAVCACHE, SG_INFO, "NavCache: restore-defaults requested, will rebuild cache");
        return true;
    }

  if (d->areDatFilesModified(DATFILETYPE_APT, true) ||
      d->isCachedFileModified(d->metarDatPath, true) ||
      d->areDatFilesModified(DATFILETYPE_NAV, true) ||
      d->areDatFilesModified(DATFILETYPE_FIX, true) ||
      d->isCachedFileModified(d->carrierDatPath, true) ||
// since POI loading is disabled on Windows, don't check for it
// this caused: https://code.google.com/p/flightgear-bugs/issues/detail?id=1227
#ifndef SG_WINDOWS
      d->isCachedFileModified(d->poiDatPath, true) ||
#endif
      d->isCachedFileModified(d->airwayDatPath, true))
  {
    SG_LOG(SG_NAVCACHE, SG_INFO, "NavCache: main cache rebuild required");
    return true;
  }

  SG_LOG(SG_NAVCACHE, SG_INFO, "NavCache: no main cache rebuild required");
  return false;
}


#if defined(SG_WINDOWS)
static HANDLE static_fgNavCacheRebuildMutex = nullptr;
#endif


NavDataCache::RebuildPhase NavDataCache::rebuild()
{
#if defined(SG_WINDOWS)
    if (static_fgNavCacheRebuildMutex == nullptr) {
        // avoid multiple copies racing on the nav-cache build
        static_fgNavCacheRebuildMutex = CreateMutexA(nullptr, FALSE, "org.flightgear.fgfs.rebuild-navcache");
        if (static_fgNavCacheRebuildMutex == nullptr) {
            flightgear::fatalMessageBoxThenExit("Multiple copies of Flightgear initializing",
                                                "Failed to create mutex for nav-cache rebuild protection:" + std::to_string(GetLastError()));
        } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
            flightgear::fatalMessageBoxThenExit("Multiple copies of Flightgear initializing",
                                                "Multiple copies of FlightGear are trying to initialise the same navigation database. "
                                                "This means something has gone badly wrong: please report this error.");
        }

        // accquire the mutex, so that other processes can check the status.
        const int result = WaitForSingleObject(static_fgNavCacheRebuildMutex, 100);
        if (result != WAIT_OBJECT_0) {
            flightgear::fatalMessageBoxThenExit("Multiple copies of Flightgear initializing",
                                                "Failed to lock mutex for nav-cache rebuild protection:" + std::to_string(GetLastError()));
        }
    }
#endif

    if (!d->rebuilder.get()) {
        d->rebuilder.reset(new RebuildThread(this));
        d->rebuilder->start();
    }

    // poll the rebuild thread
    RebuildPhase phase = d->rebuilder->currentPhase();
    if (phase == REBUILD_DONE) {
        d->rebuilder.reset(); // all done!
#if defined(SG_WINDOWS)
        ReleaseMutex(static_fgNavCacheRebuildMutex);
#endif
    }
    return phase;
}

bool NavDataCache::isAnotherProcessRebuilding()
{
#if defined(SG_WINDOWS)
    if (!static_fgNavCacheRebuildMutex) {
        static_fgNavCacheRebuildMutex = OpenMutexA(SYNCHRONIZE, FALSE, "org.flightgear.fgfs.rebuild-navcache");
        if (!static_fgNavCacheRebuildMutex) {
            // this is the common case: no other fgfs.exe is doing a rebuild, so
            // the mutex does not exist. Simple, we are done
            if (GetLastError() == ERROR_FILE_NOT_FOUND) {
                return false;
            }

            flightgear::fatalMessageBoxThenExit("Multiple copies of Flightgear initializing",
                                                "Unable to check if other copies of FlightGear are initializing. "
                                                "Please report this error.");
        }
    }

    // poll the named mutex
    auto result = WaitForSingleObject(static_fgNavCacheRebuildMutex, 0);
    if (result == WAIT_OBJECT_0) {
        // we accquired it, release it and we're done
        // (there could be multiple read-only copies in this situation)
        ReleaseMutex(static_fgNavCacheRebuildMutex);
        CloseHandle(static_fgNavCacheRebuildMutex);
        return false;
    }

    // failed to accquire the mutex, so assume another FGFS.exe is rebuilding,
    // the GU should wait.
    return true;
#else
    return false; // not implemented on macOS / Linux for now
#endif
}

unsigned int NavDataCache::rebuildPhaseCompletionPercentage() const
{
    if (!d->rebuilder.get()) {
        return 0;
    }

    return d->rebuilder->completionPercent();
}

void NavDataCache::setRebuildPhaseProgress(RebuildPhase ph, unsigned int percent)
{
    if (!d->rebuilder.get()) {
        return;
    }

    d->rebuilder->setProgress(ph, percent);
}

void NavDataCache::loadDatFiles(
    DatFileType type,
    std::function<void(const SGPath&, std::size_t, std::size_t)> loader)
{
  SGTimeStamp st;
  string typeStr = datTypeStr[type];
  string_list datFiles;
  NavDataCache::DatFilesGroupInfo datFilesInfo = getDatFilesInfo(type);
  const PathList datPaths = datFilesInfo.paths;
  std::size_t bytesReadSoFar = 0;

  st.stamp();
  for (PathList::const_iterator it = datPaths.begin();
       it != datPaths.end(); it++) {
    string path = it->realpath().utf8Str();
    datFiles.push_back(path);
    SG_LOG(SG_GENERAL, SG_INFO,
           "Loading " + typeStr + ".dat file: '" << path << "'");
    loader(*it, bytesReadSoFar, datFilesInfo.totalSize);
    bytesReadSoFar += it->sizeInBytes();
    stampCacheFile(*it); // this uses the realpath() of the file
  }

  // Store the list of .dat files we have loaded
  writeOrderedStringListProperty(typeStr + ".dat files", datFiles,
                                 SGPath::pathListSep);
  SG_LOG(SG_NAVCACHE, SG_INFO,
         typeStr + ".dat files load took: " <<
         st.elapsedMSec());
}

void NavDataCache::doRebuild()
{
  rebuildInProgress = true;

  try {
    d->close(); // completely close the sqlite object
    d->path.remove(); // remove the file on disk
    d->init(); // start again from scratch

    // initialise the root octree node
    d->runSQL("INSERT INTO octree (rowid, children) VALUES (1, 0)");

    SGTimeStamp st;
    {
        Transaction txn(this);
        APTLoader aptLoader;
        FixesLoader fixesLoader;
        NavLoader navLoader;

        using namespace std::placeholders;  // for _1, _2, _3...

        loadDatFiles(DATFILETYPE_APT,
                     std::bind(&APTLoader::readAptDatFile, &aptLoader, _1, _2, _3));

        st.stamp();
        setRebuildPhaseProgress(REBUILD_UNKNOWN);
        SG_LOG(SG_NAVCACHE, SG_DEBUG, "Processing airports");
        aptLoader.loadAirports(); // load airport data into the NavCache
        SG_LOG(SG_NAVCACHE, SG_INFO,
               "processing airports took:" <<
               st.elapsedMSec());

        setRebuildPhaseProgress(REBUILD_UNKNOWN);
        metarDataLoad(d->metarDatPath);
        stampCacheFile(d->metarDatPath);

        loadDatFiles(DATFILETYPE_FIX,
                     std::bind(&FixesLoader::loadFixes, &fixesLoader, _1, _2, _3));
        loadDatFiles(DATFILETYPE_NAV,
                     std::bind(&NavLoader::loadNav, &navLoader, _1, _2, _3));

        setRebuildPhaseProgress(REBUILD_UNKNOWN);
        st.stamp();
        txn.commit();
        SG_LOG(SG_NAVCACHE, SG_INFO, "stage 1 commit took:" << st.elapsedMSec());
    }

#ifdef SG_WINDOWS
      SG_LOG(SG_NAVCACHE, SG_MANDATORY_INFO, "SKIPPING POI load on Windows");
#else
      {
          Transaction txn(this);

          st.stamp();
          poiDBInit(d->poiDatPath);
          stampCacheFile(d->poiDatPath);
          SG_LOG(SG_NAVCACHE, SG_INFO, "poi.dat load took:" << st.elapsedMSec());

          setRebuildPhaseProgress(REBUILD_UNKNOWN);
          st.stamp();
          txn.commit();
          SG_LOG(SG_NAVCACHE, SG_INFO, "POI commit took:" << st.elapsedMSec());
      }
#endif

      {
          Transaction txn(this);
          NavLoader navLoader;
          navLoader.loadCarrierNav(d->carrierDatPath);
          stampCacheFile(d->carrierDatPath);

          st.stamp();
          Airway::loadAWYDat(d->airwayDatPath);
          stampCacheFile(d->airwayDatPath);
          SG_LOG(SG_NAVCACHE, SG_INFO, "awy.dat load took:" << st.elapsedMSec());

          d->flushDeferredOctreeUpdates();

          string sceneryPaths = SGPath::join(globals->get_fg_scenery(), ";");
          writeStringProperty("scenery_paths", sceneryPaths);

          st.stamp();
          txn.commit();
          SG_LOG(SG_NAVCACHE, SG_INFO, "final commit took:" << st.elapsedMSec());

      }

  } catch (sg_exception& e) {
    SG_LOG(SG_NAVCACHE, SG_ALERT, "caught exception rebuilding navCache:" << e.what());
  }

  rebuildInProgress = false;
}

int NavDataCache::readIntProperty(const string& key)
{
  sqlite_bind_stdstring(d->readPropertyQuery, 1, key);
  int result = 0;

  if (d->execSelect(d->readPropertyQuery)) {
    result = sqlite3_column_int(d->readPropertyQuery, 0);
  } else {
    SG_LOG(SG_NAVCACHE, SG_WARN, "readIntProperty: unknown:" << key);
  }

  d->reset(d->readPropertyQuery);
  return result;
}

double NavDataCache::readDoubleProperty(const string& key)
{
  sqlite_bind_stdstring(d->readPropertyQuery, 1, key);
  double result = 0.0;
  if (d->execSelect(d->readPropertyQuery)) {
    result = sqlite3_column_double(d->readPropertyQuery, 0);
  } else {
    SG_LOG(SG_NAVCACHE, SG_WARN, "readDoubleProperty: unknown:" << key);
  }

  d->reset(d->readPropertyQuery);
  return result;
}

string NavDataCache::readStringProperty(const string& key)
{
  sqlite_bind_stdstring(d->readPropertyQuery, 1, key);
  string result;
  if (d->execSelect(d->readPropertyQuery)) {
    result = (char*) sqlite3_column_text(d->readPropertyQuery, 0);
  } else {
    SG_LOG(SG_NAVCACHE, SG_WARN, "readStringProperty: unknown:" << key);
  }

  d->reset(d->readPropertyQuery);
  return result;
}

void NavDataCache::writeIntProperty(const string& key, int value)
{
  d->writeIntProperty(key, value);
}

void NavDataCache::writeStringProperty(const string& key, const string& value)
{
  if (!isReadOnly()) {
    sqlite_bind_stdstring(d->clearProperty, 1, key);
    d->execUpdate(d->clearProperty);

    sqlite_bind_stdstring(d->writePropertyQuery, 1, key);
    sqlite_bind_stdstring(d->writePropertyQuery, 2, value);
    d->execUpdate(d->writePropertyQuery);
  }
}

void NavDataCache::writeDoubleProperty(const string& key, const double& value)
{
  if (!isReadOnly()) {
    sqlite_bind_stdstring(d->clearProperty, 1, key);
    d->execUpdate(d->clearProperty);

    sqlite_bind_stdstring(d->writePropertyQuery, 1, key);
    sqlite3_bind_double(d->writePropertyQuery, 2, value);
    d->execUpdate(d->writePropertyQuery);
  }
}

string_list NavDataCache::readStringListProperty(const string& key)
{
  sqlite_bind_stdstring(d->readPropertyQuery, 1, key);
  string_list result;
  while (d->stepSelect(d->readPropertyQuery)) {
    result.push_back((char*) sqlite3_column_text(d->readPropertyQuery, 0));
  }
  d->reset(d->readPropertyQuery);

  return result;
}

void NavDataCache::writeStringListProperty(const string& key, const string_list& values)
{
  if (!isReadOnly()) {
    sqlite_bind_stdstring(d->clearProperty, 1, key);
    d->execUpdate(d->clearProperty);

    for (string value : values) {
      sqlite_bind_stdstring(d->writePropertyMulti, 1, key);
      sqlite_bind_stdstring(d->writePropertyMulti, 2, value);
      d->execInsert(d->writePropertyMulti);
    }
  }
}

string_list NavDataCache::readOrderedStringListProperty(const string& key,
                                                        const char* separator)
{
   string_list l = simgear::strutils::split(readStringProperty(key), separator);
   assert(!l.empty());
   // See comment in writeOrderedStringListProperty()
   l.pop_back();

   return l;
}

void NavDataCache::writeOrderedStringListProperty(const string& key,
                                                  const string_list& values,
                                                  const char* separator)
{
  string s;

  // If values == {a, b, c}, we'll write this string to the property:
  //
  //   a + separator + b + separator + c + separator
  //
  // When this is split around 'separator' after reading back the property
  // value, we'll obtain an extraneous empty element after 'c' that should be
  // discarded. This last separator allows correct write-read round-trip when
  // 'values' is empty.
  if (!values.empty()) {
    s = simgear::strutils::join(values, separator) + string(separator);
  }

  writeStringProperty(key, s);
}

bool NavDataCache::isCachedFileModified(const SGPath& path) const
{
  return d->isCachedFileModified(path, false);
}

void NavDataCache::stampCacheFile(const SGPath& path)
{
    if (!isReadOnly()){
        sqlite_bind_temp_stdstring(d->stampFileCache, 1, path.realpath().utf8Str());
        sqlite3_bind_int64(d->stampFileCache, 2, path.modTime());
        d->execInsert(d->stampFileCache);
    }
}

void NavDataCache::beginTransaction()
{
  if (d->transactionLevel == 0) {
    d->transactionAborted = false;
    d->stepSelect(d->beginTransactionStmt);
    sqlite3_reset(d->beginTransactionStmt);
  }

  ++d->transactionLevel;
}

void NavDataCache::commitTransaction()
{
  assert(d->transactionLevel > 0);
  if (--d->transactionLevel == 0) {
    // if a nested transaction aborted, we might end up here, but must
    // still abort the entire transaction. That's bad, but safer than
    // committing.
    sqlite3_stmt_ptr q = d->transactionAborted ? d->rollbackTransactionStmt : d->commitTransactionStmt;

    int retries = 0;
    int result;
    while (retries < MAX_RETRIES) {
      result = sqlite3_step(q);
      if (result == SQLITE_DONE) {
        break;
      }

      // see http://www.sqlite.org/c3ref/get_autocommit.html for a hint
      // what's going on here: autocommit in inactive inside BEGIN, so if
      // it's active, the DB was rolled-back
      if (sqlite3_get_autocommit(d->db)) {
        SG_LOG(SG_NAVCACHE, SG_ALERT, "commit: was rolled back!" << retries);
        flightgear::sentryReportException("DB commit was rolled back");
        d->transactionAborted = true;
        break;
      }

      if (result != SQLITE_BUSY) {
        break;
      }

      SGTimeStamp::sleepForMSec(++retries * 100);
      SG_LOG(SG_NAVCACHE, SG_ALERT, "NavCache contention on commit, will retry:" << retries);
    } // of retry loop for DB busy

    string errMsg;
    if (result != SQLITE_DONE) {
      errMsg = sqlite3_errmsg(d->db);
      flightgear::sentryReportException("DB error:" + errMsg + " running " + std::string{sqlite3_sql(q)});
      SG_LOG(SG_NAVCACHE, SG_ALERT, "Sqlite error:" << errMsg << " for  " << result
             << " while running:\n\t" << sqlite3_sql(q));
    }

    sqlite3_reset(q);
  }
}

void NavDataCache::abortTransaction()
{
  SG_LOG(SG_NAVCACHE, SG_WARN, "NavCache: aborting transaction");
  flightgear::sentryReportException("DB aborting transactino");

  assert(d->transactionLevel > 0);
  if (--d->transactionLevel == 0) {
    d->stepSelect(d->rollbackTransactionStmt);
    sqlite3_reset(d->rollbackTransactionStmt);
  }

  d->transactionAborted = true;
}

void NavDataCache::clearDynamicPositioneds()
{
    std::for_each(d->cache.begin(), d->cache.end(), [](PositionedCache::value_type& v) {
        if (v.second->type() == FGPositioned::MOBILE_TACAN) {
            auto mobile = fgpositioned_cast<FGMobileNavRecord>(v.second);
            mobile->clearVehicle();
        }
    });
}

FGPositionedRef NavDataCache::loadById(PositionedID rowid)
{
  if (rowid < 1) {
    return NULL;
  }
  if (!d) return NULL;
  PositionedCache::iterator it = d->cache.find(rowid);
  if (it != d->cache.end()) {
    d->cacheHits++;
    return it->second; // cache it
  }

  sqlite3_int64 aptId;
  FGPositionedRef pos = d->loadById(rowid, aptId);
  if (rebuildInProgress) {
    // Do not cache and apply ILS adjustment while rebuilding the cache.
    // The adjustment process requires all ILS navaids to be present,
    // which is not true during the cache rebuild.
    return pos;
  }
  d->cache.insert(it, PositionedCache::value_type(rowid, pos));
  d->cacheMisses++;

  // when we loaded an ILS, we must apply per-airport changes
  if ((pos->type() == FGPositioned::ILS) && (aptId > 0)) {
    FGAirport* apt = FGPositioned::loadById<FGAirport>(aptId);
    apt->validateILSData();
  }

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

  sqlite3_bind_int64(d->insertAirport, 1, rowId);
  d->execInsert(d->insertAirport);

  return rowId;
}

void NavDataCache::updatePosition(PositionedID item, const SGGeod &pos)
{
  if (d->cache.find(item) != d->cache.end()) {
    SG_LOG(SG_NAVCACHE, SG_DEBUG, "updating position of an item in the cache");
    d->cache[item]->modifyPosition(pos);
  }

  SGVec3d cartPos(SGVec3d::fromGeod(pos));

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
  bool spatialIndex = ( ty == FGPositioned::RUNWAY || ty == FGPositioned::HELIPAD);

  sqlite3_int64 rowId = d->insertPositioned(ty, cleanRunwayNo(ident), "", pos, apt,
                                            spatialIndex);
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
  sqlite3_bind_int64(d->setRunwayReciprocal, 1, runway);
  sqlite3_bind_int64(d->setRunwayReciprocal, 2, recip);
  d->execUpdate(d->setRunwayReciprocal);

// and the opposite direction too!
  sqlite3_bind_int64(d->setRunwayReciprocal, 2, runway);
  sqlite3_bind_int64(d->setRunwayReciprocal, 1, recip);
  d->execUpdate(d->setRunwayReciprocal);
}

void NavDataCache::setRunwayILS(PositionedID runway, PositionedID ils)
{
  sqlite3_bind_int64(d->setRunwayILS, 1, runway);
  sqlite3_bind_int64(d->setRunwayILS, 2, ils);
  d->execUpdate(d->setRunwayILS);

  // and the in-memory one
  if (d->cache.find(runway) != d->cache.end()) {
    FGRunway* instance = (FGRunway*) d->cache[runway].ptr();
    instance->setILS(ils);
  }
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
  sqlite3_bind_int64(d->insertNavaid, 1, rowId);
  sqlite3_bind_int(d->insertNavaid, 2, freq);
  sqlite3_bind_int(d->insertNavaid, 3, range);
  sqlite3_bind_double(d->insertNavaid, 4, multiuse);
  sqlite3_bind_int64(d->insertNavaid, 5, runway);
  sqlite3_bind_int64(d->insertNavaid, 6, 0);
  return d->execInsert(d->insertNavaid);
}

void NavDataCache::setNavaidColocated(PositionedID navaid, PositionedID colocatedDME)
{
  // Update DB entries...
  sqlite3_bind_int64(d->setNavaidColocated, 1, navaid);
  sqlite3_bind_int64(d->setNavaidColocated, 2, colocatedDME);
  d->execUpdate(d->setNavaidColocated);

  // ...and the in-memory copy of the navrecord
  if (d->cache.find(navaid) != d->cache.end()) {
    FGNavRecord* rec = (FGNavRecord*) d->cache[navaid].get();
    rec->setColocatedDME(colocatedDME);
  }
}

PositionedID NavDataCache::insertCommStation(FGPositioned::Type ty,
                                             const string& name, const SGGeod& pos, int freq, int range,
                                             PositionedID apt)
{
  sqlite3_int64 rowId = d->insertPositioned(ty, "", name, pos, apt, true);
  sqlite3_bind_int64(d->insertCommStation, 1, rowId);
  sqlite3_bind_int(d->insertCommStation, 2, freq);
  sqlite3_bind_int(d->insertCommStation, 3, range);
  return d->execInsert(d->insertCommStation);
}

PositionedID NavDataCache::insertFix(const std::string& ident, const SGGeod& aPos)
{
  return d->insertPositioned(FGPositioned::FIX, ident, string(), aPos, 0, true);
}

PositionedID NavDataCache::createPOI(FGPositioned::Type ty, const std::string& ident, const SGGeod& aPos)
{
  return d->insertPositioned(ty, ident, string(), aPos, 0,
                             true /* spatial index */);
}

bool NavDataCache::removePOI(FGPositioned::Type ty, const std::string& aIdent)
{
  d->removePositionedWithIdent(ty, aIdent);
  // should remove from the live cache too?

    return true;
}

void NavDataCache::setAirportMetar(const string& icao, bool hasMetar)
{
  sqlite_bind_stdstring(d->setAirportMetar, 1, icao);
  sqlite3_bind_int(d->setAirportMetar, 2, hasMetar);
  d->execUpdate(d->setAirportMetar);
}

//------------------------------------------------------------------------------
FGPositionedList NavDataCache::findAllWithIdent( const string& s,
                                                 FGPositioned::Filter* filter,
                                                 bool exact )
{
  return d->findAllByString(s, "ident", filter, exact);
}

//------------------------------------------------------------------------------
FGPositionedList NavDataCache::findAllWithName( const string& s,
                                                FGPositioned::Filter* filter,
                                                bool exact )
{
  return d->findAllByString(s, "name", filter, exact);
}

//------------------------------------------------------------------------------
FGPositionedRef NavDataCache::findClosestWithIdent( const string& aIdent,
                                                    const SGGeod& aPos,
                                                    FGPositioned::Filter* aFilter )
{
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

  FGPositionedRef result;

  while (d->stepSelect(d->findClosestWithIdent)) {
    FGPositionedRef pos = loadById(sqlite3_column_int64(d->findClosestWithIdent, 0));
    if (aFilter && !aFilter->pass(pos)) {
      continue;
    }

    result = pos;
    break;
  }

  d->reset(d->findClosestWithIdent);
  return result;
}


int NavDataCache::getOctreeBranchChildren(int64_t octreeNodeId)
{
    sqlite3_bind_int64(d->getOctreeChildren, 1, octreeNodeId);
    if (!d->execSelect(d->getOctreeChildren)) {
        // this can occur when in read-only mode: we don't add
        // new Octree nodes to the real DB (only in memory),
        // but will still call this code speculatively.
        // see the early-return just below in defineOctreeNode
        return 0;
    }   

  int children = sqlite3_column_int(d->getOctreeChildren, 0);
  d->reset(d->getOctreeChildren);
  return children;
}

void NavDataCache::defineOctreeNode(Octree::Branch* pr, Octree::Node* nd)
{
  if (isReadOnly()) {
    return;
  }
  
  sqlite3_bind_int64(d->insertOctree, 1, nd->guid());
  d->execInsert(d->insertOctree);

#ifdef LAZY_OCTREE_UPDATES
  d->deferredOctreeUpdates.insert(pr);
#else
  // lowest three bits of node ID are 0..7 index of the child in the parent
  int childIndex = nd->guid() & 0x07;

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
  sqlite3_bind_int64(d->getOctreeLeafChildren, 1, octreeNodeId);

  TypedPositionedVec r;
  while (d->stepSelect(d->getOctreeLeafChildren)) {
    FGPositioned::Type ty = static_cast<FGPositioned::Type>
      (sqlite3_column_int(d->getOctreeLeafChildren, 1));
    r.push_back(std::make_pair(ty,
                sqlite3_column_int64(d->getOctreeLeafChildren, 0)));
  }

  d->reset(d->getOctreeLeafChildren);
  return r;
}


/**
 * A special purpose helper (used by FGAirport::searchNamesAndIdents) to
 * implement the AirportList dialog. It's unfortunate that it needs to reside
 * here, but for now it's least ugly solution.
 */
char** NavDataCache::searchAirportNamesAndIdents(const std::string& searchInput)
{
  sqlite3_stmt_ptr stmt;
  unsigned int numMatches = 0, numAllocated = 16;
  string heliport("HELIPORT");
  bool heli_p = searchInput.substr(0, heliport.length()) == heliport;
  auto pos = searchInput.find(":");
  string aFilter((pos != string::npos) ? searchInput.substr(pos+1) : searchInput);
  string searchTerm("%" + aFilter + "%");

  if (aFilter.empty() && !heli_p) {
    stmt = d->getAllAirports;
    numAllocated = 4096; // start much larger for all airports
  } else {
    stmt = d->searchAirports;
    sqlite_bind_stdstring(stmt, 1, searchTerm);
    if (heli_p) {
        sqlite3_bind_int(stmt, 2, FGPositioned::HELIPORT);
        sqlite3_bind_int(stmt, 3, FGPositioned::HELIPORT);
    }
    else {
        sqlite3_bind_int(stmt, 2, FGPositioned::AIRPORT);
        sqlite3_bind_int(stmt, 3, FGPositioned::SEAPORT);
    }
  }

  char** result = (char**) malloc(sizeof(char*) * numAllocated);
  while (d->stepSelect(stmt)) {
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
    int nameLength = sqlite3_column_bytes(stmt, 1);
    int icaoLength = sqlite3_column_bytes(stmt, 0);
    char* entry = (char*) malloc(7 + nameLength + icaoLength);
    char* dst = entry;
    *dst++ = ' ';
    memcpy(dst, sqlite3_column_text(stmt, 1), nameLength);
    dst += nameLength;
    *dst++ = ' ';
    *dst++ = ' ';
    *dst++ = ' ';
    *dst++ = '(';
    memcpy(dst, sqlite3_column_text(stmt, 0), icaoLength);
    dst += icaoLength;
    *dst++ = ')';
    *dst++ = 0;

    result[numMatches++] = entry;
  }

  result[numMatches] = NULL; // end of list marker
  d->reset(stmt);
  return result;
}

FGPositionedRef
NavDataCache::findCommByFreq(int freqKhz, const SGGeod& aPos, FGPositioned::Filter* aFilter)
{
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
  FGPositionedRef result;

  while (d->execSelect(d->findCommByFreq)) {
    FGPositionedRef p = loadById(sqlite3_column_int64(d->findCommByFreq, 0));
    if (aFilter && !aFilter->pass(p)) {
      continue;
    }

    result = p;
    break;
  }

  d->reset(d->findCommByFreq);
  return result;
}

PositionedIDVec
NavDataCache::findNavaidsByFreq(int freqKhz, const SGGeod& aPos, FGPositioned::Filter* aFilter)
{
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

  sqlite3_bind_int64(d->getAirportItems, 1, apt);
  sqlite3_bind_int(d->getAirportItems, 2, ty);
  sqlite3_bind_int(d->getAirportItems, 3, maxTy);

  return d->selectIds(d->getAirportItems);
}

PositionedID
NavDataCache::airportItemWithIdent(PositionedID apt, FGPositioned::Type ty,
                                   const std::string& ident)
{
  sqlite3_bind_int64(d->getAirportItemByIdent, 1, apt);
  sqlite_bind_stdstring(d->getAirportItemByIdent, 2, ident);
  sqlite3_bind_int(d->getAirportItemByIdent, 3, ty);
  PositionedID result = 0;

  if (d->execSelect(d->getAirportItemByIdent)) {
    result = sqlite3_column_int64(d->getAirportItemByIdent, 0);
  }

  d->reset(d->getAirportItemByIdent);
  return result;
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

  AirportRunwayPair result;
  sqlite_bind_stdstring(d->findAirportRunway, 1, parts[0]);
  const auto cleanedRunway = cleanRunwayNo(parts[1]);
  sqlite_bind_stdstring(d->findAirportRunway, 2, cleanedRunway);

  if (d->execSelect(d->findAirportRunway)) {
    result = AirportRunwayPair(sqlite3_column_int64(d->findAirportRunway, 0),
                      sqlite3_column_int64(d->findAirportRunway, 1));

  } else {
    SG_LOG(SG_NAVCACHE, SG_WARN, "findAirportRunway: unknown airport/runway:" << aName);
  }

  d->reset(d->findAirportRunway);
  return result;
}

PositionedID
NavDataCache::findILS(PositionedID airport, const string& aRunway, const string& navIdent)
{
  string runway(cleanRunwayNo(aRunway));

  sqlite_bind_stdstring(d->findILS, 1, navIdent);
  sqlite3_bind_int64(d->findILS, 2, airport);
  sqlite_bind_stdstring(d->findILS, 3, runway);
  PositionedID result = 0;
  if (d->execSelect(d->findILS)) {
    result = sqlite3_column_int64(d->findILS, 0);
  }

  d->reset(d->findILS);
  return result;
}

int NavDataCache::findAirway(int network, const string& aName, bool create)
{
    assert((network == 1) || (network == 2));

    sqlite3_bind_int(d->findAirwayNet, 1, network);
    sqlite_bind_stdstring(d->findAirwayNet, 2, aName);

    int airway = 0;
    if (d->execSelect(d->findAirwayNet)) {
        // already exists
        airway = sqlite3_column_int(d->findAirwayNet, 0);
  } else if (create) {
    d->reset(d->insertAirway);
    sqlite_bind_stdstring(d->insertAirway, 1, aName);
    sqlite3_bind_int(d->insertAirway, 2, network);
    airway = d->execInsert(d->insertAirway);
  } else {
      // doesn't exist but don't create
  }

  d->reset(d->findAirwayNet);
  return airway;
}

int NavDataCache::findAirway(const string& aName)
{
  sqlite_bind_stdstring(d->findAirway, 1, aName);

  int airway = 0;
  if (d->execSelect(d->findAirway)) {
    // already exists
    airway = sqlite3_column_int(d->findAirway, 0);
  }

  d->reset(d->findAirway);
  return airway;
}


void NavDataCache::insertEdge(int network, int airwayID, PositionedID from, PositionedID to)
{
    sqlite3_bind_int(d->insertAirwayEdge, 1, network);
    sqlite3_bind_int(d->insertAirwayEdge, 2, airwayID);
    sqlite3_bind_int64(d->insertAirwayEdge, 3, from);
    sqlite3_bind_int64(d->insertAirwayEdge, 4, to);
    d->execInsert(d->insertAirwayEdge);
}

bool NavDataCache::isInAirwayNetwork(int network, PositionedID pos)
{
  sqlite3_bind_int(d->isPosInAirway, 1, network);
  sqlite3_bind_int64(d->isPosInAirway, 2, pos);
  bool ok = d->execSelect(d->isPosInAirway);
  d->reset(d->isPosInAirway);

  return ok;
}

AirwayEdgeVec NavDataCache::airwayEdgesFrom(int network, PositionedID pos)
{
  sqlite3_bind_int(d->airwayEdgesFrom, 1, network);
  sqlite3_bind_int64(d->airwayEdgesFrom, 2, pos);

  AirwayEdgeVec result;
  while (d->stepSelect(d->airwayEdgesFrom)) {
    result.push_back(AirwayEdge(
                     sqlite3_column_int(d->airwayEdgesFrom, 0),
                     sqlite3_column_int64(d->airwayEdgesFrom, 1)
                     ));
  }

  d->reset(d->airwayEdgesFrom);

// find bidirectional / backwsards edges
    // at present all edges are bidirectional
    sqlite3_bind_int(d->airwayEdgesTo, 1, network);
    sqlite3_bind_int64(d->airwayEdgesTo, 2, pos);

    while (d->stepSelect(d->airwayEdgesTo)) {
        result.push_back(AirwayEdge(
                                    sqlite3_column_int(d->airwayEdgesTo, 0),
                                    sqlite3_column_int64(d->airwayEdgesTo, 1)
                                    ));
    }

    d->reset(d->airwayEdgesTo);

  return result;
}

AirwayRef NavDataCache::loadAirway(int airwayID)
{
    sqlite3_bind_int(d->loadAirway, 1, airwayID);
    bool ok = d->execSelect(d->loadAirway);
    AirwayRef result;
    if (ok) {
        string ident = (char*) sqlite3_column_text(d->loadAirway, 0);
        Airway::Level network = static_cast<Airway::Level>(sqlite3_column_int(d->loadAirway, 1));
        result = new Airway(ident, network, airwayID, 0, 0);
    }
    d->reset(d->loadAirway);
    return result;
}

PositionedIDVec NavDataCache::airwayWaypts(int id)
{
    d->reset(d->airwayEdges);
    sqlite3_bind_int(d->airwayEdges, 1, id);

    typedef std::pair<PositionedID, PositionedID> Edge;
    typedef std::deque<Edge> EdgeVec;
    typedef std::deque<PositionedID> PositionedIDDeque;

// build up the EdgeVec, order is arbitrary
    EdgeVec rawEdges;
    while (d->stepSelect(d->airwayEdges)) {
        rawEdges.push_back(Edge(sqlite3_column_int64(d->airwayEdges, 0),
                                sqlite3_column_int64(d->airwayEdges, 1)
                                ));
    }

    d->reset(d->airwayEdges);
    if (rawEdges.empty()) {
        return {};
    }

// linearize
    PositionedIDVec result;

    while (!rawEdges.empty()) {
        bool didAddEdge = false;
        std::set<PositionedID> seen;
        EdgeVec nextDeque;
        PositionedIDDeque linearAirway;
        PositionedID firstId = rawEdges.front().first,
            lastId = rawEdges.front().second;

        // first edge is trivial
        linearAirway.push_back(firstId);
        linearAirway.push_back(lastId);
        seen.insert(firstId);
        seen.insert(lastId);
        rawEdges.pop_front();

        while (!rawEdges.empty()) {
            Edge e = rawEdges.front();
            rawEdges.pop_front();

            bool seenFirst = (seen.find(e.first) != seen.end());
            bool seenSecond = (seen.find(e.second) != seen.end());

            // duplicated segment, should be impossible
            assert(!(seenFirst && seenSecond));

            if (!seenFirst && !seenSecond) {
                // push back to try later on
                nextDeque.push_back(e);
                if (rawEdges.empty()) {
                    rawEdges = nextDeque;
                    nextDeque.clear();

                    if (!didAddEdge) {
                        // we have a disjoint, need to start a new section
                        // break out of the inner while loop so the outer
                        // one can process and restart
                        break;
                    }
                    didAddEdge = false;
                }

                continue;
            }

            // we have an exterior edge, grow our current linear piece
            if (seenFirst && (e.first == firstId)) {
                linearAirway.push_front(e.second);
                firstId = e.second;
                seen.insert(e.second);
            } else if (seenSecond && (e.second == firstId)) {
                linearAirway.push_front(e.first);
                firstId = e.first;
                seen.insert(e.first);
            } else if (seenFirst && (e.first == lastId)) {
                linearAirway.push_back(e.second);
                lastId = e.second;
                seen.insert(e.second);
            } else if (seenSecond && (e.second == lastId)) {
                linearAirway.push_back(e.first);
                lastId = e.first;
                seen.insert(e.first);
            }
            didAddEdge = true;

            if (rawEdges.empty()) {
                rawEdges = nextDeque;
                nextDeque.clear();
            }
        }

        if (!result.empty())
            result.push_back(0);
        result.insert(result.end(), linearAirway.begin(), linearAirway.end());
    } // outer loop

    SG_LOG(SG_AUTOPILOT, SG_WARN, "Airway:" << id);
    for (unsigned int i=0; i<result.size(); ++i) {
        if (result.at(i) == 0) {
            SG_LOG(SG_AUTOPILOT, SG_WARN, i << " <break>");
        } else {
            SG_LOG(SG_AUTOPILOT, SG_WARN, i << " " << loadById(result.at(i))->ident());

        }
    }

    return result;
}

PositionedID NavDataCache::findNavaidForRunway(PositionedID runway, FGPositioned::Type ty)
{
  sqlite3_bind_int64(d->findNavaidForRunway, 1, runway);
  sqlite3_bind_int(d->findNavaidForRunway, 2, ty);

  PositionedID result = 0;
  if (d->execSelect(d->findNavaidForRunway)) {
    result = sqlite3_column_int64(d->findNavaidForRunway, 0);
  }

  d->reset(d->findNavaidForRunway);
  return result;
}

bool NavDataCache::isReadOnly() const
{
    return d->readOnly;
}

SGPath NavDataCache::path() const
{
    return d->path;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Transaction RAII object

NavDataCache::Transaction::Transaction(NavDataCache* cache) :
    _instance(cache),
    _committed(false)
{
    assert(cache);
    if (!cache->isReadOnly()) {
      _instance->beginTransaction();
    }
}

NavDataCache::Transaction::~Transaction()
{
    if (_instance->isReadOnly()) {
        return;
    }

    if (!_committed) {
        SG_LOG(SG_NAVCACHE, SG_INFO, "aborting cache transaction!");
        _instance->abortTransaction();
    }
}

void NavDataCache::Transaction::commit()
{
    if (_instance->isReadOnly()) {
        return;
    }
        
    assert(!_committed);
    _committed = true;
    _instance->commitTransaction();
}

/////////////////////////////////////////////////////////////////////////////

class NavDataCache::ThreadedGUISearch::ThreadedGUISearchPrivate : public SGThread
{
public:
    ThreadedGUISearchPrivate() :
        db(NULL),
        isComplete(false),
        quit(false)
    {}

    virtual void run()
    {
        while (!quit) {
            int err = sqlite3_step(query);
            if (err == SQLITE_DONE) {
                break;
            } else if (err == SQLITE_ROW) {
                PositionedID r = sqlite3_column_int64(query, 0);
                std::lock_guard<std::mutex> g(lock);
                results.push_back(r);
            } else if (err == SQLITE_BUSY) {
                // sleep a tiny amount
                SGTimeStamp::sleepForMSec(1);
            } else {
                std::string errMsg = sqlite3_errmsg(db);
                SG_LOG(SG_NAVCACHE, SG_ALERT, "Sqlite error:" << errMsg << " running threaded search query");
            }
        }

        std::lock_guard<std::mutex> g(lock);
        isComplete = true;
    }

    std::mutex lock;
    sqlite3* db;
    sqlite3_stmt_ptr query;
    PositionedIDVec results;
    bool isComplete;
    bool quit;
};

NavDataCache::ThreadedGUISearch::ThreadedGUISearch(const std::string& term, bool onlyAirports) :
    d(new ThreadedGUISearchPrivate)
{
    SGPath p = NavDataCache::instance()->path();
    int openFlags = SQLITE_OPEN_READONLY;
    std::string pathUtf8 = p.utf8Str();
    sqlite3_open_v2(pathUtf8.c_str(), &d->db, openFlags, NULL);

    std::string sql;
    if (onlyAirports) {
        sql = "SELECT rowid FROM positioned WHERE name LIKE '%" + term
                + "%' AND (type >= 1 AND type <= 3)";
    } else {
        sql = "SELECT rowid FROM positioned WHERE name LIKE '%" + term
                + "%' AND ((type >= 1 AND type <= 3) OR ((type >= 9 AND type <= 11))) ";
    }
    sqlite3_prepare_v2(d->db, sql.c_str(), sql.length(), &d->query, NULL);

    d->start();
}

NavDataCache::ThreadedGUISearch::~ThreadedGUISearch()
{
    {
        std::lock_guard<std::mutex> g(d->lock);
        d->quit = true;
    }

    d->join();
    sqlite3_finalize(d->query);
    sqlite3_close_v2(d->db);
}

PositionedIDVec NavDataCache::ThreadedGUISearch::results() const
{
    PositionedIDVec r;
    {
        std::lock_guard<std::mutex> g(d->lock);
        r = std::move(d->results);
    }
    return r;
}

bool NavDataCache::ThreadedGUISearch::isComplete() const
{
    std::lock_guard<std::mutex> g(d->lock);
    return d->isComplete;
}

} // of namespace flightgear
