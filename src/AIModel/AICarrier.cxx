// FGAICarrier - FGAIShip-derived class creates an AI aircraft carrier
//
// Written by David Culp, started October 2004.
// - davidculp2@comcast.net
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

#include <config.h>

#include <algorithm>
#include <string>
#include <vector>

#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>

#include <cmath>
#include <Main/util.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>


#include "AICarrier.hxx"
#include "AINotifications.hxx"

FGAICarrier::FGAICarrier() : FGAIShip(otCarrier)
{
    simgear::Emesary::GlobalTransmitter::instance()->Register(this);
}

FGAICarrier::~FGAICarrier()
{
   simgear::Emesary::GlobalTransmitter::instance()->DeRegister(this);
}

void FGAICarrier::readFromScenario(SGPropertyNode* scFileNode) {
  if (!scFileNode)
    return;

  FGAIShip::readFromScenario(scFileNode);

  setRadius(scFileNode->getDoubleValue("turn-radius-ft", 2000));
  setSign(scFileNode->getStringValue("pennant-number"));
  setDeckAltitudeFt(scFileNode->getDoubleValue("deck-altitude"));
  setWind_from_east(scFileNode->getDoubleValue("wind_from_east", 0));
  setWind_from_north(scFileNode->getDoubleValue("wind_from_north", 0));
  setTACANChannelID(scFileNode->getStringValue("TACAN-channel-ID", "029Y"));
  setMaxLat(scFileNode->getDoubleValue("max-lat", 0));
  setMinLat(scFileNode->getDoubleValue("min-lat", 0));
  setMaxLong(scFileNode->getDoubleValue("max-long", 0));
  setMinLong(scFileNode->getDoubleValue("min-long", 0));
  setMPControl(scFileNode->getBoolValue("mp-control", false));
  setAIControl(scFileNode->getBoolValue("ai-control", false));
  setCallSign(scFileNode->getStringValue("callsign", ""));
  
  _angled_deck_degrees = scFileNode->getDoubleValue("angled-deck-degrees", -8.5);

    SGPropertyNode* flolsNode = getPositionFromNode(scFileNode, "flols-pos", _flolsPosOffset);
    if (flolsNode) {
        _flolsHeadingOffsetDeg = flolsNode->getDoubleValue("heading-offset-deg", 0.0);
        _flolsApproachAngle = flolsNode->getDoubleValue("glidepath-angle-deg", 3.5);
    }
    else {
        _flolsPosOffset(2) = -(_deck_altitude_ft * SG_FEET_TO_METER + 10);
    }
    
    //// the FLOLS (or IFLOLS) position doesn't produce an accurate angle;
    //// so to fix this we can definition the touchdown position which 
    //// is the centreline of the 3rd wire

    _flolsTouchdownPosition = _flolsPosOffset; // default to the flolsPosition
    getPositionFromNode(scFileNode, "flols-touchdown-position", _flolsTouchdownPosition);

    if (!getPositionFromNode(scFileNode, "tower-position", _towerPosition)) {
        _towerPosition(2) = -(_deck_altitude_ft * SG_FEET_TO_METER + 10);
        SG_LOG(SG_AI, SG_INFO, "AICarrier: tower-position not defined - using default");
    }

    if (!getPositionFromNode(scFileNode, "lso-position", _lsoPosition)){
        _lsoPosition(2) = -(_deck_altitude_ft * SG_FEET_TO_METER + 10);
        SG_LOG(SG_AI, SG_INFO, "AICarrier: lso-position not defined - using default");
    }


  std::vector<SGPropertyNode_ptr> props = scFileNode->getChildren("parking-pos");
  std::vector<SGPropertyNode_ptr>::const_iterator it;
  for (it = props.begin(); it != props.end(); ++it) {
    const string name = (*it)->getStringValue("name", "unnamed");
    // Transform to the right coordinate frame, configuration is done in
    // the usual x-back, y-right, z-up coordinates, computations
    // in the simulation usual body x-forward, y-right, z-down coordinates
    double offset_x = -(*it)->getDoubleValue("x-offset-m", 0);
    double offset_y = (*it)->getDoubleValue("y-offset-m", 0);
    double offset_z = -(*it)->getDoubleValue("z-offset-m", 0);
    double hd = (*it)->getDoubleValue("heading-offset-deg", 0);
    ParkPosition pp(name, SGVec3d(offset_x, offset_y, offset_z), hd);
    _ppositions.push_back(pp);
  }
}

