/*
 * Copyright (C) 2019 James Turner
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FGTEST_API_TEST_PILOT_HXX
#define _FGTEST_API_TEST_PILOT_HXX

#include <simgear/props/propsfwd.hxx>
#include <simgear/math/SGMathFwd.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/SGGeod.hxx>

class GPS;

namespace FGTestApi {

/**
 * @brief simulation of the user (pilot) fying in a particular way
 * around the world. Standard property tree values are updated
 * (position / orientation / velocity)
 */
    class TestPilot : public SGSubsystem
{
public:
    TestPilot(SGPropertyNode_ptr props = {});
    ~TestPilot();
    
    void resetAtPosition(const SGGeod& pos);

    void setSpeedKts(double knots);

    void setCourseTrue(double deg);
    void turnToCourse(double deg);

    void setVerticalFPM(double fpm);
    void setTargetAltitudeFtMSL(double altFt);

    // void setTurnRateDegSec();

    void init() override;
    void update(double dT) override;

    void flyHeading(double hdg);
    void flyDirectTo(const SGGeod& target);
    void flyGPSCourse(GPS *gps);
    
    void flyGPSCourseOffset(GPS *gps, double offsetNm);

    bool isOnHeading(double heading) const;
private:
    enum class LateralMode
    {
        Heading,
        Direct,
        GPSCourse,
        GPSOffset
    };
    
    void updateValues(double dt);
    void setPosition(const SGGeod& pos);

    SGPropertyNode_ptr _propRoot;

    double _trueCourseDeg = 0.0;
    double _speedKnots = 0.0;
    double _vspeedFPM = 0.0;

    bool _turnActive = false;
    bool _altActive = false;
    double _targetCourseDeg = 0.0;
    double _targetAltitudeFt = 0.0;
  
    LateralMode _lateralMode = LateralMode::Heading;
    SGGeod _targetPos;
    GPS* _gps = nullptr;
    double _courseOffsetNm =0.0;
    
    SGPropertyNode_ptr _latProp;
    SGPropertyNode_ptr _lonProp;
    SGPropertyNode_ptr _altitudeProp;
    SGPropertyNode_ptr _headingProp;
    SGPropertyNode_ptr _speedKnotsProp;
    SGPropertyNode_ptr _verticalFPMProp;
    
    SGPropertyNode_ptr _gpsNode;
    SGPropertyNode_ptr _gpsLegCourse;
    SGPropertyNode_ptr _courseErrorNm;
};

} // of namespace FGTestApi

#endif // of _FGTEST_API_TEST_PILOT_HXX

