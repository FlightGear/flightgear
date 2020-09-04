// AircraftPerformance.cxx - compute data about planned acft performance
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

#include "AircraftPerformance.hxx"

#include <cassert>
#include <algorithm>

#include <simgear/constants.h>

#include <Main/fg_props.hxx>

using namespace flightgear;

double distanceForTimeAndSpeeds(double tSec, double v1, double v2)
{
    return tSec * 0.5 * (v1 + v2);
}

AircraftPerformance::AircraftPerformance()
{
    // read aircraft supplied performance data
    if (fgGetNode("/aircraft/performance/bracket")) {
        readPerformanceData();
    } else {
        // falls back to heuristic determination of the category,
        // and a plausible default
        icaoCategoryData();
    }
}

double AircraftPerformance::groundSpeedForAltitudeKnots(int altitudeFt) const
{
    auto bracket = bracketForAltitude(altitudeFt);
    return bracket->gsForAltitude(altitudeFt);
}

int AircraftPerformance::computePreviousAltitude(double distanceM, int targetAltFt) const
{
    auto bracket = bracketForAltitude(targetAltFt);
    auto d = bracket->descendDistanceM(bracket->atOrBelowAltitudeFt, targetAltFt);
    if (d < distanceM) {
        // recurse to previous bracket
        return computePreviousAltitude(distanceM - d, bracket->atOrBelowAltitudeFt+1);
    }

    // work out how far we travel laterally per foot change in altitude
    // this value is in metres, we have to map FPM and GS in Knots to make
    // everything work out
    const double gsMPS =  bracket->gsForAltitude(targetAltFt) * SG_KT_TO_MPS;
    const double t = distanceM / gsMPS;
    return targetAltFt + bracket->descentRateFPM * (t / 60.0);
}

int AircraftPerformance::computeNextAltitude(double distanceM, int initialAltFt) const
{
    auto bracket = bracketForAltitude(initialAltFt);
    auto d = bracket->climbDistanceM(initialAltFt, bracket->atOrBelowAltitudeFt);
    if (d < distanceM) {
        // recurse to next bracket
        return computeNextAltitude(distanceM - d, bracket->atOrBelowAltitudeFt+1);
    }

    // work out how far we travel laterally per foot change in altitude
    // this value is in metres, we have to map FPM and GS in Knots to make
    // everything work out
    const double gsMPS =  bracket->gsForAltitude(initialAltFt) * SG_KT_TO_MPS;
    const double t = distanceM / gsMPS;
    return initialAltFt + bracket->climbRateFPM * (t / 60.0);
}

static string_list readTags()
{
    string_list r;
    const auto tagsNode = fgGetNode("/sim/tags");
    if (!tagsNode)
        return r;
    
    for (auto t : tagsNode->getChildren("tag")) {
        r.push_back(t->getStringValue());
    }
    return r;
}

static bool stringListContains(const string_list& t, const std::string& s)
{
    auto it = std::find(t.begin(), t.end(), s);
    return it != t.end();
}

std::string AircraftPerformance::heuristicCatergoryFromTags() const
{
    const auto tags(readTags());
    
    if (stringListContains(tags, "turboprop"))
        return {ICAO_AIRCRAFT_CATEGORY_C};
    
    // any way we could distuinguish fast and slow GA aircraft?
    if (stringListContains(tags, "ga")) {
        return {ICAO_AIRCRAFT_CATEGORY_A};
    }
    
    if (stringListContains(tags, "jet")) {
        return {ICAO_AIRCRAFT_CATEGORY_E};
    }
    
    return {ICAO_AIRCRAFT_CATEGORY_C};
}

