// routePath.hxx - compute data about planned route
//
// Copyright (C) 2018  James Turner  <james@flightgear.org>
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

#include "config.h"

#include <algorithm>

#include <Navaids/routePath.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>

#include <Main/globals.hxx>
#include <Airports/runways.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/FlightPlan.hxx>
#include <Navaids/positioned.hxx>
#include <Aircraft/AircraftPerformance.hxx>

namespace flightgear {

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

SGGeod turnCenterOverflight(const SGGeod& pt, double inHeadingDeg,
                           double turnAngleDeg, double turnRadiusM)
{
    double p = copysign(90.0, turnAngleDeg);
    return SGGeodesy::direct(pt, inHeadingDeg + p, turnRadiusM);
}

SGGeod turnCenterFlyBy(const SGGeod& pt, double inHeadingDeg,
                           double turnAngleDeg, double turnRadiusM)
{
    double halfAngle = turnAngleDeg * 0.5;
    double turnCenterOffset = turnRadiusM / cos(halfAngle * SG_DEGREES_TO_RADIANS);
    double p = copysign(90.0, turnAngleDeg);
    return SGGeodesy::direct(pt, inHeadingDeg + halfAngle + p, turnCenterOffset);
}

SGGeod turnCenterFromExit(const SGGeod& pt, double outHeadingDeg,
                       double turnAngleDeg, double turnRadiusM)
{
    double p = copysign(90.0, turnAngleDeg);
    return SGGeodesy::direct(pt, outHeadingDeg + p, turnRadiusM);
}

struct TurnInfo
{
    SGGeod turnCenter;
    double inboundCourseDeg = 0.0;
    double turnAngleDeg = 0.0;
    bool valid = false;
};

/**
 * given a turn exit position and heading, and an arbitrary origin position,
 * compute the turn center / angle that matches. Certain configurations may
 * fail, especially if the origin is less than two turn radii from the exit pos.
 */
TurnInfo turnCenterAndAngleFromExit(const SGGeod& pt, double outHeadingDeg,
                                double turnRadiusM, const SGGeod&origin)
{
    double bearingToExit = SGGeodesy::courseDeg(origin, pt);
    // not the final turn angle, but we need to know which side of the
    // exit course we're on, to decide if it's a left-hand or right-hand turn
    double turnAngle = outHeadingDeg - bearingToExit;
    SG_NORMALIZE_RANGE(turnAngle, -180.0, 180.0);
    double p = copysign(90.0, turnAngle);

    TurnInfo r;
    r.turnCenter = SGGeodesy::direct(pt, outHeadingDeg + p, turnRadiusM);

    double courseToTC, distanceToTC, az2;
    SGGeodesy::inverse(origin, r.turnCenter, courseToTC, az2, distanceToTC);
    if (distanceToTC < turnRadiusM) {
        SG_LOG(SG_NAVAID, SG_WARN, "turnCenterAndAngleFromExit: origin point too close to turn center");
        return r;
    }

    // find additional course angle away from the exit pos to intersect
    // the turn circle.
    double theta = asin(turnRadiusM / distanceToTC) * SG_RADIANS_TO_DEGREES;
    // invert angle sign so we increase the turn angle
    theta = -copysign(theta, turnAngle);

    r.inboundCourseDeg = courseToTC + theta;
    SG_NORMALIZE_RANGE(r.inboundCourseDeg, 0.0, 360.0);

    // turn angle must have same direction (sign) as turnAngle above, even if
    // the turn radius means the sign would cross over (happens if origin point
    // is close by
    r.turnAngleDeg = outHeadingDeg - r.inboundCourseDeg;
    if (r.turnAngleDeg > 0.0) {
        if (turnAngle < 0.0) r.turnAngleDeg -= 360.0;
    } else {
        if (turnAngle > 0.0) r.turnAngleDeg += 360.0;
    }

    r.valid = true;
    return r;
}

class WayptData;
using WayptDataVec = std::vector<WayptData>;
using WpDataIt =  WayptDataVec::iterator;

class WayptData
{
public:
  explicit WayptData(WayptRef w) :
    wpt(w),
    hasEntry(false),
    posValid(false),
    legCourseValid(false),
    skipped(false),
    turnEntryAngle(0.0),
    turnExitAngle(0.0),
    turnRadius(0.0),
    legCourseTrue(0.0),
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
      // presumption is that we always overfly such a waypoint
      if (ty == "hdgToAlt") {
        flyOver = true;
      }
    } else if (ty == "discontinuity") {

    } else {
      pos = wpt->position();
      posValid = true;
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
  void initPass1(const WayptData* previous, WayptData* next)
  {
    const auto ty = wpt->type();
    if ((ty == "vectors") || (ty == "discontinuity")) {
      // relying on the fact vectors will be followed by a static fix/wpt
      if (next && next->posValid) {
        posValid = true;
        pos = next->pos;
      }
    }

    const bool previousPosValid = previous && previous->posValid;
    if (ty == "via") {
        // even though both ends may be known, we don't
        // want to compute a leg course for a VIA
    } else if (posValid && !legCourseValid && previousPosValid) {
      const auto prevWP = previous->wpt;
        // check for duplicate points now
      if (prevWP->matches(wpt)) {
          skipped = true;
      }

      // we can compute leg course now
        if (prevWP->type() == "runway") {
            // use the runway departure end pos
            FGRunway* rwy = static_cast<RunwayWaypt*>(prevWP.get())->runway();
            legCourseTrue = SGGeodesy::courseDeg(rwy->end(), pos);
        } else if (ty == "discontinuity") {
            // we want to keep the path across DISCONs sharp (no turn entry/exit)
            // so we keep the leg course invalid, even though we actually
            // could compute it
        } else if (ty != "runway") {
            // need to wait to compute runway leg course
            legCourseTrue = SGGeodesy::courseDeg(previous->pos, pos);
            legCourseValid = true;
        }
    } // of not a VIA
  }

  void computeLegCourse(const WayptData* previous, double radiusM)
  {
    if (legCourseValid) {
      return;
    }
    
    if ((wpt->type() == "via") || (wpt->type() == "discontinuty"))
    {
      // do nothing, we can't compute a valid leg course for these types
      // we'll generate sharp turns in the path but that's no problem.
      return;
    }
    
    if (!previous || !previous->posValid) {
      // all the cases below assume previous pas exists and has a valid position
      SG_LOG(SG_NAVAID, SG_WARN, "RoutePath: Asked to compute leg course, but, previous is invalid");
      return;
    }
    
    if (wpt->type() == "runway") {
      FGRunway* rwy = static_cast<RunwayWaypt*>(wpt.get())->runway();
      flyOver = true;
      
      TurnInfo ti = turnCenterAndAngleFromExit(rwy->threshold(),
                                               rwy->headingDeg(),
                                               radiusM, previous->pos);
      if (ti.valid) {
        legCourseTrue = ti.inboundCourseDeg;
        turnEntryAngle = ti.turnAngleDeg;
        turnEntryCenter = ti.turnCenter;
        turnRadius = radiusM;
        hasEntry = true;
        turnEntryPos = pointOnEntryTurnFromHeading(ti.inboundCourseDeg);
      } else {
        // couldn't compute entry, never mind
        legCourseTrue = SGGeodesy::courseDeg(previous->pos, rwy->threshold());
      }
      legCourseValid = true;
      return;
    }
    
    if (posValid) {
      legCourseTrue = SGGeodesy::courseDeg(previous->pos, pos);
      legCourseValid = true;
    } else if (isCourseConstrained()) {
      double magVar = magVarFor(previous->pos);
      legCourseTrue = wpt->headingRadialDeg() + magVar;
      legCourseValid = true;
    } // of pos not valid
  }

    SGGeod pointOnEntryTurnFromHeading(double headingDeg) const
    {
        assert(hasEntry);
        double p = copysign(90.0, turnEntryAngle);
        return SGGeodesy::direct(turnEntryCenter, headingDeg - p, turnRadius);
    }

    SGGeod pointOnExitTurnFromHeading(double headingDeg) const
    {
        double p = copysign(90.0, turnExitAngle);
        return SGGeodesy::direct(turnExitCenter, headingDeg - p, turnRadius);
    }

    double pathDistanceForTurnAngle(double angleDeg) const
    {
        return turnRadius * fabs(angleDeg) * SG_DEGREES_TO_RADIANS;
    }

  void computeTurn(double radiusM, bool constrainLegCourse, WayptData& next)
  {
    assert(!skipped);
    assert(next.legCourseValid);
      bool isRunway = (wpt->type() == "runway");

      if (legCourseValid) {
          if (isRunway) {
              FGRunway* rwy = static_cast<RunwayWaypt*>(wpt.get())->runway();
              turnExitAngle = next.legCourseTrue - rwy->headingDeg();
          } else {
              turnExitAngle = next.legCourseTrue - legCourseTrue;
          }
      } else {
          // happens for first leg
          if (isRunway) {
              legCourseValid = true;
              FGRunway* rwy = static_cast<RunwayWaypt*>(wpt.get())->runway();
              turnExitAngle = next.legCourseTrue - rwy->headingDeg();
              legCourseTrue = rwy->headingDeg();
              flyOver = true;
          } else {
              legCourseValid = true;
              legCourseTrue = next.legCourseTrue;
              turnExitAngle = 0.0;
              turnExitPos = pos;
              flyOver = true;
              return;
          }
      }

    SG_NORMALIZE_RANGE(turnExitAngle, -180.0, 180.0);
    turnRadius = radiusM;

    if (!flyOver && fabs(turnExitAngle) > 120.0) {
        // flyBy logic blows up for sharp turns - due to the tan() term
        // heading towards infinity. By converting to flyOver we do something
        // closer to what was requested.
        flyOver = true;
    }

    if (flyOver) {
        if (isRunway) {
            FGRunway* rwy = static_cast<RunwayWaypt*>(wpt.get())->runway();
            turnExitCenter = turnCenterOverflight(rwy->end(), rwy->headingDeg(), turnExitAngle, turnRadius);
        } else {
            turnEntryPos = pos;
            turnExitCenter = turnCenterOverflight(pos, legCourseTrue, turnExitAngle, turnRadius);

        }
      turnExitPos = pointOnExitTurnFromHeading(next.legCourseTrue);
      
      if (!next.wpt->flag(WPT_DYNAMIC)) {
        // distance perpendicular to next leg course, after turning
        // through turnAngle
        double xtk = turnRadius * (1 - cos(turnExitAngle * SG_DEGREES_TO_RADIANS));

        if (constrainLegCourse || next.isCourseConstrained()) {
          // next leg course is constrained. We need to swing back onto the
          // desired course, using a compensation turn
          
          // compensation angle to turn back on course
          double theta = acos((turnRadius - (xtk * 0.5)) / turnRadius) * SG_RADIANS_TO_DEGREES;
          theta = copysign(theta, turnExitAngle);
          turnExitAngle += theta;
        
          // move by the distance to compensate
          double d = turnRadius * 2.0 * sin(theta * SG_DEGREES_TO_RADIANS);
          turnExitPos = SGGeodesy::direct(turnExitPos, next.legCourseTrue, d);
          overflightCompensationAngle = -theta;

            // sign of angles will differ, so compute distances seperately
            turnPathDistanceM = pathDistanceForTurnAngle(turnExitAngle) +
                pathDistanceForTurnAngle(overflightCompensationAngle);
        } else {
          // next leg course can be adjusted. increase the turn angle
          // and modify the next leg's course accordingly.

          // hypotenuse of triangle, opposite edge has length turnRadius
          double distAlongPath = std::min(1.0, sin(fabs(turnExitAngle) * SG_DEGREES_TO_RADIANS)) * turnRadius;
          double nextLegDistance = SGGeodesy::distanceM(pos, next.pos) - distAlongPath;
          double increaseAngle = atan2(xtk, nextLegDistance) * SG_RADIANS_TO_DEGREES;
          increaseAngle = copysign(increaseAngle, turnExitAngle);

          turnExitAngle += increaseAngle;
          turnExitPos = pointOnExitTurnFromHeading(legCourseTrue + turnExitAngle);
          // modify next leg course
          next.legCourseTrue = SGGeodesy::courseDeg(turnExitPos, next.pos);
          turnPathDistanceM = pathDistanceForTurnAngle(turnExitAngle);
        } // of next leg isn't course constrained
      } else {
          // next point is dynamic
          // no compensation needed
          turnPathDistanceM = pathDistanceForTurnAngle(turnExitAngle);
      }
    } else {
      hasEntry = true;
        turnEntryCenter = turnCenterFlyBy(pos, legCourseTrue, turnExitAngle, turnRadius);

        turnExitAngle = turnExitAngle * 0.5;
        turnEntryAngle = turnExitAngle;
        turnExitCenter = turnEntryCenter; // important that these match

      turnEntryPos = pointOnEntryTurnFromHeading(legCourseTrue);
      turnExitPos = pointOnExitTurnFromHeading(next.legCourseTrue);
      turnPathDistanceM = pathDistanceForTurnAngle(turnEntryAngle);
    }
  }
  
  double turnDistanceM() const
  {
      return turnPathDistanceM;
  }
  
  void turnEntryPath(SGGeodVec& path) const
  {
    if (!hasEntry || fabs(turnEntryAngle) < 0.5 ) {
      path.push_back(pos);
      return;
    }

    int steps = std::max(SGMiscd::roundToInt(fabs(turnEntryAngle) / 3.0), 1);
    double stepIncrement = turnEntryAngle / steps;
    double h = legCourseTrue;
    for (int s=0; s<steps; ++s) {
        path.push_back(pointOnEntryTurnFromHeading(h));
        h += stepIncrement;
    }
  }
  
  void turnExitPath(SGGeodVec& path) const
  {
    if (fabs(turnExitAngle) < 0.5) {
      path.push_back(turnExitPos);
      return;
    }

    int steps = std::max(SGMiscd::roundToInt(fabs(turnExitAngle) / 3.0), 1);
    double stepIncrement = turnExitAngle / steps;
    
    // initial exit heading
      double h = legCourseTrue + (flyOver ? 0.0 : turnEntryAngle);
      if (wpt->type() == "runway") {
          FGRunway* rwy = static_cast<RunwayWaypt*>(wpt.get())->runway();
          h = rwy->headingDeg();
      }

    for (int s=0; s<steps; ++s) {
        path.push_back(pointOnExitTurnFromHeading(h));
        h += stepIncrement;
    }

    // we can use an exact check on the compensation angle, because we
    // initialise it directly. Depending on the next element we might be
    // doing compensation or adjusting the next leg's course; if we did
    // adjust the course everything 'just works' above.

    if (flyOver && (overflightCompensationAngle != 0.0)) {
      // skew by compensation angle back
      steps = std::max(SGMiscd::roundToInt(fabs(overflightCompensationAngle) / 3.0), 1);
      
      // step in opposite direction to the turn angle to swing back onto
      // the next leg course
      stepIncrement = overflightCompensationAngle / steps;

        SGGeod p = path.back();
        double stepDist = (fabs(stepIncrement) / 360.0) * SGMiscd::twopi() * turnRadius;

      for (int s=0; s<steps; ++s) {
          h += stepIncrement;
        p = SGGeodesy::direct(p, h, stepDist);
          path.push_back(p);

      }
    } // of overflight compensation turn
  }

  SGGeod pointAlongExitPath(double distanceM) const
  {
      double theta = (distanceM / turnRadius) * SG_RADIANS_TO_DEGREES;
      double p = copysign(90, turnExitAngle);

      if (flyOver && (overflightCompensationAngle != 0.0)) {
          // figure out if we're in the compensation section
          if (theta > turnExitAngle) {
              // compute the compensation turn center - twice the turn radius
              // from turnCenter
              SGGeod tc2 = SGGeodesy::direct(turnExitCenter,
                                             legCourseTrue - overflightCompensationAngle - p,
                                             turnRadius * 2.0);
              theta = copysign(theta - turnExitAngle, overflightCompensationAngle);
              return SGGeodesy::direct(tc2,
                                       legCourseTrue - overflightCompensationAngle + theta + p, turnRadius);
          }
      }

      theta = copysign(theta, turnExitAngle);
      double inboundCourse = legCourseTrue + (flyOver ? 0.0 : turnExitAngle);
      return pointOnExitTurnFromHeading(inboundCourse + theta);
  }

  SGGeod pointAlongEntryPath(double distanceM) const
  {
    assert(hasEntry);
    double theta = (distanceM / turnRadius) * SG_RADIANS_TO_DEGREES;
      theta = copysign(theta, turnEntryAngle);
      return pointOnEntryTurnFromHeading(legCourseTrue + theta);
  }
  
  WayptRef wpt;
  bool hasEntry, posValid, legCourseValid, skipped;
  SGGeod pos, turnEntryPos, turnExitPos, turnEntryCenter, turnExitCenter;
  double turnEntryAngle, turnExitAngle, turnRadius, legCourseTrue;
  double pathDistanceM;
  double turnPathDistanceM; // for flyBy, this is half the distance; for flyOver it's the complete distance
  double overflightCompensationAngle;
  bool flyOver;
};

bool isDescentWaypoint(const WayptRef& wpt)
{
  return (wpt->flag(WPT_APPROACH) && !wpt->flag(WPT_MISS)) || wpt->flag(WPT_ARRIVAL);
}

class RoutePath::RoutePathPrivate
{
public:
    WayptDataVec waypoints;

