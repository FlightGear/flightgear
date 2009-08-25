// FGAIGroundVehicle - FGAIShip-derived class creates an AI Ground Vehicle
// by adding a ground following utility
//
// Written by Vivian Meazza, started August 2009.
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

#include <simgear/sg_inlines.h>

#include <Main/viewer.hxx>
#include <Scenery/scenery.hxx>
#include <Scenery/tilemgr.hxx>
#include <Airports/dynamics.hxx>

#include "AIGroundVehicle.hxx"

FGAIGroundVehicle::FGAIGroundVehicle() :
FGAIShip(otGroundVehicle),

_pitch(0),
_pitch_deg(0),
_speed_kt(0),
_range_ft(0),
_relbrg (0),
_parent_speed(0),
_parent_x_offset(0),
_selected_ac(0)
{
    invisible = false;
}

FGAIGroundVehicle::~FGAIGroundVehicle() {}

void FGAIGroundVehicle::readFromScenario(SGPropertyNode* scFileNode) {
    if (!scFileNode)
        return;

    FGAIShip::readFromScenario(scFileNode);

    setNoRoll(scFileNode->getBoolValue("no-roll", true));
    setName(scFileNode->getStringValue("name", "groundvehicle"));
    setSMPath(scFileNode->getStringValue("submodel-path", ""));
    setContactX1offset(scFileNode->getDoubleValue("contact_x1_offset", 0.0));
    setContactX2offset(scFileNode->getDoubleValue("contact_x2_offset", 0.0));
    setXOffset(scFileNode->getDoubleValue("hitch-x-offset", 38.55));
    setPitchoffset(scFileNode->getDoubleValue("pitch-offset", 0.0));
    setRolloffset(scFileNode->getDoubleValue("roll-offset", 0.0));
    setYawoffset(scFileNode->getDoubleValue("yaw-offset", 0.0));
    setPitchCoeff(scFileNode->getDoubleValue("pitch-coefficient", 0.5));
    setElevCoeff(scFileNode->getDoubleValue("elevation-coefficient", 0.5));
    setParentName(scFileNode->getStringValue("parent", ""));
    setTowAngleGain(scFileNode->getDoubleValue("tow-angle-gain", 2.0));
    setTowAngleLimit(scFileNode->getDoubleValue("tow-angle-limit-deg", 2.0));
    //we may need these later for towed vehicles
    //    setSubID(scFileNode->getIntValue("SubID", 0));
    //    setGroundOffset(scFileNode->getDoubleValue("ground-offset", 0.0));
    //    setFormate(scFileNode->getBoolValue("formate", true));
}

void FGAIGroundVehicle::bind() {
    FGAIShip::bind();

    props->tie("controls/constants/elevation-coeff",
        SGRawValuePointer<double>(&_elevation_coeff));
    props->tie("controls/constants/pitch-coeff",
        SGRawValuePointer<double>(&_pitch_coeff));
    props->tie("position/ht-AGL-ft",
        SGRawValuePointer<double>(&_ht_agl_ft));
    props->tie("hitch/rel-bearing-deg",
         SGRawValuePointer<double>(&_relbrg));
    props->tie("hitch/tow-angle-deg",
         SGRawValuePointer<double>(&_tow_angle));
    props->tie("hitch/range-ft",
        SGRawValuePointer<double>(&_range_ft));
    props->tie("hitch/x-offset-ft",
        SGRawValuePointer<double>(&_x_offset));
    props->tie("hitch/parent-x-offset-ft",
        SGRawValuePointer<double>(&_parent_x_offset));
    props->tie("controls/constants/tow-angle/gain",
        SGRawValuePointer<double>(&_tow_angle_gain));
    props->tie("controls/constants/tow-angle/limit-deg",
        SGRawValuePointer<double>(&_tow_angle_limit));


    //we may need these later for towed vehicles

    //    (*this, &FGAIBallistic::getElevHitchToUser));
    //props->tie("position/x-offset",
    //    SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getXOffset, &FGAIBase::setXoffset));
    //props->tie("position/y-offset",
    //    SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getYOffset, &FGAIBase::setYoffset));
    //props->tie("position/z-offset",
    //    SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getZOffset, &FGAIBase::setZoffset));
    //props->tie("position/tgt-x-offset",
    //    SGRawValueMethods<FGAIWingman,double>(*this, &FGAIWingman::getTgtXOffset, &FGAIWingman::setTgtXOffset));
    //props->tie("position/tgt-y-offset",
    //    SGRawValueMethods<FGAIWingman,double>(*this, &FGAIWingman::getTgtYOffset, &FGAIWingman::setTgtYOffset));
    //props->tie("position/tgt-z-offset",
    //    SGRawValueMethods<FGAIWingman,double>(*this, &FGAIWingman::getTgtZOffset, &FGAIWingman::setTgtZOffset));
}

