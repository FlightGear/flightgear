// rnav_waypt_controller.cxx - Waypoint-specific behaviours for RNAV systems
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

#include "rnav_waypt_controller.hxx"

#include <cassert>
#include <stdexcept>

#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>

#include <Airports/runways.hxx>
#include "Main/util.hxx" // for fgLowPass

extern double pointsKnownDistanceFromGC(const SGGeoc& a, const SGGeoc&b, const SGGeoc& d, double dist);

namespace flightgear
{

const auto turnComputeDist = SG_NM_TO_METER * 4.0;

// declared in routePath.cxx
bool geocRadialIntersection(const SGGeoc& a, double r1, const SGGeoc& b, double r2, SGGeoc& result);

/**
 * Helper function Cross track error:
 * http://williams.best.vwh.net/avform.htm#XTE
 *
 * param	distanceOriginToPosition in Nautical Miles
 * param	courseDev difference between courseOriginToAircraft(AD) and courseOriginToTarget(AB)
 * return 	XTE in Nautical Miles
 *
 * A(origin)
 * B(target)
 * D(aircraft) perhaps off course
 *
 * 			A-->--B
 * 			 \   /
 * 			  \ /
 * 			   D
 */

double greatCircleCrossTrackError(double distanceOriginToPosition,double courseDev){
	double crossTrackError = asin(
			sin(distanceOriginToPosition * SG_NM_TO_RAD) *
			sin(courseDev * SG_DEGREES_TO_RADIANS)
			 );
	return crossTrackError * SG_RAD_TO_NM;
}

SGGeod computeTurnCenter(double turnRadiusM, const SGGeod& basePos, double inboundTrack, double turnAngle)
{
   
   // this is the heading half way through the turn. Perpendicular to
   // this is our turn center
   const auto halfTurnHeading = inboundTrack + (turnAngle * 0.5);
   double p = copysign(90.0, turnAngle);
   double h = halfTurnHeading + p;
   SG_NORMALIZE_RANGE(h, 0.0, 360.0);
     
   const double tcOffset = turnRadiusM / cos(turnAngle * 0.5 * SG_DEGREES_TO_RADIANS);
   return SGGeodesy::direct(basePos, h, tcOffset);
}

////////////////////////////////////////////////////////////////////////////s

WayptController::~WayptController()
{
}

bool WayptController::init()
{
    return true;
}
  
bool WayptController::isDone() const
{
  if (_subController) {
    return _subController->isDone();
  }
  
  return _isDone;
}

void WayptController::setDone()
{
  if (_isDone) {
    SG_LOG(SG_AUTOPILOT, SG_DEV_WARN, "already done @ WayptController::setDone");
  }
  
  _isDone = true;
}

double WayptController::timeToWaypt() const
{
  double gs = _rnav->groundSpeedKts();
  if (gs < 1.0) {
    return -1.0; // stationary
  }
  
    gs*= SG_KT_TO_MPS;
  return (distanceToWayptM() / gs);
}

std::string WayptController::status() const
{
    return {};
}

void WayptController::setSubController(WayptController* sub)
{
  _subController.reset(sub);
  if (_subController) {
    // seems like a good idea to check this
    assert(_subController->_rnav == _rnav);
    _subController->init();
  }
}

double WayptController::trueBearingDeg() const
{
  if (_subController)
    return _subController->trueBearingDeg();
  
  return _targetTrack;
}
  
double WayptController::targetTrackDeg() const
{
  if (_subController)
    return _subController->targetTrackDeg();
  
  return _targetTrack;
}
  
double WayptController::xtrackErrorNm() const
{
  if (_subController)
    return _subController->xtrackErrorNm();
  
  return 0.0;
}

double WayptController::courseDeviationDeg() const
{
  if (_subController)
    return _subController->courseDeviationDeg();
  
  return 0.0;
}
  
//////////////

class BasicWayptCtl : public WayptController
{
public:
  BasicWayptCtl(RNAV* aRNAV, const WayptRef& aWpt) :
    WayptController(aRNAV, aWpt),
    _distanceAircraftTargetMeter(0.0),
    _courseDev(0.0)
  {
    if (aWpt->flag(WPT_DYNAMIC)) {
      throw sg_exception("BasicWayptCtrl doesn't work with dynamic waypoints");
    }
  }

  bool init() override
  {
    _targetTrack = SGGeodesy::courseDeg(_rnav->position(), _waypt->position());
    return true;
  }

  virtual void update(double)
  {
    double bearingAircraftToTarget;

    bearingAircraftToTarget			= SGGeodesy::courseDeg(_rnav->position(), _waypt->position());
    _distanceAircraftTargetMeter	= SGGeodesy::distanceM(_rnav->position(), _waypt->position());

    _courseDev = bearingAircraftToTarget - _targetTrack;
    SG_NORMALIZE_RANGE(_courseDev, -180.0, 180.0);
    
    if ((fabs(_courseDev) > _rnav->overflightArmAngleDeg()) && (_distanceAircraftTargetMeter < _rnav->overflightArmDistanceM())) {
      setDone();
    }
  } 

  virtual double distanceToWayptM() const
  {
    return _distanceAircraftTargetMeter;
  }
  
  virtual double xtrackErrorNm() const
  {
    double x = sin(courseDeviationDeg() * SG_DEGREES_TO_RADIANS) * _distanceAircraftTargetMeter;
    return x * SG_METER_TO_NM;
  }
  
  virtual bool toFlag() const
  {
    return (fabs(_courseDev) < _rnav->overflightArmAngleDeg());
  }
  
  virtual double courseDeviationDeg() const
  {
    return _courseDev;
  }
  
  virtual double trueBearingDeg() const
  {
    return SGGeodesy::courseDeg(_rnav->position(), _waypt->position());
  }
  
