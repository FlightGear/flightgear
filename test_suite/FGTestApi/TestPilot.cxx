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
    
void TestPilot::updateValues(double dt)
{
    if (_turnActive) {
        if (fabs(_targetCourseDeg - _trueCourseDeg) < 0.1) {
            _trueCourseDeg = _targetCourseDeg;
            _turnActive = false;
        } else {
            // standard 2-minute turn, 180-deg min, thus 3-degrees per second
            double turnDeg = 3.0 * dt;
            double errorDeg = _targetCourseDeg - _trueCourseDeg;
            if (fabs(errorDeg) > 180.0) {
                errorDeg = 360.0 - errorDeg;
            }
            
            turnDeg = copysign(turnDeg, errorDeg);
            // simple integral
            _trueCourseDeg += std::min(turnDeg, errorDeg);
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
    _propRoot->setDoubleValue("position/latitude-deg", pos.getLatitudeDeg());
    _propRoot->setDoubleValue("position/longitude-deg", pos.getLongitudeDeg());
    _propRoot->setDoubleValue("position/altitude-ft", pos.getElevationFt());
    
    _propRoot->setDoubleValue("orientation/heading-deg", _trueCourseDeg);
    _propRoot->setDoubleValue("velocities/speed-knots", _speedKnots);
    _propRoot->setDoubleValue("velocities/vertical-fpm", _vspeedFPM);

}
    
void TestPilot::setTargetAltitudeFtMSL(double altFt)
{
    _targetAltitudeFt = altFt;
    _altActive = true;
}

} // of namespace
