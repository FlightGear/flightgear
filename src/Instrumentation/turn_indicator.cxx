// turn_indicator.cxx - an electric-powered turn indicator.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

#include "turn_indicator.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


// Use a bigger number to be more responsive, or a smaller number
// to be more sluggish.
#define RESPONSIVENESS 0.5


TurnIndicator::TurnIndicator ( SGPropertyNode *node) :
    _last_rate(0),
    name("turn-indicator"),
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
            SG_LOG( SG_INSTR, SG_WARN, "Error in turn-indicator config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}

TurnIndicator::TurnIndicator () :
    _last_rate(0)
{
}

TurnIndicator::~TurnIndicator ()
{
}

void
TurnIndicator::init ()
{
    string branch;
    branch = "/instrumentation/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _roll_rate_node = fgGetNode("/orientation/roll-rate-degps", true);
    _yaw_rate_node = fgGetNode("/orientation/yaw-rate-degps", true);
    _electric_current_node = 
        fgGetNode("/systems/electrical/outputs/turn-coordinator", true);
    _rate_out_node = node->getChild("indicated-turn-rate", 0, true);

    //_serviceable_node->setBoolValue(true);
    
}

void
TurnIndicator::bind ()
{
    string branch;
    branch = "/instrumentation/" + name + "/serviceable";
    fgTie(branch.c_str(),
          &_gyro, &Gyro::is_serviceable, &Gyro::set_serviceable);
    branch = "/instrumentation/" + name + "/spin";
    fgTie(branch.c_str(),
          &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
}

void
TurnIndicator::unbind ()
{
    string branch;
    branch = "/instrumentation/" + name + "/serviceable";
    fgUntie(branch.c_str());
    branch = "/instrumentation/" + name + "/spin";
    fgUntie(branch.c_str());
}

void
TurnIndicator::update (double dt)
{
                                // Get the spin from the gyro
    _gyro.set_power_norm(_electric_current_node->getDoubleValue()/60.0);
    _gyro.update(dt);
    double spin = _gyro.get_spin_norm();

                                // Calculate the indicated rate
    double factor = 1.0 - ((1.0 - spin) * (1.0 - spin) * (1.0 - spin));
    double rate = ((_roll_rate_node->getDoubleValue() / 20.0) +
                   (_yaw_rate_node->getDoubleValue() / 3.0));

                                // Clamp the rate
    if (rate < -2.5)
        rate = -2.5;
    else if (rate > 2.5)
        rate = 2.5;

                                // Lag left, based on gyro spin
    rate = -2.5 + (factor * (rate + 2.5));
    rate = fgGetLowPass(_last_rate, rate, dt*RESPONSIVENESS);
    _last_rate = rate;
    
                                // Publish the indicated rate
    _rate_out_node->setDoubleValue(rate);
}

// end of turn_indicator.cxx
