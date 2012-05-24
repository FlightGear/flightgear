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
#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/sg_inlines.h>
#include <simgear/scene/material/mat.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/bucket/newbucket.hxx>

#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/parking.hxx>
#include <Scripting/NasalSys.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/procedure.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Scenery/scenery.hxx>
#include <ATC/CommStation.hxx>
#include <Navaids/FlightPlan.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/fix.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Navaids/procedure.hxx>
#include <Navaids/airways.hxx>

using namespace flightgear;

static void positionedGhostDestroy(void* g);
static void wayptGhostDestroy(void* g);
static void legGhostDestroy(void* g);
static void routeBaseGhostDestroy(void* g);

naGhostType PositionedGhostType = { positionedGhostDestroy, "positioned" };

static const char* airportGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType AirportGhostType = { positionedGhostDestroy, "airport", airportGhostGetMember, 0 };

static const char* navaidGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType NavaidGhostType = { positionedGhostDestroy, "navaid", navaidGhostGetMember, 0 };

static const char* runwayGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType RunwayGhostType = { positionedGhostDestroy, "runway", runwayGhostGetMember, 0 };

static const char* fixGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType FixGhostType = { positionedGhostDestroy, "fix", fixGhostGetMember, 0 };

static const char* wayptGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static void waypointGhostSetMember(naContext c, void* g, naRef field, naRef value);

naGhostType WayptGhostType = { wayptGhostDestroy, 
  "waypoint",
  wayptGhostGetMember,
  waypointGhostSetMember};

static const char* legGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static void legGhostSetMember(naContext c, void* g, naRef field, naRef value);

naGhostType FPLegGhostType = { legGhostDestroy, 
  "flightplan-leg",
  legGhostGetMember,
  legGhostSetMember};

static const char* flightplanGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static void flightplanGhostSetMember(naContext c, void* g, naRef field, naRef value);

naGhostType FlightPlanGhostType = { routeBaseGhostDestroy, 
  "flightplan",
  flightplanGhostGetMember,
  flightplanGhostSetMember
};

static const char* procedureGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType ProcedureGhostType = { routeBaseGhostDestroy, 
  "procedure",
  procedureGhostGetMember,
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

static WayptFlag wayptFlagFromString(const char* s)
{
  if (!strcmp(s, "sid")) return WPT_DEPARTURE;
  if (!strcmp(s, "star")) return WPT_ARRIVAL;
  if (!strcmp(s, "approach")) return WPT_APPROACH;
  if (!strcmp(s, "missed")) return WPT_MISS;
  if (!strcmp(s, "pseudo")) return WPT_PSEUDO;
  
  return (WayptFlag) 0;
}

static naRef wayptFlagToNasal(naContext c, unsigned int flags)
{
  if (flags & WPT_PSEUDO) return stringToNasal(c, "pseudo");
  if (flags & WPT_DEPARTURE) return stringToNasal(c, "sid");
  if (flags & WPT_ARRIVAL) return stringToNasal(c, "star");
  if (flags & WPT_MISS) return stringToNasal(c, "missed");
  if (flags & WPT_APPROACH) return stringToNasal(c, "approach");
  return naNil();
}

static FGPositioned* positionedGhost(naRef r)
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

static Waypt* wayptGhost(naRef r)
{
  if (naGhost_type(r) == &WayptGhostType)
    return (Waypt*) naGhost_ptr(r);
  
  if (naGhost_type(r) == &FPLegGhostType) {
    FlightPlan::Leg* leg = (FlightPlan::Leg*) naGhost_ptr(r);
    return leg->waypoint();
  }
  
  return 0;
}

static void wayptGhostDestroy(void* g)
{
  Waypt* wpt = (Waypt*)g;
  if (!Waypt::put(wpt)) // unref
    delete wpt;
}

static void legGhostDestroy(void* g)
{
  // nothing for now
}


static FlightPlan::Leg* fpLegGhost(naRef r)
{
  if (naGhost_type(r) == &FPLegGhostType)
    return (FlightPlan::Leg*) naGhost_ptr(r);
  return 0;
}

static Procedure* procedureGhost(naRef r)
{
  if (naGhost_type(r) == &ProcedureGhostType)
    return (Procedure*) naGhost_ptr(r);
  return 0;
}

static FlightPlan* flightplanGhost(naRef r)
{
  if (naGhost_type(r) == &FlightPlanGhostType)
    return (FlightPlan*) naGhost_ptr(r);
  return 0;
}

static void routeBaseGhostDestroy(void* g)
{
  // nothing for now
}

static naRef airportPrototype;
static naRef flightplanPrototype;
static naRef waypointPrototype;
static naRef geoCoordClass;
static naRef fpLegPrototype;
static naRef procedurePrototype;

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

naRef ghostForFix(naContext c, const FGFix* r)
{
  if (!r) {
    return naNil();
  }
  
  FGPositioned::get(r); // take a ref
  return naNewGhost2(c, &FixGhostType, (void*) r);
}


naRef ghostForWaypt(naContext c, const Waypt* wpt)
{
  if (!wpt) {
    return naNil();
  }
  
  Waypt::get(wpt); // take a ref
  return naNewGhost2(c, &WayptGhostType, (void*) wpt);
}

naRef ghostForLeg(naContext c, const FlightPlan::Leg* leg)
{
  if (!leg) {
    return naNil();
  }
  
  return naNewGhost2(c, &FPLegGhostType, (void*) leg);
}

naRef ghostForFlightPlan(naContext c, const FlightPlan* fp)
{
  if (!fp) {
    return naNil();
  }
  
  return naNewGhost2(c, &FlightPlanGhostType, (void*) fp);
}

naRef ghostForProcedure(naContext c, const Procedure* proc)
{
  if (!proc) {
    return naNil();
  }
  
  return naNewGhost2(c, &ProcedureGhostType, (void*) proc);
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

  } else {
    return 0;
  }
  
  return "";
}

