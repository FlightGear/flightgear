/// procedure.hxx - define route storing an approach, arrival or departure procedure
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

#ifndef FG_NAVAID_PROCEDURE_HXX
#define FG_NAVAID_PROCEDURE_HXX

#include <set>

#include <simgear/math/sg_types.hxx> // for string_list

#include <Navaids/route.hxx>
#include <Airports/runways.hxx>

typedef SGSharedPtr<FGRunway> FGRunwayRef;

namespace flightgear {

// forward decls
class NavdataVisitor;

typedef std::vector<FGRunwayRef> RunwayVec;
  
typedef enum {
  PROCEDURE_INVALID,
  PROCEDURE_APPROACH_ILS,
  PROCEDURE_APPROACH_VOR,
  PROCEDURE_APPROACH_NDB,
  PROCEDURE_APPROACH_RNAV,
  PROCEDURE_SID,
  PROCEDURE_STAR,
  PROCEDURE_TRANSITION,
  PROCEDURE_RUNWAY_TRANSITION
} ProcedureType;
  
class Procedure : public RouteBase
{
public:  
  virtual ProcedureType type() const = 0;
  
  virtual std::string ident() const
  { return _ident; }
  
  virtual FGAirport* airport() const = 0;
  
  virtual RunwayVec runways() const
  { return RunwayVec(); }
protected:
  Procedure(const std::string& aIdent);
  
  std::string _ident;
};

/**
 * Encapsulate a transition segment
 */
class Transition : public Procedure
{
public:
  bool route(WayptVec& aPath);
  
  Procedure* parent() const
  { return _parent; }
  
  virtual FGAirport* airport() const;
  
  /**
   * Return the enroute end of the transition
   */
  WayptRef enroute() const;
  
  /**
   * Return the procedure end of the transition
   */
  WayptRef procedureEnd() const;
  
  
  virtual ProcedureType type() const
  { return _type; }
  
  void mark(WayptFlag f);
private:
  friend class NavdataVisitor;

  Transition(const std::string& aIdent, ProcedureType ty, Procedure* aPr);

  void setPrimary(const WayptVec& aWps);
  
  ProcedureType _type;
  Procedure* _parent;
  WayptVec _primary;
};

/**
 * Describe an approach procedure, including the missed approach
 * segment
 */
class Approach : public Procedure
{
public:
  FGRunwayRef runway() 
  { return _runway; }

  static bool isApproach(ProcedureType ty);
  
  virtual FGAirport* airport() const;
  
  virtual RunwayVec runways() const;
  
  /**
   * Build a route from a valid IAF to the runway, including the missed
   * segment. Return false if no valid transition from the specified IAF
   * could be found
   */
  bool route(WayptRef aIAF, WayptVec& aWps);

  /**
   * Build route as above, but ignore transitions, and assume radar
   * vectoring to the start of main approach
   */
  bool routeFromVectors(WayptVec& aWps);
  
  const WayptVec& primary() const
    { return _primary; }
    
  const WayptVec& missed() const
    { return _missed; }

  virtual ProcedureType type() const
  { return _type; }
private:
  friend class NavdataVisitor;
  
  Approach(const std::string& aIdent, ProcedureType ty);
  
  void setRunway(FGRunwayRef aRwy);
  void setPrimaryAndMissed(const WayptVec& aPrimary, const WayptVec& aMissed);
  void addTransition(Transition* aTrans);
  
  FGRunwayRef _runway;
  ProcedureType _type;
  
  typedef std::map<WayptRef, Transition*> WptTransitionMap;
  WptTransitionMap _transitions;
  
  WayptVec _primary; // unify these?
  WayptVec _missed;
};

class ArrivalDeparture : public Procedure
{
public:
  virtual FGAirport* airport() const
  { return _airport; }
  
  /**
   * Predicate, test if this procedure applies to the requested runway
   */
  virtual bool isForRunway(const FGRunway* aWay) const;

  virtual RunwayVec runways() const;

  /**
   * Find a path between the runway and enroute structure. Waypoints 
   * corresponding to the appropriate transitions and segments will be created.
   */
  virtual bool route(FGRunwayRef aWay, Transition* trans, WayptVec& aPath) = 0;

  const WayptVec& common() const
    { return _common; }

  string_list transitionIdents() const;
  
  /**
   * Given an enroute location, find the best enroute transition point for 
   * this arrival/departure. Best is currently determined as 'closest to the
   * enroute location'.
   */
  WayptRef findBestTransition(const SGGeod& aPos) const;
  
  /**
   * Find an enroute transition waypoint by identifier. This is necessary
   * for the route-manager and similar code that that needs to talk about
   * transitions in a human-meaningful way (including persistence).
   */
  Transition* findTransitionByName(const std::string& aIdent) const;
  
  Transition* findTransitionByEnroute(Waypt* aEnroute) const;
protected:
    
  bool commonRoute(Transition* t, WayptVec& aPath, FGRunwayRef aRwy);
  
  ArrivalDeparture(const std::string& aIdent, FGAirport* apt);
  
  
  void addRunway(FGRunwayRef aRwy);

  typedef std::map<FGRunwayRef, Transition*> RunwayTransitionMap;
  RunwayTransitionMap _runways;
  
  virtual WayptFlag flagType() const = 0;
private:
  friend class NavdataVisitor;
  
  void addTransition(Transition* aTrans);
  
  void setCommon(const WayptVec& aWps);

  void addRunwayTransition(FGRunwayRef aRwy, Transition* aTrans);
  
  FGAirport* _airport;
  WayptVec _common;
  
  typedef std::map<WayptRef, Transition*> WptTransitionMap;
  WptTransitionMap _enrouteTransitions;
  
  
};

class SID : public ArrivalDeparture
{
public:  
  virtual bool route(FGRunwayRef aWay, Transition* aTrans, WayptVec& aPath);
  
  virtual ProcedureType type() const
  { return PROCEDURE_SID; }
  
protected:
  virtual WayptFlag flagType() const
  { return WPT_DEPARTURE; }
  
private:
  friend class NavdataVisitor;
    
  SID(const std::string& aIdent, FGAirport* apt);
};

class STAR : public ArrivalDeparture
{
public:  
  virtual bool route(FGRunwayRef aWay, Transition* aTrans, WayptVec& aPath);
  
  virtual ProcedureType type() const
  { return PROCEDURE_STAR; }
  
protected:
  virtual WayptFlag flagType() const
  { return WPT_ARRIVAL; }
  
private:
  friend class NavdataVisitor;
  
  STAR(const std::string& aIdent, FGAirport* apt);
};

} // of namespace

#endif 
