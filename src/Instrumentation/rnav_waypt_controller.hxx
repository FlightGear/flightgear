// rnav_waypt_controller.hxx - Waypoint-specific behaviours for RNAV systems
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

#ifndef FG_WAYPT_CONTROLLER_HXX
#define FG_WAYPT_CONTROLLER_HXX

#include <Navaids/waypoint.hxx>
#include <simgear/misc/simgear_optional.hxx>

namespace flightgear
{

/**
 * Abstract RNAV interface, for devices which implement an RNAV
 * system - INS / GPS / FMS
 */
class RNAV
{
public:
  virtual SGGeod position() = 0;
  
  /**
   * True track in degrees
   */
  virtual double trackDeg() = 0;
  
  /**
   * Ground speed (along the track) in knots
   */
  virtual double groundSpeedKts() = 0;
  
  /**
   * Vertical speed in ft/minute
   */
  virtual double vspeedFPM()= 0;
  
  /**
   * Magnetic variation at current position
   */
  virtual double magvarDeg() = 0;
  
  /**
   * device selected course (eg, from autopilot / MCP / OBS) in degrees
   */
  virtual double selectedMagCourse() = 0;
    
    virtual bool canFlyBy() const
    {
        return false;
    }
    
  /**
   * minimum distance to switch next waypoint.
   */
  virtual double overflightDistanceM() = 0;
  /**
   * minimum distance to a waypoint for overflight sequencing. 
   */
  virtual double overflightArmDistanceM() = 0;
  /**
     * angle for overflight sequencing.
     */
    virtual double overflightArmAngleDeg() = 0;

    struct LegData {
        SGGeod position;
        bool didFlyBy = false;
        SGGeod flyByTurnCenter;
        double flyByRadius = 0.0;
        double turnAngle = 0.0;
    };
    
  /**
   * device leg previous waypoint position(eg, from route manager)
   */
  virtual simgear::optional<LegData> previousLegData()
    {
        return simgear::optional<LegData>();
    }
    
    virtual simgear::optional<double> nextLegTrack()
    {
        return simgear::optional<double>{};
    }
     
  /**
   * @brief compute turn radius based on current ground-speed
   */
    
  double turnRadiusNm()
  {
    return turnRadiusNm(groundSpeedKts());
  }
  
  /**
   * @brief compute the turn radius (based on standard rate turn) for
   * a given ground speed in knots.
   */
  virtual double turnRadiusNm(const double groundSpeedKnots) = 0;
};

class WayptController
{
public:
  virtual ~WayptController();

  virtual bool init();

  virtual void update(double dt) = 0;

  /**
   * Compute time until the waypoint is done
   */
  virtual double timeToWaypt() const;

  /**
   * Compute distance until the waypoint is done
   */
  virtual double distanceToWayptM() const = 0;

  /**
   * Bearing to the waypoint, if this value is meaningful.
   * Default implementation returns the target track
   */
  virtual double trueBearingDeg() const;

  virtual double targetTrackDeg() const;

  virtual double xtrackErrorNm() const;
  
  virtual double courseDeviationDeg() const;
  
  /**
   * Position associated with the waypt. For static waypoints, this is
   * simply the waypoint position itself; for dynamic points, it's the
   * estimated location at which the controller will be done.
   */
  virtual SGGeod position() const = 0;

  /**
   * Is this controller finished?
   */
  bool isDone() const;

  /**
   * to/from flag - true = to, false = from. Defaults to 'true' because
   * nearly all waypoint controllers become done as soon as this value would
   * become false.
   */
  virtual bool toFlag() const
    { return true; }
  
  /**
   * Allow waypoints to indicate a status value as a string.
   * Useful for more complex controllers, which may have capture / exit
   * states
   */
  virtual std::string status() const;
    
    virtual simgear::optional<RNAV::LegData> legData() const
    {
        // defer to our subcontroller if it exists
        if (_subController)
            return _subController->legData();
        
        return simgear::optional<RNAV::LegData>();
    }
    
  /**
   * Static factory method, given a waypoint, return a controller bound
   * to it, of the appropriate type
   */
  static WayptController* createForWaypt(RNAV* rnav, const WayptRef& aWpt);
    