static const char* waypointCommonGetMember(naContext c, Waypt* wpt, const char* fieldName, naRef* out)
{
  if (!strcmp(fieldName, "wp_name")) *out = stringToNasal(c, wpt->ident());
  else if (!strcmp(fieldName, "wp_type")) *out = stringToNasal(c, wpt->type());
  else if (!strcmp(fieldName, "wp_role")) *out = wayptFlagToNasal(c, wpt->flags());
  else if (!strcmp(fieldName, "wp_lat")) *out = naNum(wpt->position().getLatitudeDeg());
  else if (!strcmp(fieldName, "wp_lon")) *out = naNum(wpt->position().getLongitudeDeg());
  else if (!strcmp(fieldName, "wp_parent_name")) {
    Procedure* proc = dynamic_cast<Procedure*>(wpt->owner());
    *out = proc ? stringToNasal(c, proc->ident()) : naNil();
  } else if (!strcmp(fieldName, "wp_parent")) {
    Procedure* proc = dynamic_cast<Procedure*>(wpt->owner());
    *out = ghostForProcedure(c, proc);
  } else if (!strcmp(fieldName, "fly_type")) {
    if (wpt->type() == "hold") {
      *out = stringToNasal(c, "Hold");
    } else {
      *out = stringToNasal(c, wpt->flag(WPT_OVERFLIGHT) ? "flyOver" : "flyBy");
    }
  } else {
    return NULL; // member not found
  }

  return "";
}

static void waypointCommonSetMember(naContext c, Waypt* wpt, const char* fieldName, naRef value)
{
  if (!strcmp(fieldName, "wp_role")) {
    if (!naIsString(value)) naRuntimeError(c, "wp_role must be a string");
    if (wpt->owner() != NULL) naRuntimeError(c, "cannot override wp_role on waypoint with parent");
    WayptFlag f = wayptFlagFromString(naStr_data(value));
    if (f == 0) {
      naRuntimeError(c, "unrecognized wp_role value %s", naStr_data(value));
    }
    
    wpt->setFlag(f, true);
  }
}

static const char* wayptGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  Waypt* wpt = (flightgear::Waypt*) g;
  return waypointCommonGetMember(c, wpt, fieldName, out);
}

static RouteRestriction routeRestrictionFromString(const char* s)
{
  string u(s);
  boost::to_lower(u);
  if (u == "computed") return RESTRICT_COMPUTED;
  if (u == "at") return RESTRICT_AT;
  if (u == "mach") return SPEED_RESTRICT_MACH;
  if (u == "computed-mach") return SPEED_COMPUTED_MACH;
  if (u == "delete") return RESTRICT_DELETE;
  return RESTRICT_NONE;
};

naRef routeRestrictionToNasal(naContext c, RouteRestriction rr)
{
  switch (rr) {
    case RESTRICT_NONE: return naNil();
    case RESTRICT_AT: return stringToNasal(c, "at");
    case RESTRICT_ABOVE: return stringToNasal(c, "above");
    case RESTRICT_BELOW: return stringToNasal(c, "below");
    case SPEED_RESTRICT_MACH: return stringToNasal(c, "mach");
    case RESTRICT_COMPUTED: return stringToNasal(c, "computed");
    case SPEED_COMPUTED_MACH: return stringToNasal(c, "computed-mach");
    case RESTRICT_DELETE: return stringToNasal(c, "delete");
  }
  
  return naNil();
}

static const char* legGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  FlightPlan::Leg* leg = (FlightPlan::Leg*) g;
  Waypt* wpt = leg->waypoint();
  
  if (!strcmp(fieldName, "parents")) {
    *out = naNewVector(c);
    naVec_append(*out, fpLegPrototype);
  } else if (!strcmp(fieldName, "index")) {
    *out = naNum(leg->index());
  } else if (!strcmp(fieldName, "alt_cstr")) {
    *out = naNum(leg->altitudeFt());
  } else if (!strcmp(fieldName, "alt_cstr_type")) {
    *out = routeRestrictionToNasal(c, leg->altitudeRestriction());
  } else if (!strcmp(fieldName, "speed_cstr")) {
    double s = isMachRestrict(leg->speedRestriction()) ? leg->speedMach() : leg->speedKts();
    *out = naNum(s);
  } else if (!strcmp(fieldName, "speed_cstr_type")) {
    *out = routeRestrictionToNasal(c, leg->speedRestriction());  
  } else if (!strcmp(fieldName, "leg_distance")) {
    *out = naNum(leg->distanceNm());
  } else if (!strcmp(fieldName, "leg_bearing")) {
    *out = naNum(leg->courseDeg());
  } else if (!strcmp(fieldName, "distance_along_route")) {
    *out = naNum(leg->distanceAlongRoute());
  } else { // check for fields defined on the underlying waypoint
    return waypointCommonGetMember(c, wpt, fieldName, out);
  }
  
  return ""; // success
}

