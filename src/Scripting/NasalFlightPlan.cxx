// NasalFlightPlan.cxx -- expose FlightPlan classes to Nasal
//
// Written by James Turner, started 2020.
//
// Copyright (C) 2020 James Turner
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

#include <algorithm>
#include <cstring>

#include "NasalFlightPlan.hxx"

#include "NasalPositioned.hxx"

#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/parking.hxx>
#include <Airports/runways.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Navaids/FlightPlan.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/airways.hxx>
#include <Navaids/fix.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/procedure.hxx>
#include <Navaids/routePath.hxx>
#include <Navaids/waypoint.hxx>
#include <Scenery/scenery.hxx>
#include <Scripting/NasalSys.hxx>

using namespace flightgear;

static void wayptGhostDestroy(void* g);
static void legGhostDestroy(void* g);
static void routeBaseGhostDestroy(void* g);

static const char* wayptGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static void        waypointGhostSetMember(naContext c, void* g, naRef field, naRef value);

static naGhostType WayptGhostType = {wayptGhostDestroy,
                                     "waypoint",
                                     wayptGhostGetMember,
                                     waypointGhostSetMember};

static const char* legGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static void        legGhostSetMember(naContext c, void* g, naRef field, naRef value);

static naGhostType FPLegGhostType = {legGhostDestroy,
                                     "flightplan-leg",
                                     legGhostGetMember,
                                     legGhostSetMember};

static const char* flightplanGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static void        flightplanGhostSetMember(naContext c, void* g, naRef field, naRef value);

static naGhostType FlightPlanGhostType = {routeBaseGhostDestroy,
                                          "flightplan",
                                          flightplanGhostGetMember,
                                          flightplanGhostSetMember};

static const char* procedureGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static naGhostType ProcedureGhostType = {routeBaseGhostDestroy,
                                         "procedure",
                                         procedureGhostGetMember,
                                         0};

static const char* airwayGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static naGhostType AirwayGhostType = {routeBaseGhostDestroy,
                                      "airway",
                                      airwayGhostGetMember,
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
                          const_cast<char*>(s.c_str()),
                          s.length());
}

static bool convertToNum(naRef v, double& result)
{
    naRef n = naNumValue(v);
    if (naIsNil(n)) {
        return false; // couldn't convert
    }

    result = n.num;
    return true;
}