void AircraftPerformance::icaoCategoryData()
{
    std::string propCat = fgGetString("/aircraft/performance/icao-category");
    if (propCat.empty()) {
        propCat = heuristicCatergoryFromTags();
    }

    const char aircraftCategory = propCat.front();
     //     pathTurnRate = 3.0; // 3 deg/sec = 180deg/min = standard rate turn
      switch (aircraftCategory) {
      case ICAO_AIRCRAFT_CATEGORY_A:
          _perfData.push_back(Bracket(4000, 600, 1200, 75));
          _perfData.push_back(Bracket(10000, 600, 1200, 140));
          break;

      case ICAO_AIRCRAFT_CATEGORY_B:
          _perfData.push_back(Bracket(4000, 100, 1200, 100));
          _perfData.push_back(Bracket(10000, 800, 1200, 160));
          _perfData.push_back(Bracket(18000, 600, 1800, 200));
          break;

      case ICAO_AIRCRAFT_CATEGORY_C:
          _perfData.push_back(Bracket(4000, 1800, 1800, 150));
          _perfData.push_back(Bracket(10000, 1800, 1800, 200));
          _perfData.push_back(Bracket(18000, 1200, 1800, 270));
          _perfData.push_back(Bracket(60000, 800, 1200, 0.80, true /* is Mach */));
          break;

      case ICAO_AIRCRAFT_CATEGORY_D:
      case ICAO_AIRCRAFT_CATEGORY_E:
      default:
          _perfData.push_back(Bracket(4000, 1800, 1800, 180));
          _perfData.push_back(Bracket(10000, 1800, 1800, 230));
          _perfData.push_back(Bracket(18000, 1200, 1800, 270));
          _perfData.push_back(Bracket(60000, 800, 1200, 0.87, true /* is Mach */));
          break;
      }
}

void AircraftPerformance::readPerformanceData()
{
    for (auto nd : fgGetNode("/aircraft/performance/")->getChildren("bracket")) {
        const int atOrBelowAlt = nd->getIntValue("at-or-below-ft");
        const int climbFPM = nd->getIntValue("climb-rate-fpm");
        const int descentFPM = nd->getIntValue("descent-rate-fpm");
        bool isMach = nd->hasChild("speed-mach");
        double speed;
        if (isMach) {
            speed = nd->getDoubleValue("speed-mach");
        } else {
            speed = nd->getIntValue("speed-ias-knots");
        }

        Bracket b(atOrBelowAlt, climbFPM, descentFPM, speed, isMach);
        _perfData.push_back(b);
    }
}

auto AircraftPerformance::bracketForAltitude(int altitude) const
        -> PerformanceVec::const_iterator
{
    assert(!_perfData.empty());
    if (_perfData.front().atOrBelowAltitudeFt >= altitude)
        return _perfData.begin();

    for (auto it = _perfData.begin(); it != _perfData.end(); ++it) {
        if (it->atOrBelowAltitudeFt > altitude) {
            return it;
        }
    }

    return _perfData.end() - 1;
}

auto AircraftPerformance::rangeForAltitude(int lowAltitude, int highAltitude) const
        -> BracketRange
{
    return {bracketForAltitude(lowAltitude), bracketForAltitude(highAltitude)};
}

void AircraftPerformance::traverseAltitudeRange(int initialElevationFt, int targetElevationFt,
                                                TraversalFunc tf) const
{
    auto r = rangeForAltitude(initialElevationFt, targetElevationFt);
    if (r.first == r.second) {
       tf(*r.first, initialElevationFt, targetElevationFt);
       return;
    }

    if (initialElevationFt < targetElevationFt) {
        tf(*r.first, initialElevationFt, r.first->atOrBelowAltitudeFt);
        int previousBracketCapAltitude = r.first->atOrBelowAltitudeFt;
        for (auto bracket = r.first + 1; bracket != r.second; ++bracket) {
            tf(*bracket, previousBracketCapAltitude, bracket->atOrBelowAltitudeFt);
            previousBracketCapAltitude = bracket->atOrBelowAltitudeFt;
        }
        
        tf(*r.second, previousBracketCapAltitude, targetElevationFt);
    } else {
        int nextBracketCapAlt = (r.first - 1)->atOrBelowAltitudeFt;
        tf(*r.first, initialElevationFt, nextBracketCapAlt);
        for (auto bracket = r.first - 1; bracket != r.second; --bracket) {
            nextBracketCapAlt = (r.first - 1)->atOrBelowAltitudeFt;
            tf(*bracket, bracket->atOrBelowAltitudeFt, nextBracketCapAlt);
        }
        
        tf(*r.second, nextBracketCapAlt, targetElevationFt);
    }
}

