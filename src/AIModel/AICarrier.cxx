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

FGAICarrier::FGAICarrier() : FGAIShip(otCarrier),
                             deck_altitude_ft(65.0065),
    AIControl(false),
    MPControl(false),
    angle(0),
    base_course(0), 
    base_speed(0),
    dist(0),
    elevators(false),
    in_to_wind(false),
    jbd(false),
    jbd_pos_norm(0),
    jbd_time_constant(0),
    jbd_transition_time(0),
    lineup(0),
    max_lat(0),
    max_long(0),
    min_lat(0),
    min_long(0),
    pos_norm(0),
    raw_jbd_pos_norm(0),
    raw_pos_norm(0),
    rel_wind(0),
    rel_wind_from_deg(0),
    rel_wind_speed_kts(0),
    returning(false),
    source(0),
    time_constant(0),
     transition_time(0),
    turn_to_base_course(true),
    turn_to_recovery_hdg(true),
    turn_to_launch_hdg(true),
    view_index(0),
    wave_off_lights_demand(false),
    wind_from_deg(0),
    wind_from_east(0),
    wind_from_north(0),
    wind_speed_kts(0)
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
  
  angled_deck_degrees = scFileNode->getDoubleValue("angled-deck-degrees", -8.5);

    SGPropertyNode* flolsNode = getPositionFromNode(scFileNode, "flols-pos", _flolsPosOffset);
    if (flolsNode) {
        _flolsHeadingOffsetDeg = flolsNode->getDoubleValue("heading-offset-deg", 0.0);
        _flolsApproachAngle = flolsNode->getDoubleValue("glidepath-angle-deg", 3.5);
    }
    else {
        _flolsPosOffset(2) = -(deck_altitude_ft * SG_FEET_TO_METER + 10);
    }
    
    //// the FLOLS (or IFLOLS) position doesn't produce an accurate angle;
    //// so to fix this we can definition the touchdown position which 
    //// is the centreline of the 3rd wire

    _flolsTouchdownPosition = _flolsPosOffset; // default to the flolsPosition
    getPositionFromNode(scFileNode, "flols-touchdown-position", _flolsTouchdownPosition);

    if (!getPositionFromNode(scFileNode, "tower-position", _towerPosition)) {
        _towerPosition(2) = -(deck_altitude_ft * SG_FEET_TO_METER + 10);
        SG_LOG(SG_AI, SG_INFO, "AICarrier: tower-position not defined - using default");
    }

    if (!getPositionFromNode(scFileNode, "lso-position", _lsoPosition)){
        _lsoPosition(2) = -(deck_altitude_ft * SG_FEET_TO_METER + 10);
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
    ppositions.push_back(pp);
  }
}

void FGAICarrier::setWind_from_east(double fps) {
    wind_from_east = fps;
}

void FGAICarrier::setWind_from_north(double fps) {
    wind_from_north = fps;
}

void FGAICarrier::setMaxLat(double deg) {
    max_lat = fabs(deg);
}

void FGAICarrier::setMinLat(double deg) {
    min_lat = fabs(deg);
}

void FGAICarrier::setMaxLong(double deg) {
    max_long = fabs(deg);
}

void FGAICarrier::setMinLong(double deg) {
    min_long = fabs(deg);
}


void FGAICarrier::setDeckAltitudeFt(const double altitude_feet) {
    deck_altitude_ft = altitude_feet;
}

void FGAICarrier::setSign(const string& s) {
    sign = s;
}

void FGAICarrier::setTACANChannelID(const string& id) {
    TACAN_channel_id = id;
}

void FGAICarrier::setMPControl(bool c) {
    MPControl = c;
}

void FGAICarrier::setAIControl(bool c) {
    AIControl = c;
}

