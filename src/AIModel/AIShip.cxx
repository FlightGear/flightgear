// FGAIShip - FGAIBase-derived class creates an AI ship
//
// Written by David Culp, started October 2003.
// with major amendments and additions by Vivian Meazza, 2004 - 2007
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

#ifdef _MSC_VER
#  include <float.h>
#  define finite _finite
#elif defined(__sun) || defined(sgi)
#  include <ieeefp.h>
#endif

#include <math.h>

#include <simgear/sg_inlines.h>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/math/sg_random.h>

#include <simgear/scene/util/SGNodeMasks.hxx>
#include <Scenery/scenery.hxx>

#include "AIShip.hxx"


FGAIShip::FGAIShip(object_type ot) :
// allow HOT to be enabled
FGAIBase(ot, true),


_waiting(false),
_new_waypoint(true),
_tunnel(false),
_initial_tunnel(false),
_restart(false),
_hdg_constant(0.01),
_limit(100),
_elevation_m(0),
_elevation_ft(0),
_tow_angle(0),
_missed_count(0),
_wp_range(0),
_dt_count(0),
_next_run(0),
_roll_constant(0.001),
_roll_factor(-0.0083335),
_old_range(0),
_range_rate(0),
_missed_time_sec(30),
_day(86400),
_lead_angle(0),
_xtrack_error(0),
_curr_alt(0),
_prev_alt(0),
_until_time(""),
_fp_init(false),
_missed(false)
{
        invisible = false;
}

FGAIShip::~FGAIShip() {
}

void FGAIShip::readFromScenario(SGPropertyNode* scFileNode) {

    if (!scFileNode)
        return;

    FGAIBase::readFromScenario(scFileNode);

    setRudder(scFileNode->getFloatValue("rudder", 0.0));
    setName(scFileNode->getStringValue("name", "Titanic"));
    setRadius(scFileNode->getDoubleValue("turn-radius-ft", 2000));
    std::string flightplan = scFileNode->getStringValue("flightplan");
    setRepeat(scFileNode->getBoolValue("repeat", false));
    setRestart(scFileNode->getBoolValue("restart", false));
    setStartTime(scFileNode->getStringValue("time", ""));
    setLeadAngleGain(scFileNode->getDoubleValue("lead-angle-gain", 1.5));
    setLeadAngleLimit(scFileNode->getDoubleValue("lead-angle-limit-deg", 15));
    setLeadAngleProp(scFileNode->getDoubleValue("lead-angle-proportion", 0.75));
    setRudderConstant(scFileNode->getDoubleValue("rudder-constant", 0.5));
    setFixedTurnRadius(scFileNode->getDoubleValue("fixed-turn-radius-ft", 500));
    setSpeedConstant(scFileNode->getDoubleValue("speed-constant", 0.5));
    setSMPath(scFileNode->getStringValue("submodel-path", ""));
    setRollFactor(scFileNode->getDoubleValue("roll-factor", 1));

    if (!flightplan.empty()) {
        SG_LOG(SG_AI, SG_ALERT, "getting flightplan: " << _name );

        FGAIFlightPlan* fp = new FGAIFlightPlan(flightplan);
        setFlightPlan(fp);
    }

}

bool FGAIShip::init(bool search_in_AI_path) {
    reinit();
    return FGAIBase::init(search_in_AI_path);
}

void FGAIShip::reinit()
{
    prev = 0; // the one behind you
    curr = 0; // the one ahead
    next = 0; // the next plus 1

    props->setStringValue("name", _name.c_str());
    props->setStringValue("waypoint/name-prev", _prev_name.c_str());
    props->setStringValue("waypoint/name-curr", _curr_name.c_str());
    props->setStringValue("waypoint/name-next", _next_name.c_str());
    props->setStringValue("submodels/path", _path.c_str());
    props->setStringValue("waypoint/start-time", _start_time.c_str());
    props->setStringValue("waypoint/wait-until-time", _until_time.c_str());

    _hdg_lock = false;
    _rudder = 0.0;
    no_roll = false;

    _rd_turn_radius_ft = _sp_turn_radius_ft = turn_radius_ft;

    if (fp)
        _fp_init = initFlightPlan();

    FGAIBase::reinit();
}

