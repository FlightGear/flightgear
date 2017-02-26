// navdb.cxx -- top level navaids management routines
//
// Written by Curtis Olson, started May 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string>
#include <vector>
#include <istream>
#include <cmath>
#include <cstddef>              // std::size_t
#include <cerrno>
#include <limits>
#include <map>

#include "navdb.hxx"

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGGeod.hxx>
#include <simgear/math/SGMathFwd.hxx>
#include <simgear/math/SGVec3.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/sg_inlines.h>

#include "navlist.hxx"
#include <Main/globals.hxx>
#include <Navaids/markerbeacon.hxx>
#include <Airports/airport.hxx>
#include <Airports/runways.hxx>
#include <Airports/xmlloader.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>

using std::string;
using std::vector;

// Duplicate navaids with the same ident will be removed if the disance
// between them is less than this.
static const double DUPLICATE_DETECTION_RADIUS_NM = 10;


static void throwExceptionIfStreamError(const std::istream& inputStream,
                                        const SGPath& path)
{
  if (inputStream.bad()) {
    const std::string errMsg = simgear::strutils::error_string(errno);

    SG_LOG(SG_NAVAID, SG_ALERT,
           "Error while reading '" << path.utf8Str() << "': " << errMsg);
    throw sg_io_exception("Error reading file (" + errMsg + ")",
                          sg_location(path));
  }
}

static FGPositioned::Type
mapRobinTypeToFGPType(int aTy)
{
  switch (aTy) {
  case 2: return FGPositioned::NDB;
  case 3: return FGPositioned::VOR;
  case 4: return FGPositioned::ILS;
  case 5: return FGPositioned::LOC;
  case 6: return FGPositioned::GS;
  case 7: return FGPositioned::OM;
  case 8: return FGPositioned::MM;
  case 9: return FGPositioned::IM;
  case 12:
  case 13: return FGPositioned::DME;
  default: return FGPositioned::INVALID;
  }
}

static bool autoAlignLocalizers = false;
static double autoAlignThreshold = 0.0;

/**
 * Given a runway, and proposed localizer data (ident, positioned and heading),
 * precisely align the localizer with the actual runway heading, providing the
 * difference between the localizer course and runway heading is less than a
 * threshold. (To allow for localizers such as Kai-Tak requiring a turn on final).
 *
 * The positioned and heading argument are modified if changes are made.
 */
void alignLocaliserWithRunway(FGRunway* rwy, const string& ident, SGGeod& pos, double& heading)
{
  assert(rwy);
  // find the distance from the threshold to the localizer
  double dist = SGGeodesy::distanceM(pos, rwy->threshold());
  
  // back project that distance along the runway center line
  SGGeod newPos = rwy->pointOnCenterline(dist);
  
  double hdg_diff = heading - rwy->headingDeg();
  SG_NORMALIZE_RANGE(hdg_diff, -180.0, 180.0);
  
  if ( fabs(hdg_diff) <= autoAlignThreshold ) {
    pos = SGGeod::fromGeodFt(newPos, pos.getElevationFt());
    heading = rwy->headingDeg();
  } else {
    SG_LOG(SG_NAVAID, SG_DEBUG, "localizer:" << ident << ", aligning with runway "
           << rwy->ident() << " exceeded heading threshold");
  }
}

static double defaultNavRange(const string& ident, FGPositioned::Type type)
{
  // Ranges are included with the latest data format, no need to
  // assign our own defaults, unless the range is not set for some
  // reason.
  SG_LOG(SG_NAVAID, SG_DEBUG, "navaid " << ident << " has no range set, using defaults");
  switch (type) {
    case FGPositioned::NDB:
    case FGPositioned::VOR:
      return FG_NAV_DEFAULT_RANGE;
      
    case FGPositioned::LOC:
    case FGPositioned::ILS:
    case FGPositioned::GS:
      return FG_LOC_DEFAULT_RANGE;
      
    case FGPositioned::DME:
      return FG_DME_DEFAULT_RANGE;

    case FGPositioned::TACAN:
    case FGPositioned::MOBILE_TACAN:
      return FG_TACAN_DEFAULT_RANGE;

    default:
      return FG_LOC_DEFAULT_RANGE;
  }
}


