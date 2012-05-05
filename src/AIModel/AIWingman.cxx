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

#include <simgear/sg_inlines.h>


#include "AIWingman.hxx"

FGAIWingman::FGAIWingman() : FGAIBallistic(otWingman),
_formate_to_ac(true),
_break(false),
_join(false),
_break_angle(-90),
_coeff_hdg(5.0),
_coeff_pch(5.0),
_coeff_bnk(5.0),
_coeff_spd(2.0)

{
    invisible = false;
    _parent="";
    tgt_heading = 250;

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
    setSubID(scFileNode->getIntValue("SubID", 0));
    setXoffset(scFileNode->getDoubleValue("x-offset", 0.0));
    setYoffset(scFileNode->getDoubleValue("y-offset", 0.0));
    setZoffset(scFileNode->getDoubleValue("z-offset", 0.0));
    setPitchoffset(scFileNode->getDoubleValue("pitch-offset", 0.0));
    setRolloffset(scFileNode->getDoubleValue("roll-offset", 0.0));
    setYawoffset(scFileNode->getDoubleValue("yaw-offset", 0.0));
    setGroundOffset(scFileNode->getDoubleValue("ground-offset", 0.0));
    setFormate(scFileNode->getBoolValue("formate", true));
    setMaxSpeed(scFileNode->getDoubleValue("max-speed-kts", 300.0));
    setCoeffHdg(scFileNode->getDoubleValue("coefficients/heading", 5.0));
    setCoeffPch(scFileNode->getDoubleValue("coefficients/pitch", 5.0));
    setCoeffBnk(scFileNode->getDoubleValue("coefficients/bank", 4.0));
    setCoeffSpd(scFileNode->getDoubleValue("coefficients/speed", 2.0));


}

void FGAIWingman::bind() {
    FGAIBallistic::bind();

    props->untie("controls/slave-to-ac");

    tie("id", SGRawValueMethods<FGAIBase,int>(*this,
        &FGAIBase::getID));
    tie("subID", SGRawValueMethods<FGAIBase,int>(*this,
        &FGAIBase::_getSubID));
    tie("position/altitude-ft",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getElevationFt,
        &FGAIBase::_setAltitude));
    tie("position/latitude-deg",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getLatitude,
        &FGAIBase::_setLatitude));
    tie("position/longitude-deg",
        SGRawValueMethods<FGAIBase,double>(*this,
        &FGAIBase::_getLongitude,
        &FGAIBase::_setLongitude));

    tie("controls/break", SGRawValuePointer<bool>(&_break));
    tie("controls/join", SGRawValuePointer<bool>(&_join));

    tie("controls/formate-to-ac",
        SGRawValueMethods<FGAIWingman,bool>
        (*this, &FGAIWingman::getFormate, &FGAIWingman::setFormate));
    tie("controls/tgt-heading-deg",
        SGRawValueMethods<FGAIWingman,double>
        (*this, &FGAIWingman::getTgtHdg, &FGAIWingman::setTgtHdg));
    tie("controls/tgt-speed-kt",
        SGRawValueMethods<FGAIWingman,double>
        (*this, &FGAIWingman::getTgtSpd, &FGAIWingman::setTgtSpd));
    tie("controls/break-deg-rel",
        SGRawValueMethods<FGAIWingman,double>
        (*this, &FGAIWingman::getBrkAng, &FGAIWingman::setBrkAng));
    tie("controls/coefficients/heading",
        SGRawValuePointer<double>(&_coeff_hdg));
    tie("controls/coefficients/pitch",
        SGRawValuePointer<double>(&_coeff_pch));
    tie("controls/coefficients/bank",
        SGRawValuePointer<double>(&_coeff_bnk));
    tie("controls/coefficients/speed",
        SGRawValuePointer<double>(&_coeff_spd));

    tie("orientation/pitch-deg",   SGRawValuePointer<double>(&pitch));
    tie("orientation/roll-deg",    SGRawValuePointer<double>(&roll));
    tie("orientation/true-heading-deg", SGRawValuePointer<double>(&hdg));

    tie("submodels/serviceable", SGRawValuePointer<bool>(&serviceable));

    tie("load/rel-brg-to-user-deg",
        SGRawValueMethods<FGAIBallistic,double>
        (*this, &FGAIBallistic::getRelBrgHitchToUser));
    tie("load/elev-to-user-deg",
        SGRawValueMethods<FGAIBallistic,double>
        (*this, &FGAIBallistic::getElevHitchToUser));

    tie("velocities/vertical-speed-fps",
        SGRawValuePointer<double>(&vs));
    tie("velocities/true-airspeed-kt",
        SGRawValuePointer<double>(&speed));
    tie("velocities/speed-east-fps",
        SGRawValuePointer<double>(&_speed_east_fps));
    tie("velocities/speed-north-fps",
        SGRawValuePointer<double>(&_speed_north_fps));

    tie("position/x-offset",
        SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getXOffset, &FGAIBase::setXoffset));
    tie("position/y-offset",
        SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getYOffset, &FGAIBase::setYoffset));
    tie("position/z-offset",
        SGRawValueMethods<FGAIBase,double>(*this, &FGAIBase::_getZOffset, &FGAIBase::setZoffset));
    tie("position/tgt-x-offset",
        SGRawValueMethods<FGAIBallistic,double>(*this, &FGAIBallistic::getTgtXOffset, &FGAIBallistic::setTgtXOffset));
    tie("position/tgt-y-offset",
        SGRawValueMethods<FGAIBallistic,double>(*this, &FGAIBallistic::getTgtYOffset, &FGAIBallistic::setTgtYOffset));
    tie("position/tgt-z-offset",
        SGRawValueMethods<FGAIBallistic,double>(*this, &FGAIBallistic::getTgtZOffset, &FGAIBallistic::setTgtZOffset));
}

