// procedure.cxx - define route storing an approach, arrival or departure procedure
// Written by James Turner, started 2009.
//
// Copyright (C) 2009  Curtis L. Olson
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

#include "procedure.hxx"

#include <cassert>
#include <algorithm> // for reverse_copy

#include <simgear/structure/exception.hxx>

#include <Navaids/waypoint.hxx>

using std::string;

namespace flightgear
{
  
static void markWaypoints(WayptVec& wps, WayptFlag f)
{
  for (unsigned int i=0; i<wps.size(); ++i) {
    wps[i]->setFlag(f, true);
  }
}

Procedure::Procedure(const string& aIdent) :
  _ident(aIdent)
{
}

Approach::Approach(const string& aIdent, ProcedureType ty) : 
  Procedure(aIdent),
  _type(ty)
{

}

void Approach::setRunway(FGRunwayRef aRwy)
{
  _runway = aRwy;
}

FGAirport* Approach::airport() const
{
  return _runway->airport();
}

RunwayVec Approach::runways() const
{
  RunwayVec r;
  r.push_back(_runway);
  return r;
}
  
void Approach::setPrimaryAndMissed(const WayptVec& aPrimary, const WayptVec& aMissed)
{
  _primary = aPrimary;
  _primary[0]->setFlag(WPT_IAF, true);
  _primary[_primary.size()-1]->setFlag(WPT_FAF, true);
  markWaypoints(_primary, WPT_APPROACH);
  
  _missed = aMissed;
  
  if (!_missed.empty()) {
    // mark the first point as the published missed-approach point
    _missed[0]->setFlag(WPT_MAP, true);
    markWaypoints(_missed, WPT_MISS);
    markWaypoints(_missed, WPT_APPROACH);
  }
}

void Approach::addTransition(Transition* aTrans)
{
  WayptRef entry = aTrans->enroute();
  _transitions[entry] = aTrans;
  aTrans->mark(WPT_APPROACH);
}

bool Approach::route(WayptRef aIAF, WayptVec& aWps)
{
  if (aIAF.valid()) {
    WptTransitionMap::iterator it;
    bool haveTrans = false;
    for (it = _transitions.begin(); it != _transitions.end(); ++it) {
      Transition* t= it->second;
      if (t->enroute()->matches(aIAF)) {
        t->route(aWps);
        haveTrans = true;
        break;
      }
    } // of transitions iteration
    
    if (!haveTrans) {
      SG_LOG(SG_GENERAL, SG_INFO, "approach " << ident() << " has no transition " <<
        "for IAF: " << aIAF->ident());
      return false;
    }
  }
  
  return routeFromVectors(aWps);
}

bool Approach::routeFromVectors(WayptVec& aWps)
{
  aWps.insert(aWps.end(), _primary.begin(), _primary.end());
  RunwayWaypt* rwy = new RunwayWaypt(_runway, NULL);
  rwy->setFlag(WPT_APPROACH);
  aWps.push_back(rwy);
  aWps.insert(aWps.end(), _missed.begin(), _missed.end());
  return true;
}

bool Approach::isApproach(ProcedureType ty)
{
  return (ty >= PROCEDURE_APPROACH_ILS) && (ty <= PROCEDURE_APPROACH_RNAV);
}
  
//////////////////////////////////////////////////////////////////////////////

ArrivalDeparture::ArrivalDeparture(const string& aIdent, FGAirport* apt) :
  Procedure(aIdent),
  _airport(apt)
{
}

void ArrivalDeparture::addRunway(FGRunwayRef aWay)
{
  assert(aWay->airport() == _airport);
  _runways[aWay] = NULL;
}

bool ArrivalDeparture::isForRunway(const FGRunway* aWay) const
{
  // null runway always passes
  if (!aWay) {
    return true;
  }
  
  FGRunwayRef r(const_cast<FGRunway*>(aWay));
  return (_runways.count(r));
}

RunwayVec ArrivalDeparture::runways() const
{
  RunwayVec r;
  RunwayTransitionMap::const_iterator it = _runways.begin();
  for (; it != _runways.end(); ++it) {
    r.push_back(it->first);
  }
  
  return r;
}
    
void ArrivalDeparture::addTransition(Transition* aTrans)
{
  WayptRef entry = aTrans->enroute();
  aTrans->mark(flagType());
  _enrouteTransitions[entry] = aTrans;
}

string_list ArrivalDeparture::transitionIdents() const
{
  string_list r;
  WptTransitionMap::const_iterator eit;
  for (eit = _enrouteTransitions.begin(); eit != _enrouteTransitions.end(); ++eit) {
    r.push_back(eit->second->ident());
  }
  return r;
}
  
void ArrivalDeparture::addRunwayTransition(FGRunwayRef aWay, Transition* aTrans)
{
  assert(aWay->ident() == aTrans->ident());
  if (!isForRunway(aWay)) {
    throw sg_io_exception("adding transition for unspecified runway:" + aWay->ident(), ident());
  }
  
  aTrans->mark(flagType());
  _runways[aWay] = aTrans;
}

void ArrivalDeparture::setCommon(const WayptVec& aWps)
{
  _common = aWps;
  markWaypoints(_common, flagType());
}

bool ArrivalDeparture::commonRoute(Transition* t, WayptVec& aPath, FGRunwayRef aRwy)
{
  // assume we're routing from enroute, to the runway.
  // for departures, we'll flip the result points

  WayptVec::iterator firstCommon = _common.begin();
  if (t) {
    t->route(aPath);

    Waypt* transEnd = t->procedureEnd();
    for (; firstCommon != _common.end(); ++firstCommon) {
      if ((*firstCommon)->matches(transEnd)) {
        // found transition->common point, stop search
        break;
      }
    } // of common points
    
    // if we hit this point, the transition doesn't end (start, for a SID) on
    // a common point. We assume this means we should just append the entire
    // common section after the transition.
    firstCommon = _common.begin();
  } else {
    // no tranasition
  } // of not using a transition
  
  // append (some) common points
  aPath.insert(aPath.end(), firstCommon, _common.end());
  
  if (!aRwy) {
    // no runway specified, we're done
    return true;
  }
  
  RunwayTransitionMap::iterator r = _runways.find(aRwy);
  assert(r != _runways.end());
  if (!r->second) {
    // no transitions specified. Not great, but not
    // much we can do about it. Calling code will insert VECTORS to the approach
    // if required, or maybe there's an approach transition defined.
    return true;
  }
  
  SG_LOG(SG_GENERAL, SG_INFO, ident() << " using runway transition for " << r->first->ident());
  r->second->route(aPath);
  return true;
}

Transition* ArrivalDeparture::findTransitionByEnroute(Waypt* aEnroute) const
{
  if (!aEnroute) {
    return NULL;
  }
  
  WptTransitionMap::const_iterator eit;
  for (eit = _enrouteTransitions.begin(); eit != _enrouteTransitions.end(); ++eit) {
    if (eit->second->enroute()->matches(aEnroute)) {
      SG_LOG(SG_GENERAL, SG_INFO, ident() << " using enroute transition " << eit->second->ident());
      return eit->second;
    }
  } // of enroute transition iteration
  
  return NULL;
}

WayptRef ArrivalDeparture::findBestTransition(const SGGeod& aPos) const
{
  // no transitions, that's easy
  if (_enrouteTransitions.empty()) {
    SG_LOG(SG_GENERAL, SG_INFO, "no enroute transitions for " << ident());
    return _common.front();
  }
  
  double d = 1e9;
  WayptRef w;
  WptTransitionMap::const_iterator eit;
  for (eit = _enrouteTransitions.begin(); eit != _enrouteTransitions.end(); ++eit) {
    WayptRef c = eit->second->enroute();
    SG_LOG(SG_GENERAL, SG_INFO, "findBestTransition for " << ident() << ", looking at " << c->ident());
    // assert(c->hasFixedPosition());
    double cd = SGGeodesy::distanceM(aPos, c->position());
    
    if (cd < d) { // distance to 'c' is less, new best match
      d = cd;
      w = c;
    }
  } // of transitions iteration
  
  assert(w);
  return w;
}

Transition* ArrivalDeparture::findTransitionByName(const string& aIdent) const
{
  WptTransitionMap::const_iterator eit;
  for (eit = _enrouteTransitions.begin(); eit != _enrouteTransitions.end(); ++eit) {
    if (eit->second->ident() == aIdent) {
      return eit->second;
    }
  }
  
  return NULL;
}

////////////////////////////////////////////////////////////////////////////

SID::SID(const string& aIdent, FGAirport* apt) :
    ArrivalDeparture(aIdent, apt)
{
}

bool SID::route(FGRunwayRef aWay, Transition* trans, WayptVec& aPath)
{
  if (!isForRunway(aWay)) {
    SG_LOG(SG_GENERAL, SG_WARN, "SID " << ident() << " not for runway " << aWay->ident());
    return false;
  }
  
  WayptVec path;
  if (!commonRoute(trans, path, aWay)) {
    return false;
  }
  
  // SID waypoints (including transitions) are stored reversed, so we can
  // re-use the routing code. This is where we fix the ordering for client code
  std::back_insert_iterator<WayptVec> bi(aPath);
  std::reverse_copy(path.begin(), path.end(), bi);

  return true;
}

////////////////////////////////////////////////////////////////////////////

STAR::STAR(const string& aIdent, FGAirport* apt) :
    ArrivalDeparture(aIdent, apt)
{
}

bool STAR::route(FGRunwayRef aWay, Transition* trans, WayptVec& aPath)
{
  if (aWay && !isForRunway(aWay)) {
    return false;
  }
    
  return commonRoute(trans, aPath, aWay);
}

/////////////////////////////////////////////////////////////////////////////

Transition::Transition(const std::string& aIdent, ProcedureType ty, Procedure* aPr) :
  Procedure(aIdent),
  _type(ty),
  _parent(aPr)
{
  assert(aPr);
}
  
void Transition::setPrimary(const WayptVec& aWps)
{
  _primary = aWps;
  assert(!_primary.empty());
  _primary[0]->setFlag(WPT_TRANSITION, true);
}

WayptRef Transition::enroute() const
{
  assert(!_primary.empty());
  return _primary[0];
}

WayptRef Transition::procedureEnd() const
{
  assert(!_primary.empty());
  return _primary[_primary.size() - 1];
}

bool Transition::route(WayptVec& aPath)
{
  aPath.insert(aPath.end(), _primary.begin(), _primary.end());
  return true;
}

FGAirport* Transition::airport() const
{
  return _parent->airport();
}
  
void Transition::mark(WayptFlag f)
{
  markWaypoints(_primary, f);
}
  
} // of namespace