    AircraftPerformance perf;
    bool constrainLegCourses;

  void computeDynamicPosition(int index)
  {
    auto previous(previousValidWaypoint(index));
    if ((previous == waypoints.end()) || !previous->posValid)
    {
      SG_LOG(SG_NAVAID, SG_WARN, "couldn't compute position for dynamic waypoint: no preceeding valid waypoint");
      return;
    }
    
    WayptRef wpt = waypoints[index].wpt;

    const std::string& ty(wpt->type());
    if (ty == "hdgToAlt") {
      HeadingToAltitude* h = (HeadingToAltitude*) wpt.get();
      
      double altFt = computeVNAVAltitudeFt(index - 1);
      double distanceM = perf.distanceNmBetween(altFt, h->altitudeFt()) * SG_NM_TO_METER;
      double hdg = h->headingDegMagnetic() + magVarFor(previous->pos);
      waypoints[index].pos = SGGeodesy::direct(previous->turnExitPos, hdg, distanceM);
      waypoints[index].posValid = true;
    } else if (ty == "radialIntercept") {
      // start from previous.turnExit
      RadialIntercept* i = (RadialIntercept*) wpt.get();
      
      SGGeoc prevGc = SGGeoc::fromGeod(previous->turnExitPos);
      SGGeoc navid = SGGeoc::fromGeod(wpt->position());
      SGGeoc rGc;
      double magVar = magVarFor(previous->pos);
      
      double radial = i->radialDegMagnetic() + magVar;
      double track = i->courseDegMagnetic() + magVar;
      bool ok = geocRadialIntersection(prevGc, track, navid, radial, rGc);
      if (!ok) {
        // try pulling backward along the radial in case we're too close.
        // suggests bad procedure construction if this is happening!
          SGGeoc navidAdjusted = SGGeodesy::advanceDegM(navid, radial, -10 * SG_NM_TO_METER);

        // try again
        ok = geocRadialIntersection(prevGc, track, navidAdjusted, radial, rGc);
        if (!ok) {
          SG_LOG(SG_NAVAID, SG_WARN, "couldn't compute interception for radial:"
               << previous->turnExitPos << " / " << track << "/" << wpt->position()
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
      
      SGGeoc prevGc = SGGeoc::fromGeod(previous->turnExitPos);
      SGGeoc navid = SGGeoc::fromGeod(wpt->position());
      double distRad = di->dmeDistanceNm() * SG_NM_TO_RAD;
      SGGeoc rGc;
      
      double crs = di->courseDegMagnetic() + magVarFor(wpt->position());
      SGGeoc bPt = SGGeodesy::advanceDegM(prevGc, crs, 1e5);
      
      double dNm = pointsKnownDistanceFromGC(prevGc, bPt, navid, distRad);
      if (dNm < 0.0) {
        SG_LOG(SG_NAVAID, SG_WARN, "dmeIntercept failed");
        waypoints[index].pos = wpt->position(); // horrible fallback
      } else {
        waypoints[index].pos = SGGeodesy::direct(previous->turnExitPos, crs, dNm * SG_NM_TO_METER);
      }
      
      waypoints[index].posValid = true;
    } else if (ty == "vectors") {
      waypoints[index].legCourseTrue = SGGeodesy::courseDeg(previous->turnExitPos, waypoints[index].pos);
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
      return perf.computePreviousAltitude(distanceM, fixedAlt);
    } else {
      // climb
      int prev = findPreceedingKnownAltitude(index);
      if (prev < 0) {
        return 0.0;
      }
      
      double fixedAlt = altitudeForIndex(prev);
      double distanceM = distanceBetweenIndices(prev, index);
      return perf.computeNextAltitude(distanceM, fixedAlt);
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
      SG_LOG(SG_NAVAID, SG_WARN, "findPreceedingKnownAltitude: no preceding altitude value found");
      return -1;
    }
    
    // recurse earlier in the route
    return findPreceedingKnownAltitude(index - 1);
  }
  
  int findNextKnownAltitude(unsigned int index) const
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
  
  double distanceBetweenIndices(int from, int to) const
  {
    double total = 0.0;
    
    for (int i=from+1; i<= to; ++i) {
      total += waypoints[i].pathDistanceM;
    }
    
    return total;
  }
  
    WayptDataVec::iterator previousValidWaypoint(unsigned int index)
    {
        do {
            if (index == 0) {
                return waypoints.end();
            }
            
            --index;
        } while (waypoints.at(index).skipped || (waypoints.at(index).wpt->type() == "discontinuity"));

        return waypoints.begin() + index;
    }

    WayptDataVec::iterator previousValidWaypoint(WayptDataVec::iterator it)
    {
        return previousValidWaypoint(std::distance(waypoints.begin(), it));
    }


    WayptDataVec::iterator nextValidWaypoint(int index)
    {
        return nextValidWaypoint(waypoints.begin() + index);
    }

    WayptDataVec::iterator nextValidWaypoint(WayptDataVec::iterator it)
    {
        if (it == waypoints.end()) {
            return it;
        }

        ++it;
        while ((it != waypoints.end()) && (it->skipped || (it->wpt->type() == "discontinuity"))) {
            ++it;
        }
        return it;
    }
}; // of RoutePathPrivate class

RoutePath::RoutePath(const flightgear::FlightPlan* fp) :
  d(new RoutePathPrivate)
{
    for (int l=0; l<fp->numLegs(); ++l) {
        WayptRef wpt = fp->legAtIndex(l)->waypoint();
        if (!wpt) {
            SG_LOG(SG_NAVAID, SG_DEV_ALERT, "Waypoint " << l << " of " << fp->numLegs() << "is NULL");
            break;
        }
        d->waypoints.push_back(WayptData(wpt));
    }

    d->constrainLegCourses = fp->followLegTrackToFixes();
    commonInit();
}


RoutePath::RoutePath(const RoutePath& other)
{
    d.reset(new RoutePathPrivate(*other.d));
}

RoutePath& RoutePath::operator=(const RoutePath& other)
{
    d.reset(new RoutePathPrivate(*other.d));
    return *this;
}

RoutePath::~RoutePath()
{
}

void RoutePath::commonInit()
{
  for (auto& w : d->waypoints) {
    w.initPass0();
  }
  
  for (unsigned int i=1; i<d->waypoints.size(); ++i) {
    WayptData* nextPtr = ((i + 1) < d->waypoints.size()) ? &d->waypoints[i+1] : nullptr;
    auto prev = d->previousValidWaypoint(i);
    WayptData* prevPtr = (prev == d->waypoints.end()) ? nullptr : &(*prev);
    d->waypoints[i].initPass1(prevPtr, nextPtr);
  }

  for (unsigned int i=0; i<d->waypoints.size(); ++i) {
      if (d->waypoints[i].skipped) {
          continue;
      }

      double alt = 0.0; // FIXME
      double radiusM = d->perf.turnRadiusMForAltitude(alt);

      if (i > 0) {
          auto prevIt = d->previousValidWaypoint(i);
          WayptData* prevPtr = (prevIt == d->waypoints.end()) ? nullptr : &(*prevIt);

          d->waypoints[i].computeLegCourse(prevPtr, radiusM);
          d->computeDynamicPosition(i);
      }

      auto nextIt = d->nextValidWaypoint(i);
      if (nextIt != d->waypoints.end()) {
          nextIt->computeLegCourse(&(d->waypoints[i]), radiusM);

          if (nextIt->legCourseValid) {
              d->waypoints[i].computeTurn(radiusM, d->constrainLegCourses, *nextIt);
          } else {
            // next waypoint has indeterminate course. Let's create a sharp turn
            // this can happen when the following point is ATC vectors, for example.
            d->waypoints[i].turnEntryPos = d->waypoints[i].pos;
            d->waypoints[i].turnExitPos = d->waypoints[i].pos;
          }
    } else {
      // final waypt, fix up some data
      d->waypoints[i].turnExitPos = d->waypoints[i].pos;
      d->waypoints[i].turnEntryPos = d->waypoints[i].pos;
    }
    
    // now turn is computed, can resolve distances
    d->waypoints[i].pathDistanceM = computeDistanceForIndex(i);
  }
}

SGGeodVec RoutePath::pathForIndex(int index) const
{
  const WayptData& w(d->waypoints[index]);
  const std::string& ty(w.wpt->type());
  SGGeodVec r;
  
  if (d->waypoints[index].skipped) {
    return {};
  }
  
  // don't show any path
  if (w.wpt->flag(WPT_HIDDEN)) {
    return {};
  }

  // also hide the path if the prev WP is hidden
  auto prevIt = d->previousValidWaypoint(index);
  if (prevIt != d->waypoints.end()) {
      if (prevIt->wpt->flag(WPT_HIDDEN)) {
          return {};
      }
  }

  if (ty == "vectors") {
    // ideally we'd show a stippled line to connect the route?
    return {};
  }
  
  if (ty == "discontinuity") {
    return {}; // no points for a discontinuity of course
  }
  
  if (ty == "via") {
    return pathForVia(static_cast<Via*>(d->waypoints[index].wpt.get()), index);
  }


  if (prevIt != d->waypoints.end()) {
    prevIt->turnExitPath(r);
    
    SGGeod from = prevIt->turnExitPos,
    to = w.turnEntryPos;
    
    // compute rounding offset, we want to round towards the direction of travel
    // which depends on the east/west sign of the longitude change
    double lonDelta = to.getLongitudeDeg() - from.getLongitudeDeg();
    if (fabs(lonDelta) > 0.5) {
      interpolateGreatCircle(from, to, r);
    }
  } // of have previous waypoint
  
  w.turnEntryPath(r);
  
  // hold is the normal leg and then the hold waypoints as well
  if (ty== "hold") {
    const auto h = static_cast<Hold*>(d->waypoints[index].wpt.get());
    const auto holdPath = pathForHold(h);
    r.insert(r.end(), holdPath.begin(), holdPath.end());
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

SGGeodVec RoutePath::pathForVia(Via* via, int index) const
{
    // previous waypoint must be valid for a VIA
    auto prevIt = d->previousValidWaypoint(index);
    if (prevIt == d->waypoints.end()) {
        return SGGeodVec();
    }

    WayptVec enrouteWaypoints = via->expandToWaypoints(prevIt->wpt);
    SGGeodVec r;

    WayptVec::const_iterator it;
    SGGeod legStart = prevIt->wpt->position();
    for (it = enrouteWaypoints.begin(); it != enrouteWaypoints.end(); ++it) {
        // interpolate directly into the result vector
        interpolateGreatCircle(legStart, (*it)->position(), r);
        legStart = (*it)->position();
    }

    return r;
}

SGGeodVec RoutePath::pathForHold(Hold* hold) const
{
    int turnSteps = 16;
    double hdg = hold->inboundRadial();
    double turnDelta = 180.0 / turnSteps;
    double altFt = 0.0; // FIXME
    double gsKts = d->perf.groundSpeedForAltitudeKnots(altFt);

    SGGeodVec r;
    double az2;
    double stepTime = turnDelta / 3.0; // 3.0 is degrees/sec for standard rate turn
    double stepDist = gsKts * SG_KT_TO_MPS * stepTime;
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

    auto it = d->waypoints.begin() + index;
    if ((index == 0) || it->skipped) {
        // first waypoint, distance is 0
        return 0.0;
    }

    const auto ty = it->wpt->type();
    if (ty == "via") {
        return distanceForVia(static_cast<Via*>(it->wpt.get()), index);
    }
    
    if (ty == "discontinuity") {
        return 0.0;
    }

  auto prevIt = d->previousValidWaypoint(index);
  if (prevIt == d->waypoints.end()) {
    return 0.0;
  }

    double dist = SGGeodesy::distanceM(prevIt->turnExitPos, it->turnEntryPos);
    dist += prevIt->turnDistanceM();

    if (!it->flyOver) {
        // add entry distance
        dist += it->turnDistanceM();
    }
    
    return dist;
}

double RoutePath::distanceForVia(Via* via, int index) const
{
    auto prevIt = d->previousValidWaypoint(index);
    if (prevIt == d->waypoints.end()) {
        return 0.0;
    }

    WayptVec enrouteWaypoints = via->expandToWaypoints(prevIt->wpt);
    double dist = 0.0;

    WayptVec::const_iterator it;
    SGGeod legStart = prevIt->wpt->position();
    for (auto wp : enrouteWaypoints) {
        dist += SGGeodesy::distanceM(legStart, wp->position());
        legStart = wp->position();
    }

    return dist;
}

double RoutePath::trackForIndex(int index) const
{
    const auto sz = static_cast<int>(d->waypoints.size());
    if ((index < 0) || (index >= sz)) {
        return 0.0;
    }

    if (d->waypoints[index].skipped)
        return trackForIndex(index - 1);

    const WayptData& wd(d->waypoints[index]);
    if (!wd.legCourseValid)
        return 0.0;

    return wd.legCourseTrue;
}

double RoutePath::distanceForIndex(int index) const
{
    const auto sz = static_cast<int>(d->waypoints.size());
    if ((index < 0) || (index >= sz)) {
        return 0.0;
    }

  return d->waypoints[index].pathDistanceM;
}

double RoutePath::distanceBetweenIndices(int from, int to) const
{
  return d->distanceBetweenIndices(from, to);
}

SGGeod RoutePath::positionForDistanceFrom(int index, double distanceM) const
{
  int sz = (int) d->waypoints.size();
  if (index < 0) {
    index = sz - 1; // map negative values to end of the route
  }
  
  if ((index < 0) || (index >= sz)) {
    throw sg_range_exception("waypt index out of range",
                             "RoutePath::positionForDistanceFrom");
  }
  
  // find the actual leg we're within
  if (distanceM < 0.0) {
    // scan backwards
    while ((index > 0) && (distanceM < 0.0)) {
      // we are looking at index n, say 4, but with a negative distance.
      // we want to look at index n-1 (so, 3), and see if this makes
      // distance positive. We need to offset by distance from 3 -> 4,
      // which is waypoint 4's path distance.
      
      // note pathDistanceM is 0 for skipped waypoints, so this works out
      distanceM += d->waypoints[index].pathDistanceM;
      --index;
    }
    
    if (distanceM < 0.0) {
      // still negative, return route start
      return d->waypoints[0].pos;
    }
    
  } else {
    // scan forwards
    int nextIndex = index + 1;
    while ((nextIndex < sz) && (d->waypoints[nextIndex].pathDistanceM < distanceM)) {
      distanceM -= d->waypoints[nextIndex].pathDistanceM;
      index = nextIndex++;
    }
  }
  
  auto nextIt = d->nextValidWaypoint(index);
  if (nextIt == d->waypoints.end()) {
    // past route end, just return final position
    return d->waypoints[sz - 1].pos;
  }
  
  // this is important so we start from a valid WP if we're
  // working either side of a DISCON
  auto curIt = d->previousValidWaypoint(nextIt);
  if (curIt == d->waypoints.end()) {
    SG_LOG(SG_NAVAID, SG_WARN, "Couldn't find valid preceeding waypoint " << index);
    return nextIt->pos;
  }
  
  const WayptData& wpt = *curIt;
  const WayptData& next = *nextIt;
  
  if (next.wpt->type() == "via") {
    return positionAlongVia(static_cast<Via*>(next.wpt.get()), index, distanceM);
  }
  
  if (wpt.turnPathDistanceM > distanceM) {
    // on the exit path of current wpt
    return wpt.pointAlongExitPath(distanceM);
  } else {
    distanceM -= wpt.turnPathDistanceM;
  }
  
  double corePathDistance = next.pathDistanceM - next.turnPathDistanceM;
  if (next.hasEntry && (distanceM > corePathDistance)) {
    // on the entry path of next waypoint
    return next.pointAlongEntryPath(distanceM - corePathDistance);
  }
  
  // linear between turn exit and turn entry points
  return SGGeodesy::direct(wpt.turnExitPos, next.legCourseTrue, distanceM);
}

SGGeod RoutePath::positionAlongVia(Via* via, int previousIndex, double distanceM) const
{
    SG_LOG(SG_NAVAID, SG_ALERT, "RoutePath::positionAlongVia not implemented");
    return SGGeod();
}