bool FGAIWingman::init(bool search_in_AI_path) {
    if (!FGAIBallistic::init(search_in_AI_path))
        return false;
    reinit();
    return true;
}

void FGAIWingman::reinit() {
    invisible = false;

    _tgt_x_offset = _x_offset;
    _tgt_y_offset = _y_offset;
    _tgt_z_offset = _z_offset;

    hdg = _azimuth;
    pitch = _elevation;
    roll = _rotation;
    _ht_agl_ft = 1e10;

    if(_parent != ""){
        setParentNode();
    }

    setParentNodes(_selected_ac);

    props->setStringValue("submodels/path", _path.c_str());
    user_WoW_node      = fgGetNode("gear/gear[1]/wow", true);

    FGAIBallistic::reinit();
}

void FGAIWingman::update(double dt) {

//    FGAIBallistic::update(dt);

    if (_formate_to_ac){
        formateToAC(dt);
        Transform();
        setBrkHdg(_break_angle);
    }else if (_break) {
        FGAIBase::update(dt);
        tgt_altitude_ft = altitude_ft;
        tgt_speed = speed;
        tgt_roll = roll;
        tgt_pitch = pitch;
        Break(dt);
        Transform();
    } else {
        Join(dt);
        Transform();
    }

}

double FGAIWingman::calcDistanceM(SGGeod pos1, SGGeod pos2) const {
    //calculate the distance load to hitch
    SGVec3d cartPos1 = SGVec3d::fromGeod(pos1);
    SGVec3d cartPos2 = SGVec3d::fromGeod(pos2);

    SGVec3d diff = cartPos1 - cartPos2;
    double distance = norm(diff);
    return distance;
}

double FGAIWingman::calcAngle(double range, SGGeod pos1, SGGeod pos2){

    double angle = 0;
    double distance = calcDistanceM(pos1, pos2);
    double daltM = pos1.getElevationM() - pos2.getElevationM();

    if (fabs(distance) < SGLimits<float>::min()) {
        angle = 0;
    } else {
        double sAngle = daltM/range;
        sAngle = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
        angle = SGMiscd::rad2deg(asin(sAngle));
    }

    return angle;
}

