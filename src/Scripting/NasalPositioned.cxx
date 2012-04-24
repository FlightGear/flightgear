// NasalPositioned.cxx -- expose FGPositioned classes to Nasal
//
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>

#include "NasalPositioned.hxx"

#include <boost/foreach.hpp>

#include <simgear/scene/material/mat.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/bucket/newbucket.hxx>

#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/parking.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/procedure.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>
#include <ATC/CommStation.hxx>
#include <Navaids/route.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Navaids/procedure.hxx>

static void sgrefGhostDestroy(void* g);
naGhostType PositionedGhostType = { sgrefGhostDestroy, "positioned" };
naGhostType WayptGhostType = { sgrefGhostDestroy, "waypoint" };

static void hashset(naContext c, naRef hash, const char* key, naRef val)
{
  naRef s = naNewString(c);
  naStr_fromdata(s, (char*)key, strlen(key));
  naHash_set(hash, s, val);
}

static naRef stringToNasal(naContext c, const std::string& s)
{
    return naStr_fromdata(naNewString(c),
                   const_cast<char *>(s.c_str()), 
                   s.length());
}

static FGPositioned* positionedGhost(naRef r)
{
    if (naGhost_type(r) == &PositionedGhostType)
        return (FGPositioned*) naGhost_ptr(r);
    return 0;
}

static flightgear::Waypt* wayptGhost(naRef r)
{
  if (naGhost_type(r) == &WayptGhostType)
    return (flightgear::Waypt*) naGhost_ptr(r);
  return 0;
}

static void sgrefGhostDestroy(void* g)
{
    SGReferenced* ref = (SGReferenced*)g;
    SGReferenced::put(ref); // unref
}

static naRef airportPrototype;
static naRef routePrototype;
static naRef waypointPrototype;

naRef ghostForPositioned(naContext c, const FGPositioned* pos)
{
    if (!pos) {
        return naNil();
    }
    
    SGReferenced::get(pos); // take a ref
    return naNewGhost(c, &PositionedGhostType, (void*) pos);
}

naRef ghostForWaypt(naContext c, const flightgear::Waypt* wpt)
{
  if (!wpt) {
    return naNil();
  }
  
  SGReferenced::get(wpt); // take a ref
  return naNewGhost(c, &WayptGhostType, (void*) wpt);
}

naRef hashForAirport(naContext c, const FGAirport* apt)
{
    std::string id = apt->ident();
    std::string name = apt->name();
    
  // build runways hash
    naRef rwys = naNewHash(c);
    for(unsigned int r=0; r<apt->numRunways(); ++r) {
      FGRunway* rwy(apt->getRunwayByIndex(r));
      naRef rwyid = stringToNasal(c, rwy->ident());
      naRef rwydata = hashForRunway(c, rwy);
      naHash_set(rwys, rwyid, rwydata);
    }
  
    naRef aptdata = naNewHash(c);
    hashset(c, aptdata, "id", stringToNasal(c, id));
    hashset(c, aptdata, "name", stringToNasal(c, name));
    hashset(c, aptdata, "lat", naNum(apt->getLatitude()));
    hashset(c, aptdata, "lon", naNum(apt->getLongitude()));
    hashset(c, aptdata, "elevation", naNum(apt->getElevation() * SG_FEET_TO_METER));
    hashset(c, aptdata, "has_metar", naNum(apt->getMetar()));
    hashset(c, aptdata, "runways", rwys);
    hashset(c, aptdata, "_positioned", ghostForPositioned(c, apt));
    naRef parents = naNewVector(c);
    naVec_append(parents, airportPrototype);
    hashset(c, aptdata, "parents", parents);
    
    return aptdata;
}

