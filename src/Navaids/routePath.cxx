
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <Navaids/routePath.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/globals.hxx>
#include <Airports/runways.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/FlightPlan.hxx>
#include <Navaids/positioned.hxx>

namespace flightgear {
  bool geocRadialIntersection(const SGGeoc& a, double r1, const SGGeoc& b, double r2, SGGeoc& result);
}

using namespace flightgear;

// implement Point(s) known distance from a great circle

static double sqr(const double x)
{
  return x * x;
}

double pointsKnownDistanceFromGC(const SGGeoc& a, const SGGeoc&b, const SGGeoc& d, double dist)
{
  double A = SGGeodesy::courseRad(a, d) - SGGeodesy::courseRad(a, b);
  double bDist = SGGeodesy::distanceRad(a, d);
  
  // r=(cos(b)^2+sin(b)^2*cos(A)^2)^(1/2)
  double r = pow(sqr(cos(bDist)) + sqr(sin(bDist)) * sqr(cos(A)), 0.5); 
  
  double p = atan2(sin(bDist)*cos(A), cos(bDist));
  
  if (sqr(cos(dist)) > sqr(r)) {
    SG_LOG(SG_GENERAL, SG_INFO, "pointsKnownDistanceFromGC, no points exist");
    return -1.0;
  }
  
  double dp1 = p + acos(cos(dist)/r);
  double dp2 = p - acos(cos(dist)/r);
  
  double dp1Nm = fabs(dp1 * SG_RAD_TO_NM);
  double dp2Nm = fabs(dp2 * SG_RAD_TO_NM);
  
  return SGMiscd::min(dp1Nm, dp2Nm);
}

RoutePath::RoutePath(const flightgear::WayptVec& wpts) :
  _waypts(wpts)
{
  commonInit();
}

RoutePath::RoutePath(const flightgear::FlightPlan* fp)
{
  for (int l=0; l<fp->numLegs(); ++l) {
    _waypts.push_back(fp->legAtIndex(l)->waypoint());
  }
  commonInit();
}

void RoutePath::commonInit()
{
  _pathClimbFPM = 1200;
  _pathDescentFPM = 800;
  _pathIAS = 190; 
  _pathTurnRate = 3.0; // 3 deg/sec = 180def/min = standard rate turn  
}

SGGeodVec RoutePath::pathForIndex(int index) const
{
  if (index == 0) {
    return SGGeodVec(); // no path for first waypoint
  }
  
  if (_waypts[index]->type() == "vectors") {
    return SGGeodVec(); // empty
  }
  
  if (_waypts[index]->type() == "hold") {
    return pathForHold((Hold*) _waypts[index].get());
  }
    
  SGGeodVec r;
  SGGeod pos;
  if (!computedPositionForIndex(index-1, pos)) {
    return SGGeodVec();
  }
  
  r.push_back(pos);
  if (!computedPositionForIndex(index, pos)) {
    return SGGeodVec();
  }
  
  r.push_back(pos);
  
  if (_waypts[index]->type() == "runway") {
    // runways get an extra point, at the end. this is particularly
    // important so missed approach segments draw correctly
    FGRunway* rwy = static_cast<RunwayWaypt*>(_waypts[index].get())->runway();
    r.push_back(rwy->end());
  }
  
  return r;
}

SGGeod RoutePath::positionForIndex(int index) const
{
  SGGeod r;
  bool ok = computedPositionForIndex(index, r);
  if (!ok) {
    return SGGeod();
  }
  
  return r;
}