  virtual SGGeod position() const
  {
    return _waypt->position();
  }

private:
  double _distanceAircraftTargetMeter;
  double _courseDev;
};

/**
 * Controller for leg course interception.
 * In leg mode, we want to intercept the leg between 2 waypoints(A->B).
 * If we can't reach the the selected waypoint leg,we going direct to B.
 */

class LegWayptCtl : public WayptController
{
public:
	LegWayptCtl(RNAV* aRNAV, const WayptRef& aWpt) :
    WayptController(aRNAV, aWpt),
    _waypointOrigin(),
    _distanceOriginAircraftMetre(0.0),
    _distanceAircraftTargetMetre(0.0),
    _courseOriginToAircraft(0.0),
    _courseAircraftToTarget(0.0),
    _courseDev(0.0),
    _toFlag(true)
  {
    if (aWpt->flag(WPT_DYNAMIC)) {
      throw sg_exception("LegWayptCtl doesn't work with dynamic waypoints");
    }
  }

  bool init() override
  {
    auto previousLeg = _rnav->previousLegData();
    if (previousLeg) {
      _waypointOrigin = previousLeg.value().position;
      _initialLegCourse = SGGeodesy::courseDeg(_waypointOrigin,_waypt->position());
    } else {
      // capture current position
      _waypointOrigin = _rnav->position();
    }
    
    _courseAircraftToTarget			= SGGeodesy::courseDeg(_rnav->position(),_waypt->position());
    _distanceAircraftTargetMetre 	= SGGeodesy::distanceM(_rnav->position(),_waypt->position());
    
    
    // check reach the leg in 45Deg or going direct
    bool canReachLeg = (fabs(_initialLegCourse -_courseAircraftToTarget) < 45.0);
    
    if (previousLeg && canReachLeg) {
      _targetTrack = _initialLegCourse;
    } else {
      // can't reach the leg with out a crazy turn, just go direct to the
      // destination waypt
      _targetTrack = _courseAircraftToTarget;
      _initialLegCourse = _courseAircraftToTarget;
      _waypointOrigin = _rnav->position();
    }
      
  // turn angle depends on final leg course, not initial
  // do this here so we have a chance of doing a fly-by at the end of the leg
  _finalLegCourse = SGGeodesy::courseDeg(_waypt->position(), _waypointOrigin) + 180;
  SG_NORMALIZE_RANGE(_finalLegCourse, 0.0, 360.0);
    
    // turn-in logic
    if (previousLeg.has_value() && previousLeg.value().didFlyBy) {
      _flyByTurnCenter = previousLeg.value().flyByTurnCenter;
      _flyByTurnAngle = previousLeg.value().turnAngle;
      _flyByTurnRadius = previousLeg.value().flyByRadius;
      _entryFlyByActive = true;
      SG_NORMALIZE_RANGE(_flyByTurnAngle, -180.0, 180.0);
    }

    return true;
  }
  
  void computeTurnAnticipation()
  {
    _didComputeTurn = true;
    
    if (_waypt->flag(WPT_OVERFLIGHT)) {
      return; // can't fly-by
    }
    
    if (!_rnav->canFlyBy())
      return;
    
    auto nextLegTrack = _rnav->nextLegTrack();
    
    if (!nextLegTrack.has_value()) {
      return;
    }
    
    _flyByTurnAngle = nextLegTrack.value() - _finalLegCourse;
    SG_NORMALIZE_RANGE(_flyByTurnAngle, -180.0, 180.0);
    
    if (fabs(_flyByTurnAngle) > 120.0) {
      // too sharp, don't anticipate
      return;
    }
    
    _flyByTurnRadius =  _rnav->turnRadiusNm() * SG_NM_TO_METER;
      _flyByTurnCenter = computeTurnCenter(_flyByTurnRadius, _waypt->position(), _finalLegCourse, _flyByTurnAngle);
    _doFlyBy = true;
  }
  
  bool updateInTurn()
  {
    // find bearing to turn center
    // when it hits 90 off our track
    
    auto bearingToTurnCenter = SGGeodesy::courseDeg(_rnav->position(), _flyByTurnCenter);
    auto distToTurnCenter = SGGeodesy::distanceM(_rnav->position(), _flyByTurnCenter);

    if (!_flyByStarted) {
      // check for the turn center being 90 degrees off one wing (start the turn)
      auto a = bearingToTurnCenter - _finalLegCourse;
      SG_NORMALIZE_RANGE(a, -180.0, 180.0);
        if (fabs(a) < 90.0) {
        return false; // keep flying normal leg
      }
      
      _flyByStarted = true;
    }
    
    // check for us passing the half-way point; that's when we should
    // sequence to the next WP
    const auto halfPointAngle = (_finalLegCourse + (_flyByTurnAngle * 0.5));
    auto b = bearingToTurnCenter - halfPointAngle;
    SG_NORMALIZE_RANGE(b, -180.0, 180.0);
            
    if (fabs(b) >= 90.0) {
      _toFlag = false;
      setDone();
    }
    
    // in the actual turn, our desired track is always pependicular to the
    // bearing to the turn center we computed
    _targetTrack = bearingToTurnCenter - copysign(90, _flyByTurnAngle);
      
    SG_NORMALIZE_RANGE(_targetTrack, 0.0, 360.0);

    _crossTrackError = (distToTurnCenter - _flyByTurnRadius) * SG_METER_TO_NM;
    _courseDev = _crossTrackError * 10.0; // arbitrary guess for now
    
    return true;
  }
  
  void updateInEntryTurn()
  {
    auto bearingToTurnCenter = SGGeodesy::courseDeg(_rnav->position(), _flyByTurnCenter);
    auto distToTurnCenter = SGGeodesy::distanceM(_rnav->position(), _flyByTurnCenter);
  
    auto b = bearingToTurnCenter - _initialLegCourse;

      SG_NORMALIZE_RANGE(b, -180.0, 180.0);
    if (fabs(b) >= 90.0) {
      _entryFlyByActive = false;
      return; // we're done with the entry turn
    }
    
    _targetTrack = bearingToTurnCenter - copysign(90, _flyByTurnAngle);

    SG_NORMALIZE_RANGE(_targetTrack, 0.0, 360.0);
    _crossTrackError = (distToTurnCenter - _flyByTurnRadius) * SG_METER_TO_NM;
    _courseDev = _crossTrackError * 10.0; // arbitrary guess for now
  }

