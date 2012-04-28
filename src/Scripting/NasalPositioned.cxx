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

#include <simgear/sg_inlines.h>
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

static void positionedGhostDestroy(void* g);
static void wayptGhostDestroy(void* g);
naGhostType PositionedGhostType = { positionedGhostDestroy, "positioned" };

static const char* airportGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType AirportGhostType = { positionedGhostDestroy, "airport", airportGhostGetMember, 0 };

static const char* navaidGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType NavaidGhostType = { positionedGhostDestroy, "navaid", navaidGhostGetMember, 0 };

static const char* runwayGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType RunwayGhostType = { positionedGhostDestroy, "runway", runwayGhostGetMember, 0 };

static const char* wayptGhostGetMember(naContext c, void* g, naRef field, naRef* out);

naGhostType WayptGhostType = { wayptGhostDestroy, 
  "waypoint",
  wayptGhostGetMember,
  0};

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

static FGAirport* airportGhost(naRef r)
{
  if (naGhost_type(r) == &AirportGhostType)
    return (FGAirport*) naGhost_ptr(r);
  return 0;
}

static FGNavRecord* navaidGhost(naRef r)
{
  if (naGhost_type(r) == &NavaidGhostType)
    return (FGNavRecord*) naGhost_ptr(r);
  return 0;
}

static FGRunway* runwayGhost(naRef r)
{
  if (naGhost_type(r) == &RunwayGhostType)
    return (FGRunway*) naGhost_ptr(r);
  return 0;
}

static void positionedGhostDestroy(void* g)
{
    FGPositioned* pos = (FGPositioned*)g;
    if (!FGPositioned::put(pos)) // unref
        delete pos;
}

static flightgear::Waypt* wayptGhost(naRef r)
{
  if (naGhost_type(r) == &WayptGhostType)
    return (flightgear::Waypt*) naGhost_ptr(r);
  return 0;
}

static void wayptGhostDestroy(void* g)
{
  flightgear::Waypt* wpt = (flightgear::Waypt*)g;
  if (!flightgear::Waypt::put(wpt)) // unref
    delete wpt;
}

static naRef airportPrototype;
static naRef routePrototype;
static naRef waypointPrototype;
static naRef geoCoordClass;

naRef ghostForPositioned(naContext c, const FGPositioned* pos)
{
    if (!pos) {
        return naNil();
    }
    
    FGPositioned::get(pos); // take a ref
    return naNewGhost(c, &PositionedGhostType, (void*) pos);
}

naRef ghostForAirport(naContext c, const FGAirport* apt)
{
  if (!apt) {
    return naNil();
  }
  
  FGPositioned::get(apt); // take a ref
  return naNewGhost2(c, &AirportGhostType, (void*) apt);
}

naRef ghostForNavaid(naContext c, const FGNavRecord* n)
{
  if (!n) {
    return naNil();
  }
  
  FGPositioned::get(n); // take a ref
  return naNewGhost2(c, &NavaidGhostType, (void*) n);
}

naRef ghostForRunway(naContext c, const FGRunway* r)
{
  if (!r) {
    return naNil();
  }
  
  FGPositioned::get(r); // take a ref
  return naNewGhost2(c, &RunwayGhostType, (void*) r);
}

naRef ghostForWaypt(naContext c, const flightgear::Waypt* wpt)
{
  if (!wpt) {
    return naNil();
  }
  
  flightgear::Waypt::get(wpt); // take a ref
  return naNewGhost2(c, &WayptGhostType, (void*) wpt);
}

static const char* airportGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  FGAirport* apt = (FGAirport*) g;
  
  if (!strcmp(fieldName, "parents")) {
    *out = naNewVector(c);
    naVec_append(*out, airportPrototype);
  } else if (!strcmp(fieldName, "id")) *out = stringToNasal(c, apt->ident());
  else if (!strcmp(fieldName, "name")) *out = stringToNasal(c, apt->name());
  else if (!strcmp(fieldName, "lat")) *out = naNum(apt->getLatitude());
  else if (!strcmp(fieldName, "lon")) *out = naNum(apt->getLongitude());
  else if (!strcmp(fieldName, "elevation")) {
    *out = naNum(apt->getElevation() * SG_FEET_TO_METER);
  } else if (!strcmp(fieldName, "has_metar")) {
    *out = naNum(apt->getMetar());
  } else if (!strcmp(fieldName, "runways")) {
    *out = naNewHash(c);
    for(unsigned int r=0; r<apt->numRunways(); ++r) {
      FGRunway* rwy(apt->getRunwayByIndex(r));
      naRef rwyid = stringToNasal(c, rwy->ident());
      naRef rwydata = ghostForRunway(c, rwy);
      naHash_set(*out, rwyid, rwydata);
    }

  } else {
    return 0;
  }
  
  return "";
}

