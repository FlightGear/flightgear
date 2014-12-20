
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

// http://williams.best.vwh.net/avform.htm#POINTDME
double pointsKnownDistanceFromGC(const SGGeoc& a, const SGGeoc&b, const SGGeoc& d, double dist)
{
  double A = SGGeodesy::courseRad(a, d) - SGGeodesy::courseRad(a, b);
  double bDist = SGGeodesy::distanceRad(a, d);
  
  // r=(cos(b)^2+sin(b)^2*cos(A)^2)^(1/2)
  double r = pow(sqr(cos(bDist)) + sqr(sin(bDist)) * sqr(cos(A)), 0.5); 
  
  double p = atan2(sin(bDist)*cos(A), cos(bDist));
  
  if (sqr(cos(dist)) > sqr(r)) {
    SG_LOG(SG_NAVAID, SG_INFO, "pointsKnownDistanceFromGC, no points exist");
    return -1.0;
  }
  
  double dp1 = p + acos(cos(dist)/r);
  double dp2 = p - acos(cos(dist)/r);
  
  double dp1Nm = fabs(dp1 * SG_RAD_TO_NM);
  double dp2Nm = fabs(dp2 * SG_RAD_TO_NM);
  
  return SGMiscd::min(dp1Nm, dp2Nm);
}

// http://williams.best.vwh.net/avform.htm#Int
double latitudeForGCLongitude(const SGGeoc& a, const SGGeoc& b, double lon)
{
#if 0
Intermediate points {lat,lon} lie on the great circle connecting points 1 and 2 when:

lat=atan((sin(lat1)*cos(lat2)*sin(lon-lon2)
     -sin(lat2)*cos(lat1)*sin(lon-lon1))/(cos(lat1)*cos(lat2)*sin(lon1-lon2)))
#endif
  double lonDiff = a.getLongitudeRad() - b.getLongitudeRad();
  double cosLat1 = cos(a.getLatitudeRad()),
      cosLat2 = cos(b.getLatitudeRad());
  double x = sin(a.getLatitudeRad()) * cosLat2 * sin(lon - b.getLongitudeRad());
  double y = sin(b.getLatitudeRad()) * cosLat1 * sin(lon - a.getLongitudeRad());
  double denom = cosLat1 * cosLat2 * sin(lonDiff);
  double lat = atan((x - y) / denom);
  return lat;
}

static double magVarFor(const SGGeod& geod)
{
  double jd = globals->get_time_params()->getJD();
  return sgGetMagVar(geod, jd) * SG_RADIANS_TO_DEGREES;
}

class WayptData
{
public:
  explicit WayptData(WayptRef w) :
    wpt(w),
    hasEntry(false),
    posValid(false),
    legCourseValid(false),
    skipped(false),
    turnAngle(0.0),
    turnRadius(0.0),
    pathDistanceM(0.0),
    turnPathDistanceM(0.0),
    overflightCompensationAngle(0.0),
    flyOver(w->flag(WPT_OVERFLIGHT))
  {
  }
  
  void initPass0()
  {
    const std::string& ty(wpt->type());
    if (wpt->flag(WPT_DYNAMIC)) {
      if ((ty == "hdgToAlt") || (ty == "radialIntercept") || (ty == "dmeIntercept")) {
        legCourse = wpt->headingRadialDeg();
        legCourseValid = true;
      }
      
      // presumtpion is that we always overfly such a waypoint
      if (ty == "hdgToAlt") {
        flyOver = true;
      }
    } else {
      pos = wpt->position();
      posValid = true;
      
      if ((ty == "runway") || (ty == "hold")) {
        legCourse = wpt->headingRadialDeg();
        legCourseValid = true;
      }
      
      if (ty == "runway") {
        FGRunway* rwy = static_cast<RunwayWaypt*>(wpt.get())->runway();
        turnExitPos = rwy->end();
      }
    } // of static waypt
  }

  /**
   * test if course of this leg can be adjusted or is contrained to an exact value
   */
  bool isCourseConstrained() const
  {
    const std::string& ty(wpt->type());
    return (ty == "hdgToAlt") || (ty == "radialIntercept") || (ty == "dmeIntercept");
  }

