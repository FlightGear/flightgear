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
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>

#include "AIShip.hxx"


FGAIShip::FGAIShip(object_type ot) :
    FGAIBase(ot),
    _dt_count(0),
    _next_run(0)
{
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

    if (!flightplan.empty()) {
        FGAIFlightPlan* fp = new FGAIFlightPlan(flightplan);
        setFlightPlan(fp);
    }

}

bool FGAIShip::init(bool search_in_AI_path) {
    prev = 0; // the one behind you
    curr = 0; // the one ahead
    next = 0; // the next plus 1

    props->setStringValue("name", _name.c_str());
    props->setStringValue("position/waypoint-name-prev", _prev_name.c_str());
    props->setStringValue("position/waypoint-name-curr", _curr_name.c_str());
    props->setStringValue("position/waypoint-name-next", _next_name.c_str());

    _hdg_lock = false;
    _rudder = 0.0;
    no_roll = false;

    _rudder_constant = 0.5;
    _roll_constant = 0.001;
    _speed_constant = 0.05;
    _hdg_constant = 0.01;

    _rd_turn_radius_ft = _sp_turn_radius_ft = turn_radius_ft;

    _fp_init = false;
    _missed = false;
    _waiting = false;
    _new_waypoint = true;

    _missed_count = 0;
    _wait_count = 0;
    _missed_time_sec = 30;

    _wp_range = _old_range = 0;
    _range_rate = 0;

    if (fp)
        _fp_init = initFlightPlan();

    return FGAIBase::init(search_in_AI_path);
}


void FGAIShip::bind() {
    FGAIBase::bind();

    props->tie("surface-positions/rudder-pos-deg",
        SGRawValuePointer<float>(&_rudder));
    props->tie("controls/heading-lock",
        SGRawValuePointer<bool>(&_hdg_lock));
    props->tie("controls/tgt-speed-kts",
        SGRawValuePointer<double>(&tgt_speed));
    props->tie("controls/tgt-heading-degs",
        SGRawValuePointer<double>(&tgt_heading));
    props->tie("controls/constants/rudder",
        SGRawValuePointer<double>(&_rudder_constant));
    props->tie("controls/constants/roll",
        SGRawValuePointer<double>(&_roll_constant));
    props->tie("controls/constants/rudder",
        SGRawValuePointer<double>(&_rudder_constant));
    props->tie("controls/constants/speed",
        SGRawValuePointer<double>(&_speed_constant));
    props->tie("position/waypoint-range-nm",
        SGRawValuePointer<double>(&_wp_range));
    props->tie("position/waypoint-range-old-nm",
        SGRawValuePointer<double>(&_old_range));
    props->tie("position/waypoint-range-rate-nm-sec",
        SGRawValuePointer<double>(&_range_rate));
    props->tie("position/waypoint-new",
        SGRawValuePointer<bool>(&_new_waypoint));
    props->tie("position/waypoint-missed",
        SGRawValuePointer<bool>(&_missed));
    props->tie("position/waypoint-missed-count",
        SGRawValuePointer<double>(&_missed_count));
    props->tie("position/waypoint-missed-time-sec",
        SGRawValuePointer<double>(&_missed_time_sec));
    props->tie("position/waypoint-wait-count",
        SGRawValuePointer<double>(&_wait_count));
    props->tie("position/waypoint-waiting",
        SGRawValuePointer<bool>(&_waiting));
}

void FGAIShip::unbind() {
    FGAIBase::unbind();
    props->untie("surface-positions/rudder-pos-deg");
    props->untie("controls/heading-lock");
    props->untie("controls/tgt-speed-kts");
    props->untie("controls/tgt-heading-degs");
    props->untie("controls/constants/roll");
    props->untie("controls/constants/rudder");
    props->untie("controls/constants/speed");
    props->untie("position/waypoint-range-nm");
    props->untie("position/waypoint-range-old-nm");
    props->untie("position/waypoint-range-rate-nm-sec");
    props->untie("position/waypoint-new");
    props->untie("position/waypoint-missed");
    props->untie("position/waypoint-wait-count");
    props->untie("position/waypoint-waiting");
    props->untie("position/waypoint-missed-time-sec");
}

void FGAIShip::update(double dt) {
    FGAIBase::update(dt);
    Run(dt);
    Transform();
}