naRef hashForWaypoint(naContext c, flightgear::Waypt* wpt, flightgear::Waypt* next)
{
  SGGeod pos = wpt->position();
  naRef h = naNewHash(c);
  
  flightgear::Procedure* proc = dynamic_cast<flightgear::Procedure*>(wpt->owner());
  if (proc) {
    hashset(c, h, "wp_parent_name", stringToNasal(c, proc->ident()));
    // set 'wp_parent' route object to query the SID / STAR / airway?
    // TODO - needs some extensions to flightgear::Route
  }

  if (wpt->type() == "hold") {
    hashset(c, h, "fly_type", stringToNasal(c, "Hold"));
  } else if (wpt->flag(flightgear::WPT_OVERFLIGHT)) {
    hashset(c, h, "fly_type", stringToNasal(c, "flyOver"));
  } else {
    hashset(c, h, "fly_type", stringToNasal(c, "flyBy"));
  }
  
  hashset(c, h, "wp_type", stringToNasal(c, wpt->type()));
  hashset(c, h, "wp_name", stringToNasal(c, wpt->ident()));
  hashset(c, h, "wp_lat", naNum(pos.getLatitudeDeg()));
  hashset(c, h, "wp_lon", naNum(pos.getLongitudeDeg()));
  hashset(c, h, "alt_cstr", naNum(wpt->altitudeFt()));
  
  if (wpt->speedRestriction() == flightgear::SPEED_RESTRICT_MACH) {
    hashset(c, h, "spd_cstr", naNum(wpt->speedMach()));
  } else {
    hashset(c, h, "spd_cstr", naNum(wpt->speedKts()));
  }
  
  if (next) {
    std::pair<double, double> crsDist =
      next->courseAndDistanceFrom(pos);
    hashset(c, h, "leg_distance", naNum(crsDist.second * SG_METER_TO_NM));
    hashset(c, h, "leg_bearing", naNum(crsDist.first));
    hashset(c, h, "hdg_radial", naNum(wpt->headingRadialDeg()));
  }
  
// leg bearing, distance, etc
  
  
// parents and ghost of the C++ object
  hashset(c, h, "_waypt", ghostForWaypt(c, wpt));
  naRef parents = naNewVector(c);
  naVec_append(parents, waypointPrototype);
  hashset(c, h, "parents", parents);
  
  return h;

}

naRef hashForRunway(naContext c, FGRunway* rwy)
{
    naRef rwyid = stringToNasal(c, rwy->ident());
    naRef rwydata = naNewHash(c);
#define HASHSET(s,l,n) naHash_set(rwydata, naStr_fromdata(naNewString(c),s,l),n)
    HASHSET("id", 2, rwyid);
    HASHSET("lat", 3, naNum(rwy->latitude()));
    HASHSET("lon", 3, naNum(rwy->longitude()));
    HASHSET("heading", 7, naNum(rwy->headingDeg()));
    HASHSET("length", 6, naNum(rwy->lengthM()));
    HASHSET("width", 5, naNum(rwy->widthM()));
    HASHSET("threshold", 9, naNum(rwy->displacedThresholdM()));
    HASHSET("stopway", 7, naNum(rwy->stopwayM()));
        
    if (rwy->ILS()) {
      HASHSET("ils_frequency_mhz", 17, naNum(rwy->ILS()->get_freq() / 100.0));
      HASHSET("ils", 3, hashForNavRecord(c, rwy->ILS(), SGGeod()));
    }
    
    HASHSET("_positioned", 11, ghostForPositioned(c, rwy));
#undef HASHSET
    return rwydata;
}

naRef hashForNavRecord(naContext c, const FGNavRecord* nav, const SGGeod& rel)
{
    naRef navdata = naNewHash(c);
#define HASHSET(s,l,n) naHash_set(navdata, naStr_fromdata(naNewString(c),s,l),n)
    HASHSET("id", 2, stringToNasal(c, nav->ident()));
    HASHSET("name", 4, stringToNasal(c, nav->name()));
    HASHSET("frequency", 9, naNum(nav->get_freq()));
    HASHSET("lat", 3, naNum(nav->get_lat()));
    HASHSET("lon", 3, naNum(nav->get_lon()));
    HASHSET("elevation", 9, naNum(nav->get_elev_ft() * SG_FEET_TO_METER));
    HASHSET("type", 4, stringToNasal(c, nav->nameForType(nav->type())));
    
// FIXME - get rid of these, people should use courseAndDistance instead
    HASHSET("distance", 8, naNum(SGGeodesy::distanceNm( rel, nav->geod() ) * SG_NM_TO_METER ) );
    HASHSET("bearing", 7, naNum(SGGeodesy::courseDeg( rel, nav->geod() ) ) );
    
    // record the real object as a ghost for further operations
    HASHSET("_positioned",11, ghostForPositioned(c, nav));
#undef HASHSET
    
    return navdata;
}

bool geodFromHash(naRef ref, SGGeod& result)
{
  if (!naIsHash(ref)) {
    return false;
  }
  
// first, see if the hash contains a FGPositioned ghost - in which case
// we can read off its position directly
  naRef posGhost = naHash_cget(ref, (char*) "_positioned");
  if (!naIsNil(posGhost)) {
    FGPositioned* pos = positionedGhost(posGhost);
    result = pos->geod();
    return true;
  }
  
  naRef ghost = naHash_cget(ref, (char*) "_waypt");
  if (!naIsNil(ghost)) {
    flightgear::Waypt* w = wayptGhost(ghost);
    result = w->position();
    return true;
  }
  
// then check for manual latitude / longitude names
  naRef lat = naHash_cget(ref, (char*) "lat");
  naRef lon = naHash_cget(ref, (char*) "lon");
  if (naIsNum(lat) && naIsNum(lon)) {
    result = SGGeod::fromDeg(naNumValue(lat).num, naNumValue(lon).num);
    return true;
  }
  
// check for geo.Coord type
    
// check for any synonyms?
    // latitude + longitude?
  
  return false;
}

