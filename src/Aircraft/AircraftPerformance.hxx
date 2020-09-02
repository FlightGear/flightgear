// AircraftPerformance.hxx - compute data about planned acft performance
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

#ifndef AIRCRAFTPERFORMANCE_HXX
#define AIRCRAFTPERFORMANCE_HXX

#include <string>
#include <vector>
#include <functional>

namespace flightgear
{

const char ICAO_AIRCRAFT_CATEGORY_A = 'A';
const char ICAO_AIRCRAFT_CATEGORY_B = 'B';
const char ICAO_AIRCRAFT_CATEGORY_C = 'C';
const char ICAO_AIRCRAFT_CATEGORY_D = 'D';
const char ICAO_AIRCRAFT_CATEGORY_E = 'E';

/**
 * Calculate flight parameter based on aircraft performance data. 
 * This is based on simple rules: it does not (yet) include data
 * such as winds aloft, payload or temperature impact on engine
 * performance. */
class AircraftPerformance
{
public:
    AircraftPerformance();

    double turnRateDegSec() const;

    double turnRadiusMForAltitude(int altitudeFt) const;
    
    double groundSpeedForAltitudeKnots(int altitudeFt) const;

    int computePreviousAltitude(double distanceM, int targetAltFt) const;
    int computeNextAltitude(double distanceM, int initialAltFt) const;

    double distanceNmBetween(int initialElevationFt, int targetElevationFt) const;
    double timeBetween(int initialElevationFt, int targetElevationFt) const;

    double timeToCruise(double cruiseDistanceNm, int cruiseAltitudeFt) const;

    static double groundSpeedForCAS(int altitudeFt, double cas);
    static double machForCAS(int altitudeFt, double cas);
    static double groundSpeedForMach(int altitudeFt, double mach);

private:
    void readPerformanceData();

    void icaoCategoryData();

    /**
     * @brief heuristicCatergoryFromTags - based on the aircraft tags, figure
     * out a plausible ICAO category. Returns cat A if nothing better could
     * be determined.
     * @return a string containing a single ICAO category character A..E
     */
    std::string heuristicCatergoryFromTags() const;

    class Bracket
    {
    public:
        Bracket(int atOrBelow, int climb, int descent, double speed, bool isMach = false) :
            atOrBelowAltitudeFt(atOrBelow),
            climbRateFPM(climb),
            descentRateFPM(descent),
            speedIASOrMach(speed),
            speedIsMach(isMach)
        { }
        
        int gsForAltitude(int altitude) const;

        double climbTime(int alt1, int alt2) const;
        double climbDistanceM(int alt1, int alt2) const;
        double descendTime(int alt1, int alt2) const;
        double descendDistanceM(int alt1, int alt2) const;

        int atOrBelowAltitudeFt;
        int climbRateFPM;
        int descentRateFPM;
        double speedIASOrMach;
        bool speedIsMach = false;
    };

    using PerformanceVec = std::vector<Bracket>;

    using BracketRange = std::pair<PerformanceVec::const_iterator, PerformanceVec::const_iterator>;

    PerformanceVec::const_iterator bracketForAltitude(int altitude) const;
    BracketRange rangeForAltitude(int lowAltitude, int highAltitude) const;


    using TraversalFunc = std::function<void(const Bracket& bk, int alt1, int alt2)>;
    void traverseAltitudeRange(int initialElevationFt, int targetElevationFt, TraversalFunc tf) const;


    PerformanceVec _perfData;
};

}

#endif // AIRCRAFTPERFORMANCE_HXX
