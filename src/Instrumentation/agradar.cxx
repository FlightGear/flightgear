// Air Ground Radar
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

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include "agradar.hxx"


agRadar::agRadar(SGPropertyNode *node) : wxRadarBg(node) 
{

    _name = node->getStringValue("name", "air-ground-radar");
    _num = node->getIntValue("number", 0);

}

agRadar::~agRadar ()
{
}

void
agRadar::init ()
{
    _user_hdg_deg_node      = fgGetNode("/orientation/heading-deg", true);
    _user_pitch_deg_node    = fgGetNode("/orientation/pitch-deg", true);
    _user_roll_deg_node     = fgGetNode("/orientation/roll-deg", true);

    _terrain_warning_node   = fgGetNode("/sim/alarms/terrain-warning", true);
    _terrain_warning_node->setBoolValue(false);

    wxRadarBg::init();

    // those properties are used by a radar instrument of a MFD
    // input switch = OFF | TST | STBY | ON
    // input mode = WX | WXA | MAP | TW
    // output status = STBY | TEST | WX | WXA | MAP | blank
    // input lightning = true | false
    // input TRK = +/- n degrees
    // input TILT = +/- n degree
    // input autotilt = true | false
    // input range = n nm (20/40/80)
    // input display-mode = arc | rose | map | plan

    _Instrument->setFloatValue("trk", 0.0);
    _Instrument->setFloatValue("tilt",-2.5);
    _Instrument->setStringValue("status","");
    _Instrument->setIntValue("mode-control", 5); 

    _Instrument->setBoolValue("stabilisation/roll", false);
    _Instrument->setBoolValue("stabilisation/pitch", false);
    
    _xOffsetMNode = getInstrumentNode("antenna/x-offset-m", 0.0);
    _yOffsetMNode = getInstrumentNode("antenna/y-offset-m", 0.0);
    _zOffsetMNode = getInstrumentNode("antenna/z-offset-m", 0.0);

    _elevLimitDegNode = getInstrumentNode("terrain-warning/elev-limit-deg", 2.0);
    _elevStepDegNode = getInstrumentNode("terrain-warning/elev-step-deg", 1.0);
    _azLimitDegNode = getInstrumentNode("terrain-warning/az-limit-deg", 1.0);
    _azStepDegNode = getInstrumentNode("terrain-warning/az-step-deg", 1.5);
    _maxRangeMNode = getInstrumentNode("terrain-warning/max-range-m", 4000.0);
    _minRangeMNode = getInstrumentNode("terrain-warning/min-range-m", 250.0);
    _tiltNode = getInstrumentNode("terrain-warning/tilt", -2.0);

    _brgDegNode = getInstrumentNode("terrain-warning/hit/brg-deg", 0.0);
    _rangeMNode = getInstrumentNode("terrain-warning/hit/range-m", 0.0);
    _elevationMNode = getInstrumentNode("terrain-warning/hit/elevation-m", 0.0);
    _materialNode = getInstrumentNode("terrain-warning/hit/material", "");
    _bumpinessNode = getInstrumentNode("terrain-warning/hit/bumpiness", 0.0);

    _rollStabNode = getInstrumentNode("terrain-warning/stabilisation/roll",
                                      true);
    _pitchStabNode = getInstrumentNode("terrain-warning/stabilisation/pitch",
                                       false);
//    cout << "init done" << endl;

}

void
agRadar::update (double delta_time_sec)
{
    if (!_sceneryLoaded->getBoolValue())
        return;

    if ( !_odg || ! _serviceable_node->getBoolValue() ) {
        _Instrument->setStringValue("status","");
        return;
    }

    _time += delta_time_sec;

    if (_time < _interval)
        return;

    _time = 0.0;

    update_terrain();
//    wxRadarBg::update(delta_time_sec);
}

void
agRadar::setUserPos()
{
    userpos.setLatitudeDeg(_user_lat_node->getDoubleValue());
    userpos.setLongitudeDeg(_user_lon_node->getDoubleValue());
    userpos.setElevationM(_user_alt_node->getDoubleValue() * SG_FEET_TO_METER);
}

SGVec3d
agRadar::getCartUserPos() const {
    SGVec3d cartUserPos = SGVec3d::fromGeod(userpos);
    return cartUserPos;
}

SGVec3d
agRadar::getCartAntennaPos() const {

    float yaw   = _user_hdg_deg_node->getDoubleValue();
    float pitch = _user_pitch_deg_node->getDoubleValue();
    float roll  = _user_roll_deg_node->getDoubleValue();

    double x_offset_m =_xOffsetMNode->getDoubleValue();
    double y_offset_m =_yOffsetMNode->getDoubleValue();
    double z_offset_m =_zOffsetMNode->getDoubleValue();

    // convert geodetic positions to geocentered
    SGVec3d cartuserPos = getCartUserPos();

    // Transform to the right coordinate frame, configuration is done in
    // the x-forward, y-right, z-up coordinates (feet), computation
    // in the simulation usual body x-forward, y-right, z-down coordinates
    // (meters) )
    SGVec3d _off(x_offset_m, y_offset_m, -z_offset_m);

    // Transform the user position to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(userpos);

    // and postrotate the orientation of the user model wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(yaw,pitch,roll);

    // The offset converted to the usual body fixed coordinate system
    // rotated to the earth-fixed coordinates axis
    SGVec3d off = hlTrans.backTransform(_off);

    // Add the position offset of the user model to get the geocentered position
    SGVec3d offsetPos = cartuserPos + off;

    return offsetPos;
}

