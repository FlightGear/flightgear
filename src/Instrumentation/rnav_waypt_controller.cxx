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

#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>

#include <Airports/runways.hxx>
#include "Main/util.hxx" // for fgLowPass

extern double pointsKnownDistanceFromGC(const SGGeoc& a, const SGGeoc&b, const SGGeoc& d, double dist);

namespace flightgear
{

// implementation of
// http://williams.best.vwh.net/avform.htm#Intersection
bool geocRadialIntersection(const SGGeoc& a, double r1, const SGGeoc& b, double r2, SGGeoc& result)
{
  double crs13 = r1 * SG_DEGREES_TO_RADIANS;
  double crs23 = r2 * SG_DEGREES_TO_RADIANS;
  double dst12 = SGGeodesy::distanceRad(a, b);
  
  //IF sin(lon2-lon1)<0
  // crs12=acos((sin(lat2)-sin(lat1)*cos(dst12))/(sin(dst12)*cos(lat1)))
  // crs21=2.*pi-acos((sin(lat1)-sin(lat2)*cos(dst12))/(sin(dst12)*cos(lat2)))
  // ELSE
  // crs12=2.*pi-acos((sin(lat2)-sin(lat1)*cos(dst12))/(sin(dst12)*cos(lat1)))
  // crs21=acos((sin(lat1)-sin(lat2)*cos(dst12))/(sin(dst12)*cos(lat2)))
  // ENDIF

  double sinLat1 = sin(a.getLatitudeRad());
  double cosLat1 = cos(a.getLatitudeRad());
  double sinDst12 = sin(dst12);
  double cosDst12 = cos(dst12);
  
  double crs12 = SGGeodesy::courseRad(a, b),
    crs21 = SGGeodesy::courseRad(b, a);
    

  // normalise to -pi .. pi range
  double ang1 = SGMiscd::normalizeAngle(crs13-crs12);
  double ang2 = SGMiscd::normalizeAngle(crs21-crs23);
    
  if ((sin(ang1) == 0.0) && (sin(ang2) == 0.0)) {
    SG_LOG(SG_INSTR, SG_INFO, "geocRadialIntersection: infinity of intersections");
    return false;
  }
  
  if ((sin(ang1)*sin(ang2))<0.0) {
   // SG_LOG(SG_INSTR, SG_INFO, "geocRadialIntersection: intersection ambiguous:"
    //       << ang1 << " " << ang2 << " sin1 " << sin(ang1) << " sin2 " << sin(ang2));
    return false;
  }
  
  ang1 = fabs(ang1);
  ang2 = fabs(ang2);

  //ang3=acos(-cos(ang1)*cos(ang2)+sin(ang1)*sin(ang2)*cos(dst12)) 
  //dst13=atan2(sin(dst12)*sin(ang1)*sin(ang2),cos(ang2)+cos(ang1)*cos(ang3))
  //lat3=asin(sin(lat1)*cos(dst13)+cos(lat1)*sin(dst13)*cos(crs13))
  
  //lon3=mod(lon1-dlon+pi,2*pi)-pi

  double ang3 = acos(-cos(ang1) * cos(ang2) + sin(ang1) * sin(ang2) * cosDst12);
  double dst13 = atan2(sinDst12 * sin(ang1) * sin(ang2), cos(ang2) + cos(ang1)*cos(ang3));

  SGGeoc pt3;
  SGGeodesy::advanceRadM(a, crs13, dst13 * SG_RAD_TO_NM * SG_NM_TO_METER, pt3);

  double lat3 = asin(sinLat1 * cos(dst13) + cosLat1 * sin(dst13) * cos(crs13));
  
  //dlon=atan2(sin(crs13)*sin(dst13)*cos(lat1),cos(dst13)-sin(lat1)*sin(lat3))
  double dlon = atan2(sin(crs13)*sin(dst13)*cosLat1, cos(dst13)- (sinLat1 * sin(lat3)));
  double lon3 = SGMiscd::normalizeAngle(-a.getLongitudeRad()-dlon);
  
  result = SGGeoc::fromRadM(-lon3, lat3, a.getRadiusM());
  //result = pt3;
  return true;
}

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


////////////////////////////////////////////////////////////////////////////

WayptController::~WayptController()
{
}

void WayptController::init()
{
}

void WayptController::setDone()
{
  if (_isDone) {
    SG_LOG(SG_AUTOPILOT, SG_WARN, "already done @ WayptController::setDone");
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
  
  virtual void init()
  {
    _targetTrack = SGGeodesy::courseDeg(_rnav->position(), _waypt->position());
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

  virtual void init()
  {
	double courseOriginTarget;
	bool isPreviousLegValid = false;

	_waypointOrigin = _rnav->previousLegWaypointPosition(isPreviousLegValid);

	courseOriginTarget 				= SGGeodesy::courseDeg(_waypointOrigin,_waypt->position());

	_courseAircraftToTarget			= SGGeodesy::courseDeg(_rnav->position(),_waypt->position());
	_distanceAircraftTargetMetre 	= SGGeodesy::distanceM(_rnav->position(),_waypt->position());


	// check reach the leg in 45Deg or going direct
	bool canReachLeg = (fabs(courseOriginTarget -_courseAircraftToTarget) < 45.0);

	if ( isPreviousLegValid && canReachLeg){
		_targetTrack = courseOriginTarget;
	}else{
		_targetTrack = _courseAircraftToTarget;
		_waypointOrigin	 = _rnav->position();
	}

  }

  virtual void update(double)
  {
    _courseOriginToAircraft			= SGGeodesy::courseDeg(_waypointOrigin,_rnav->position());
    _distanceOriginAircraftMetre 	= SGGeodesy::distanceM(_waypointOrigin,_rnav->position());

    _courseAircraftToTarget			= SGGeodesy::courseDeg(_rnav->position(),_waypt->position());
    _distanceAircraftTargetMetre 	= SGGeodesy::distanceM(_rnav->position(),_waypt->position());

    _courseDev = -(_courseOriginToAircraft - _targetTrack);

    bool isMinimumOverFlightDistanceReached = _distanceAircraftTargetMetre < _rnav->overflightDistanceM();
    bool isOverFlightConeArmed 				= _distanceAircraftTargetMetre < ( _rnav->overflightArmDistanceM() + _rnav->overflightDistanceM() );
    bool leavingOverFlightCone 				= (fabs(_courseAircraftToTarget - _targetTrack) > _rnav->overflightArmAngleDeg());

    if( isMinimumOverFlightDistanceReached ){
    	_toFlag = false;
    	setDone();
    }else{
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

  virtual double distanceToWayptM() const
  {
    return _distanceAircraftTargetMetre;
  }

  virtual double xtrackErrorNm() const
  {
	  return greatCircleCrossTrackError(_distanceOriginAircraftMetre * SG_METER_TO_NM, _courseDev);
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
  double _distanceOriginAircraftMetre;
  double _distanceAircraftTargetMetre;
  double _courseOriginToAircraft;
  double _courseAircraftToTarget;
  double _courseDev;
  bool _toFlag;

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
  
  virtual void init()
  {
    _runway = static_cast<RunwayWaypt*>(_waypt.get())->runway();
    _targetTrack = _runway->headingDeg();
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

    double _courseDev = bearingAircraftRunwayEnd - _targetTrack;
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

  virtual void init()
  {
    HeadingToAltitude* w = (HeadingToAltitude*) _waypt.get();
    _targetTrack = w->headingDegMagnetic() + _rnav->magvarDeg();
      _filteredFPM = _lastFPM = _rnav->vspeedFPM();
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

  virtual void init()
  {
    RadialIntercept* w = (RadialIntercept*) _waypt.get();
      _trueRadial = w->radialDegMagnetic() + _rnav->magvarDeg();
    _targetTrack = w->courseDegMagnetic() + _rnav->magvarDeg();
  }
  
  virtual void update(double)
  {
      SGGeoc c,
        geocPos = SGGeoc::fromGeod(_rnav->position()),
        geocWayptPos = SGGeoc::fromGeod(_waypt->position());

      bool ok = geocRadialIntersection(geocPos, _targetTrack,
                                       geocWayptPos, _trueRadial, c);
      if (!ok) {
          // try with a backwards offset from the waypt pos, in case the
          // procedure waypt location is too close. (eg, KSFO OCEAN SID)

          SGGeoc navidAdjusted;
          SGGeodesy::advanceRadM(geocWayptPos, _trueRadial, SG_NM_TO_METER * -10, navidAdjusted);

          ok = geocRadialIntersection(geocPos, _targetTrack,
                                      navidAdjusted, _trueRadial, c);
          if (!ok) {
              SG_LOG(SG_INSTR, SG_WARN, "InterceptCtl, bad intersection, skipping waypt");
              setDone();
              return;
          }
      }

      _projectedPosition = SGGeod::fromGeoc(c);


    // note we want the outbound radial from the waypt, hence the ordering
    // of arguments to courseDeg
    double r = SGGeodesy::courseDeg(_waypt->position(), _rnav->position());
      double bearingDiff = r - _trueRadial;
      SG_NORMALIZE_RANGE(bearingDiff, -180.0, 180.0);
    if (fabs(bearingDiff) < 0.5) {
      setDone();
    }
  }
  
  virtual double distanceToWayptM() const
  {
    return SGGeodesy::distanceM(_rnav->position(), position());
  }

    virtual SGGeod position() const
    {
        return _projectedPosition;
    }
private:
  double _trueRadial;
    SGGeod _projectedPosition;
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

  virtual void init()
  {
    _dme  = (DMEIntercept*) _waypt.get();
    _targetTrack = _dme->courseDegMagnetic() + _rnav->magvarDeg();
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
        SGGeoc bPt;
        SGGeodesy::advanceRadM(geocPos, _targetTrack, 100 * SG_NM_TO_RAD, bPt);

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

class HoldCtl : public WayptController
{
public:
  HoldCtl(RNAV* aRNAV, const WayptRef& aWpt) :
    WayptController(aRNAV, aWpt)
    
  {

  }

  virtual void init()
  {
  }
  
  virtual void update(double)
  {
    // fly inbound / outbound sides, or execute the turn
  #if 0
    if (inTurn) {
    
      targetTrack += dt * turnRateSec * turnDirection;
      if (inbound) {
        if .. targetTrack has passed inbound radial, doen with this turn
      } else {
        if target track has passed reciprocal radial done with turn
      }
    } else {
      check time / distance elapsed
      
      if (sideDone) {
        inTurn = true;
        inbound = !inbound;
        nextHeading = inbound;
        if (!inbound) {
          nextHeading += 180.0;
          SG_NORMALIZE_RANGE(nextHeading, 0.0, 360.0);
        }
      }
    
    }
  
  #endif
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
};

class VectorsCtl : public WayptController
{
public:
  VectorsCtl(RNAV* aRNAV, const WayptRef& aWpt) :
    WayptController(aRNAV, aWpt)
    
  {
  }

  virtual void init()
  {
 
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

void DirectToController::init()
{
  if (_waypt->flag(WPT_DYNAMIC)) {
    throw sg_exception("can't direct-to a dynamic waypoint");
  }

  _targetTrack = SGGeodesy::courseDeg(_origin, _waypt->position());
}

void DirectToController::update(double)
{
  double courseOriginToAircraft;

  courseOriginToAircraft 		= SGGeodesy::courseDeg(_origin,_rnav->position());

  _courseAircraftToTarget		= SGGeodesy::courseDeg(_rnav->position(),_waypt->position());
  _distanceAircraftTargetMeter	= SGGeodesy::distanceM(_rnav->position(),_waypt->position());

  _courseDev = -(courseOriginToAircraft - _targetTrack);

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

void OBSController::init()
{
  if (_waypt->flag(WPT_DYNAMIC)) {
    throw sg_exception("can't use a dynamic waypoint for OBS mode");
  }
  
  _targetTrack = _rnav->selectedMagCourse() + _rnav->magvarDeg();
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