static void waypointGhostSetMember(naContext c, void* g, naRef field, naRef value)
{
  const char* fieldName = naStr_data(field);
  Waypt* wpt = (Waypt*) g;
  waypointCommonSetMember(c, wpt, fieldName, value);
}

static void legGhostSetMember(naContext c, void* g, naRef field, naRef value)
{
  const char* fieldName = naStr_data(field);
  FlightPlan::Leg* leg = (FlightPlan::Leg*) g;
    
  waypointCommonSetMember(c, leg->waypoint(), fieldName, value);
}

static const char* flightplanGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  FlightPlan* fp = (FlightPlan*) g;
  
  if (!strcmp(fieldName, "parents")) {
    *out = naNewVector(c);
    naVec_append(*out, flightplanPrototype);
  } else if (!strcmp(fieldName, "id")) *out = stringToNasal(c, fp->ident());
  else if (!strcmp(fieldName, "departure")) *out = ghostForAirport(c, fp->departureAirport());
  else if (!strcmp(fieldName, "destination")) *out = ghostForAirport(c, fp->destinationAirport());
  else if (!strcmp(fieldName, "departure_runway")) *out = ghostForRunway(c, fp->departureRunway());
  else if (!strcmp(fieldName, "destination_runway")) *out = ghostForRunway(c, fp->destinationRunway());
  else if (!strcmp(fieldName, "sid")) *out = ghostForProcedure(c, fp->sid());
  else if (!strcmp(fieldName, "sid_trans")) *out = ghostForProcedure(c, fp->sidTransition());
  else if (!strcmp(fieldName, "star")) *out = ghostForProcedure(c, fp->star());
  else if (!strcmp(fieldName, "star_trans")) *out = ghostForProcedure(c, fp->starTransition());
  else if (!strcmp(fieldName, "approach")) *out = ghostForProcedure(c, fp->approach());
  else if (!strcmp(fieldName, "current")) *out = naNum(fp->currentIndex());
  else {
    return 0;
  }
  
  return "";
}

static void flightplanGhostSetMember(naContext c, void* g, naRef field, naRef value)
{
  const char* fieldName = naStr_data(field);
  FlightPlan* fp = (FlightPlan*) g;
  
  if (!strcmp(fieldName, "id")) {
    if (!naIsString(value)) naRuntimeError(c, "flightplan.id must be a string");
    fp->setIdent(naStr_data(value));
  } else if (!strcmp(fieldName, "current")) {
    int index = value.num;
    if ((index < 0) || (index >= fp->numLegs())) {
      return;
    }
    fp->setCurrentIndex(index);
  } else if (!strcmp(fieldName, "departure")) {
    FGAirport* apt = airportGhost(value);
    if (apt) {
      fp->setDeparture(apt);
      return;
    }
    
    FGRunway* rwy = runwayGhost(value);
    if (rwy){
      fp->setDeparture(rwy);
      return;
    }
    
    naRuntimeError(c, "bad argument type setting departure");
  } else if (!strcmp(fieldName, "destination")) {
    FGAirport* apt = airportGhost(value);
    if (apt) {
      fp->setDestination(apt);
      return;
    }
    
    FGRunway* rwy = runwayGhost(value);
    if (rwy){
      fp->setDestination(rwy);
      return;
    }
    
    naRuntimeError(c, "bad argument type setting destination");
  } else if (!strcmp(fieldName, "departure_runway")) {
    FGRunway* rwy = runwayGhost(value);
    if (rwy){
      fp->setDeparture(rwy);
      return;
    }
    
    naRuntimeError(c, "bad argument type setting departure");
  } else if (!strcmp(fieldName, "destination_runway")) {
    FGRunway* rwy = runwayGhost(value);
    if (rwy){
      fp->setDestination(rwy);
      return;
    }
    
    naRuntimeError(c, "bad argument type setting departure");
  } else if (!strcmp(fieldName, "sid")) {
    Procedure* proc = procedureGhost(value);
    if (proc && (proc->type() == PROCEDURE_SID)) {
      fp->setSID((flightgear::SID*) proc);
      return;
    }
    // allow a SID transition to be set, implicitly include the SID itself
    if (proc && (proc->type() == PROCEDURE_TRANSITION)) {
      fp->setSID((Transition*) proc);
      return;
    }
        
    if (naIsString(value)) {
      FGAirport* apt = fp->departureAirport();
      fp->setSID(apt->findSIDWithIdent(naStr_data(value)));
      return;
    }
    
    naRuntimeError(c, "bad argument type setting SID");
  } else if (!strcmp(fieldName, "star")) {
    Procedure* proc = procedureGhost(value);
    if (proc && (proc->type() == PROCEDURE_STAR)) {
      fp->setSTAR((STAR*) proc);
      return;
    }
    
    if (proc && (proc->type() == PROCEDURE_TRANSITION)) {
      fp->setSTAR((Transition*) proc);
      return;
    }
    
    if (naIsString(value)) {
      FGAirport* apt = fp->destinationAirport();
      fp->setSTAR(apt->findSTARWithIdent(naStr_data(value)));
      return;
    }
    
    naRuntimeError(c, "bad argument type setting STAR");
  } else if (!strcmp(fieldName, "approach")) {
    Procedure* proc = procedureGhost(value);
    if (proc && Approach::isApproach(proc->type())) {
      fp->setApproach((Approach*) proc);
      return;
    }
    
    if (naIsString(value)) {
      FGAirport* apt = fp->destinationAirport();
      fp->setApproach(apt->findApproachWithIdent(naStr_data(value)));
      return;
    }
    
    naRuntimeError(c, "bad argument type setting approach");
  }
}


