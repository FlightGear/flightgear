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

#include <functional>

#include <Navaids/route.hxx>
#include <Airports/airport.hxx>

namespace flightgear
{

class Transition;
class FlightPlan;

typedef SGSharedPtr<FlightPlan> FlightPlanRef;

enum class ICAOFlightRules
{
    VFR = 0,
    IFR,
    IFR_VFR,    // type Y
    VFR_IFR     // type Z
};

enum class ICAOFlightType
{
    Scheduled = 0,
    NonScheduled,
    GeneralAviation,
    Military,
    Other // type X
};
    
class FlightPlan : public RouteBase
{
public:
  FlightPlan();
  virtual ~FlightPlan();

  virtual std::string ident() const;
  void setIdent(const std::string& s);

    // propogate the GPS/FMS setting for this through to the RoutePath
    void setFollowLegTrackToFixes(bool tf);
    bool followLegTrackToFixes() const;

    // aircraft approach category as per CFR 97.3, etc
    // http://www.flightsimaviation.com/data/FARS/part_97-3.html
    std::string icaoAircraftCategory() const;
    void setIcaoAircraftCategory(const std::string& cat);

    std::string icaoAircraftType() const
    { return _aircraftType; }

    void setIcaoAircraftType(const std::string& ty);

    FlightPlan* clone(const std::string& newIdent = std::string()) const;

  /**
   * flight-plan leg encapsulation
   */
  class Leg : public SGReferenced
  {
  public:
    FlightPlan* owner() const
    { return const_cast<FlightPlan*>(_parent); }

    Waypt* waypoint() const
    { return _waypt; }

    // return the next leg after this one
    Leg* nextLeg() const;

    /**
     * requesting holding at the waypoint upon reaching it. This will
     * convert the waypt to a Hold if not already defined as one, but
     * with default hold data.
     *
     * If the waypt is not of a type suitable for holding at, returns false
     * (eg a runway or dynamic waypoint)
     */
    bool setHoldCount(int count);
    
    int holdCount() const;
    
      
    bool convertWaypointToHold();
      
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

    /**
     * helper function, if the waypoint is modified in some way, to
     * notify the flightplan owning this leg, and hence any delegates
     * obsering us
     */
    void markWaypointDirty();
  private:
    friend class FlightPlan;

    Leg(FlightPlan* owner, WayptRef wpt);

    Leg* cloneFor(FlightPlan* owner) const;

      void writeToProperties(SGPropertyNode* node) const;
      
    const FlightPlan* _parent;
    RouteRestriction _speedRestrict = RESTRICT_NONE,
      _altRestrict = RESTRICT_NONE;
    int _speed = 0;
    int _altitudeFt = 0;

    // if > 0, we will hold at the waypoint using
    // the published hold side/course
    // This only works if _waypt is a Hold, either defined by a procedure
    // or modified to become one
    int _holdCount = 0;
      
    WayptRef _waypt;
    /// length of this leg following the flown path
    mutable double _pathDistance = -1.0;
    mutable double _courseDeg = -1.0;
    /// total distance of this leg from departure point
    mutable double _distanceAlongPath = 11.0;
  };
    
  using LegRef = SGSharedPtr<Leg>;

  class DelegateFactory;
  using DelegateFactoryRef = std::shared_ptr<DelegateFactory>;

  class Delegate
  {
  public:
    virtual ~Delegate();

    virtual void departureChanged() { }
    virtual void arrivalChanged() { }
    virtual void waypointsChanged() { }
    virtual void cruiseChanged()  { }
    virtual void cleared() { }
    virtual void activated() { }
    
      /**
       * Invoked when the C++ code determines the active leg is done / next
       * leg should be sequenced. The default route-manager delegate will
       * advance to the next waypoint when handling this.
       *
       * If multiple delegates are installed, take special care not to sequence
       * the waypoint twice.
       */
    virtual void sequence() { }
    
    virtual void currentWaypointChanged() { }
    virtual void endOfFlightPlan() { }
      
      virtual void loaded() { }
  protected:
    Delegate();

  private:
    friend class FlightPlan;
    
    // record the factory which created us, so we have the option to clean up
    DelegateFactoryRef _factory;
  };

  LegRef insertWayptAtIndex(Waypt* aWpt, int aIndex);
  void insertWayptsAtIndex(const WayptVec& wps, int aIndex);

  void deleteIndex(int index);
  void clearAll();
  void clearLegs();
  int clearWayptsWithFlag(WayptFlag flag);

  int currentIndex() const
  { return _currentIndex; }

  void sequence();
    
  void setCurrentIndex(int index);

  void activate();

  void finish();

    bool isActive() const;
    
  LegRef currentLeg() const;
  LegRef nextLeg() const;
  LegRef previousLeg() const;

  int numLegs() const
  { return static_cast<int>(_legs.size()); }

  LegRef legAtIndex(int index) const;

  int findWayptIndex(const SGGeod& aPos) const;
  int findWayptIndex(const FGPositionedRef aPos) const;

    int indexOfFirstNonDepartureWaypoint() const;
    int indexOfFirstArrivalWaypoint() const;
    int indexOfFirstApproachWaypoint() const;
    int indexOfDestinationRunwayWaypoint() const;
    
  bool load(const SGPath& p);
  bool save(const SGPath& p) const;

    bool save(std::ostream& stream) const;
    bool load(std::istream& stream);

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

    void clearDeparture();
    
  SID* sid() const
  { return _sid; }

  Transition* sidTransition() const;

  void setSID(SID* sid, const std::string& transition = std::string());

  void setSID(Transition* sidWithTrans);

    void clearSID();
    