void FGAIWingman::formateToAC(double dt){

    double p_hdg, p_pch, p_rll, p_agl, p_ht, p_wow = 0;

    setTgtOffsets(dt, 25);

    if (_pnode != 0) {
        setParentPos();
        p_hdg = _p_hdg_node->getDoubleValue();
        p_pch = _p_pch_node->getDoubleValue();
        p_rll = _p_rll_node->getDoubleValue();
        p_ht  = _p_alt_node->getDoubleValue();
        setOffsetPos(_parentpos, p_hdg, p_pch, p_rll);
        setSpeed(_p_spd_node->getDoubleValue());
    }else {
        _setUserPos();
        p_hdg = manager->get_user_heading();
        p_pch = manager->get_user_pitch();
        p_rll = manager->get_user_roll();
        p_ht  = manager->get_user_altitude();
        setOffsetPos(userpos, p_hdg,p_pch, p_rll);
        setSpeed(manager->get_user_speed());
    }

    // elapsed time has a random initialisation so that each
    // wingman moves differently
    _elapsed_time += dt;

    // we derive a sine based factor to give us smoothly
    // varying error between -1 and 1
    double factor  = sin(SGMiscd::deg2rad(_elapsed_time * 10));
    double r_angle = 5 * factor;
    double p_angle = 2.5 * factor;
    double h_angle = 5 * factor;
    double h_feet  = 3 * factor;

    p_agl = manager->get_user_agl();
    p_wow = user_WoW_node->getDoubleValue();

    if(p_agl <= 10 || p_wow == 1) {
        _height = p_ht;
        //cout << "ht case1 " ;
    } else if (p_agl > 10 && p_agl <= 150 ) {
        setHt(p_ht, dt, 1.0);
        //cout << "ht case2 " ;
    } else if (p_agl > 150 && p_agl <= 250) {
        setHt(_offsetpos.getElevationFt()+ h_feet, dt, 0.75);
        //cout << "ht case3 " ;
    } else{
        setHt(_offsetpos.getElevationFt()+ h_feet, dt, 0.5);
        //cout << "ht case4 " ;
    }

    pos.setElevationFt(_height);
    pos.setLatitudeDeg(_offsetpos.getLatitudeDeg());
    pos.setLongitudeDeg(_offsetpos.getLongitudeDeg());

    // these calculations are unreliable at slow speeds
    // and we don't want random movement on the ground
    if(speed >= 10 && p_wow != 1) {
        setHdg(p_hdg + h_angle, dt, 0.9);
        setPch(p_pch + p_angle + _pitch_offset, dt, 0.9);

        if (roll <= 115 && roll >= -115)
            setBnk(p_rll + r_angle + _roll_offset, dt, 0.5);
        else
            roll = p_rll + r_angle + _roll_offset;

    } else {
        setHdg(p_hdg, dt, 0.9);
        setPch(p_pch + _pitch_offset, dt, 0.9);
        setBnk(p_rll + _roll_offset, dt, 0.9);
    }

        setOffsetVelocity(dt, pos);
}// end formateToAC

void FGAIWingman::Break(double dt) {

    Run(dt);

    //calculate the turn direction: 1 = right, -1 = left
    double rel_brg = calcRelBearingDeg(tgt_heading, hdg);
    int turn = SGMiscd::sign(rel_brg);

    // set heading and pitch
    setHdg(tgt_heading, dt, _coeff_hdg);
    setPch(0, dt, _coeff_pch);

    if (fabs(tgt_heading - hdg) >= 10)
        setBnk(45 * turn , dt, _coeff_bnk);
    else
        setBnk(0, dt, _coeff_bnk);

}  // end Break