static naRef procedureTpType(naContext c, ProcedureType ty)
{
  switch (ty) {
    case PROCEDURE_SID: return stringToNasal(c, "sid");
    case PROCEDURE_STAR: return stringToNasal(c, "star");
    case PROCEDURE_APPROACH_VOR: 
    case PROCEDURE_APPROACH_ILS: 
    case PROCEDURE_APPROACH_RNAV: 
    case PROCEDURE_APPROACH_NDB:
      return stringToNasal(c, "IAP");
    default:
      return naNil();
  }
}

static naRef procedureRadioType(naContext c, ProcedureType ty)
{
  switch (ty) {
    case PROCEDURE_APPROACH_VOR: return stringToNasal(c, "VOR");
    case PROCEDURE_APPROACH_ILS: return stringToNasal(c, "ILS");
    case PROCEDURE_APPROACH_RNAV: return stringToNasal(c, "RNAV");
    case PROCEDURE_APPROACH_NDB: return stringToNasal(c, "NDB");
    default:
      return naNil();
  }
}

static const char* procedureGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  Procedure* proc = (Procedure*) g;
  
  if (!strcmp(fieldName, "parents")) {
    *out = naNewVector(c);
    naVec_append(*out, procedurePrototype);
  } else if (!strcmp(fieldName, "id")) *out = stringToNasal(c, proc->ident());
  else if (!strcmp(fieldName, "airport")) *out = ghostForAirport(c, proc->airport());
  else if (!strcmp(fieldName, "tp_type")) *out = procedureTpType(c, proc->type());
  else if (!strcmp(fieldName, "radio")) *out = procedureRadioType(c, proc->type());
  else if (!strcmp(fieldName, "runways")) {
    *out = naNewVector(c);
    BOOST_FOREACH(FGRunwayPtr rwy, proc->runways()) {
      naVec_append(*out, stringToNasal(c, rwy->ident()));
    }
  } else if (!strcmp(fieldName, "transitions")) {
    if ((proc->type() != PROCEDURE_SID) && (proc->type() != PROCEDURE_STAR)) {
      *out = naNil();
      return "";
    }
        
    ArrivalDeparture* ad = static_cast<ArrivalDeparture*>(proc);
    *out = naNewVector(c);
    BOOST_FOREACH(string id, ad->transitionIdents()) {
      naVec_append(*out, stringToNasal(c, id));
    }
  } else {
    return 0;
  }
  
  return "";
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