  void setDestination(FGAirport* apt);
  void setDestination(FGRunway* rwy);

    void clearDestination();
    
    FGAirportRef alternate() const;
    void setAlternate(FGAirportRef alt);

  /**
    * note setting an approach will implicitly update the destination
    * airport and runway to match
    */
    void setApproach(Approach* app, const std::string& transition = {});

    void setApproach(Transition* approachWithTrans);

    
    Transition* approachTransition() const;
    
  STAR* star() const
  { return _star; }

  Transition* starTransition() const;

  void setSTAR(STAR* star, const std::string& transition = std::string());

  void setSTAR(Transition* starWithTrans);

  void clearSTAR();
    
  double totalDistanceNm() const
  { return _totalDistance; }

  int estimatedDurationMinutes() const
  { return _estimatedDuration; }

  void setEstimatedDurationMinutes(int minutes);

  /**
   * @brief computeDurationMinutes - use performance data and cruise data
   * to estimate enroute time
   */
  void computeDurationMinutes();

  /**
   * given a waypoint index, and an offset in NM, find the geodetic
   * position on the route path. I.e the point 10nm before or after
   * a particular waypoint.
   */
  SGGeod pointAlongRoute(int aIndex, double aOffsetNm) const;

  /**
   * Create a WayPoint from a string in the following format:
   *  - simple identifier
   *  - decimal-lon,decimal-lat
   *  - airport-id/runway-id
   *  - navaid/radial-deg/offset-nm
   */
  WayptRef waypointFromString(const std::string& target);

    /**
     * attempt to replace the route waypoints (and potentially the SID and
     * STAR) based on an ICAO standard route string, i.e item 15.
     * Returns true if the rotue was parsed successfully (and this flight
     * plan modified accordingly) or false if the string could not be
     * parsed.
     */
    bool parseICAORouteString(const std::string& routeData);

    std::string asICAORouteString() const;

// ICAO flight-plan data
    void setFlightRules(ICAOFlightRules rules);
    ICAOFlightRules flightRules() const;
    
    void setFlightType(ICAOFlightType type);
    ICAOFlightType flightType() const;
    
    void setCallsign(const std::string& callsign);
    std::string callsign() const
    { return _callsign; }
    
    void setRemarks(const std::string& remarks);
    std::string remarks() const
    { return _remarks; }
    
// cruise data
    void setCruiseSpeedKnots(int kts);
    int cruiseSpeedKnots() const;
    
    void setCruiseSpeedMach(double mach);
    double cruiseSpeedMach() const;
    
    void setCruiseFlightLevel(int flightLevel);
    int cruiseFlightLevel() const;
    
    void setCruiseAltitudeFt(int altFt);
    int cruiseAltitudeFt() const;
    
  /**
   * abstract interface for creating delegates automatically when a
   * flight-plan is created or loaded
   */
  class DelegateFactory
  {
  public:
    virtual Delegate* createFlightPlanDelegate(FlightPlan* fp) = 0;
    virtual void destroyFlightPlanDelegate(FlightPlan* fp, Delegate* d);
  };
    
  static void registerDelegateFactory(DelegateFactoryRef df);
  static void unregisterDelegateFactory(DelegateFactoryRef df);

  void addDelegate(Delegate* d);
  void removeDelegate(Delegate* d);
    
    using LegVisitor = std::function<void(Leg*)>;
    void forEachLeg(const LegVisitor& lv);
private:
  friend class Leg;
  
  int findLegIndex(const Leg* l) const;
    
  void lockDelegates();
  void unlockDelegates();

  void notifyCleared();
    
  unsigned int _delegateLock = 0;
  bool _arrivalChanged = false,
    _departureChanged = false,
    _waypointsChanged = false,
    _currentWaypointChanged = false,
    _cruiseDataChanged = false;
  bool _didLoadFP = false;
    
    void saveToProperties(SGPropertyNode* d) const;
    
  bool loadXmlFormat(const SGPath& path);
  bool loadGpxFormat(const SGPath& path);
  bool loadPlainTextFormat(const SGPath& path);

  bool loadVersion1XMLRoute(SGPropertyNode_ptr routeData);
  bool loadVersion2XMLRoute(SGPropertyNode_ptr routeData);
  void loadXMLRouteHeader(SGPropertyNode_ptr routeData);
  WayptRef parseVersion1XMLWaypt(SGPropertyNode* aWP);

  double magvarDegAt(const SGGeod& pos) const;
  bool parseICAOLatLon(const std::string &s, SGGeod &p);

  std::string _ident;
  std::string _callsign;
  std::string _remarks;
  std::string _aircraftType;

  int _currentIndex;
    bool _followLegTrackToFix;
    char _aircraftCategory;
    ICAOFlightType _flightType = ICAOFlightType::Other;
    ICAOFlightRules _flightRules = ICAOFlightRules::VFR;
    int _cruiseAirspeedKnots = 0;
    double _cruiseAirspeedMach = 0.0;
    int _cruiseFlightLevel = 0;
    int _cruiseAltitudeFt = 0;
    int _estimatedDuration = 0;

  FGAirportRef _departure, _destination;
  FGAirportRef _alternate;
  FGRunway* _departureRunway, *_destinationRunway;
  SGSharedPtr<SID> _sid;
  SGSharedPtr<STAR> _star;
  SGSharedPtr<Approach> _approach;
  std::string _sidTransition, _starTransition, _approachTransition;

  double _totalDistance;
  void rebuildLegData();

  using LegVec = std::vector<LegRef>;
  LegVec _legs;

  std::vector<Delegate*> _delegates;
};

} // of namespace flightgear

#endif // of FG_FLIGHTPLAN_HXX