  void update(double) override
  {
    _courseOriginToAircraft			= SGGeodesy::courseDeg(_waypointOrigin,_rnav->position());
    _distanceOriginAircraftMetre 	= SGGeodesy::distanceM(_waypointOrigin,_rnav->position());

    _courseAircraftToTarget			= SGGeodesy::courseDeg(_rnav->position(),_waypt->position());
    _distanceAircraftTargetMetre 	= SGGeodesy::distanceM(_rnav->position(),_waypt->position());

    if (_entryFlyByActive) {
      updateInEntryTurn();
      return;
    }
    
    if (!_didComputeTurn && (_distanceAircraftTargetMetre < turnComputeDist)) {
      computeTurnAnticipation();
    }
        
    if (_didComputeTurn && _doFlyBy) {
      bool ok = updateInTurn();
      if (ok) {
        return;
      }
      
      // otherwise we fall through
    }
    
    // from the Aviation Formulary
#if 0
    Suppose you are proceeding on a great circle route from A to B (course =crs_AB) and end up at D,
    perhaps off course. (We presume that A is ot a pole!) You can calculate the course from A to D
    (crs_AD) and the distance from A to D (dist_AD) using the formulae above. In terms of these the
    cross track error, XTD, (distance off course) is given by
    
    XTD =asin(sin(dist_AD)*sin(crs_AD-crs_AB))
    (positive XTD means right of course, negative means left)
    (If the point A is the N. or S. Pole replace crs_AD-crs_AB with
     lon_D-lon_B or lon_B-lon_D, respectively.)
#endif
    // however, just for fun, our convention for polarity of the cross-track
    // sign is opposite, so we add a -ve to the computation below.
    
    double distOriginAircraftRad = _distanceOriginAircraftMetre * SG_METER_TO_NM * SG_NM_TO_RAD;
    double xtkRad = asin(sin(distOriginAircraftRad) * sin((_courseOriginToAircraft - _initialLegCourse) * SG_DEGREES_TO_RADIANS));
    
    // convert to NM and flip sign for consistency with existing code.
    // since we derive the abeam point and course-deviation from this, and
    // thus the GPS cdi-deflection, if we don't fix this here, the sign of
    // all of those comes out backwards.
    _crossTrackError = -(xtkRad * SG_RAD_TO_NM);
    
    /*
     The "along track distance", ATD, the distance from A along the course towards B
     to the point abeam D is given by:
     ATD=acos(cos(dist_AD)/cos(XTD))
     */
    double atd = acos(cos(distOriginAircraftRad) / cos(xtkRad));

    // fix sign of ATD, if we are 'behind' the waypoint origin, rather than
    // in front of it. We need to set a -ve sign on atd, otherwise we compute
    // an abeamPoint ahead of the waypoint origin, potentially so far ahead
    // that the computed track is backwards.
    // see test-case GPSTests::testCourseLegIntermediateWaypoint
    // for this happening
    {
        double x = _courseOriginToAircraft - _initialLegCourse;
        SG_NORMALIZE_RANGE(x, -180.0, 180.0);
        if (fabs(x) > 90.0) {
            atd = -atd;
        }
    }

    const double atdM = atd * SG_RAD_TO_NM * SG_NM_TO_METER;
    SGGeod abeamPoint = SGGeodesy::direct(_waypointOrigin, _initialLegCourse, atdM);

    // if we're close to the target point, we enter something similar to the VOR zone
    // of confusion. Force target track to the final course from the origin.
    if (_distanceAircraftTargetMetre < (SG_NM_TO_METER * 2.0)) {
      _targetTrack = SGGeodesy::courseDeg(_waypt->position(), _waypointOrigin) + 180.0;
      SG_NORMALIZE_RANGE(_targetTrack, 0.0, 360.0);
    } else {
      // use the abeam point to compute the desired course
      double desiredCourse = SGGeodesy::courseDeg(abeamPoint, _waypt->position());
      _targetTrack = desiredCourse;
    }
    
    _courseDev = _courseAircraftToTarget - _targetTrack;
    SG_NORMALIZE_RANGE(_courseDev, -180.0, 180.0);

    
    bool isMinimumOverFlightDistanceReached = _distanceAircraftTargetMetre < _rnav->overflightDistanceM();
    bool isOverFlightConeArmed 				= _distanceAircraftTargetMetre < ( _rnav->overflightArmDistanceM() + _rnav->overflightDistanceM() );
    bool leavingOverFlightCone 				= (fabs(_courseDev) > _rnav->overflightArmAngleDeg());
    
    if ( isMinimumOverFlightDistanceReached ){
      _toFlag = false;
      setDone();
    } else {
      if( isOverFlightConeArmed ){
        _toFlag = false;
        if ( leavingOverFlightCone ) {
          setDone();
        }
      }else{
        _toFlag = true;
      }
    }
  }

  simgear::optional<RNAV::LegData> legData() const override
  {
    RNAV::LegData r;
    r.position = _waypt->position();
    if (_doFlyBy) {
      // copy all the fly by paramters, so the next controller can
      // smoothly link up
      r.didFlyBy = true;
      r.flyByRadius = _flyByTurnRadius;
      r.flyByTurnCenter = _flyByTurnCenter;
      r.turnAngle = _flyByTurnAngle;
    }
    
    return r;
  }
  
  virtual double distanceToWayptM() const
  {
    return _distanceAircraftTargetMetre;
  }

  virtual double xtrackErrorNm() const
  {
    return _crossTrackError;
  }

  virtual bool toFlag() const
  {
    return _toFlag;
  }

  virtual double courseDeviationDeg() const
  {
    return _courseDev;
  }

  virtual double trueBearingDeg() const
  {
    return _courseAircraftToTarget;
  }

  virtual SGGeod position() const
  {
    return _waypt->position();
  }

private:

  /*
   * great circle route
   * A(from), B(to), D(position) perhaps off course
   */
  SGGeod _waypointOrigin;
  double _initialLegCourse;
  double _finalLegCourse;
  double _distanceOriginAircraftMetre;
  double _distanceAircraftTargetMetre;
  double _courseOriginToAircraft;
  double _courseAircraftToTarget;
  double _courseDev;
  bool _toFlag;
  double _crossTrackError;
  
