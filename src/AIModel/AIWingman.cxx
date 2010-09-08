// FGAIWingman - FGAIBllistic-derived class creates an AI Wingman
//
// Written by Vivian Meazza, started February 2008.
// - vivian.meazza at lineone.net 
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "AIWingman.hxx"

FGAIWingman::FGAIWingman() : FGAIBallistic(otWingman)
{
    invisible = false;
    _formate_to_ac = true;

}

FGAIWingman::~FGAIWingman() {}

void FGAIWingman::readFromScenario(SGPropertyNode* scFileNode) {
    if (!scFileNode)
        return;

    FGAIBase::readFromScenario(scFileNode);

    setAzimuth(scFileNode->getDoubleValue("azimuth", 0.0));
    setElevation(scFileNode->getDoubleValue("elevation", 0.0));
    setLife(scFileNode->getDoubleValue("life", -1));
    setNoRoll(scFileNode->getBoolValue("no-roll", false));
    setName(scFileNode->getStringValue("name", "Wingman"));
    setParentName(scFileNode->getStringValue("parent", ""));
    //setSMPath(scFileNode->getStringValue("submodel-path", ""));
    setSubID(scFileNode->getIntValue("SubID", 0));
    setXoffset(scFileNode->getDoubleValue("x-offset", 0.0));
    setYoffset(scFileNode->getDoubleValue("y-offset", 0.0));
    setZoffset(scFileNode->getDoubleValue("z-offset", 0.0));
    setPitchoffset(scFileNode->getDoubleValue("pitch-offset", 0.0));
    setRolloffset(scFileNode->getDoubleValue("roll-offset", 0.0));
    setYawoffset(scFileNode->getDoubleValue("yaw-offset", 0.0));
    setGroundOffset(scFileNode->getDoubleValue("ground-offset", 0.0));
    setFormate(scFileNode->getBoolValue("formate", true));
}

void FGAIWingman::bind() {
    FGAIBallistic::bind();

    props->tie("id", SGRawValueMethods<FGAIBase,int>(*this,
        &FGAIBase::getID));
    props->tie("subID", SGRawValueMethods<FGAIBase,int>(*this,
        &FGAIBase::_getSubID));
    props->tie("position/altitude-ft",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getElevationFt,
        &FGAIBase::_setAltitude));
    props->tie("position/latitude-deg",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getLatitude,
        &FGAIBase::_setLatitude));
    props->tie("position/longitude-deg",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getLongitude,
        &FGAIBase::_setLongitude));

    props->tie("controls/formate-to-ac",
        SGRawValueMethods<FGAIBallistic,bool>
        (*this, &FGAIBallistic::getFormate, &FGAIBallistic::setFormate));


    props->tie("orientation/pitch-deg",   SGRawValuePointer<double>(&pitch));
    props->tie("orientation/roll-deg",    SGRawValuePointer<double>(&roll));
    props->tie("orientation/true-heading-deg", SGRawValuePointer<double>(&hdg));

    props->tie("submodels/serviceable", SGRawValuePointer<bool>(&serviceable));

    props->tie("load/rel-brg-to-user-deg",
        SGRawValueMethods<FGAIBallistic,double>
        (*this, &FGAIBallistic::getRelBrgHitchToUser));
    props->tie("load/elev-to-user-deg",
        SGRawValueMethods<FGAIBallistic,double>
        (*this, &FGAIBallistic::getElevHitchToUser));

    props->tie("velocities/vertical-speed-fps",
        SGRawValuePointer<double>(&vs));
    props->tie("velocities/true-airspeed-kt",
        SGRawValuePointer<double>(&speed));
    props->tie("velocities/speed-east-fps",
        SGRawValuePointer<double>(&_speed_east_fps));
    props->tie("velocities/speed-north-fps",
        SGRawValuePointer<double>(&_speed_north_fps));


    props->tie("position/x-offset", 
        SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getXOffset, &FGAIBase::setXoffset));
    props->tie("position/y-offset", 
        SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getYOffset, &FGAIBase::setYoffset));
    props->tie("position/z-offset", 
        SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getZOffset, &FGAIBase::setZoffset));
    props->tie("position/tgt-x-offset", 
        SGRawValueMethods<FGAIBallistic,double>(*this, &FGAIBallistic::getTgtXOffset, &FGAIBallistic::setTgtXOffset));
    props->tie("position/tgt-y-offset", 
        SGRawValueMethods<FGAIBallistic,double>(*this, &FGAIBallistic::getTgtYOffset, &FGAIBallistic::setTgtYOffset));
    props->tie("position/tgt-z-offset", 
        SGRawValueMethods<FGAIBallistic,double>(*this, &FGAIBallistic::getTgtZOffset, &FGAIBallistic::setTgtZOffset));
}

void FGAIWingman::unbind() {
    FGAIBallistic::unbind();

    props->untie("id");
    props->untie("SubID");

    props->untie("orientation/pitch-deg");
    props->untie("orientation/roll-deg");
    props->untie("orientation/true-heading-deg");

    props->untie("controls/formate-to-ac");

    props->untie("submodels/serviceable");

    props->untie("velocities/true-airspeed-kt");
    props->untie("velocities/vertical-speed-fps");
    props->untie("velocities/speed_east_fps");
    props->untie("velocities/speed_north_fps");

    props->untie("load/rel-brg-to-user-deg");
    props->untie("load/elev-to-user-deg");

    props->untie("position/altitude-ft");
    props->untie("position/latitude-deg");
    props->untie("position/longitude-deg");
    props->untie("position/x-offset");
    props->untie("position/y-offset");
    props->untie("position/z-offset");
    props->untie("position/tgt-x-offset");
    props->untie("position/tgt-y-offset");
    props->untie("position/tgt-z-offset");
}

bool FGAIWingman::init(bool search_in_AI_path) {
    if (!FGAIBallistic::init(search_in_AI_path))
        return false;

    invisible = false;

    _tgt_x_offset = _x_offset;
    _tgt_y_offset = _y_offset;
    _tgt_z_offset = _z_offset;

    hdg = _azimuth;
    pitch = _elevation;
    roll = _rotation;
    _ht_agl_ft = 1e10;

    props->setStringValue("submodels/path", _path.c_str());
    return true;
}

void FGAIWingman::update(double dt) {
    FGAIBallistic::update(dt);
}

// end AIWingman