double AircraftPerformance::distanceNmBetween(int initialElevationFt, int targetElevationFt) const
{
    double result = 0.0;
    TraversalFunc tf = [&result](const Bracket& bk, int alt1, int alt2) {
        result += (alt1 > alt2) ? bk.descendDistanceM(alt1, alt2) : bk.climbDistanceM(alt1, alt2);
    };
    traverseAltitudeRange(initialElevationFt, targetElevationFt, tf);
    return result * SG_METER_TO_NM;
}

double AircraftPerformance::timeBetween(int initialElevationFt, int targetElevationFt) const
{
    double result = 0.0;
    TraversalFunc tf = [&result](const Bracket& bk, int alt1, int alt2) {
        SG_LOG(SG_GENERAL, SG_INFO, "Range:" << alt1 << " " << alt2);
        result += (alt1 > alt2) ? bk.descendTime(alt1, alt2) : bk.climbTime(alt1, alt2);
    };
    traverseAltitudeRange(initialElevationFt, targetElevationFt, tf);
    return result;
}

double AircraftPerformance::timeToCruise(double cruiseDistanceNm, int cruiseAltitudeFt) const
{
    auto b = bracketForAltitude(cruiseAltitudeFt);
    return (cruiseDistanceNm / b->gsForAltitude(cruiseAltitudeFt)) * 3600.0;
}

double oatCForAltitudeFt(int altitudeFt)
{
    if (altitudeFt > 36089)
        return -56.5;

    // lapse rate in C per ft
    const double T_r = .0019812;
    return 15.0 - (altitudeFt * T_r);
}

double oatKForAltitudeFt(int altitudeFt)
{
    return oatCForAltitudeFt(altitudeFt) + 273.15;
}

double pressureAtAltitude(int altitude)
{
/*
  p= P_0*(1-6.8755856*10^-6 h)^5.2558797    h<36,089.24ft
  p_Tr= 0.2233609*P_0                  
  p=p_Tr*exp(-4.806346*10^-5(h-36089.24)) h>36,089.24ft 

    magic numbers
    6.8755856*10^-6 = T'/T_0, where T' is the standard temperature lapse rate and T_0 is the standard sea-level temperature.
    5.2558797 = Mg/RT', where M is the (average) molecular weight of air, g is the acceleration of gravity and R is the gas constant.
    4.806346*10^-5 = Mg/RT_tr, where T_tr is the temperature at the tropopause.
*/

    const double k = 6.8755856e-6;
    const double MgRT = 5.2558797;
    const double MgRT_tr = 4.806346e-5;
    const double P_0 = 29.92126; // (standard) sea-level pressure
    if (altitude > 36089) {
        const double P_Tr = 0.2233609 * P_0;
        const double altAboveTr = altitude - 36089;
        return P_Tr * exp(MgRT_tr * altAboveTr);
    } else {
        return P_0 * pow(1.0 - (k * altitude), MgRT);
    }
}