  bool _didComputeTurn = false;
  bool _doFlyBy = false;
  SGGeod _flyByTurnCenter;
  double _flyByTurnAngle;
  double _flyByTurnRadius;
  bool _entryFlyByActive = false;
  bool _flyByStarted = false;
};

/**
 * Special controller for runways. For runways, we want very narrow deviation
 * constraints, and to understand that any point along the paved area is
 * equivalent to being 'at' the runway.
 */
class RunwayCtl : public WayptController
{
public:
  RunwayCtl(RNAV* aRNAV, const WayptRef& aWpt) :
    WayptController(aRNAV, aWpt),
    _runway(NULL),
    _distanceAircraftRunwayEnd(0.0),
    _courseDev(0.0)
  {
  }

  bool init() override
  {
    _runway = static_cast<RunwayWaypt*>(_waypt.get())->runway();
    if (!_runway) {
        return false;
    }
    _targetTrack = _runway->headingDeg();
    return true;
  }

  virtual void update(double)
  {
    double bearingAircraftRunwayEnd;
    // use the far end of the runway for course deviation calculations. 
    // this should do the correct thing both for takeoffs (including entering 
    // the runway at a taxiway after the threshold) and also landings.
    // seperately compute the distance to the threshold for timeToWaypt calc

    bearingAircraftRunwayEnd	= SGGeodesy::courseDeg(_rnav->position(), _runway->end());
    _distanceAircraftRunwayEnd	= SGGeodesy::distanceM(_rnav->position(), _runway->end());

    _courseDev = bearingAircraftRunwayEnd - _targetTrack;
    SG_NORMALIZE_RANGE(_courseDev, -180.0, 180.0);
    
    if ((fabs(_courseDev) > _rnav->overflightArmAngleDeg()) && (_distanceAircraftRunwayEnd < _rnav->overflightArmDistanceM())) {
      setDone();
    }
  } 
  
  virtual double distanceToWayptM() const
  {
    return SGGeodesy::distanceM(_rnav->position(), _runway->threshold());
  }
  
  virtual double xtrackErrorNm() const
  {
    double x = sin(_courseDev * SG_DEGREES_TO_RADIANS) * _distanceAircraftRunwayEnd;
    return x * SG_METER_TO_NM;
  }

  virtual double courseDeviationDeg() const
  {
    return _courseDev;
  }

  virtual double trueBearingDeg() const
  {
    // as in update(), use runway->end here, so the value remains
    // sensible whether taking off or landing.
    return SGGeodesy::courseDeg(_rnav->position(), _runway->end());
  }
  
  virtual SGGeod position() const
  {
    return _runway->threshold();
  }
private:
  FGRunway* _runway;
  double _distanceAircraftRunwayEnd;
  double _courseDev;
};

class ConstHdgToAltCtl : public WayptController
{
public:
  ConstHdgToAltCtl(RNAV* aRNAV, const WayptRef& aWpt) :
    WayptController(aRNAV, aWpt)
    
  {
    if (_waypt->type() != "hdgToAlt") {
      throw sg_exception("invalid waypoint type", "ConstHdgToAltCtl ctor");
    }
    
    if (_waypt->altitudeRestriction() == RESTRICT_NONE) {
      throw sg_exception("invalid waypoint alt restriction", "ConstHdgToAltCtl ctor");
    }
  }

  bool init() override
  {
    HeadingToAltitude* w = (HeadingToAltitude*) _waypt.get();
    _targetTrack = w->headingDegMagnetic() + _rnav->magvarDeg();
      _filteredFPM = _lastFPM = _rnav->vspeedFPM();
      return true;
  }
  
  virtual void update(double dt)
  {
    double curAlt = _rnav->position().getElevationFt();
      // adjust to get a stable FPM value; bigger values mean slower
      // response but more stable.
      const double RESPONSIVENESS = 1.0;

      _filteredFPM = fgGetLowPass(_lastFPM, _rnav->vspeedFPM(), dt * RESPONSIVENESS);
      _lastFPM = _rnav->vspeedFPM();

    switch (_waypt->altitudeRestriction()) {
    case RESTRICT_AT: 
    case RESTRICT_COMPUTED:  
    {
      double d = curAlt - _waypt->altitudeFt();
      if (fabs(d) < 50.0) {
        SG_LOG(SG_INSTR, SG_INFO, "ConstHdgToAltCtl, reached target altitude " << _waypt->altitudeFt());
        setDone();
      }
    } break;
      
    case RESTRICT_ABOVE:
      if (curAlt >= _waypt->altitudeFt()) {
        SG_LOG(SG_INSTR, SG_INFO, "ConstHdgToAltCtl, above target altitude " << _waypt->altitudeFt());
        setDone();
      }
      break;
      
    case RESTRICT_BELOW:
      if (curAlt <= _waypt->altitudeFt()) {
        SG_LOG(SG_INSTR, SG_INFO, "ConstHdgToAltCtl, below target altitude " << _waypt->altitudeFt());
        setDone();
      }
      break;
    
    default:
      break;
    }
  }
  
  virtual double timeToWaypt() const
  {
    double d = fabs(_rnav->position().getElevationFt() - _waypt->altitudeFt());
    return (d / _filteredFPM) * 60.0;
  }
  
  virtual double distanceToWayptM() const
  {
      // we could filter ground speed here, but it's likely stable enough,
      // and timeToWaypt already filters the FPM value
    double gsMsec = _rnav->groundSpeedKts() * SG_KT_TO_MPS;
    return timeToWaypt() * gsMsec;
  }
  
  virtual SGGeod position() const
  {
    SGGeod p;
    double az2;
    SGGeodesy::direct(_rnav->position(), _targetTrack, distanceToWayptM(), p, az2);
    return p;
  }

private:
    double _lastFPM, _filteredFPM;
};

class InterceptCtl : public WayptController
{
public:
  InterceptCtl(RNAV* aRNAV, const WayptRef& aWpt) :
    WayptController(aRNAV, aWpt),
    _trueRadial(0.0)
  {
    if (_waypt->type() != "radialIntercept") {
      throw sg_exception("invalid waypoint type", "InterceptCtl ctor");
    }
  }

  bool init() override
  {
    RadialIntercept* w = (RadialIntercept*) _waypt.get();
      _trueRadial = w->radialDegMagnetic() + _rnav->magvarDeg();
    _targetTrack = w->courseDegMagnetic() + _rnav->magvarDeg();
      
      _canFlyBy = !_waypt->flag(WPT_OVERFLIGHT) && _rnav->canFlyBy();

      return true;
  }
  