static const char* wayptGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  flightgear::Waypt* wpt = (flightgear::Waypt*) g;

  if (!strcmp(fieldName, "parents")) {
    *out = naNewVector(c);
    naVec_append(*out, waypointPrototype);
  } else if (!strcmp(fieldName, "wp_name")) *out =stringToNasal(c, wpt->ident());
  else if (!strcmp(fieldName, "wp_type")) *out = stringToNasal(c, wpt->type());
  else if (!strcmp(fieldName, "wp_lat")) *out = naNum(wpt->position().getLatitudeDeg());
  else if (!strcmp(fieldName, "wp_lon")) *out = naNum(wpt->position().getLongitudeDeg());
  else if (!strcmp(fieldName, "wp_parent_name")) {
    flightgear::Procedure* proc = dynamic_cast<flightgear::Procedure*>(wpt->owner());
    *out = proc ? stringToNasal(c, proc->ident()) : naNil();
  } else if (!strcmp(fieldName, "fly_type")) {
    if (wpt->type() == "hold") {
      *out = stringToNasal(c, "Hold");
    } else {
      *out = stringToNasal(c, wpt->flag(flightgear::WPT_OVERFLIGHT) ? "flyOver" : "flyBy");
    }
  } else if (!strcmp(fieldName, "alt_cstr")) *out = naNum(wpt->altitudeFt());
  else if (!strcmp(fieldName, "speed_cstr")) {
    double s = (wpt->speedRestriction() == flightgear::SPEED_RESTRICT_MACH) 
      ? wpt->speedMach() : wpt->speedKts();
    *out = naNum(s);
  } else if (!strcmp(fieldName, "leg_distance")) {
    return "please implement me";
  } else if (!strcmp(fieldName, "leg_bearing")) {
    return "please implement me";
  } else {
    return NULL; // member not found
  }
  
  return ""; // success
}

static const char* runwayGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  FGRunway* rwy = (FGRunway*) g;
  
  if (!strcmp(fieldName, "id")) *out = stringToNasal(c, rwy->ident());
  else if (!strcmp(fieldName, "lat")) *out = naNum(rwy->latitude());
  else if (!strcmp(fieldName, "lon")) *out = naNum(rwy->longitude());
  else if (!strcmp(fieldName, "heading")) *out = naNum(rwy->headingDeg());
  else if (!strcmp(fieldName, "length")) *out = naNum(rwy->lengthM());
  else if (!strcmp(fieldName, "width")) *out = naNum(rwy->widthM());
  else if (!strcmp(fieldName, "threshold")) *out = naNum(rwy->displacedThresholdM());
  else if (!strcmp(fieldName, "stopway")) *out = naNum(rwy->stopwayM());
  else if (!strcmp(fieldName, "ils_frequency_mhz")) {
    *out = rwy->ILS() ? naNum(rwy->ILS()->get_freq() / 100.0) : naNil();
  } else if (!strcmp(fieldName, "ils")) {
    *out = ghostForNavaid(c, rwy->ILS());
  } else {
    return 0;
  }
  
  return "";
}

static const char* navaidGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  FGNavRecord* nav = (FGNavRecord*) g;
  
  if (!strcmp(fieldName, "id")) *out = stringToNasal(c, nav->ident());
  else if (!strcmp(fieldName, "name")) *out = stringToNasal(c, nav->name());
  else if (!strcmp(fieldName, "lat")) *out = naNum(nav->get_lat());
  else if (!strcmp(fieldName, "lon")) *out = naNum(nav->get_lon());
  else if (!strcmp(fieldName, "elevation")) {
    *out = naNum(nav->get_elev_ft() * SG_FEET_TO_METER);
  } else if (!strcmp(fieldName, "type")) {
    *out = stringToNasal(c, nav->nameForType(nav->type()));
  } else if (!strcmp(fieldName, "frequency")) {
    *out = naNum(nav->get_freq()); 
  } else if (!strcmp(fieldName, "range_nm")) {
    *out = naNum(nav->get_range()); 
  } else if (!strcmp(fieldName, "course")) {
    if ((nav->type() == FGPositioned::ILS) || (nav->type() == FGPositioned::LOC)) {
      double radial = nav->get_multiuse();
      SG_NORMALIZE_RANGE(radial, 0.0, 360.0);
      *out = naNum(radial);
    } else {
      *out = naNil();
    }
  } else {
    return 0;
  }
  
  return "";
}

