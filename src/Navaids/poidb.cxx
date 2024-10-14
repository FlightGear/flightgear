/*
 * SPDX-FileCopyrightText: (C) Christian Schmitt, March 2013
 * SPDX_FileComment: points of interest management routines
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <istream>              // std::ws
#include "poidb.hxx"

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/io/iostreams/sgstream.hxx>

#include <Navaids/NavDataCache.hxx>


using std::string;

static FGPositioned::Type
mapPOITypeToFGPType(int aTy)
{
  switch (aTy) {
  case 10: return FGPositioned::COUNTRY;
  case 12: return FGPositioned::CITY;
  case 13: return FGPositioned::TOWN;
  case 14: return FGPositioned::VILLAGE;
  
  case 1000: return FGPositioned::VISUAL_REPORTING_POINT;
  case 1001: return FGPositioned::WAYPOINT;

  default:
    throw sg_range_exception("Unknown POI type", "FGNavDataCache::readPOIFromStream");
  }
}

namespace flightgear
{

// Duplicate POIs with the same ident will be removed if the disance
// between them is less than this.
static const double DUPLICATE_DETECTION_RADIUS_NM = 10;

static const double DUPLICATE_DETECTION_RADIUS_SQR_M = DUPLICATE_DETECTION_RADIUS_NM * DUPLICATE_DETECTION_RADIUS_NM * SG_NM_TO_METER * SG_NM_TO_METER;

static bool isNearby(const SGVec3d& pos1, const SGVec3d& pos2) 
{
  const double d = distSqr(pos1, pos2);
  return d <= DUPLICATE_DETECTION_RADIUS_SQR_M;
}

POILoader::POILoader() : 
  _cache(NavDataCache::instance())
{ 

}


void POILoader::loadPOIs(const NavDataCache::SceneryLocation& sceneryLocation,
                            std::size_t bytesReadSoFar,
                            std::size_t totalSizeOfAllDatFiles)
{
    _path = sceneryLocation.datPath;
    sg_gzifstream in( _path );
    const std::string utf8path = _path.utf8Str();

    if ( !in.is_open() ) {
      throw sg_io_exception(
        "Cannot open file (" + simgear::strutils::error_string(errno) + ")",
        sg_location(_path));
    }

    // Skip the first two lines
    for (int i = 0; i < 2; i++) {
      std::string line;
      std::getline(in, line);
      throwExceptionIfStreamError(in);
    }


    unsigned int lineNumber = 3;

  // read in each remaining line of the file
  for (std::string line; !in.eof(); lineNumber++) {

    readPOIFromStream(in, lineNumber);

    ++lineNumber;
    if ((lineNumber % 100) == 0) {
      // every 100 lines
      unsigned int percent = ((bytesReadSoFar + in.approxOffset()) * 100)
        / totalSizeOfAllDatFiles;
      _cache->setRebuildPhaseProgress(NavDataCache::REBUILD_POIS, percent);
    }
  } // of line iteration

  throwExceptionIfStreamError(in);
}

void POILoader::throwExceptionIfStreamError(const sg_gzifstream& input_stream)
{
  if (input_stream.bad()) {
    const std::string errMsg = simgear::strutils::error_string(errno);

    SG_LOG(SG_NAVAID, SG_ALERT,
           "Error while reading '" << _path.utf8Str() << "': " << errMsg);
    throw sg_io_exception("POILoader: error reading file (" + errMsg + ")",
                          sg_location(_path));
  }
}

PositionedID POILoader::readPOIFromStream(std::istream& aStream,
                                        unsigned int lineNumber,FGPositioned::Type type)
{
    if (aStream.eof()) {
        return 0;
    }

    aStream >> std::ws;
    if (aStream.peek() == '#') {
        aStream >> skipeol;
        return 0;
    }
    
  int rawType;
  aStream >> rawType;
  double lat, lon;
  std::string name;
  aStream >> lat >> lon;
  getline(aStream, name);

  SGGeod pos(SGGeod::fromDeg(lon, lat));
  name = simgear::strutils::strip(name);

  // the type can be forced by our caller, but normally we use the value
  // supplied in the .dat file
  if (type == FGPositioned::INVALID) {
    type = mapPOITypeToFGPType(rawType);
  }
  if (type == FGPositioned::INVALID) {
    return 0;
  }

  const auto cartPos = SGVec3d::fromGeod(pos);

  // de-duplication
  const auto key = std::make_tuple(type, name);
  auto matching = _loadedPOIs.equal_range(key);
  for (auto it = matching.first; it != matching.second; ++it) {
    if (isNearby(cartPos, it->second)) {
      SG_LOG(SG_NAVAID, SG_INFO,
             _path.utf8Str() << ":"  << lineNumber << ": skipping POI '" <<
             name << "' (already defined nearby)");
      return 0;
    }
  }

  _loadedPOIs.emplace(key, cartPos);
  return _cache->createPOI(type, name, pos, name, false);
}


} // of namespace flightgear