void FGAIShip::Run(double dt) {

    if (_fp_init)
        ProcessFlightPlan(dt);

    double speed_north_deg_sec;
    double speed_east_deg_sec;
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

    // do not allow unreasonable ship speeds
    if (speed > 40)
        speed = 40;

    // convert speed to degrees per second
    speed_north_deg_sec = cos(hdg / SGD_RADIANS_TO_DEGREES)
        * speed * 1.686 / ft_per_deg_lat;
    speed_east_deg_sec = sin(hdg / SGD_RADIANS_TO_DEGREES)
        * speed * 1.686 / ft_per_deg_lon;

    // set new position
    pos.setLatitudeDeg(pos.getLatitudeDeg() + speed_north_deg_sec * dt);
    pos.setLongitudeDeg(pos.getLongitudeDeg() + speed_east_deg_sec * dt);

    // adjust heading based on current _rudder angle

    //cout << "turn_radius_ft " << turn_radius_ft ;

    if (turn_radius_ft <= 0)
        turn_radius_ft = 0; // don't allow nonsense values

    if (_rudder > 45)
        _rudder = 45;

    if (_rudder < -45)
        _rudder = -45;

    //we assume that at slow speed ships will manoeuvre using engines/bow thruster
    if (fabs(speed)<=5)
        _sp_turn_radius_ft = 500;
    else
        // adjust turn radius for speed. The equation is very approximate.
        // we need to allow for negative speeds
        _sp_turn_radius_ft = 10 * pow ((fabs(speed) - 15), 2) + turn_radius_ft;

    //cout << " speed turn radius " << _sp_turn_radius_ft ;

    if (_rudder <= -0.25 || _rudder >= 0.25) {
        // adjust turn radius for _rudder angle. The equation is even more approximate.
        float a = 19;
        float b = -0.2485;
        float c = 0.543;

        _rd_turn_radius_ft = (a * exp(b * fabs(_rudder)) + c) * _sp_turn_radius_ft;

        //cout <<" _rudder turn radius " << _rd_turn_radius_ft << endl;

        // calculate the angle, alpha, subtended by the arc traversed in time dt
        alpha = ((speed * 1.686 * dt) / _rd_turn_radius_ft) * SG_RADIANS_TO_DEGREES;

        // make sure that alpha is applied in the right direction
        hdg += alpha * sign(_rudder);

        if (hdg > 360.0)
            hdg -= 360.0;

        if (hdg < 0.0)
            hdg += 360.0;

        //adjust roll for _rudder angle and speed. Another bit of voodoo
        raw_roll = -0.0166667 * speed * _rudder;
    } else {
        // _rudder angle is 0
        raw_roll = 0;
    }

    //low pass filter
    if (speed < 0)
        roll = -roll;

    roll = (raw_roll * _roll_constant) + (roll * (1 - _roll_constant));

    // adjust target _rudder angle if heading lock engaged
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
    }

    // adjust _rudder angle
    double rudder_diff = _tgt_rudder - _rudder;
    // set the _rudder limit by speed
    if (speed <= 40)
        rudder_limit = (-0.825 * speed) + 35;
    else
        rudder_limit = 2;

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
}