static WayptFlag wayptFlagFromString(const char* s)
{
    if (!strcmp(s, "sid")) return WPT_DEPARTURE;
    if (!strcmp(s, "star")) return WPT_ARRIVAL;
    if (!strcmp(s, "approach")) return WPT_APPROACH;
    if (!strcmp(s, "missed")) return WPT_MISS;
    if (!strcmp(s, "pseudo")) return WPT_PSEUDO;

    return (WayptFlag)0;
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

Waypt* wayptGhost(naRef r)
{
    if (naGhost_type(r) == &WayptGhostType)
        return (Waypt*)naGhost_ptr(r);

    if (naGhost_type(r) == &FPLegGhostType) {
        FlightPlan::Leg* leg = (FlightPlan::Leg*)naGhost_ptr(r);
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
    FlightPlan::Leg* leg = (FlightPlan::Leg*)g;
    if (!FlightPlan::Leg::put(leg)) // unref
        delete leg;
}


FlightPlan::Leg* fpLegGhost(naRef r)
{
    if (naGhost_type(r) == &FPLegGhostType)
        return (FlightPlan::Leg*)naGhost_ptr(r);
    return 0;
}

Procedure* procedureGhost(naRef r)
{
    if (naGhost_type(r) == &ProcedureGhostType)
        return (Procedure*)naGhost_ptr(r);
    return 0;
}

static FlightPlan* flightplanGhost(naRef r)
{
    if (naGhost_type(r) == &FlightPlanGhostType)
        return (FlightPlan*)naGhost_ptr(r);
    return 0;
}

static Airway* airwayGhost(naRef r)
{
    if (naGhost_type(r) == &AirwayGhostType)
        return (Airway*)naGhost_ptr(r);
    return 0;
}

static void routeBaseGhostDestroy(void* g)
{
    RouteBase* r = (RouteBase*)g;
    if (!RouteBase::put(r)) // unref
        delete r;
}

static naRef flightplanPrototype;
static naRef fpLegPrototype;
static naRef procedurePrototype;
static naRef airwayPrototype;

naRef ghostForWaypt(naContext c, const Waypt* wpt)
{
    if (!wpt) {
        return naNil();
    }

    Waypt::get(wpt); // take a ref
    return naNewGhost2(c, &WayptGhostType, (void*)wpt);
}

naRef ghostForLeg(naContext c, const FlightPlan::Leg* leg)
{
    if (!leg) {
        return naNil();
    }

    FlightPlan::Leg::get(leg); // take a ref
    return naNewGhost2(c, &FPLegGhostType, (void*)leg);
}

naRef ghostForFlightPlan(naContext c, const FlightPlan* fp)
{
    if (!fp) {
        return naNil();
    }

    FlightPlan::get(fp); // take a ref
    return naNewGhost2(c, &FlightPlanGhostType, (void*)fp);
}

naRef ghostForProcedure(naContext c, const Procedure* proc)
{
    if (!proc) {
        return naNil();
    }

    FlightPlan::get(proc); // take a ref
    return naNewGhost2(c, &ProcedureGhostType, (void*)proc);
}

naRef ghostForAirway(naContext c, const Airway* awy)
{
    if (!awy) {
        return naNil();
    }

    Airway::get(awy); // take a ref
    return naNewGhost2(c, &AirwayGhostType, (void*)awy);
}

// Return the navaid ghost associated with a waypoint of navaid type.
static naRef waypointNavaid(naContext c, Waypt* wpt)
{
    FGPositioned* pos = wpt->source();
    if (!pos || (!FGNavRecord::isNavaidType(pos) && !fgpositioned_cast<FGFix>(pos))) {
        return naNil();
    }

    return ghostForPositioned(c, wpt->source());
}

// Return the airport ghost associated with a waypoint of airport or runway
// type.
static naRef waypointAirport(naContext c, Waypt* wpt)
{
    FGPositioned* pos = wpt->source();

    if (FGPositioned::isRunwayType(pos)) {
        pos = static_cast<FGRunway*>(pos)->airport();
    } else if (!FGPositioned::isAirportType(pos)) {
        return naNil();
    }

    return ghostForAirport(c, static_cast<FGAirport*>(pos));
}

// Return the runway ghost associated with a waypoint of runway type.
static naRef waypointRunway(naContext c, Waypt* wpt)
{
    FGPositioned* pos = wpt->source();

    if (!FGPositioned::isRunwayType(pos)) {
        return naNil();
    }

    return ghostForRunway(c, static_cast<FGRunway*>(pos));
}

static const char* waypointCommonGetMember(naContext c, Waypt* wpt, const char* fieldName, naRef* out)
{
    if (!strcmp(fieldName, "wp_name") || !strcmp(fieldName, "id"))
        *out = stringToNasal(c, wpt->ident());
    else if (!strcmp(fieldName, "wp_type"))
        *out = stringToNasal(c, wpt->type());
    else if (!strcmp(fieldName, "wp_role"))
        *out = wayptFlagToNasal(c, wpt->flags());
    else if (!strcmp(fieldName, "wp_lat") || !strcmp(fieldName, "lat"))
        *out = naNum(wpt->position().getLatitudeDeg());
    else if (!strcmp(fieldName, "wp_lon") || !strcmp(fieldName, "lon"))
        *out = naNum(wpt->position().getLongitudeDeg());
    else if (!strcmp(fieldName, "wp_parent_name")) {
        if (wpt->owner()) {
            *out = stringToNasal(c, wpt->owner()->ident());
        } else {
            *out = naNil();
        }
    } else if (!strcmp(fieldName, "wp_parent")) {
        // TODO add ghostForRouteElement to cover all this
        Procedure* proc = dynamic_cast<Procedure*>(wpt->owner());
        if (proc) {
            *out = ghostForProcedure(c, proc);
        } else {
            Airway* airway = dynamic_cast<Airway*>(wpt->owner());
            if (airway) {
                *out = ghostForAirway(c, airway);
            } else {
                *out = naNil();
            }
        }
    } else if (!strcmp(fieldName, "fly_type")) {
        if (wpt->type() == "hold") {
            *out = stringToNasal(c, "Hold");
        } else {
            *out = stringToNasal(c, wpt->flag(WPT_OVERFLIGHT) ? "flyOver" : "flyBy");
        }
    } else if (!strcmp(fieldName, "heading_course")) {
        *out = naNum(wpt->headingRadialDeg());
    } else if (!strcmp(fieldName, "navaid")) {
        *out = waypointNavaid(c, wpt);
    } else if (!strcmp(fieldName, "airport")) {
        *out = waypointAirport(c, wpt);
    } else if (!strcmp(fieldName, "runway")) {
        *out = waypointRunway(c, wpt);
    } else if (!strcmp(fieldName, "airway")) {
        if (wpt->type() == "via") {
            AirwayRef awy = static_cast<Via*>(wpt)->airway();
            assert(awy);
            *out = ghostForAirway(c, awy);
        } else {
            *out = naNil();
        }
    } else if (!strcmp(fieldName, "hidden")) {
        *out = naNum(wpt->flag(WPT_HIDDEN));
    } else if (wpt->type() == "hold") {
        // hold-specific properties
        const auto hold = static_cast<Hold*>(wpt);
        if (!strcmp(fieldName, "hold_is_left_handed")) {
            *out = naNum(hold->isLeftHanded());
        } else if (!strcmp(fieldName, "hold_is_distance")) {
            *out = naNum(hold->isDistance());
        } else if (!strcmp(fieldName, "hold_is_time")) {
            *out = naNum(!hold->isDistance());
        } else if (!strcmp(fieldName, "hold_inbound_radial")) {
            *out = naNum(hold->inboundRadial());
        } else if (!strcmp(fieldName, "hold_heading_radial_deg")) {
            *out = naNum(hold->inboundRadial());
        } else if (!strcmp(fieldName, "hold_time_or_distance")) {
            // This is the leg length, defined either as a time in seconds, or a
            // distance in nm.
            *out = naNum(hold->timeOrDistance());
        } else {
            return nullptr; // member not found
        }
    } else {
        return nullptr; // member not found
    }

    return "";
}

static bool waypointCommonSetMember(naContext c, Waypt* wpt, const char* fieldName, naRef value)
{
    if (!strcmp(fieldName, "wp_role")) {
        if (!naIsString(value)) naRuntimeError(c, "wp_role must be a string");
        if (wpt->owner() != NULL) naRuntimeError(c, "cannot override wp_role on waypoint with parent");
        WayptFlag f = wayptFlagFromString(naStr_data(value));
        if (f == 0) {
            naRuntimeError(c, "unrecognized wp_role value %s", naStr_data(value));
        }

        wpt->setFlag(f, true);
    } else if (!strcmp(fieldName, "fly_type")) {
        if (!naIsString(value)) naRuntimeError(c, "fly_type must be a string");
        bool flyOver = (strcmp(naStr_data(value), "flyOver") == 0);
        wpt->setFlag(WPT_OVERFLIGHT, flyOver);
    } else if (!strcmp(fieldName, "hidden")) {
        if (!naIsNum(value)) naRuntimeError(c, "wpt.hidden must be a number");
        wpt->setFlag(WPT_HIDDEN, static_cast<int>(value.num) != 0);
    } else if (wpt->type() == "hold") {
        const auto hold = static_cast<Hold*>(wpt);
        if (!strcmp(fieldName, "hold_heading_radial_deg")) {
            if (!naIsNum(value)) naRuntimeError(c, "set hold_heading_radial_deg: invalid hold radial");
            hold->setHoldRadial(value.num);
        } else if (!strcmp("hold_is_left_handed", fieldName)) {
            bool leftHanded = static_cast<int>(value.num) > 0;
            if (leftHanded) {
                hold->setLeftHanded();
            } else {
                hold->setRightHanded();
            }
        }
    } else {
        // nothing changed
        return false;
    }

    return true;
}

static const char* wayptGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
    const char* fieldName = naStr_data(field);
    Waypt*      wpt = (flightgear::Waypt*)g;
    return waypointCommonGetMember(c, wpt, fieldName, out);
}

static RouteRestriction routeRestrictionFromArg(naRef arg)
{
    if (naIsNil(arg) || !naIsString(arg)) {
        return RESTRICT_NONE;
    }

    const std::string u = simgear::strutils::lowercase(naStr_data(arg));
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

// navaid() method of FPLeg ghosts
static naRef f_fpLeg_navaid(naContext c, naRef me, int argc, naRef* args)
{
    flightgear::Waypt* w = wayptGhost(me);
    if (!w) {
        naRuntimeError(c,
                       "flightplan-leg.navaid() called, but can't find the "
                       "underlying waypoint for the flightplan-leg object");
    }

    return waypointNavaid(c, w);
}

// airport() method of FPLeg ghosts
static naRef f_fpLeg_airport(naContext c, naRef me, int argc, naRef* args)
{
    flightgear::Waypt* w = wayptGhost(me);
    if (!w) {
        naRuntimeError(c,
                       "flightplan-leg.airport() called, but can't find the "
                       "underlying waypoint for the flightplan-leg object");
    }

    return waypointAirport(c, w);
}

// runway() method of FPLeg ghosts
static naRef f_fpLeg_runway(naContext c, naRef me, int argc, naRef* args)
{
    flightgear::Waypt* w = wayptGhost(me);
    if (!w) {
        naRuntimeError(c,
                       "flightplan-leg.runway() called, but can't find the "
                       "underlying waypoint for the flightplan-leg object");
    }

    return waypointRunway(c, w);
}

static const char* legGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
    const char*      fieldName = naStr_data(field);
    FlightPlan::Leg* leg = (FlightPlan::Leg*)g;
    if (!leg) {
        *out = naNil();
        naRuntimeError(c, "leg ghost member fetched, but no associated leg object found");
        return "";
    }

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
    } else if (!strcmp(fieldName, "airport")) {
        *out = naNewFunc(c, naNewCCode(c, f_fpLeg_airport));
    } else if (!strcmp(fieldName, "navaid")) {
        *out = naNewFunc(c, naNewCCode(c, f_fpLeg_navaid));
    } else if (!strcmp(fieldName, "runway")) {
        *out = naNewFunc(c, naNewCCode(c, f_fpLeg_runway));
    } else if (!strcmp(fieldName, "hold_count")) {
        *out = naNum(leg->holdCount());
    } else {       // check for fields defined on the underlying waypoint
        if (wpt) { // FIXME null check shouldn't be required, check refcount
            return waypointCommonGetMember(c, wpt, fieldName, out);
        } else {
            naRuntimeError(c, "leg ghost member fetched, but no underlying waypoint object found");
        }
    }

    return ""; // success
}

static void waypointGhostSetMember(naContext c, void* g, naRef field, naRef value)
{
    const char* fieldName = naStr_data(field);
    Waypt*      wpt = (Waypt*)g;
    waypointCommonSetMember(c, wpt, fieldName, value);
}