void FGAIShip::bind() {
    FGAIBase::bind();

    tie("surface-positions/rudder-pos-deg",
        SGRawValuePointer<float>(&_rudder));
    tie("controls/heading-lock",
        SGRawValuePointer<bool>(&_hdg_lock));
    tie("controls/tgt-speed-kts",
        SGRawValuePointer<double>(&tgt_speed));
    tie("controls/tgt-heading-degs",
        SGRawValuePointer<double>(&tgt_heading));
    tie("controls/constants/rudder",
        SGRawValuePointer<double>(&_rudder_constant));
    tie("controls/constants/roll-factor",
        SGRawValuePointer<double>(&_roll_factor));
    tie("controls/constants/roll",
        SGRawValuePointer<double>(&_roll_constant));
    tie("controls/constants/rudder",
        SGRawValuePointer<double>(&_rudder_constant));
    tie("controls/constants/speed",
        SGRawValuePointer<double>(&_speed_constant));
    tie("waypoint/range-nm",
        SGRawValuePointer<double>(&_wp_range));
    tie("waypoint/brg-deg",
        SGRawValuePointer<double>(&_course));
    tie("waypoint/rangerate-nm-sec",
        SGRawValuePointer<double>(&_range_rate));
    tie("waypoint/new",
        SGRawValuePointer<bool>(&_new_waypoint));
    tie("waypoint/missed",
        SGRawValuePointer<bool>(&_missed));
    tie("waypoint/missed-count-sec",
        SGRawValuePointer<double>(&_missed_count));
    tie("waypoint/missed-range-nm",
        SGRawValuePointer<double>(&_missed_range));
    tie("waypoint/missed-time-sec",
        SGRawValuePointer<double>(&_missed_time_sec));
    tie("waypoint/wait-count-sec",
        SGRawValuePointer<double>(&_wait_count));
    tie("waypoint/xtrack-error-ft",
        SGRawValuePointer<double>(&_xtrack_error));
    tie("waypoint/waiting",
        SGRawValuePointer<bool>(&_waiting));
    tie("waypoint/lead-angle-deg",
        SGRawValuePointer<double>(&_lead_angle));
    tie("waypoint/tunnel",
        SGRawValuePointer<bool>(&_tunnel));
    tie("waypoint/alt-curr-m",
        SGRawValuePointer<double>(&_curr_alt));
    tie("waypoint/alt-prev-m",
        SGRawValuePointer<double>(&_prev_alt));
    tie("submodels/serviceable",
        SGRawValuePointer<bool>(&_serviceable));
    tie("controls/turn-radius-ft",
        SGRawValuePointer<double>(&turn_radius_ft));
    tie("controls/turn-radius-corrected-ft",
        SGRawValuePointer<double>(&_rd_turn_radius_ft));
    tie("controls/constants/lead-angle/gain",
        SGRawValuePointer<double>(&_lead_angle_gain));
    tie("controls/constants/lead-angle/limit-deg",
        SGRawValuePointer<double>(&_lead_angle_limit));
    tie("controls/constants/lead-angle/proportion",
        SGRawValuePointer<double>(&_proportion));
    tie("controls/fixed-turn-radius-ft",
        SGRawValuePointer<double>(&_fixed_turn_radius));
    tie("controls/restart",
        SGRawValuePointer<bool>(&_restart));
    tie("velocities/speed-kts",
        SGRawValuePointer<double>(&speed));
}

void FGAIShip::update(double dt) {
    //SG_LOG(SG_AI, SG_ALERT, "updating Ship: " << _name <<hdg<<pitch<<roll);
    // For computation of rotation speeds we just use finite differences here.
    // That is perfectly valid since this thing is not driven by accelerations
    // but by just apply discrete changes at its velocity variables.
    // Update the velocity information stored in those nodes.
    // Transform that one to the horizontal local coordinate system.
    SGQuatd ec2hl = SGQuatd::fromLonLat(pos);
    // The orientation of the ship wrt the horizontal local frame
    SGQuatd hl2body = SGQuatd::fromYawPitchRollDeg(hdg, pitch, roll);
    // and postrotate the orientation of the AIModel wrt the horizontal
    // local frame
    SGQuatd ec2body = ec2hl*hl2body;
    // The cartesian position of the ship in the wgs84 world
    //SGVec3d cartPos = SGVec3d::fromGeod(pos);

    // The simulation time this transform is meant for
    aip.setReferenceTime(globals->get_sim_time_sec());

    // Compute the velocity in m/s in the body frame
    aip.setBodyLinearVelocity(SGVec3d(0.51444444*speed, 0, 0));

    FGAIBase::update(dt);
    Run(dt);
    Transform();
    if (fp)
        setXTrackError();

    // Only change these values if we are able to compute them safely
    if (SGLimits<double>::min() < dt) {
        // Now here is the finite difference ...

        // Transform that one to the horizontal local coordinate system.
        SGQuatd ec2hlNew = SGQuatd::fromLonLat(pos);
        // compute the new orientation
        SGQuatd hl2bodyNew = SGQuatd::fromYawPitchRollDeg(hdg, pitch, roll);
        // The rotation difference
        SGQuatd dOr = inverse(ec2body)*ec2hlNew*hl2bodyNew;
        SGVec3d dOrAngleAxis;
        dOr.getAngleAxis(dOrAngleAxis);
        // divided by the time difference provides a rotation speed vector
        dOrAngleAxis /= dt;

        aip.setBodyAngularVelocity(dOrAngleAxis);
    }
}

