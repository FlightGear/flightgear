// Radar Altimeter
//
// Written by Vivian MEAZZA, started Feb 2008.
//
//
// Copyright (C) 2008  Vivian Meazza
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
//
//

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "rad_alt.hxx"

#include <simgear/scene/material/mat.hxx>


#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Scenery/scenery.hxx>


RadarAltimeter::RadarAltimeter(SGPropertyNode *node) :
    _time(0.0),
    _interval(node->getDoubleValue("update-interval-sec", 1.0))
{
    _name = node->getStringValue("name", "radar-altimeter");
    _num = node->getIntValue("number", 0);
}

RadarAltimeter::~RadarAltimeter()
{
}

void
RadarAltimeter::init ()
{

    std::string branch = "/instrumentation/" + _name;
    _Instrument = fgGetNode(branch.c_str(), _num, true);

    _sceneryLoaded = fgGetNode("/sim/sceneryloaded", true);
    _serviceable_node = _Instrument->getNode("serviceable", true);

    _user_alt_agl_node     = fgGetNode("/position/altitude-agl-ft", true);
    _rad_alt_warning_node   = fgGetNode("/sim/alarms/rad-alt-warning", true);

    _Instrument->setFloatValue("tilt",-85);
    _Instrument->setStringValue("status","RA");

    _Instrument->getDoubleValue("elev-limit", true);
    _Instrument->getDoubleValue("elev-step-deg", true);
    _Instrument->getDoubleValue("az-limit-deg", true);
    _Instrument->getDoubleValue("az-step-deg", true);
    _Instrument->getDoubleValue("max-range-m", true);
    _Instrument->getDoubleValue("min-range-m", true);
    _Instrument->getDoubleValue("tilt", true);
    _Instrument->getDoubleValue("set-height-ft", true);
    _Instrument->getDoubleValue("set-excursion-percent", true);

    _antennaOffset = SGVec3d(_Instrument->getDoubleValue("antenna/x-offset-m"),
                             _Instrument->getDoubleValue("antenna/y-offset-m"),
                             _Instrument->getDoubleValue("antenna/z-offset-m"));
}

void
RadarAltimeter::update (double delta_time_sec)
{
    if (!_sceneryLoaded->getBoolValue())
        return;

    if ( ! _serviceable_node->getBoolValue() ) {
        _Instrument->setStringValue("status","");
        return;
    }

    _time += delta_time_sec;
    if (_time < _interval)
        return;

    _time -= _interval;

    update_altitude();
    updateSetHeight();
}

double
RadarAltimeter::getDistanceAntennaToHit(const SGVec3d& nearestHit) const
{
    return norm(nearestHit - getCartAntennaPos());
}

void
RadarAltimeter::updateSetHeight()
{
    double set_ht_ft   = _Instrument->getDoubleValue("set-height-ft", 9999);
    double set_excur   = _Instrument->getDoubleValue("set-excursion-percent", 0);
    if (set_ht_ft == 9999) {
        _rad_alt_warning_node->setIntValue(9999);
        return;
    }
    
    double radarAltFt = _min_radalt * SG_METER_TO_FEET;
    if (radarAltFt < set_ht_ft * (100 - set_excur)/100)
        _rad_alt_warning_node->setIntValue(-1);
    else if (radarAltFt > set_ht_ft * (100 + set_excur)/100)
        _rad_alt_warning_node->setIntValue(1);
    else
        _rad_alt_warning_node->setIntValue(0);
}

void
RadarAltimeter::update_altitude()
{
    double el_limit    = _Instrument->getDoubleValue("elev-limit", 15);
    double el_step     = _Instrument->getDoubleValue("elev-step-deg", 15);
    double az_limit    = _Instrument->getDoubleValue("az-limit-deg", 15);
    double az_step     = _Instrument->getDoubleValue("az-step-deg", 15);
    double max_range   = _Instrument->getDoubleValue("max-range-m", 1500);
    double min_range   = _Instrument->getDoubleValue("min-range-m", 0.001);


    _min_radalt = max_range;
    bool haveHit = false;
    SGVec3d cartantennapos = getCartAntennaPos();

    for(double brg = -az_limit; brg <= az_limit; brg += az_step){
        for(double elev = el_limit; elev >= - el_limit; elev -= el_step){
            SGVec3d userVec = rayVector(brg, elev);

            SGVec3d nearestHit;
            globals->get_scenery()->get_cart_ground_intersection(cartantennapos, userVec, nearestHit);
            double measuredDistance = dist(cartantennapos, nearestHit);

            if (measuredDistance >= min_range && measuredDistance <= max_range) {                
                if (measuredDistance < _min_radalt) {
                    _min_radalt = measuredDistance;
                    haveHit = true;
                }
            } // of hit within permissible range
        } // of elevation step
    } // of azimuth step
    
    _Instrument->setDoubleValue("radar-altitude-ft", _min_radalt * SG_METER_TO_FEET);
    if (!haveHit) {
        _rad_alt_warning_node->setIntValue(9999);
    }
}

SGVec3d
RadarAltimeter::getCartAntennaPos() const
{
    double yaw, pitch, roll;
    globals->get_aircraft_orientation(yaw, pitch, roll);
    
    // Transform to the right coordinate frame, configuration is done in
    // the x-forward, y-right, z-up coordinates (feet), computation
    // in the simulation usual body x-forward, y-right, z-down coordinates
    // (meters) )
    
    // Transform the user position to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(globals->get_aircraft_position());
    
    // and postrotate the orientation of the user model wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(yaw,pitch,roll);
    
    // The offset converted to the usual body fixed coordinate system
    // rotated to the earth-fixed coordinates axis
    SGVec3d ecfOffset = hlTrans.backTransform(_antennaOffset);
    
    // Add the position offset of the user model to get the geocentered position
    return globals->get_aircraft_position_cart() + ecfOffset;
}

SGVec3d RadarAltimeter::rayVector(double az, double el) const
{
    double yaw, pitch, roll;
    globals->get_aircraft_orientation(yaw, pitch, roll);
    
    double tilt = _Instrument->getDoubleValue("tilt");
    bool roll_stab = false,
        pitch_stab = false;
    
    SGQuatd offset = SGQuatd::fromYawPitchRollDeg(az, el + tilt, 0);
    
    // Transform the antenna position to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(globals->get_aircraft_position());
    
    // and postrotate the orientation of the radar wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(yaw,
                                            pitch_stab ? 0 :pitch,
                                            roll_stab ? 0 : roll);
    hlTrans *= offset;
    
    // now rotate the rotation vector back into the
    // earth centered frames coordinates
    SGVec3d angleaxis(1,0,0);
    return hlTrans.backTransform(angleaxis);
    
}