namespace flightgear
{

static bool isNearby(const SGGeod& pos1, const SGGeod& pos2) {
  double distNm = dist(SGVec3d::fromGeod(pos1),
                       SGVec3d::fromGeod(pos2)) * SG_METER_TO_NM;
  return distNm <= DUPLICATE_DETECTION_RADIUS_NM;
}

static bool canBeDuplicate(FGPositionedRef ref, FGPositioned::Type type,
                           const std::string& name,
                           const SGGeod& pos, int freq)
{
  if ((type >= FGPositioned::ILS) && (type <= FGPositioned::IM)) {
    NavDataCache* cache = NavDataCache::instance();
    AirportRunwayPair AR = cache->findAirportRunway(name);
    PositionedID navaidId = cache->findNavaidForRunway(AR.second, type);
    return navaidId != 0;
  } else if (type == FGPositioned::DME) {
    FGNavRecord* navRecord = dynamic_cast<FGNavRecord*>(ref.ptr());
    return navRecord->get_freq() == freq;
  } else {
    return true;
  }
}

// Parse a line from a file such as nav.dat or carrier_nav.dat. Load the
// corresponding data into the NavDataCache.
static PositionedID processNavLine(
  const string& line, const string& utf8Path, unsigned int lineNum,
  FGPositioned::Type type = FGPositioned::INVALID, unsigned long version = 810)
{
  NavDataCache* cache = NavDataCache::instance();
  int rowCode, elev_ft, freq, range;
  // 'multiuse': different meanings depending on the record's row code
  double lat, lon, multiuse;
  // Short identifier and longer name for a navaid (e.g., 'OLN' and
  // 'LFPO 02 OM')
  string ident, name, arpt_code;

  if (simgear::strutils::starts_with(line, "#")) {
    // carrier_nav.dat has a comment line using this syntax...
    return 0;
  }

  int num_splits;
  if (version < 1100) {
    // At most 9 fields (the ninth field may contain spaces)
    num_splits = 8;
  } else {
    // At most 11 fields (the eleventh field may contain spaces)
    num_splits = 10;
  }

  vector<string> fields(simgear::strutils::split(line, 0, num_splits));
  vector<string>::size_type nbFields = fields.size();
  static const string endOfData = "99"; // special code in the nav.dat spec

  if (nbFields == 0) {       // blank line
    return 0;
  } else if (nbFields == 1) {
    if (fields[0] != endOfData) {
      SG_LOG( SG_NAVAID, SG_WARN,
              utf8Path << ":" << lineNum << ": malformed line: only one "
              "field, but it is not '99'" );
    }

    return 0;
  } else if (nbFields < 9) {
    SG_LOG( SG_NAVAID, SG_WARN,
            utf8Path << ":"  << lineNum << ": invalid line "
            "(at least 9 fields are required)" );
    return 0;
  }

  // When their string argument can't be properly converted, std::stoi(),
  // std::stof() and std::stod() all raise an exception which is always a
  // subclass of std::logic_error.
  try {
    rowCode = std::stoi(fields[0]);
    lat = std::stod(fields[1]);
    lon = std::stod(fields[2]);
    elev_ft = std::stoi(fields[3]);
    // The input data is a floating point number, but we are going to feed it
    // to NavDataCache::insertNavaid(), which takes an int.
    freq = static_cast<int>(std::lround(std::stof(fields[4])));
    range = std::stoi(fields[5]);
    multiuse = std::stod(fields[6]);
    ident = fields[7];
    if (version >= 1100) {
      // Convert names to the format present in 810 version.

      // 1. fields[9] is ICAO region code, we skip over it.
      // 2. For NDB, VOR and DMEs not associated with ILS,
      //    fields[8] is always ENRT, we skip over this too, to match
      //    the naming with version 810.
      if ((rowCode == 2 || rowCode == 3 || rowCode == 12 || rowCode == 13)
          && fields[8] == "ENRT") {
        name = fields[10];
      } else {
        name = fields[8] + " " + fields[10];
      }
    } else {
      name = fields[8];
    }
    // Canonicalize name, removing whitespace from the beginning, the end
    // and extraneous spaces between tokens.
    name = simgear::strutils::simplify(name);
  } catch (const std::logic_error& exc) {
    // On my system using GNU libstdc++, exc.what() is limited to the function
    // name (e.g., 'stod')!
    SG_LOG( SG_NAVAID, SG_WARN,
            utf8Path << ":"  << lineNum << ": unable to parse (" <<
            exc.what() << "): '" <<
            simgear::strutils::stripTrailingNewlines(line) << "'" );
    return 0;
  }

  SGGeod pos(SGGeod::fromDegFt(lon, lat, static_cast<double>(elev_ft)));

  // The type can be forced by our caller, but normally we use the value
  // supplied in the .dat file.
  if (type == FGPositioned::INVALID) {
    type = mapRobinTypeToFGPType(rowCode);
    if (type == FGPositioned::INVALID) {
      static std::set<int> ignoredCodes;
      if (ignoredCodes.insert(rowCode).second) {
        SG_LOG(SG_NAVAID, SG_WARN,
               utf8Path << ":"  << lineNum << ": unrecognized row code "
               << rowCode << ", ignoring this line and all further lines "
               << "with the same code");
      }
      return 0;
    }
  }

  // Silently multiply ADF frequencies by 100 so that ADF
  // vs. NAV/LOC frequency lookups can use the same code.
  if (type == FGPositioned::NDB) {
    freq *= 100;
  }

  //
  // Deduplication rules:
  //
  // 1. Navaid files are loaded according to the order in $FG_SCENERY,
  //    followed by the default file in $FG_ROOT/Navaids.
  //
  // 2. Navaids from each of these files are considered one by one and added
  //    to the cache, except a navaid is *not* added if another one lies within
  //    DUPLICATE_DETECTION_RADIUS_NM and:
  //
  //     - it has the same name, type and ident, or
  //     - it has the same type and ident, and:
  //       * is either attached to the same runway (for navaid types LOC, ILS,
  //         GS, IM, MM, OM), or
  //       * has type FGPositioned::DME and the same frequency
  //         (this ensures that colocated DMEs and TACANs are *not* considered
  //          as duplicates, since they normally have different frequencies)
  //
  // For this logic to work reasonably, each set of nav.dat files in a given
  // scenery path must be self-contained regarding the colocated navaids --
  // if one of the colocated navaids is present, the other one must be too,
  // and the files must be sorted by row codes as mandated in XP-NAV1100 spec.
  //

  // First, eliminate nearby with the same name, type and ident.
  static std::multimap<std::tuple<FGPositioned::Type, std::string,
      std::string>, SGGeod> loadedNavs;  // Maps (type, ident, name) tuples
                                         // already loaded to their locations.
  auto loadedNavsKey = std::make_tuple(type, ident, name);
  auto matchingNavs = loadedNavs.equal_range(loadedNavsKey);
  for (auto it = matchingNavs.first; it != matchingNavs.second; ++it) {
    if (isNearby(pos, it->second)) {
      SG_LOG(SG_NAVAID, SG_INFO,
             utf8Path << ":"  << lineNum << ": skipping navaid '" <<
             name << "' (already defined nearby)");
      return 0;
    }
  }
  loadedNavs.emplace(loadedNavsKey, pos);

  // Then, eliminate nearby with the same type and ident.
  FGPositioned::TypeFilter dupTypeFilter(type);
  FGPositionedRef ref = FGPositioned::findClosestWithIdent(ident, pos,
                                                           &dupTypeFilter);
  if (ref.valid()) {
    if (isNearby(pos, ref->geod())
        && canBeDuplicate(ref, type, name, pos, freq)) {
      SG_LOG(SG_NAVAID, SG_INFO,
             utf8Path << ":"  << lineNum << ": skipping navaid '" <<
             name << "' (nearby suspected duplicate '" << ref->name() << "')");
      return 0;
    }
  }

  if ((type >= FGPositioned::OM) && (type <= FGPositioned::IM)) {
    AirportRunwayPair arp(cache->findAirportRunway(name));

    if (!arp.first || !arp.second) {
      SG_LOG(SG_NAVAID, SG_INFO,
             utf8Path << ":" << lineNum << ": couldn't find matching runway " <<
             "for marker '" << name << "', skipping.");
      return 0;
    }

    if (arp.second && (elev_ft <= 0)) {
      // snap to runway elevation
      FGPositionedRef runway = cache->loadById(arp.second);
      assert(runway);
      pos.setElevationFt(runway->geod().getElevationFt());
    }

    return cache->insertNavaid(type, string(), name, pos, 0, 0, 0,
                               arp.first, arp.second);
  }
  
  if (range < 1) {
    range = defaultNavRange(ident, type);
  }
  
  AirportRunwayPair arp;
  FGRunwayRef runway;
  PositionedID navaid_dme = 0;

  if (type == FGPositioned::DME) {
      // complication here: the database doesn't record TACAN sites at all,
      // we simply infer their existence from DMEs whose name includes 'VORTAC'
      // or 'TACAN' (since all TACAN stations include DME)
      // hence the cache never actually includes entries of type TACAN
    FGPositioned::TypeFilter f(FGPositioned::INVALID);
    if ( name.find("VOR-DME") != std::string::npos ) {
      f.addType(FGPositioned::VOR);
    } else if ( name.find("DME-ILS") != std::string::npos ) {
      f.addType(FGPositioned::ILS);
      f.addType(FGPositioned::LOC);
    } else if ( name.find("VORTAC") != std::string::npos ) {
      f.addType(FGPositioned::VOR);
    } else if ( name.find("NDB-DME") != std::string::npos ) {
      f.addType(FGPositioned::NDB);
    }

    if (f.maxType() > 0) {
      FGPositionedRef ref = FGPositioned::findClosestWithIdent(ident, pos, &f);
      if (ref.valid()) {
        string_list dme_part = simgear::strutils::split(name , 0 ,1);
        string_list navaid_part = simgear::strutils::split(ref.get()->name(), 0 ,1);

        if ( simgear::strutils::uppercase(navaid_part[0]) == simgear::strutils::uppercase(dme_part[0]) ) {
          navaid_dme = ref.get()->guid();
        } else {
            SG_LOG(SG_NAVAID, SG_INFO,
                   utf8Path << ":" << lineNum << ": DME '" << ident << "' (" <<
                   name << "): while looking for a colocated navaid, found " <<
                   "that the closest match has wrong name: '" <<
                   ref->ident() << "' (" << ref->name() << ")");
        }
      } else {
          SG_LOG(SG_NAVAID, SG_INFO,
                 utf8Path << ":" << lineNum << ": couldn't find any colocated "
                 "navaid for DME '" << ident << "' (" << name << ")");
      }
    } // of have a co-located navaid to locate
  }

  if ((type >= FGPositioned::ILS) && (type <= FGPositioned::GS)) {
    arp = cache->findAirportRunway(name);
    if (!arp.first || !arp.second) {
      SG_LOG(SG_NAVAID, SG_INFO,
             utf8Path << ":" << lineNum << ": couldn't find matching runway " <<
             "for ILS/LOC/GS navaid '" << name << "', ignoring it.");
      return 0;
    }
    if (arp.second) {
      runway = FGPositioned::loadById<FGRunway>(arp.second);
      assert(runway);
#if 0
      // code is disabled since it's causing some problems, see
      // https://sourceforge.net/p/flightgear/codetickets/926/
      if (elev_ft <= 0) {
        // snap to runway elevation
        pos.setElevationFt(runway->geod().getElevationFt());
      }
#endif
    } // of found runway in the DB
  } // of type is runway-related
  
  bool isLoc = (type == FGPositioned::ILS) || (type == FGPositioned::LOC);
  if (runway && autoAlignLocalizers && isLoc) {
    alignLocaliserWithRunway(runway, ident, pos, multiuse);
  }

  PositionedID r = cache->insertNavaid(type, ident, name, pos, freq, range, multiuse,
                             arp.first, arp.second);
  
  if (isLoc) {
    cache->setRunwayILS(arp.second, r);
  }

  if (navaid_dme) {
    cache->setNavaidColocated(navaid_dme, r);
  }
  
  return r;
}

// load and initialize the navigational databases
void navDBInit(const SGPath& path)
{
  NavDataCache* cache = NavDataCache::instance();
  const string utf8Path = path.utf8Str();
  const std::size_t fileSize = path.sizeInBytes();
  sg_gzifstream in(path);

  if ( !in.is_open() ) {
    throw sg_io_exception(
      "Cannot open file (" + simgear::strutils::error_string(errno) + ")",
      sg_location(path));
  }

  autoAlignLocalizers = fgGetBool("/sim/navdb/localizers/auto-align", true);
  autoAlignThreshold = fgGetDouble( "/sim/navdb/localizers/auto-align-threshold-deg", 5.0 );

  string line;

  // Skip the first two lines
  for (int i = 0; i < 2; i++) {
    std::getline(in, line);
    throwExceptionIfStreamError(in, path);
  }

  unsigned int lineNumber;
  unsigned long version;

  try {
    vector<string> fields(simgear::strutils::split(line, 0, 1));
    version = std::stoul(fields[0]);
  } catch (const std::logic_error& exc) {
    std::string errMsg = utf8Path + ": unable to parse version from header";
    std::string strippedLine = simgear::strutils::stripTrailingNewlines(line);
    SG_LOG(SG_NAVAID, SG_ALERT, errMsg << ": " << strippedLine );
    throw sg_format_exception(errMsg, strippedLine);
  }

  for (lineNumber = 3; std::getline(in, line); lineNumber++) {
    processNavLine(line, utf8Path, lineNumber, FGPositioned::INVALID, version);

    if ((lineNumber % 100) == 0) {
      // every 100 lines
      unsigned int percent = (in.approxOffset() * 100) / fileSize;
      cache->setRebuildPhaseProgress(NavDataCache::REBUILD_NAVAIDS, percent);
    }

  } // of stream data loop

  throwExceptionIfStreamError(in, path);
}

void loadCarrierNav(const SGPath& path)
{
  SG_LOG( SG_NAVAID, SG_DEBUG, "Opening file: " << path );
  const string utf8Path = path.utf8Str();
  sg_gzifstream in(path);

  if ( !in.is_open() ) {
    throw sg_io_exception(
      "Cannot open file (" + simgear::strutils::error_string(errno) + ")",
      sg_location(path));
  }

  string line;
  unsigned int lineNumber;

  for (lineNumber = 1; std::getline(in, line); lineNumber++) {
    // Force the navaid type to be MOBILE_TACAN
    processNavLine(line, utf8Path, lineNumber, FGPositioned::MOBILE_TACAN);
  }

  throwExceptionIfStreamError(in, path);
}

bool loadTacan(const SGPath& path, FGTACANList *channellist)
{
    SG_LOG( SG_NAVAID, SG_DEBUG, "opening file: " << path );
    sg_gzifstream inchannel( path );
    
    if ( !inchannel.is_open() ) {
        SG_LOG( SG_NAVAID, SG_ALERT, "Cannot open file: " << path );
      return false;
    }
    
    // skip first line
    inchannel >> skipeol;
    while ( ! inchannel.eof() ) {
        FGTACANRecord *r = new FGTACANRecord;
        inchannel >> (*r);
        channellist->add ( r );
 	
    } // end while

    return true;
}
  
} // of namespace flightgear