  // compute leg courses for all static legs (both ends are fixed)
  void initPass1(const WayptData& previous, WayptData* next)
  {
    if (wpt->type() == "vectors") {
      // relying on the fact vectors will be followed by a static fix/wpt
      if (next && next->posValid) {
        posValid = true;
        pos = next->pos;
      }
    }
    
    if (posValid && !legCourseValid && previous.posValid) {
    // check for duplicate points now
      if (previous.wpt->matches(wpt)) {
          skipped = true;
      }

      // we can compute leg course now
      legCourse = SGGeodesy::courseDeg(previous.pos, pos);
      legCourseValid = true;
    }
  }

  void computeTurn(double radiusM, const WayptData& previous, WayptData& next)
  {
    assert(!skipped);
    assert(legCourseValid && next.legCourseValid);
    
    turnAngle = next.legCourse - legCourse;
    SG_NORMALIZE_RANGE(turnAngle, -180.0, 180.0);
    turnRadius = radiusM;
    
    double p = copysign(90.0, turnAngle);

    if (flyOver) {
      turnEntryPos = pos;
      turnCenter = SGGeodesy::direct(pos, legCourse + p, turnRadius);
      // use the leg course
      turnExitPos = SGGeodesy::direct(turnCenter, next.legCourse - p, turnRadius);
      
      if (!next.wpt->flag(WPT_DYNAMIC)) {
        // distance perpendicular to next leg course, after turning
        // through turnAngle
        double xtk = turnRadius * (1 - cos(turnAngle * SG_DEGREES_TO_RADIANS));

        if (next.isCourseConstrained()) {
          // next leg course is constrained. We need to swing back onto the
          // desired course, using a compensation turn
          
          // compensation angle to turn back on course
          double theta = acos((turnRadius - (xtk * 0.5)) / turnRadius) * SG_RADIANS_TO_DEGREES;
          theta = copysign(theta, turnAngle);
          turnAngle += theta;
        
          // move by the distance to compensate
          double d = turnRadius * 2.0 * sin(theta * SG_DEGREES_TO_RADIANS);
          turnExitPos = SGGeodesy::direct(turnExitPos, next.legCourse, d);
          overflightCompensationAngle = -theta;

          turnPathDistanceM = turnRadius * (fabs(turnAngle) +
                                            fabs(overflightCompensationAngle)) * SG_DEGREES_TO_RADIANS;
        } else {
          // next leg course can be adjusted. increase the turn angle
          // and modify the next leg's course accordingly.

          // hypotenuse of triangle, opposite edge has length turnRadius
          double distAlongPath = std::min(1.0, sin(fabs(turnAngle) * SG_DEGREES_TO_RADIANS)) * turnRadius;
          double nextLegDistance = SGGeodesy::distanceM(pos, next.pos) - distAlongPath;
          double increaseAngle = atan2(xtk, nextLegDistance) * SG_RADIANS_TO_DEGREES;
          increaseAngle = copysign(increaseAngle, turnAngle);

          turnAngle += increaseAngle;
          turnExitPos = SGGeodesy::direct(turnCenter, legCourse + turnAngle - p, turnRadius);

          // modify next leg course
          next.legCourse = SGGeodesy::courseDeg(turnExitPos, next.pos);

          turnPathDistanceM = turnRadius * (fabs(turnAngle) * SG_DEGREES_TO_RADIANS);
        }
      }
    } else {
      hasEntry = true;

      double halfAngle = turnAngle * 0.5;
      
      double turnCenterOffset = turnRadius / cos(halfAngle * SG_DEGREES_TO_RADIANS);
      turnCenter = SGGeodesy::direct(pos, legCourse + halfAngle + p, turnCenterOffset);
      
      double distAlongPath = turnRadius * tan(fabs(halfAngle) * SG_DEGREES_TO_RADIANS);
      
      turnEntryPos = SGGeodesy::direct(pos, legCourse, -distAlongPath);
      turnExitPos = SGGeodesy::direct(pos, next.legCourse, distAlongPath);

      turnPathDistanceM = turnRadius * (fabs(halfAngle) * SG_DEGREES_TO_RADIANS);
    }
  }
  