static int geodFromArgs(naRef* args, int offset, int argc, SGGeod& result)
{
  if (offset >= argc) {
    return 0;
  }
  
  if (geodFromHash(args[offset], result)) {
    return 1;
  }
  
  if (((argc - offset) >= 2) && naIsNum(args[offset]) && naIsNum(args[offset + 1])) {
    double lat = naNumValue(args[0]).num,
    lon = naNumValue(args[1]).num;
    result = SGGeod::fromDeg(lon, lat);
    return 2;
  }
  
  return 0;
}

// Convert a cartesian point to a geodetic lat/lon/altitude.
static naRef f_carttogeod(naContext c, naRef me, int argc, naRef* args)
{
  double lat, lon, alt, xyz[3];
  if(argc != 3) naRuntimeError(c, "carttogeod() expects 3 arguments");
  for(int i=0; i<3; i++)
    xyz[i] = naNumValue(args[i]).num;
  sgCartToGeod(xyz, &lat, &lon, &alt);
  lat *= SG_RADIANS_TO_DEGREES;
  lon *= SG_RADIANS_TO_DEGREES;
  naRef vec = naNewVector(c);
  naVec_append(vec, naNum(lat));
  naVec_append(vec, naNum(lon));
  naVec_append(vec, naNum(alt));
  return vec;
}

// Convert a geodetic lat/lon/altitude to a cartesian point.
static naRef f_geodtocart(naContext c, naRef me, int argc, naRef* args)
{
  if(argc != 3) naRuntimeError(c, "geodtocart() expects 3 arguments");
  double lat = naNumValue(args[0]).num * SG_DEGREES_TO_RADIANS;
  double lon = naNumValue(args[1]).num * SG_DEGREES_TO_RADIANS;
  double alt = naNumValue(args[2]).num;
  double xyz[3];
  sgGeodToCart(lat, lon, alt, xyz);
  naRef vec = naNewVector(c);
  naVec_append(vec, naNum(xyz[0]));
  naVec_append(vec, naNum(xyz[1]));
  naVec_append(vec, naNum(xyz[2]));
  return vec;
}

// For given geodetic point return array with elevation, and a material data
// hash, or nil if there's no information available (tile not loaded). If
// information about the material isn't available, then nil is returned instead
// of the hash.
static naRef f_geodinfo(naContext c, naRef me, int argc, naRef* args)
{
#define HASHSET(s,l,n) naHash_set(matdata, naStr_fromdata(naNewString(c),s,l),n)
  if(argc < 2 || argc > 3)
    naRuntimeError(c, "geodinfo() expects 2 or 3 arguments: lat, lon [, maxalt]");
  double lat = naNumValue(args[0]).num;
  double lon = naNumValue(args[1]).num;
  double elev = argc == 3 ? naNumValue(args[2]).num : 10000;
  const SGMaterial *mat;
  SGGeod geod = SGGeod::fromDegM(lon, lat, elev);
  if(!globals->get_scenery()->get_elevation_m(geod, elev, &mat))
    return naNil();
  naRef vec = naNewVector(c);
  naVec_append(vec, naNum(elev));
  naRef matdata = naNil();
  if(mat) {
    matdata = naNewHash(c);
    naRef names = naNewVector(c);
    BOOST_FOREACH(const std::string& n, mat->get_names())
      naVec_append(names, stringToNasal(c, n));
      
    HASHSET("names", 5, names);
    HASHSET("solid", 5, naNum(mat->get_solid()));
    HASHSET("friction_factor", 15, naNum(mat->get_friction_factor()));
    HASHSET("rolling_friction", 16, naNum(mat->get_rolling_friction()));
    HASHSET("load_resistance", 15, naNum(mat->get_load_resistance()));
    HASHSET("bumpiness", 9, naNum(mat->get_bumpiness()));
    HASHSET("light_coverage", 14, naNum(mat->get_light_coverage()));
  }
  naVec_append(vec, matdata);
  return vec;
#undef HASHSET
}


class AirportInfoFilter : public FGAirport::AirportFilter
{
public:
  AirportInfoFilter() : type(FGPositioned::AIRPORT) {
  }
  
  bool fromArg(naRef arg)
  {
    const char *s = naStr_data(arg);
    if(!strcmp(s, "airport")) type = FGPositioned::AIRPORT;
    else if(!strcmp(s, "seaport")) type = FGPositioned::SEAPORT;
    else if(!strcmp(s, "heliport")) type = FGPositioned::HELIPORT;
    else
      return false;
    
    return true;
  }
  
  virtual FGPositioned::Type minType() const {
    return type;
  }
  
  virtual FGPositioned::Type maxType() const {
    return type;
  }
  