void FGAICarrier::setWind_from_east(double fps) {
    _wind_from_east = fps;
}

void FGAICarrier::setWind_from_north(double fps) {
    _wind_from_north = fps;
}

void FGAICarrier::setMaxLat(double deg) {
    _max_lat = fabs(deg);
}

void FGAICarrier::setMinLat(double deg) {
    _min_lat = fabs(deg);
}

void FGAICarrier::setMaxLong(double deg) {
    _max_lon = fabs(deg);
}

void FGAICarrier::setMinLong(double deg) {
    _min_lon = fabs(deg);
}


void FGAICarrier::setDeckAltitudeFt(const double altitude_feet) {
    _deck_altitude_ft = altitude_feet;
}

void FGAICarrier::setSign(const string& s) {
    _sign = s;
}

void FGAICarrier::setTACANChannelID(const string& id) {
    _TACAN_channel_id = id;
}

void FGAICarrier::setMPControl(bool c) {
    _MPControl = c;
}

void FGAICarrier::setAIControl(bool c) {
    _AIControl = c;
}

void FGAICarrier::update(double dt) {
    // Now update the position and heading. This will compute new hdg and
    // roll values required for the rotation speed computation.
    FGAIShip::update(dt);
    
    if (_is_user_craft->getBoolValue()) {
        _latitude_node->setDoubleValue(pos.getLatitudeDeg());
        _longitude_node->setDoubleValue(pos.getLongitudeDeg());
        _altitude_node->setDoubleValue(pos.getElevationFt());
        _heading_node->setDoubleValue(hdg);
        _pitch_node->setDoubleValue(pitch);
        _roll_node->setDoubleValue(roll);
    }

    //automatic turn into wind with a target wind of 25 kts otd
    //SG_LOG(SG_AI, SG_ALERT, "AICarrier: MPControl " << MPControl << " AIControl " << AIControl);
    if (strcmp(_ai_latch_node->getStringValue(), "")) {
        SG_LOG(SG_AI, SG_DEBUG, "FGAICarrier::update(): not updating because ai-latch=" << _ai_latch_node->getStringValue());
    }
    else if (!_MPControl && _AIControl){

        if(_turn_to_launch_hdg){
            TurnToLaunch();
        } else if(_turn_to_recovery_hdg ){
            TurnToRecover();
        } else if(OutsideBox() || _returning ) {// check that the carrier is inside
            ReturnToBox();                     // the operating box,
        } else {
            TurnToBase();
        }

    } else {
        FGAIShip::TurnTo(tgt_heading);
        FGAIShip::AccelTo(tgt_speed);
    }

    UpdateWind(dt);
    UpdateElevator(dt);
    UpdateJBD(dt);

    // Transform that one to the horizontal local coordinate system.
    SGQuatd ec2hl = SGQuatd::fromLonLat(pos);
    // The orientation of the carrier wrt the horizontal local frame
    SGQuatd hl2body = SGQuatd::fromYawPitchRollDeg(hdg, pitch, roll);
    // and postrotate the orientation of the AIModel wrt the horizontal
    // local frame
    SGQuatd ec2body = ec2hl * hl2body;
    // The cartesian position of the carrier in the wgs84 world
    SGVec3d cartPos = SGVec3d::fromGeod(pos);

    // The position of the eyepoint - at least near that ...
    SGVec3d eyePos(globals->get_ownship_reference_position_cart());
    // Add the position offset of the AIModel to gain the earth
    // centered position
    SGVec3d eyeWrtCarrier = eyePos - cartPos;
    // rotate the eyepoint wrt carrier vector into the carriers frame
    eyeWrtCarrier = ec2body.transform(eyeWrtCarrier);
    // the eyepoints vector wrt the flols position
    SGVec3d eyeWrtFlols = eyeWrtCarrier - _flolsPosOffset;

    SGVec3d flols_location = getCartPosAt(_flolsPosOffset);

    // the distance from the eyepoint to the flols
    _flols_dist = norm(eyeWrtFlols);

    // lineup (left/right) - stern lights and Carrier landing system (Aircraft/Generic/an_spn_46.nas)
    double lineup_hdg, lineup_az2, lineup_s;

    SGGeod g_eyePos = SGGeod::fromCart(eyePos);
    SGGeod g_carrier = SGGeod::fromCart(cartPos);

    //
    // set the view as requested by control/view-index.
    SGGeod viewPosition;
    switch (_view_index) {
    default:
    case 0:
        viewPosition = SGGeod::fromCart(getCartPosAt(_towerPosition));
        break;

    case 1:
        viewPosition = SGGeod::fromCart(getCartPosAt(_flolsTouchdownPosition));
        break;

    case 2:
        viewPosition = SGGeod::fromCart(getCartPosAt(_lsoPosition));
        break;
    }
    _view_position_lat_deg_node->setDoubleValue(viewPosition.getLatitudeDeg());
    _view_position_lon_deg_node->setDoubleValue(viewPosition.getLongitudeDeg());
    _view_position_alt_ft_node->setDoubleValue(viewPosition.getElevationFt());

    SGGeodesy::inverse(g_carrier, g_eyePos, lineup_hdg, lineup_az2, lineup_s);

    double target_lineup = _getHeading() + _angled_deck_degrees + 180.0;
    SG_NORMALIZE_RANGE(target_lineup, 0.0, 360.0);

    _lineup = lineup_hdg - target_lineup;
    // now the angle, positive angles are upwards
    if (fabs(_flols_dist) < SGLimits<double>::min()) {
        _flols_angle = 0;
    } else {
        double sAngle = -eyeWrtFlols(2) / _flols_dist;
        sAngle        = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
        _flols_angle         = SGMiscd::rad2deg(asin(sAngle));
    }
    if (_flols_dist < 8000){
        SGVec3d eyeWrtFlols_tdp = eyeWrtCarrier - _flolsTouchdownPosition;

        // the distance from the eyepoint to the flols
        double dist_tdp = norm(eyeWrtFlols_tdp);
        double angle_tdp = 0;

        // now the angle, positive angles are upwards
        if (fabs(dist_tdp) < SGLimits<double>::min()) {
            angle_tdp = 0;
        }
        else {
            double sAngle = -eyeWrtFlols_tdp(2) / dist_tdp;
            sAngle = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
            angle_tdp = SGMiscd::rad2deg(asin(sAngle));
        }
//        printf("angle %5.2f td angle %5.2f \n", _flols_angle, angle_tdp);
        //angle += 1.481; // adjust for FLOLS offset (measured on Nimitz class)
    }

    // set the value of _flols_visible_light
    if ( _flols_angle <= 4.35 && _flols_angle > 4.01 )
      _flols_visible_light = 1;
    else if ( _flols_angle <= 4.01 && _flols_angle > 3.670 )
      _flols_visible_light = 2;
    else if ( _flols_angle <= 3.670 && _flols_angle > 3.330 )
      _flols_visible_light = 3;
    else if ( _flols_angle <= 3.330 && _flols_angle > 2.990 )
      _flols_visible_light = 4;
    else if ( _flols_angle <= 2.990 && _flols_angle > 2.650 )
      _flols_visible_light = 5;
    else if ( _flols_angle <= 2.650 )
      _flols_visible_light = 6;
    else
      _flols_visible_light = 0;
      
    // only bother with waveoff FLOLS when ownship within a reasonable range.
    // red ball is <= 3.075 to 2.65, below this is off. above this is orange.
    // only do this when within ~1.8nm
    if (_flols_dist < 3200) {
        if (_flols_dist > 100) {
            bool new_wave_off_lights_demand = (_flols_angle <= 3.0);

            if (new_wave_off_lights_demand != _wave_off_lights_demand) {
                // start timing when the lights come up.
                _wave_off_lights_demand = new_wave_off_lights_demand;
            }

            //// below 1degrees close in is to low to continue; wave them off.
            if (_flols_angle < 2 && _flols_dist < 800) {
                _wave_off_lights_demand = true;
            }
        }
    }
    else {
        _wave_off_lights_demand = true; // sensible default when very far away.
    }
} //end update

