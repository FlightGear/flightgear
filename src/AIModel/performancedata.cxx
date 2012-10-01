
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "performancedata.hxx"

#include <simgear/props/props.hxx>
#include "AIAircraft.hxx"

// For now, make this a define
// Later on, additional class variables can simulate settings such as braking power
// also, the performance parameters can be tweaked a little to add some personality
// to the AIAircraft.
#define BRAKE_SETTING 1.6

PerformanceData::PerformanceData() :
  _acceleration(4.0),
  _deceleration(2.0),
  _climbRate(3000.0),
  _descentRate(1500.0),
  _vRotate(150.0),
  _vTakeOff(160.0),
  _vClimb(300.0),
  _vCruise(430.0),
  _vDescent(300.0),
  _vApproach(170.0),
  _vTouchdown(150.0),
  _vTaxi(15.0)
{
  _rollrate = 9.0; // degrees per second
  _maxbank = 30.0; // passenger friendly bank angle

}

PerformanceData::PerformanceData(PerformanceData* clone) :
  _acceleration(clone->_acceleration),
  _deceleration(clone->_deceleration),
  _climbRate(clone->_climbRate),
  _descentRate(clone->_descentRate),
  _vRotate(clone->_vRotate),
  _vTakeOff(clone->_vTakeOff),
  _vClimb(clone->_vClimb),
  _vCruise(clone->_vCruise),
  _vDescent(clone->_vDescent),
  _vApproach(clone->_vApproach),
  _vTouchdown(clone->_vTouchdown),
  _vTaxi(clone->_vTaxi)
{
  _rollrate = clone->_rollrate;
  _maxbank = clone->_maxbank;
}

PerformanceData::~PerformanceData()
{}

void PerformanceData::initFromProps(SGPropertyNode *db_node)
{
// read the values, using the existing values as defaults
  _acceleration = db_node->getDoubleValue("acceleration-kts-hour", _acceleration);
  _deceleration = db_node->getDoubleValue("deceleration-kts-hour", _deceleration);
  _climbRate    = db_node->getDoubleValue("climbrate-fpm", _climbRate);
  _descentRate  = db_node->getDoubleValue("decentrate-fpm", _descentRate);
  _vRotate      = db_node->getDoubleValue("rotate-speed-kts", _vRotate);
  _vTakeOff     = db_node->getDoubleValue("takeoff-speed-kts", _vTakeOff);
  _vClimb       = db_node->getDoubleValue("climb-speed-kts", _vClimb);
  _vCruise      = db_node->getDoubleValue("cruise-speed-kts", _vCruise);
  _vDescent     = db_node->getDoubleValue("decent-speed-kts", _vDescent);
  _vApproach    = db_node->getDoubleValue("approach-speed-kts", _vApproach);
  _vTouchdown   = db_node->getDoubleValue("touchdown-speed-kts", _vTouchdown);
  _vTaxi        = db_node->getDoubleValue("taxi-speed-kts", _vTaxi);
}

double PerformanceData::actualSpeed(FGAIAircraft* ac, double tgt_speed, double dt, bool maxBrakes) {
    // if (tgt_speed > _vTaxi & ac->onGround()) // maximum taxi speed on ground
    //    tgt_speed = _vTaxi;
    // bad idea for a take off roll :-)

    double speed = ac->getSpeed();
    double speed_diff = tgt_speed - speed;

    if (speed_diff > 0.0)        // need to accelerate
    {
        speed += _acceleration * dt;
        if ( speed > tgt_speed )
            speed = tgt_speed;

    } else if (speed_diff < 0.0) { // decelerate
        if (ac->onGround()) {
            // deceleration performance is better due to wheel brakes.
            double brakePower = 0;
            if (maxBrakes) {
                brakePower = 3;
            } else {
                brakePower = BRAKE_SETTING;
            }
            speed -= brakePower * _deceleration * dt;
        } else {
            speed -= _deceleration * dt;
        }

        if ( speed < tgt_speed )
            speed = tgt_speed;

    }

    return speed;
}

double PerformanceData::decelerationOnGround() const
{
  return _deceleration * BRAKE_SETTING;
}

double PerformanceData::actualBankAngle(FGAIAircraft* ac, double tgt_roll, double dt) {
    // check maximum bank angle
    if (fabs(tgt_roll) > _maxbank)
        tgt_roll = _maxbank * tgt_roll/fabs(tgt_roll);

    double roll = ac->getRoll();
    double bank_diff = tgt_roll - roll;

    if (fabs(bank_diff) > 0.2) {
        if (bank_diff > 0.0) {
            roll += _rollrate * dt;
            if (roll > tgt_roll)
                roll = tgt_roll;
        }
        else if (bank_diff < 0.0) {
            roll -= _rollrate * dt;

            if (roll < tgt_roll)
                roll = tgt_roll;
        }
        //while (roll > 180) roll -= 360;
        //while (roll < 180) roll += 360;
    }

    return roll;
}

double PerformanceData::actualPitch(FGAIAircraft* ac, double tgt_pitch, double dt) {
    double pitch = ac->getPitch();
    double pdiff = tgt_pitch - pitch;

    if (pdiff > 0.0) { // nose up
        pitch += 0.005*_climbRate * dt / 3.0; //TODO avoid hardcoded 3 secs

        if (pitch > tgt_pitch)
            pitch = tgt_pitch;
    } else if (pdiff < 0.0) { // nose down
        pitch -= 0.002*_descentRate * dt / 3.0;

        if (pitch < tgt_pitch)
            pitch = tgt_pitch;
    }

    return pitch;
}

double PerformanceData::actualAltitude(FGAIAircraft* ac, double tgt_altitude, double dt) {
    if (ac->onGround()) {
        //FIXME: a return sensible value here
        return 0.0; // 0 for now to avoid compiler errors
    } else
        return ac->getAltitude() + ac->getVerticalSpeed()*dt/60.0;
}

double PerformanceData::actualVerticalSpeed(FGAIAircraft* ac, double tgt_vs, double dt) {
    double vs = ac->getVerticalSpeed();
    double vs_diff = tgt_vs - vs;

    if (fabs(vs_diff) > .001) {
        if (vs_diff > 0.0) {
            vs += _climbRate * dt / 3.0; //TODO avoid hardcoded 3 secs to attain climb rate from level flight

            if (vs > tgt_vs)
                vs = tgt_vs;
        } else if (vs_diff < 0.0) {
            vs -= _descentRate * dt / 3.0;

            if (vs < tgt_vs)
                vs = tgt_vs;
        }
    }

    return vs;
}

bool PerformanceData::gearExtensible(const FGAIAircraft* ac) {
    return (ac->altitudeAGL() < 900.0)
            && (ac->airspeed() < _vTouchdown * 1.25);
}