void FGAIGroundVehicle::unbind() {
    FGAIShip::unbind();

    props->untie("controls/constants/elevation-coeff");
    props->untie("position/ht-AGL-ft");
    props->untie("controls/constants/pitch-coeff");
    props->untie("hitch/rel-bearing-deg");
    props->untie("hitch/tow-angle-deg");
    props->untie("hitch/range-ft");
    props->untie("hitch/x-offset-ft");
    props->untie("hitch/parent-x-offset-ft");
    props->untie("controls/constants/tow-angle/gain");
    props->untie("controls/constants/tow-angle/limit-deg");

    //we may need these later for towed vehicles
    //props->untie("load/rel-brg-to-user-deg");
    //props->untie("load/elev-to-user-deg");
    //props->untie("velocities/vertical-speed-fps");
    //props->untie("position/x-offset");
    //props->untie("position/y-offset");
    //props->untie("position/z-offset");
    //props->untie("position/tgt-x-offset");
    //props->untie("position/tgt-y-offset");
    //props->untie("position/tgt-z-offset");
}

bool FGAIGroundVehicle::init(bool search_in_AI_path) {
    if (!FGAIShip::init(search_in_AI_path))
        return false;

    invisible = false;

    _limit = 200;
    no_roll = true;

    return true;
}

void FGAIGroundVehicle::update(double dt) {
    //    SG_LOG(SG_GENERAL, SG_ALERT, "updating GroundVehicle: " << _name );

    if (getPitch()){
        setElevation(_elevation, dt, _elevation_coeff);
        ClimbTo(_elevation_ft);
        setPitch(_pitch, dt, _pitch_coeff);
        PitchTo(_pitch_deg);
    }

    if(_parent !=""){
        setParent();

        string parent_next_name = _selected_ac->getStringValue("waypoint/name-next");
        bool parent_waiting = _selected_ac->getBoolValue("waypoint/waiting");
        _parent_speed = _selected_ac->getDoubleValue("velocities/true-airspeed-kt");

        if (parent_next_name == "END" && fp->getNextWaypoint()->name != "END" ){
            SG_LOG(SG_GENERAL, SG_DEBUG, "AIGroundVeh1cle: " << _name
                << " setting END: getting new waypoints ");
            AdvanceFP();
            setWPNames();
        /*} else if (parent_next_name == "WAIT" && fp->getNextWaypoint()->name != "WAIT" ){*/
        } else if (parent_waiting && !_waiting){
            SG_LOG(SG_GENERAL, SG_DEBUG, "AIGroundVeh1cle: " << _name
                << " setting WAIT/WAITUNTIL: getting new waypoints ");
            AdvanceFP();
            setWPNames();
            _waiting = true;
        } else if (parent_next_name != "WAIT" && fp->getNextWaypoint()->name == "WAIT"){
            SG_LOG(SG_GENERAL, SG_DEBUG, "AIGroundVeh1cle: " << _name
                << " wait done: getting new waypoints ");
            _waiting = false;
            _wait_count = 0;
            fp->IncrementWaypoint(false);
            next = fp->getNextWaypoint();

            if (next->name == "WAITUNTIL" || next->name == "WAIT"
                || next->name == "END"){
            } else {
                prev = curr;
                fp->IncrementWaypoint(false);
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
            }

            setWPNames();
        } else if (_range_ft > 1000){

             SG_LOG(SG_GENERAL, SG_INFO, "AIGroundVeh1cle: " << _name
                << " rescue: reforming train " << _range_ft << " " << _x_offset * 15);

            setTowAngle(0, dt, 1);
            setSpeed(_parent_speed * 2);

        } else if (_parent_speed > 1){

            setTowSpeed();
            setTowAngle(_relbrg, dt, 1);

        } else if (_parent_speed < -1){

            setTowSpeed();

            if (_relbrg < 0)
                setTowAngle(-(180 - (360 + _relbrg)), dt, 1);
            else
                setTowAngle(-(180 - _relbrg), dt, 1);

        } else
            setSpeed(_parent_speed);
    }

    FGAIShip::update(dt);
}

