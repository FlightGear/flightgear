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

  /**
   * minimum distance to a waypoint for overflight sequencing. 
   */
  virtual double overflightArmDistanceM() = 0;
};

class WayptController
{
public:
  virtual ~WayptController();
  
  virtual void init();

  virtual void update() = 0;

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
  virtual double trueBearingDeg() const
    { return _targetTrack; }

  virtual double targetTrackDeg() const 
    { return _targetTrack; }

  virtual double xtrackErrorNm() const
    { return 0.0; }

  virtual double courseDeviationDeg() const
    { return 0.0; }

  /**
   * Position associated with the waypt. For static waypoints, this is
   * simply the waypoint position itself; for dynamic points, it's the
   * estimated location at which the controller will be done.
   */
  virtual SGGeod position() const = 0;

  /**
   * Is this controller finished?
   */
  bool isDone() const
    { return _isDone; }

  /**
   * to/from flag - true = to, false = from. Defaults to 'true' because
   * nearly all waypoint controllers become done as soon as this value would
   * become false.
   */
  virtual bool toFlag() const
    { return true; }
    
  /**
   * Static factory method, given a waypoint, return a controller bound
   * to it, of the appropriate type
   */
  static WayptController* createForWaypt(RNAV* rnav, const WayptRef& aWpt);
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
  
  virtual void init();
  virtual void update();
  virtual double distanceToWayptM() const;  
  virtual double xtrackErrorNm() const;  
  virtual double courseDeviationDeg() const;  
  virtual double trueBearingDeg() const;
  virtual SGGeod position() const;
private:
  SGGeod _origin;
  double _distanceM;
  double _courseDev;
};

/**
 *
 */
class OBSController : public WayptController
{
public:
  OBSController(RNAV* aRNAV, const WayptRef& aWpt);
  
  virtual void init();
  virtual void update();
  virtual double distanceToWayptM() const;  
  virtual double xtrackErrorNm() const;  
  virtual double courseDeviationDeg() const;  
  virtual double trueBearingDeg() const;
  virtual bool toFlag() const;
  virtual SGGeod position() const;
private:
  double _distanceM;
  double _courseDev;
};

} // of namespace flightgear

#endif
