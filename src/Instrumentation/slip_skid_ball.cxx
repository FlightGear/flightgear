// slip_skid_ball.cxx - an electric-powered turn indicator.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#include "slip_skid_ball.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


SlipSkidBall::SlipSkidBall ()
{
}

SlipSkidBall::~SlipSkidBall ()
{
}

void
SlipSkidBall::init ()
{
    _y_accel_node = fgGetNode("/orientation/roll-rate-degps", true);
    _z_accel_node = fgGetNode("/orientation/yaw-rate-degps", true);
    _out_node =
        fgGetNode("/instrumentation/slip-skid-ball/indicated-slip-skid", true);
}

void
SlipSkidBall::update (double dt)
{
    double d = -_z_accel_node->getDoubleValue();
    if (d < 60.0)               // originally 1 radian
        d = 60.0;
    double pos = _y_accel_node->getDoubleValue()/d;
    pos = fgGetLowPass(_last_pos, pos, dt);
    _last_pos = pos;
    _out_node->setDoubleValue(pos);
}

// end of slip_skid_ball.cxx