bool FGAICarrier::init(ModelSearchOrder searchOrder) {
    if (!FGAIShip::init(searchOrder))
        return false;

    _longitude_node = fgGetNode("/position/longitude-deg", true);
    _latitude_node = fgGetNode("/position/latitude-deg", true);
    _altitude_node = fgGetNode("/position/altitude-ft", true);
    _heading_node = fgGetNode("/orientation/true-heading-deg", true);
    _pitch_node = fgGetNode("/orientation/pitch-deg", true);
    _roll_node = fgGetNode("/orientation/roll-deg", true);

    _launchbar_state_node = fgGetNode("/gear/launchbar/state", true);

    _surface_wind_from_deg_node = fgGetNode("/environment/config/boundary/entry[0]/wind-from-heading-deg", true);
    _surface_wind_speed_node = fgGetNode("/environment/config/boundary/entry[0]/wind-speed-kt", true);


    int dmd_course = fgGetInt("/sim/presets/carrier-course");
    if (dmd_course == 2) {
        // launch
        _turn_to_launch_hdg   = true;
        _turn_to_recovery_hdg = false;
        _turn_to_base_course  = false;
    } else if (dmd_course == 3) {
        //  recovery
        _turn_to_launch_hdg   = false;
        _turn_to_recovery_hdg = true;
        _turn_to_base_course  = false;
    } else {
        // default to base
        _turn_to_launch_hdg   = false;
        _turn_to_recovery_hdg = false;
        _turn_to_base_course  = true;
    }

    _returning = false;
    _in_to_wind = false;

    _mOpBoxPos = pos;
    _base_course = hdg;
    _base_speed = speed;

    _elevator_pos_norm = 0;
    _elevator_pos_norm_raw = 0;
    _elevators = false;
    _elevator_transition_time = 150;
    _elevator_time_constant = 0.005;
    _jbd_elevator_pos_norm = 0;
    _jbd_elevator_pos_norm_raw = 0;
    _jbd = false ;
    _jbd_transition_time = 3;
    _jbd_time_constant = 0.1;
    return true;
}