static void legGhostSetMember(naContext c, void* g, naRef field, naRef value)
{
    const char*      fieldName = naStr_data(field);
    FlightPlan::Leg* leg = (FlightPlan::Leg*)g;

    bool didChange = false;
    if (!strcmp(fieldName, "hold_count")) {
        const int count = static_cast<int>(value.num);
        // this may upgrade the waypoint to a hold
        if (!leg->setHoldCount(count))
            naRuntimeError(c, "unable to set hold on leg waypoint: maybe unsuitable waypt type?");
    } else if (!strcmp(fieldName, "hold_heading_radial_deg")) {
        if (!leg->convertWaypointToHold())
            naRuntimeError(c, "couldn't convert leg waypoint into a hold");

        // now we can call the base method
        didChange = waypointCommonSetMember(c, leg->waypoint(), fieldName, value);
    } else {
        didChange = waypointCommonSetMember(c, leg->waypoint(), fieldName, value);
    }

    if (didChange) {
        leg->markWaypointDirty();
    }
}

static const char* flightplanGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
    const char* fieldName = naStr_data(field);
    FlightPlan* fp = static_cast<FlightPlan*>(g);

    if (!strcmp(fieldName, "parents")) {
        *out = naNewVector(c);
        naVec_append(*out, flightplanPrototype);
    } else if (!strcmp(fieldName, "id"))
        *out = stringToNasal(c, fp->ident());
    else if (!strcmp(fieldName, "departure"))
        *out = ghostForAirport(c, fp->departureAirport());
    else if (!strcmp(fieldName, "destination"))
        *out = ghostForAirport(c, fp->destinationAirport());
    else if (!strcmp(fieldName, "departure_runway"))
        *out = ghostForRunway(c, fp->departureRunway());
    else if (!strcmp(fieldName, "destination_runway"))
        *out = ghostForRunway(c, fp->destinationRunway());
    else if (!strcmp(fieldName, "sid"))
        *out = ghostForProcedure(c, fp->sid());
    else if (!strcmp(fieldName, "sid_trans"))
        *out = ghostForProcedure(c, fp->sidTransition());
    else if (!strcmp(fieldName, "star"))
        *out = ghostForProcedure(c, fp->star());
    else if (!strcmp(fieldName, "star_trans"))
        *out = ghostForProcedure(c, fp->starTransition());
    else if (!strcmp(fieldName, "approach"))
        *out = ghostForProcedure(c, fp->approach());
    else if (!strcmp(fieldName, "approach_trans"))
        *out = ghostForProcedure(c, fp->approachTransition());
    else if (!strcmp(fieldName, "current"))
        *out = naNum(fp->currentIndex());
    else if (!strcmp(fieldName, "aircraftCategory"))
        *out = stringToNasal(c, fp->icaoAircraftCategory());
    else if (!strcmp(fieldName, "followLegTrackToFix"))
        *out = naNum(fp->followLegTrackToFixes());
    else if (!strcmp(fieldName, "active"))
        *out = naNum(fp->isActive());
    else if (!strcmp(fieldName, "cruiseAltitudeFt"))
        *out = naNum(fp->cruiseAltitudeFt());
    else if (!strcmp(fieldName, "cruiseFlightLevel"))
        *out = naNum(fp->cruiseFlightLevel());
    else if (!strcmp(fieldName, "cruiseSpeedKt"))
        *out = naNum(fp->cruiseSpeedKnots());
    else if (!strcmp(fieldName, "cruiseSpeedMach"))
        *out = naNum(fp->cruiseSpeedMach());
    else if (!strcmp(fieldName, "remarks"))
        *out = stringToNasal(c, fp->remarks());
    else if (!strcmp(fieldName, "callsign"))
        *out = stringToNasal(c, fp->callsign());
    else if (!strcmp(fieldName, "estimatedDurationMins"))
        *out = naNum(fp->estimatedDurationMinutes());
    else if (!strcmp(fieldName, "firstNonDepartureLeg"))
        *out = naNum(fp->indexOfFirstNonDepartureWaypoint());
    else if (!strcmp(fieldName, "firstArrivalLeg"))
        *out = naNum(fp->indexOfFirstArrivalWaypoint());
    else if (!strcmp(fieldName, "firstApproachLeg"))
        *out = naNum(fp->indexOfFirstApproachWaypoint());
    else if (!strcmp(fieldName, "destination_runway_leg"))
        *out = naNum(fp->indexOfDestinationRunwayWaypoint());
    else if (!strcmp(fieldName, "totalDistanceNm"))
        *out = naNum(fp->totalDistanceNm());

    else {
        return nullptr;
    }

    return "";
}