   void update(double) override
  {
      SGGeoc c,
        geocPos = SGGeoc::fromGeod(_rnav->position()),
        geocWayptPos = SGGeoc::fromGeod(_waypt->position());

      bool ok = geocRadialIntersection(geocPos, _targetTrack,
                                       geocWayptPos, _trueRadial, c);
      if (!ok) {
          // try with a backwards offset from the waypt pos, in case the
          // procedure waypt location is too close. (eg, KSFO OCEAN SID)

          SGGeoc navidAdjusted = SGGeodesy::advanceDegM(geocWayptPos, _trueRadial, -10 * SG_NM_TO_METER);
          ok = geocRadialIntersection(geocPos, _targetTrack,
                                      navidAdjusted, _trueRadial, c);
          if (!ok) {
              SG_LOG(SG_INSTR, SG_WARN, "InterceptCtl, bad intersection, skipping waypt");
              setDone();
              return;
          }
      }

      _projectedPosition = SGGeod::fromGeoc(c);
      _distanceToProjectedInterceptM =  SGGeodesy::distanceM(_rnav->position(), _projectedPosition);
      if (!_didComputeTurn && (_distanceToProjectedInterceptM < turnComputeDist)) {
          computeTurn();
      }
      
      if (_doFlyBy) {
          updateFlyByTurn();
      }
      
      
    // note we want the outbound radial from the waypt, hence the ordering
    // of arguments to courseDeg
    double r = SGGeodesy::courseDeg(_waypt->position(), _rnav->position());
      double bearingDiff = r - _trueRadial;
      SG_NORMALIZE_RANGE(bearingDiff, -180.0, 180.0);
    if (fabs(bearingDiff) < 0.5) {
      setDone();
    }
  }
          
    bool updateFlyByTurn()
    {
        // find bearing to turn center
        // when it hits 90 off our track
        
        auto bearingToTurnCenter = SGGeodesy::courseDeg(_rnav->position(), _flyByTurnCenter);
        auto distToTurnCenter = SGGeodesy::distanceM(_rnav->position(), _flyByTurnCenter);

        if (!_flyByStarted) {
          // check for the turn center being 90 degrees off one wing (start the turn)
          auto a = bearingToTurnCenter - _targetTrack;
          SG_NORMALIZE_RANGE(a, -180.0, 180.0);
            if (fabs(a) < 90.0) {
                return false;
          }
          
          _flyByStarted = true;
        }
        
        
        // in the actual turn, our desired track is always pependicular to the
        // bearing to the turn center we computed
        _targetTrack = bearingToTurnCenter - copysign(90, _flyByTurnAngle);
          
        SG_NORMALIZE_RANGE(_targetTrack, 0.0, 360.0);
        _crossTrackError = (distToTurnCenter - _flyByTurnRadius) * SG_METER_TO_NM;
        return true;
    }
    
    void computeTurn()
    {
        _didComputeTurn = true;
        if (!_canFlyBy)
            return;
        
        double inverseRadial = _trueRadial + 180.0;
        SG_NORMALIZE_RANGE(inverseRadial, 0.0, 360.0);
        _flyByTurnAngle = inverseRadial - _targetTrack;
        SG_NORMALIZE_RANGE(_flyByTurnAngle, -180.0, 180.0);
        
        if (fabs(_flyByTurnAngle) > 120.0) {
          // too sharp, no fly-by
          return;
        }
        
        _flyByTurnRadius =  _rnav->turnRadiusNm() * SG_NM_TO_METER;
        _flyByTurnCenter = computeTurnCenter(_flyByTurnRadius, _projectedPosition, _targetTrack, _flyByTurnAngle);
        _doFlyBy = true;
    }
  
double distanceToWayptM() const override
  {
      return _distanceToProjectedInterceptM;
  }

     SGGeod position() const override
    {
        return _projectedPosition;
    }
    
     double xtrackErrorNm() const override
     {
         if (!_flyByStarted)
             return 0.0; // unless we're doing the fly-by, we're always on course
         return _crossTrackError;
     }

    double courseDeviationDeg() const override
     {
         // guessed scaling factor
         return xtrackErrorNm() * 10.0;
     }
private:
  double _trueRadial;
    SGGeod _projectedPosition;
    bool _canFlyBy = false;
    bool _didComputeTurn = false;
    bool _doFlyBy = false;
    double _distanceToProjectedInterceptM = 0.0;
    SGGeod _flyByTurnCenter;
    double _flyByTurnAngle;
    double _flyByTurnRadius;
    bool _flyByStarted = false;
    double _crossTrackError = 0.0;
};

class DMEInterceptCtl : public WayptController
{
public:
  DMEInterceptCtl(RNAV* aRNAV, const WayptRef& aWpt) :
    WayptController(aRNAV, aWpt),
    _dme(NULL),
    _distanceNm(0.0)
  {
    if (_waypt->type() != "dmeIntercept") {
      throw sg_exception("invalid waypoint type", "DMEInterceptCtl ctor");
    }
  }

  bool init() override
  {
    _dme  = (DMEIntercept*) _waypt.get();
    _targetTrack = _dme->courseDegMagnetic() + _rnav->magvarDeg();
    return true;
  }
  
  virtual void update(double)
  {
    _distanceNm = SGGeodesy::distanceNm(_rnav->position(), _dme->position());
    double d = fabs(_distanceNm - _dme->dmeDistanceNm());
    if (d < 0.1) {
      setDone();
    }
  }
  
  virtual double distanceToWayptM() const
  {
    return fabs(_distanceNm - _dme->dmeDistanceNm()) * SG_NM_TO_METER;
  }
  