void FGAIShip::Run(double dt) {
    if (_fp_init)
        ProcessFlightPlan(dt);

    string type = getTypeString();

    double alpha;
    double rudder_limit;
    double raw_roll;

    // adjust speed
    double speed_diff = tgt_speed - speed;

    if (fabs(speed_diff) > 0.1) {

        if (speed_diff > 0.0)
            speed += _speed_constant * dt;

        if (speed_diff < 0.0)
            speed -= _speed_constant * dt;

    }

    // do not allow unreasonable speeds
    SG_CLAMP_RANGE(speed, -_limit * 0.75, _limit);

    // convert speed to degrees per second
    speed_north_deg_sec = cos(hdg / SGD_RADIANS_TO_DEGREES)
        * speed * 1.686 / ft_per_deg_lat;
    speed_east_deg_sec = sin(hdg / SGD_RADIANS_TO_DEGREES)
        * speed * 1.686 / ft_per_deg_lon;

    // set new position
    //cout << _name << " " << type << " run: " << _elevation_m << " " <<_elevation_ft << endl;
    pos.setLatitudeDeg(pos.getLatitudeDeg() + speed_north_deg_sec * dt);
    pos.setLongitudeDeg(pos.getLongitudeDeg() + speed_east_deg_sec * dt);
    pos.setElevationFt(tgt_altitude_ft);
    pitch = tgt_pitch;

    // adjust heading based on current _rudder angle
    if (turn_radius_ft <= 0)
        turn_radius_ft = 0; // don't allow nonsense values

    if (_rudder > 45)
        _rudder = 45;

    if (_rudder < -45)
        _rudder = -45;


    //we assume that at slow speed ships will manoeuvre using engines/bow thruster
	if(type == "ship" || type == "carrier" || type == "escort"){

		if (fabs(speed)<=5)
			_sp_turn_radius_ft = _fixed_turn_radius;
		else {
			// adjust turn radius for speed. The equation is very approximate.
			// we need to allow for negative speeds
			_sp_turn_radius_ft = 10 * pow ((fabs(speed) - 15), 2) + turn_radius_ft;
		}

	} else {
		
		if (fabs(speed) <= 40)
           _sp_turn_radius_ft = _fixed_turn_radius;
		else {
			// adjust turn radius for speed. 
			_sp_turn_radius_ft = turn_radius_ft;
		}
	}


    if (_rudder <= -0.25 || _rudder >= 0.25) {
        // adjust turn radius for _rudder angle. The equation is even more approximate.
        float a = 19;
        float b = -0.2485;
        float c = 0.543;

        _rd_turn_radius_ft = (a * exp(b * fabs(_rudder)) + c) * _sp_turn_radius_ft;

        // calculate the angle, alpha, subtended by the arc traversed in time dt
        alpha = ((speed * 1.686 * dt) / _rd_turn_radius_ft) * SG_RADIANS_TO_DEGREES;
        //cout << _name << " alpha " << alpha << endl;
        // make sure that alpha is applied in the right direction
        hdg += alpha * sign(_rudder);

        SG_NORMALIZE_RANGE(hdg, 0.0, 360.0);

        //adjust roll for rudder angle and speed. Another bit of voodoo
        raw_roll = _roll_factor * speed * _rudder;
    } else {
        // _rudder angle is 0
        raw_roll = 0;
    }

    //low pass filter
    if (speed < 0)
        roll = -roll;

    roll = (raw_roll * _roll_constant) + (roll * (1 - _roll_constant));

    // adjust target _rudder angle if heading lock engaged
    double rudder_diff = 0.0;
    if (_hdg_lock) {
        double rudder_sense = 0.0;
        double diff = fabs(hdg - tgt_heading);
        //cout << "_rudder diff" << diff << endl;
        if (diff > 180)
            diff = fabs(diff - 360);

        double sum = hdg + diff;

        if (sum > 360.0)
            sum -= 360.0;

        if (fabs(sum - tgt_heading)< 1.0)
            rudder_sense = 1.0;
        else
            rudder_sense = -1.0;

        if (speed < 0)
            rudder_sense = -rudder_sense;

        if (diff < 15)
            _tgt_rudder = diff * rudder_sense;
        else
            _tgt_rudder = 45 * rudder_sense;

        rudder_diff = _tgt_rudder - _rudder;
    }

    // set the _rudder limit by speed
    if (type == "ship" || type == "carrier" || type == "escort"){

        if (speed <= 40)
            rudder_limit = (-0.825 * speed) + 35;
        else
            rudder_limit = 2;

    } else
        rudder_limit = 20;

    if (fabs(rudder_diff)> 0.1) { // apply dead zone

        if (rudder_diff > 0.0) {
            _rudder += _rudder_constant * dt;

            if (_rudder > rudder_limit) // apply the _rudder limit
                _rudder = rudder_limit;

        } else if (rudder_diff < 0.0) {
            _rudder -= _rudder_constant * dt;

            if (_rudder < -rudder_limit)
                _rudder = -rudder_limit;

        }

        // do calculations for radar
        UpdateRadar(manager);
    }
}//end function