void FGAIWingman::Join(double dt) {

    double range, bearing, az2;
    double parent_hdg, parent_spd = 0;
    double p_hdg, p_pch, p_rll = 0;

    setTgtOffsets(dt, 25);

    if (_pnode != 0) {
        setParentPos();
        p_hdg = _p_hdg_node->getDoubleValue();
        p_pch = _p_pch_node->getDoubleValue();
        p_rll = _p_rll_node->getDoubleValue();
        setOffsetPos(_parentpos, p_hdg, p_pch, p_rll);
        parent_hdg = _p_hdg_node->getDoubleValue();
        parent_spd = _p_spd_node->getDoubleValue();
    }else {
        _setUserPos();
        p_hdg = manager->get_user_heading();
        p_pch = manager->get_user_pitch();
        p_rll = manager->get_user_roll();
        setOffsetPos(userpos, p_hdg, p_pch, p_rll);
        parent_hdg = manager->get_user_heading();
        parent_spd = manager->get_user_speed();
    }

    setSpeed(parent_spd);

    double distance = calcDistanceM(pos, _offsetpos);
    double daltM = _offsetpos.getElevationM() - pos.getElevationM();
    double limit = 10;
    double hdg_l_lim = parent_hdg - limit;
    SG_NORMALIZE_RANGE(hdg_l_lim, 0.0, 360.0);
    double hdg_r_lim = parent_hdg + limit;
    SG_NORMALIZE_RANGE(hdg_r_lim, 0.0, 360.0);

    if (distance <= 2 && fabs(daltM) <= 2 &&
        (hdg >= hdg_l_lim || hdg <= hdg_r_lim)){
            _height = _offsetpos.getElevationFt();
            _formate_to_ac = true;
            _join = false;

            SG_LOG(SG_AI, SG_ALERT, _name << " joined " << " RANGE " << distance
            << " SPEED " << speed );

            return;
    }

    geo_inverse_wgs_84(pos, _offsetpos, &bearing, &az2, &range);

    double rel_brg   = calcRelBearingDeg(bearing, hdg);
    double recip_brg = calcRecipBearingDeg(bearing);
    double angle = calcAngle(distance,_offsetpos, pos);
    //double approx_angle = atan2(daltM, range);
    double frm_spd = 50; // formation speed
    double join_rnge = 1000.0;
//    double recip_parent_hdg = calcRecipBearingDeg(parent_hdg);
    int turn = SGMiscd::sign(rel_brg);// turn direction: 1 = right, -1 = left

    if (range <= join_rnge && (hdg >= hdg_l_lim || hdg <= hdg_r_lim)){

        //these are the rules governing joining

        if ((rel_brg <= -175 || rel_brg >= 175) && range <=10 ){
            // station is behind us - back up a bit
            setSpeed(parent_spd - ((frm_spd/join_rnge) * range));
            setHdg(recip_brg, dt, _coeff_hdg);
            setPch(angle, dt, _coeff_pch);
            //cout << _name << " backing up HEADING " << hdg
            //    << " RANGE " << range;
        } else if (rel_brg >= -5 || rel_brg <= 5) {
            // station is in front of us - slow down
            setSpeed(parent_spd + ((frm_spd/100) * range));
            //SGMiscd::clip
            setHdg(bearing, dt, 1.5);
            setPch(angle, dt, _coeff_pch);
            //cout << _name << " slowing HEADING " << hdg
            //    << " RANGE " << range <<endl;
        } else if ( range <=10 ){
            // station is to one side - equal speed and turn towards
            setSpd(parent_spd , dt, 2.0);
            setSpeed(_speed);
            setHdg(parent_hdg + (5 * turn), dt, _coeff_hdg);
            //cout << _name << " equal speed HEADING " << hdg
            //    << " RANGE " << range<< endl;
        } else {
            // we missed it - equal speed and turn to recip
            setSpd(parent_spd , dt, 2.0);
            setSpeed(_speed);
            setHdg(recip_brg, dt, _coeff_hdg);
            //cout << _name << " WHOOPS!! missed join HEADING " << hdg
            //    << " RANGE " << range<< endl;
        }

    } else if (range <= join_rnge) {
        // we missed it - equal speed and turn to recip
        setSpd(parent_spd , dt, 2.0);
        setSpeed(_speed);
        setHdg(recip_brg , dt, _coeff_hdg);
        //cout << _name << " WHOOPS!! missed approach HEADING " << hdg
        //    << " " << recip_brg
        //    /*<< " " << recip_parent_hdg*/
        //    << " RANGE " << range<< endl;
    } else if (range > join_rnge && range <= 2000 ){
        //approach phase
        //cout << _name << " approach HEADING " << hdg
        //        << " RANGE " << range<< endl;
        setSpd(parent_spd + frm_spd, dt, 2.0);
        setSpeed(_speed);
        setHdg(bearing, dt, _coeff_hdg);
        setPch(angle, dt, _coeff_pch);
    } else {
        //hurry up
        //cout << _name << " hurry up HEADING " << hdg
        //        << " RANGE " << range<< endl;
        setSpd(_max_speed -10, dt, 2.0);
        setSpeed(_speed);
        setHdg(bearing, dt, _coeff_hdg);
        setPch(angle, dt, _coeff_pch);
    }

    Run(dt);

    // set roll

    if (fabs(bearing - hdg) >= 10)
        setBnk(45 * turn , dt, _coeff_bnk);
    else
        setBnk(0, dt, _coeff_bnk);

}  // end Join