    virtual SGGeod position() const
    {
        SGGeoc geocPos = SGGeoc::fromGeod(_rnav->position());
        SGGeoc navid = SGGeoc::fromGeod(_dme->position());
        double distRad = _dme->dmeDistanceNm() * SG_NM_TO_RAD;

        // compute radial GC course
        SGGeoc bPt = SGGeodesy::advanceDegM(geocPos, _targetTrack, 1e5);
        double dNm = pointsKnownDistanceFromGC(geocPos, bPt, navid, distRad);
        if (dNm < 0.0) {
            SG_LOG(SG_AUTOPILOT, SG_WARN, "DMEInterceptCtl::position failed");
            return _dme->position(); // horrible fallback
        }

        return SGGeodesy::direct(_rnav->position(), _targetTrack, dNm * SG_NM_TO_METER);
    }

private:
  DMEIntercept* _dme;
  double _distanceNm;
};


HoldCtl::HoldCtl(RNAV* aRNAV, const WayptRef& aWpt) :
  WayptController(aRNAV, aWpt)
  
{
  // find published hold for aWpt
  // do we have this?!
    if (aWpt->type() == "hold") {
        const auto hold = static_cast<Hold*>(aWpt.ptr());
        _holdCourse = hold->inboundRadial();
        if (hold->isDistance())
            _holdLegDistance = hold->timeOrDistance();
        else
            _holdLegTime = hold->timeOrDistance();
        _leftHandTurns = hold->isLeftHanded();
    }
}

bool HoldCtl::init()
{
  _segmentEnd = _waypt->position();
  _state = LEG_TO_HOLD;
  
  // use leg controller to fly us to the hold point - this also gives
  // the normal legl behaviour if the hold is not enabled
  setSubController(new LegWayptCtl(_rnav, _waypt));

  return true;
}
  
void HoldCtl::computeEntry()
{
  const double entryCourse = SGGeodesy::courseDeg(_rnav->position(), _waypt->position());
  const double diff = SGMiscd::normalizePeriodic( -180.0, 180.0, _holdCourse - entryCourse);
  
  if (_leftHandTurns) {
    if ((diff > -70) && (diff < 120.0)) {
      _state = ENTRY_DIRECT;
    } else if (diff < 0.0) {
      _state = ENTRY_PARALLEL;
    } else {
      _state = ENTRY_TEARDROP;
    }
  } else {
    // right handed entry angles
    if ((diff > -120) && (diff < 70.0)) {
      _state = ENTRY_DIRECT;
    } else if (diff > 0.0) {
      _state = ENTRY_PARALLEL;
    } else {
      _state = ENTRY_TEARDROP;
    }
  }
}
    
void HoldCtl::update(double dt)
{
  const auto rnavPos = _rnav->position();
  const double dEnd = SGGeodesy::distanceNm(rnavPos, _segmentEnd);
  
  // fly inbound / outbound sides, or execute the turn
  switch (_state) {
    case LEG_TO_HOLD:
      // update the leg controller
      _subController->update(dt);
      break;
      
    case HOLD_EXITING:
      // in the common case of a hold in a procedure, we often just fly
      // the hold waypoint as leg. Keep running the Leg sub-controller until
      // it's done (WayptController::isDone calls the subcController
      // automaticlaly)
      if (_subController) {
        _subController->update(dt);
      }
      break;

    default:
      break;
  }
  
  if (_inTurn) {
    const double turnOffset = inLeftTurn() ? 90 : -90;
    const double bearingTurnCenter = SGGeodesy::courseDeg(rnavPos, _turnCenter);
    _targetTrack = bearingTurnCenter + turnOffset;
    SG_NORMALIZE_RANGE(_targetTrack, 0.0, 360.0);

    const double angleToTurn = SGMiscd::normalizePeriodic( -180.0, 180.0, _targetTrack - _turnEndAngle);
    if (fabs(angleToTurn) < 0.5) {
      exitTurn();
    }
  } else if (_state == LEG_TO_HOLD) {
    checkInitialEntry(dEnd);
  } else if ((_state == HOLD_INBOUND) || (_state == ENTRY_PARALLEL_INBOUND)) {
    if (checkOverHold()) {
      // we are done
    } else {
      // straight line XTK/deviation
      // computed on-demand in xtrackErrorNm so nothing to do here
    }
  } else if ((_state == HOLD_OUTBOUND) || (_state == ENTRY_TEARDROP)) {
    if (dEnd < 0.2) {
      startInboundTurn();
    } else {
      // straight line XTK
    }
  } else if (_state == ENTRY_PARALLEL_OUTBOUND) {
    if (dEnd < 0.2) {
      startParallelEntryTurn(); // opposite direction to normal
    } else {
      // straight line XTK
    }
  }
}
  
void HoldCtl::setHoldCount(int count)
{
  _holdCount = count;
}
    
void HoldCtl::exitHold()
{
  _holdCount = 0;
}

bool HoldCtl::checkOverHold()
{
  // check distance to entry fix
  const double d = SGGeodesy::distanceNm(_rnav->position(), _waypt->position());
  if (d > 0.2) {
      return false;
  }
  
  if (_holdCount == 0) {
    setDone();
    return true;
  }
    
    startOutboundTurn();
  return true;
}
    
void HoldCtl::checkInitialEntry(double dNm)
{
  _turnRadius = _rnav->turnRadiusNm();
  if (dNm > _turnRadius) {
    // keep going;
    return;
  }
  
  if (_holdCount == 0) {
    // we're done, but we want to keep the leg controller going until
    // we're right on top
    setDone();
    
    // ensure we keep running the Leg cub-controller until it's done,
    // which happens a bit later
    _state = HOLD_EXITING;
    return;
  }
  
  // clear the leg controller we were using to fly us to the hold
  setSubController(nullptr);
  computeEntry();
  
  if (_state == ENTRY_DIRECT) {
    startOutboundTurn();
  } else if (_state == ENTRY_TEARDROP) {
    _targetTrack = _holdCourse + (_leftHandTurns ? -150 : 150);
    SG_NORMALIZE_RANGE(_targetTrack, 0.0, 360.0);
    _segmentEnd = outboundEndPoint();
  } else {
    // parallel entry
    assert(_state == ENTRY_PARALLEL);
    _state = ENTRY_PARALLEL_OUTBOUND;
    _targetTrack = _holdCourse + 180;
    SG_NORMALIZE_RANGE(_targetTrack, 0.0, 360.0);
    const double legLengthM = holdLegLengthNm() * SG_NM_TO_METER;
    _segmentEnd = SGGeodesy::direct(_waypt->position(), _holdCourse, -legLengthM);
  }
}

void HoldCtl::startInboundTurn()
{
  _state = HOLD_INBOUND;
  _inTurn = true;
  _turnCenter = inboundTurnCenter();
  _turnRadius = _rnav->turnRadiusNm();
  _turnEndAngle = _holdCourse;
  SG_NORMALIZE_RANGE(_turnEndAngle, 0.0, 360.0);
}

void HoldCtl::startOutboundTurn()
{
  _state = HOLD_OUTBOUND;
  _inTurn = true;
  _turnCenter = outboundTurnCenter();
  _turnRadius = _rnav->turnRadiusNm();
  _turnEndAngle = _holdCourse + 180.0;
  SG_NORMALIZE_RANGE(_turnEndAngle, 0.0, 360.0);
}
  
void HoldCtl::startParallelEntryTurn()
{
  _state = ENTRY_PARALLEL_INBOUND;
  _inTurn = true;
  _turnRadius = _rnav->turnRadiusNm();
  _turnCenter = inboundTurnCenter();
  _turnEndAngle = _holdCourse + (_leftHandTurns ? 45.0 : -45.0);
  SG_NORMALIZE_RANGE(_turnEndAngle, 0.0, 360.0);
}
  
void HoldCtl::exitTurn()
{
  _inTurn = false;
  switch (_state) {
  case HOLD_INBOUND:
      _targetTrack = _holdCourse;
      _segmentEnd = _waypt->position();
      break;
      
  case ENTRY_PARALLEL_INBOUND:
      // possible improvement : fly the current track until the bearing tp
      // the hold point matches the hold radial. This would cause us to fly
      // a neater parallel entry, rather than flying to the hold point
      // direct from where we are now.
       _targetTrack = SGGeodesy::courseDeg(_rnav->position(), _waypt->position());
      _segmentEnd = _waypt->position();
      break;
          
  case HOLD_OUTBOUND:
      _targetTrack = _holdCourse + 180.0;
      SG_NORMALIZE_RANGE(_targetTrack, 0.0, 360.0);
      // start a timer for timed holds?
      _segmentEnd = outboundEndPoint();
      break;
      
  default:
      SG_LOG(SG_INSTR, SG_DEV_WARN, "HoldCOntroller: bad state at exitTurn:" << _state);
  }
}
  
SGGeod HoldCtl::outboundEndPoint()
{
  // FIXME flip for left hand-turns
  const double turnRadiusM = _rnav->turnRadiusNm() * SG_NM_TO_METER;
  const double legLengthM = holdLegLengthNm() * SG_NM_TO_METER;
  const auto p1 = SGGeodesy::direct(_waypt->position(), _holdCourse, -legLengthM);
  const double turnOffset = _leftHandTurns ? -90 : 90;
  return SGGeodesy::direct(p1, _holdCourse + turnOffset, turnRadiusM * 2.0);
}

SGGeod HoldCtl::outboundTurnCenter()
{
  const double turnOffset = _leftHandTurns ? -90 : 90;
  return SGGeodesy::direct(_waypt->position(), _holdCourse + turnOffset, _rnav->turnRadiusNm() * SG_NM_TO_METER);
}
  
SGGeod HoldCtl::inboundTurnCenter()
{
  const double legLengthM = holdLegLengthNm() * SG_NM_TO_METER;
  const auto p1 = SGGeodesy::direct(_waypt->position(), _holdCourse, -legLengthM);
  const double turnOffset = _leftHandTurns ? -90 : 90;
  return SGGeodesy::direct(p1, _holdCourse + turnOffset, _rnav->turnRadiusNm() * SG_NM_TO_METER);
}
        
double HoldCtl::distanceToWayptM() const
{
  return -1.0;
}

SGGeod HoldCtl::position() const
{
  return _waypt->position();
}
  
bool HoldCtl::inLeftTurn() const
{
    return (_state == ENTRY_PARALLEL_INBOUND) ? !_leftHandTurns : _leftHandTurns;
}

double HoldCtl::holdLegLengthNm() const
{
  const double gs = _rnav->groundSpeedKts();
  if (_holdLegTime > 0.0) {
    return _holdLegTime * gs / 3600.0;
  }
  
  return _holdLegDistance;
}
  
double HoldCtl::xtrackErrorNm() const
{
  if (_subController) {
    return _subController->xtrackErrorNm();
  }
  
  if (_inTurn) {
    const double dR = SGGeodesy::distanceNm(_turnCenter, _rnav->position());
    const double xtk = dR - _turnRadius;
    return inLeftTurn() ? -xtk : xtk;
  } else {
      const double courseToSegmentEnd = SGGeodesy::courseDeg(_rnav->position(), _segmentEnd);
      const double courseDev = courseToSegmentEnd - _targetTrack;
      const auto d = SGGeodesy::distanceNm(_rnav->position(), _segmentEnd);
      return greatCircleCrossTrackError(d, courseDev);
  }
}
      
double HoldCtl::courseDeviationDeg() const
{
  if (_subController) {
    return _subController->courseDeviationDeg();
  }
  
  // convert XTK to 'dots' deviation
  // assuming 10-degree peg to peg, this means 0.1nm of course is
  // one degree of error, feels about right for a hold
  return xtrackErrorNm() * 10.0;
}
    
