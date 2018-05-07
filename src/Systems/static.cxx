// static.cxx - the static air system.
// Written by David Megginson, started 2002.
//
// Last modified by Eric van den Berg, 09 Nov 2013
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "static.hxx"

#include <string>

#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <simgear/constants.h>
#include <simgear/math/SGMisc.hxx>
#include <simgear/math/SGLimits.hxx>
#include <simgear/math/SGMathFwd.hxx>
#include <simgear/sg_inlines.h>


StaticSystem::StaticSystem ( SGPropertyNode *node )
    :
    _name(node->getStringValue("name", "static")),
    _num(node->getIntValue("number", 0)),
    _tau(SGMiscd::max(.0,node->getDoubleValue("tau", 1))),
    _error_factor(node->getDoubleValue("error-factor", 0)),
    _type(node->getIntValue("type", 0))
{
}

StaticSystem::~StaticSystem ()
{
}

void
StaticSystem::init ()
{
    std::string branch = "/systems/" + _name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _pressure_in_node = fgGetNode("/environment/pressure-inhg", true);
    _pressure_out_node = node->getChild("pressure-inhg", 0, true);
    _beta_node = fgGetNode("/orientation/side-slip-deg", true);
    _alpha_node = fgGetNode("/orientation/alpha-deg", true);
    _mach_node = fgGetNode("/velocities/mach", true);
    SG_CLAMP_RANGE(_error_factor,0.0,1.0);     // making sure the error_factor is between 0 and 1

    reinit();
}

void
StaticSystem::reinit ()
{
    // start with settled static pressure
    _pressure_out_node->setDoubleValue(_pressure_in_node->getDoubleValue());
}

void
StaticSystem::bind ()
{
}

void
StaticSystem::unbind ()
{
}

void
StaticSystem::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
        double p_new = _pressure_in_node->getDoubleValue();                         //current static pressure around aircraft
        double p = _pressure_out_node->getDoubleValue();                            //last pressure in aircraft static system

        double beta;
        double alpha;
        double mach;
        double trat = _tau ? dt/_tau : SGLimitsd::max();

        double proj_factor = 0;
        double pt;
        double qc_part;

        if (_type == 1) {                                       // type 1 = static pressure dependent on side-slip only: static port on the fuselage
            beta = _beta_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
            proj_factor = sin(beta);
        }

        if (_type == 2) {                                       // type 2 = static pressure dependent on aoa and side-slip: static port on the pitot tube
            alpha = _alpha_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
            beta = _beta_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
            proj_factor = sqrt( 1.0 - cos(beta)*cos(beta) * cos(alpha)*cos(alpha) );
        }

        if ( (_type ==1) || (_type == 2) ) {
            mach = _mach_node->getDoubleValue();
            pt = p_new * pow(1 + 0.2 * mach*mach*proj_factor*proj_factor, 3.5 );    //total pressure perpendicular to static port (=perpendicular to body x-axis)
            qc_part = (pt - p_new) * _error_factor ;                            //part of impact pressure to be added to static pressure (due to sideslip)
            p_new = p_new + qc_part;
        }

        _pressure_out_node->setDoubleValue(
            _tau > .0 ? fgGetLowPass(p, p_new, trat) : p_new
        );           //setting new pressure in static system

    }
}


// Register the subsystem.
#if 0
SGSubsystemMgr::Registrant<StaticSystem> registrantStaticSystem(
    SGSubsystemMgr::GENERAL,
    {{"vacuum", SGSubsystemMgr::Dependency::HARD}});
#endif

// end of static.cxx
