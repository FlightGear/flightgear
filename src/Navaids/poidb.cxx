// poidb.cxx -- points of interest management routines
//
// Written by Christian Schmitt, March 2013
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

#include "poidb.hxx"

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sgstream.hxx>

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
  default:
    throw sg_range_exception("Unknown POI type", "FGNavDataCache::readPOIFromStream");
  }
}

namespace flightgear
{

static PositionedID readPOIFromStream(std::istream& aStream, NavDataCache* cache,
                                        FGPositioned::Type type = FGPositioned::INVALID)
{
    if (aStream.eof()) {
        return 0;
    }

    aStream >> skipws;
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

  return cache->createPOI(type, name, pos);
}

// load and initialize the POI database
bool poiDBInit(const SGPath& path)
{
    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_NAVAID, SG_ALERT, "Cannot open file: " << path.str() );
      return false;
    }

    NavDataCache* cache = NavDataCache::instance();
    while (!in.eof()) {
      readPOIFromStream(in, cache);
    } // of stream data loop

    return true;
}

} // of namespace flightgear
