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

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include "rad_alt.hxx"


radAlt::radAlt(SGPropertyNode *node) : agRadar(node) 
{

    _name = node->getStringValue("name", "radar-altimeter");
    _num = node->getIntValue("number", 0);

}

radAlt::~radAlt()
{
}

void
radAlt::init ()
{
    agRadar::init();

    _user_alt_agl_node     = fgGetNode("/position/altitude-agl-ft", true);
    _rad_alt_warning_node   = fgGetNode("/sim/alarms/rad-alt-warning", true);

    //// those properties are used by a radar instrument of a MFD
    //// input switch = OFF | TST | STBY | ON
    //// input mode = WX | WXA | MAP | TW
    //// output status = STBY | TEST | WX | WXA | MAP | blank
    //// input lightning = true | false
    //// input TRK = +/- n degrees
    //// input TILT = +/- n degree
    //// input autotilt = true | false
    //// input range = n nm (20/40/80)
    //// input display-mode = arc | rose | map | plan

    _Instrument->setFloatValue("tilt",-85);
    _Instrument->setStringValue("status","RA");

    //_Instrument->setIntValue("mode-control", 10);

    _Instrument->getDoubleValue("elev-limit", true);
    _Instrument->getDoubleValue("elev-step-deg", true);
    _Instrument->getDoubleValue("az-limit-deg", true);
    _Instrument->getDoubleValue("az-step-deg", true);
    _Instrument->getDoubleValue("max-range-m", true);
    _Instrument->getDoubleValue("min-range-m", true);
    _Instrument->getDoubleValue("tilt", true);
    _Instrument->getDoubleValue("set-height-ft", true);
    _Instrument->getDoubleValue("set-excursion-percent", true);

    _Instrument->setDoubleValue("hit/brg-deg", 0);
    _Instrument->setDoubleValue("hit/range-m", 0);
    _Instrument->setStringValue("hit/material", "");
    _Instrument->setDoubleValue("hit/bumpiness", 0);



    _Instrument->removeChild("terrain-warning");
    _Instrument->removeChild("mode-control");
    _Instrument->removeChild("limit-deg");
    _Instrument->removeChild("reference-range-nm");
    _Instrument->removeChild("heading-marker");
    _Instrument->removeChild("display-controls");
    _Instrument->removeChild("font");

    //cout << " radar alt init done" << endl;
}

void
radAlt::update (double delta_time_sec)
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

    update_altitude();
}

double
radAlt::getDistanceAntennaToHit(SGVec3d nearestHit) const 
{
    //calculate the distance antenna to hit

    SGVec3d cartantennapos = getCartAntennaPos();;

    SGVec3d diff = nearestHit - cartantennapos;
    double distance = norm(diff);
    return distance ;
}

void
radAlt::update_altitude()
{
//    int mode           = _radar_mode_control_node->getIntValue();
//    double tilt        = _Instrument->getDoubleValue("tilt", -85);
    double el_limit    = _Instrument->getDoubleValue("elev-limit", 15);
    double el_step     = _Instrument->getDoubleValue("elev-step-deg", 15);
    double az_limit    = _Instrument->getDoubleValue("az-limit-deg", 15);
    double az_step     = _Instrument->getDoubleValue("az-step-deg", 15);
    double max_range   = _Instrument->getDoubleValue("max-range-m", 1500);
    double min_range   = _Instrument->getDoubleValue("min-range-m", 0.001);
    double set_ht_ft   = _Instrument->getDoubleValue("set-height-ft", 9999);
    double set_excur   = _Instrument->getDoubleValue("set-excursion-percent", 0);

    _min_radalt = max_range;

    setUserPos();
    setAntennaPos();
    SGVec3d cartantennapos = getCartAntennaPos();

    for(double brg = -az_limit; brg <= az_limit; brg += az_step){
        for(double elev = el_limit; elev >= - el_limit; elev -= el_step){
            setUserVec(brg, elev);

            SGVec3d nearestHit;
            globals->get_scenery()->get_cart_ground_intersection(cartantennapos, uservec, nearestHit);
            SGGeodesy::SGCartToGeod(nearestHit, hitpos);

            double radalt = getDistanceAntennaToHit(nearestHit);
            double course1, course2, distance;

            SGGeodesy::inverse(hitpos, antennapos, course1, course2, distance);
            _Instrument->setDoubleValue("hit/altitude-agl-ft",
                     _user_alt_agl_node->getDoubleValue());

            if (radalt >= min_range && radalt <= max_range) {
                getMaterial();
                
                if (radalt < _min_radalt)
                    _min_radalt = radalt;

                _Instrument->setDoubleValue("radar-altitude-ft", _min_radalt * SG_METER_TO_FEET);
                _Instrument->setDoubleValue("hit/radar-altitude-ft", radalt * SG_METER_TO_FEET);
                _Instrument->setDoubleValue("hit/brg-deg", course2);
                _Instrument->setDoubleValue("hit/range-m", distance);
                _Instrument->setStringValue("hit/material", _mat_name.c_str());
                _Instrument->setDoubleValue("hit/bumpiness", _bumpinessFactor);

                if (set_ht_ft!= 9999){

                    if (_min_radalt * SG_METER_TO_FEET < set_ht_ft * (100 - set_excur)/100)
                        _rad_alt_warning_node->setIntValue(-1);
                    else if (_min_radalt * SG_METER_TO_FEET > set_ht_ft * (100 + set_excur)/100)
                        _rad_alt_warning_node->setIntValue(1);
                    else
                        _rad_alt_warning_node->setIntValue(0);

                } else
                    _rad_alt_warning_node->setIntValue(9999);

            } else {
                _rad_alt_warning_node->setIntValue(9999);
                _Instrument->setDoubleValue("radar-altitude-ft", _min_radalt * SG_METER_TO_FEET);
                _Instrument->setDoubleValue("hit/radar-altitude-ft",0);
                _Instrument->setDoubleValue("hit/brg-deg", 0);
                _Instrument->setDoubleValue("hit/range-m", 0);
                _Instrument->setStringValue("hit/material", "");
                _Instrument->setDoubleValue("hit/bumpiness", 0);
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