void FGAICarrier::update(double dt) {
    // Now update the position and heading. This will compute new hdg and
    // roll values required for the rotation speed computation.
    FGAIShip::update(dt);

    //automatic turn into wind with a target wind of 25 kts otd
    //SG_LOG(SG_AI, SG_ALERT, "AICarrier: MPControl " << MPControl << " AIControl " << AIControl);
    if (!MPControl && AIControl){

        if(turn_to_launch_hdg){
            TurnToLaunch();
        } else if(turn_to_recovery_hdg ){
            TurnToRecover();
        } else if(OutsideBox() || returning ) {// check that the carrier is inside
            ReturnToBox();                     // the operating box,
        } else {
            TurnToBase();
        }

    } else {
        FGAIShip::TurnTo(tgt_heading);
        FGAIShip::AccelTo(tgt_speed);
    }

    UpdateWind(dt);
    UpdateElevator(dt, transition_time);
    UpdateJBD(dt, jbd_transition_time);

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
    dist = norm(eyeWrtFlols);

    // lineup (left/right) - stern lights and Carrier landing system (Aircraft/Generic/an_spn_46.nas)
    double lineup_hdg, lineup_az2, lineup_s;

    SGGeod g_eyePos = SGGeod::fromCart(eyePos);
    SGGeod g_carrier = SGGeod::fromCart(cartPos);

    //
    // set the view as requested by control/view-index.
    SGGeod viewPosition;
    switch (view_index) {
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

    double target_lineup = _getHeading() + angled_deck_degrees + 180.0;
    SG_NORMALIZE_RANGE(target_lineup, 0.0, 360.0);

    lineup = lineup_hdg - target_lineup;
    // now the angle, positive angles are upwards
    if (fabs(dist) < SGLimits<double>::min()) {
        angle = 0;
    } else {
        double sAngle = -eyeWrtFlols(2) / dist;
        sAngle        = SGMiscd::min(1, SGMiscd::max(-1, sAngle));
        angle         = SGMiscd::rad2deg(asin(sAngle));
    }
    if (dist < 8000){
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

//        printf("angle %5.2f td angle %5.2f \n", angle, angle_tdp);
        //angle += 1.481; // adjust for FLOLS offset (measured on Nimitz class)
    }

    // set the value of source
    if ( angle <= 4.35 && angle > 4.01 )
      source = 1;
    else if ( angle <= 4.01 && angle > 3.670 )
      source = 2;
    else if ( angle <= 3.670 && angle > 3.330 )
      source = 3;
    else if ( angle <= 3.330 && angle > 2.990 )
      source = 4;
    else if ( angle <= 2.990 && angle > 2.650 )
      source = 5;
    else if ( angle <= 2.650 )
      source = 6;
    else
      source = 0;
      
    // only bother with waveoff FLOLS when ownship within a reasonable range.
    // red ball is <= 3.075 to 2.65, below this is off. above this is orange.
    // only do this when within ~1.8nm
    if (dist < 3200) {
        if (dist > 100) {
            bool new_wave_off_lights_demand = (angle <= 3.0);

            if (new_wave_off_lights_demand != wave_off_lights_demand) {
                // start timing when the lights come up.
                wave_off_lights_demand = new_wave_off_lights_demand;
            }

            //// below 1degrees close in is to low to continue; wave them off.
            if (angle < 2 && dist < 800) {
                wave_off_lights_demand = true;
            }
        }
    }
    else {
        wave_off_lights_demand = true; // sensible default when very far away.
    }
} //end update

bool FGAICarrier::init(ModelSearchOrder searchOrder) {
    if (!FGAIShip::init(searchOrder))
        return false;

    _longitude_node = fgGetNode("/position/longitude-deg", true);
    _latitude_node = fgGetNode("/position/latitude-deg", true);
    _altitude_node = fgGetNode("/position/altitude-ft", true);

    _launchbar_state_node = fgGetNode("/gear/launchbar/state", true);

    _surface_wind_from_deg_node =
            fgGetNode("/environment/config/boundary/entry[0]/wind-from-heading-deg", true);
    _surface_wind_speed_node =
            fgGetNode("/environment/config/boundary/entry[0]/wind-speed-kt", true);


    int dmd_course = fgGetInt("/sim/presets/carrier-course");
    if (dmd_course == 2) {
        // launch
        turn_to_launch_hdg   = true;
        turn_to_recovery_hdg = false;
        turn_to_base_course  = false;
    } else if (dmd_course == 3) {
        //  recovery
        turn_to_launch_hdg   = false;
        turn_to_recovery_hdg = true;
        turn_to_base_course  = false;
    } else {
        // default to base
        turn_to_launch_hdg   = false;
        turn_to_recovery_hdg = false;
        turn_to_base_course  = true;
    }

    returning = false;
    in_to_wind = false;

    mOpBoxPos = pos;
    base_course = hdg;
    base_speed = speed;

    pos_norm = raw_pos_norm = 0;
    elevators = false;
    transition_time = 150;
    time_constant = 0.005;
    jbd_pos_norm = raw_jbd_pos_norm = 0;
    jbd = false ;
    jbd_transition_time = 3;
    jbd_time_constant = 0.1;
    return true;
}

void FGAICarrier::bind(){
    FGAIShip::bind();

    props->untie("velocities/true-airspeed-kt");

    props->getNode("position/deck-altitude-feet", true)->setDoubleValue(deck_altitude_ft);

    tie("controls/flols/source-lights",
        SGRawValuePointer<int>(&source));
    tie("controls/flols/distance-m",
        SGRawValuePointer<double>(&dist));
    tie("controls/flols/angle-degs",
        SGRawValuePointer<double>(&angle));
    tie("controls/flols/lineup-degs",
        SGRawValuePointer<double>(&lineup));
    tie("controls/turn-to-launch-hdg",
        SGRawValuePointer<bool>(&turn_to_launch_hdg));
    tie("controls/in-to-wind",
        SGRawValuePointer<bool>(&turn_to_launch_hdg));
    tie("controls/base-course-deg",
        SGRawValuePointer<double>(&base_course));
    tie("controls/base-speed-kts",
        SGRawValuePointer<double>(&base_speed));
    tie("controls/start-pos-lat-deg",
        SGRawValueMethods<SGGeod,double>(pos, &SGGeod::getLatitudeDeg));
    tie("controls/start-pos-long-deg",
        SGRawValueMethods<SGGeod,double>(pos, &SGGeod::getLongitudeDeg));
    tie("controls/mp-control",
        SGRawValuePointer<bool>(&MPControl));
    tie("controls/ai-control",
        SGRawValuePointer<bool>(&AIControl));
    tie("environment/surface-wind-speed-true-kts",
        SGRawValuePointer<double>(&wind_speed_kts));
    tie("environment/surface-wind-from-true-degs",
        SGRawValuePointer<double>(&wind_from_deg));
    tie("environment/rel-wind-from-degs",
        SGRawValuePointer<double>(&rel_wind_from_deg));
    tie("environment/rel-wind-from-carrier-hdg-degs",
        SGRawValuePointer<double>(&rel_wind));
    tie("environment/rel-wind-speed-kts",
        SGRawValuePointer<double>(&rel_wind_speed_kts));
    tie("environment/in-to-wind",
        SGRawValuePointer<bool>(&in_to_wind));
    tie("controls/flols/wave-off-lights-demand",
        SGRawValuePointer<bool>(&wave_off_lights_demand));
    tie("controls/elevators",
        SGRawValuePointer<bool>(&elevators));
    tie("surface-positions/elevators-pos-norm",
        SGRawValuePointer<double>(&pos_norm));
    tie("controls/constants/elevators/trans-time-s",
        SGRawValuePointer<double>(&transition_time));
    tie("controls/constants/elevators/time-constant",
        SGRawValuePointer<double>(&time_constant));
    tie("controls/jbd",
        SGRawValuePointer<bool>(&jbd));
    tie("surface-positions/jbd-pos-norm",
        SGRawValuePointer<double>(&jbd_pos_norm));
    tie("controls/constants/jbd/trans-time-s",
        SGRawValuePointer<double>(&jbd_transition_time));
    tie("controls/constants/jbd/time-constant",
        SGRawValuePointer<double>(&jbd_time_constant));
    tie("controls/turn-to-recovery-hdg",
        SGRawValuePointer<bool>(&turn_to_recovery_hdg));
    tie("controls/turn-to-base-course",
        SGRawValuePointer<bool>(&turn_to_base_course));

    tie("controls/view-index", SGRawValuePointer<int>(&view_index));

    props->setBoolValue("controls/flols/cut-lights", false);
    props->setBoolValue("controls/flols/wave-off-lights", false);
    props->setBoolValue("controls/flols/wave-off-lights-emergency", false);
    props->setBoolValue("controls/flols/cond-datum-lights", true);
    props->setBoolValue("controls/crew", false);
    props->setStringValue("navaids/tacan/channel-ID", TACAN_channel_id.c_str());
    props->setStringValue("sign", sign.c_str());
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
    for (const auto& ppos : ppositions) {
        if (ppos.name != "") props->addChild("parking-pos")->setStringValue("name", ppos.name);
    }
}

bool FGAICarrier::getParkPosition(const string& id, SGGeod& geodPos,
                                  double& hdng, SGVec3d& uvw)
{

    // FIXME: does not yet cover rotation speeds.
    for (const auto& ppos : ppositions) {
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
    wind_from_deg = _surface_wind_from_deg_node->getDoubleValue();
    wind_speed_kts  = _surface_wind_speed_node->getDoubleValue();

    //calculate the surface wind speed north and east in kts
    double wind_speed_from_north_kts = cos( wind_from_deg / SGD_RADIANS_TO_DEGREES )* wind_speed_kts ;
    double wind_speed_from_east_kts  = sin( wind_from_deg / SGD_RADIANS_TO_DEGREES )* wind_speed_kts ;

    //calculate the carrier speed north and east in kts
    double speed_north_kts = cos( hdg / SGD_RADIANS_TO_DEGREES )* speed ;
    double speed_east_kts  = sin( hdg / SGD_RADIANS_TO_DEGREES )* speed ;

    //calculate the relative wind speed north and east in kts
    double rel_wind_speed_from_east_kts = wind_speed_from_east_kts + speed_east_kts;
    double rel_wind_speed_from_north_kts = wind_speed_from_north_kts + speed_north_kts;

    //combine relative speeds north and east to get relative windspeed in kts
    rel_wind_speed_kts = sqrt((rel_wind_speed_from_east_kts * rel_wind_speed_from_east_kts)
    + (rel_wind_speed_from_north_kts * rel_wind_speed_from_north_kts));

    //calculate the relative wind direction
    rel_wind_from_deg = SGMiscd::rad2deg(atan2(rel_wind_speed_from_east_kts, rel_wind_speed_from_north_kts));

    //calculate rel wind
    rel_wind = rel_wind_from_deg - hdg;
    SG_NORMALIZE_RANGE(rel_wind, -180.0, 180.0);

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
    if (wind_speed_kts < 3){
        tgt_heading = base_course;
    } else {
        tgt_heading = wind_from_deg;
    }

    //calculate tgt speed
    double tgt_speed = 25 - wind_speed_kts;
    if (tgt_speed < 10)
        tgt_speed = 10;

    //turn the carrier
    FGAIShip::TurnTo(tgt_heading);
    FGAIShip::AccelTo(tgt_speed);

}

void FGAICarrier::TurnToRecover(){

    //these are the rules for adjusting heading to provide a relative wind
    //down the angled flightdeck

    if (wind_speed_kts < 3){
        tgt_heading = base_course + 60;
    } else if (rel_wind < -9 && rel_wind >= -180){
        tgt_heading = wind_from_deg;
    } else if (rel_wind > -7 && rel_wind < 45){
        tgt_heading = wind_from_deg + 60;
    } else if (rel_wind >=45 && rel_wind < 180){
        tgt_heading = wind_from_deg + 45;
    } else
        tgt_heading = hdg;

    SG_NORMALIZE_RANGE(tgt_heading, 0.0, 360.0);

    //calculate tgt speed
    double tgt_speed = 26 - wind_speed_kts;
    if (tgt_speed < 10)
        tgt_speed = 10;

    //turn the carrier
    FGAIShip::TurnTo(tgt_heading);
    FGAIShip::AccelTo(tgt_speed);

}
void FGAICarrier::TurnToBase(){

    //turn the carrier
    FGAIShip::TurnTo(base_course);
    FGAIShip::AccelTo(base_speed);

}


void FGAICarrier::ReturnToBox(){
    double course, distance, az2;

    //calculate the bearing and range of the initial position from the carrier
    geo_inverse_wgs_84(pos, mOpBoxPos, &course, &az2, &distance);

    distance *= SG_METER_TO_NM;

    //cout << "return course: " << course << " distance: " << distance << endl;
    //turn the carrier
    FGAIShip::TurnTo(course);
    FGAIShip::AccelTo(base_speed);

    if (distance >= 1)
        returning = true;
    else
        returning = false;

} //  end turn to base


bool FGAICarrier::OutsideBox() { //returns true if the carrier is outside operating box

    if ( max_lat == 0 && min_lat == 0 && max_long == 0 && min_long == 0) {
        SG_LOG(SG_AI, SG_DEBUG, "AICarrier: No Operating Box defined" );
        return false;
    }

    if (mOpBoxPos.getLatitudeDeg() >= 0) { //northern hemisphere
        if (pos.getLatitudeDeg() >= mOpBoxPos.getLatitudeDeg() + max_lat)
            return true;

        if (pos.getLatitudeDeg() <= mOpBoxPos.getLatitudeDeg() - min_lat)
            return true;

    } else {                  //southern hemisphere
        if (pos.getLatitudeDeg() <= mOpBoxPos.getLatitudeDeg() - max_lat)
            return true;

        if (pos.getLatitudeDeg() >= mOpBoxPos.getLatitudeDeg() + min_lat)
            return true;
    }

    if (mOpBoxPos.getLongitudeDeg() >=0) { //eastern hemisphere
        if (pos.getLongitudeDeg() >= mOpBoxPos.getLongitudeDeg() + max_long)
            return true;

        if (pos.getLongitudeDeg() <= mOpBoxPos.getLongitudeDeg() - min_long)
            return true;

    } else {                 //western hemisphere
        if (pos.getLongitudeDeg() <= mOpBoxPos.getLongitudeDeg() - max_long)
            return true;

        if (pos.getLongitudeDeg() >= mOpBoxPos.getLongitudeDeg() + min_long)
            return true;
    }

    return false;

} // end OutsideBox


bool FGAICarrier::InToWind() {
    in_to_wind = false;

    if ( fabs(rel_wind) < 10 ){
        in_to_wind = true;
        return true;
    }
    return false;
}


void FGAICarrier::UpdateElevator(double dt, double transition_time) {

    double step = 0;

    if ((elevators && pos_norm >= 1 ) || (!elevators && pos_norm <= 0 ))
        return;

    // move the elevators
    if ( elevators ) {
        step = dt/transition_time;
        if ( step > 1 )
            step = 1;
    } else {
        step = -dt/transition_time;
        if ( step < -1 )
            step = -1;
    }
    // assume a linear relationship
    raw_pos_norm += step;

    //low pass filter
    pos_norm = (raw_pos_norm * time_constant) + (pos_norm * (1 - time_constant));

    //sanitise the output
    if (raw_pos_norm >= 1) {
        raw_pos_norm = 1;
    } else if (raw_pos_norm <= 0) {
        raw_pos_norm = 0;
    }
    return;

} // end UpdateElevator

void FGAICarrier::UpdateJBD(double dt, double jbd_transition_time) {

    const string launchbar_state = _launchbar_state_node->getStringValue();
    double step = 0;

    if (launchbar_state == "Engaged"){
        jbd = true;
    } else {
        jbd = false;
    }

    if (( jbd && jbd_pos_norm >= 1 ) || ( !jbd && jbd_pos_norm <= 0 )){
        return;
    }

    // move the jbds
    if ( jbd ) {
        step = dt/jbd_transition_time;
        if ( step > 1 )
            step = 1;
    } else {
        step = -dt/jbd_transition_time;
        if ( step < -1 )
            step = -1;
    }

    // assume a linear relationship
    raw_jbd_pos_norm += step;

    //low pass filter
    jbd_pos_norm = (raw_jbd_pos_norm * jbd_time_constant) + (jbd_pos_norm * (1 - jbd_time_constant));

    //sanitise the output
    if (jbd_pos_norm >= 1) {
        jbd_pos_norm = 1;
    } else if (jbd_pos_norm <= 0) {
        jbd_pos_norm = 0;
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
            if ((c->sign == namePennant) || (c->_getName() == namePennant)) {
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
            nctn->SetDeckheight(deck_altitude_ft);
            nctn->SetHeading(hdg);
            nctn->SetVckts(speed);
            nctn->SetCarrierIdent(this->_getName());
        }
        return simgear::Emesary::ReceiptStatus::OK;
    }
    return simgear::Emesary::ReceiptStatus::NotProcessed;
}