  std::string HoldCtl::status() const
  {
    switch (_state) {
      case LEG_TO_HOLD:     return "leg-to-hold";
      case HOLD_INIT:       return "hold-init";
      case ENTRY_DIRECT:    return "entry-direct";
      case ENTRY_PARALLEL:
      case ENTRY_PARALLEL_OUTBOUND:
      case ENTRY_PARALLEL_INBOUND:
        return "entry-parallel";
      case ENTRY_TEARDROP:
        return "entry-teardrop";
        
      case HOLD_OUTBOUND:   return "hold-outbound";
      case HOLD_INBOUND:    return "hold-inbound";
      case HOLD_EXITING:    return "hold-exiting";
    }

    throw std::domain_error("Unsupported HoldState.");
  }

///////////////////////////////////////////////////////////////////////////////////
  
class VectorsCtl : public WayptController
{
public:
  VectorsCtl(RNAV* aRNAV, const WayptRef& aWpt) :
    WayptController(aRNAV, aWpt)
    
  {
  }

  bool init() override
  {
      return true;
  }
  
  virtual void update(double)
  {
    setDone();
  }
  
  virtual double distanceToWayptM() const
  {
    return -1.0;
  }
  
  virtual SGGeod position() const
  {
    return _waypt->position();
  }

private:
};

///////////////////////////////////////////////////////////////////////////////

DirectToController::DirectToController(RNAV* aRNAV, const WayptRef& aWpt, const SGGeod& aOrigin) :
  WayptController(aRNAV, aWpt),
  _origin(aOrigin),
  _distanceAircraftTargetMeter(0.0),
  _courseDev(0.0),
  _courseAircraftToTarget(0.0)
{
}

bool DirectToController::init()
{
  if (_waypt->flag(WPT_DYNAMIC)) {
      SG_LOG(SG_AUTOPILOT, SG_WARN, "can't use a dynamic waypoint for direct-to: " << _waypt->ident());
      return false;
  }

  _targetTrack = SGGeodesy::courseDeg(_origin, _waypt->position());
  return true;
}

void DirectToController::update(double)
{
  double courseOriginToAircraft;
  double az2, distanceOriginToAircraft;
  SGGeodesy::inverse(_origin,_rnav->position(), courseOriginToAircraft, az2, distanceOriginToAircraft);
  if (distanceOriginToAircraft < 100.0) {
    // if we are very close to the origin point, use the target track so
    // course deviation comes out as zero
    courseOriginToAircraft = _targetTrack;
  }
    
  _courseAircraftToTarget		= SGGeodesy::courseDeg(_rnav->position(),_waypt->position());
  _distanceAircraftTargetMeter	= SGGeodesy::distanceM(_rnav->position(),_waypt->position());

  _courseDev = _courseAircraftToTarget - _targetTrack;
  SG_NORMALIZE_RANGE(_courseDev, -180.0, 180.0);

  bool isMinimumOverFlightDistanceReached 	= _distanceAircraftTargetMeter < _rnav->overflightDistanceM();
  bool isOverFlightConeArmed 				= _distanceAircraftTargetMeter < ( _rnav->overflightArmDistanceM() + _rnav->overflightDistanceM() );
  bool leavingOverFlightCone 				= (fabs(_courseAircraftToTarget - _targetTrack) > _rnav->overflightArmAngleDeg());

  if( isMinimumOverFlightDistanceReached ){
  	setDone();
  }else{
		if( isOverFlightConeArmed && leavingOverFlightCone ){
				setDone();
		}
  }
}

double DirectToController::distanceToWayptM() const
{
  return _distanceAircraftTargetMeter;
}

double DirectToController::xtrackErrorNm() const
{
	return greatCircleCrossTrackError(_distanceAircraftTargetMeter * SG_METER_TO_NM, _courseDev);
}

double DirectToController::courseDeviationDeg() const
{
  return _courseDev;
}

double DirectToController::trueBearingDeg() const
{
  return _courseAircraftToTarget;
}

SGGeod DirectToController::position() const
{
  return _waypt->position();
}

///////////////////////////////////////////////////////////////////////////////

OBSController::OBSController(RNAV* aRNAV, const WayptRef& aWpt) :
  WayptController(aRNAV, aWpt),
  _distanceAircraftTargetMeter(0.0),
  _courseDev(0.0),
  _courseAircraftToTarget(0.0)
{
}

bool OBSController::init()
{
  if (_waypt->flag(WPT_DYNAMIC)) {
      SG_LOG(SG_AUTOPILOT, SG_WARN, "can't use a dynamic waypoint for OBS mode" << _waypt->ident());
      return false;
  }
  
  _targetTrack = _rnav->selectedMagCourse() + _rnav->magvarDeg();
  return true;
}

void OBSController::update(double)
{
  _targetTrack = _rnav->selectedMagCourse() + _rnav->magvarDeg();

  _courseAircraftToTarget		= SGGeodesy::courseDeg(_rnav->position(),_waypt->position());
  _distanceAircraftTargetMeter	= SGGeodesy::distanceM(_rnav->position(),_waypt->position());

  // _courseDev inverted we use point target as origin
  _courseDev = (_courseAircraftToTarget - _targetTrack);
  SG_NORMALIZE_RANGE(_courseDev, -180.0, 180.0);
}

bool OBSController::toFlag() const
{
  return (fabs(_courseDev) < _rnav->overflightArmAngleDeg());
}

double OBSController::distanceToWayptM() const
{
  return _distanceAircraftTargetMeter;
}

double OBSController::xtrackErrorNm() const
{
	return greatCircleCrossTrackError(_distanceAircraftTargetMeter * SG_METER_TO_NM, _courseDev);
}

double OBSController::courseDeviationDeg() const
{
//  if (fabs(_courseDev) > 90.0) {
 //   double d = -_courseDev;
 //   SG_NORMALIZE_RANGE(d, -90.0, 90.0);
  //  return d;
  //}
  
  return _courseDev;
}

double OBSController::trueBearingDeg() const
{
  return _courseAircraftToTarget;
}

SGGeod OBSController::position() const
{
  return _waypt->position();
}

///////////////////////////////////////////////////////////////////////////////

WayptController* WayptController::createForWaypt(RNAV* aRNAV, const WayptRef& aWpt)
{
  if (!aWpt) {
    throw sg_exception("Passed null waypt", "WayptController::createForWaypt");
  }
  
  const std::string& wty(aWpt->type());
  if (wty == "runway") {
    return new RunwayCtl(aRNAV, aWpt);
  }
  
  if (wty == "radialIntercept") {
    return new InterceptCtl(aRNAV, aWpt);
  }
  
  if (wty == "dmeIntercept") {
    return new DMEInterceptCtl(aRNAV, aWpt);
  }
  
  if (wty == "hdgToAlt") {
    return new ConstHdgToAltCtl(aRNAV, aWpt);
  }
  
  if (wty == "vectors") {
    return new VectorsCtl(aRNAV, aWpt);
  }
  
  if (wty == "hold") {
    return new HoldCtl(aRNAV, aWpt);
  }
  
  return new LegWayptCtl(aRNAV, aWpt);
}

} // of namespace flightgear