static void flightplanGhostSetMember(naContext c, void* g, naRef field, naRef value)
{
    const char* fieldName = naStr_data(field);
    FlightPlan* fp = static_cast<FlightPlan*>(g);

    if (!strcmp(fieldName, "id")) {
        if (!naIsString(value)) naRuntimeError(c, "flightplan.id must be a string");
        fp->setIdent(naStr_data(value));
    } else if (!strcmp(fieldName, "current")) {
        int index = static_cast<int>(value.num);
        if ((index < -1) || (index >= fp->numLegs())) {
            naRuntimeError(c, "flightplan.current must be a valid index or -1");
        }
        fp->setCurrentIndex(index);
    } else if (!strcmp(fieldName, "departure")) {
        FGAirport* apt = airportGhost(value);
        if (apt) {
            fp->setDeparture(apt);
            return;
        }

        FGRunway* rwy = runwayGhost(value);
        if (rwy) {
            fp->setDeparture(rwy);
            return;
        }

        if (naIsNil(value)) {
            fp->clearDeparture();
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
        if (rwy) {
            fp->setDestination(rwy);
            return;
        }

        if (naIsNil(value)) {
            fp->clearDestination();
            return;
        }

        naRuntimeError(c, "bad argument type setting destination");
    } else if (!strcmp(fieldName, "departure_runway")) {
        FGRunway* rwy = runwayGhost(value);
        if (rwy) {
            fp->setDeparture(rwy);
            return;
        }

        naRuntimeError(c, "bad argument type setting departure runway");
    } else if (!strcmp(fieldName, "destination_runway")) {
        if (naIsNil(value)) {
            fp->setDestination(static_cast<FGRunway*>(nullptr));
            return;
        }

        FGRunway* rwy = runwayGhost(value);
        if (rwy) {
            fp->setDestination(rwy);
            return;
        }

        naRuntimeError(c, "bad argument type setting destination runway");
    } else if (!strcmp(fieldName, "sid")) {
        Procedure* proc = procedureGhost(value);
        if (proc && (proc->type() == PROCEDURE_SID)) {
            fp->setSID((flightgear::SID*)proc);
            return;
        }
        // allow a SID transition to be set, implicitly include the SID itself
        if (proc && (proc->type() == PROCEDURE_TRANSITION)) {
            fp->setSID((Transition*)proc);
            return;
        }

        if (naIsString(value)) {
            const std::string s(naStr_data(value));
            FGAirport*        apt = fp->departureAirport();
            auto              sid = apt->findSIDWithIdent(s);
            if (!sid) {
                naRuntimeError(c, "Unknown SID %s at %s", s.c_str(), apt->ident().c_str());
            }

            fp->setSID(sid);
            return;
        }

        if (naIsNil(value)) {
            fp->clearSID();
            return;
        }

        naRuntimeError(c, "bad argument type setting SID");
    } else if (!strcmp(fieldName, "sid_trans")) {
        Procedure* proc = procedureGhost(value);
        if (proc && (proc->type() == PROCEDURE_TRANSITION)) {
            fp->setSID((Transition*)proc);
            return;
        }

        if (naIsNil(value)) {
            fp->setSID(fp->sid(), string{});
            return;
        }

        if (naIsString(value)) {
            const std::string s(naStr_data(value));
            Transition*       trans = nullptr;

            if (fp->sid()) {
                trans = fp->sid()->findTransitionByName(s);
                if (!trans) {
                    naRuntimeError(c, "No such transition %s for SID %s at %s",
                                   s.c_str(),
                                   fp->sid()->ident().c_str(),
                                   fp->departureAirport()->ident().c_str());
                }
            } else {
                trans = fp->departureAirport()->selectSIDByTransition(fp->departureRunway(), s);
                if (!trans) {
                    naRuntimeError(c, "Couldn't find SID transition to %s at %s",
                                   s.c_str(),
                                   fp->departureAirport()->ident().c_str());
                }
            }

            if (trans) {
                fp->setSID(trans);
            }

            return;
        }

        naRuntimeError(c, "bad argument type setting sid_trans");
    } else if (!strcmp(fieldName, "star")) {
        Procedure* proc = procedureGhost(value);
        if (proc && (proc->type() == PROCEDURE_STAR)) {
            fp->setSTAR((STAR*)proc);
            return;
        }

        if (proc && (proc->type() == PROCEDURE_TRANSITION)) {
            fp->setSTAR((Transition*)proc);
            return;
        }

        if (naIsString(value)) {
            const std::string s(naStr_data(value));
            FGAirport*        apt = fp->destinationAirport();
            auto              star = apt->findSTARWithIdent(s);
            if (!star) {
                naRuntimeError(c, "Unknown SID %s at %s", s.c_str(), apt->ident().c_str());
            }
            fp->setSTAR(star);
            return;
        }

        if (naIsNil(value)) {
            fp->clearSTAR();
            return;
        }

        naRuntimeError(c, "bad argument type setting STAR");
    } else if (!strcmp(fieldName, "star_trans")) {
        Procedure* proc = procedureGhost(value);
        if (proc && (proc->type() == PROCEDURE_TRANSITION)) {
            fp->setSTAR((Transition*)proc);
            return;
        }

        if (naIsNil(value)) {
            fp->setSTAR(fp->star(), string{});
            return;
        }

        if (naIsString(value)) {
            const std::string s(naStr_data(value));
            Transition*       trans = nullptr;

            if (fp->star()) {
                trans = fp->star()->findTransitionByName(s);
                if (!trans) {
                    naRuntimeError(c, "No such transition %s for STAR %s at %s",
                                   s.c_str(),
                                   fp->star()->ident().c_str(),
                                   fp->destinationAirport()->ident().c_str());
                }
            } else {
                trans = fp->destinationAirport()->selectSTARByTransition(fp->destinationRunway(), s);
                if (!trans) {
                    naRuntimeError(c, "Couldn't find STAR transition to %s at %s",
                                   s.c_str(),
                                   fp->destinationAirport()->ident().c_str());
                }
            }

            if (trans) {
                fp->setSTAR(trans);
            }

            return;
        }

        naRuntimeError(c, "bad argument type setting star_trans");
    } else if (!strcmp(fieldName, "approach")) {
        Procedure* proc = procedureGhost(value);
        if (proc && Approach::isApproach(proc->type())) {
            fp->setApproach((Approach*)proc);
            return;
        }

        if (proc && (proc->type() == PROCEDURE_TRANSITION)) {
            fp->setApproach((Transition*)proc);
            return;
        }

        if (naIsString(value)) {
            FGAirport* apt = fp->destinationAirport();
            fp->setApproach(apt->findApproachWithIdent(naStr_data(value)));
            return;
        }

        if (naIsNil(value)) {
            fp->setApproach(static_cast<Approach*>(nullptr));
            return;
        }

        naRuntimeError(c, "bad argument type setting approach");
    } else if (!strcmp(fieldName, "approach_trans")) {
        Procedure* proc = procedureGhost(value);
        if (proc && (proc->type() == PROCEDURE_TRANSITION)) {
            fp->setApproach((Transition*)proc);
            return;
        }

        if (naIsNil(value)) {
            fp->setApproach(fp->approach(), string{});
            return;
        }

        if (naIsString(value)) {
            const std::string s(naStr_data(value));
            Transition* trans = nullptr;

            if (fp->approach()) {
                trans = fp->approach()->findTransitionByName(s);
                if (!trans) {
                    naRuntimeError(c, "No such transition %s for approach %s at %s",
                                   s.c_str(),
                                   fp->approach()->ident().c_str(),
                                   fp->destinationAirport()->ident().c_str());
                }
            } else {
                naRuntimeError(c, "No approach selected, can't set approach_trans");
            }

            if (trans) {
                fp->setApproach(trans);
            }

            return;
        }

        naRuntimeError(c, "bad argument type setting approach_trans");
    } else if (!strcmp(fieldName, "aircraftCategory")) {
        if (!naIsString(value)) naRuntimeError(c, "aircraftCategory must be a string");
        fp->setIcaoAircraftCategory(naStr_data(value));
    } else if (!strcmp(fieldName, "followLegTrackToFix")) {
        fp->setFollowLegTrackToFixes(static_cast<bool>(value.num));
    } else if (!strcmp(fieldName, "cruiseAltitudeFt")) {
        fp->setCruiseAltitudeFt(static_cast<int>(value.num));
    } else if (!strcmp(fieldName, "cruiseFlightLevel")) {
        fp->setCruiseFlightLevel(static_cast<int>(value.num));
    } else if (!strcmp(fieldName, "cruiseSpeedKt")) {
        fp->setCruiseSpeedKnots(static_cast<int>(value.num));
    } else if (!strcmp(fieldName, "cruiseSpeedMach")) {
        fp->setCruiseSpeedMach(value.num);
    } else if (!strcmp(fieldName, "callsign")) {
        if (!naIsString(value)) naRuntimeError(c, "flightplan.callsign must be a string");
        fp->setCallsign(naStr_data(value));
    } else if (!strcmp(fieldName, "remarks")) {
        if (!naIsString(value)) naRuntimeError(c, "flightplan.remarks must be a string");
        fp->setRemarks(naStr_data(value));
    } else if (!strcmp(fieldName, "estimatedDurationMins")) {
        fp->setEstimatedDurationMinutes(static_cast<int>(value.num));
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
    Procedure*  proc = (Procedure*)g;

    if (!strcmp(fieldName, "parents")) {
        *out = naNewVector(c);
        naVec_append(*out, procedurePrototype);
    } else if (!strcmp(fieldName, "id"))
        *out = stringToNasal(c, proc->ident());
    else if (!strcmp(fieldName, "airport"))
        *out = ghostForAirport(c, proc->airport());
    else if (!strcmp(fieldName, "tp_type"))
        *out = procedureTpType(c, proc->type());
    else if (!strcmp(fieldName, "radio"))
        *out = procedureRadioType(c, proc->type());
    else if (!strcmp(fieldName, "runways")) {
        *out = naNewVector(c);
        for (FGRunwayRef rwy : proc->runways()) {
            naVec_append(*out, stringToNasal(c, rwy->ident()));
        }
    } else if (!strcmp(fieldName, "transitions")) {
        const auto ty = proc->type();
        string_list idents;
        if (Approach::isApproach(ty)) {
            const auto app = static_cast<Approach*>(proc);
            idents = app->transitionIdents();
        } else if ((ty == PROCEDURE_SID) || (ty == PROCEDURE_STAR)) {
            ArrivalDeparture* ad = static_cast<ArrivalDeparture*>(proc);
            idents = ad->transitionIdents();
        } else {
            *out = naNil();
            return "";
        }

        *out = naNewVector(c);
        for (std::string id : idents) {
            naVec_append(*out, stringToNasal(c, id));
        }
    } else {
        return 0;
    }

    return "";
}

static const char* airwayGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
    const char* fieldName = naStr_data(field);
    Airway*     awy = (Airway*)g;

    if (!strcmp(fieldName, "parents")) {
        *out = naNewVector(c);
        naVec_append(*out, airwayPrototype);
    } else if (!strcmp(fieldName, "id"))
        *out = stringToNasal(c, awy->ident());
    else if (!strcmp(fieldName, "level")) {
        const auto level = awy->level();
        switch (level) {
        case Airway::HighLevel: *out = stringToNasal(c, "high"); break;
        case Airway::LowLevel: *out = stringToNasal(c, "low"); break;
        case Airway::Both: *out = stringToNasal(c, "both"); break;
        default: *out = naNil();
        }
    } else {
        return 0;
    }

    return "";
}

static naRef f_createFlightplan(naContext c, naRef me, int argc, naRef* args)
{
    flightgear::FlightPlanRef fp(new flightgear::FlightPlan);

    if ((argc > 0) && naIsString(args[0])) {
        SGPath path(naStr_data(args[0]));
        if (!path.exists()) {
            std::string pdata = path.utf8Str();
            naRuntimeError(c, "createFlightplan, no file at path %s", pdata.c_str());
        }

        if (!fp->load(path)) {
            SG_LOG(SG_NASAL, SG_WARN, "failed to load flight-plan from " << path);
            return naNil();
        }
    }

    return ghostForFlightPlan(c, fp.get());
}

static naRef f_flightplan(naContext c, naRef me, int argc, naRef* args)
{
    if (argc == 0) {
        FGRouteMgr* rm = static_cast<FGRouteMgr*>(globals->get_subsystem("route-manager"));
        return ghostForFlightPlan(c, rm->flightPlan());
    }

    if ((argc > 0) && naIsString(args[0])) {
        return f_createFlightplan(c, me, argc, args);
    }

    naRuntimeError(c, "bad arguments to flightplan()");
    return naNil();
}

class NasalFPDelegate : public FlightPlan::Delegate
{
public:
    NasalFPDelegate(FlightPlan* fp, FGNasalSys* sys, naRef ins) : _nasal(sys),
                                                                  _plan(fp),
                                                                  _instance(ins)
    {
        assert(fp);
        assert(sys);
        _gcSaveKey = _nasal->gcSave(ins);
    }

    ~NasalFPDelegate() override
    {
        _nasal->gcRelease(_gcSaveKey);
    }

    void departureChanged() override
    {
        callDelegateMethod("departureChanged");
    }

    void arrivalChanged() override
    {
        callDelegateMethod("arrivalChanged");
    }

    void waypointsChanged() override
    {
        callDelegateMethod("waypointsChanged");
    }

    void currentWaypointChanged() override
    {
        callDelegateMethod("currentWaypointChanged");
    }

    void cleared() override
    {
        callDelegateMethod("cleared");
    }

    void endOfFlightPlan() override
    {
        callDelegateMethod("endOfFlightPlan");
    }

    void activated() override
    {
        callDelegateMethod("activated");
    }

    void sequence() override
    {
        callDelegateMethod("sequence");
    }

    void loaded() override
    {
        callDelegateMethod("loaded");
    }

private:
    void callDelegateMethod(const char* method)
    {
        naRef     f;
        naContext ctx = naNewContext();

        if (naMember_cget(ctx, _instance, method, &f) != 0) {
            naRef arg[1];
            arg[0] = ghostForFlightPlan(ctx, _plan);
            _nasal->callMethod(f, _instance, 1, arg, naNil());
        }

        naFreeContext(ctx);
    }

    FGNasalSys* _nasal;
    FlightPlan* _plan;
    naRef       _instance;
    int         _gcSaveKey;
};


class NasalFPDelegateFactory : public FlightPlan::DelegateFactory
{
public:
    NasalFPDelegateFactory(naRef code, const std::string& id) : _id(id)
    {
        _nasal = globals->get_subsystem<FGNasalSys>();
        _func = code;
        _gcSaveKey = _nasal->gcSave(_func);
    }

    virtual ~NasalFPDelegateFactory()
    {
        _nasal->gcRelease(_gcSaveKey);
    }

    FlightPlan::Delegate* createFlightPlanDelegate(FlightPlan* fp) override
    {
        naRef     args[1];
        naContext ctx = naNewContext();
        args[0] = ghostForFlightPlan(ctx, fp);
        naRef instance = _nasal->call(_func, 1, args, naNil());

        FlightPlan::Delegate* result = nullptr;
        if (!naIsNil(instance)) {
            // will GC-save instance
            result = new NasalFPDelegate(fp, _nasal, instance);
        }

        naFreeContext(ctx);
        return result;
    }

    void destroyFlightPlanDelegate(FlightPlan* fp, FlightPlan::Delegate* d) override
    {
        delete d;
    }

    const std::string& id() const
    {
        return _id;
    }

private:
    const std::string _id;
    FGNasalSys*       _nasal;
    naRef             _func;
    int               _gcSaveKey;
};

static std::vector<FlightPlan::DelegateFactoryRef> static_nasalDelegateFactories;

void shutdownNasalFlightPlan()
{
    for (auto f : static_nasalDelegateFactories) {
        FlightPlan::unregisterDelegateFactory(f);
    }
    static_nasalDelegateFactories.clear();
}

static naRef f_registerFPDelegate(naContext c, naRef me, int argc, naRef* args)
{
    if ((argc < 1) || !naIsFunc(args[0])) {
        naRuntimeError(c, "non-function argument to registerFlightPlanDelegate");
    }

    const std::string delegateId = (argc > 1) ? naStr_data(args[1]) : std::string{};
    if (!delegateId.empty()) {
        auto it = std::find_if(static_nasalDelegateFactories.begin(), static_nasalDelegateFactories.end(),
                               [delegateId](FlightPlan::DelegateFactoryRef factory) {
                                   auto nfpd = static_cast<NasalFPDelegateFactory*>(factory.get());
                                   return nfpd->id() == delegateId;
                               });
        if (it != static_nasalDelegateFactories.end()) {
            naRuntimeError(c, "duplicate delegate ID at registerFlightPlanDelegate: %s", delegateId.c_str());
        }
    }

    auto factory = std::make_shared<NasalFPDelegateFactory>(args[0], delegateId);
    FlightPlan::registerDelegateFactory(factory);
    static_nasalDelegateFactories.push_back(factory);
    return naNil();
}

static naRef f_unregisterFPDelegate(naContext c, naRef me, int argc, naRef* args)
{
    if ((argc < 1) || !naIsString(args[0])) {
        naRuntimeError(c, "non-string argument to unregisterFlightPlanDelegate");
    }

    const std::string delegateId = naStr_data(args[0]);
    auto              it = std::find_if(static_nasalDelegateFactories.begin(), static_nasalDelegateFactories.end(),
                           [delegateId](FlightPlan::DelegateFactoryRef factory) {
                               auto nfpd = static_cast<NasalFPDelegateFactory*>(factory.get());
                               return nfpd->id() == delegateId;
                           });

    if (it == static_nasalDelegateFactories.end()) {
        SG_LOG(SG_NASAL, SG_DEV_WARN, "f_unregisterFPDelegate: no delegate with ID:" << delegateId);
        return naNil();
    }

    FlightPlan::unregisterDelegateFactory(*it);
    static_nasalDelegateFactories.erase(it);

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
        return new RunwayWaypt((FGRunway*)pos, NULL);
    }

    return new NavaidWaypoint(pos, NULL);
}