  FGPositioned::Type type;
};

// Returns data hash for particular or nearest airport of a <type>, or nil
// on error.
//
// airportinfo(<id>);                   e.g. "KSFO"
// airportinfo(<type>);                 type := ("airport"|"seaport"|"heliport")
// airportinfo()                        same as  airportinfo("airport")
// airportinfo(<lat>, <lon> [, <type>]);
static naRef f_airportinfo(naContext c, naRef me, int argc, naRef* args)
{
  SGGeod pos = globals->get_aircraft_position();
  FGAirport* apt = NULL;
  
  if(argc >= 2 && naIsNum(args[0]) && naIsNum(args[1])) {
    pos = SGGeod::fromDeg(args[1].num, args[0].num);
    args += 2;
    argc -= 2;
  }
  
  double maxRange = 10000.0; // expose this? or pick a smaller value?
  
  AirportInfoFilter filter; // defaults to airports only
  
  if(argc == 0) {
    // fall through and use AIRPORT
  } else if(argc == 1 && naIsString(args[0])) {
    if (filter.fromArg(args[0])) {
      // done!
    } else {
      // user provided an <id>, hopefully
      apt = FGAirport::findByIdent(naStr_data(args[0]));
      if (!apt) {
        // return nil here, but don't raise a runtime error; this is a
        // legitamate way to validate an ICAO code, for example in a
        // dialog box or similar.
        return naNil();
      }
    }
  } else {
    naRuntimeError(c, "airportinfo() with invalid function arguments");
    return naNil();
  }
  
  if(!apt) {
    apt = FGAirport::findClosest(pos, maxRange, &filter);
    if(!apt) return naNil();
  }
  
  return hashForAirport(c, apt);
}

static naRef f_findAirportsWithinRange(naContext c, naRef me, int argc, naRef* args)
{
  int argOffset = 0;
  SGGeod pos = globals->get_aircraft_position();
  argOffset += geodFromArgs(args, 0, argc, pos);
  
  if (!naIsNum(args[argOffset])) {
    naRuntimeError(c, "findAirportsWithinRange expected range (in nm) as arg %d", argOffset);
  }
  
  AirportInfoFilter filter; // defaults to airports only
  double rangeNm = args[argOffset++].num;
  if (argOffset < argc) {
    filter.fromArg(args[argOffset++]);
  }
  
  naRef r = naNewVector(c);
  
  FGPositioned::List apts = FGPositioned::findWithinRange(pos, rangeNm, &filter);
  FGPositioned::sortByRange(apts, pos);
  
  BOOST_FOREACH(FGPositionedRef a, apts) {
    FGAirport* apt = (FGAirport*) a.get();
    naVec_append(r, hashForAirport(c, apt));
  }
  
  return r;
}

static naRef f_findAirportsByICAO(naContext c, naRef me, int argc, naRef* args)
{
  if (!naIsString(args[0])) {
    naRuntimeError(c, "findAirportsByICAO expects string as arg 0");
  }
  
  int argOffset = 0;
  string prefix(naStr_data(args[argOffset++]));
  AirportInfoFilter filter; // defaults to airports only
  if (argOffset < argc) {
    filter.fromArg(args[argOffset++]);
  }
  
  naRef r = naNewVector(c);
  
  FGPositioned::List apts = FGPositioned::findAllWithIdent(prefix, &filter, false);
  
  BOOST_FOREACH(FGPositionedRef a, apts) {
    FGAirport* apt = (FGAirport*) a.get();
    naVec_append(r, hashForAirport(c, apt));
  }
  
  return r;
}

static FGAirport* airportFromMe(naRef me)
{  
    naRef ghost = naHash_cget(me, (char*) "_positioned");
    if (naIsNil(ghost)) {
        return NULL;
    }
  
    FGPositioned* pos = positionedGhost(ghost);
    if (pos && FGAirport::isAirportType(pos)) {
        return (FGAirport*) pos;
    }

    return NULL;
}

static naRef f_airport_tower(naContext c, naRef me, int argc, naRef* args)
{
    FGAirport* apt = airportFromMe(me);
    if (!apt) {
      naRuntimeError(c, "airport.tower called on non-airport object");
    }
  
    // build a hash for the tower position    
    SGGeod towerLoc = apt->getTowerLocation();
    naRef tower = naNewHash(c);
    hashset(c, tower, "lat", naNum(towerLoc.getLatitudeDeg()));
    hashset(c, tower, "lon", naNum(towerLoc.getLongitudeDeg()));
    hashset(c, tower, "elevation", naNum(towerLoc.getElevationM()));
    return tower;
}

