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
#include <simgear/timing/sg_time.hxx>
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
    setStartTime(scFileNode->getStringValue("time", ""));

    if (!flightplan.empty()) {
        FGAIFlightPlan* fp = new FGAIFlightPlan(flightplan);
        setFlightPlan(fp);
    }

}

bool FGAIShip::init(bool search_in_AI_path) {
    prev = 0; // the one behind you
    curr = 0; // the one ahead
    next = 0; // the next plus 1

    _until_time = "";

    props->setStringValue("name", _name.c_str());
    props->setStringValue("position/waypoint-name-prev", _prev_name.c_str());
    props->setStringValue("position/waypoint-name-curr", _curr_name.c_str());
    props->setStringValue("position/waypoint-name-next", _next_name.c_str());
    props->setStringValue("submodels/path", _path.c_str());
    props->setStringValue("position/waypoint-start-time", _start_time.c_str());
    props->setStringValue("position/waypoint-wait-until-time", _until_time.c_str());

    _hdg_lock = false;
    _rudder = 0.0;
    no_roll = false;

    _rudder_constant = 0.5;
    _roll_constant = 0.001;
    _speed_constant = 0.05;
    _hdg_constant = 0.01;
    _roll_factor = -0.0083335;

    _rd_turn_radius_ft = _sp_turn_radius_ft = turn_radius_ft;

    _fp_init = false;
    _missed = false;
    _waiting = false;
    _new_waypoint = true;

    _missed_count = 0;
    _wait_count = 0;
    _missed_time_sec = 30;

    _day = 86400;


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
    props->tie("controls/constants/roll-factor",
        SGRawValuePointer<double>(&_roll_factor));
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
    props->tie("submodels/serviceable",
        SGRawValuePointer<bool>(&_serviceable));
}

void FGAIShip::unbind() {
    FGAIBase::unbind();
    props->untie("surface-positions/rudder-pos-deg");
    props->untie("controls/heading-lock");
    props->untie("controls/tgt-speed-kts");
    props->untie("controls/tgt-heading-degs");
    props->untie("controls/constants/roll");
    props->untie("controls/constants/rudder");
    props->untie("controls/constants/roll-factor");
    props->untie("controls/constants/speed");
    props->untie("position/waypoint-range-nm");
    props->untie("position/waypoint-range-old-nm");
    props->untie("position/waypoint-range-rate-nm-sec");
    props->untie("position/waypoint-new");
    props->untie("position/waypoint-missed");
    props->untie("position/waypoint-wait-count");
    props->untie("position/waypoint-waiting");
    props->untie("position/waypoint-missed-time-sec");
    props->untie("submodels/serviceable");
}

void FGAIShip::update(double dt) {
    FGAIBase::update(dt);
    Run(dt);
    Transform();
}