static const char* fixGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  FGFix* fix = (FGFix*) g;
  
  if (!strcmp(fieldName, "id")) *out = stringToNasal(c, fix->ident());
  else if (!strcmp(fieldName, "lat")) *out = naNum(fix->get_lat());
  else if (!strcmp(fieldName, "lon")) *out = naNum(fix->get_lon());
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
  
  return naEqual(naVec_get(parents, 0), geoCoordClass);
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
    
    if (gt == &FixGhostType) {
      result = fixGhost(args[offset])->geod();
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
    minRunwayLengthFt = fgGetDouble("/sim/navdb/min-runway-length-ft", 0.0);
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
    
  virtual bool pass(FGPositioned* aPos) const
  {
    FGAirport* apt = (FGAirport*) aPos;
    if ((apt->type() == FGPositioned::AIRPORT) && 
        !apt->hasHardRunwayOfLengthFt(minRunwayLengthFt)) 
    {
      return false;
    }

    return true;
  }
  
  FGPositioned::Type type;
  double minRunwayLengthFt;
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
  boost::to_upper(ident);
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

static naRef f_airport_approaches(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.getApproachList called on non-airport object");
  }
  
  naRef approaches = naNewVector(c);
  
  ProcedureType ty = PROCEDURE_INVALID;
  if ((argc > 1) && naIsString(args[1])) {
    std::string u(naStr_data(args[1]));
    boost::to_upper(u);
    if (u == "NDB") ty = PROCEDURE_APPROACH_NDB;
    if (u == "VOR") ty = PROCEDURE_APPROACH_VOR;
    if (u == "ILS") ty = PROCEDURE_APPROACH_ILS;
    if (u == "RNAV") ty = PROCEDURE_APPROACH_RNAV;
  }
  
  FGRunway* rwy = NULL;
  if (argc > 0 && (rwy = runwayGhost(args[0]))) {
    // ok
  } else if (argc > 0 && naIsString(args[0])) {
    if (!apt->hasRunwayWithIdent(naStr_data(args[0]))) {
      return naNil();
    }
    
    rwy = apt->getRunwayByIdent(naStr_data(args[0]));
  }
  
  if (rwy) {
    BOOST_FOREACH(Approach* s, rwy->getApproaches()) {
      if ((ty != PROCEDURE_INVALID) && (s->type() != ty)) {
        continue;
      }
      
      naRef procId = stringToNasal(c, s->ident());
      naVec_append(approaches, procId);
    }
  } else {
    // no runway specified, report them all
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

static naRef f_airport_getSid(naContext c, naRef me, int argc, naRef* args)
{
  FGAirport* apt = airportGhost(me);
  if (!apt) {
    naRuntimeError(c, "airport.getSid called on non-airport object");
  }
  
  if ((argc != 1) || !naIsString(args[0])) {
    naRuntimeError(c, "airport.getSid passed invalid argument");
  }
  
  string ident = naStr_data(args[0]);
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
  
  string ident = naStr_data(args[0]);
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
  
  string ident = naStr_data(args[0]);
  return ghostForProcedure(c, apt->findApproachWithIdent(ident));
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

static naRef f_findFixesByIdent(naContext c, naRef me, int argc, naRef* args)
{
  int argOffset = 0;
  SGGeod pos = globals->get_aircraft_position();
  argOffset += geodFromArgs(args, 0, argc, pos);
  
  if (!naIsString(args[argOffset])) {
    naRuntimeError(c, "findFixesByIdent expectes ident string as arg %d", argOffset);
  }
  
  string ident(naStr_data(args[argOffset]));
  naRef r = naNewVector(c);
  
  FGPositioned::TypeFilter filter(FGPositioned::FIX);
  FGPositioned::List fixes = FGPositioned::findAllWithIdent(ident, &filter);
  FGPositioned::sortByRange(fixes, pos);
  
  BOOST_FOREACH(FGPositionedRef f, fixes) {
    naVec_append(r, ghostForFix(c, (FGFix*) f.ptr()));
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
  if (argc == 0) {
    FGRouteMgr* rm = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));  
    return ghostForFlightPlan(c, rm->flightPlan());
  }
  
  if ((argc > 0) && naIsString(args[0])) {
    flightgear::FlightPlan* fp = new flightgear::FlightPlan;
    SGPath path(naStr_data(args[0]));
    if (!path.exists()) {
      naRuntimeError(c, "flightplan, no file at path %s", path.c_str());
    }
    
    if (!fp->load(path)) {
      SG_LOG(SG_NASAL, SG_WARN, "failed to load flight-plan from " << path);
      delete fp;
      return naNil();
    }
    
    return ghostForFlightPlan(c, fp);
  }
  
  naRuntimeError(c, "bad arguments to flightplan()");
  return naNil();
}

class NasalFPDelegate : public FlightPlan::Delegate
{
public:
  NasalFPDelegate(FlightPlan* fp, FGNasalSys* sys, naRef ins) :
    _nasal(sys),
    _plan(fp),
    _instance(ins)
  {
    SG_LOG(SG_NASAL, SG_INFO, "created Nasal delegate for " << fp);
    _gcSaveKey = _nasal->gcSave(ins);
  }
  
  virtual ~NasalFPDelegate()
  {
    SG_LOG(SG_NASAL, SG_INFO, "destroying Nasal delegate for " << _plan);
    _nasal->gcRelease(_gcSaveKey);
  }
  
  virtual void departureChanged()
  {
    callDelegateMethod("departureChanged");
  }
  
  virtual void arrivalChanged()
  {
    callDelegateMethod("arrivalChanged");
  }
  
  virtual void waypointsChanged()
  {
    callDelegateMethod("waypointsChanged");
  }
  
  virtual void currentWaypointChanged()
  {
    callDelegateMethod("currentWaypointChanged");
  }
private:
  
  void callDelegateMethod(const char* method)
  {
    naRef f;
    naMember_cget(_nasal->context(), _instance, method, &f);
    if (naIsNil(f)) {
      return; // no method on the delegate
    }
    
    naRef arg[1];
    arg[0] = ghostForFlightPlan(_nasal->context(), _plan);
    _nasal->callMethod(f, _instance, 1, arg, naNil());
  }
  
  FGNasalSys* _nasal;
  FlightPlan* _plan;
  naRef _instance;
  int _gcSaveKey;
};

class NasalFPDelegateFactory : public FlightPlan::DelegateFactory
{
public:
  NasalFPDelegateFactory(naRef code)
  {
    _nasal = (FGNasalSys*) globals->get_subsystem("nasal");
    _func = code;
    _gcSaveKey = _nasal->gcSave(_func);
  }
  
  ~NasalFPDelegateFactory()
  {
    _nasal->gcRelease(_gcSaveKey);
  }
  
  virtual FlightPlan::Delegate* createFlightPlanDelegate(FlightPlan* fp)
  {
    naRef args[1];
    args[0] = ghostForFlightPlan(_nasal->context(), fp);
    naRef instance = _nasal->call(_func, 1, args, naNil());
    if (naIsNil(instance)) {
      return NULL;
    }
    
    return new NasalFPDelegate(fp, _nasal, instance);
  }
private:
  FGNasalSys* _nasal;
  naRef _func;
  int _gcSaveKey;
};

static naRef f_registerFPDelegate(naContext c, naRef me, int argc, naRef* args)
{
  if ((argc < 1) || !naIsFunc(args[0])) {
    naRuntimeError(c, "non-function argument to registerFlightPlanDelegate");
  }
  
  NasalFPDelegateFactory* factory = new NasalFPDelegateFactory(args[0]);
  FlightPlan::registerDelegateFactory(factory);
  
  return naNil();
}

static WayptRef wayptFromArg(naRef arg)
{
  WayptRef r = wayptGhost(arg);
  if (r.valid()) {
    return r;
  }
  
  FGPositioned* pos = positionedGhost(arg);
  if (!pos) {
    // let's check if the arg is hash, coudl extra a geod and hence build
    // a simple waypoint
    
    return WayptRef();
  }
  
// special-case for runways
  if (pos->type() == FGPositioned::RUNWAY) {
    return new RunwayWaypt((FGRunway*) pos, NULL);
  }
  
  return new NavaidWaypoint(pos, NULL);
}

static naRef convertWayptVecToNasal(naContext c, const WayptVec& wps)
{
  naRef result = naNewVector(c);
  BOOST_FOREACH(WayptRef wpt, wps) {
    naVec_append(result, ghostForWaypt(c, wpt.get()));
  }
  return result;
}

static naRef f_airwaySearch(naContext c, naRef me, int argc, naRef* args)
{
  if (argc < 2) {
    naRuntimeError(c, "airwaysSearch needs at least two arguments");
  }
  
  WayptRef start = wayptFromArg(args[0]), 
    end = wayptFromArg(args[1]);
  
  if (!start || !end) {
    SG_LOG(SG_NASAL, SG_WARN, "airwaysSearch: start or end points are invalid");
    return naNil();
  }
  
  bool highLevel = true;
  if ((argc > 2) && naIsString(args[2])) {
    if (!strcmp(naStr_data(args[2]), "lowlevel")) {
      highLevel = false;
    }
  }
  
  WayptVec route;
  if (highLevel) {
    Airway::highLevel()->route(start, end, route);
  } else {
    Airway::lowLevel()->route(start, end, route);
  }
  
  return convertWayptVecToNasal(c, route);
}

static naRef f_createWP(naContext c, naRef me, int argc, naRef* args)
{
  SGGeod pos;
  int argOffset = geodFromArgs(args, 0, argc, pos);
  
  if (((argc - argOffset) < 1) || !naIsString(args[argOffset])) {
    naRuntimeError(c, "createWP: no identifier supplied");
  }
    
  string ident = naStr_data(args[argOffset++]);
  WayptRef wpt = new BasicWaypt(pos, ident, NULL);
  
// set waypt flags - approach, departure, pseudo, etc
  if (argc > argOffset) {
    WayptFlag f = wayptFlagFromString(naStr_data(args[argOffset++]));
    wpt->setFlag(f);
  }
  
  return ghostForWaypt(c, wpt);
}

static naRef f_createWPFrom(naContext c, naRef me, int argc, naRef* args)
{
  if (argc < 1) {
    naRuntimeError(c, "createWPFrom: need at least one argument");
  }
  
  FGPositioned* positioned = positionedGhost(args[0]);
  if (!positioned) {
    naRuntimeError(c, "createWPFrom: couldn;t convert arg[0] to FGPositioned");
  }
  
  WayptRef wpt;
  if (positioned->type() == FGPositioned::RUNWAY) {
    wpt = new RunwayWaypt((FGRunway*) positioned, NULL);
  } else {
    wpt = new NavaidWaypoint(positioned, NULL);
  }

  // set waypt flags - approach, departure, pseudo, etc
  if (argc > 1) {
    WayptFlag f = wayptFlagFromString(naStr_data(args[1]));
    wpt->setFlag(f);
  }
  
  return ghostForWaypt(c, wpt);
}

static naRef f_flightplan_getWP(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.getWP called on non-flightplan object");
  }

  int index;
  if (argc == 0) {
    index = fp->currentIndex();
  } else {
    index = (int) naNumValue(args[0]).num;
  }
  
  if ((index < 0) || (index >= fp->numLegs())) {
    return naNil();
  }
  
  return ghostForLeg(c, fp->legAtIndex(index));
}

static naRef f_flightplan_currentWP(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.currentWP called on non-flightplan object");
  }
  return ghostForLeg(c, fp->currentLeg());
}