static naRef convertWayptVecToNasal(naContext c, const WayptVec& wps)
{
    naRef result = naNewVector(c);
    for (WayptRef wpt : wps) {
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

static naRef f_findAirway(naContext c, naRef me, int argc, naRef* args)
{
    if ((argc < 1) || !naIsString(args[0])) {
        naRuntimeError(c, "findAirway needs at least one string arguments");
    }

    std::string     ident = naStr_data(args[0]);
    FGPositionedRef pos;
    Airway::Level   level = Airway::Both;
    if (argc >= 2) {
        pos = positionedFromArg(args[1]);
        if (naIsString(args[1])) {
            // level spec,
        }
    }

    AirwayRef awy;
    if (pos) {
        SG_LOG(SG_NASAL, SG_INFO, "Pevious navaid for airway():" << pos->ident());
        awy = Airway::findByIdentAndNavaid(ident, pos);
    } else {
        awy = Airway::findByIdent(ident, level);
    }

    if (!awy)
        return naNil();

    return ghostForAirway(c, awy.get());
}


static naRef f_createWP(naContext c, naRef me, int argc, naRef* args)
{
    SGGeod pos;
    int    argOffset = geodFromArgs(args, 0, argc, pos);

    if (((argc - argOffset) < 1) || !naIsString(args[argOffset])) {
        naRuntimeError(c, "createWP: no identifier supplied");
    }

    std::string ident = naStr_data(args[argOffset++]);
    WayptRef    wpt = new BasicWaypt(pos, ident, NULL);

    // set waypt flags - approach, departure, pseudo, etc
    if (argc > argOffset) {
        WayptFlag f = wayptFlagFromString(naStr_data(args[argOffset++]));
        if (f == 0) {
            naRuntimeError(c, "createWP: bad waypoint role");
        }

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
        naRuntimeError(c, "createWPFrom: couldn't convert arg[0] to FGPositioned");
    }

    WayptRef wpt;
    if (positioned->type() == FGPositioned::RUNWAY) {
        wpt = new RunwayWaypt((FGRunway*)positioned, NULL);
    } else {
        wpt = new NavaidWaypoint(positioned, NULL);
    }

    // set waypt flags - approach, departure, pseudo, etc
    if (argc > 1) {
        WayptFlag f = wayptFlagFromString(naStr_data(args[1]));
        if (f == 0) {
            naRuntimeError(c, "createWPFrom: bad waypoint role");
        }
        wpt->setFlag(f);
    }

    return ghostForWaypt(c, wpt);
}

static naRef f_createViaTo(naContext c, naRef me, int argc, naRef* args)
{
    if (argc != 2) {
        naRuntimeError(c, "createViaTo: needs exactly two arguments");
    }

    std::string airwayName = naStr_data(args[0]);
    AirwayRef   airway = Airway::findByIdent(airwayName, Airway::Both);
    if (!airway) {
        naRuntimeError(c, "createViaTo: couldn't find airway with provided name: %s",
                       naStr_data(args[0]));
    }

    FGPositionedRef nav;
    if (naIsString(args[1])) {
        WayptRef enroute = airway->findEnroute(naStr_data(args[1]));
        if (!enroute) {
            naRuntimeError(c, "unknown waypoint on airway %s: %s",
                           naStr_data(args[0]), naStr_data(args[1]));
        }

        nav = enroute->source();
    } else {
        nav = positionedGhost(args[1]);
        if (!nav) {
            naRuntimeError(c, "createViaTo: arg[1] is not a navaid");
        }
    }

    if (!airway->containsNavaid(nav)) {
        naRuntimeError(c, "createViaTo: navaid not on airway");
    }

    Via* via = new Via(nullptr, airway, nav);
    return ghostForWaypt(c, via);
}

static naRef f_createViaFromTo(naContext c, naRef me, int argc, naRef* args)
{
    if (argc != 3) {
        naRuntimeError(c, "createViaFromTo: needs exactly three arguments");
    }

    auto from = positionedFromArg(args[0]);
    if (!from) {
        naRuntimeError(c, "createViaFromTo: from wp not found");
    }

    std::string airwayName = naStr_data(args[1]);
    AirwayRef   airway = Airway::findByIdentAndNavaid(airwayName, from);
    if (!airway) {
        naRuntimeError(c, "createViaFromTo: couldn't find airway with provided name: %s from wp %s",
                       naStr_data(args[0]),
                       from->ident().c_str());
    }

    FGPositionedRef nav;
    if (naIsString(args[2])) {
        WayptRef enroute = airway->findEnroute(naStr_data(args[2]));
        if (!enroute) {
            naRuntimeError(c, "unknown waypoint on airway %s: %s",
                           naStr_data(args[1]), naStr_data(args[2]));
        }

        nav = enroute->source();
    } else {
        nav = positionedFromArg(args[2]);
        if (!nav) {
            naRuntimeError(c, "createViaFromTo: arg[2] is not a navaid");
        }
    }

    if (!airway->containsNavaid(nav)) {
        naRuntimeError(c, "createViaFromTo: navaid not on airway");
    }

    Via* via = new Via(nullptr, airway, nav);
    return ghostForWaypt(c, via);
}

static naRef f_createDiscontinuity(naContext c, naRef me, int argc, naRef* args)
{
    return ghostForWaypt(c, new Discontinuity(NULL));
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
        index = (int)naNumValue(args[0]).num;
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

    int index = fp->currentIndex();
    if (argc > 0) {
        index += static_cast<int>(naNumValue(args[0]).num);
    }

    if ((index < 0) || (index >= fp->numLegs())) {
        return naNil();
    }

    return ghostForLeg(c, fp->legAtIndex(index));
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

static naRef f_flightplan_numRemainingWaypoints(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "flightplan.f_flightplan_numRemainingWaypoints called on non-flightplan object");
    }

    // for an inactive flightplan, just reutnr the total number of WPs
    if (fp->currentIndex() < 0) {
        return naNum(fp->numLegs());
    }

    return naNum(fp->numLegs() - fp->currentIndex());
}

static naRef f_flightplan_appendWP(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "flightplan.appendWP called on non-flightplan object");
    }

    WayptRef wp = wayptGhost(args[0]);
    int      index = fp->numLegs();
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
    int      index = -1; // append
    if ((argc > 1) && naIsNum(args[1])) {
        index = (int)args[1].num;
    }

    auto leg = fp->insertWayptAtIndex(wp.get(), index);
    return ghostForLeg(c, leg);
}