void FGAIShip::Run(double dt) {
    //cout << _name << " init: " << _fp_init << endl;
    if (_fp_init)
        ProcessFlightPlan(dt);

    //    double speed_north_deg_sec;
    //    double speed_east_deg_sec;
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

void FGAIShip::setStartTime(const string& st) {
    _start_time = st;
}

void FGAIShip::setUntilTime(const string& ut) {
    _until_time = ut;
    props->setStringValue("position/waypoint-wait-until-time", _until_time.c_str());
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

void FGAIShip::setRudder(float r) {
    _rudder = r;
}

void FGAIShip::setRoll(double rl) {
    roll = rl;
}

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

    SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: prev wp name " << prev->name);
    SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: current wp name " << curr->name);
    SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: next wp name " << next->name);

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

void FGAIShip::ProcessFlightPlan(double dt) {

    double time_sec = getDaySeconds();
    double until_time_sec = 0;

    _missed = false;
    _dt_count += dt;

    ///////////////////////////////////////////////////////////////////////////
    // Check Execution time (currently once every 1 sec)
    // Add a bit of randomization to prevent the execution of all flight plans
    // in synchrony, which can add significant periodic framerate flutter.
    ///////////////////////////////////////////////////////////////////////////

    //cout << "_start_sec " << _start_sec << " time_sec " << time_sec << endl;
    if (_dt_count < _next_run && _start_sec < time_sec)
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
    setWPNames();

    if ((_wp_range < sp_turn_radius_nm) || _missed || _waiting && !_new_waypoint) {

        if (_next_name == "END") {

            if (_repeat) {
                SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: Flightplan restarting ");
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
                SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: Flightplan dieing ");
                setDie(true);
                _dt_count = 0;
                return;
            }

        } else if (_next_name == "WAIT") {

            if (_wait_count < next->time_sec) {
                SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: " << _name << " waiting ");
                setSpeed(0);
                _waiting = true;
                _wait_count += _dt_count;
                _dt_count = 0;
                return;
            } else {
                SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: " << _name
                    << " wait done: getting new waypoints ");
                _waiting = false;
                _wait_count = 0;
                fp->IncrementWaypoint(false);
                next = fp->getNextWaypoint();

                if (next->name == "WAITUNTIL" || next->name == "WAIT"
                        || next->name == "END")
                    return;

                prev = curr;
                fp->IncrementWaypoint(false);
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
            }

        } else if (_next_name == "WAITUNTIL") {
            time_sec = getDaySeconds();
            until_time_sec = processTimeString(next->time);
            _until_time = next->time;
            setUntilTime(next->time);
            if (until_time_sec > time_sec) {
                SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: " << _name << " waiting until: "
                    << _until_time << " " << until_time_sec << " now " << time_sec );
                setSpeed(0);
                _waiting = true;
                return;
            } else {
                SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: "
                    << _name << " wait until done: getting new waypoints ");
                setUntilTime("");
                fp->IncrementWaypoint(false);

                while (next->name == "WAITUNTIL") {
                    fp->IncrementWaypoint(false);
                    next = fp->getNextWaypoint();
                }

                if (next->name == "WAIT")
                    return;

                prev = curr;
                fp->IncrementWaypoint(false);
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
                _waiting = false;
            }

        } else {
            //now reorganise the waypoints, so that next becomes current and so on
            SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: " << _name << " getting new waypoints ");
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
        SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: Bearing or Range is not a finite number");

     _dt_count = 0;
} // end Processing FlightPlan

bool FGAIShip::initFlightPlan() {

    SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: " << _name << " initializing waypoints ");

    bool init = false;

    _start_sec = 0;

    fp->restart();
    fp->IncrementWaypoint(false);

    prev = fp->getPreviousWaypoint(); //first waypoint
    curr = fp->getCurrentWaypoint();  //second waypoint
    next = fp->getNextWaypoint();     //third waypoint (might not exist!)

    while (curr->name == "WAIT" || curr->name == "WAITUNTIL") {  // don't wait when initialising
        SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: " << _name << " re-initializing waypoints ");
        fp->IncrementWaypoint(false);
        curr = fp->getCurrentWaypoint();
        next = fp->getNextWaypoint();
    }

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
    setLatitude(prev->latitude);
    setLongitude(prev->longitude);
    setSpeed(prev->speed);
    }

    setWPNames();
    setHeading(getCourse(prev->latitude, prev->longitude, curr->latitude, curr->longitude));
    _wp_range = getRange(prev->latitude, prev->longitude, curr->latitude, curr->longitude);
    _old_range = _wp_range;
    _range_rate = 0;
    _hdg_lock = true;
    _missed = false;
    _missed_count = 0;
    _new_waypoint = true;

    SG_LOG(SG_GENERAL, SG_DEBUG, "AIShip: " << _name << " done initialising waypoints ");
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

        if (next->name == "END") {

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

        } else if (next->name == "WAIT") {
            //cout << _name << ": begin WAIT: " << prev->name << " ";
            //cout << curr->name << " " << next->name << endl;

            elapsed_sec += next->time_sec;

            if ( elapsed_sec >= day_sec)
                continue;

            fp->IncrementWaypoint(false);
            next = fp->getNextWaypoint();

            if (next->name != "WAITUNTIL" && next->name != "WAIT"
                    && next->name != "END") {
                prev = curr;
                fp->IncrementWaypoint(false);
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
            }

        } else if (next->name == "WAITUNTIL") {
            double until_sec = processTimeString(next->time);

            if (until_sec > _start_sec && start_sec < 0)
                until_sec -= _day;

            if (elapsed_sec < until_sec)
                elapsed_sec = until_sec;

            if (elapsed_sec >= day_sec )
                break;

            fp->IncrementWaypoint(false);
            next = fp->getNextWaypoint();

            if (next->name != "WAITUNTIL" && next->name != "WAIT") {
                prev = curr;
                fp->IncrementWaypoint(false);
                curr = fp->getCurrentWaypoint();
                next = fp->getNextWaypoint();
            }

            //cout << _name << ": end WAITUNTIL: ";
            //cout << prev->name << " " << curr->name << " " << next->name <<  endl;

        } else {
            distance_nm = getRange(prev->latitude, prev->longitude, curr->latitude, curr->longitude);
            elapsed_sec += distance_nm * 60 * 60 / prev->speed;

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
    /*cout << "advancing flight plan done elapsed_sec: " << elapsed_sec
        << " " << day_sec << endl;*/

    double time_diff = elapsed_sec - day_sec;
    double lat, lon, recip;

    //cout << " time diff " << time_diff << endl;

    if (next->name == "WAIT" ){
        setSpeed(0);
        lat = curr->latitude;
        lon = curr->longitude;
        _wait_count= time_diff;
        _waiting = true;
    } else if (next->name == "WAITUNTIL") {
        setSpeed(0);
        lat = curr->latitude;
        lon = curr->longitude;
        _waiting = true;
    } else {
        setSpeed(prev->speed);
        distance_nm = speed * time_diff / (60 * 60);
        double brg = getCourse(curr->latitude, curr->longitude, prev->latitude, prev->longitude);

        //cout << " brg " << brg << " from " << curr->name << " to " << prev->name << " "
        //    << " lat "  << curr->latitude << " lon " << curr->longitude
        //    << " distance m " << distance_nm * SG_NM_TO_METER << endl;

        lat = geo_direct_wgs_84 (curr->latitude, curr->longitude, brg,
            distance_nm * SG_NM_TO_METER, &lat, &lon, &recip );
        lon = geo_direct_wgs_84 (curr->latitude, curr->longitude, brg,
            distance_nm * SG_NM_TO_METER, &lat, &lon, &recip );
        recip = geo_direct_wgs_84 (curr->latitude, curr->longitude, brg,
            distance_nm * SG_NM_TO_METER, &lat, &lon, &recip );
    }

    //cout << "Pos " << lat << ", " << lon << " recip " << recip << endl;

    setLatitude(lat);
    setLongitude(lon);
    return true;
}