static naRef f_airport_comms(naContext c, naRef me, int argc, naRef* args)
{
    FGAirport* apt = airportFromMe(me);
    if (!apt) {
      naRuntimeError(c, "airport.comms called on non-airport object");
    }
    naRef comms = naNewVector(c);
    
// if we have an explicit type, return a simple vector of frequencies
    if (argc > 0 && naIsScalar(args[0])) {
        std::string commName = naStr_data(args[0]);
        FGPositioned::Type commType = FGPositioned::typeFromName(commName);
        
        BOOST_FOREACH(flightgear::CommStation* comm, apt->commStationsOfType(commType)) {
            naVec_append(comms, naNum(comm->freqMHz()));
        }
    } else {
// otherwise return a vector of hashes, one for each comm station.
        BOOST_FOREACH(flightgear::CommStation* comm, apt->commStations()) {
            naRef commHash = naNewHash(c);
            hashset(c, commHash, "frequency", naNum(comm->freqMHz()));
            hashset(c, commHash, "ident", stringToNasal(c, comm->ident()));
            naVec_append(comms, commHash);
        }
    }
    
    return comms;
}

static naRef f_airport_sids(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportFromMe(me);
  if (!apt) {
    naRuntimeError(c, "airport.sids called on non-airport object");
  }
  
  naRef sids = naNewVector(c);
  
  // if we have an explicit type, return a simple vector of frequencies
  if (argc > 0 && naIsString(args[0])) {
    if (!apt->hasRunwayWithIdent(naStr_data(args[0]))) {
      return naNil();
    }

    FGRunway* rwy = apt->getRunwayByIdent(naStr_data(args[0]));
    BOOST_FOREACH(flightgear::SID* sid, rwy->getSIDs()) {
      naRef procId = stringToNasal(c, sid->ident());
      naVec_append(sids, procId);
    }
  } else {
    for (unsigned int s=0; s<apt->numSIDs(); ++s) {
      flightgear::SID* sid = apt->getSIDByIndex(s);
      naRef procId = stringToNasal(c, sid->ident());
      naVec_append(sids, procId);
    }
  }
  
  return sids;
}

static naRef f_airport_stars(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportFromMe(me);
  if (!apt) {
    naRuntimeError(c, "airport.stars called on non-airport object");
  }
  
  naRef stars = naNewVector(c);
  
  // if we have an explicit type, return a simple vector of frequencies
  if (argc > 0 && naIsString(args[0])) {
    if (!apt->hasRunwayWithIdent(naStr_data(args[0]))) {
      return naNil();
    }
        
    FGRunway* rwy = apt->getRunwayByIdent(naStr_data(args[0]));
    BOOST_FOREACH(flightgear::STAR* s, rwy->getSTARs()) {
      naRef procId = stringToNasal(c, s->ident());
      naVec_append(stars, procId);
    }
  } else {
    for (unsigned int s=0; s<apt->numSTARs(); ++s) {
      flightgear::STAR* star = apt->getSTARByIndex(s);
      naRef procId = stringToNasal(c, star->ident());
      naVec_append(stars, procId);
    }
  }
  
  return stars;
}

static naRef f_airport_parking(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportFromMe(me);
  if (!apt) {
    naRuntimeError(c, "airport.parking called on non-airport object");
  }
  
  naRef r = naNewVector(c);
  std::string type;
  bool onlyAvailable = false;
  
  if (argc > 0 && naIsString(args[0])) {
    type = naStr_data(args[0]);
  }
  
  if ((argc > 1) && naIsNum(args[1])) {
    onlyAvailable = (args[1].num != 0.0);
  }
  
  FGAirportDynamics* dynamics = apt->getDynamics();
  for (int i=0; i<dynamics->getNrOfParkings(); ++i) {
    FGParking* park = dynamics->getParking(i);
  // filter out based on availability and type
    if (onlyAvailable && !park->isAvailable()) {
      continue;
    }
    
    if (!type.empty() && (park->getType() != type)) {
      continue;
    }
    
    naRef nm = stringToNasal(c, park->getName());
    naVec_append(r, nm);
  }
  
  return r;
}