SGGeodVec RoutePath::pathForHold(Hold* hold) const
{
  int turnSteps = 16;
  double hdg = hold->inboundRadial();
  double turnDelta = 180.0 / turnSteps;
  
  SGGeodVec r;
  double az2;
  double stepTime = turnDelta / _pathTurnRate; // in seconds
  double stepDist = _pathIAS * (stepTime / 3600.0) * SG_NM_TO_METER;
  double legDist = hold->isDistance() ? 
    hold->timeOrDistance() 
    : _pathIAS * (hold->timeOrDistance() / 3600.0);
  legDist *= SG_NM_TO_METER;
  
  if (hold->isLeftHanded()) {
    turnDelta = -turnDelta;
  }  
  SGGeod pos = hold->position();
  r.push_back(pos);

  // turn+leg sides are a mirror
  for (int j=0; j < 2; ++j) {
  // turn
    for (int i=0;i<turnSteps; ++i) {
      hdg += turnDelta;
      SGGeodesy::direct(pos, hdg, stepDist, pos, az2);
      r.push_back(pos);
    }
    
  // leg
    SGGeodesy::direct(pos, hdg, legDist, pos, az2);
    r.push_back(pos);
  } // of leg+turn duplication
  
  return r;
}

/**
 * the path context holds the state of of an imaginary aircraft traversing
 * the route, and limits the rate at which heading / altitude / position can
 * change
 */
class RoutePath::PathCtx
{
public:
  SGGeod pos;
  double heading;
};

bool RoutePath::computedPositionForIndex(int index, SGGeod& r) const
{
  if ((index < 0) || (index >= (int) _waypts.size())) {
    throw sg_range_exception("waypt index out of range", 
      "RoutePath::computedPositionForIndex");
  }

  WayptRef w = _waypts[index];
  if (!w->flag(WPT_DYNAMIC)) {
    r = w->position();
    return true;
  }
  
  if (w->type() == "radialIntercept") {
    // radial intersection along track
    SGGeod prev;
    if (!computedPositionForIndex(index - 1, prev)) {
      return false;
    }
  
    SGGeoc prevGc = SGGeoc::fromGeod(prev);
    SGGeoc navid = SGGeoc::fromGeod(w->position());
    SGGeoc rGc;
    double magVar = magVarFor(prev);
    
    RadialIntercept* i = (RadialIntercept*) w.get();
    double radial = i->radialDegMagnetic() + magVar;
    double track = i->courseDegMagnetic() + magVar;
    bool ok = geocRadialIntersection(prevGc, track, navid, radial, rGc);
    if (!ok) {
      return false;
    }
    
    r = SGGeod::fromGeoc(rGc);
    return true;
  } else if (w->type() == "dmeIntercept") {
    // find the point along the DME track, from prev, that is the correct distance
    // from the DME
    SGGeod prev;
    if (!computedPositionForIndex(index - 1, prev)) {
      return false;
    }
    
    DMEIntercept* di = (DMEIntercept*) w.get();
    
    SGGeoc prevGc = SGGeoc::fromGeod(prev);
    SGGeoc navid = SGGeoc::fromGeod(w->position());
    double distRad = di->dmeDistanceNm() * SG_NM_TO_RAD;
    SGGeoc rGc;

    SGGeoc bPt;
    double crs = di->courseDegMagnetic() + magVarFor(prev);
    SGGeodesy::advanceRadM(prevGc, crs, 100 * SG_NM_TO_RAD, bPt);

    double dNm = pointsKnownDistanceFromGC(prevGc, bPt, navid, distRad);
    if (dNm < 0.0) {
      return false;
    }
    
    double az2;
    SGGeodesy::direct(prev, crs, dNm * SG_NM_TO_METER, r, az2);
    return true;
  } else if (w->type() == "hdgToAlt") {
    HeadingToAltitude* h = (HeadingToAltitude*) w.get();
    double climb = h->altitudeFt() - computeAltitudeForIndex(index - 1);
    double d = distanceForClimb(climb);

    SGGeod prevPos;
    if (!computedPositionForIndex(index - 1, prevPos)) {
      return false;
    }
    
    double hdg = h->headingDegMagnetic() + magVarFor(prevPos);
    
    double az2;
    SGGeodesy::direct(prevPos, hdg, d * SG_NM_TO_METER, r, az2);
    return true;
  } else if (w->type() == "vectors"){
    return false;
  } else if (w->type() == "hold") {
    r = w->position();
    return true;
  }
  
  SG_LOG(SG_GENERAL, SG_INFO, "RoutePath::computedPositionForIndex: unhandled type:" << w->type());
  return false;
}

