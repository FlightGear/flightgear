/**
 * FlightPlan.hxx - defines a full flight-plan object, including
 * departure, cruise, arrival information and waypoints
 */
 
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

#ifndef FG_FLIGHTPLAN_HXX
#define FG_FLIGHTPLAN_HXX

#include <Navaids/route.hxx>
#include <Airports/simple.hxx>

typedef SGSharedPtr<FGAirport> FGAirportRef;
    
namespace flightgear
{

class Transition;

class FlightPlan : public RouteBase
{
public:
  FlightPlan();
  virtual ~FlightPlan();
  
  virtual std::string ident() const;
  void setIdent(const std::string& s);
  
  FlightPlan* clone(const std::string& newIdent = std::string()) const;
  
  /**
   * flight-plan leg encapsulation
   */
  class Leg
  {
  public:
    FlightPlan* owner() const
    { return _parent; }
    
    Waypt* waypoint() const
    { return _waypt; }
    
    // reutrn the next leg after this one
    Leg* nextLeg() const;
    
    unsigned int index() const;
    
    int altitudeFt() const;		
    int speed() const;
    
    int speedKts() const;
    double speedMach() const;
    
    RouteRestriction altitudeRestriction() const;    
    RouteRestriction speedRestriction() const;
    
    void setSpeed(RouteRestriction ty, double speed);
    void setAltitude(RouteRestriction ty, int altFt);
    
    double courseDeg() const;
    double distanceNm() const;
    double distanceAlongRoute() const;
  private:
    friend class FlightPlan;
    
    Leg(FlightPlan* owner, WayptRef wpt);
    
    Leg* cloneFor(FlightPlan* owner) const;
    
    FlightPlan* _parent;
    RouteRestriction _speedRestrict, _altRestrict;
    int _speed;
    int _altitudeFt;
    WayptRef _waypt;
    /// length of this leg following the flown path
    mutable double _pathDistance;
    mutable double _courseDeg;
    /// total distance of this leg from departure point
    mutable double _distanceAlongPath; 
  };
  
  class Delegate
  {
  public:
    virtual ~Delegate();
        
    virtual void departureChanged() { }
    virtual void arrivalChanged() { }
    virtual void waypointsChanged() { }
    
    virtual void currentWaypointChanged() { }
  
  protected:
    Delegate();
    
  private:
    void removeInner(Delegate* d);
    
    void runDepartureChanged();
    void runArrivalChanged();
    void runWaypointsChanged();
    void runCurrentWaypointChanged();
    
    friend class FlightPlan;
    
    bool _deleteWithPlan;
    Delegate* _inner;
  };
  
  Leg* insertWayptAtIndex(Waypt* aWpt, int aIndex);
  void insertWayptsAtIndex(const WayptVec& wps, int aIndex);
  
  void deleteIndex(int index);
  void clear();
  int clearWayptsWithFlag(WayptFlag flag);
  
  int currentIndex() const
  { return _currentIndex; }
  
  void setCurrentIndex(int index);
  
  Leg* currentLeg() const;
  Leg* nextLeg() const;
  Leg* previousLeg() const;
  
  int numLegs() const
  { return _legs.size(); }
  
  Leg* legAtIndex(int index) const;
  int findLegIndex(const Leg* l) const;
  
  int findWayptIndex(const SGGeod& aPos) const;
  
  bool load(const SGPath& p);
  bool save(const SGPath& p);
  
  FGAirportRef departureAirport() const
  { return _departure; }
  
  FGAirportRef destinationAirport() const
  { return _destination; }
  
  FGRunway* departureRunway() const
  { return _departureRunway; }
  
  FGRunway* destinationRunway() const
  { return _destinationRunway; }
  
  Approach* approach() const
  { return _approach; }
  
  void setDeparture(FGAirport* apt);
  void setDeparture(FGRunway* rwy);
  
  SID* sid() const
  { return _sid; }
  
  Transition* sidTransition() const;
  
  void setSID(SID* sid, const std::string& transition = std::string());
  
  void setSID(Transition* sidWithTrans);
  
  void setDestination(FGAirport* apt);
  void setDestination(FGRunway* rwy);
  
  /**
    * note setting an approach will implicitly update the destination
    * airport and runway to match
    */
  void setApproach(Approach* app);
  
  STAR* star() const
  { return _star; }
  
  Transition* starTransition() const;
  
  void setSTAR(STAR* star, const std::string& transition = std::string());
  
  void setSTAR(Transition* starWithTrans);
  
  double totalDistanceNm() const
  { return _totalDistance; }
  
  /**
   * Create a WayPoint from a string in the following format:
   *  - simple identifier
   *  - decimal-lon,decimal-lat
   *  - airport-id/runway-id
   *  - navaid/radial-deg/offset-nm
   */
  WayptRef waypointFromString(const std::string& target);
  
  /**
   * abstract interface for creating delegates automatically when a
   * flight-plan is created or loaded
   */
  class DelegateFactory
  {
  public:
    virtual Delegate* createFlightPlanDelegate(FlightPlan* fp) = 0;
  };
  
  static void registerDelegateFactory(DelegateFactory* df);
  static void unregisterDelegateFactory(DelegateFactory* df);
  
  void addDelegate(Delegate* d);
  void removeDelegate(Delegate* d);
private:
  void lockDelegate();
  void unlockDelegate();
  
  int _delegateLock;
  bool _arrivalChanged, 
    _departureChanged, 
    _waypointsChanged, 
    _currentWaypointChanged;
  
  bool loadPlainTextRoute(const SGPath& path);
  
  void loadVersion1XMLRoute(SGPropertyNode_ptr routeData);
  void loadVersion2XMLRoute(SGPropertyNode_ptr routeData);
  void loadXMLRouteHeader(SGPropertyNode_ptr routeData);
  WayptRef parseVersion1XMLWaypt(SGPropertyNode* aWP);
  
  double magvarDegAt(const SGGeod& pos) const;
  
  std::string _ident;
  int _currentIndex;
  
  FGAirportRef _departure, _destination;
  FGRunway* _departureRunway, *_destinationRunway;
  SID* _sid;
  STAR* _star;
  Approach* _approach;
  std::string _sidTransition, _starTransition;
  
  double _totalDistance;
  void rebuildLegData();
  
  typedef std::vector<Leg*> LegVec;
  LegVec _legs;
  
  Delegate* _delegate;
};
  
} // of namespace flightgear

#endif // of FG_FLIGHTPLAN_HXX