  double turnDistanceM() const
  {
      return turnPathDistanceM;
  }
  
  void turnEntryPath(SGGeodVec& path) const
  {
    assert(!flyOver);
    if (fabs(turnAngle) < 0.5 ) {
      path.push_back(pos);
      return;
    }

    double halfAngle = turnAngle * 0.5;
    int steps = std::max(SGMiscd::roundToInt(fabs(halfAngle) / 3.0), 1);
    double stepIncrement = halfAngle / steps;
    double h = legCourse;
    SGGeod p = turnEntryPos;
    double stepDist = (fabs(stepIncrement) / 360.0) * SGMiscd::twopi() * turnRadius;
    
    for (int s=0; s<steps; ++s) {
      path.push_back(p);
      p = SGGeodesy::direct(p, h, stepDist);
      h += stepIncrement;
    }
    
    path.push_back(p);
  }
  
  void turnExitPath(SGGeodVec& path) const
  {
    if (fabs(turnAngle) < 0.5) {
      path.push_back(turnExitPos);
      return;
    }

    double t = flyOver ? turnAngle : turnAngle * 0.5;
    int steps = std::max(SGMiscd::roundToInt(fabs(t) / 3.0), 1);
    double stepIncrement = t / steps;
    
    // initial exit heading
    double h = legCourse + (flyOver ? 0.0 : (turnAngle * 0.5));
    double turnDirOffset = copysign(90.0, turnAngle);
    
    // compute the first point on the exit path. Depends on fly-over vs fly-by
    SGGeod p = flyOver ? pos : SGGeodesy::direct(turnCenter, h + turnDirOffset, -turnRadius);
    double stepDist = (fabs(stepIncrement) / 360.0) * SGMiscd::twopi() * turnRadius;
    
    for (int s=0; s<steps; ++s) {
      path.push_back(p);
      p = SGGeodesy::direct(p, h, stepDist);
      h += stepIncrement;
    }

    // we can use an exact check on the compensation angle, becayse we
    // initialise it directly. Depending on the next element we might be
    // doing compensation or adjusting the next leg's course; if we did
    // adjust the course ecerything 'just works' above.

    if (flyOver && (overflightCompensationAngle != 0.0)) {
      // skew by compensation angle back
      steps = std::max(SGMiscd::roundToInt(fabs(overflightCompensationAngle) / 3.0), 1);
      
      // step in opposite direction to the turn angle to swing back onto
      // the next leg course
      stepIncrement = overflightCompensationAngle / steps;
      
      for (int s=0; s<steps; ++s) {
        path.push_back(p);
        p = SGGeodesy::direct(p, h, stepDist);
        h += stepIncrement;
      }
    }
    
    path.push_back(p);
  }

  SGGeod pointAlongExitPath(double distanceM) const
  {
      double theta = (distanceM / turnRadius) * SG_RADIANS_TO_DEGREES;
      double p = copysign(90, turnAngle);

      if (flyOver && (overflightCompensationAngle != 0.0)) {
          // figure out if we're in the compensation section
          if (theta > turnAngle) {
              // compute the compensation turn center - twice the turn radius
              // from turnCenter
              SGGeod tc2 = SGGeodesy::direct(turnCenter,
                                             legCourse - overflightCompensationAngle - p,
                                             turnRadius * 2.0);
              theta = copysign(theta - turnAngle, overflightCompensationAngle);
              return SGGeodesy::direct(tc2,
                                       legCourse - overflightCompensationAngle + theta + p, turnRadius);
          }
      }

      theta = copysign(theta, turnAngle);
      double halfAngle = turnAngle * 0.5;
      return SGGeodesy::direct(turnCenter, legCourse - halfAngle + theta - p, turnRadius);
  }

  SGGeod pointAlongEntryPath(double distanceM) const
  {
    assert(hasEntry);
    double theta = (distanceM / turnRadius) * SG_RADIANS_TO_DEGREES;
    theta = copysign(theta, turnAngle);
    double p = copysign(90, turnAngle);
    return SGGeodesy::direct(turnCenter, legCourse + theta - p, turnRadius);
  }
  
