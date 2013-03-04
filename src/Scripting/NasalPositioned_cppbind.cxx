// NasalPositioned_cppbind.cxx -- expose FGPositioned classes to Nasal
//
// Port of NasalPositioned.cpp to the new nasal/cppbind helpers. Will replace
// old NasalPositioned.cpp once finished.
//
// Copyright (C) 2013  Thomas Geymayer <tomgey@gmail.com>
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

#include "NasalPositioned.hxx"

#include <algorithm>
#include <functional>

#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <ATC/CommStation.hxx>
#include <Navaids/NavDataCache.hxx>

typedef nasal::Ghost<FGPositionedRef> NasalPositioned;
typedef nasal::Ghost<FGRunwayRef> NasalRunway;
typedef nasal::Ghost<FGParkingRef> NasalParking;
typedef nasal::Ghost<FGAirportRef> NasalAirport;
typedef nasal::Ghost<flightgear::CommStationRef> NasalCommStation;

//------------------------------------------------------------------------------
naRef to_nasal_helper(naContext c, FGPositioned* positioned)
{
  return NasalPositioned::create(c, positioned);
}

//------------------------------------------------------------------------------
naRef to_nasal_helper(naContext c, FGPavement* rwy)
{
  return NasalPositioned::create(c, (FGPositioned*)rwy);
}

//------------------------------------------------------------------------------
naRef to_nasal_helper(naContext c, FGRunwayBase* rwy)
{
  return NasalPositioned::create(c, (FGPositioned*)rwy);
}

//------------------------------------------------------------------------------
naRef to_nasal_helper(naContext c, FGParking* parking)
{
  return NasalParking::create(c, parking);
}

//------------------------------------------------------------------------------
naRef to_nasal_helper(naContext c, flightgear::SID* sid)
{
  // TODO SID ghost
  return nasal::to_nasal(c, sid->ident());
}

//------------------------------------------------------------------------------
naRef to_nasal_helper(naContext c, flightgear::STAR* star)
{
  // TODO STAR ghost
  return nasal::to_nasal(c, star->ident());
}

//------------------------------------------------------------------------------
naRef to_nasal_helper(naContext c, flightgear::Approach* iap)
{
  // TODO Approach ghost
  return nasal::to_nasal(c, iap->ident());
}

//------------------------------------------------------------------------------
naRef to_nasal_helper(naContext c, FGAirport* apt)
{
  return NasalAirport::create(c, apt);
}

//------------------------------------------------------------------------------
naRef to_nasal_helper(naContext c, const SGGeod& pos)
{
  nasal::Hash hash(c);
  hash.set("lat", pos.getLatitudeDeg());
  hash.set("lon", pos.getLongitudeDeg());
  hash.set("elevation", pos.getElevationM());
  return hash.get_naRef();
}

//------------------------------------------------------------------------------
static FGRunwayBase* f_airport_runway(FGAirport& apt, std::string ident)
{
  boost::to_upper(ident);

  if( apt.hasRunwayWithIdent(ident) )
    return apt.getRunwayByIdent(ident);
  else if( apt.hasHelipadWithIdent(ident) )
    return apt.getHelipadByIdent(ident);

  return 0;
}

//------------------------------------------------------------------------------
template<class T, class C1, class C2>
std::vector<T> extract( const std::vector<C1*>& in,
                        T (C2::*getter)() const )
{
  std::vector<T> ret(in.size());
  std::transform(in.begin(), in.end(), ret.begin(), std::mem_fun(getter));
  return ret;
}

//------------------------------------------------------------------------------
static naRef f_airport_comms(FGAirport& apt, const nasal::CallContext& ctx)
{
  FGPositioned::Type comm_type =
    FGPositioned::typeFromName( ctx.getArg<std::string>(0) );

  // if we have an explicit type, return a simple vector of frequencies
  if( comm_type != FGPositioned::INVALID )
    return ctx.to_nasal
    (
      extract( apt.commStationsOfType(comm_type),
               &flightgear::CommStation::freqMHz )
    );
  else
    // otherwise return a vector of ghosts, one for each comm station.
    return ctx.to_nasal(apt.commStations());
}

//------------------------------------------------------------------------------
FGRunway* runwayFromNasalArg( const FGAirport& apt,
                              const nasal::CallContext& ctx,
                              size_t index = 0 )
{
  if( index >= ctx.argc )
    return NULL;

  try
  {
    std::string ident = ctx.getArg<std::string>(index);
    if( !ident.empty() )
    {
      if( !apt.hasRunwayWithIdent(ident) )
        // TODO warning/exception?
        return NULL;

      return apt.getRunwayByIdent(ident);
    }
  }
  catch(...)
  {}

  // TODO warn/error if no runway?
  return NasalRunway::fromNasal(ctx.c, ctx.args[index]);
}

//------------------------------------------------------------------------------
static naRef f_airport_sids(FGAirport& apt, const nasal::CallContext& ctx)
{
  FGRunway* rwy = runwayFromNasalArg(apt, ctx);
  return ctx.to_nasal
  (
    extract(rwy ? rwy->getSIDs() : apt.getSIDs(), &flightgear::SID::ident)
  );
}