// Returns vector of data hash for navaid of a <type>, nil on error
// navaids sorted by ascending distance 
// navinfo([<lat>,<lon>],[<type>],[<id>])
// lat/lon (numeric): use latitude/longitude instead of ac position
// type:              ("fix"|"vor"|"ndb"|"ils"|"dme"|"tacan"|"any")
// id:                (partial) id of the fix
// examples:
// navinfo("vor")     returns all vors
// navinfo("HAM")     return all navaids who's name start with "HAM"
// navinfo("vor", "HAM") return all vor who's name start with "HAM"
//navinfo(34,48,"vor","HAM") return all vor who's name start with "HAM" 
//                           sorted by distance relative to lat=34, lon=48
static naRef f_navinfo(naContext c, naRef me, int argc, naRef* args)
{
  SGGeod pos;
  
  if(argc >= 2 && naIsNum(args[0]) && naIsNum(args[1])) {
    pos = SGGeod::fromDeg(args[1].num, args[0].num);
    args += 2;
    argc -= 2;
  } else {
    pos = globals->get_aircraft_position();
  }
  
  FGPositioned::Type type = FGPositioned::INVALID;
  nav_list_type navlist;
  const char * id = "";
  
  if(argc > 0 && naIsString(args[0])) {
    const char *s = naStr_data(args[0]);
    if(!strcmp(s, "any")) type = FGPositioned::INVALID;
    else if(!strcmp(s, "fix")) type = FGPositioned::FIX;
    else if(!strcmp(s, "vor")) type = FGPositioned::VOR;
    else if(!strcmp(s, "ndb")) type = FGPositioned::NDB;
    else if(!strcmp(s, "ils")) type = FGPositioned::ILS;
    else if(!strcmp(s, "dme")) type = FGPositioned::DME;
    else if(!strcmp(s, "tacan")) type = FGPositioned::TACAN;
    else id = s; // this is an id
    ++args;
    --argc;
  } 
  
  if(argc > 0 && naIsString(args[0])) {
    if( *id != 0 ) {
      naRuntimeError(c, "navinfo() called with navaid id");
      return naNil();
    }
    id = naStr_data(args[0]);
    ++args;
    --argc;
  }
  
  if( argc > 0 ) {
    naRuntimeError(c, "navinfo() called with too many arguments");
    return naNil();
  }
  
  navlist = globals->get_navlist()->findByIdentAndFreq( pos, id, 0.0, type );
  
  naRef reply = naNewVector(c);
  for( nav_list_type::const_iterator it = navlist.begin(); it != navlist.end(); ++it ) {
    naVec_append( reply, hashForNavRecord(c, *it, pos) );
  }
  return reply;
}

static naRef f_findNavaidsWithinRange(naContext c, naRef me, int argc, naRef* args)
{
  int argOffset = 0;
  SGGeod pos = globals->get_aircraft_position();
  argOffset += geodFromArgs(args, 0, argc, pos);
  
  if (!naIsNum(args[argOffset])) {
    naRuntimeError(c, "findNavaidsWithinRange expected range (in nm) as arg %d", argOffset);
  }
  
  FGPositioned::Type type = FGPositioned::INVALID;
  double rangeNm = args[argOffset++].num;
  if (argOffset < argc) {
    type = FGPositioned::typeFromName(naStr_data(args[argOffset]));
  }
  
  naRef r = naNewVector(c);
  FGNavList::TypeFilter filter(type);
  FGPositioned::List navs = FGPositioned::findWithinRange(pos, rangeNm, &filter);
  FGPositioned::sortByRange(navs, pos);
  
  BOOST_FOREACH(FGPositionedRef a, navs) {
    FGNavRecord* nav = (FGNavRecord*) a.get();
    naVec_append(r, hashForNavRecord(c, nav, pos));
  }
  
  return r;
}

static naRef f_findNavaidByFrequency(naContext c, naRef me, int argc, naRef* args)
{
  int argOffset = 0;
  SGGeod pos = globals->get_aircraft_position();
  argOffset += geodFromArgs(args, 0, argc, pos);
  
  if (!naIsNum(args[argOffset])) {
    naRuntimeError(c, "findNavaidByFrequency expectes frequency (in Mhz) as arg %d", argOffset);
  }
  
  FGPositioned::Type type = FGPositioned::INVALID;
  double freqMhz = args[argOffset++].num;
  if (argOffset < argc) {
    type = FGPositioned::typeFromName(naStr_data(args[argOffset]));
  }
  
  nav_list_type navs = globals->get_navlist()->findAllByFreq(freqMhz, pos, type);
  if (navs.empty()) {
    return naNil();
  }
  
  return hashForNavRecord(c, navs.front().ptr(), pos);
}

static naRef f_findNavaidsByFrequency(naContext c, naRef me, int argc, naRef* args)
{
  int argOffset = 0;
  SGGeod pos = globals->get_aircraft_position();
  argOffset += geodFromArgs(args, 0, argc, pos);
  
  if (!naIsNum(args[argOffset])) {
    naRuntimeError(c, "findNavaidsByFrequency expectes frequency (in Mhz) as arg %d", argOffset);
  }
  
  FGPositioned::Type type = FGPositioned::INVALID;
  double freqMhz = args[argOffset++].num;
  if (argOffset < argc) {
    type = FGPositioned::typeFromName(naStr_data(args[argOffset]));
  }
  
  naRef r = naNewVector(c);
  nav_list_type navs = globals->get_navlist()->findAllByFreq(freqMhz, pos, type);
  
  BOOST_FOREACH(nav_rec_ptr a, navs) {
    naVec_append(r, hashForNavRecord(c, a.ptr(), pos));
  }
  
  return r;
}