void FGAIShip::TurnTo(double heading) {
    tgt_heading = heading;
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

void FGAIShip::setName(const string& n) {
    _name = n;
}

void FGAIShip::setCurrName(const string& c) {
    _curr_name = c;
    props->setStringValue("position/waypoint-name-curr", _curr_name.c_str());
}

void FGAIShip::setNextName(const string& n) {
    _next_name = n;
    props->setStringValue("position/waypoint-name-next", _next_name.c_str());
}

void FGAIShip::setPrevName(const string& p) {
    _prev_name = p;
    props->setStringValue("position/waypoint-name-prev", _prev_name.c_str());
}

void FGAIShip::setRepeat(bool r) {
    _repeat = r;
}

void FGAIShip::setMissed(bool m) {
    _missed = m;
    props->setBoolValue("position/waypoint-missed", _missed);
}

void FGAIShip::ProcessFlightPlan(double dt) {

    _missed = false;
    _dt_count += dt;

    ///////////////////////////////////////////////////////////////////////////
    // Check Execution time (currently once every 1 sec)
    // Add a bit of randomization to prevent the execution of all flight plans
    // in synchrony, which can add significant periodic framerate flutter.
    ///////////////////////////////////////////////////////////////////////////
    if (_dt_count < _next_run)
        return;

    _next_run = 1.0 + (0.5 * sg_random());

    // check to see if we've reached the point for our next turn
    // if the range to the waypoint is less than the calculated turn
    // radius we can start the turn to the next leg
    _wp_range = getRange(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr->latitude, curr->longitude);
    _range_rate = (_wp_range - _old_range) / _dt_count;
    double sp_turn_radius_nm = _sp_turn_radius_ft / 6076.1155;

    // we need to try to identify a _missed waypoint

    // calculate the time needed to turn through an arc of 90 degrees, and allow an error of 30 secs
    if (speed != 0)
        _missed_time_sec = 30 + ((SGD_PI * sp_turn_radius_nm * 60 * 60) / (2 * fabs(speed)));
    else
        _missed_time_sec = 30;

    if ((_range_rate > 0) && (_wp_range < 3 * sp_turn_radius_nm) && !_new_waypoint)
        _missed_count += _dt_count;


    if (_missed_count >= _missed_time_sec) {
        setMissed(true);
    } else {
        setMissed(false);
    }

    _old_range = _wp_range;

    if ((_wp_range < sp_turn_radius_nm) || _missed || _waiting && !_new_waypoint) {

        if (_next_name == "END") {

            if (_repeat) {
                SG_LOG(SG_GENERAL, SG_INFO, "AIShip: Flightplan restarting ");
                fp->restart();
                prev = curr;
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
                setWPNames();
                _wp_range = getRange(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr->latitude, curr->longitude);
                _old_range = _wp_range;
                _range_rate = 0;
                _new_waypoint = true;
                _missed_count = 0;
                AccelTo(prev->speed);
            } else {
                SG_LOG(SG_GENERAL, SG_INFO, "AIShip: Flightplan dieing ");
                setDie(true);
                _dt_count = 0;
                return;
            }

        } else if (_next_name == "WAIT") {

            if (_wait_count < next->wait_time) {
                SG_LOG(SG_GENERAL, SG_INFO, "AIShip: " << _name << " _waiting ");
                setSpeed(0);
                _waiting = true;
                _wait_count += _dt_count;
                _dt_count = 0;
                return;
            } else {
                SG_LOG(SG_GENERAL, SG_INFO, "AIShip: " << _name << " wait done: getting new waypoints ");
                prev = curr;
                fp->IncrementWaypoint(false);
                fp->IncrementWaypoint(false);  // do it twice
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
                _waiting = false;
                _wait_count = 0;
            }

        } else {
            //now reorganise the waypoints, so that next becomes current and so on
            SG_LOG(SG_GENERAL, SG_INFO, "AIShip: " << _name << " getting new waypoints ");
            fp->IncrementWaypoint(false);
            prev = fp->getPreviousWaypoint(); //first waypoint
            curr = fp->getCurrentWaypoint();  //second waypoint
            next = fp->getNextWaypoint();     //third waypoint (might not exist!)
        }

        setWPNames();
        _new_waypoint = true;
        _missed_count = 0;
        _range_rate = 0;
        _wp_range = getRange(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr->latitude, curr->longitude);
        _old_range = _wp_range;
        AccelTo(prev->speed);
    } else {
        _new_waypoint = false;
    }

    //   now revise the required course for the next way point
    double course = getCourse(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr->latitude, curr->longitude);

    if (finite(course))
        TurnTo(course);
    else
        SG_LOG(SG_GENERAL, SG_ALERT, "AIShip: Bearing or Range is not a finite number");

     _dt_count = 0;
} // end Processing FlightPlan

void FGAIShip::setRudder(float r) {
    _rudder = r;
}

void FGAIShip::setRoll(double rl) {
    roll = rl;
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
    } else {
        return recip;
    }
}

bool FGAIShip::initFlightPlan() {
    SG_LOG(SG_GENERAL, SG_ALERT, "AIShip: " << _name << " initialising waypoints ");
    fp->restart();
    fp->IncrementWaypoint(false);

    prev = fp->getPreviousWaypoint(); //first waypoint
    curr = fp->getCurrentWaypoint();  //second waypoint
    next = fp->getNextWaypoint();     //third waypoint (might not exist!)

    if (curr->name == "WAIT") {  // don't wait when initialising
        SG_LOG(SG_GENERAL, SG_ALERT, "AIShip: " << _name << " re-initialising waypoints ");
        fp->IncrementWaypoint(false);
        curr = fp->getCurrentWaypoint();
        next = fp->getNextWaypoint();
    }

    setWPNames();
    setLatitude(prev->latitude);
    setLongitude(prev->longitude);
    setSpeed(prev->speed);
    setHeading(getCourse(prev->latitude, prev->longitude, curr->latitude, curr->longitude));
    _hdg_lock = true;
    _wp_range = getRange(pos.getLatitudeDeg(), pos.getLongitudeDeg(), curr->latitude, curr->longitude);
    _old_range = _wp_range;
    _range_rate = 0;
    _missed = false;
    _missed_count = 0;
    _new_waypoint = true;

    SG_LOG(SG_GENERAL, SG_INFO, "AIShip: " << _name << " done initialising waypoints ");

    if (prev)
        return true;
    else
        return false;

} // end of initialization

void FGAIShip::setWPNames() {

    if (prev != 0)
        setPrevName(prev->name);
    else
        setPrevName("");

    setCurrName(curr->name);

    if (next != 0)
        setNextName(next->name);
    else
        setNextName("");

    SG_LOG(SG_GENERAL, SG_INFO, "AIShip: prev wp name " << prev->name);
    SG_LOG(SG_GENERAL, SG_INFO, "AIShip: current wp name " << curr->name);
    SG_LOG(SG_GENERAL, SG_INFO, "AIShip: next wp name " << next->name);

}