static naRef f_flightplan_insertWPAfter(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "flightplan.insertWPAfter called on non-flightplan object");
    }

    WayptRef wp = wayptGhost(args[0]);
    int      index = -1; // append
    if ((argc > 1) && naIsNum(args[1])) {
        index = (int)args[1].num;
    }

    auto leg = fp->insertWayptAtIndex(wp.get(), index + 1);
    return ghostForLeg(c, leg);
}

static naRef f_flightplan_insertWaypoints(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "flightplan.insertWaypoints called on non-flightplan object");
    }

    // don't warn when passing a nil to this, which can happen in certain
    // procedure construction situations
    if (naIsNil(args[0])) {
        return naNil();
    }

    WayptVec wps;
    if (!naIsVector(args[0])) {
        naRuntimeError(c, "flightplan.insertWaypoints expects vector as first arg");
    }

    int count = naVec_size(args[0]);
    for (int i = 0; i < count; ++i) {
        Waypt* wp = wayptGhost(naVec_get(args[0], i));
        if (wp) {
            wps.push_back(wp);
        }
    }

    int index = -1; // append
    if ((argc > 1) && naIsNum(args[1])) {
        index = (int)args[1].num;
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

    int index = (int)args[0].num;
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
    if (flag == 0) {
        naRuntimeError(c, "clearWPType: bad waypoint role");
    }

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

static naRef f_flightplan_pathGeod(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "flightplan.pathGeod called on non-flightplan object");
    }

    if ((argc < 1) || !naIsNum(args[0])) {
        naRuntimeError(c, "bad argument to flightplan.pathGeod");
    }

    if ((argc > 1) && !naIsNum(args[1])) {
        naRuntimeError(c, "bad argument to flightplan.pathGeod");
    }

    int    index = (int)args[0].num;
    double offset = (argc > 1) ? args[1].num : 0.0;
    naRef  result = naNewHash(c);
    SGGeod g = fp->pointAlongRoute(index, offset);
    hashset(c, result, "lat", naNum(g.getLatitudeDeg()));
    hashset(c, result, "lon", naNum(g.getLongitudeDeg()));
    return result;
}