void FGAIWingman::Run(double dt) {

    // don't let speed become negative
    SG_CLAMP_RANGE(speed, 100.0, _max_speed);

    double speed_fps = speed * SG_KT_TO_FPS;

    // calculate vertical and horizontal speed components
    if (speed == 0.0) {
        hs = vs = 0.0;
    } else {
        vs = sin( pitch * SG_DEGREES_TO_RADIANS ) * speed_fps;
        hs = cos( pitch * SG_DEGREES_TO_RADIANS ) * speed_fps;
    }

    //cout << "vs hs " << vs << " " << hs << endl;

    //resolve horizontal speed into north and east components:
    double speed_north_fps = cos(hdg / SG_RADIANS_TO_DEGREES) * hs;
    double speed_east_fps = sin(hdg / SG_RADIANS_TO_DEGREES) * hs;

    // convert horizontal speed (fps) to degrees per second
    double speed_north_deg_sec = speed_north_fps / ft_per_deg_lat;
    double speed_east_deg_sec  = speed_east_fps / ft_per_deg_lon;

    //get wind components
    _wind_from_north = manager->get_wind_from_north();
    _wind_from_east = manager->get_wind_from_east();

    // convert wind speed (fps) to degrees lat/lon per second
    double wind_speed_from_north_deg_sec = _wind_from_north / ft_per_deg_lat;
    double wind_speed_from_east_deg_sec  = _wind_from_east / ft_per_deg_lon;

    //recombine the horizontal velocity components
    hs = sqrt(((speed_north_fps) * (speed_north_fps))
        + ((speed_east_fps)* (speed_east_fps )));

    if (hs <= 0.00001)
        hs = 0;

    if (vs <= 0.00001 && vs >= -0.00001)
        vs = 0;

    //cout << "lat " << pos.getLatitudeDeg()<< endl;
    // set new position
    pos.setLatitudeDeg( pos.getLatitudeDeg()
            + (speed_north_deg_sec - wind_speed_from_north_deg_sec) * dt );
    pos.setLongitudeDeg( pos.getLongitudeDeg()
            + (speed_east_deg_sec - wind_speed_from_east_deg_sec ) * dt );
    pos.setElevationFt(pos.getElevationFt() + vs * dt);

    //cout << _name << " run hs " << hs << " vs " << vs << endl;

    // recalculate total speed
    if ( vs == 0 && hs == 0)
        speed = 0;
    else
        speed = sqrt( vs * vs + hs * hs) / SG_KT_TO_FPS;

    // recalculate elevation and azimuth (velocity vectors)
    pitch = atan2( vs, hs ) * SG_RADIANS_TO_DEGREES;
    hdg   = atan2((speed_east_fps),(speed_north_fps))* SG_RADIANS_TO_DEGREES;

    // rationalise heading
    SG_NORMALIZE_RANGE(hdg, 0.0, 360.0);

}// end Run

// end AIWingman