static naRef f_flightplan_nextWP(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.nextWP called on non-flightplan object");
  }
  return ghostForLeg(c, fp->nextLeg());
}

static naRef f_flightplan_numWaypoints(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.numWaypoints called on non-flightplan object");
  }
  return naNum(fp->numLegs());
}

static naRef f_flightplan_appendWP(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.appendWP called on non-flightplan object");
  }
  
  WayptRef wp = wayptGhost(args[0]);
  int index = fp->numLegs();
  fp->insertWayptAtIndex(wp.get(), index);
  return naNum(index);
}

static naRef f_flightplan_insertWP(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.insertWP called on non-flightplan object");
  }
  
  WayptRef wp = wayptGhost(args[0]);
  int index = -1; // append
  if ((argc > 1) && naIsNum(args[1])) {
    index = (int) args[1].num;
  }
  
  fp->insertWayptAtIndex(wp.get(), index);
  return naNil();
}

static naRef f_flightplan_insertWPAfter(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.insertWPAfter called on non-flightplan object");
  }
  
  WayptRef wp = wayptGhost(args[0]);
  int index = -1; // append
  if ((argc > 1) && naIsNum(args[1])) {
    index = (int) args[1].num;
  }
  
  fp->insertWayptAtIndex(wp.get(), index + 1);
  return naNil();
}

