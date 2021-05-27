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

#include "config.h"

#include <cstring>
#include <algorithm>

#include "NasalPositioned.hxx"

#include <simgear/sg_inlines.h>
#include <simgear/scene/material/mat.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/bucket/newbucket.hxx>

#include <Airports/runways.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/parking.hxx>
#include <Scripting/NasalSys.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/procedure.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <Scenery/scenery.hxx>
#include <ATC/CommStation.hxx>
#include <Navaids/fix.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Navaids/NavDataCache.hxx>

#include "NasalFlightPlan.hxx"

using namespace flightgear;

static void positionedGhostDestroy(void* g);

///static naGhostType PositionedGhostType = { positionedGhostDestroy, "positioned", nullptr, nullptr };

static const char* airportGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static naGhostType AirportGhostType = { positionedGhostDestroy, "airport", airportGhostGetMember, nullptr };

static const char* navaidGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static naGhostType NavaidGhostType = { positionedGhostDestroy, "navaid", navaidGhostGetMember, nullptr };

static const char* runwayGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static naGhostType RunwayGhostType = { positionedGhostDestroy, "runway", runwayGhostGetMember, nullptr };
static naGhostType HelipadGhostType = { positionedGhostDestroy, "helipad", runwayGhostGetMember, nullptr };
static naGhostType TaxiwayGhostType = { positionedGhostDestroy, "taxiway", runwayGhostGetMember, nullptr };

static const char* fixGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static naGhostType FixGhostType = { positionedGhostDestroy, "fix", fixGhostGetMember, nullptr };

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

FGPositioned* positionedGhost(naRef r)
{
    if ((naGhost_type(r) == &AirportGhostType) ||
        (naGhost_type(r) == &NavaidGhostType) ||
        (naGhost_type(r) == &RunwayGhostType) ||
        (naGhost_type(r) == &FixGhostType))
    {
        return (FGPositioned*) naGhost_ptr(r);
    }

    return 0;
}

FGAirport* airportGhost(naRef r)
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

FGRunway* runwayGhost(naRef r)
{
  if (naGhost_type(r) == &RunwayGhostType)
    return (FGRunway*) naGhost_ptr(r);
  return 0;
}

static FGTaxiway* taxiwayGhost(naRef r)
{
  if (naGhost_type(r) == &TaxiwayGhostType)
    return (FGTaxiway*) naGhost_ptr(r);
  return 0;
}

static FGFix* fixGhost(naRef r)
{
  if (naGhost_type(r) == &FixGhostType)
    return (FGFix*) naGhost_ptr(r);
  return 0;
}


static void positionedGhostDestroy(void* g)
{
    FGPositioned* pos = (FGPositioned*)g;
    if (!FGPositioned::put(pos)) // unref
        delete pos;
}


static naRef airportPrototype;
static naRef geoCoordClass;

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

naRef ghostForHelipad(naContext c, const FGHelipad* r)
{
  if (!r) {
    return naNil();
  }

  FGPositioned::get(r); // take a ref
  return naNewGhost2(c, &HelipadGhostType, (void*) r);
}

naRef ghostForTaxiway(naContext c, const FGTaxiway* r)
{
  if (!r) {
    return naNil();
  }

  FGPositioned::get(r); // take a ref
  return naNewGhost2(c, &TaxiwayGhostType, (void*) r);
}

naRef ghostForFix(naContext c, const FGFix* r)
{
  if (!r) {
    return naNil();
  }

  FGPositioned::get(r); // take a ref
  return naNewGhost2(c, &FixGhostType, (void*) r);
}