void FGAICarrier::bind(){
    FGAIShip::bind();
    _is_user_craft = props->getNode("is-user-craft", true /*create*/);
    _ai_latch_node = props->getNode("ai-latch", true /*create*/);
    
    props->untie("velocities/true-airspeed-kt");

    props->getNode("position/deck-altitude-feet", true)->setDoubleValue(_deck_altitude_ft);

    tie("controls/flols/source-lights",
        SGRawValuePointer<int>(&_flols_visible_light));
    tie("controls/flols/distance-m",
        SGRawValuePointer<double>(&_flols_dist));
    tie("controls/flols/angle-degs",
        SGRawValuePointer<double>(&_flols_angle));
    tie("controls/flols/lineup-degs",
        SGRawValuePointer<double>(&_lineup));
    tie("controls/turn-to-launch-hdg",
        SGRawValuePointer<bool>(&_turn_to_launch_hdg));
    tie("controls/in-to-wind",
        SGRawValuePointer<bool>(&_turn_to_launch_hdg));
    tie("controls/base-course-deg",
        SGRawValuePointer<double>(&_base_course));
    tie("controls/base-speed-kts",
        SGRawValuePointer<double>(&_base_speed));
    tie("controls/start-pos-lat-deg",
        SGRawValueMethods<SGGeod,double>(pos, &SGGeod::getLatitudeDeg));
    tie("controls/start-pos-long-deg",
        SGRawValueMethods<SGGeod,double>(pos, &SGGeod::getLongitudeDeg));
    tie("controls/mp-control",
        SGRawValuePointer<bool>(&_MPControl));
    tie("controls/ai-control",
        SGRawValuePointer<bool>(&_AIControl));
    tie("environment/surface-wind-speed-true-kts",
        SGRawValuePointer<double>(&_wind_speed_kts));
    tie("environment/surface-wind-from-true-degs",
        SGRawValuePointer<double>(&_wind_from_deg));
    tie("environment/rel-wind-from-degs",
        SGRawValuePointer<double>(&_rel_wind_from_deg));
    tie("environment/rel-wind-from-carrier-hdg-degs",
        SGRawValuePointer<double>(&_rel_wind));
    tie("environment/rel-wind-speed-kts",
        SGRawValuePointer<double>(&_rel_wind_speed_kts));
    tie("environment/in-to-wind",
        SGRawValuePointer<bool>(&_in_to_wind));
    tie("controls/flols/wave-off-lights-demand",
        SGRawValuePointer<bool>(&_wave_off_lights_demand));
    tie("controls/elevators",
        SGRawValuePointer<bool>(&_elevators));
    tie("surface-positions/elevators-pos-norm",
        SGRawValuePointer<double>(&_elevator_pos_norm));
    tie("controls/constants/elevators/trans-time-s",
        SGRawValuePointer<double>(&_elevator_transition_time));
    tie("controls/constants/elevators/time-constant",
        SGRawValuePointer<double>(&_elevator_time_constant));
    tie("controls/jbd",
        SGRawValuePointer<bool>(&_jbd));
    tie("surface-positions/jbd-pos-norm",
        SGRawValuePointer<double>(&_jbd_elevator_pos_norm));
    tie("controls/constants/jbd/trans-time-s",
        SGRawValuePointer<double>(&_jbd_transition_time));
    tie("controls/constants/jbd/time-constant",
        SGRawValuePointer<double>(&_jbd_time_constant));
    tie("controls/turn-to-recovery-hdg",
        SGRawValuePointer<bool>(&_turn_to_recovery_hdg));
    tie("controls/turn-to-base-course",
        SGRawValuePointer<bool>(&_turn_to_base_course));

    tie("controls/view-index", SGRawValuePointer<int>(&_view_index));

    props->setBoolValue("controls/flols/cut-lights", false);
    props->setBoolValue("controls/flols/wave-off-lights", false);
    props->setBoolValue("controls/flols/wave-off-lights-emergency", false);
    props->setBoolValue("controls/flols/cond-datum-lights", true);
    props->setBoolValue("controls/crew", false);
    props->setStringValue("navaids/tacan/channel-ID", _TACAN_channel_id.c_str());
    props->setStringValue("sign", _sign.c_str());
    props->setBoolValue("controls/lighting/deck-lights", false);
    props->setDoubleValue("controls/lighting/flood-lights-red-norm", 0);

    _flols_x_node = props->getNode("position/flols-x", true);
    _flols_y_node = props->getNode("position/flols-y", true);
    _flols_z_node = props->getNode("position/flols-z", true);

    _view_position_lat_deg_node = props->getNode("position/view-position-lat", true);
    _view_position_lon_deg_node = props->getNode("position/view-position-lon", true);
    _view_position_alt_ft_node = props->getNode("position/view-position-alt", true);

    // Write out a list of the parking positions - useful for the UI to select
    // from
    for (const auto& ppos : _ppositions) {
        if (ppos.name != "") props->addChild("parking-pos")->setStringValue("name", ppos.name);
    }
}