void FGAIShip::AccelTo(double speed) {
    tgt_speed = speed;
}

void FGAIShip::PitchTo(double angle) {
    tgt_pitch = angle;
}

void FGAIShip::RollTo(double angle) {
    tgt_roll = angle;
}

void FGAIShip::YawTo(double angle) {
}

void FGAIShip::ClimbTo(double altitude) {
    tgt_altitude_ft = altitude;
    _setAltitude(altitude);
}

void FGAIShip::TurnTo(double heading) {
    tgt_heading = heading - _lead_angle + _tow_angle;
    SG_NORMALIZE_RANGE(tgt_heading, 0.0, 360.0);
    _hdg_lock = true;
}

double FGAIShip::sign(double x) {
    if (x < 0.0)
        return -1.0;
    else
        return 1.0;
}

void FGAIShip::setFlightPlan(FGAIFlightPlan* f) {
    fp = f;
}

void FGAIShip::setStartTime(const string& st) {
    _start_time = st;
}

void FGAIShip::setUntilTime(const string& ut) {
    _until_time = ut;
    props->setStringValue("waypoint/wait-until-time", _until_time.c_str());
}

void FGAIShip::setCurrName(const string& c) {
    _curr_name = c;
    props->setStringValue("waypoint/name-curr", _curr_name.c_str());
}

void FGAIShip::setNextName(const string& n) {
    _next_name = n;
    props->setStringValue("waypoint/name-next", _next_name.c_str());
}

void FGAIShip::setPrevName(const string& p) {
    _prev_name = p;
    props->setStringValue("waypoint/name-prev", _prev_name.c_str());
}

void FGAIShip::setRepeat(bool r) {
    _repeat = r;
}

void FGAIShip::setRestart(bool r) {
    _restart = r;
}

void FGAIShip::setMissed(bool m) {
    _missed = m;
    props->setBoolValue("waypoint/missed", _missed);
}

void FGAIShip::setRudder(float r) {
    _rudder = r;
}

void FGAIShip::setRoll(double rl) {
    roll = rl;
}

void FGAIShip::setLeadAngleGain(double g) {
    _lead_angle_gain = g;
}

void FGAIShip::setLeadAngleLimit(double l) {
    _lead_angle_limit = l;
}

void FGAIShip::setLeadAngleProp(double p) {
    _proportion = p;
}

void FGAIShip::setRudderConstant(double rc) {
    _rudder_constant = rc;
}

void FGAIShip::setSpeedConstant(double sc) {
    _speed_constant = sc;
}

void FGAIShip::setFixedTurnRadius(double ftr) {
    _fixed_turn_radius = ftr;
}

void FGAIShip::setRollFactor(double rf) {
    _roll_factor = rf * -0.0083335;
}

void FGAIShip::setInitialTunnel(bool t) {
    _initial_tunnel = t;
    setTunnel(_initial_tunnel);
}

void FGAIShip::setTunnel(bool t) {
    _tunnel = t;
}