static bool hashIsCoord(naRef h)
{
  naRef parents = naHash_cget(h, (char*) "parents");
  if (!naIsVector(parents)) {
    return false;
  }
  
  return naEqual(naVec_get(parents, 0), geoCoordClass);
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
  
// then check for manual latitude / longitude names
  naRef lat = naHash_cget(ref, (char*) "lat");
  naRef lon = naHash_cget(ref, (char*) "lon");
  if (naIsNum(lat) && naIsNum(lon)) {
    result = SGGeod::fromDeg(naNumValue(lon).num, naNumValue(lat).num);
    return true;
  }
  
  if (hashIsCoord(ref)) {
    naRef lat = naHash_cget(ref, (char*) "_lat");
    naRef lon = naHash_cget(ref, (char*) "_lon");
    if (naIsNum(lat) && naIsNum(lon)) {
      result = SGGeod::fromRad(naNumValue(lon).num, naNumValue(lat).num);
      return true;
    }
  }
    
// check for any synonyms?
    // latitude + longitude?
  
  return false;
}

static int geodFromArgs(naRef* args, int offset, int argc, SGGeod& result)
{
  if (offset >= argc) {
    return 0;
  }
  
  if (naIsGhost(args[offset])) {
    naGhostType* gt = naGhost_type(args[offset]);
    if (gt == &AirportGhostType) {
      result = airportGhost(args[offset])->geod();
      return 1;
    }
    
    if (gt == &NavaidGhostType) {
      result = navaidGhost(args[offset])->geod();
      return 1;
    }
    
    if (gt == &RunwayGhostType) {
      result = runwayGhost(args[offset])->geod();
      return 1;
    }
    
    if (gt == &WayptGhostType) {
      result = wayptGhost(args[offset])->position();
      return 1;
    }
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
  
  return ghostForAirport(c, apt);
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
    naVec_append(r, ghostForAirport(c, apt));
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
    naVec_append(r, ghostForAirport(c, apt));
  }
  
  return r;
}

static naRef f_airport_tower(naContext c, naRef me, int argc, naRef* args)
{
    FGAirport* apt = airportGhost(me);
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
    FGAirport* apt = airportGhost(me);
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

static naRef f_airport_runway(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.runway called on non-airport object");
  }
  
  if ((argc < 1) || !naIsString(args[0])) {
    naRuntimeError(c, "airport.runway expects a runway ident argument");
  }
  
  std::string ident(naStr_data(args[0]));
  if (!apt->hasRunwayWithIdent(ident)) {
    return naNil();
  }
  
  return ghostForRunway(c, apt->getRunwayByIdent(ident));
}

static naRef f_airport_sids(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.sids called on non-airport object");
  }
  
  naRef sids = naNewVector(c);
  
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
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.stars called on non-airport object");
  }
  
  naRef stars = naNewVector(c);
  
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
  FGAirport* apt = airportGhost(me);
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
    naVec_append( reply, ghostForNavaid(c, *it) );
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
    naVec_append(r, ghostForNavaid(c, nav));
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
  
  return ghostForNavaid(c, navs.front().ptr());
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
    naVec_append(r, ghostForNavaid(c, a.ptr()));
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
    naVec_append(r, ghostForNavaid(c, a.ptr()));
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

static naRef f_greatCircleMove(naContext c, naRef me, int argc, naRef* args)
{
  SGGeod from = globals->get_aircraft_position(), to;
  int argOffset = 0;
  
  // complication - don't inerpret two doubles (as the only args)
  // as a lat,lon pair - only do so if we have at least three args.
  if (argc > 2) {
    argOffset = geodFromArgs(args, 0, argc, from);
  }
  
  if ((argOffset + 1) >= argc) {
    naRuntimeError(c, "isufficent arguments to greatCircleMove");
  }
  
  if (!naIsNum(args[argOffset]) || !naIsNum(args[argOffset+1])) {
    naRuntimeError(c, "invalid arguments %d and %d to greatCircleMove",
                   argOffset, argOffset + 1);
  }
  
  double course = args[argOffset].num, course2;
  double distanceNm = args[argOffset + 1].num;
  SGGeodesy::direct(from, course, distanceNm * SG_NM_TO_METER, to, course2);
  
  // return geo.Coord
  naRef coord = naNewHash(c);
  hashset(c, coord, "lat", naNum(to.getLatitudeDeg()));
  hashset(c, coord, "lon", naNum(to.getLongitudeDeg()));
  return coord;
}