    WayptRef waypoint() const
    { return _waypt; }
    
protected:
  WayptController(RNAV* aRNAV, const WayptRef& aWpt) :
    _waypt(aWpt),
    _targetTrack(0),
    _rnav(aRNAV),
    _isDone(false)
  { }
  
  WayptRef _waypt;
  double _targetTrack;
  RNAV* _rnav;
  
  void setDone();
  
  // take asubcontroller ref (will be destroyed automatically)
  // pass nullptr to clear any activ esubcontroller
  // the subcontroller will be initialised
  void setSubController(WayptController* sub);
  
  // if a subcontroller exists, we can delegate to it automatically
  std::unique_ptr<WayptController> _subController;
private:
  bool _isDone;
};

/**
 * Controller supports 'directTo' (DTO) navigation to a waypoint. This
 * creates a course from a starting point, to the waypoint, and reports
 * deviation from that course.
 *
 * The controller is done when the waypoint is reached (to/from goes to 'from')
 */
class DirectToController : public WayptController
{
public:
  DirectToController(RNAV* aRNAV, const WayptRef& aWpt, const SGGeod& aOrigin);

  bool init() override;
  virtual void update(double dt);
  virtual double distanceToWayptM() const;  
  virtual double xtrackErrorNm() const;  
  virtual double courseDeviationDeg() const;  
  virtual double trueBearingDeg() const;
  virtual SGGeod position() const;
private:
  SGGeod _origin;
  double _distanceAircraftTargetMeter;
  double _courseDev;
  double _courseAircraftToTarget;
};

/**
 *
 */
class OBSController : public WayptController
{
public:
  OBSController(RNAV* aRNAV, const WayptRef& aWpt);

  bool init() override;
  virtual void update(double dt);
  virtual double distanceToWayptM() const;  
  virtual double xtrackErrorNm() const;  
  virtual double courseDeviationDeg() const;  
  virtual double trueBearingDeg() const;
  virtual bool toFlag() const;
  virtual SGGeod position() const;
private:
  double _distanceAircraftTargetMeter;
  double _courseDev;
  double _courseAircraftToTarget;
};

class HoldCtl : public WayptController
{
  enum HoldState {
    HOLD_INIT,
    LEG_TO_HOLD,
    ENTRY_DIRECT,
    ENTRY_PARALLEL, // flying the outbound part of a parallel entry
    ENTRY_PARALLEL_OUTBOUND,
    ENTRY_PARALLEL_INBOUND,
    ENTRY_TEARDROP, // flying the outbound leg of teardrop entry
    HOLD_OUTBOUND,
    HOLD_INBOUND,
    HOLD_EXITING, // we are going to exit the hold
  };
  
  HoldState _state = HOLD_INIT;
  double _holdCourse = 0.0;
  double _holdLegTime = 60.0;
  double _holdLegDistance = 0.0;
  double _holdCount = 0;
  bool _leftHandTurns = false;
    
  bool _inTurn = false;
  SGGeod _turnCenter;
  double _turnEndAngle, _turnRadius;
  SGGeod _segmentEnd;
  
  bool checkOverHold();
  void checkInitialEntry(double dNm);
    
  void startInboundTurn();
  void startOutboundTurn();
  void startParallelEntryTurn();
  void exitTurn();
  
  SGGeod outboundEndPoint();
  SGGeod outboundTurnCenter();
  SGGeod inboundTurnCenter();
  
  double holdLegLengthNm() const;
  
    /**
     * are we turning left? This is basically the )leftHandTurns member,
     * but if we're in the inbound turn of a parallel entry, it's flipped
     */
    bool inLeftTurn() const;
  
  void computeEntry();
public:
  HoldCtl(RNAV* aRNAV, const WayptRef& aWpt);
  
  void setHoldCount(int count);
  void exitHold();

  bool init() override;
  void update(double) override;
  
  double distanceToWayptM() const override;
  SGGeod position() const override;
  double xtrackErrorNm() const override;
  double courseDeviationDeg() const override;
    
  std::string status() const override;
};
    
} // of namespace flightgear

#endif