void
agRadar::setAntennaPos() {
    SGGeodesy::SGCartToGeod(getCartAntennaPos(), antennapos);
}

void
agRadar::setUserVec(double az, double el)
{
    float yaw   = _user_hdg_deg_node->getDoubleValue();
    float pitch = _user_pitch_deg_node->getDoubleValue();
    float roll  = _user_roll_deg_node->getDoubleValue();
    double tilt = _Instrument->getDoubleValue("tilt");
    double trk  = _Instrument->getDoubleValue("trk");
    bool roll_stab   = _Instrument->getBoolValue("stabilisation/roll");
    bool pitch_stab  = _Instrument->getBoolValue("stabilisation/pitch");

    SGQuatd offset = SGQuatd::fromYawPitchRollDeg(az + trk, el + tilt, 0);

    // Transform the antenna position to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(antennapos);

    // and postrotate the orientation of the radar wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(yaw,
                                            pitch_stab ? 0 :pitch,
                                            roll_stab ? 0 : roll);
    hlTrans *= offset;

    // now rotate the rotation vector back into the
    // earth centered frames coordinates
    SGVec3d angleaxis(1,0,0);
    uservec = hlTrans.backTransform(angleaxis);

}

bool
agRadar::getMaterial(){

    if (globals->get_scenery()->get_elevation_m(hitpos, _elevation_m, &_material)){
        //_ht_agl_ft = pos.getElevationFt() - _elevation_m * SG_METER_TO_FEET;
        if (_material) {
            const std::vector<std::string>& names = _material->get_names();

            _solid = _material->get_solid();
            _load_resistance = _material->get_load_resistance();
            _frictionFactor =_material->get_friction_factor();
            _bumpinessFactor = _material->get_bumpiness();

            if (!names.empty()) 
                _mat_name = names[0].c_str();
            else
                _mat_name = "";

        }
        /*cout << "material " << mat_name 
        << " solid " << _solid 
        << " load " << _load_resistance
        << " frictionFactor " << frictionFactor 
        << " _bumpinessFactor " << _bumpinessFactor
        << endl;*/
        return true;
    } else {
        return false;
    }

}

void
agRadar::update_terrain()
{
    int mode = _radar_mode_control_node->getIntValue();

    double el_limit = 1;
    double el_step = 1;
    double az_limit = 50;
    double az_step = 10;
    double max_range = 40000;
    double min_range = 250;
    double tilt = -2.5;
    bool roll_stab   = _rollStabNode->getBoolValue();
    bool pitch_stab  = _pitchStabNode->getBoolValue();
    const char* status = "";
    bool hdg_mkr = true;

    if (mode == 5){
        status = "TW";
        hdg_mkr = false;
        tilt        = _tiltNode->getDoubleValue();
        el_limit    = _elevLimitDegNode->getDoubleValue();
        el_step     = _elevStepDegNode->getDoubleValue();
        az_limit    = _azLimitDegNode->getDoubleValue();
        az_step     = _azStepDegNode->getDoubleValue();
        max_range   = _maxRangeMNode->getDoubleValue();
        min_range   = _minRangeMNode->getDoubleValue();
    }

    _Instrument->setDoubleValue("tilt", tilt);
    _Instrument->setBoolValue("stabilisation/roll", roll_stab);
    _Instrument->setBoolValue("stabilisation/pitch", pitch_stab);
    _Instrument->setStringValue("status", status);
    _Instrument->setDoubleValue("limit-deg", az_limit);
    _Instrument->setBoolValue("heading-marker", hdg_mkr);
    setUserPos();
    setAntennaPos();
    SGVec3d cartantennapos = getCartAntennaPos();
    
    for(double brg = -az_limit; brg <= az_limit; brg += az_step){
        for(double elev = el_limit; elev >= - el_limit; elev -= el_step){
            setUserVec(brg, elev);
            SGVec3d nearestHit;
            globals->get_scenery()->get_cart_ground_intersection(cartantennapos, uservec, nearestHit);
            SGGeodesy::SGCartToGeod(nearestHit, hitpos);

            double course1, course2, distance;

            SGGeodesy::inverse(hitpos, antennapos, course1, course2, distance);

            if (distance >= min_range && distance <= max_range) {
                _terrain_warning_node->setBoolValue(true);
                getMaterial();
                _brgDegNode->setDoubleValue(course2);
                _rangeMNode->setDoubleValue(distance);
                _materialNode->setStringValue(_mat_name.c_str());
                _bumpinessNode->setDoubleValue(_bumpinessFactor);
                _elevationMNode->setDoubleValue(_elevation_m);
            } else {
                _terrain_warning_node->setBoolValue(false);
                _brgDegNode->setDoubleValue(0);
                _rangeMNode->setDoubleValue(0);
                _materialNode->setStringValue("");
                _bumpinessNode->setDoubleValue(0);
                _elevationMNode->setDoubleValue(0);
            }

            //cout  << "usr hdg " << _user_hdg_deg_node->getDoubleValue()
            //    << " ant brg " << course2 
            //    << " elev " << _Instrument->getDoubleValue("tilt")
            //    << " gnd rng nm " << distance * SG_METER_TO_NM
            //    << " ht " << hitpos.getElevationFt()
            //    << " mat " << _mat_name
            //    << " solid " << _solid
            //    << " bumpiness " << _bumpinessFactor
            //    << endl;

        }
    }
}