static naRef f_findNavaidsByIdent(naContext c, naRef me, int argc, naRef* args)
{
  int argOffset = 0;
  SGGeod pos = globals->get_aircraft_position();
  argOffset += geodFromArgs(args, 0, argc, pos);
  
  if (!naIsString(args[argOffset])) {
    naRuntimeError(c, "findNavaidsByIdent expectes ident string as arg %d", argOffset);
  }
  
  FGPositioned::Type type = FGPositioned::INVALID;
  string ident = naStr_data(args[argOffset++]);
  if (argOffset < argc) {
    type = FGPositioned::typeFromName(naStr_data(args[argOffset]));
  }
  
  naRef r = naNewVector(c);
  nav_list_type navs = globals->get_navlist()->findByIdentAndFreq(pos, ident, 0.0, type);
  
  BOOST_FOREACH(nav_rec_ptr a, navs) {
    naVec_append(r, hashForNavRecord(c, a.ptr(), pos));
  }
  
  return r;
}


// Convert a cartesian point to a geodetic lat/lon/altitude.
static naRef f_magvar(naContext c, naRef me, int argc, naRef* args)
{
  SGGeod pos = globals->get_aircraft_position();
  if (argc == 0) {
    // fine, use aircraft position
  } else if (geodFromArgs(args, 0, argc, pos)) {
    // okay
  } else {
    naRuntimeError(c, "magvar() expects no arguments, a positioned hash or lat,lon pair");
  }
  
  double jd = globals->get_time_params()->getJD();
  double magvarDeg = sgGetMagVar(pos, jd) * SG_RADIANS_TO_DEGREES;
  return naNum(magvarDeg);
}

static naRef f_courseAndDistance(naContext c, naRef me, int argc, naRef* args)
{
    SGGeod from = globals->get_aircraft_position(), to, p;
    int argOffset = geodFromArgs(args, 0, argc, p);
    if (geodFromArgs(args, argOffset, argc, to)) {
      from = p; // we parsed both FROM and TO args, so first was from
    } else {
      to = p; // only parsed one arg, so FROM is current
    }
  
    if (argOffset == 0) {
        naRuntimeError(c, "invalid arguments to courseAndDistance");
    }
    
    double course, course2, d;
    SGGeodesy::inverse(from, to, course, course2, d);
    
    naRef result = naNewVector(c);
    naVec_append(result, naNum(course));
    naVec_append(result, naNum(d * SG_METER_TO_NM));
    return result;
}

static naRef f_tilePath(naContext c, naRef me, int argc, naRef* args)
{
    SGGeod pos = globals->get_aircraft_position();
    geodFromArgs(args, 0, argc, pos);
    SGBucket b(pos);
    return stringToNasal(c, b.gen_base_path());
}

static naRef f_route(naContext c, naRef me, int argc, naRef* args)
{
  naRef route = naNewHash(c);
  
  // return active route hash by default,
  // other routes in the future
  
  naRef parents = naNewVector(c);
  naVec_append(parents, routePrototype);
  hashset(c, route, "parents", parents);
  
  return route;
}

static naRef f_route_getWP(naContext c, naRef me, int argc, naRef* args)
{
  FGRouteMgr* rm = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
  
  int index;
  if (argc == 0) {
    index = rm->currentIndex();
  } else {
    index = (int) naNumValue(args[0]).num;
  }
  
  if ((index < 0) || (index >= rm->numWaypts())) {
    return naNil();
  }
  
  flightgear::Waypt* next = NULL;
  if (index < (rm->numWaypts() - 1)) {
    next = rm->wayptAtIndex(index + 1);
  }
  return hashForWaypoint(c, rm->wayptAtIndex(index), next);
}

static naRef f_route_currentWP(naContext c, naRef me, int argc, naRef* args)
{
  FGRouteMgr* rm = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
  flightgear::Waypt* next = NULL;
  if (rm->currentIndex() < (rm->numWaypts() - 1)) {
    next = rm->wayptAtIndex(rm->currentIndex() + 1);
  }
  return hashForWaypoint(c, rm->currentWaypt(), next);
}

static naRef f_route_currentIndex(naContext c, naRef me, int argc, naRef* args)
{
  FGRouteMgr* rm = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
  return naNum(rm->currentIndex());
}

static naRef f_route_numWaypoints(naContext c, naRef me, int argc, naRef* args)
{
  FGRouteMgr* rm = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
  return naNum(rm->numWaypts());
}

static flightgear::Waypt* wayptFromMe(naRef me)
{  
  naRef ghost = naHash_cget(me, (char*) "_waypt");
  if (naIsNil(ghost)) {
    return NULL;
  }
  
  return wayptGhost(ghost);
}

