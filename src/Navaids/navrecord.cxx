// navrecord.cxx -- generic vor/dme/ndb class
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
  #include "config.h"
#endif

#include <istream>

#include <simgear/misc/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/debug/logstream.hxx>

#include "Navaids/navrecord.hxx"
#include "Airports/simple.hxx"
#include "Airports/runways.hxx"
#include "Main/fg_props.hxx"

FGNavRecord::FGNavRecord(Type aTy, const std::string& aIdent, 
  const std::string& aName, const SGGeod& aPos,
  int aFreq, int aRange, double aMultiuse) :
  FGPositioned(aTy, aIdent, aPos),
  freq(aFreq),
  range(aRange),
  multiuse(aMultiuse),
  _name(aName),
  serviceable(true),
  trans_ident(aIdent)
{ 
  initAirportRelation();

  // Ranges are included with the latest data format, no need to
  // assign our own defaults, unless the range is not set for some
  // reason.
  if (range < 0.1) {
    SG_LOG(SG_GENERAL, SG_WARN, "navaid " << ident() << " has no range set, using defaults");
    switch (type()) {
    case NDB:
    case VOR:
      range = FG_NAV_DEFAULT_RANGE;
      break;
    
    case LOC:
    case ILS:
    case GS:
      range = FG_LOC_DEFAULT_RANGE;
      break;
      
    case DME:
      range = FG_DME_DEFAULT_RANGE;
      break;
    
    default:
      range = FG_LOC_DEFAULT_RANGE;
    }
  }
  
}

static FGPositioned::Type
mapRobinTypeToFGPType(int aTy)
{
  switch (aTy) {
 // case 1:
  case 2: return FGPositioned::NDB;
  case 3: return FGPositioned::VOR;
  case 4: return FGPositioned::LOC;
  case 5: return FGPositioned::ILS;
  case 6: return FGPositioned::GS;
  case 7: return FGPositioned::OM;
  case 8: return FGPositioned::MM;
  case 9: return FGPositioned::IM;
  case 12:
  case 13: return FGPositioned::DME;
  case 99: return FGPositioned::INVALID; // end-of-file code
  default:
    throw sg_range_exception("Got a nav.dat type we don't recognize", "FGNavRecord::createFromStream");
  }
}

FGNavRecord* FGNavRecord::createFromStream(std::istream& aStream)
{
  int rawType;
  aStream >> rawType;
  if (aStream.eof()) {
    return NULL; // happens with, eg, carrier_nav.dat
  }
  
  Type type = mapRobinTypeToFGPType(rawType);
  if (type == INVALID) return NULL;
  
  double lat, lon, elev_ft, multiuse;
  int freq, range;
  std::string name, ident;
  aStream >> lat >> lon >> elev_ft >> freq >> range >> multiuse >> ident;
  getline(aStream, name);
  
  // silently multiply adf frequencies by 100 so that adf
  // vs. nav/loc frequency lookups can use the same code.
  if (type == NDB) {
    freq *= 100;
  }
  
  // ensure marker beacons are anonymous, so they don't get added to the
  // name index
  if ((type >= OM) && (type <= IM)) {
    ident.clear();
  }
  
  FGNavRecord* result = new FGNavRecord(type, ident,
    simgear::strutils::strip(name), SGGeod::fromDegFt(lon, lat, elev_ft),
    freq, range, multiuse);
    
  return result;
}

void FGNavRecord::initAirportRelation()
{
  if ((type() < ILS) || (type() > IM)) {
    return; // not airport-located
  }
  
  vector<string> parts = simgear::strutils::split(_name);
  if (parts.size() < 2) {
    SG_LOG(SG_GENERAL, SG_WARN, "navaid has malformed name:" << _name);
    return;
  }
  
  const FGAirport* apt = fgFindAirportID(parts[0]);
  
  if (!apt) {
    SG_LOG(SG_GENERAL, SG_WARN, "navaid " << _name << " associated with bogus airport ID:" << parts[0]);
    return;
  }
  
  runway = apt->getRunwayByIdent(parts[1]);
  if (!runway) {
    SG_LOG(SG_GENERAL, SG_WARN, "navaid " << _name << " associated with bogus runway ID:" << parts[1]);
    return;
  }
    
  // fudge elevation to the field elevation if it's not specified
  if (fabs(elevation()) < 0.01) {
    mPosition.setElevationFt(apt->getElevation());
  }
  
  // align localizers with their runway
  if ((type() == ILS) || (type() == LOC)) {
    if (!fgGetBool("/sim/navdb/localizers/auto-align", true)) {
      return;
    }
    
   double threshold 
     = fgGetDouble( "/sim/navdb/localizers/auto-align-threshold-deg", 5.0 );
    alignLocaliserWithRunway(runway, threshold);
  }
}

void FGNavRecord::alignLocaliserWithRunway(FGRunway* aRunway, double aThreshold)
{
// find the distance from the threshold to the localizer
  SGGeod runwayThreshold(aRunway->threshold());
  double dist, az1, az2;
  SGGeodesy::inverse(mPosition, runwayThreshold, az1, az2, dist);

// back project that distance along the runway center line
  SGGeod newPos = aRunway->pointOnCenterline(dist);

  double hdg_diff = get_multiuse() - aRunway->headingDeg();

  // clamp to [-180.0 ... 180.0]
  if ( hdg_diff < -180.0 ) {
      hdg_diff += 360.0;
  } else if ( hdg_diff > 180.0 ) {
      hdg_diff -= 360.0;
  }

  if ( fabs(hdg_diff) <= aThreshold ) {
    mPosition = newPos;
    set_multiuse( aRunway->headingDeg() );
  } else {
    SG_LOG(SG_GENERAL, SG_WARN, "localizer:" << ident() << ", aligning with runway " 
      << aRunway->ident() << " exceeded heading threshold");
  }
}

FGTACANRecord::FGTACANRecord(void) :
    channel(""),
    freq(0)
    
{
}

std::istream&
operator >> ( std::istream& in, FGTACANRecord& n )
{
    in >> n.channel >> n.freq ;
    //getline( in, n.name );

    return in;
}