static naRef f_flightplan_finish(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "flightplan.finish called on non-flightplan object");
    }

    fp->finish();
    return naNil();
}

static naRef f_flightplan_activate(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "activate called on non-flightplan object");
    }

    fp->activate();
    return naNil();
}


static naRef f_flightplan_indexOfWp(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "flightplan.indexOfWP called on non-flightplan object");
    }

    FGPositioned* positioned = positionedGhost(args[0]);
    if (positioned) {
        return naNum(fp->findWayptIndex(positioned));
    }

    FlightPlan::Leg* leg = fpLegGhost(args[0]);
    if (leg) {
        if (leg->owner() == fp) {
            return naNum(leg->index());
        }

        naRuntimeError(c, "flightplan.indexOfWP called on leg from different flightplan");
    }

    SGGeod pos;
    int    argOffset = geodFromArgs(args, 0, argc, pos);
    if (argOffset > 0) {
        return naNum(fp->findWayptIndex(pos));
    }

    return naNum(-1);
}

static naRef f_flightplan_save(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "save called on non-flightplan object");
    }

    if ((argc < 1) || !naIsString(args[0])) {
        naRuntimeError(c, "flightplan.save, no file path argument");
    }

    SGPath raw_path(naStr_data(args[0]));
    SGPath validated_path = fgValidatePath(raw_path, true);
    if (validated_path.isNull()) {
        naRuntimeError(c, "flightplan.save, writing to path is not permitted");
    }

    bool ok = fp->save(validated_path);
    return naNum(ok);
}

static naRef f_flightplan_parseICAORoute(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "parseICAORoute called on non-flightplan object");
    }

    if ((argc < 1) || !naIsString(args[0])) {
        naRuntimeError(c, "flightplan.parseICAORoute, no route argument");
    }

    bool ok = fp->parseICAORouteString(naStr_data(args[0]));
    return naNum(ok);
}

static naRef f_flightplan_toICAORoute(naContext c, naRef me, int, naRef*)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "toICAORoute called on non-flightplan object");
    }

    return stringToNasal(c, fp->asICAORouteString());
}

static naRef f_flightplan_computeDuration(naContext c, naRef me, int, naRef*)
{
    FlightPlan* fp = flightplanGhost(me);
    if (!fp) {
        naRuntimeError(c, "computeDuration called on non-flightplan object");
    }

    fp->computeDurationMinutes();
    return naNum(fp->estimatedDurationMinutes());
}

static naRef f_leg_setSpeed(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan::Leg* leg = fpLegGhost(me);
    if (!leg) {
        naRuntimeError(c, "leg.setSpeed called on non-flightplan-leg object");
    }

    double           speed = 0.0;
    RouteRestriction rr = RESTRICT_AT;
    if (argc > 0) {
        if (naIsNil(args[0])) {
            // clear the restriction to NONE
            rr = RESTRICT_NONE;
        } else if (convertToNum(args[0], speed)) {
            if ((argc > 1) && naIsString(args[1])) {
                rr = routeRestrictionFromArg(args[1]);
            } else {
                naRuntimeError(c, "bad arguments to setSpeed");
            }
        }

        leg->setSpeed(rr, speed);
    } else {
        naRuntimeError(c, "bad arguments to setSpeed");
    }

    return naNil();
}

static naRef f_leg_setAltitude(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan::Leg* leg = fpLegGhost(me);
    if (!leg) {
        naRuntimeError(c, "leg.setAltitude called on non-flightplan-leg object");
    }

    double           altitude = 0.0;
    RouteRestriction rr = RESTRICT_AT;
    if (argc > 0) {
        if (naIsNil(args[0])) {
            // clear the restriction to NONE
            rr = RESTRICT_NONE;
        } else if (convertToNum(args[0], altitude)) {
            if (argc > 1) {
                rr = routeRestrictionFromArg(args[1]);
            } else {
                naRuntimeError(c, "bad arguments to leg.setAltitude");
            }
        }

        leg->setAltitude(rr, altitude);
    } else {
        naRuntimeError(c, "bad arguments to setleg.setAltitude");
    }

    return naNil();
}

static naRef f_leg_courseAndDistanceFrom(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan::Leg* leg = fpLegGhost(me);
    if (!leg || !leg->owner()) {
        naRuntimeError(c, "leg.courseAndDistanceFrom called on non-flightplan-leg object");
    }

    SGGeod pos;
    geodFromArgs(args, 0, argc, pos);

    RoutePath path(leg->owner());
    SGGeod    wpPos = path.positionForIndex(leg->index());
    double    courseDeg, az2, distanceM;
    SGGeodesy::inverse(pos, wpPos, courseDeg, az2, distanceM);

    naRef result = naNewVector(c);
    naVec_append(result, naNum(courseDeg));
    naVec_append(result, naNum(distanceM * SG_METER_TO_NM));
    return result;
}

static naRef f_leg_path(naContext c, naRef me, int argc, naRef* args)
{
    FlightPlan::Leg* leg = fpLegGhost(me);
    if (!leg || !leg->owner()) {
        naRuntimeError(c, "leg.setAltitude called on non-flightplan-leg object");
    }

    RoutePath path(leg->owner());
    SGGeodVec gv(path.pathForIndex(leg->index()));

    naRef result = naNewVector(c);
    for (SGGeod p : gv) {
        // construct a geo.Coord!
        naRef coord = naNewHash(c);
        hashset(c, coord, "lat", naNum(p.getLatitudeDeg()));
        hashset(c, coord, "lon", naNum(p.getLongitudeDeg()));
        naVec_append(result, coord);
    }

    return result;
}

static naRef f_procedure_transition(naContext c, naRef me, int argc, naRef* args)
{
    Procedure* proc = procedureGhost(me);
    if (!proc) {
        naRuntimeError(c, "procedure.transition called on non-procedure object");
    }

    const string ident{naStr_data(args[0])};
    const auto ty = proc->type();
    if (Approach::isApproach(ty)) {
        const auto app = static_cast<Approach*>(proc);
        const auto trans = app->findTransitionByName(ident);
        return ghostForProcedure(c, trans);
    }

    if ((ty != PROCEDURE_SID) && (ty != PROCEDURE_STAR)) {
        naRuntimeError(c, "procedure.transition called on non-SID or -STAR");
    }

    ArrivalDeparture* ad = (ArrivalDeparture*)proc;
    Transition* trans = ad->findTransitionByName(ident);

    return ghostForProcedure(c, trans);
}

