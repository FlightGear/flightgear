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

#include "navdb.hxx"

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/sg_inlines.h>

#include "navrecord.hxx"
#include "navlist.hxx"
#include <Main/globals.hxx>
#include <Navaids/markerbeacon.hxx>
#include <Airports/airport.hxx>
#include <Airports/runways.hxx>
#include <Airports/xmlloader.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/NavDataCache.hxx>

using std::string;
using std::vector;

static FGPositioned::Type
mapRobinTypeToFGPType(int aTy)
{
  switch (aTy) {
 // case 1:
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
  case 99: return FGPositioned::INVALID; // end-of-file code
  default:
    throw sg_range_exception("Got a nav.dat type we don't recognize", "FGNavRecord::createFromStream");
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

static PositionedID readNavFromStream(std::istream& aStream,
                                        FGPositioned::Type type = FGPositioned::INVALID)
{
  NavDataCache* cache = NavDataCache::instance();
  
  int rawType;
  aStream >> rawType;
  if( aStream.eof() || (rawType == 99) || (rawType == 0) )
    return 0; // happens with, eg, carrier_nav.dat
  
  double lat, lon, elev_ft, multiuse;
  int freq, range;
  std::string name, ident;
  aStream >> lat >> lon >> elev_ft >> freq >> range >> multiuse >> ident;
  getline(aStream, name);
  
  SGGeod pos(SGGeod::fromDegFt(lon, lat, elev_ft));
  name = simgear::strutils::strip(name);

// the type can be forced by our caller, but normally we use th value
// supplied in the .dat file
  if (type == FGPositioned::INVALID) {
    type = mapRobinTypeToFGPType(rawType);
  }
  if (type == FGPositioned::INVALID) {
    return 0;
  }

  if ((type >= FGPositioned::OM) && (type <= FGPositioned::IM)) {
    AirportRunwayPair arp(cache->findAirportRunway(name));
    if (arp.second && (elev_ft < 0.01)) {
    // snap to runway elevation
      FGPositioned* runway = cache->loadById(arp.second);
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
  FGRunway* runway = NULL;
  PositionedID navaid_dme = 0;

  if (type == FGPositioned::DME) {
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
    } else if ( name.find("TACAN") != std::string::npos ) {
      f.addType(FGPositioned::VOR);
    }

    if (f.maxType() > 0) {
      FGPositionedRef ref = FGPositioned::findClosestWithIdent(ident, pos, &f);
      if (ref.valid()) {
        string_list dme_part = simgear::strutils::split(name , 0 ,1);
        string_list navaid_part = simgear::strutils::split(ref.get()->name(), 0 ,1);

        if ( simgear::strutils::uppercase(navaid_part[0]) == simgear::strutils::uppercase(dme_part[0]) ) {
          navaid_dme = ref.get()->guid();
        }
      }
    }
  }

  if ((type >= FGPositioned::ILS) && (type <= FGPositioned::GS)) {
    arp = cache->findAirportRunway(name);
    if (arp.second) {
      runway = FGPositioned::loadById<FGRunway>(arp.second);
      assert(runway);
#if 0
      // code is disabled since it's causing some problems, see
      // http://code.google.com/p/flightgear-bugs/issues/detail?id=926
      if (elev_ft < 0.01) {
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
    
  // silently multiply adf frequencies by 100 so that adf
  // vs. nav/loc frequency lookups can use the same code.
  if (type == FGPositioned::NDB) {
    freq *= 100;
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
bool navDBInit(const SGPath& path)
{
    sg_gzifstream in( path.str() );
    if ( !in.is_open() ) {
        SG_LOG( SG_NAVAID, SG_ALERT, "Cannot open file: " << path.str() );
      return false;
    }
  
  autoAlignLocalizers = fgGetBool("/sim/navdb/localizers/auto-align", true);
  autoAlignThreshold = fgGetDouble( "/sim/navdb/localizers/auto-align-threshold-deg", 5.0 );

    // skip first two lines
    in >> skipeol;
    in >> skipeol;

    while (!in.eof()) {
      readNavFromStream(in);
      in >> skipcomment;
    } // of stream data loop
  
  return true;
}
  
  
bool loadCarrierNav(const SGPath& path)
{    
    SG_LOG( SG_NAVAID, SG_INFO, "opening file: " << path.str() );    
    sg_gzifstream incarrier( path.str() );
    
    if ( !incarrier.is_open() ) {
        SG_LOG( SG_NAVAID, SG_ALERT, "Cannot open file: " << path.str() );
      return false;
    }
    
    while ( ! incarrier.eof() ) {
      incarrier >> skipcomment;
      // force the type to be MOBILE_TACAN
      readNavFromStream(incarrier, FGPositioned::MOBILE_TACAN);
    }

  return true;
}
  
bool loadTacan(const SGPath& path, FGTACANList *channellist)
{
    SG_LOG( SG_NAVAID, SG_INFO, "opening file: " << path.str() );
    sg_gzifstream inchannel( path.str() );
    
    if ( !inchannel.is_open() ) {
        SG_LOG( SG_NAVAID, SG_ALERT, "Cannot open file: " << path.str() );
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