  WayptRef wpt;
  bool hasEntry, posValid, legCourseValid, skipped;
  SGGeod pos, turnEntryPos, turnExitPos, turnCenter;
  double turnAngle, turnRadius, legCourse;
  double pathDistanceM;
  double turnPathDistanceM; // for flyBy, this is half the distance; for flyOver it's the completel distance
  double overflightCompensationAngle;
  bool flyOver;
};

typedef std::vector<WayptData> WayptDataVec;

class PerformanceBracket
{
public:
  PerformanceBracket(double atOrBelow, double climb, double descent, double speed, bool isMach = false) :
    atOrBelowAltitudeFt(atOrBelow),
    climbRateFPM(climb),
    descentRateFPM(descent),
    speedIASOrMach(speed),
    speedIsMach(isMach)
  { }
  
  double atOrBelowAltitudeFt;
  double climbRateFPM;
  double descentRateFPM;
  double speedIASOrMach;
  bool speedIsMach;
};

typedef std::vector<PerformanceBracket> PerformanceBracketVec;

bool isDescentWaypoint(const WayptRef& wpt)
{
  return (wpt->flag(WPT_APPROACH) && !wpt->flag(WPT_MISS)) || wpt->flag(WPT_ARRIVAL);
}

class RoutePath::RoutePathPrivate
{
public:
  WayptDataVec waypoints;
  PerformanceBracketVec perf;
  
  
  PerformanceBracketVec::const_iterator
  findPerformanceBracket(double altFt) const
  {
    PerformanceBracketVec::const_iterator r;
    PerformanceBracketVec::const_iterator result = perf.begin();

    for (r = perf.begin(); r != perf.end(); ++r) {
      if (r->atOrBelowAltitudeFt > altFt) {
        break;
      }
      
      result = r;
    } // of brackets iteration
    
    return result;
  }
  
  void computeDynamicPosition(int index)
  {
    const WayptData& previous(previousValidWaypoint(index));
    WayptRef wpt = waypoints[index].wpt;
    
    assert(previous.posValid);
    const std::string& ty(wpt->type());
    if (ty == "hdgToAlt") {
      HeadingToAltitude* h = (HeadingToAltitude*) wpt.get();
      
      double altFt = computeVNAVAltitudeFt(index - 1);
      double altChange = h->altitudeFt() - altFt;
      PerformanceBracketVec::const_iterator it = findPerformanceBracket(altFt);
      double speedMSec = groundSpeedForAltitude(altFt) * SG_KT_TO_MPS;
      double timeToChangeSec;
      
      if (isDescentWaypoint(wpt)) {
        timeToChangeSec = (altChange / it->descentRateFPM) * 60.0;
      } else {
        timeToChangeSec = (altChange / it->climbRateFPM) * 60.0;
      }

      double distanceM = timeToChangeSec * speedMSec;
      double hdg = h->headingDegMagnetic() + magVarFor(previous.pos);
      waypoints[index].pos = SGGeodesy::direct(previous.turnExitPos, hdg, distanceM);
      waypoints[index].posValid = true;
    } else if (ty == "radialIntercept") {
      // start from previous.turnExit
      RadialIntercept* i = (RadialIntercept*) wpt.get();
      
      SGGeoc prevGc = SGGeoc::fromGeod(previous.turnExitPos);
      SGGeoc navid = SGGeoc::fromGeod(wpt->position());
      SGGeoc rGc;
      double magVar = magVarFor(previous.pos);
      
      double radial = i->radialDegMagnetic() + magVar;
      double track = i->courseDegMagnetic() + magVar;
      bool ok = geocRadialIntersection(prevGc, track, navid, radial, rGc);
      if (!ok) {
        SGGeoc navidAdjusted;
        // try pulling backward along the radial in case we're too close.
        // suggests bad procedure construction if this is happening!
        SGGeodesy::advanceRadM(navid, radial, SG_NM_TO_METER * -10, navidAdjusted);

        // try again
        ok = geocRadialIntersection(prevGc, track, navidAdjusted, radial, rGc);
        if (!ok) {
          SG_LOG(SG_NAVAID, SG_WARN, "couldn't compute interception for radial:"
               << previous.turnExitPos << " / " << track << "/" << wpt->position()
               << "/" << radial);
          waypoints[index].pos = wpt->position(); // horrible fallback

        } else {
          waypoints[index].pos = SGGeod::fromGeoc(rGc);
        }
      } else {
        waypoints[index].pos = SGGeod::fromGeoc(rGc);
      }
      
      waypoints[index].posValid = true;
    } else if (ty == "dmeIntercept") {
      DMEIntercept* di = (DMEIntercept*) wpt.get();
      
      SGGeoc prevGc = SGGeoc::fromGeod(previous.turnExitPos);
      SGGeoc navid = SGGeoc::fromGeod(wpt->position());
      double distRad = di->dmeDistanceNm() * SG_NM_TO_RAD;
      SGGeoc rGc;
      
      SGGeoc bPt;
      double crs = di->courseDegMagnetic() + magVarFor(wpt->position());
      SGGeodesy::advanceRadM(prevGc, crs, 100 * SG_NM_TO_RAD, bPt);
      
      double dNm = pointsKnownDistanceFromGC(prevGc, bPt, navid, distRad);
      if (dNm < 0.0) {
        SG_LOG(SG_NAVAID, SG_WARN, "dmeIntercept failed");
        waypoints[index].pos = wpt->position(); // horrible fallback
      } else {
        waypoints[index].pos = SGGeodesy::direct(previous.turnExitPos, crs, dNm * SG_NM_TO_METER);
      }
      
      waypoints[index].posValid = true;
    } else if (ty == "vectors") {
      waypoints[index].legCourse = SGGeodesy::courseDeg(previous.turnExitPos, waypoints[index].pos);
      waypoints[index].legCourseValid = true;
      // no turn data
    }
  }
  