bool FGAICarrier::getParkPosition(const string& id, SGGeod& geodPos,
                                  double& hdng, SGVec3d& uvw)
{

    // FIXME: does not yet cover rotation speeds.
    for (const auto& ppos : _ppositions) {
        // Take either the specified one or the first one ...
        if (ppos.name == id || id.empty()) {
            SGVec3d cartPos = getCartPosAt(ppos.offset);
            geodPos = SGGeod::fromCart(cartPos);
            hdng = hdg + ppos.heading_deg;
            double shdng = sin(ppos.heading_deg * SGD_DEGREES_TO_RADIANS);
            double chdng = cos(ppos.heading_deg * SGD_DEGREES_TO_RADIANS);
            double speed_fps = speed*1.6878099;
            uvw = SGVec3d(chdng*speed_fps, shdng*speed_fps, 0);
            return true;
        }
    }

    return false;
}

bool FGAICarrier::getFLOLSPositionHeading(SGGeod& geodPos, double& heading) const
{
    SGVec3d cartPos = getCartPosAt(_flolsPosOffset);
    geodPos = SGGeod::fromCart(cartPos);

    // at present we don't support a heading offset for the FLOLS, so
    // heading is just the carrier heading
    heading = hdg + _flolsHeadingOffsetDeg;

    return true;
}

double FGAICarrier::getFLOLFSGlidepathAngleDeg() const
{
    return _flolsApproachAngle;
}

