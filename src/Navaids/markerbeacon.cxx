// markerbeacon.cxx -- marker beacon class
//
// Written by James Turner, started December 2008.
//
// Copyright (C) 2008  Curtis L. Olson - http://www.flightgear.org/~curt
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
  #include "config.h"
#endif

#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <Navaids/markerbeacon.hxx>
#include <Airports/runways.hxx>
#include <Navaids/navdb.hxx>

using std::string;

FGPositioned::Type
FGMarkerBeaconRecord::mapType(int aTy)
{
  switch (aTy) {
  case 7: return FGPositioned::OM;
  case 8: return FGPositioned::MM;
  case 9: return FGPositioned::IM;
  default:
    throw sg_range_exception("Got a non-marker-beacon-type", 
      "FGMarkerBeaconRecord::mapType");
  }
}

FGMarkerBeaconRecord*
FGMarkerBeaconRecord::create(int aTy, const string& aName, const SGGeod& aPos)
{
  Type fgpTy = mapType(aTy);
  FGRunway* runway = getRunwayFromName(aName);
  if (!runway)
  {
      SG_LOG(SG_GENERAL, SG_WARN, "Failed to create beacon for unknown runway '" << aName << "'.");
      return NULL;
  }
  SGGeod pos(aPos);
  // fudge elevation to the runway elevation if it's not specified
  if (fabs(pos.getElevationFt()) < 0.01) {
    pos.setElevationFt(runway->elevation());
  }
  
  return new FGMarkerBeaconRecord(fgpTy, runway, pos);
}


FGMarkerBeaconRecord::FGMarkerBeaconRecord(Type aTy, FGRunway* aRunway, const SGGeod& aPos) :
  FGPositioned(aTy, string(), aPos),
  _runway(aRunway)
{
  init(true); // init FGPositioned
}