naRef ghostForPositioned(naContext c, FGPositionedRef pos)
{
    if (!pos) {
        return naNil();
    }

    switch (pos->type()) {
    case FGPositioned::VOR:
    case FGPositioned::NDB:
    case FGPositioned::TACAN:
    case FGPositioned::DME:
    case FGPositioned::ILS:
        return ghostForNavaid(c, fgpositioned_cast<FGNavRecord>(pos));
    case FGPositioned::FIX:
        return ghostForFix(c, fgpositioned_cast<FGFix>(pos));
    case FGPositioned::HELIPAD:
        return ghostForHelipad(c, fgpositioned_cast<FGHelipad>(pos));
    case FGPositioned::RUNWAY:
        return ghostForRunway(c, fgpositioned_cast<FGRunway>(pos));
    default:
        SG_LOG(SG_NASAL, SG_DEV_ALERT, "Type lacks Nasal ghost mapping:" << pos->typeString());
        return naNil();
    }
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
    double minLengthFt = fgGetDouble("/sim/navdb/min-runway-length-ft");
    for(unsigned int r=0; r<apt->numRunways(); ++r) {
      FGRunway* rwy(apt->getRunwayByIndex(r));
      // ignore unusably short runways
      if (rwy->lengthFt() < minLengthFt) {
        continue;
      }
      naRef rwyid = stringToNasal(c, rwy->ident());
      naRef rwydata = ghostForRunway(c, rwy);
      naHash_set(*out, rwyid, rwydata);
    }
  } else if (!strcmp(fieldName, "helipads")) {
    *out = naNewHash(c);

    for(unsigned int r=0; r<apt->numHelipads(); ++r) {
      FGHelipad* hp(apt->getHelipadByIndex(r));

      naRef rwyid = stringToNasal(c, hp->ident());
      naRef rwydata = ghostForHelipad(c, hp);
      naHash_set(*out, rwyid, rwydata);
    }

  } else if (!strcmp(fieldName, "taxiways")) {
    *out = naNewVector(c);
    for(unsigned int r=0; r<apt->numTaxiways(); ++r) {
      FGTaxiway* taxi(apt->getTaxiwayByIndex(r));
      naRef taxidata = ghostForTaxiway(c, taxi);
      naVec_append(*out, taxidata);
    }

  } else {
    return 0;
  }

  return "";
}

static const char* runwayGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  FGRunwayBase* base = (FGRunwayBase*) g;

  if (!strcmp(fieldName, "id")) *out = stringToNasal(c, base->ident());
  else if (!strcmp(fieldName, "lat")) *out = naNum(base->latitude());
  else if (!strcmp(fieldName, "lon")) *out = naNum(base->longitude());
  else if (!strcmp(fieldName, "heading")) *out = naNum(base->headingDeg());
  else if (!strcmp(fieldName, "length")) *out = naNum(base->lengthM());
  else if (!strcmp(fieldName, "width")) *out = naNum(base->widthM());
  else if (!strcmp(fieldName, "surface")) *out = naNum(base->surface());
  else if (base->type() == FGRunwayBase::RUNWAY) {
    FGRunway* rwy = (FGRunway*) g;
    if (!strcmp(fieldName, "threshold")) *out = naNum(rwy->displacedThresholdM());
    else if (!strcmp(fieldName, "stopway")) *out = naNum(rwy->stopwayM());
    else if (!strcmp(fieldName, "reciprocal")) {
      *out = ghostForRunway(c, rwy->reciprocalRunway());
    } else if (!strcmp(fieldName, "ils_frequency_mhz")) {
      *out = rwy->ILS() ? naNum(rwy->ILS()->get_freq() / 100.0) : naNil();
    } else if (!strcmp(fieldName, "ils")) {
      *out = ghostForNavaid(c, rwy->ILS());
    } else {
      return 0;
    }
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
  } else if (!strcmp(fieldName, "magvar")) {
    if (nav->type() == FGPositioned::VOR) {
      // For VORs, the multiuse function provides the magnetic variation
      double variation = nav->get_multiuse();
      SG_NORMALIZE_RANGE(variation, 0.0, 360.0);
      *out = naNum(variation);
    } else {
      *out = naNil();
    }
  } else if (!strcmp(fieldName, "colocated_dme")) {
      FGNavRecordRef dme = FGPositioned::loadById<FGNavRecord>(nav->colocatedDME());
      if (dme) {
          *out = ghostForNavaid(c, dme);
      } else {
          *out = naNil();
      }
  } else if (!strcmp(fieldName, "dme")) {
    *out = naNum(nav->hasDME());
  } else if (!strcmp(fieldName, "vortac")) {
    *out = naNum(nav->isVORTAC());
  } else if (!strcmp(fieldName, "course")) {
    if ((nav->type() == FGPositioned::ILS) || (nav->type() == FGPositioned::LOC)) {
      double radial = nav->get_multiuse();
      SG_NORMALIZE_RANGE(radial, 0.0, 360.0);
      *out = naNum(radial);
    } else {
      *out = naNil();
    }
  } else if (!strcmp(fieldName, "guid")) {
      *out = naNum(nav->guid());
  } else {
    return 0;
  }

  return "";
}