// find relative wind
void FGAICarrier::UpdateWind( double dt) {

    //get the surface wind speed and direction
    _wind_from_deg = _surface_wind_from_deg_node->getDoubleValue();
    _wind_speed_kts  = _surface_wind_speed_node->getDoubleValue();

    //calculate the surface wind speed north and east in kts
    double wind_speed_from_north_kts = cos( _wind_from_deg / SGD_RADIANS_TO_DEGREES )* _wind_speed_kts ;
    double wind_speed_from_east_kts  = sin( _wind_from_deg / SGD_RADIANS_TO_DEGREES )* _wind_speed_kts ;

    //calculate the carrier speed north and east in kts
    double speed_north_kts = cos( hdg / SGD_RADIANS_TO_DEGREES )* speed ;
    double speed_east_kts  = sin( hdg / SGD_RADIANS_TO_DEGREES )* speed ;

    //calculate the relative wind speed north and east in kts
    double rel_wind_speed_from_east_kts = wind_speed_from_east_kts + speed_east_kts;
    double rel_wind_speed_from_north_kts = wind_speed_from_north_kts + speed_north_kts;

    //combine relative speeds north and east to get relative windspeed in kts
    _rel_wind_speed_kts = sqrt((rel_wind_speed_from_east_kts * rel_wind_speed_from_east_kts) 
                       + (rel_wind_speed_from_north_kts * rel_wind_speed_from_north_kts));

    //calculate the relative wind direction
    _rel_wind_from_deg = SGMiscd::rad2deg(atan2(rel_wind_speed_from_east_kts, rel_wind_speed_from_north_kts));

    //calculate rel wind
    _rel_wind = _rel_wind_from_deg - hdg;
    SG_NORMALIZE_RANGE(_rel_wind, -180.0, 180.0);

    //set in to wind property
    InToWind();

    //switch the wave-off lights
    //if (InToWind())
    //   wave_off_lights = false;
    //else
    //   wave_off_lights = true;

    // cout << "rel wind: " << rel_wind << endl;

}// end update wind


void FGAICarrier::TurnToLaunch(){

    // calculate tgt heading
    if (_wind_speed_kts < 3){
        tgt_heading = _base_course;
    } else {
        tgt_heading = _wind_from_deg;
    }

    //calculate tgt speed
    double tgt_speed = 25 - _wind_speed_kts;
    if (tgt_speed < 10)
        tgt_speed = 10;

    //turn the carrier
    FGAIShip::TurnTo(tgt_heading);
    FGAIShip::AccelTo(tgt_speed);

}

void FGAICarrier::TurnToRecover(){

    //these are the rules for adjusting heading to provide a relative wind
    //down the angled flightdeck

    if (_wind_speed_kts < 3){
        tgt_heading = _base_course + 60;
    } else if (_rel_wind < -9 && _rel_wind >= -180){
        tgt_heading = _wind_from_deg;
    } else if (_rel_wind > -7 && _rel_wind < 45){
        tgt_heading = _wind_from_deg + 60;
    } else if (_rel_wind >=45 && _rel_wind < 180){
        tgt_heading = _wind_from_deg + 45;
    } else
        tgt_heading = hdg;

    SG_NORMALIZE_RANGE(tgt_heading, 0.0, 360.0);

    //calculate tgt speed
    double tgt_speed = 26 - _wind_speed_kts;
    if (tgt_speed < 10)
        tgt_speed = 10;

    //turn the carrier
    FGAIShip::TurnTo(tgt_heading);
    FGAIShip::AccelTo(tgt_speed);

}
void FGAICarrier::TurnToBase(){

    //turn the carrier
    FGAIShip::TurnTo(_base_course);
    FGAIShip::AccelTo(_base_speed);

}


void FGAICarrier::ReturnToBox(){
    double course, distance, az2;

    //calculate the bearing and range of the initial position from the carrier
    geo_inverse_wgs_84(pos, _mOpBoxPos, &course, &az2, &distance);

    distance *= SG_METER_TO_NM;

    //cout << "return course: " << course << " distance: " << distance << endl;
    //turn the carrier
    FGAIShip::TurnTo(course);
    FGAIShip::AccelTo(_base_speed);

    if (distance >= 1)
        _returning = true;
    else
        _returning = false;

} //  end turn to base