void FGAIGroundVehicle::setNoRoll(bool nr) {
    no_roll = nr;
}

void FGAIGroundVehicle::setContactX1offset(double x1) {
    _contact_x1_offset = x1;
}

void FGAIGroundVehicle::setContactX2offset(double x2) {
    _contact_x2_offset = x2;
}

void FGAIGroundVehicle::setXOffset(double x) {
    _x_offset = x;
}

void FGAIGroundVehicle::setPitchCoeff(double pc) {
    _pitch_coeff = pc;
}

void FGAIGroundVehicle::setElevCoeff(double ec) {
    _elevation_coeff = ec;
}

void FGAIGroundVehicle::setTowAngleGain(double g) {
    _tow_angle_gain = g;
}

void FGAIGroundVehicle::setTowAngleLimit(double l) {
    _tow_angle_limit = l;
}

void FGAIGroundVehicle::setElevation(double h, double dt, double coeff){
    double c = dt / (coeff + dt);
    _elevation_ft = (h * c) + (_elevation_ft * (1 - c));
}

void FGAIGroundVehicle::setPitch(double p, double dt, double coeff){
    double c = dt / (coeff + dt);
    _pitch_deg = (p * c) + (_pitch_deg * (1 - c));
}

void FGAIGroundVehicle::setParentName(const string& p) {
    _parent = p;
}

void FGAIGroundVehicle::setTowAngle(double ta, double dt, double coeff){
    ta *= _tow_angle_gain;
    //_tow_angle = pow(ta,2) * sign(ta);
    //if (_tow_angle > _tow_angle_limit) _tow_angle = _tow_angle_limit;
    //if (_tow_angle < -_tow_angle_limit) _tow_angle = -_tow_angle_limit;
    SG_CLAMP_RANGE(_tow_angle, -_tow_angle_limit, _tow_angle_limit);
}

bool FGAIGroundVehicle::getGroundElev(SGGeod inpos) {

    if (globals->get_scenery()->get_elevation_m(SGGeod::fromGeodM(inpos, 10000),
        _elevation_m, &_material)){
            _ht_agl_ft = pos.getElevationFt() - _elevation_m * SG_METER_TO_FEET;

            if (_material) {
                const vector<string>& names = _material->get_names();

                _solid = _material->get_solid();
                _load_resistance = _material->get_load_resistance();
                _frictionFactor =_material->get_friction_factor();
                _elevation = _elevation_m * SG_METER_TO_FEET;

                if (!names.empty())
                    props->setStringValue("material/name", names[0].c_str());
                else
                    props->setStringValue("material/name", "");

                //cout << "material " << names[0].c_str()
                //    << " _elevation_m " << _elevation_m
                //    << " solid " << _solid
                //    << " load " << _load_resistance
                //    << " frictionFactor " << _frictionFactor
                //    << endl;

            }

            return true;
    } else {
        return false;
    }

}

