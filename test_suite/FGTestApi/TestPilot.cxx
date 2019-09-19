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

#include "TestPilot.hxx"

#include <simgear/math/SGGeodesy.hxx>
#include <simgear/props/props.hxx>
#include <simgear/math/SGGeod.hxx>

#include <Main/globals.hxx>

namespace FGTestApi {

TestPilot::TestPilot(SGPropertyNode_ptr props) :
    _propRoot(props)
{
    if (!_propRoot) {
        // use default properties
        _propRoot = globals->get_props();
    }
    
    _latProp = _propRoot->getNode("position/latitude-deg", true);
    _lonProp = _propRoot->getNode("position/longitude-deg", true);
    _altitudeProp = _propRoot->getNode("position/altitude-ft", true);
    _headingProp = _propRoot->getNode("orientation/heading-deg", true);
    _speedKnotsProp = _propRoot->getNode("velocities/speed-knots", true);
    _verticalFPMProp = _propRoot->getNode("velocities/vertical-fpm", true);

    
    SGPropertyNode_ptr _latProp;
    SGPropertyNode_ptr _lonProp;
    SGPropertyNode_ptr _altitudeProp;
    SGPropertyNode_ptr _headingProp;
    SGPropertyNode_ptr _speedKnotsProp;
    SGPropertyNode_ptr _verticalFPMProp;
    
    globals->add_subsystem("flight", this, SGSubsystemMgr::FDM);
}

TestPilot::~TestPilot()
{
}

void TestPilot::resetAtPosition(const SGGeod& pos)
{
    _turnActive = false;
    setPosition(pos);
}

void TestPilot::init()
{
    _vspeedFPM = 1200;
}

void TestPilot::update(double dt)
{
    updateValues(dt);
}
    
void TestPilot::setSpeedKts(double knots)
{
    _speedKnots = knots;
}

void TestPilot::setCourseTrue(double deg)
{
    _trueCourseDeg = deg;
}

void TestPilot::turnToCourse(double deg)
{
    _turnActive = true;
    _targetCourseDeg = deg;
}

void TestPilot::flyHeading(double hdg)
{
    _lateralMode = LateralMode::Heading;
    _turnActive = true;
    _targetCourseDeg = hdg;
}
    
void TestPilot::flyGPSCourse(GPS *gps)
{
    _gps = gps;
    _gpsNode = globals->get_props()->getNode("instrumentation/gps");
    _lateralMode = LateralMode::GPSCourse;
    _turnActive = false;
}

void TestPilot::flyDirectTo(const SGGeod& target)
{
    _lateralMode = LateralMode::Direct;
    _targetPos = target;
}
    
void TestPilot::updateValues(double dt)
{
    if (_gps && (_lateralMode == LateralMode::GPSCourse)) {
        double deviationDeg = _gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg");
        _targetCourseDeg = _gpsNode->getDoubleValue("wp/leg-true-course-deg");
        
        const double absDev = fabs(deviationDeg);
        const double minInterceptAngle = 1.5;
        // if we're getting close to the leg track, artifically keep the deviation a bit up,
        // to avoid really slow convergence on it
        if ((absDev > 0.05) && (absDev < minInterceptAngle)) {
            deviationDeg = copysign(minInterceptAngle, deviationDeg);
        }
        
        // set how aggressively we try to correct our course
        const double courseCorrectionFactor = 8.0;
        _targetCourseDeg += courseCorrectionFactor * deviationDeg;
        
        SG_NORMALIZE_RANGE(_targetCourseDeg, 0.0, 360.0);
        if (!_turnActive &&(fabs(_trueCourseDeg - _targetCourseDeg) > 0.5)) {
            _turnActive = true;
        }
    }
    
    if (_lateralMode == LateralMode::Direct) {
        _targetCourseDeg = SGGeodesy::courseDeg(globals->get_aircraft_position(), _targetPos);
        SG_NORMALIZE_RANGE(_targetCourseDeg, 0.0, 360.0);
        if (!_turnActive && (fabs(_trueCourseDeg - _targetCourseDeg) > 0.5)) {
            _turnActive = true;
        }
    }
    
    if (_turnActive) {
        if (fabs(_targetCourseDeg - _trueCourseDeg) < 0.1) {
            _trueCourseDeg = _targetCourseDeg;
            _turnActive = false;
        } else {
            // standard 2-minute turn, 180-deg min, thus 3-degrees per second
            double turnDeg = 3.0 * dt;
            double errorDeg = _targetCourseDeg - _trueCourseDeg;
            if (errorDeg > 180.0) {
                errorDeg = errorDeg -= 360;
            } else if (errorDeg < -180) {
                errorDeg += 360.0;
            }
            
            // clamp turn to error value
            turnDeg = std::min(turnDeg, fabs(errorDeg));
            
            // and now ensure we follow the correct sign
            turnDeg = copysign(turnDeg, errorDeg);
            
            // simple integral
            _trueCourseDeg += turnDeg;
            SG_NORMALIZE_RANGE(_trueCourseDeg, 0.0, 360.0);
        }
    }
    
    SGGeod currentPos = globals->get_aircraft_position();
    double d = _speedKnots * SG_KT_TO_MPS * dt;
    SGGeod newPos = SGGeodesy::direct(currentPos, _trueCourseDeg, d);
    
    if (_altActive) {
        if (fabs(_targetAltitudeFt - currentPos.getElevationFt()) < 1) {
            _altActive = false;
            newPos.setElevationFt(_targetAltitudeFt);
        } else {
            double errorFt = _targetAltitudeFt - currentPos.getElevationFt();
            double vspeed = std::min(fabs(errorFt),_vspeedFPM * dt / 60.0);
            double dv = copysign(vspeed, errorFt);
            newPos.setElevationFt(currentPos.getElevationFt() + dv);
        }
    }
    
    setPosition(newPos);
}

void TestPilot::setPosition(const SGGeod& pos)
{
    _latProp->setDoubleValue(pos.getLatitudeDeg());
    _lonProp->setDoubleValue(pos.getLongitudeDeg());
    _altitudeProp->setDoubleValue(pos.getElevationFt());
    
    _headingProp->setDoubleValue(_trueCourseDeg);
    _speedKnotsProp->setDoubleValue(_speedKnots);
    _verticalFPMProp->setDoubleValue(_vspeedFPM);
}
    
void TestPilot::setTargetAltitudeFtMSL(double altFt)
{
    _targetAltitudeFt = altFt;
    _altActive = true;
}

} // of namespace
