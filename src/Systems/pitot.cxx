// pitot.cxx - the pitot air system.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include "pitot.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>



PitotSystem::PitotSystem ( SGPropertyNode *node )
    :
    name("pitot"),
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
            SG_LOG( SG_SYSTEMS, SG_WARN, "Error in systems config logic" );
            if ( name.length() ) {
                SG_LOG( SG_SYSTEMS, SG_WARN, "Section = " << name );
            }
        }
    }
}

PitotSystem::PitotSystem ( int i )
{
    num = i;
    name = "pitot";
}

PitotSystem::~PitotSystem ()
{
}

void
PitotSystem::init ()
{
    string branch;
    branch = "/systems/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _pressure_node = fgGetNode("/environment/pressure-inhg", true);
    _density_node = fgGetNode("/environment/density-slugft3", true);
    _velocity_node = fgGetNode("/velocities/airspeed-kt", true);
    _total_pressure_node = node->getChild("total-pressure-inhg", 0, true);
}

void
PitotSystem::bind ()
{
}

void
PitotSystem::unbind ()
{
}

#ifndef INHGTOPSF
# define INHGTOPSF (2116.217/29.9212)
#endif

#ifndef PSFTOINHG
# define PSFTOINHG (1/INHGTOPSF)
#endif

#ifndef KTTOFPS
# define KTTOFPS 1.68781
#endif


void
PitotSystem::update (double dt)
{
    if (_serviceable_node->getBoolValue()) {
                                // The pitot tube sees the forward
                                // velocity in the body axis.
        double p = _pressure_node->getDoubleValue() * INHGTOPSF;
        double r = _density_node->getDoubleValue();
        double v = _velocity_node->getDoubleValue() * KTTOFPS;
        double q = 0.5 * r * v * v; // dynamic
        _total_pressure_node->setDoubleValue((p + q) * PSFTOINHG);
    }
}

// end of pitot.cxx