void FGAIShip::setWPNames() {

    if (prev != 0)
        setPrevName(prev->getName());
    else
        setPrevName("");

    if (curr != 0)
        setCurrName(curr->getName());
    else{
        setCurrName("");
        SG_LOG(SG_AI, SG_ALERT, "AIShip: current wp name error" );
    }

    if (next != 0)
        setNextName(next->getName());
    else
        setNextName("");

    SG_LOG(SG_AI, SG_DEBUG, "AIShip: prev wp name " << prev->getName());
    SG_LOG(SG_AI, SG_DEBUG, "AIShip: current wp name " << curr->getName());
    SG_LOG(SG_AI, SG_DEBUG, "AIShip: next wp name " << next->getName());

}

double FGAIShip::getRange(double lat, double lon, double lat2, double lon2) const {

    double course, distance, az2;

    //calculate the bearing and range of the second pos from the first
    geo_inverse_wgs_84(lat, lon, lat2, lon2, &course, &az2, &distance);
    distance *= SG_METER_TO_NM;
    return distance;
}

double FGAIShip::getCourse(double lat, double lon, double lat2, double lon2) const {

    double course, distance, recip;

    //calculate the bearing and range of the second pos from the first
    geo_inverse_wgs_84(lat, lon, lat2, lon2, &course, &recip, &distance);
    if (tgt_speed >= 0) {
        return course;
        SG_LOG(SG_AI, SG_DEBUG, "AIShip: course " << course);
    } else {
        return recip;
        SG_LOG(SG_AI, SG_DEBUG, "AIShip: recip " << recip);
    }
}