bool FGAIGroundVehicle::getPitch() {

    double vel = props->getDoubleValue("velocities/true-airspeed-kt", 0);
    double contact_offset_x1_m = _contact_x1_offset * SG_FEET_TO_METER;
    double contact_offset_x2_m = _contact_x2_offset * SG_FEET_TO_METER;

    SGVec3d front(-contact_offset_x1_m, 0, 0);
    SGVec3d rear(-contact_offset_x2_m, 0, 0);
    SGVec3d Front = getCartPosAt(front);
    SGVec3d Rear = getCartPosAt(rear);

    SGGeod geodFront = SGGeod::fromCart(Front);
    SGGeod geodRear = SGGeod::fromCart(Rear);

    double front_elev_m = 0;
    double rear_elev_m = 0;
    double elev_front = 0;
    double elev_rear = 0;

    if (globals->get_scenery()->get_elevation_m(SGGeod::fromGeodM(geodFront, 10000),
        elev_front, &_material)){

            front_elev_m = elev_front;

            //if (_material) {
            //
            //}

    } else
        return false;

    if (globals->get_scenery()->get_elevation_m(SGGeod::fromGeodM(geodRear, 10000),
        elev_rear, &_material)){
            rear_elev_m = elev_rear;
            //if (_material) {
            //    rear_elev_m = elev_rear;
            //}

    } else
        return false;

    if (vel >= 0){
        double diff = front_elev_m - rear_elev_m;
        _pitch = atan2 (diff,
            fabs(contact_offset_x1_m) + fabs(contact_offset_x2_m)) * SG_RADIANS_TO_DEGREES;
        _elevation = (rear_elev_m + diff/2) * SG_METER_TO_FEET;
    } else {
        double diff = rear_elev_m - front_elev_m;
        _pitch = atan2 (diff,
            fabs(contact_offset_x1_m) + fabs(contact_offset_x2_m)) * SG_RADIANS_TO_DEGREES;
        _elevation = (front_elev_m + diff/2) * SG_METER_TO_FEET;
        _pitch = -_pitch;
    }

    return true;
}

void FGAIGroundVehicle::setParent() {

    const SGPropertyNode *ai = fgGetNode("/ai/models", true);

    for (int i = ai->nChildren() - 1; i >= -1; i--) {
        const SGPropertyNode *model;

        if (i < 0) { // last iteration: selected model
            model = _selected_ac;
        } else {
            model = ai->getChild(i);
            const string name = model->getStringValue("name");

            if (!model->nChildren()){
                continue;
            }
            if (name == _parent) {
                _selected_ac = model;  // save selected model for last iteration
                break;
            }

        }
        if (!model)
            continue;

    }// end for loop

    if (_selected_ac != 0){
        const string name = _selected_ac->getStringValue("name");
        double lat = _selected_ac->getDoubleValue("position/latitude-deg");
        double lon = _selected_ac->getDoubleValue("position/longitude-deg");
        double elevation = _selected_ac->getDoubleValue("position/altitude-ft");
        double hitch_offset_m = _x_offset * SG_FEET_TO_METER;
        _selectedpos.setLatitudeDeg(lat);
        _selectedpos.setLongitudeDeg(lon);
        _selectedpos.setElevationFt(elevation);

        SGVec3d rear_hitch(-hitch_offset_m, 0, 0);
        SGVec3d RearHitch = getCartHitchPosAt(rear_hitch);

        SGGeod rearpos = SGGeod::fromCart(RearHitch);

        double user_lat = rearpos.getLatitudeDeg();
        double user_lon = rearpos.getLongitudeDeg();

        double range, bearing;

        calcRangeBearing(pos.getLatitudeDeg(), pos.getLongitudeDeg(),
            user_lat, user_lon, range, bearing);
        _range_ft = range * 6076.11549;
        _relbrg = calcRelBearingDeg(bearing, hdg);
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "AIGroundVeh1cle: " << _name
                << " parent not found: dying ");
        setDie(true);
    }

}

void FGAIGroundVehicle::calcRangeBearing(double lat, double lon, double lat2, double lon2,
                            double &range, double &bearing) const
{
    // calculate the bearing and range of the second pos from the first
    double az2, distance;
    geo_inverse_wgs_84(lat, lon, lat2, lon2, &bearing, &az2, &distance);
    range = distance *= SG_METER_TO_NM;
}

double FGAIGroundVehicle::calcRelBearingDeg(double bearing, double heading)
{
    double angle = bearing - heading;

    SG_NORMALIZE_RANGE(angle, -180.0, 180.0);

    return angle;
}