static naRef f_flightplan_insertWaypoints(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.insertWaypoints called on non-flightplan object");
  }
  
  WayptVec wps;
  if (!naIsVector(args[0])) {
    naRuntimeError(c, "flightplan.insertWaypoints expects vector as first arg");
  }

  int count = naVec_size(args[0]);
  for (int i=0; i<count; ++i) {
    Waypt* wp = wayptGhost(naVec_get(args[0], i));
    if (wp) {
      wps.push_back(wp);
    }
  }
  
  int index = -1; // append
  if ((argc > 1) && naIsNum(args[1])) {
    index = (int) args[1].num;
  }

  fp->insertWayptsAtIndex(wps, index);
  return naNil();
}

static naRef f_flightplan_deleteWP(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.deleteWP called on non-flightplan object");
  }
  
  if ((argc < 1) || !naIsNum(args[0])) {
    naRuntimeError(c, "bad argument to flightplan.deleteWP");
  }
  
  int index = (int) args[0].num;
  fp->deleteIndex(index);
  return naNil();
}

static naRef f_flightplan_clearPlan(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.clearPlan called on non-flightplan object");
  }
  
  fp->clear();
  return naNil();
}

static naRef f_flightplan_clearWPType(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.clearWPType called on non-flightplan object");
  }
  
  if (argc < 1) {
    naRuntimeError(c, "insufficent args to flightplan.clearWPType");
  }
  
  WayptFlag flag = wayptFlagFromString(naStr_data(args[0]));
  fp->clearWayptsWithFlag(flag);
  return naNil();
}

static naRef f_flightplan_clone(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan* fp = flightplanGhost(me);
  if (!fp) {
    naRuntimeError(c, "flightplan.clone called on non-flightplan object");
  }
  
  return ghostForFlightPlan(c, fp->clone());
}

static naRef f_leg_setSpeed(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan::Leg* leg = fpLegGhost(me);
  if (!leg) {
    naRuntimeError(c, "leg.setSpeed called on non-flightplan-leg object");
  }
  
  if (argc < 2) {
    naRuntimeError(c, "bad arguments to leg.setSpeed");
  }
  
  RouteRestriction rr = routeRestrictionFromString(naStr_data(args[1]));
  leg->setSpeed(rr, args[0].num);
  return naNil();
}