//------------------------------------------------------------------------------
static naRef f_airport_stars(FGAirport& apt, const nasal::CallContext& ctx)
{
  FGRunway* rwy = runwayFromNasalArg(apt, ctx);
  return ctx.to_nasal
  (
    extract(rwy ? rwy->getSTARs() : apt.getSTARs(), &flightgear::STAR::ident)
  );
}

//------------------------------------------------------------------------------
static naRef f_airport_approaches(FGAirport& apt, const nasal::CallContext& ctx)
{
  FGRunway* rwy = runwayFromNasalArg(apt, ctx);

  flightgear::ProcedureType type = flightgear::PROCEDURE_INVALID;
  std::string type_str = ctx.getArg<std::string>(1);
  if( !type_str.empty() )
  {
    boost::to_upper(type_str);
    if(      type_str == "NDB" ) type = flightgear::PROCEDURE_APPROACH_NDB;
    else if( type_str == "VOR" ) type = flightgear::PROCEDURE_APPROACH_VOR;
    else if( type_str == "ILS" ) type = flightgear::PROCEDURE_APPROACH_ILS;
    else if( type_str == "RNAV") type = flightgear::PROCEDURE_APPROACH_RNAV;
  }

  return ctx.to_nasal
  (
    extract( rwy ? rwy->getApproaches(type)
                 // no runway specified, report them all
                 : apt.getApproaches(type),
             &flightgear::Approach::ident )
  );
}

//------------------------------------------------------------------------------
static FGParkingList
f_airport_parking(FGAirport& apt, const nasal::CallContext& ctx)
{
  std::string type = ctx.getArg<std::string>(0);
  bool only_available = ctx.getArg<bool>(1);

  FGAirportDynamics* dynamics = apt.getDynamics();
  PositionedIDVec parkings =
    flightgear::NavDataCache::instance()
      ->airportItemsOfType(apt.guid(), FGPositioned::PARKING);

  FGParkingList ret;
  BOOST_FOREACH(PositionedID parking, parkings)
  {
    // filter out based on availability and type
    if( only_available && !dynamics->isParkingAvailable(parking) )
      continue;

    FGParking* park = dynamics->getParking(parking);
    if( !type.empty() && (park->getType() != type) )
      continue;

    ret.push_back(park);
  }

  return ret;
}

//------------------------------------------------------------------------------
// Returns Nasal ghost for particular or nearest airport of a <type>, or nil
// on error. (Currently only airportinfo(<id>) is implemented)
//
// airportinfo(<id>);                   e.g. "KSFO"
// airportinfo(<type>);                 type := ("airport"|"seaport"|"heliport")
// airportinfo()                        same as  airportinfo("airport")
// airportinfo(<lat>, <lon> [, <type>]);
static naRef f_airportinfo(naContext c, naRef me, int argc, naRef* args)
{
  nasal::CallContext ctx(c, argc, args);
  // TODO think of something comfortable to overload functions or use variable
  //      number/types of arguments.
  return ctx.to_nasal(FGAirport::findByIdent( ctx.requireArg<std::string>(0) ));
}

//------------------------------------------------------------------------------
naRef initNasalPositioned_cppbind(naRef globalsRef, naContext c, naRef gcSave)
{
  NasalPositioned::init("FGPositioned")
    .member("id", &FGPositioned::ident)
    .member("ident", &FGPositioned::ident) // TODO to we really need id and ident?
    .member("name", &FGPositioned::name)
    .member("lat", &FGPositioned::latitude)
    .member("lon", &FGPositioned::longitude)
    .member("elevation", &FGPositioned::elevationM);
  NasalRunway::init("FGRunway")
    .bases<NasalPositioned>();
  NasalParking::init("FGParking")
    .bases<NasalPositioned>();
  NasalCommStation::init("CommStation")
    .bases<NasalPositioned>()
    .member("frequency", &flightgear::CommStation::freqMHz);
  NasalAirport::init("FGAirport")
    .bases<NasalPositioned>()
    .member("has_metar", &FGAirport::getMetar)
    .member("runways", &FGAirport::getRunwayMap)
    .member("helipads", &FGAirport::getHelipadMap)
    .member("taxiways", &FGAirport::getTaxiways)
    .member("pavements", &FGAirport::getPavements)
    .method("runway", &f_airport_runway)
    .method("helipad", &f_airport_runway)
    .method("tower", &FGAirport::getTowerLocation)
    .method("comms", &f_airport_comms)
    .method("sids", &f_airport_sids)
    .method("stars", &f_airport_stars)
    .method("getApproachList", f_airport_approaches)
    .method("parking", &f_airport_parking)
    .method("getSid", &FGAirport::findSIDWithIdent)
    .method("getStar", &FGAirport::findSTARWithIdent)
    .method("getIAP", &FGAirport::findApproachWithIdent)
    .method("tostring", &FGAirport::toString);

  nasal::Hash globals(globalsRef, c),
              positioned( globals.createHash("positioned") );

  positioned.set("airportinfo", &f_airportinfo);

  return naNil();
}