SGVec3d FGAIGroundVehicle::getCartHitchPosAt(const SGVec3d& _off) const {
    double hdg = _selected_ac->getDoubleValue("orientation/true-heading-deg");
    double pitch = _selected_ac->getDoubleValue("orientation/pitch-deg");
    double roll = _selected_ac->getDoubleValue("orientation/roll-deg");

    // Transform that one to the horizontal local coordinate system.
    SGQuatd hlTrans = SGQuatd::fromLonLat(_selectedpos);

    // and postrotate the orientation of the AIModel wrt the horizontal
    // local frame
    hlTrans *= SGQuatd::fromYawPitchRollDeg(hdg, pitch, roll);

    // The offset converted to the usual body fixed coordinate system
    // rotated to the earth fiexed coordinates axis
    SGVec3d off = hlTrans.backTransform(_off);

    // Add the position offset of the AIModel to gain the earth centered position
    SGVec3d cartPos = SGVec3d::fromGeod(_selectedpos);

    return cartPos + off;
}

void FGAIGroundVehicle::AdvanceFP(){

    double count = 0;
    string parent_next_name =_selected_ac->getStringValue("waypoint/name-next");

    while(fp->getNextWaypoint() != 0 && fp->getNextWaypoint()->name != "END" && count < 5){
        SG_LOG(SG_GENERAL, SG_DEBUG, "AIGroundVeh1cle: " << _name
            <<" advancing waypoint to: " << parent_next_name);

        if (fp->getNextWaypoint()->name == parent_next_name){
            SG_LOG(SG_GENERAL, SG_DEBUG, "AIGroundVeh1cle: " << _name
                << " not setting waypoint already at: " << fp->getNextWaypoint()->name);
            return;
        }

        prev = curr;
        fp->IncrementWaypoint(false);
        curr = fp->getCurrentWaypoint();
        next = fp->getNextWaypoint();

        if (fp->getNextWaypoint()->name == parent_next_name){
            SG_LOG(SG_GENERAL, SG_DEBUG, "AIGroundVeh1cle: " << _name
            << " waypoint set to: " << fp->getNextWaypoint()->name);
            return;
        }

        count++;

    }// end while loop

    while(fp->getPreviousWaypoint() != 0 && fp->getPreviousWaypoint()->name != "END"
        && count > -10){
            SG_LOG(SG_GENERAL, SG_DEBUG, "AIGroundVeh1cle: " << _name
            << " retreating waypoint to: " << parent_next_name
            << " at: " << fp->getNextWaypoint()->name);

        if (fp->getNextWaypoint()->name == parent_next_name){
            SG_LOG(SG_GENERAL, SG_DEBUG, "AIGroundVeh1cle: " << _name
                << " not setting waypoint already at:" << fp->getNextWaypoint()->name );
            return;
        }

        prev = curr;
        fp->DecrementWaypoint(false);
        curr = fp->getCurrentWaypoint();
        next = fp->getNextWaypoint();

        if (fp->getNextWaypoint()->name == parent_next_name){
            SG_LOG(SG_GENERAL, SG_DEBUG, "AIGroundVeh1cle: " << _name
            << " waypoint set to: " << fp->getNextWaypoint()->name);
            return;
        }

        count--;

    }// end while loop
}

void FGAIGroundVehicle::setTowSpeed(){

    _parent_x_offset = _selected_ac->getDoubleValue("hitch/x-offset-ft");

    // double diff = _range_ft - _parent_x_offset;
    double  x = 0;

    if (_range_ft > _x_offset * 3) x = 50;

    if (_relbrg < -90 || _relbrg > 90){
        setSpeed(_parent_speed - 5 - x);
        //cout << _name << " case 1r _relbrg spd - 5 " << _relbrg << " " << diff << endl;
    }else if (_range_ft > _parent_x_offset + 0.25 && _relbrg >= -90 && _relbrg <= 90){
        setSpeed(_parent_speed + 1 + x);
        //cout << _name << " case 2r _relbrg spd + 1 " << _relbrg << " "
        //    << diff << " range " << _range_ft << endl;
    } else if (_range_ft < _parent_x_offset - 0.25 && _relbrg >= -90 && _relbrg <= 90){
        setSpeed(_parent_speed - 1 - x);
        //cout << _name << " case 3r _relbrg spd - 2 " << _relbrg << " "
        //    << diff << " " << _range_ft << endl;
    } else {
        setSpeed(_parent_speed);
        //cout << _name << " else r _relbrg " << _relbrg << " " << diff << endl;
    }

}
// end AIGroundvehicle