void FGAIShip::ProcessFlightPlan(double dt) {

    if ( dt < 0.00001 ) {
	return;
    }

    double time_sec = getDaySeconds();

    _dt_count += dt;

    ///////////////////////////////////////////////////////////////////////////
    // Check Execution time (currently once every 1 sec)
    // Add a bit of randomization to prevent the execution of all flight plans
    // in synchrony, which can add significant periodic framerate flutter.
    ///////////////////////////////////////////////////////////////////////////

    //cout << "_start_sec " << _start_sec << " time_sec " << time_sec << endl;
    if (_dt_count < _next_run && _start_sec < time_sec)
        return;

    _next_run = 0.05 + (0.025 * sg_random());

    double until_time_sec = 0;
    _missed = false;

    // check to see if we've reached the point for our next turn
    // if the range to the waypoint is less than the calculated turn
    // radius we can start the turn to the next leg
    _wp_range = getRange(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr->getLatitude(), curr->getLongitude());
    _range_rate = (_wp_range - _old_range) / _dt_count;
    double sp_turn_radius_nm = _sp_turn_radius_ft / 6076.1155;
    // we need to try to identify a _missed waypoint

    // calculate the time needed to turn through an arc of 90 degrees,
    // and allow a time error
    if (speed != 0)
        _missed_time_sec = 10 + ((SGD_PI * sp_turn_radius_nm * 60 * 60) / (2 * fabs(speed)));
    else
        _missed_time_sec = 10;

    _missed_range = 4 * sp_turn_radius_nm;

    //cout << _name << " range_rate " << _range_rate << " " << _new_waypoint<< endl ;
    //if ((_range_rate > 0) && !_new_waypoint){
    if (_range_rate > 0 && _wp_range < _missed_range && !_new_waypoint){
        _missed_count += _dt_count;
    }

    if (_missed_count >= 120)
        setMissed(true);
    else if (_missed_count >= _missed_time_sec)
        setMissed(true);
    else
        setMissed(false);

    _old_range = _wp_range;
    setWPNames();

    if ((_wp_range < (sp_turn_radius_nm * 1.25)) || _missed || (_waiting && !_new_waypoint)) {

        if (_next_name == "TUNNEL"){
            _tunnel = !_tunnel;

            SG_LOG(SG_AI, SG_DEBUG, "AIShip: " << _name << " " << sp_turn_radius_nm );

            fp->IncrementWaypoint(false);
            next = fp->getNextWaypoint();

            if (next->getName() == "WAITUNTIL" || next->getName() == "WAIT"
                || next->getName() == "END" || next->getName() == "TUNNEL")
                return;

            prev = curr;
            fp->IncrementWaypoint(false);
            curr = fp->getCurrentWaypoint();
            next = fp->getNextWaypoint();

        }else if(_next_name == "END" || fp->getNextWaypoint() == 0) {

            if (_repeat) {
                SG_LOG(SG_AI, SG_INFO, "AIShip: "<< _name << " Flightplan repeating ");
                fp->restart();
                prev = curr;
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
                setWPNames();
                _wp_range = getRange(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr->getLatitude(), curr->getLongitude());
                _old_range = _wp_range;
                _range_rate = 0;
                _new_waypoint = true;
                _missed_count = 0;
                _lead_angle = 0;
                AccelTo(prev->getSpeed());
            } else if (_restart){
                SG_LOG(SG_AI, SG_INFO, "AIShip: " << _name << " Flightplan restarting ");
                _missed_count = 0;
                initFlightPlan();
            } else {
                SG_LOG(SG_AI, SG_ALERT, "AIShip: " << _name << " Flightplan dying ");
                setDie(true);
                _dt_count = 0;
                return;
            }

        } else if (_next_name == "WAIT") {

            if (_wait_count < next->getTime_sec()) {
                SG_LOG(SG_AI, SG_DEBUG, "AIShip: " << _name << " waiting ");
                setSpeed(0);
                _waiting = true;
                _wait_count += _dt_count;
                _dt_count = 0;
                _lead_angle = 0;
                return;
            } else {
                SG_LOG(SG_AI, SG_DEBUG, "AIShip: " << _name
                    << " wait done: getting new waypoints ");
                _waiting = false;
                _wait_count = 0;
                fp->IncrementWaypoint(false);
                next = fp->getNextWaypoint();

                if (next->getName() == "WAITUNTIL" || next->getName() == "WAIT"
                    || next->getName() == "END" || next->getName() == "TUNNEL")
                    return;

                prev = curr;
                fp->IncrementWaypoint(false);
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
            }

        } else if (_next_name == "WAITUNTIL") {
            time_sec = getDaySeconds();
            until_time_sec = processTimeString(next->getTime());
            _until_time = next->getTime();
            setUntilTime(next->getTime());
            if (until_time_sec > time_sec) {
                SG_LOG(SG_AI, SG_INFO, "AIShip: " << _name << " "
                    << curr->getName() << " waiting until: "
                    << _until_time << " " << until_time_sec << " now " << time_sec );
                setSpeed(0);
                _lead_angle = 0;
                _waiting = true;
                return;
            } else {
                SG_LOG(SG_AI, SG_INFO, "AIShip: "
                    << _name << " wait until done: getting new waypoints ");
                setUntilTime("");
                fp->IncrementWaypoint(false);

                while (next->getName() == "WAITUNTIL") {
                    fp->IncrementWaypoint(false);
                    next = fp->getNextWaypoint();
                }

                if (next->getName() == "WAIT")
                    return;

                prev = curr;
                fp->IncrementWaypoint(false);
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
                _waiting = false;
            }

        } else {
            //now reorganise the waypoints, so that next becomes current and so on
            SG_LOG(SG_AI, SG_DEBUG, "AIShip: " << _name << " getting new waypoints ");
            fp->IncrementWaypoint(false);
            prev = fp->getPreviousWaypoint(); //first waypoint
            curr = fp->getCurrentWaypoint();  //second waypoint
            next = fp->getNextWaypoint();     //third waypoint (might not exist!)
        }

        setWPNames();
        _new_waypoint = true;
        _missed_count = 0;
        _range_rate = 0;
        _lead_angle = 0;
        _wp_range = getRange(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr->getLatitude(), curr->getLongitude());
        _old_range = _wp_range;
        setWPPos();
        object_type type = getType();

        if (type != 10)
            AccelTo(prev->getSpeed());

        _curr_alt = curr->getAltitude();
        _prev_alt = prev->getAltitude();

    } else {
        _new_waypoint = false;
    }

    //   now revise the required course for the next way point
    _course = getCourse(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr->getLatitude(), curr->getLongitude());

    if (finite(_course))
        TurnTo(_course);
    else
        SG_LOG(SG_AI, SG_ALERT, "AIShip: Bearing or Range is not a finite number");

    _dt_count = 0;
} // end Processing FlightPlan