bool FGAICarrier::OutsideBox() { //returns true if the carrier is outside operating box

    if ( _max_lat == 0 && _min_lat == 0 && _max_lon == 0 && _min_lon == 0) {
        SG_LOG(SG_AI, SG_DEBUG, "AICarrier: No Operating Box defined" );
        return false;
    }

    if (_mOpBoxPos.getLatitudeDeg() >= 0) { //northern hemisphere
        if (pos.getLatitudeDeg() >= _mOpBoxPos.getLatitudeDeg() + _max_lat)
            return true;

        if (pos.getLatitudeDeg() <= _mOpBoxPos.getLatitudeDeg() - _min_lat)
            return true;

    } else {                  //southern hemisphere
        if (pos.getLatitudeDeg() <= _mOpBoxPos.getLatitudeDeg() - _max_lat)
            return true;

        if (pos.getLatitudeDeg() >= _mOpBoxPos.getLatitudeDeg() + _min_lat)
            return true;
    }

    if (_mOpBoxPos.getLongitudeDeg() >=0) { //eastern hemisphere
        if (pos.getLongitudeDeg() >= _mOpBoxPos.getLongitudeDeg() + _max_lon)
            return true;

        if (pos.getLongitudeDeg() <= _mOpBoxPos.getLongitudeDeg() - _min_lon)
            return true;

    } else {                 //western hemisphere
        if (pos.getLongitudeDeg() <= _mOpBoxPos.getLongitudeDeg() - _max_lon)
            return true;

        if (pos.getLongitudeDeg() >= _mOpBoxPos.getLongitudeDeg() + _min_lon)
            return true;
    }

    return false;

} // end OutsideBox


bool FGAICarrier::InToWind() {
    _in_to_wind = false;

    if ( fabs(_rel_wind) < 10 ){
        _in_to_wind = true;
        return true;
    }
    return false;
}


void FGAICarrier::UpdateElevator(double dt) {

    double step = 0;

    if ((_elevators && _elevator_pos_norm >= 1 ) || (!_elevators && _elevator_pos_norm <= 0 ))
        return;

    // move the elevators
    if (_elevators ) {
        step = dt / _elevator_transition_time;
        if ( step > 1 )
            step = 1;
    } else {
        step = -dt / _elevator_transition_time;
        if ( step < -1 )
            step = -1;
    }
    // assume a linear relationship
    _elevator_pos_norm_raw += step;

    //low pass filter
    _elevator_pos_norm = (_elevator_pos_norm_raw * _elevator_time_constant) + (_elevator_pos_norm * (1 - _elevator_time_constant));

    //sanitise the output
    if (_elevator_pos_norm_raw >= 1) {
        _elevator_pos_norm_raw = 1;
    } else if (_elevator_pos_norm_raw <= 0) {
        _elevator_pos_norm_raw = 0;
    }
    return;

} // end UpdateElevator

void FGAICarrier::UpdateJBD(double dt) {

    const string launchbar_state = _launchbar_state_node->getStringValue();
    double step = 0;

    if (launchbar_state == "Engaged"){
        _jbd = true;
    } else {
        _jbd = false;
    }

    if ((_jbd && _jbd_elevator_pos_norm >= 1 ) || ( !_jbd && _jbd_elevator_pos_norm <= 0 )){
        return;
    }

    // move the jbds
    if ( _jbd ) {
        step = dt / _jbd_transition_time;
        if ( step > 1 )
            step = 1;
    } else {
        step = -dt / _jbd_transition_time;
        if ( step < -1 )
            step = -1;
    }

    // assume a linear relationship
    _jbd_elevator_pos_norm_raw += step;

    //low pass filter
    _jbd_elevator_pos_norm = (_jbd_elevator_pos_norm_raw * _jbd_time_constant) + (_jbd_elevator_pos_norm * (1 - _jbd_time_constant));

    //sanitise the output
    if (_jbd_elevator_pos_norm >= 1) {
        _jbd_elevator_pos_norm = 1;
    } else if (_jbd_elevator_pos_norm <= 0) {
        _jbd_elevator_pos_norm = 0;
    }

    return;

} // end UpdateJBD