static naRef f_tilePath(naContext c, naRef me, int argc, naRef* args)
{
    SGGeod pos = globals->get_aircraft_position();
    geodFromArgs(args, 0, argc, pos);
    SGBucket b(pos);
    return stringToNasal(c, b.gen_base_path());
}

static naRef f_tileIndex(naContext c, naRef me, int argc, naRef* args)
{
  SGGeod pos = globals->get_aircraft_position();
  geodFromArgs(args, 0, argc, pos);
  SGBucket b(pos);
  return naNum(b.gen_index());
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
  
  return ghostForWaypt(c, rm->wayptAtIndex(index));
}

static naRef f_route_currentWP(naContext c, naRef me, int argc, naRef* args)
{
  FGRouteMgr* rm = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
  return ghostForWaypt(c, rm->currentWaypt());
}

static naRef f_route_nextWP(naContext c, naRef me, int argc, naRef* args)
{
  FGRouteMgr* rm = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
  flightgear::WayptRef wp = rm->nextWaypt();
  if (!wp) {
    return naNil();
  }
  return ghostForWaypt(c, wp);
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

static naRef f_waypoint_navaid(naContext c, naRef me, int argc, naRef* args)
{
  flightgear::Waypt* w = wayptGhost(me);
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
    return ghostForNavaid(c, nav);
  }
      
  default:
    return naNil();
  }
}

static naRef f_waypoint_airport(naContext c, naRef me, int argc, naRef* args)
{
  flightgear::Waypt* w = wayptGhost(me);
  if (!w) {
    naRuntimeError(c, "waypoint.navaid called on non-waypoint object");
  }
  
  FGPositioned* pos = w->source();
  if (!pos || FGAirport::isAirportType(pos)) {
    return naNil();
  }
  
  return ghostForAirport(c, (FGAirport*) pos);
}

static naRef f_waypoint_runway(naContext c, naRef me, int argc, naRef* args)
{
  flightgear::Waypt* w = wayptGhost(me);
  if (!w) {
    naRuntimeError(c, "waypoint.navaid called on non-waypoint object");
  }
  
  FGPositioned* pos = w->source();
  if (!pos || (pos->type() != FGPositioned::RUNWAY)) {
    return naNil();
  }
  
  return ghostForRunway(c, (FGRunway*) pos);
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
  { "greatCircleMove", f_greatCircleMove },
  { "tileIndex", f_tileIndex },
  { "tilePath", f_tilePath },
  { 0, 0 }
};


naRef initNasalPositioned(naRef globals, naContext c, naRef gcSave)
{
    airportPrototype = naNewHash(c);
    hashset(c, gcSave, "airportProto", airportPrototype);
  
    hashset(c, airportPrototype, "runway", naNewFunc(c, naNewCCode(c, f_airport_runway)));
    hashset(c, airportPrototype, "tower", naNewFunc(c, naNewCCode(c, f_airport_tower)));
    hashset(c, airportPrototype, "comms", naNewFunc(c, naNewCCode(c, f_airport_comms)));
    hashset(c, airportPrototype, "sids", naNewFunc(c, naNewCCode(c, f_airport_sids)));
    hashset(c, airportPrototype, "stars", naNewFunc(c, naNewCCode(c, f_airport_stars)));
    hashset(c, airportPrototype, "parking", naNewFunc(c, naNewCCode(c, f_airport_parking)));
  
    routePrototype = naNewHash(c);
    hashset(c, gcSave, "routeProto", routePrototype);
      
    hashset(c, routePrototype, "getWP", naNewFunc(c, naNewCCode(c, f_route_getWP)));
    hashset(c, routePrototype, "currentWP", naNewFunc(c, naNewCCode(c, f_route_currentWP))); 
    hashset(c, routePrototype, "nextWP", naNewFunc(c, naNewCCode(c, f_route_nextWP))); 
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

void postinitNasalPositioned(naRef globals, naContext c)
{
  naRef geoModule = naHash_cget(globals, (char*) "geo");
  if (naIsNil(geoModule)) {
    SG_LOG(SG_GENERAL, SG_WARN, "postinitNasalPositioned: geo.nas not loaded");
    return;
  }
  
  geoCoordClass = naHash_cget(geoModule, (char*) "Coord");
}