static const char* fixGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  FGFix* fix = (FGFix*) g;

  if (!strcmp(fieldName, "id")) *out = stringToNasal(c, fix->ident());
  else if (!strcmp(fieldName, "lat")) *out = naNum(fix->get_lat());
  else if (!strcmp(fieldName, "lon")) *out = naNum(fix->get_lon());
    // for homogenity with other values returned by navinfo()
  else if (!strcmp(fieldName, "type")) *out = stringToNasal(c, "fix");
  else if (!strcmp(fieldName, "name")) *out = stringToNasal(c, fix->ident());
  else {
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

  return naEqual(naVec_get(parents, 0), geoCoordClass) != 0;
}

bool geodFromHash(naRef ref, SGGeod& result)
{
  if (!naIsHash(ref)) {
    return false;
  }


// check for manual latitude / longitude names
  naRef lat = naHash_cget(ref, (char*) "lat");
  naRef lon = naHash_cget(ref, (char*) "lon");
  if (naIsNum(lat) && naIsNum(lon)) {
    result = SGGeod::fromDeg(naNumValue(lon).num, naNumValue(lat).num);
    return true;
  }

  if (hashIsCoord(ref)) {
    naRef lat = naHash_cget(ref, (char*) "_lat");
    naRef lon = naHash_cget(ref, (char*) "_lon");
    naRef alt_feet = naHash_cget(ref, (char*) "_alt");
    if (naIsNum(lat) && naIsNum(lon) && naIsNil(alt_feet)) {
        result = SGGeod::fromRad(naNumValue(lon).num, naNumValue(lat).num);
        return true;
    }
    if (naIsNum(lat) && naIsNum(lon) && naIsNum(alt_feet)) {
        result = SGGeod::fromRadFt(naNumValue(lon).num, naNumValue(lat).num, naNumValue(alt_feet).num);
        return true;
    }
  }
// check for any synonyms?
  // latitude + longitude?

  return false;
}