std::pair<bool, SGGeod> FGAICarrier::initialPositionForCarrier(const std::string& namePennant)
{
    FGAIManager::registerScenarios();
    // this is actually a three-layer search (we want the scenario with the
    // carrier with the correct penanant or name. Sometimes an XPath for
    // properties would be quite handy :)

    for (auto s : fgGetNode("/sim/ai/scenarios")->getChildren("scenario")) {
        auto carriers = s->getChildren("carrier");
        auto it = std::find_if(carriers.begin(), carriers.end(),
                               [namePennant] (const SGPropertyNode* n)
        {
            // don't want to use a recursive lambda here, so inner search is a flat loop
            for (auto nameChild : n->getChildren("name")) {
                if (nameChild->getStringValue() == namePennant) return true;
            }
            return false;
        });
        if (it == carriers.end()) {
            continue;
        }

        // mark the scenario for loading (which will happen in post-init of the AIManager)
        fgGetNode("/sim/ai/")->addChild("scenario")->setStringValue(s->getStringValue("id"));

        // read out the initial-position
        SGGeod geod = SGGeod::fromDeg((*it)->getDoubleValue("longitude"),
                                      (*it)->getDoubleValue("latitude"));
        return std::make_pair(true,  geod);
    } // of scenarios iteration

    return std::make_pair(false, SGGeod());
}

SGSharedPtr<FGAICarrier> FGAICarrier::findCarrierByNameOrPennant(const std::string& namePennant)
{
    const FGAIManager* aiManager = globals->get_subsystem<FGAIManager>();
    if (!aiManager) {
        return {};
    }

    for (const auto& aiObject : aiManager->get_ai_list()) {
        if (aiObject->isa(FGAIBase::otCarrier)) {
            SGSharedPtr<FGAICarrier> c = static_cast<FGAICarrier*>(aiObject.get());
            if ((c->_sign == namePennant) || (c->_getName() == namePennant)) {
                return c;
            }
        }
    } // of all objects iteration

    return {};
}

void FGAICarrier::extractCarriersFromScenario(SGPropertyNode_ptr xmlNode, SGPropertyNode_ptr scenario)
{
    for (auto c : xmlNode->getChildren("entry")) {
        if (c->getStringValue("type") != std::string("carrier"))
            continue;

        const std::string name = c->getStringValue("name");
        const std::string pennant = c->getStringValue("pennant-number");
        if (name.empty() && pennant.empty()) {
            continue;
        }

        SGPropertyNode_ptr carrierNode = scenario->addChild("carrier");

        // extract the initial position from the scenario
        carrierNode->setDoubleValue("longitude", c->getDoubleValue("longitude"));
        carrierNode->setDoubleValue("latitude", c->getDoubleValue("latitude"));

        // A description of the carrier is also available from the entry.  Primarily for use by the launcher
        carrierNode->setStringValue("description", c->getStringValue("description"));

        // the find code above just looks for anything called a name (so alias
        // are possible, for example)
        if (!name.empty()) carrierNode->addChild("name")->setStringValue(name);
        if (!pennant.empty()) {
            carrierNode->addChild("name")->setStringValue(pennant);
            carrierNode->addChild("pennant-number")->setStringValue(pennant);
        }

        // extact parkings
        for (auto p : c->getChildren("parking-pos")) {
            carrierNode->addChild("parking-pos")->setStringValue(p->getStringValue("name"));
        }
    }
}

simgear::Emesary::ReceiptStatus FGAICarrier::Receive(simgear::Emesary::INotificationPtr n)
{
    auto nctn = dynamic_pointer_cast<NearestCarrierToNotification>(n);

    if (nctn) {
        if (!nctn->GetCarrier() || nctn->GetDistanceMeters() > nctn->GetDistanceToMeters(pos)) {
            nctn->SetCarrier(this, &pos);
            nctn->SetViewPositionLatNode(_view_position_lat_deg_node);
            nctn->SetViewPositionLonNode(_view_position_lon_deg_node);
            nctn->SetViewPositionAltNode(_view_position_alt_ft_node);
            nctn->SetDeckheight(_deck_altitude_ft);
            nctn->SetHeading(hdg);
            nctn->SetVckts(speed);
            nctn->SetCarrierIdent(this->_getName());
        }
        return simgear::Emesary::ReceiptStatus::OK;
    }
    return simgear::Emesary::ReceiptStatus::NotProcessed;
}