double computeMachFromIAS(int iasKnots, int altitudeFt)
{
#if 0
 // from the aviation formulary
  DP=P_0*((1 + 0.2*(IAS/CS_0)^2)^3.5 -1)
  M=(5*( (DP/P + 1)^(2/7) -1) )^0.5   (*)
#endif
    const double Cs_0 = 661.4786; // speed of sound at sea level, knots
    const double P_0 = 29.92126; // (standard) sea-level pressure
    const double iasCsRatio = iasKnots / Cs_0;
    const double P = pressureAtAltitude(altitudeFt);
    // differential pressure
    const double DP = P_0 * (pow(1.0 + 0.2 * pow(iasCsRatio, 2.0), 3.5) - 1.0);

    const double pressureRatio = DP / P + 1.0;
    const double M = pow(5.0 * (pow(pressureRatio, 2.0 / 7.0) - 1.0), 0.5);
    if (M > 1.0) {
        SG_LOG(SG_GENERAL, SG_INFO, "computeMachFromIAS: computed Mach is supersonic, fix for shock wave");
    }
    return M;
}

double AircraftPerformance::machForCAS(int altitudeFt, double cas)
{
    return computeMachFromIAS(static_cast<int>(cas), altitudeFt);
}


double AircraftPerformance::groundSpeedForCAS(int altitudeFt, double cas)
{
    return groundSpeedForMach(altitudeFt, computeMachFromIAS(cas, altitudeFt));
}

double AircraftPerformance::groundSpeedForMach(int altitudeFt, double mach)
{
    // CS = sound speed= 38.967854*sqrt(T+273.15)  where T is the OAT in celsius.
    const double CS = 38.967854 * sqrt(oatKForAltitudeFt(altitudeFt));
    const double TAS = mach * CS;
    return TAS;
}

int AircraftPerformance::Bracket::gsForAltitude(int altitude) const
{
    double M = 0.0;
    if (speedIsMach) {
        M = speedIASOrMach; // simple
    } else {
        M = computeMachFromIAS(speedIASOrMach, altitude);
    }

    return groundSpeedForMach(altitude, M);
}

double AircraftPerformance::Bracket::climbTime(int alt1, int alt2) const
{
    return (alt2 - alt1) / static_cast<double>(climbRateFPM) * 60.0;
}

double AircraftPerformance::Bracket::climbDistanceM(int alt1, int alt2) const
{
    const double t = climbTime(alt1, alt2);
    return distanceForTimeAndSpeeds(t,
                                    SG_KT_TO_MPS * gsForAltitude(alt1),
                                    SG_KT_TO_MPS * gsForAltitude(alt2));
}

double AircraftPerformance::Bracket::descendTime(int alt1, int alt2) const
{
    return (alt1 - alt2) / static_cast<double>(descentRateFPM) * 60.0;
}

double AircraftPerformance::Bracket::descendDistanceM(int alt1, int alt2) const
{
    const double t = descendTime(alt1, alt2);
    return distanceForTimeAndSpeeds(t,
                                    SG_KT_TO_MPS * gsForAltitude(alt1),
                                    SG_KT_TO_MPS * gsForAltitude(alt2));
}

double AircraftPerformance::turnRadiusMForAltitude(int altitudeFt) const
{
#if 0
    From the aviation formulary again
    In a steady turn, in no wind, with bank angle, b at an airspeed v
    
    tan(b)= v^2/(R g)
    
    With R in feet, v in knots, b in degrees and w in degrees/sec (inconsistent units!), numerical constants are introduced:
    
    R =v^2/(11.23*tan(0.01745*b))
    (Example) At 100 knots, with a 45 degree bank, the radius of turn is 100^2/(11.23*tan(0.01745*45))= 891 feet.
    
    The bank angle b_s for a standard rate turn is given by:
        
        b_s = 57.3*atan(v/362.1)
        (Example) for 100 knots, b_s = 57.3*atan(100/362.1) = 15.4 degrees
            
    Working in meter-per-second and radians removes a bunch of constants again.
#endif
    const double gsKts = groundSpeedForAltitudeKnots(altitudeFt);
    const double gs = gsKts * SG_KT_TO_MPS;
    const double bankAngleRad = atan(gsKts/362.1);
    const double r = (gs * gs)/(SG_g0_m_p_s2 * tan(bankAngleRad));
    return r;
}