static naRef approachRoute(Approach* app, naContext c, int argc, naRef* args)
{
    int argOffset = 0;
    FGRunway* rwy = runwayGhost(args[0]);
    if (rwy) ++argOffset;

    WayptVec r;
    Transition* trans = nullptr;
    WayptRef iaf;

    if (argOffset < argc) {
        // one of these might work, never both
        trans = (Transition*)procedureGhost(args[argOffset]);
        iaf = wayptFromArg(args[argOffset]);

        if (trans) {
            bool ok = app->routeWithTransition(rwy, trans, r);
            if (!ok)
                return naNil();

            return convertWayptVecToNasal(c, r);
        }
    }

    bool ok = app->route(rwy, iaf, r);
    if (!ok)
        return naNil();

    return convertWayptVecToNasal(c, r);
}

static naRef f_procedure_route(naContext c, naRef me, int argc, naRef* args)
{
    Procedure* proc = procedureGhost(me);
    if (!proc) {
        naRuntimeError(c, "procedure.route called on non-procedure object");
    }

    // wrapping up tow different routines here - approach routing
    // to the associated runway, and SID/STAR routing via an enroute transition
    // and possibly a runway transition or not.
    if (Approach::isApproach(proc->type())) {
        return approachRoute(static_cast<Approach*>(proc), c, argc, args);
    } else if ((proc->type() != PROCEDURE_SID) && (proc->type() != PROCEDURE_STAR)) {
        naRuntimeError(c, "procedure.route called on unsuitable procedure type");
    }

    int       argOffset = 0;
    FGRunway* rwy = runwayGhost(args[0]);
    if (rwy) ++argOffset;

    ArrivalDeparture* ad = (ArrivalDeparture*)proc;
    Transition*       trans = NULL;
    if (argOffset < argc) {
        trans = (Transition*)procedureGhost(args[argOffset]);
    }

    // note either runway or trans may be NULL - that's ok
    WayptVec r;
    if (!ad->route(rwy, trans, r)) {
        SG_LOG(SG_NASAL, SG_WARN, "procedure.route failed for ArrivalDeparture somehow");
        return naNil();
    }

    return convertWayptVecToNasal(c, r);
}

static naRef f_airway_contains(naContext c, naRef me, int argc, naRef* args)
{
    Airway* awy = airwayGhost(me);
    if (!awy) {
        naRuntimeError(c, "airway.contains called on non-airway object");
    }

    if (argc < 1) {
        naRuntimeError(c, "missing arg to airway.contains");
    }

    auto pos = positionedFromArg(args[0]);
    if (!pos) {
        return naNum(0);
    }

    return naNum(awy->containsNavaid(pos));
}

// Table of extension functions.  Terminate with zeros.
static struct {
    const char* name;
    naCFunction func;
} funcs[] = {
    {"flightplan", f_flightplan},
    {"createFlightplan", f_createFlightplan},
    {"registerFlightPlanDelegate", f_registerFPDelegate},
    {"unregisterFlightPlanDelegate", f_unregisterFPDelegate},
    {"createWP", f_createWP},
    {"createWPFrom", f_createWPFrom},
    {"createViaTo", f_createViaTo},
    {"createViaFromTo", f_createViaFromTo},
    {"createDiscontinuity", f_createDiscontinuity},
    {"airwaysRoute", f_airwaySearch},
    {"airway", f_findAirway},
    {nullptr, nullptr}};


naRef initNasalFlightPlan(naRef globals, naContext c)
{
    flightplanPrototype = naNewHash(c);
    naSave(c, flightplanPrototype);

    hashset(c, flightplanPrototype, "getWP", naNewFunc(c, naNewCCode(c, f_flightplan_getWP)));
    hashset(c, flightplanPrototype, "currentWP", naNewFunc(c, naNewCCode(c, f_flightplan_currentWP)));
    hashset(c, flightplanPrototype, "nextWP", naNewFunc(c, naNewCCode(c, f_flightplan_nextWP)));
    hashset(c, flightplanPrototype, "getPlanSize", naNewFunc(c, naNewCCode(c, f_flightplan_numWaypoints)));
    // alias to this name also
    hashset(c, flightplanPrototype, "numWaypoints", naNewFunc(c, naNewCCode(c, f_flightplan_numWaypoints)));
    hashset(c, flightplanPrototype, "numRemainingWaypoints", naNewFunc(c, naNewCCode(c, f_flightplan_numRemainingWaypoints)));

    hashset(c, flightplanPrototype, "appendWP", naNewFunc(c, naNewCCode(c, f_flightplan_appendWP)));
    hashset(c, flightplanPrototype, "insertWP", naNewFunc(c, naNewCCode(c, f_flightplan_insertWP)));
    hashset(c, flightplanPrototype, "deleteWP", naNewFunc(c, naNewCCode(c, f_flightplan_deleteWP)));
    hashset(c, flightplanPrototype, "insertWPAfter", naNewFunc(c, naNewCCode(c, f_flightplan_insertWPAfter)));
    hashset(c, flightplanPrototype, "insertWaypoints", naNewFunc(c, naNewCCode(c, f_flightplan_insertWaypoints)));
    hashset(c, flightplanPrototype, "cleanPlan", naNewFunc(c, naNewCCode(c, f_flightplan_clearPlan)));
    hashset(c, flightplanPrototype, "clearWPType", naNewFunc(c, naNewCCode(c, f_flightplan_clearWPType)));
    hashset(c, flightplanPrototype, "clone", naNewFunc(c, naNewCCode(c, f_flightplan_clone)));
    hashset(c, flightplanPrototype, "pathGeod", naNewFunc(c, naNewCCode(c, f_flightplan_pathGeod)));
    hashset(c, flightplanPrototype, "finish", naNewFunc(c, naNewCCode(c, f_flightplan_finish)));
    hashset(c, flightplanPrototype, "activate", naNewFunc(c, naNewCCode(c, f_flightplan_activate)));
    hashset(c, flightplanPrototype, "indexOfWP", naNewFunc(c, naNewCCode(c, f_flightplan_indexOfWp)));
    hashset(c, flightplanPrototype, "computeDuration", naNewFunc(c, naNewCCode(c, f_flightplan_computeDuration)));
    hashset(c, flightplanPrototype, "parseICAORoute", naNewFunc(c, naNewCCode(c, f_flightplan_parseICAORoute)));
    hashset(c, flightplanPrototype, "toICAORoute", naNewFunc(c, naNewCCode(c, f_flightplan_toICAORoute)));

    hashset(c, flightplanPrototype, "save", naNewFunc(c, naNewCCode(c, f_flightplan_save)));

    procedurePrototype = naNewHash(c);
    naSave(c, procedurePrototype);
    hashset(c, procedurePrototype, "transition", naNewFunc(c, naNewCCode(c, f_procedure_transition)));
    hashset(c, procedurePrototype, "route", naNewFunc(c, naNewCCode(c, f_procedure_route)));

    fpLegPrototype = naNewHash(c);
    naSave(c, fpLegPrototype);
    hashset(c, fpLegPrototype, "setSpeed", naNewFunc(c, naNewCCode(c, f_leg_setSpeed)));
    hashset(c, fpLegPrototype, "setAltitude", naNewFunc(c, naNewCCode(c, f_leg_setAltitude)));
    hashset(c, fpLegPrototype, "path", naNewFunc(c, naNewCCode(c, f_leg_path)));
    hashset(c, fpLegPrototype, "courseAndDistanceFrom", naNewFunc(c, naNewCCode(c, f_leg_courseAndDistanceFrom)));

    airwayPrototype = naNewHash(c);
    naSave(c, airwayPrototype);
    hashset(c, airwayPrototype, "contains", naNewFunc(c, naNewCCode(c, f_airway_contains)));

    for (int i = 0; funcs[i].name; i++) {
        hashset(c, globals, funcs[i].name,
                naNewFunc(c, naNewCCode(c, funcs[i].func)));
    }

    return naNil();
}