double RoutePath::computeAltitudeForIndex(int index) const
{
  if ((index < 0) || (index >= (int) _waypts.size())) {
    throw sg_range_exception("waypt index out of range", 
      "RoutePath::computeAltitudeForIndex");
  }
  
  WayptRef w = _waypts[index];
  if (w->altitudeRestriction() != RESTRICT_NONE) {
    return w->altitudeFt(); // easy!
  }
  
  if (w->type() == "runway") {
    FGRunway* rwy = static_cast<RunwayWaypt*>(w.get())->runway();
    return rwy->threshold().getElevationFt();
  } else if ((w->type() == "hold") || (w->type() == "vectors")) {
    // pretend we don't change altitude in holds/vectoring
    return computeAltitudeForIndex(index - 1);
  }

  double prevAlt = computeAltitudeForIndex(index - 1);
// find distance to previous, and hence climb/descent 
  SGGeod pos, prevPos;
  
  if (!computedPositionForIndex(index, pos) ||
      !computedPositionForIndex(index - 1, prevPos))
  {
    SG_LOG(SG_GENERAL, SG_WARN, "unable to compute position for waypoints");
    throw sg_range_exception("unable to compute position for waypoints");
  }
  
  double d = SGGeodesy::distanceNm(prevPos, pos);
  double tMinutes = (d / _pathIAS) * 60.0; // (nm / knots) * 60 = time in minutes
  
  double deltaFt; // change in altitude in feet
  if (w->flag(WPT_ARRIVAL) && !w->flag(WPT_MISS)) {
    deltaFt = -_pathDescentFPM * tMinutes;
  } else {
    deltaFt = _pathClimbFPM * tMinutes;
  }
  
  return prevAlt + deltaFt;
}

double RoutePath::computeTrackForIndex(int index) const
{
  if ((index < 0) || (index >= (int) _waypts.size())) {
    throw sg_range_exception("waypt index out of range", 
      "RoutePath::computeTrackForIndex");
  }
  
  WayptRef w = _waypts[index];
  if (w->type() == "radialIntercept") {
    RadialIntercept* r = (RadialIntercept*) w.get();
    return r->courseDegMagnetic();
  } else if (w->type() == "dmeIntercept") {
    DMEIntercept* d = (DMEIntercept*) w.get();
    return d->courseDegMagnetic();
  } else if (w->type() == "hdgToAlt") {
    HeadingToAltitude* h = (HeadingToAltitude*) w.get();
    return h->headingDegMagnetic();
  } else if (w->type() == "hold") {
    Hold* h = (Hold*) w.get();
    return h->inboundRadial();
  } else if (w->type() == "vectors") {
    SG_LOG(SG_GENERAL, SG_WARN, "asked for track from VECTORS");
    throw sg_range_exception("asked for track from vectors waypt");
  } else if (w->type() == "runway") {
    FGRunway* rwy = static_cast<RunwayWaypt*>(w.get())->runway();
    return rwy->headingDeg();
  }
  
// course based upon previous and current pos
  SGGeod pos, prevPos;
  
  if (!computedPositionForIndex(index, pos) ||
      !computedPositionForIndex(index - 1, prevPos))
  {
    SG_LOG(SG_GENERAL, SG_WARN, "unable to compute position for waypoints");
    throw sg_range_exception("unable to compute position for waypoints");
  }

  return SGGeodesy::courseDeg(prevPos, pos);
}

double RoutePath::distanceForClimb(double climbFt) const
{
  double t = 0.0; // in seconds
  if (climbFt > 0.0) {
    t = (climbFt / _pathClimbFPM) * 60.0;
  } else if (climbFt < 0.0) {
    t = (climbFt / _pathDescentFPM) * 60.0;
  }
  
  return _pathIAS * (t / 3600.0);
}

double RoutePath::magVarFor(const SGGeod& geod) const
{
  double jd = globals->get_time_params()->getJD();
  return sgGetMagVar(geod, jd) * SG_RADIANS_TO_DEGREES;
}

