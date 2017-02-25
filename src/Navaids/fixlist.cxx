// fixlist.cxx -- fix list management class
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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
#  include <config.h>
#endif

#include <stdlib.h>             // atof()

#include <algorithm>
#include <string>               // std::getline()
#include <errno.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx> // simgear::strutils::split()
#include <simgear/math/SGGeod.hxx>
#include <simgear/math/SGMathFwd.hxx>
#include <simgear/math/SGVec3.hxx>
#include <simgear/constants.h>
#include <simgear/structure/exception.hxx>

#include "fixlist.hxx"
#include <Navaids/fix.hxx>
#include <Navaids/NavDataCache.hxx>

// A navaid with the same ident as an existing navaid not more distant than
// this will be considered duplicate.
static const double DUPLICATE_DETECTION_RADIUS_NM = 15;

FGFix::FGFix(PositionedID aGuid, const std::string& aIdent, const SGGeod& aPos) :
  FGPositioned(aGuid, FIX, aIdent, aPos)
{
}


namespace flightgear
{

const unsigned int LINES_IN_FIX_DAT = 119724;

FixesLoader::FixesLoader() : _cache(NavDataCache::instance())
{ }

FixesLoader::~FixesLoader()
{ }

// Load fixes from the specified fix.dat (or fix.dat.gz) file
void FixesLoader::loadFixes(const SGPath& path)
{
  sg_gzifstream in( path );
  const std::string utf8path = path.utf8Str();

  if ( !in.is_open() ) {
    throw sg_io_exception(
      "Cannot open file (" + simgear::strutils::error_string(errno) + ")",
      sg_location(path));
  }

  // toss the first two lines of the file
  for (int i = 0; i < 2; i++) {
    in >> skipeol;
    throwExceptionIfStreamError(in, path);
  }

  unsigned int lineNumber = 3;

  // read in each remaining line of the file
  for (std::string line; std::getline(in, line); lineNumber++) {
    std::vector<std::string> fields = simgear::strutils::split(line);
    std::vector<std::string>::size_type nb_fields = fields.size();
    const std::string endOfData = "99"; // special code in the fix.dat spec

    if (nb_fields == 0) {       // blank line
      continue;
    } else if (nb_fields == 1) {
      if (fields[0] == endOfData)
        break;
      else {
        SG_LOG(SG_NAVAID, SG_WARN, utf8path << ": malformed line #" <<
               lineNumber << ": only one field, but it is not '99'");
        continue;
      }
    } else if (nb_fields < 3) {
      SG_LOG(SG_NAVAID, SG_WARN, utf8path << ": malformed line #" <<
             lineNumber << ": expected at least 3 fields, but got " <<
             fields.size());
      continue;
    } else if (nb_fields != 3 && nb_fields != 5 && nb_fields != 6) {
      // XP FIX1101 format calls for 6 fields, the last being optional.
      // XP FIX1100 has 5 fields.
      // Earlier formats have 3 fields.
      // In all these cases we need the first three only.
      SG_LOG(SG_NAVAID, SG_INFO, utf8path << ": line #" <<
             lineNumber << ": ignoring extra fields, past the first three " <<
             "(expected 3 or 5 or 6 fields, but got " << fields.size() << ")");
    }

    std::string ident = fields[2];
    double lat, lon;
    try {
      lat = std::stod(fields[0]);
      lon = std::stod(fields[1]);
    } catch (const std::exception& e) {
      SG_LOG(SG_NAVAID, SG_WARN, utf8path << ": malformed line #" <<
             lineNumber << ": error parsing coordinates: " << fields[0] <<
             " " << fields[1]);
      continue;
    }
    SGGeod pos(SGGeod::fromDeg(lon, lat));
    bool duplicate = false;
    auto range = _loadedFixes.equal_range(ident);
    for (auto it = range.first; it != range.second; ++it) {
      double distNm = dist(SGVec3d::fromGeod(pos),
                           SGVec3d::fromGeod(it->second)) * SG_METER_TO_NM;
      if (distNm < DUPLICATE_DETECTION_RADIUS_NM) {
        SG_LOG(SG_NAVAID, SG_INFO,
               utf8path << ":"  << lineNumber << ": skipping fix " <<
               ident << " (already defined nearby)");
        duplicate = true;
        break;
      }
    }

    if (!duplicate) {
      _cache->insertFix(ident, pos);
      _loadedFixes.insert({ident, pos});
    }

    if ((lineNumber % 100) == 0) {
      // every 100 lines
      unsigned int percent = (lineNumber * 100) / LINES_IN_FIX_DAT;
      _cache->setRebuildPhaseProgress(NavDataCache::REBUILD_FIXES, percent);
    }
  }

  throwExceptionIfStreamError(in, path);
}

void FixesLoader::throwExceptionIfStreamError(
  const sg_gzifstream& input_stream, const SGPath& path)
{
  if (input_stream.bad()) {
    const std::string errMsg = simgear::strutils::error_string(errno);

    SG_LOG(SG_NAVAID, SG_ALERT,
           "Error while reading '" << path.utf8Str() << "': " << errMsg);
    throw sg_io_exception("Error reading file (" + errMsg + ")",
                          sg_location(path));
  }
}


} // of namespace flightgear;
