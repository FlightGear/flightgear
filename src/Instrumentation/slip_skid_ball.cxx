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
    _serviceable_node =
        fgGetNode("/instrumentation/slip-skid-ball/serviceable", true);
    _y_accel_node = fgGetNode("/accelerations/pilot/y-accel-fps_sec", true);
    _z_accel_node = fgGetNode("/accelerations/pilot/z-accel-fps_sec", true);
    _out_node =
        fgGetNode("/instrumentation/slip-skid-ball/indicated-slip-skid", true);
    _override_node =
        fgGetNode("/instrumentation/slip-skid-ball/override", true);
}

void
SlipSkidBall::update (double delta_time_sec)
{
    if (_serviceable_node->getBoolValue() && !_override_node->getBoolValue()) {
        double d = -_z_accel_node->getDoubleValue();
        if (d < 1.0)
            d = 1.0;
        double pos = _y_accel_node->getDoubleValue() / d * 10.0;
        pos = fgGetLowPass(_out_node->getDoubleValue(), pos, delta_time_sec);
        _out_node->setDoubleValue(pos);
    }
}

// end of slip_skid_ball.cxx
