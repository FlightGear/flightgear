// slip_skid_ball.cxx - an electric-powered turn indicator.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#include "slip_skid_ball.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


SlipSkidBall::SlipSkidBall ( SGPropertyNode *node)
    :
    name("slip-skid-ball"),
    num(0)
{
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_INSTR, SG_WARN, "Error in slip-skid-ball config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}

SlipSkidBall::SlipSkidBall ()
{
}

SlipSkidBall::~SlipSkidBall ()
{
}

void
SlipSkidBall::init ()
{
    string branch;
    branch = "/instrumentation/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _y_accel_node = fgGetNode("/accelerations/pilot/y-accel-fps_sec", true);
    _z_accel_node = fgGetNode("/accelerations/pilot/z-accel-fps_sec", true);
    _out_node = node->getChild("indicated-slip-skid", 0, true);
    _override_node = node->getChild("iverride", 0, true);
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