bool FGAIShip::initFlightPlan() {

    SG_LOG(SG_AI, SG_ALERT, "AIShip: " << _name << " initializing waypoints ");

    bool init = false;
    _start_sec = 0;
    _tunnel = _initial_tunnel;

    fp->restart();
    fp->IncrementWaypoint(false);

    prev = fp->getPreviousWaypoint(); //first waypoint
    curr = fp->getCurrentWaypoint();  //second waypoint
    next = fp->getNextWaypoint();     //third waypoint (might not exist!)

    while (curr->getName() == "WAIT" || curr->getName() == "WAITUNTIL") {  // don't wait when initialising
        SG_LOG(SG_AI, SG_DEBUG, "AIShip: " << _name << " re-initializing waypoints ");
        fp->IncrementWaypoint(false);
        curr = fp->getCurrentWaypoint();
        next = fp->getNextWaypoint();
    } // end while loop

    if (!_start_time.empty()){
        _start_sec = processTimeString(_start_time);
        double day_sec = getDaySeconds();

        if (_start_sec < day_sec){
            //cout << "flight plan has already started " << _start_time << endl;
            init = advanceFlightPlan(_start_sec, day_sec);

        } else if (_start_sec > day_sec && _repeat) {
            //cout << "flight plan has not started, " << _start_time;
            //cout << "offsetting start time by -24 hrs" << endl;
            _start_sec -= _day;
            init = advanceFlightPlan(_start_sec, day_sec);
        }

        if (init)
            _start_sec = 0; // set to zero for an immediate start of the Flight Plan
        else {
            fp->restart();
            fp->IncrementWaypoint(false);
            prev = fp->getPreviousWaypoint();
            curr = fp->getCurrentWaypoint();
            next = fp->getNextWaypoint();
            return false;
        }

    } else {
        setLatitude(prev->getLatitude());
        setLongitude(prev->getLongitude());
        setSpeed(prev->getSpeed());
    }

    setWPNames();
    setHeading(getCourse(prev->getLatitude(), prev->getLongitude(), curr->getLatitude(), curr->getLongitude()));
    _wp_range = getRange(prev->getLatitude(), prev->getLongitude(), curr->getLatitude(), curr->getLongitude());
    _old_range = _wp_range;
    _range_rate = 0;
    _hdg_lock = true;
    _missed = false;
    _missed_count = 0;
    _new_waypoint = true;

    SG_LOG(SG_AI, SG_ALERT, "AIShip: " << _name << " done initialising waypoints " << _tunnel);
    if (prev)
        init = true;

    if (init)
        return true;
    else
        return false;

} // end of initialization


double FGAIShip::processTimeString(const string& theTime) {

    int Hour;
    int Minute;
    int Second;

    // first split theTime string into
    //  hour, minute, second and convert to int;
    Hour   = atoi(theTime.substr(0,2).c_str());
    Minute = atoi(theTime.substr(3,5).c_str());
    Second = atoi(theTime.substr(6,8).c_str());

    // offset by a day-sec to allow for starting a day earlier
    double time_seconds = Hour * 3600
        + Minute * 60
        + Second;

    return time_seconds;
}

double FGAIShip::getDaySeconds () {
    // Date and time
    struct tm *t = globals->get_time_params()->getGmt();

    double day_seconds = t->tm_hour * 3600
        + t->tm_min * 60
        + t->tm_sec;

    return day_seconds;
}