  double computeVNAVAltitudeFt(int index)
  {
    WayptRef w = waypoints[index].wpt;
    if ((w->flag(WPT_APPROACH) && !w->flag(WPT_MISS)) || w->flag(WPT_ARRIVAL)) {
      // descent
      int next = findNextKnownAltitude(index);
      if (next < 0) {
        return 0.0;
      }
      
      double fixedAlt = altitudeForIndex(next);
      double distanceM = distanceBetweenIndices(index, next);
      double speedMSec = groundSpeedForAltitude(fixedAlt) * SG_KT_TO_MPS;
      double minutes = (distanceM / speedMSec) / 60.0;
      
      PerformanceBracketVec::const_iterator it = findPerformanceBracket(fixedAlt);
      return fixedAlt + (it->descentRateFPM * minutes);

    } else {
      // climb
      int prev = findPreceedingKnownAltitude(index);
      if (prev < 0) {
        return 0.0;
      }
      
      double fixedAlt = altitudeForIndex(prev);
      double distanceM = distanceBetweenIndices(prev, index);
      double speedMSec = groundSpeedForAltitude(fixedAlt) * SG_KT_TO_MPS;
      double minutes = (distanceM / speedMSec) / 60.0;
      
      PerformanceBracketVec::const_iterator it = findPerformanceBracket(fixedAlt);
      return fixedAlt + (it->climbRateFPM * minutes);
    }
    
  }
  
  int findPreceedingKnownAltitude(int index) const
  {
    const WayptData& w(waypoints[index]);
    if (w.wpt->altitudeRestriction() == RESTRICT_AT) {
      return index;
    }
    
    // principal base case is runways.
    const std::string& ty(w.wpt->type());
    if (ty == "runway") {
      return index; // runway always has a known elevation
    }
    
    if (index == 0) {
      SG_LOG(SG_NAVAID, SG_WARN, "findPreceedingKnownAltitude: no preceeding altitude value found");
      return -1;
    }
    
    // recurse earlier in the route
    return findPreceedingKnownAltitude(index - 1);
  }
  
  int findNextKnownAltitude(int index) const
  {
    if (index >= waypoints.size()) {
      SG_LOG(SG_NAVAID, SG_WARN, "findNextKnownAltitude: no next altitude value found");
      return -1;
    }
    
    const WayptData& w(waypoints[index]);
    if (w.wpt->altitudeRestriction() == RESTRICT_AT) {
      return index;
    }
    
    // principal base case is runways.
    const std::string& ty(w.wpt->type());
    if (ty == "runway") {
      return index; // runway always has a known elevation
    }
    
    if (index == waypoints.size() - 1) {
      SG_LOG(SG_NAVAID, SG_WARN, "findNextKnownAltitude: no next altitude value found");
      return -1;
    }
    
    return findNextKnownAltitude(index + 1);
  }
  
