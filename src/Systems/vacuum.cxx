// vacuum.cxx - a vacuum pump connected to the aircraft engine.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "vacuum.hxx"
#include <Main/fg_props.hxx>


VacuumSystem::VacuumSystem ( SGPropertyNode *node )
    :
    name("vacuum"),
    num(0),
    scale(1.0)

{
    rpms.clear();
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else if ( cname == "rpm" ) {
            rpms.push_back(cval);
        } else if ( cname == "scale" ) {
            scale = child->getDoubleValue();
        } else {
            SG_LOG( SG_SYSTEMS, SG_WARN, "Error in vacuum config logic" );
            if ( name.length() ) {
                SG_LOG( SG_SYSTEMS, SG_WARN, "Section = " << name );
            }
        }
    }
}

VacuumSystem::VacuumSystem( int i )
{
    name = "vacuum";
    num = i;
    rpms.clear();
    scale = 1.0;
}

VacuumSystem::~VacuumSystem ()
{
}

void
VacuumSystem::init()
{
    unsigned int i;
    string branch;
    branch = "/systems/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    for ( i = 0; i < rpms.size(); i++ ) {
      SGPropertyNode_ptr _rpm_node = fgGetNode(rpms[i].c_str(), true);
      _rpm_nodes.push_back( _rpm_node );
    }
    _pressure_node = fgGetNode("/environment/pressure-inhg", true);
    _suction_node = node->getChild("suction-inhg", 0, true);
}

void
VacuumSystem::bind ()
{
}

void
VacuumSystem::unbind ()
{
}

void
VacuumSystem::update (double dt)
{
    // Model taken from steam.cxx

    double suction;
    unsigned int i;

    if (!_serviceable_node->getBoolValue()) {
        suction = 0.0;
    } else {
	// select the source with the max rpm
        double rpm = 0.0;
	for ( i = 0; i < _rpm_nodes.size(); i++ ) {
	  double tmp = _rpm_nodes[i]->getDoubleValue() * scale;
	  if ( tmp > rpm ) {
	    rpm = tmp;
	  }
	}
        double pressure = _pressure_node->getDoubleValue();
        // This magic formula yields about 4 inhg at 700 rpm
        suction = pressure * rpm / (rpm + 4875.0);

        // simple regulator model that clamps smoothly to about 5 inhg
        // over a normal rpm range
        double max = (rpm > 0 ? 5.39 - 1.0 / ( rpm * 0.00111 ) : 0);
        if ( suction < 0.0 ) suction = 0.0;
        if ( suction > max ) suction = max;
    }
    _suction_node->setDoubleValue(suction);
}

// end of vacuum.cxx
