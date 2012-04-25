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

#include <Viewer/viewer.hxx>
#include <Scenery/scenery.hxx>
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
_parent_y_offset(0),
_parent_z_offset(0),
_dt_count(0),
_next_run(0),
_break_count(0)

{
    invisible = false;
    _parent = "";
}

FGAIGroundVehicle::~FGAIGroundVehicle() {}

void FGAIGroundVehicle::readFromScenario(SGPropertyNode* scFileNode) {
    if (!scFileNode)
        return;

    FGAIShip::readFromScenario(scFileNode);

    setName(scFileNode->getStringValue("name", "groundvehicle"));
    setParentName(scFileNode->getStringValue("parent", ""));
    setNoRoll(scFileNode->getBoolValue("no-roll", true));
    setContactX1offset(scFileNode->getDoubleValue("contact-x1-offset", 0.0));
    setContactX2offset(scFileNode->getDoubleValue("contact-x2-offset", 0.0));
    setXOffset(scFileNode->getDoubleValue("hitch-x-offset", 35.0));
    setYOffset(scFileNode->getDoubleValue("hitch-y-offset", 0.0));
    setZOffset(scFileNode->getDoubleValue("hitch-z-offset", 0.0));
    setPitchoffset(scFileNode->getDoubleValue("pitch-offset", 0.0));
    setRolloffset(scFileNode->getDoubleValue("roll-offset", 0.0));
    setYawoffset(scFileNode->getDoubleValue("yaw-offset", 0.0));
    setPitchCoeff(scFileNode->getDoubleValue("pitch-coefficient", 0.1));
    setElevCoeff(scFileNode->getDoubleValue("elevation-coefficient", 0.25));
    setTowAngleGain(scFileNode->getDoubleValue("tow-angle-gain", 1.0));
    setTowAngleLimit(scFileNode->getDoubleValue("tow-angle-limit-deg", 2.0));
    setInitialTunnel(scFileNode->getBoolValue("tunnel", false));
    //we may need these later for towed vehicles
    //    setSubID(scFileNode->getIntValue("SubID", 0));
    //    setGroundOffset(scFileNode->getDoubleValue("ground-offset", 0.0));
    //    setFormate(scFileNode->getBoolValue("formate", true));
}

void FGAIGroundVehicle::bind() {
    FGAIShip::bind();

    tie("controls/constants/elevation-coeff",
        SGRawValuePointer<double>(&_elevation_coeff));
    tie("controls/constants/pitch-coeff",
        SGRawValuePointer<double>(&_pitch_coeff));
    tie("position/ht-AGL-ft",
        SGRawValuePointer<double>(&_ht_agl_ft));
    tie("hitch/rel-bearing-deg",
         SGRawValuePointer<double>(&_relbrg));
    tie("hitch/tow-angle-deg",
         SGRawValuePointer<double>(&_tow_angle));
    tie("hitch/range-ft",
        SGRawValuePointer<double>(&_range_ft));
    tie("hitch/x-offset-ft",
        SGRawValuePointer<double>(&_x_offset));
    tie("hitch/y-offset-ft",
        SGRawValuePointer<double>(&_y_offset));
    tie("hitch/z-offset-ft",
        SGRawValuePointer<double>(&_z_offset));
    tie("hitch/parent-x-offset-ft",
        SGRawValuePointer<double>(&_parent_x_offset));
    tie("hitch/parent-y-offset-ft",
        SGRawValuePointer<double>(&_parent_y_offset));
    tie("hitch/parent-z-offset-ft",
        SGRawValuePointer<double>(&_parent_z_offset));
    tie("controls/constants/tow-angle/gain",
        SGRawValuePointer<double>(&_tow_angle_gain));
    tie("controls/constants/tow-angle/limit-deg",
        SGRawValuePointer<double>(&_tow_angle_limit));
    tie("controls/contact-x1-offset-ft",
        SGRawValuePointer<double>(&_contact_x1_offset));
    tie("controls/contact-x2-offset-ft",
        SGRawValuePointer<double>(&_contact_x2_offset));
}

bool FGAIGroundVehicle::init(bool search_in_AI_path) {
    if (!FGAIShip::init(search_in_AI_path))
        return false;
    reinit();
    return true;
}