  double altitudeForIndex(int index) const
  {
    const WayptData& w(waypoints[index]);
    if (w.wpt->altitudeRestriction() != RESTRICT_NONE) {
      return w.wpt->altitudeFt();
    }
    
    const std::string& ty(w.wpt->type());
    if (ty == "runway") {
      FGRunway* rwy = static_cast<RunwayWaypt*>(w.wpt.get())->runway();
      return rwy->threshold().getElevationFt();
    }
    
    SG_LOG(SG_NAVAID, SG_WARN, "altitudeForIndex: waypoint has no explicit altitude");
    return 0.0;
  }
  
  double groundSpeedForAltitude(double altitude) const
  {
    // FIXME
#if 0
    if (0) {
      PerformanceBracketVec::const_iterator it = findPerformanceBracket(altitude);
      double mach;
      
      if (it->speedIsMach) {
        mach = it->speedIASOrMach; // easy
      } else {
        const double Cs_0 = 661.4786; // speed of sound at sea level, knots
        const double P_0 = 29.92126;
        const double P = P_0 * pow(, );
        // convert IAS (which we will treat as CAS) to Mach based on altitude
      }
      
      double oatK;
      double Cs = sqrt(SG_gamma * SG_R_m2_p_s2_p_K * oatK);
      
      double tas = mach * Cs;
      
#if 0
      P_0= 29.92126 "Hg = 1013.25 mB = 2116.2166 lbs/ft^2
      P= P_0*(1-6.8755856*10^-6*PA)^5.2558797, pressure altitude, PA<36,089.24ft
      CS= 38.967854*sqrt(T+273.15)  where T is the (static/true) OAT in Celsius.
      
      DP=P_0*((1 + 0.2*(IAS/CS_0)^2)^3.5 -1)
      M=(5*( (DP/P + 1)^(2/7) -1) )^0.5   (*)
      TAS= M*CS
#endif
    }
#endif
    
    return 250.0;
  }

  double distanceBetweenIndices(int from, int to) const
  {
    double total = 0.0;
    
    for (int i=from+1; i<= to; ++i) {
      total += waypoints[i].pathDistanceM;
    }
    
    return total;
  }
  
  void initPerfData()
  {
    // assume category C/D aircraft for now
    perf.push_back(PerformanceBracket(4000, 1800, 1800, 180));
    perf.push_back(PerformanceBracket(10000, 1800, 1800, 230));
    perf.push_back(PerformanceBracket(18000, 1200, 1800, 270));
    perf.push_back(PerformanceBracket(60000, 800, 1200, 0.85, true /* is Mach */));
  }

    const WayptData& previousValidWaypoint(int index) const
    {
        assert(index > 0);
        if (waypoints[index-1].skipped) {
            return waypoints[index-2];
        }

        return waypoints[index-1];
    }

}; // of RoutePathPrivate class

RoutePath::RoutePath(const flightgear::WayptVec& wpts) :
  d(new RoutePathPrivate)
{
  WayptVec::const_iterator it;
  for (it = wpts.begin(); it != wpts.end(); ++it) {
    d->waypoints.push_back(WayptData(*it));
  }
  commonInit();
}

RoutePath::RoutePath(const flightgear::FlightPlan* fp) :
  d(new RoutePathPrivate)
{
  for (int l=0; l<fp->numLegs(); ++l) {
     d->waypoints.push_back(WayptData(fp->legAtIndex(l)->waypoint()));
  }
  commonInit();
}