bool FGAIShip::advanceFlightPlan (double start_sec, double day_sec) {

    double elapsed_sec = start_sec;
    double distance_nm = 0;

    //cout << "advancing flight plan start_sec: " << start_sec << " " << day_sec << endl;

    while ( elapsed_sec < day_sec ) {

        if (next->getName() == "END" || fp->getNextWaypoint() == 0) {

            if (_repeat ) {
                //cout << _name << ": " << "restarting flightplan" << endl;
                fp->restart();
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
            } else {
                //cout << _name << ": " << "ending flightplan" << endl;
                setDie(true);
                return false;
            }

        } else if (next->getName() == "WAIT") {
            //cout << _name << ": begin WAIT: " << prev->name << " ";
            //cout << curr->name << " " << next->name << endl;

            elapsed_sec += next->getTime_sec();

            if ( elapsed_sec >= day_sec)
                continue;

            fp->IncrementWaypoint(false);
            next = fp->getNextWaypoint();

            if (next->getName() != "WAITUNTIL" && next->getName() != "WAIT"
                && next->getName() != "END") {
                    prev = curr;
                    fp->IncrementWaypoint(false);
                    curr = fp->getCurrentWaypoint();
                    next = fp->getNextWaypoint();
            }

        } else if (next->getName() == "WAITUNTIL") {
            double until_sec = processTimeString(next->getTime());

            if (until_sec > _start_sec && start_sec < 0)
                until_sec -= _day;

            if (elapsed_sec < until_sec)
                elapsed_sec = until_sec;

            if (elapsed_sec >= day_sec )
                break;

            fp->IncrementWaypoint(false);
            next = fp->getNextWaypoint();

            if (next->getName() != "WAITUNTIL" && next->getName() != "WAIT") {
                prev = curr;
                fp->IncrementWaypoint(false);
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
            }

            //cout << _name << ": end WAITUNTIL: ";
            //cout << prev->name << " " << curr->name << " " << next->name <<  endl;

        } else {
            distance_nm = getRange(prev->getLatitude(), prev->getLongitude(), curr->getLatitude(), curr->getLongitude());
            elapsed_sec += distance_nm * 60 * 60 / prev->getSpeed();

            if (elapsed_sec >= day_sec)
                continue;

            fp->IncrementWaypoint(false);
            prev = fp->getPreviousWaypoint();
            curr = fp->getCurrentWaypoint();
            next = fp->getNextWaypoint();
        }

    }   // end while

    // the required position lies between the previous and current waypoints
    // so we will calculate the distance back up the track from the current waypoint
    // then calculate the lat and lon.

    //cout << "advancing flight plan done elapsed_sec: " << elapsed_sec
    //    << " " << day_sec << endl;

    double time_diff = elapsed_sec - day_sec;
    double lat, lon, recip;

    //cout << " time diff " << time_diff << endl;

    if (next->getName() == "WAIT" ){
        setSpeed(0);
        lat = curr->getLatitude();
        lon = curr->getLongitude();
        _wait_count= time_diff;
        _waiting = true;
    } else if (next->getName() == "WAITUNTIL") {
        setSpeed(0);
        lat = curr->getLatitude();
        lon = curr->getLongitude();
        _waiting = true;
    } else {
        setSpeed(prev->getSpeed());
        distance_nm = speed * time_diff / (60 * 60);
        double brg = getCourse(curr->getLatitude(), curr->getLongitude(), prev->getLatitude(), prev->getLongitude());

        //cout << " brg " << brg << " from " << curr->name << " to " << prev->name << " "
        //    << " lat "  << curr->latitude << " lon " << curr->longitude
        //    << " distance m " << distance_nm * SG_NM_TO_METER << endl;

        lat = geo_direct_wgs_84 (curr->getLatitude(), curr->getLongitude(), brg,
            distance_nm * SG_NM_TO_METER, &lat, &lon, &recip );
        lon = geo_direct_wgs_84 (curr->getLatitude(), curr->getLongitude(), brg,
            distance_nm * SG_NM_TO_METER, &lat, &lon, &recip );
        recip = geo_direct_wgs_84 (curr->getLatitude(), curr->getLongitude(), brg,
            distance_nm * SG_NM_TO_METER, &lat, &lon, &recip );
    }

    setLatitude(lat);
    setLongitude(lon);

    return true;
}

void FGAIShip::setWPPos() {

    if (curr->getName() == "END" || curr->getName() == "WAIT"
        || curr->getName() == "WAITUNTIL" || curr->getName() == "TUNNEL"){
            //cout << curr->name << " returning" << endl;
            return;
    }

    double elevation_m = 0;
    wppos.setLatitudeDeg(curr->getLatitude());
    wppos.setLongitudeDeg(curr->getLongitude());
    wppos.setElevationM(0);

    if (curr->getOn_ground()){

        if (globals->get_scenery()->get_elevation_m(SGGeod::fromGeodM(wppos, 3000),
            elevation_m, &_material, 0)){
                wppos.setElevationM(elevation_m);
        }

        //cout << curr->name << " setting measured  elev " << elevation_m << endl;

    } else {
        wppos.setElevationM(curr->getAltitude());
        //cout << curr->name << " setting FP elev " << elevation_m << endl;
    }

    curr->setAltitude(wppos.getElevationM());

}

void FGAIShip::setXTrackError() {

    double course = getCourse(prev->getLatitude(), prev->getLongitude(),
        curr->getLatitude(), curr->getLongitude());
    double brg = getCourse(pos.getLatitudeDeg(), pos.getLongitudeDeg(),
        curr->getLatitude(), curr->getLongitude());
    double xtrack_error_nm = sin((course - brg)* SG_DEGREES_TO_RADIANS) * _wp_range;
    double factor = -0.0045 * speed + 1;
    double limit = _lead_angle_limit * factor;

    if (_wp_range > 0){
        _lead_angle = atan2(xtrack_error_nm,(_wp_range * _proportion)) * SG_RADIANS_TO_DEGREES;
    } else
        _lead_angle = 0;

    _lead_angle *= _lead_angle_gain * factor;
    _xtrack_error = xtrack_error_nm * 6076.1155;

    SG_CLAMP_RANGE(_lead_angle, -limit, limit);

}
