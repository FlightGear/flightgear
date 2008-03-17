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
    
    _Instrument->getNode("antenna/x-offset-m", true);
    _Instrument->getNode("antenna/y-offset-m", true);
    _Instrument->getNode("antenna/z-offset-m", true);

    _Instrument->getNode("terrain-warning/elev-limit-deg", true);
    _Instrument->getNode("terrain-warning/elev-step-deg", true);
    _Instrument->getNode("terrain-warning/az-limit-deg", true);
    _Instrument->getNode("terrain-warning/az-step-deg", true);
    _Instrument->getNode("terrain-warning/max-range-m", true);
    _Instrument->getNode("terrain-warning/min-range-m", true);
    _Instrument->getNode("terrain-warning/tilt",true);

    _Instrument->getNode("terrain-warning/hit/brg-deg", true);
    _Instrument->getNode("terrain-warning/hit/range-m", true);
    _Instrument->getNode("terrain-warning/hit/material", true);
    _Instrument->getNode("terrain-warning/hit/bumpiness", true);

    _Instrument->getNode("terrain-warning/stabilisation/roll", true);
    _Instrument->getNode("terrain-warning/stabilisation/pitch", true);
//    cout << "init done" << endl;

}

void
agRadar::update (double delta_time_sec)
{
    if ( ! _sim_init_done ) {

        if ( ! fgGetBool("sim/sceneryloaded", false) )
            return;

        _sim_init_done = true;
    }

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

    double x_offset_m =_Instrument->getDoubleValue("antenna/x-offset-m", 0);
    double y_offset_m =_Instrument->getDoubleValue("antenna/y-offset-m", 0);
    double z_offset_m =_Instrument->getDoubleValue("antenna/y-offset-m", 0);

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
agRadar::setUserVec(int az, double el)
{
    float yaw   = _user_hdg_deg_node->getDoubleValue();
    float pitch = _user_pitch_deg_node->getDoubleValue();
    float roll  = _user_roll_deg_node->getDoubleValue();
    int mode    = _radar_mode_control_node->getIntValue();
    double tilt = _Instrument->getDoubleValue("tilt");
    double trk  = _Instrument->getDoubleValue("trk");
    bool roll_stab   = _Instrument->getBoolValue("stabilisation/roll");
    bool pitch_stab  = _Instrument->getBoolValue("stabilisation/pitch");

    SGQuatd offset = SGQuatd::fromYawPitchRollDeg(az + trk, el + tilt, 0);

    // Transform the antenna position to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(antennapos);

    // and postrotate the orientation of the radar wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(yaw, pitch, roll);
    hlTrans *= offset;

    if (roll_stab)
        hlTrans *= SGQuatd::fromYawPitchRollDeg(0, 0, -roll);  

    if (pitch_stab)
        hlTrans *= SGQuatd::fromYawPitchRollDeg(0, -pitch, 0);

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
            const vector<string>& names = _material->get_names();

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
    bool roll_stab   = _Instrument->getBoolValue("stabilisation/roll");
    bool pitch_stab  = _Instrument->getBoolValue("stabilisation/pitch");
    //string status = "";
    const char* status;
    bool hdg_mkr = true;

    if (mode == 5){
        status = "TW";
        hdg_mkr = false;
        roll_stab   = _Instrument->getBoolValue("terrain-warning/stabilisation/roll", true);
        pitch_stab  = _Instrument->getBoolValue("terrain-warning/stabilisation/pitch", false);
        tilt        = _Instrument->getDoubleValue("terrain-warning/tilt", -2);
        el_limit    = _Instrument->getDoubleValue("terrain-warning/elev-limit-deg", 2);
        el_step     = _Instrument->getDoubleValue("terrain-warning/elev-step-deg", 1);
        az_limit    = _Instrument->getDoubleValue("terrain-warning/az-limit-deg", 1);
        az_step     = _Instrument->getDoubleValue("terrain-warning/az-step-deg", 1.5);
        max_range   = _Instrument->getDoubleValue("terrain-warning/max-range-m", 4000);
        min_range   = _Instrument->getDoubleValue("terrain-warning/min-range-m", 250);
    }

    _Instrument->setDoubleValue("tilt", tilt);
    _Instrument->setBoolValue("stabilisation/roll", roll_stab);
    _Instrument->setBoolValue("stabilisation/pitch", pitch_stab);
    _Instrument->setStringValue("status", status);
    _Instrument->setDoubleValue("limit-deg", az_limit);
    _Instrument->setBoolValue("heading-marker", hdg_mkr);

    for(double brg = -az_limit; brg <= az_limit; brg += az_step){
        setUserPos();
        setAntennaPos();
        SGVec3d cartantennapos = getCartAntennaPos();

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
                _Instrument->setDoubleValue("terrain-warning/hit/brg-deg", course2);
                _Instrument->setDoubleValue("terrain-warning/hit/range-m", distance);
                _Instrument->setStringValue("terrain-warning/hit/material", _mat_name.c_str());
                _Instrument->setDoubleValue("terrain-warning/hit/bumpiness", _bumpinessFactor);
                _Instrument->setDoubleValue("terrain-warning/hit/elevation-m", _elevation_m);
            } else {
                _terrain_warning_node->setBoolValue(false);
                _Instrument->setDoubleValue("terrain-warning/hit/brg-deg", 0);
                _Instrument->setDoubleValue("terrain-warning/hit/range-m", 0);
                _Instrument->setStringValue("terrain-warning/hit/material", "");
                _Instrument->setDoubleValue("terrain-warning/hit/bumpiness", 0);
                _Instrument->setDoubleValue("terrain-warning/hit/elevation-m",0);
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