void RoutePath::commonInit()
{
  _pathTurnRate = 3.0; // 3 deg/sec = 180deg/min = standard rate turn
 
  d->initPerfData();
  
  WayptDataVec::iterator it;
  for (it = d->waypoints.begin(); it != d->waypoints.end(); ++it) {
    it->initPass0();
  }
  
  for (unsigned int i=1; i<d->waypoints.size(); ++i) {
    WayptData* nextPtr = ((i + 1) < d->waypoints.size()) ? &d->waypoints[i+1] : 0;
    d->waypoints[i].initPass1(d->waypoints[i-1], nextPtr);
  }
  
  for (unsigned int i=1; i<d->waypoints.size(); ++i) {
      if (d->waypoints[i].skipped) {
          continue;
      }

    d->computeDynamicPosition(i);
      const WayptData& prev(d->previousValidWaypoint(i));


    double alt = 0.0; // FIXME
    double gs = d->groundSpeedForAltitude(alt);
    double radiusM = ((360.0 / _pathTurnRate) * gs * SG_KT_TO_MPS) / SGMiscd::twopi();
    
    if (i < (d->waypoints.size() - 1)) {
        int nextIndex = i + 1;
        if (d->waypoints[nextIndex].skipped) {
            nextIndex++;
        }
        WayptData& next(d->waypoints[nextIndex]);

      if (!next.legCourseValid && next.posValid) {
        // compute leg course now our own position is valid
        next.legCourse = SGGeodesy::courseDeg(d->waypoints[i].pos, next.pos);
        next.legCourseValid = true;
      }

      if (next.legCourseValid) {
        d->waypoints[i].computeTurn(radiusM, prev, next);
      } else {
        // next waypoint has indeterminate course. Let's create a sharp turn
        // this can happen when the following point is ATC vectors, for example.
        d->waypoints[i].turnEntryPos = d->waypoints[i].pos;
        d->waypoints[i].turnExitPos = d->waypoints[i].pos;
      }
    } else {
      // final waypt, fix up some data
      d->waypoints[i].turnEntryPos = d->waypoints[i].pos;
      d->waypoints[i].turnExitPos = d->waypoints[i].pos;
    }
    
    // now turn is computed, can resolve distances
    d->waypoints[i].pathDistanceM = computeDistanceForIndex(i);
  }
}

SGGeodVec RoutePath::pathForIndex(int index) const
{
  if (index == 0) {
    return SGGeodVec(); // no path for first waypoint
  }

    if (d->waypoints[index].skipped) {
        return SGGeodVec();
    }

  const WayptData& w(d->waypoints[index]);
  const std::string& ty(w.wpt->type());
  if (ty == "vectors") {
    return SGGeodVec(); // empty
  }
  
  if (ty== "hold") {
    return pathForHold((Hold*) d->waypoints[index].wpt.get());
  }
  
  SGGeodVec r;
  const WayptData& prev(d->previousValidWaypoint(index));
  prev.turnExitPath(r);
  
  SGGeod from = prev.turnExitPos,
    to = w.turnEntryPos;
  
  // compute rounding offset, we want to round towards the direction of travel
  // which depends on the east/west sign of the longitude change
  double lonDelta = to.getLongitudeDeg() - from.getLongitudeDeg();
  if (fabs(lonDelta) > 0.5) {
    interpolateGreatCircle(from, to, r);
  }
  
  if (w.flyOver) {
    r.push_back(w.pos);
  } else {
    // flyBy
    w.turnEntryPath(r);
  }
  
  if (ty == "runway") {
    // runways get an extra point, at the end. this is particularly
    // important so missed approach segments draw correctly
    FGRunway* rwy = static_cast<RunwayWaypt*>(w.wpt.get())->runway();
    r.push_back(rwy->end());
  }
  
  return r;
}

void RoutePath::interpolateGreatCircle(const SGGeod& aFrom, const SGGeod& aTo, SGGeodVec& r) const
{
  SGGeoc gcFrom = SGGeoc::fromGeod(aFrom),
    gcTo = SGGeoc::fromGeod(aTo);
  
  double lonDelta = gcTo.getLongitudeRad() - gcFrom.getLongitudeRad();
  if (fabs(lonDelta) < 1e-3) {
    return;
  }
  
  lonDelta = SGMiscd::normalizeAngle(lonDelta);    
  int steps = static_cast<int>(fabs(lonDelta) * SG_RADIANS_TO_DEGREES * 2);
  double lonStep = (lonDelta / steps);
  
  double lon = gcFrom.getLongitudeRad() + lonStep;
  for (int s=0; s < (steps - 1); ++s) {
    lon = SGMiscd::normalizeAngle(lon);
    double lat = latitudeForGCLongitude(gcFrom, gcTo, lon);
    r.push_back(SGGeod::fromGeoc(SGGeoc::fromRadM(lon, lat, SGGeodesy::EQURAD)));
    //SG_LOG(SG_GENERAL, SG_INFO, "lon:" << lon * SG_RADIANS_TO_DEGREES << " gives lat " << lat * SG_RADIANS_TO_DEGREES);
    lon += lonStep;
  }
}