static naRef f_leg_setAltitude(naContext c, naRef me, int argc, naRef* args)
{
  FlightPlan::Leg* leg = fpLegGhost(me);
  if (!leg) {
    naRuntimeError(c, "leg.setAltitude called on non-flightplan-leg object");
  }
  
  if (argc < 2) {
    naRuntimeError(c, "bad arguments to leg.setAltitude");
  }
  
  RouteRestriction rr = routeRestrictionFromString(naStr_data(args[1]));
  leg->setAltitude(rr, args[0].num);
  return naNil();
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

static naRef f_procedure_transition(naContext c, naRef me, int argc, naRef* args)
{
  Procedure* proc = procedureGhost(me);
  if (!proc) {
    naRuntimeError(c, "procedure.transition called on non-procedure object");
  }
  
  if ((proc->type() != PROCEDURE_SID) && (proc->type() != PROCEDURE_STAR)) {
    naRuntimeError(c, "procedure.transition called on non-SID or -STAR");
  }
  
  ArrivalDeparture* ad = (ArrivalDeparture*) proc;
  Transition* trans = ad->findTransitionByName(naStr_data(args[0]));
  
  return ghostForProcedure(c, trans);
}

static naRef f_procedure_route(naContext c, naRef me, int argc, naRef* args)
{
  Procedure* proc = procedureGhost(me);
  if (!proc) {
    naRuntimeError(c, "procedure.route called on non-procedure object");
  }
  
// wrapping up tow different routines here - approach routing from the IAF
// to the associated runway, and SID/STAR routing via an enroute transition
// and possibly a runway transition or not.
  if (Approach::isApproach(proc->type())) {
    WayptRef iaf;
    if (argc > 0) {
      iaf = wayptFromArg(args[0]);
    }
    
    WayptVec r;
    Approach* app = (Approach*) proc;
    if (!app->route(iaf, r)) {
      SG_LOG(SG_NASAL, SG_WARN, "procedure.route failed for Approach somehow");
      return naNil();
    }
    
    return convertWayptVecToNasal(c, r);
  } else if ((proc->type() != PROCEDURE_SID) && (proc->type() != PROCEDURE_STAR)) {
    naRuntimeError(c, "procedure.route called on unsuitable procedure type");
  }
  
  int argOffset = 0;
  FGRunway* rwy = runwayGhost(args[0]);
  if (rwy) ++argOffset;
  
  ArrivalDeparture* ad = (ArrivalDeparture*) proc;
  Transition* trans = NULL;
  if (argOffset < argc) {
    trans = (Transition*) procedureGhost(args[argOffset]);
  }
  
  // note either runway or trans may be NULL - that's ok
  WayptVec r;
  if (!ad->route(rwy, trans, r)) {
    SG_LOG(SG_NASAL, SG_WARN, "prcoedure.route failed for ArrvialDeparture somehow");
    return naNil();
  }
  
  return convertWayptVecToNasal(c, r);
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
  { "findFixesByID", f_findFixesByIdent },
  { "flightplan", f_route },
  { "registerFlightPlanDelegate", f_registerFPDelegate },
  { "createWP", f_createWP },
  { "createWPFrom", f_createWPFrom },
  { "airwaysRoute", f_airwaySearch },
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
    hashset(c, airportPrototype, "getApproachList", naNewFunc(c, naNewCCode(c, f_airport_approaches)));
    hashset(c, airportPrototype, "parking", naNewFunc(c, naNewCCode(c, f_airport_parking)));
    hashset(c, airportPrototype, "getSid", naNewFunc(c, naNewCCode(c, f_airport_getSid)));
    hashset(c, airportPrototype, "getStar", naNewFunc(c, naNewCCode(c, f_airport_getStar)));
    hashset(c, airportPrototype, "getIAP", naNewFunc(c, naNewCCode(c, f_airport_getApproach)));
    hashset(c, airportPrototype, "tostring", naNewFunc(c, naNewCCode(c, f_airport_toString)));
  
    flightplanPrototype = naNewHash(c);
    hashset(c, gcSave, "flightplanProto", flightplanPrototype);
      
    hashset(c, flightplanPrototype, "getWP", naNewFunc(c, naNewCCode(c, f_flightplan_getWP)));
    hashset(c, flightplanPrototype, "currentWP", naNewFunc(c, naNewCCode(c, f_flightplan_currentWP))); 
    hashset(c, flightplanPrototype, "nextWP", naNewFunc(c, naNewCCode(c, f_flightplan_nextWP))); 
    hashset(c, flightplanPrototype, "getPlanSize", naNewFunc(c, naNewCCode(c, f_flightplan_numWaypoints)));
    hashset(c, flightplanPrototype, "appendWP", naNewFunc(c, naNewCCode(c, f_flightplan_appendWP))); 
    hashset(c, flightplanPrototype, "insertWP", naNewFunc(c, naNewCCode(c, f_flightplan_insertWP))); 
    hashset(c, flightplanPrototype, "deleteWP", naNewFunc(c, naNewCCode(c, f_flightplan_deleteWP))); 
    hashset(c, flightplanPrototype, "insertWPAfter", naNewFunc(c, naNewCCode(c, f_flightplan_insertWPAfter))); 
    hashset(c, flightplanPrototype, "insertWaypoints", naNewFunc(c, naNewCCode(c, f_flightplan_insertWaypoints))); 
    hashset(c, flightplanPrototype, "cleanPlan", naNewFunc(c, naNewCCode(c, f_flightplan_clearPlan))); 
    hashset(c, flightplanPrototype, "clearWPType", naNewFunc(c, naNewCCode(c, f_flightplan_clearWPType))); 
    hashset(c, flightplanPrototype, "clone", naNewFunc(c, naNewCCode(c, f_flightplan_clone))); 
  
    waypointPrototype = naNewHash(c);
    hashset(c, gcSave, "wayptProto", waypointPrototype);
    
    hashset(c, waypointPrototype, "navaid", naNewFunc(c, naNewCCode(c, f_waypoint_navaid)));
    hashset(c, waypointPrototype, "runway", naNewFunc(c, naNewCCode(c, f_waypoint_runway)));
    hashset(c, waypointPrototype, "airport", naNewFunc(c, naNewCCode(c, f_waypoint_airport)));
  
    procedurePrototype = naNewHash(c);
    hashset(c, gcSave, "procedureProto", procedurePrototype);
    hashset(c, procedurePrototype, "transition", naNewFunc(c, naNewCCode(c, f_procedure_transition)));
    hashset(c, procedurePrototype, "route", naNewFunc(c, naNewCCode(c, f_procedure_route)));
  
    fpLegPrototype = naNewHash(c);
    hashset(c, gcSave, "fpLegProto", fpLegPrototype);
    hashset(c, fpLegPrototype, "setSpeed", naNewFunc(c, naNewCCode(c, f_leg_setSpeed)));
    hashset(c, fpLegPrototype, "setAltitude", naNewFunc(c, naNewCCode(c, f_leg_setAltitude)));
  
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