void FGAIGroundVehicle::reinit() {
    invisible = false;
    _limit = 200;
    no_roll = true;

    props->setStringValue("controls/parent-name", _parent.c_str());

    if (setParentNode()){
        _parent_x_offset = _selected_ac->getDoubleValue("hitch/x-offset-ft");
        _parent_y_offset = _selected_ac->getDoubleValue("hitch/y-offset-ft");
        _parent_z_offset = _selected_ac->getDoubleValue("hitch/z-offset-ft");
        _hitch_x_offset_m = _selected_ac->getDoubleValue("hitch/x-offset-ft")
            * SG_FEET_TO_METER;
        _hitch_y_offset_m = _selected_ac->getDoubleValue("hitch/y-offset-ft")
            * SG_FEET_TO_METER;
        _hitch_z_offset_m = _selected_ac->getDoubleValue("hitch/z-offset-ft")
            * SG_FEET_TO_METER;
        setParent();
    }

    FGAIShip::reinit();
}

void FGAIGroundVehicle::update(double dt) {
    //    SG_LOG(SG_AI, SG_ALERT, "updating GroundVehicle: " << _name );
    FGAIShip::update(dt);

    RunGroundVehicle(dt);
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

void FGAIGroundVehicle::setYOffset(double y) {
    _y_offset = y;
}

void FGAIGroundVehicle::setZOffset(double z) {
    _z_offset = z;
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

void FGAIGroundVehicle::setTowAngle(double ta, double dt, double coeff){
    ta *= _tow_angle_gain;
    double factor = -0.0045 * speed + 1;
    double limit = _tow_angle_limit * factor;
//	cout << "speed "<< speed << " _factor " << _factor<<" " <<_tow_angle_limit<< endl; 
     _tow_angle = pow(ta,2) * sign(ta) * factor;
    SG_CLAMP_RANGE(_tow_angle, -limit, limit);
}

bool FGAIGroundVehicle::getPitch() {

    if (!_tunnel){
        double vel = props->getDoubleValue("velocities/true-airspeed-kt", 0);
        double contact_offset_x1_m = _contact_x1_offset * SG_FEET_TO_METER;
        double contact_offset_x2_m = _contact_x2_offset * SG_FEET_TO_METER;
        double _z_offset_m = _parent_z_offset * SG_FEET_TO_METER;

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
        //double max_alt = 10000;

        if (globals->get_scenery()->get_elevation_m(SGGeod::fromGeodM(geodFront, 3000),
            elev_front, &_material, 0)){
                front_elev_m = elev_front + _z_offset_m;
        } else
            return false;

        if (globals->get_scenery()->get_elevation_m(SGGeod::fromGeodM(geodRear, 3000),
            elev_rear, &_material, 0)){
                rear_elev_m = elev_rear;
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

    } else {

        if (prev->getAltitude() == 0 || curr->getAltitude() == 0) return false;

        static double distance;
        static double d_alt;
        static double curr_alt;
        static double prev_alt;

        if (_new_waypoint){
            //cout << "new waypoint, calculating pitch " << endl;
            curr_alt = curr->getAltitude();
            prev_alt = prev->getAltitude();
            //cout << "prev_alt" <<prev_alt << endl;
            d_alt = (curr_alt - prev_alt) * SG_METER_TO_FEET;
            //_elevation = prev->altitude;
            distance = SGGeodesy::distanceM(SGGeod::fromDeg(prev->getLongitude(), prev->getLatitude()),
            SGGeod::fromDeg(curr->getLongitude(), curr->getLatitude()));
            _pitch = atan2(d_alt, distance * SG_METER_TO_FEET) * SG_RADIANS_TO_DEGREES;
            //cout << "new waypoint, calculating pitch " <<  _pitch << 
            //    " " << _pitch_offset << " " << _elevation <<endl;
        }

        double distance_to_go = SGGeodesy::distanceM(SGGeod::fromDeg(pos.getLongitudeDeg(), pos.getLatitudeDeg()),
            SGGeod::fromDeg(curr->getLongitude(), curr->getLatitude()));

        /*cout << "tunnel " << _tunnel
             << " distance prev & curr " << prev->name << " " << curr->name << " " << distance * SG_METER_TO_FEET
             << " distance to go " << distance_to_go * SG_METER_TO_FEET
             << " d_alt ft " << d_alt
             << endl;*/

        if (distance_to_go > distance)
            _elevation = prev_alt;
        else
            _elevation = curr_alt - (tan(_pitch * SG_DEGREES_TO_RADIANS) * distance_to_go * SG_METER_TO_FEET);

    }

    return true;
}

void FGAIGroundVehicle::setParent(){

    double lat = _selected_ac->getDoubleValue("position/latitude-deg");
    double lon = _selected_ac->getDoubleValue("position/longitude-deg");
    double elevation = _selected_ac->getDoubleValue("position/altitude-ft");

    _selectedpos.setLatitudeDeg(lat);
    _selectedpos.setLongitudeDeg(lon);
    _selectedpos.setElevationFt(elevation);

    _parent_speed = _selected_ac->getDoubleValue("velocities/true-airspeed-kt");

    SGVec3d rear_hitch(-_hitch_x_offset_m, _hitch_y_offset_m, 0);
    SGVec3d RearHitch = getCartHitchPosAt(rear_hitch);

    SGGeod rearpos = SGGeod::fromCart(RearHitch);

    double user_lat = rearpos.getLatitudeDeg();
    double user_lon = rearpos.getLongitudeDeg();

    double range, bearing;

    calcRangeBearing(pos.getLatitudeDeg(), pos.getLongitudeDeg(),
        user_lat, user_lon, range, bearing);
    _range_ft = range * 6076.11549;
    _relbrg = calcRelBearingDeg(bearing, hdg);
}

void FGAIGroundVehicle::calcRangeBearing(double lat, double lon, double lat2, double lon2,
                            double &range, double &bearing) const
{
    // calculate the bearing and range of the second pos from the first
    double az2, distance;
    geo_inverse_wgs_84(lat, lon, lat2, lon2, &bearing, &az2, &distance);
    range = distance * SG_METER_TO_NM;
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

    while(fp->getNextWaypoint() != 0 && fp->getNextWaypoint()->getName() != "END" && count < 5){
        SG_LOG(SG_AI, SG_DEBUG, "AIGroundVeh1cle: " << _name
            <<" advancing waypoint to: " << parent_next_name);

        if (fp->getNextWaypoint()->getName() == parent_next_name){
            SG_LOG(SG_AI, SG_DEBUG, "AIGroundVeh1cle: " << _name
                << " not setting waypoint already at: " << fp->getNextWaypoint()->getName());
            return;
        }

        prev = curr;
        fp->IncrementWaypoint(false);
        curr = fp->getCurrentWaypoint();
        next = fp->getNextWaypoint();

        if (fp->getNextWaypoint()->getName() == parent_next_name){
            SG_LOG(SG_AI, SG_DEBUG, "AIGroundVeh1cle: " << _name
            << " waypoint set to: " << fp->getNextWaypoint()->getName());
            return;
        }

        count++;

    }// end while loop

    while(fp->getPreviousWaypoint() != 0 && fp->getPreviousWaypoint()->getName() != "END"
        && count > -10){
            SG_LOG(SG_AI, SG_DEBUG, "AIGroundVeh1cle: " << _name
            << " retreating waypoint to: " << parent_next_name
            << " at: " << fp->getNextWaypoint()->getName());

        if (fp->getNextWaypoint()->getName() == parent_next_name){
            SG_LOG(SG_AI, SG_DEBUG, "AIGroundVeh1cle: " << _name
                << " not setting waypoint already at:" << fp->getNextWaypoint()->getName() );
            return;
        }

        prev = curr;
        fp->DecrementWaypoint(false);
        curr = fp->getCurrentWaypoint();
        next = fp->getNextWaypoint();

        if (fp->getNextWaypoint()->getName() == parent_next_name){
            SG_LOG(SG_AI, SG_DEBUG, "AIGroundVeh1cle: " << _name
            << " waypoint set to: " << fp->getNextWaypoint()->getName());
            return;
        }

        count--;

    }// end while loop
}

void FGAIGroundVehicle::setTowSpeed(){

    //double diff = _range_ft - _x_offset;
    double  x = 0;

    if (_range_ft > _x_offset * 3) x = 50;

    if (_relbrg < -90 || _relbrg > 90){
        setSpeed(_parent_speed - 5 - x);
        //cout << _name << " case 1r _relbrg spd - 5 " << _relbrg << " " << diff << endl;
    }else if (_range_ft > _x_offset + 0.25 && _relbrg >= -90 && _relbrg <= 90){
        setSpeed(_parent_speed + 1 + x);
        //cout << _name << " case 2r _relbrg spd + 1 " << _relbrg << " "
        //    << diff << " range " << _range_ft << endl;
    } else if (_range_ft < _x_offset - 0.25 && _relbrg >= -90 && _relbrg <= 90){
        setSpeed(_parent_speed - 1 - x);
        //cout << _name << " case 3r _relbrg spd - 2 " << _relbrg << " "
        //    << diff << " " << _range_ft << endl;
    } else {
        setSpeed(_parent_speed);
        //cout << _name << " else r _relbrg " << _relbrg << " " << diff << endl;
    }

}

void FGAIGroundVehicle::RunGroundVehicle(double dt){

    _dt_count += dt;

    ///////////////////////////////////////////////////////////////////////////
    // Check execution time (currently once every 0.05 sec or 20 fps)
    // Add a bit of randomization to prevent the execution of all flight plans
    // in synchrony, which can add significant periodic framerate flutter.
    // Randomization removed to get better appearance
    ///////////////////////////////////////////////////////////////////////////

    //cout << "_start_sec " << _start_sec << " time_sec " << time_sec << endl;
    if (_dt_count < _next_run)
        return;

    _next_run = 0.05 /*+ (0.015 * sg_random())*/;

    if (getPitch()){
        setElevation(_elevation, _dt_count, _elevation_coeff);
        ClimbTo(_elevation_ft);
        setPitch(_pitch, _dt_count, _pitch_coeff);
        PitchTo(_pitch_deg);
    }

    if(_parent == ""){
        AccelTo(prev->getSpeed());
        _dt_count = 0;
        return;
    }

    setParent();

    string parent_next_name = _selected_ac->getStringValue("waypoint/name-next");
    bool parent_waiting = _selected_ac->getBoolValue("waypoint/waiting");
    //bool parent_restart = _selected_ac->getBoolValue("controls/restart"); 

    if (parent_next_name == "END" && fp->getNextWaypoint()->getName() != "END" ){
        SG_LOG(SG_AI, SG_DEBUG, "AIGroundVeh1cle: " << _name
            << " setting END: getting new waypoints ");
        AdvanceFP();
        setWPNames();
        setTunnel(_initial_tunnel);
        if(_restart) _missed_count = 200;
        /*} else if (parent_next_name == "WAIT" && fp->getNextWaypoint()->name != "WAIT" ){*/
    } else if (parent_waiting && !_waiting){
        SG_LOG(SG_AI, SG_DEBUG, "AIGroundVeh1cle: " << _name
            << " setting WAIT/WAITUNTIL: getting new waypoints ");
        AdvanceFP();
        setWPNames();
        _waiting = true;
    } else if (parent_next_name != "WAIT" && fp->getNextWaypoint()->getName() == "WAIT"){
        SG_LOG(SG_AI, SG_DEBUG, "AIGroundVeh1cle: " << _name
            << " wait done: getting new waypoints ");
        _waiting = false;
        _wait_count = 0;
        fp->IncrementWaypoint(false);
        next = fp->getNextWaypoint();

        if (next->getName() == "WAITUNTIL" || next->getName() == "WAIT"
            || next->getName() == "END"){
        } else {
            prev = curr;
            fp->IncrementWaypoint(false);
            curr = fp->getCurrentWaypoint();
            next = fp->getNextWaypoint();
        }

        setWPNames();
    } else if (_range_ft > (_x_offset +_parent_x_offset)* 4
        ){
        SG_LOG(SG_AI, SG_ALERT, "AIGroundVeh1cle: " << _name
            << " rescue: reforming train " << _range_ft 
            );

        setTowAngle(0, dt, 1);
        setSpeed(_parent_speed + (10 * sign(_parent_speed)));

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

//    FGAIShip::update(_dt_count);
    _dt_count = 0;

}

// end AIGroundvehicle