int geodFromArgs(naRef* args, int offset, int argc, SGGeod& result)
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

    if (gt == &TaxiwayGhostType) {
      result = taxiwayGhost(args[offset])->geod();
      return 1;
    }

    if (gt == &FixGhostType) {
      result = fixGhost(args[offset])->geod();
      return 1;
    }

    auto wp = wayptGhost(args[offset]);
    if (wp) {
      result = wp->position();
      return 1;
    }

    auto leg = fpLegGhost(args[offset]);
    if (leg) {
      result = leg->waypoint()->position();
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

bool vec3dFromHash(naRef ref, SGVec3d& result)
{
    if (!naIsHash(ref)) {
        return false;
    }

    // check for manual latitude / longitude names
    naRef x = naHash_cget(ref, (char*) "x");
    naRef y = naHash_cget(ref, (char*) "y");
    naRef z = naHash_cget(ref, (char*) "z");
    if (naIsNum(x) && naIsNum(y) && naIsNum(z)) {
        result = SGVec3d(naNumValue(x).num, naNumValue(y).num, naNumValue(z).num);
        return true;
    }
    return false;
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

/**
* @name    f_get_cart_ground_intersection
* @brief   Returns where the given position in the specified direction will intersect with the ground
*
* Exposes the built in function to Nasal to allow a craft to ascertain
* whether or not a certain position and direction pair intersect with
* the ground.
*
* Useful for radars, terrain avoidance (GPWS), etc.
*
* @param [in] vec3d(x,y,z) position
* @param [in] vec3d(x,y,z) direction
*
* @retval geod hash (lat:rad,lon:rad,elevation:Meters) intersection
* @retval nil  no intersection found.
*
* Example Usage:
* @code
*     var end = geo.Coord.new(start);
*     end.apply_course_distance(heading, speed_horz_fps*FT2M);
*     end.set_alt(end.alt() - speed_down_fps*FT2M);
*
*     var dir_x = end.x() - start.x();
*     var dir_y = end.y() - start.y();
*     var dir_z = end.z() - start.z();
*     var xyz = { "x":start.x(),  "y" : start.y(),  "z" : start.z() };
*     var dir = { "x":dir_x,      "y" : dir_y,      "z" : dir_z };
*
*     var geod = get_cart_ground_intersection(xyz, dir);
*     if (geod != nil) {
*         end.set_latlon(geod.lat, geod.lon, geod.elevation);
          var dist = start.direct_distance_to(end)*M2FT;
*         var time = dist / speed_fps;
*         setprop("/sim/model/radar/time-until-impact", time);
*     }
* @endcode
*/
static naRef f_get_cart_ground_intersection(naContext c, naRef me, int argc, naRef* args)
{
    SGVec3d dir;
    SGVec3d pos;

    if (argc != 2)
        naRuntimeError(c, "geod_hash get_cart_ground_intersection(position: hash{x,y,z}, direction:hash{x,y,z}) expects 2 arguments");

    if (!vec3dFromHash(args[0], pos))
        naRuntimeError(c, "geod_hash get_cart_ground_intersection(position:hash{x,y,z}, direction:hash{x,y,z}) expects argument(0) to be hash of position containing x,y,z");

    if (!vec3dFromHash(args[1], dir))
        naRuntimeError(c, "geod_hash get_cart_ground_intersection(position: hash{x,y,z}, direction:hash{x,y,z}) expects argument(1) to be hash of direction containing x,y,z");

    SGVec3d nearestHit;
    if (!globals->get_scenery()->get_cart_ground_intersection(pos, dir, nearestHit))
        return naNil();

    const SGGeod geodHit = SGGeod::fromCart(nearestHit);

    // build a hash for returned intersection
    naRef intersection_h = naNewHash(c);
    hashset(c, intersection_h, "lat", naNum(geodHit.getLatitudeDeg()));
    hashset(c, intersection_h, "lon", naNum(geodHit.getLongitudeDeg()));
    hashset(c, intersection_h, "elevation", naNum(geodHit.getElevationM()));
    return intersection_h;
}

// convert from aircraft reference frame to global (ECEF) cartesian
static naRef f_aircraftToCart(naContext c, naRef me, int argc, naRef* args)
{
    if (argc != 1)
        naRuntimeError(c, "hash{x,y,z} aircraftToCart(position: hash{x,y,z}) expects one argument");

    SGVec3d offset;
    if (!vec3dFromHash(args[0], offset))
        naRuntimeError(c, "aircraftToCart expects argument(0) to be a hash containing x,y,z");

    double heading, pitch, roll;
    globals->get_aircraft_orientation(heading, pitch, roll);

    // Transform that one to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(globals->get_aircraft_position());

    // post-rotate the orientation of the aircraft wrt the horizontal local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(heading, pitch, roll);

    // The offset converted to the usual body fixed coordinate system
    // rotated to the earth fiexed coordinates axis
    offset = hlTrans.backTransform(offset);

    SGVec3d v = globals->get_aircraft_position_cart() + offset;

    // build a hash for returned location
    naRef pos_h = naNewHash(c);
    hashset(c, pos_h, "x", naNum(v.x()));
    hashset(c, pos_h, "y", naNum(v.y()));
    hashset(c, pos_h, "z", naNum(v.z()));
    return pos_h;
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
  const simgear::BVHMaterial *material;
  SGGeod geod = SGGeod::fromDegM(lon, lat, elev);

  const auto scenery = globals->get_scenery();
  if (scenery == nullptr)
    return naNil();

  if(!scenery->get_elevation_m(geod, elev, &material)) {
      return naNil();
  }

  naRef vec = naNewVector(c);
  naVec_append(vec, naNum(elev));

  naRef matdata = naNil();

  const SGMaterial *mat = dynamic_cast<const SGMaterial *>(material);
  if(mat) {
    matdata = naNewHash(c);
    naRef names = naNewVector(c);
    for (const std::string& n : mat->get_names())
      naVec_append(names, stringToNasal(c, n));

    HASHSET("region", 6, stringToNasal(c, mat->get_region_name()));
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

  FGAirport::TypeRunwayFilter filter; // defaults to airports only

  if(argc == 0) {
    // fall through and use AIRPORT
  } else if(argc == 1 && naIsString(args[0])) {
    if (filter.fromTypeString(naStr_data(args[0]))) {
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

  FGAirport::TypeRunwayFilter filter; // defaults to airports only
  double rangeNm = args[argOffset++].num;
  if (argOffset < argc) {
    filter.fromTypeString(naStr_data(args[argOffset++]));
  }

  naRef r = naNewVector(c);

  FGPositionedList apts = FGPositioned::findWithinRange(pos, rangeNm, &filter);
  FGPositioned::sortByRange(apts, pos);

  for (FGPositionedRef a : apts) {
    naVec_append(r, ghostForAirport(c, fgpositioned_cast<FGAirport>(a)));
  }

  return r;
}

static naRef f_findAirportsByICAO(naContext c, naRef me, int argc, naRef* args)
{
  if (!naIsString(args[0])) {
    naRuntimeError(c, "findAirportsByICAO expects string as arg 0");
  }

  int argOffset = 0;
  std::string prefix(naStr_data(args[argOffset++]));
  FGAirport::TypeRunwayFilter filter; // defaults to airports only
  if (argOffset < argc) {
    filter.fromTypeString(naStr_data(args[argOffset++]));
  }

  naRef r = naNewVector(c);

  FGPositionedList apts = FGPositioned::findAllWithIdent(prefix, &filter, false);
  for (FGPositionedRef a : apts) {
    naVec_append(r, ghostForAirport(c, fgpositioned_cast<FGAirport>(a)));
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
    if (argc > 0 && !naIsString(args[0])) {
        naRuntimeError(c, "airport.comms argument must be a frequency type name");
    }

    if (argc > 0) {
        std::string commName = naStr_data(args[0]);
        FGPositioned::Type commType = FGPositioned::typeFromName(commName);

        for (auto comm : apt->commStationsOfType(commType)) {
            naVec_append(comms, naNum(comm->freqMHz()));
        }
    } else {
// otherwise return a vector of hashes, one for each comm station.
        for (auto comm : apt->commStations()) {
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

  std::string ident = simgear::strutils::uppercase(naStr_data(args[0]));

  if (apt->hasRunwayWithIdent(ident)) {
    return ghostForRunway(c, apt->getRunwayByIdent(ident));
  } else if (apt->hasHelipadWithIdent(ident)) {
    return ghostForHelipad(c, apt->getHelipadByIdent(ident));
  }
  return naNil();
}

static naRef f_airport_runwaysWithoutReciprocals(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.runwaysWithoutReciprocals called on non-airport object");
  }

  FGRunwayList rwylist(apt->getRunwaysWithoutReciprocals());
  naRef runways = naNewVector(c);
  for (unsigned int r=0; r<rwylist.size(); ++r) {
    FGRunway* rwy(rwylist[r]);
    naVec_append(runways, ghostForRunway(c, apt->getRunwayByIdent(rwy->ident())));
  }
  return runways;
}

static naRef f_airport_sids(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.sids called on non-airport object");
  }

  naRef sids = naNewVector(c);

  FGRunway* rwy = NULL;
  if (argc > 0 && naIsString(args[0])) {
    if (!apt->hasRunwayWithIdent(naStr_data(args[0]))) {
      return naNil();
    }

    rwy = apt->getRunwayByIdent(naStr_data(args[0]));
  } else if (argc > 0) {
    rwy = runwayGhost(args[0]);
  }

  if (rwy) {
    for (auto sid : rwy->getSIDs()) {
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

  FGRunway* rwy = NULL;
  if (argc > 0 && naIsString(args[0])) {
    if (!apt->hasRunwayWithIdent(naStr_data(args[0]))) {
      return naNil();
    }

    rwy = apt->getRunwayByIdent(naStr_data(args[0]));
  } else if (argc > 0) {
    rwy = runwayGhost(args[0]);
  }

  if (rwy) {
    for (flightgear::STAR* s : rwy->getSTARs()) {
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

static naRef f_airport_approaches(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.getApproachList called on non-airport object");
  }

  naRef approaches = naNewVector(c);

  ProcedureType ty = PROCEDURE_INVALID;
  if ((argc > 1) && naIsString(args[1])) {
    std::string u = simgear::strutils::uppercase(naStr_data(args[1]));
    if (u == "NDB") ty = PROCEDURE_APPROACH_NDB;
    if (u == "VOR") ty = PROCEDURE_APPROACH_VOR;
    if (u == "ILS") ty = PROCEDURE_APPROACH_ILS;
    if (u == "RNAV") ty = PROCEDURE_APPROACH_RNAV;
  }

  FGRunway* rwy = NULL;
  STAR* star = nullptr;
  if (argc > 0 && (rwy = runwayGhost(args[0]))) {
    // ok
  } else if (argc > 0 && (procedureGhost(args[0]))) {
      Procedure* proc = procedureGhost(args[0]);
      if (proc->type() != PROCEDURE_STAR)
          return naNil();
      star = static_cast<STAR*>(proc);
  } else if (argc > 0 && naIsString(args[0])) {
    if (!apt->hasRunwayWithIdent(naStr_data(args[0]))) {
      return naNil();
    }

    rwy = apt->getRunwayByIdent(naStr_data(args[0]));
  }

  if (rwy) {
    for (Approach* s : rwy->getApproaches()) {
      if ((ty != PROCEDURE_INVALID) && (s->type() != ty)) {
        continue;
      }

      naRef procId = stringToNasal(c, s->ident());
      naVec_append(approaches, procId);
    }
  } else if (star) {
      std::set<std::string> appIds;
      for (auto rwy : star->runways()) {
          for (auto app : rwy->getApproaches()) {
              appIds.insert(app->ident());
          }
      }

      for (auto s : appIds) {
          naVec_append(approaches, stringToNasal(c, s));
      }
  } else {
    // no runway specified, report them all
    RunwayVec runways;
    if (star)
        runways = star->runways();

    for (unsigned int s=0; s<apt->numApproaches(); ++s) {
      Approach* app = apt->getApproachByIndex(s);
      if ((ty != PROCEDURE_INVALID) && (app->type() != ty)) {
        continue;
      }

      naRef procId = stringToNasal(c, app->ident());
      naVec_append(approaches, procId);
    }
  }

  return approaches;
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

  FGAirportDynamicsRef dynamics = apt->getDynamics();
  FGParkingList parkings = dynamics->getParkings(onlyAvailable, type);
  FGParkingList::const_iterator it;
  for (it = parkings.begin(); it != parkings.end(); ++it) {
    FGParkingRef park = *it;
    const SGGeod& parkLoc = park->geod();
    naRef ph = naNewHash(c);
    hashset(c, ph, "name", stringToNasal(c, park->getName()));
    hashset(c, ph, "lat", naNum(parkLoc.getLatitudeDeg()));
    hashset(c, ph, "lon", naNum(parkLoc.getLongitudeDeg()));
    hashset(c, ph, "elevation", naNum(parkLoc.getElevationM()));
    naVec_append(r, ph);
  }

  return r;
}

static naRef f_airport_getSid(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.getSid called on non-airport object");
  }

  if ((argc != 1) || !naIsString(args[0])) {
    naRuntimeError(c, "airport.getSid passed invalid argument");
  }

  std::string ident = naStr_data(args[0]);
  return ghostForProcedure(c, apt->findSIDWithIdent(ident));
}

static naRef f_airport_getStar(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.getStar called on non-airport object");
  }

  if ((argc != 1) || !naIsString(args[0])) {
    naRuntimeError(c, "airport.getStar passed invalid argument");
  }

  std::string ident = naStr_data(args[0]);
  return ghostForProcedure(c, apt->findSTARWithIdent(ident));
}

static naRef f_airport_getApproach(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.getIAP called on non-airport object");
  }

  if ((argc != 1) || !naIsString(args[0])) {
    naRuntimeError(c, "airport.getIAP passed invalid argument");
  }

  std::string ident = naStr_data(args[0]);
  return ghostForProcedure(c, apt->findApproachWithIdent(ident));
}

static naRef f_airport_findBestRunway(naContext c, naRef me, int argc, naRef* args)
{
    FGAirport* apt = airportGhost(me);
    if (!apt) {
        naRuntimeError(c, "findBestRunway called on non-airport object");
    }

    SGGeod pos;
    if (!geodFromArgs(args, 0, argc, pos)) {
        naRuntimeError(c, "findBestRunway must be passed a position");
    }

    return ghostForRunway(c,  apt->findBestRunwayForPos(pos));
}

static naRef f_airport_toString(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.tostring called on non-airport object");
  }

  return stringToNasal(c, "an airport " + apt->ident());
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

  FGNavList::TypeFilter filter(type);
  navlist = FGNavList::findByIdentAndFreq( pos, id, 0.0, &filter );

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
  FGPositionedList navs = FGPositioned::findWithinRange(pos, rangeNm, &filter);
  FGPositioned::sortByRange(navs, pos);

  for (FGPositionedRef a : navs) {
    FGNavRecord* nav = (FGNavRecord*) a.get();
    naVec_append(r, ghostForNavaid(c, nav));
  }

  return r;
}

static naRef f_findNDBByFrequency(naContext c, naRef me, int argc, naRef* args)
{
  int argOffset = 0;
  SGGeod pos = globals->get_aircraft_position();
  argOffset += geodFromArgs(args, 0, argc, pos);

  if (!naIsNum(args[argOffset])) {
    naRuntimeError(c, "findNDBByFrquency expectes frequency (in kHz) as arg %d", argOffset);
  }

  double dbFreq = args[argOffset++].num;
  FGNavList::TypeFilter filter(FGPositioned::NDB);
  nav_list_type navs = FGNavList::findAllByFreq(dbFreq, pos, &filter);
  if (navs.empty()) {
    return naNil();
  }

  return ghostForNavaid(c, navs.front().ptr());
}

static naRef f_findNDBsByFrequency(naContext c, naRef me, int argc, naRef* args)
{
  int argOffset = 0;
  SGGeod pos = globals->get_aircraft_position();
  argOffset += geodFromArgs(args, 0, argc, pos);

  if (!naIsNum(args[argOffset])) {
    naRuntimeError(c, "findNDBsByFrquency expectes frequency (in kHz) as arg %d", argOffset);
  }

  double dbFreq = args[argOffset++].num;
  FGNavList::TypeFilter filter(FGPositioned::NDB);
  nav_list_type navs = FGNavList::findAllByFreq(dbFreq, pos, &filter);
  if (navs.empty()) {
    return naNil();
  }

  naRef r = naNewVector(c);
  for (nav_rec_ptr a : navs) {
    naVec_append(r, ghostForNavaid(c, a.ptr()));
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
    if (type == FGPositioned::NDB) {
      naRuntimeError(c, "Use findNDBByFrquency to seach NDBs");
    }
  }

  FGNavList::TypeFilter filter(type);
  auto navs = FGNavList::findAllByFreq(freqMhz, pos, &filter);
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
    if (type == FGPositioned::NDB) {
      naRuntimeError(c, "Use findNDBsByFrquency to seach NDBs");
    }
  }

  naRef r = naNewVector(c);
  FGNavList::TypeFilter filter(type);
  auto navs = FGNavList::findAllByFreq(freqMhz, pos, &filter);
  for (nav_rec_ptr a : navs) {
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
  std::string ident = naStr_data(args[argOffset++]);
  if (argOffset < argc) {
    type = FGPositioned::typeFromName(naStr_data(args[argOffset]));
  }

  FGNavList::TypeFilter filter(type);
  naRef r = naNewVector(c);
  nav_list_type navs = FGNavList::findByIdentAndFreq(pos, ident, 0.0, &filter);

  for (nav_rec_ptr a : navs) {
    naVec_append(r, ghostForNavaid(c, a.ptr()));
  }

  return r;
}

static naRef f_findFixesByIdent(naContext c, naRef me, int argc, naRef* args)
{
  int argOffset = 0;
  SGGeod pos = globals->get_aircraft_position();
  argOffset += geodFromArgs(args, 0, argc, pos);

  if (!naIsString(args[argOffset])) {
    naRuntimeError(c, "findFixesByIdent expectes ident string as arg %d", argOffset);
  }

  std::string ident(naStr_data(args[argOffset]));
  naRef r = naNewVector(c);

  FGPositioned::TypeFilter filter(FGPositioned::FIX);
  FGPositionedList fixes = FGPositioned::findAllWithIdent(ident, &filter);
  FGPositioned::sortByRange(fixes, pos);

  for (FGPositionedRef f : fixes) {
    naVec_append(r, ghostForFix(c, (FGFix*) f.ptr()));
  }

  return r;
}

static naRef f_findByIdent(naContext c, naRef me, int argc, naRef* args)
{
    if ((argc < 2) || !naIsString(args[0]) || !naIsString(args[1]) ) {
        naRuntimeError(c, "finxByIdent: expects ident and type as first two args");
    }

    std::string ident(naStr_data(args[0]));
    std::string typeSpec(naStr_data(args[1]));

    // optional specify search pos as final argument
    SGGeod pos = globals->get_aircraft_position();
    geodFromArgs(args, 2, argc, pos);
    FGPositioned::TypeFilter filter(FGPositioned::TypeFilter::fromString(typeSpec));

    naRef r = naNewVector(c);

    FGPositionedList matches = FGPositioned::findAllWithIdent(ident, &filter);
    FGPositioned::sortByRange(matches, pos);

    for (auto f : matches) {
        naVec_append(r, ghostForPositioned(c, f));
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

static naRef f_formatLatLon(naContext c, naRef me, int argc, naRef* args)
{
  SGGeod p;
  int argOffset = geodFromArgs(args, 0, argc, p);
  if (argOffset == 0) {
    naRuntimeError(c, "invalid arguments to formatLatLon, expect a geod or lat,lon");
  }

  simgear::strutils::LatLonFormat format =
    static_cast<simgear::strutils::LatLonFormat>(fgGetInt("/sim/lon-lat-format"));
  if (argOffset < argc && naIsNum(args[argOffset])) {
    format = static_cast<simgear::strutils::LatLonFormat>((int) args[argOffset].num);
    if (format > simgear::strutils::LatLonFormat::DECIMAL_DEGREES_SYMBOL) {
      naRuntimeError(c, "invalid lat-lon format requested");
    }
  }

  const auto s = simgear::strutils::formatGeodAsString(p, format);
  return stringToNasal(c, s);
}

static naRef f_parseStringAsLatLonValue(naContext c, naRef me, int argc, naRef* args)
{
  if ((argc < 1) || !naIsString(args[0])) {
    naRuntimeError(c, "Missing / bad argument to parseStringAsLatLonValue");
  }

  double value;
  bool ok = simgear::strutils::parseStringAsLatLonValue(naStr_data(args[0]), value);
  if (!ok) {
    return naNil();
  }

  return naNum(value);
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


void shutdownNasalPositioned()
{
}

FGPositionedRef positionedFromArg(naRef ref)
{
  if (!naIsGhost(ref))
    return {};

  naGhostType* gt = naGhost_type(ref);
  if (gt == &AirportGhostType)
    return airportGhost(ref);

  if (gt == &NavaidGhostType)
    return navaidGhost(ref);

  if (gt == &RunwayGhostType)
    return runwayGhost(ref);

  if (gt == &TaxiwayGhostType)
    return taxiwayGhost(ref);

  if (gt == &FixGhostType)
    return fixGhost(ref);

  auto wpg = wayptGhost(ref);
  if (wpg)
      return wpg->source();

  return {};
}

// Table of extension functions.  Terminate with zeros.
static struct { const char* name; naCFunction func; } funcs[] = {
  { "carttogeod", f_carttogeod },
  { "geodtocart", f_geodtocart },
  { "geodinfo", f_geodinfo },
  { "formatLatLon", f_formatLatLon },
  { "parseStringAsLatLonValue", f_parseStringAsLatLonValue},
  { "get_cart_ground_intersection", f_get_cart_ground_intersection },
  { "aircraftToCart", f_aircraftToCart },
  { "airportinfo", f_airportinfo },
  { "findAirportsWithinRange", f_findAirportsWithinRange },
  { "findAirportsByICAO", f_findAirportsByICAO },
  { "navinfo", f_navinfo },
  { "findNavaidsWithinRange", f_findNavaidsWithinRange },
  { "findNDBByFrequencyKHz", f_findNDBByFrequency },
  { "findNDBsByFrequencyKHz", f_findNDBsByFrequency },
  { "findNavaidByFrequencyMHz", f_findNavaidByFrequency },
  { "findNavaidsByFrequencyMHz", f_findNavaidsByFrequency },
  { "findNavaidsByID", f_findNavaidsByIdent },
  { "findFixesByID", f_findFixesByIdent },
  { "findByIdent", f_findByIdent },
  { "magvar", f_magvar },
  { "courseAndDistance", f_courseAndDistance },
  { "greatCircleMove", f_greatCircleMove },
  { "tileIndex", f_tileIndex },
  { "tilePath", f_tilePath },
  { 0, 0 }
};


naRef initNasalPositioned(naRef globals, naContext c)
{
    airportPrototype = naNewHash(c);
    naSave(c, airportPrototype);

    hashset(c, airportPrototype, "runway", naNewFunc(c, naNewCCode(c, f_airport_runway)));
    hashset(c, airportPrototype, "runwaysWithoutReciprocals", naNewFunc(c, naNewCCode(c, f_airport_runwaysWithoutReciprocals)));
    hashset(c, airportPrototype, "helipad", naNewFunc(c, naNewCCode(c, f_airport_runway)));
    hashset(c, airportPrototype, "tower", naNewFunc(c, naNewCCode(c, f_airport_tower)));
    hashset(c, airportPrototype, "comms", naNewFunc(c, naNewCCode(c, f_airport_comms)));
    hashset(c, airportPrototype, "sids", naNewFunc(c, naNewCCode(c, f_airport_sids)));
    hashset(c, airportPrototype, "stars", naNewFunc(c, naNewCCode(c, f_airport_stars)));
    hashset(c, airportPrototype, "getApproachList", naNewFunc(c, naNewCCode(c, f_airport_approaches)));
    hashset(c, airportPrototype, "parking", naNewFunc(c, naNewCCode(c, f_airport_parking)));
    hashset(c, airportPrototype, "getSid", naNewFunc(c, naNewCCode(c, f_airport_getSid)));
    hashset(c, airportPrototype, "getStar", naNewFunc(c, naNewCCode(c, f_airport_getStar)));

    naRef approachFunc = naNewFunc(c, naNewCCode(c, f_airport_getApproach));

    // allow this to be used under either name
    hashset(c, airportPrototype, "getIAP", approachFunc);
    hashset(c, airportPrototype, "getApproach", approachFunc);

    hashset(c, airportPrototype, "findBestRunwayForPos", naNewFunc(c, naNewCCode(c, f_airport_findBestRunway)));
    hashset(c, airportPrototype, "tostring", naNewFunc(c, naNewCCode(c, f_airport_toString)));

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