static naRef f_waypoint_navaid(naContext c, naRef me, int argc, naRef* args)
{
  flightgear::Waypt* w = wayptFromMe(me);
  if (!w) {
    naRuntimeError(c, "waypoint.navaid called on non-waypoint object");
  }
  
  FGPositioned* pos = w->source();
  if (!pos) {
    return naNil();
  }
  
  switch (pos->type()) {
  case FGPositioned::VOR:
  case FGPositioned::NDB:
  case FGPositioned::ILS:
  case FGPositioned::LOC:
  case FGPositioned::GS:
  case FGPositioned::DME:
  case FGPositioned::TACAN: {
    FGNavRecord* nav = (FGNavRecord*) pos;
    return hashForNavRecord(c, nav, globals->get_aircraft_position());
  }
      
  default:
    return naNil();
  }
}

static naRef f_waypoint_airport(naContext c, naRef me, int argc, naRef* args)
{
  flightgear::Waypt* w = wayptFromMe(me);
  if (!w) {
    naRuntimeError(c, "waypoint.navaid called on non-waypoint object");
  }
  
  FGPositioned* pos = w->source();
  if (!pos || FGAirport::isAirportType(pos)) {
    return naNil();
  }
  
  return hashForAirport(c, (FGAirport*) pos);
}

static naRef f_waypoint_runway(naContext c, naRef me, int argc, naRef* args)
{
  flightgear::Waypt* w = wayptFromMe(me);
  if (!w) {
    naRuntimeError(c, "waypoint.navaid called on non-waypoint object");
  }
  
  FGPositioned* pos = w->source();
  if (!pos || (pos->type() != FGPositioned::RUNWAY)) {
    return naNil();
  }
  
  return hashForRunway(c, (FGRunway*) pos);
}

// Table of extension functions.  Terminate with zeros.
static struct { const char* name; naCFunction func; } funcs[] = {
  { "carttogeod", f_carttogeod },
  { "geodtocart", f_geodtocart },
  { "geodinfo", f_geodinfo },
  { "airportinfo", f_airportinfo },
  { "findAirportsWithinRange", f_findAirportsWithinRange },
  { "findAirportsByICAO", f_findAirportsByICAO },
  { "navinfo", f_navinfo },
  { "findNavaidsWithinRange", f_findNavaidsWithinRange },
  { "findNavaidByFrequency", f_findNavaidByFrequency },
  { "findNavaidsByFrequency", f_findNavaidsByFrequency },
  { "findNavaidsByID", f_findNavaidsByIdent },
  { "route", f_route },
  { "magvar", f_magvar },
  { "courseAndDistance", f_courseAndDistance },
  { "bucketPath", f_tilePath },
  { 0, 0 }
};


naRef initNasalPositioned(naRef globals, naContext c, naRef gcSave)
{
    airportPrototype = naNewHash(c);
    hashset(c, gcSave, "airportProto", airportPrototype);
  
    hashset(c, airportPrototype, "tower", naNewFunc(c, naNewCCode(c, f_airport_tower)));
    hashset(c, airportPrototype, "comms", naNewFunc(c, naNewCCode(c, f_airport_comms)));
    hashset(c, airportPrototype, "sids", naNewFunc(c, naNewCCode(c, f_airport_sids)));
    hashset(c, airportPrototype, "stars", naNewFunc(c, naNewCCode(c, f_airport_stars)));
    hashset(c, airportPrototype, "parking", naNewFunc(c, naNewCCode(c, f_airport_parking)));
  
    routePrototype = naNewHash(c);
    hashset(c, gcSave, "routeProto", routePrototype);
      
    hashset(c, routePrototype, "getWP", naNewFunc(c, naNewCCode(c, f_route_getWP)));
    hashset(c, routePrototype, "currentWP", naNewFunc(c, naNewCCode(c, f_route_currentWP)));
    hashset(c, routePrototype, "currentIndex", naNewFunc(c, naNewCCode(c, f_route_currentIndex)));
    hashset(c, routePrototype, "getPlanSize", naNewFunc(c, naNewCCode(c, f_route_numWaypoints)));
    
    waypointPrototype = naNewHash(c);
    hashset(c, gcSave, "wayptProto", waypointPrototype);
    
    hashset(c, waypointPrototype, "navaid", naNewFunc(c, naNewCCode(c, f_waypoint_navaid)));
    hashset(c, waypointPrototype, "runway", naNewFunc(c, naNewCCode(c, f_waypoint_runway)));
    hashset(c, waypointPrototype, "airport", naNewFunc(c, naNewCCode(c, f_waypoint_airport)));
  
    for(int i=0; funcs[i].name; i++) {
      hashset(c, globals, funcs[i].name,
            naNewFunc(c, naNewCCode(c, funcs[i].func)));
    }
  
  return naNil();
}