SGGeod RoutePath::positionForIndex(int index) const
{
  return d->waypoints[index].pos;
}

SGGeodVec RoutePath::pathForHold(Hold* hold) const
{
  int turnSteps = 16;
  double hdg = hold->inboundRadial();
  double turnDelta = 180.0 / turnSteps;
  double altFt = 0.0; // FIXME
  double gsKts = d->groundSpeedForAltitude(altFt);
  
  SGGeodVec r;
  double az2;
  double stepTime = turnDelta / _pathTurnRate; // in seconds
  double stepDist = gsKts * (stepTime / 3600.0) * SG_NM_TO_METER;
  double legDist = hold->isDistance() ? 
    hold->timeOrDistance() 
    : gsKts * (hold->timeOrDistance() / 3600.0);
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

double RoutePath::computeDistanceForIndex(int index) const
{
  if ((index < 0) || (index >= (int) d->waypoints.size())) {
    throw sg_range_exception("waypt index out of range",
                             "RoutePath::computeDistanceForIndex");
  }
  
  if (index == 0) {
    // first waypoint, distance is 0
    return 0.0;
  }

    if (d->waypoints[index].skipped) {
        return 0.0;
    }

    const WayptData& prev(d->previousValidWaypoint(index));
  double dist = SGGeodesy::distanceM(prev.turnExitPos,
                              d->waypoints[index].turnEntryPos);
  dist += prev.turnDistanceM();
  
  if (!d->waypoints[index].flyOver) {
    // add entry distance
    dist += d->waypoints[index].turnDistanceM();
  }
  
  return dist;
}

double RoutePath::trackForIndex(int index) const
{
    if (d->waypoints[index].skipped)
        return trackForIndex(index - 1);
  return d->waypoints[index].legCourse;
}

double RoutePath::distanceForIndex(int index) const
{
  return d->waypoints[index].pathDistanceM;
}

double RoutePath::distanceBetweenIndices(int from, int to) const
{
  return d->distanceBetweenIndices(from, to);
}

SGGeod RoutePath::positionForDistanceFrom(int index, double distanceM) const
{
    int sz = (int) d->waypoints.size();
    if ((index < 0) || (index >= sz)) {
        throw sg_range_exception("waypt index out of range",
                                 "RoutePath::positionForDistanceFrom");
    }

    // find the actual leg we're within
    while ((index < sz) && (d->waypoints[index].pathDistanceM < distanceM)) {
        ++index;
        distanceM -= d->waypoints[index].pathDistanceM;
    }

    if (index >= sz) {
        // past route end, just return final position
        return d->waypoints[sz - 1].pos;
    } else if (index == 0) {
        return d->waypoints[0].pos;
    }

    const WayptData& wpt(d->waypoints[index]);
    const WayptData& prev(d->previousValidWaypoint(index));
    // check turn exit of previous
    if (prev.turnPathDistanceM > distanceM) {
        // on the exit path of previous wpt
        return prev.pointAlongExitPath(distanceM);
    }

    double corePathDistance = wpt.pathDistanceM - wpt.turnPathDistanceM;
    if (wpt.hasEntry && (distanceM > corePathDistance)) {
        // on the entry path
        return wpt.pointAlongEntryPath(distanceM - corePathDistance);
    }

    // linear between turn exit and turn entry points
    SGGeod previousTurnExit = prev.turnExitPos;
    distanceM -= prev.turnPathDistanceM;
    return SGGeodesy::direct(previousTurnExit, wpt.legCourse, distanceM);
}
