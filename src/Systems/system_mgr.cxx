// system_mgr.cxx - manage aircraft systems.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>

#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>

#include "system_mgr.hxx"
#include "electrical.hxx"
#include "pitot.hxx"
#include "static.hxx"
#include "vacuum.hxx"


FGSystemMgr::FGSystemMgr ()
{
    SGPropertyNode_ptr config_props = new SGPropertyNode;

    SGPropertyNode *path_n = fgGetNode("/sim/systems/path");

    if (path_n) {
        SGPath config = globals->resolve_aircraft_path(path_n->getStringValue());
        if (!config.exists()) {
                SG_LOG( SG_SYSTEMS, SG_DEV_ALERT, "System model file not found:" << config);
            return;
        }

        SG_LOG( SG_SYSTEMS, SG_INFO, "Reading systems from "
                << config );
        try
        {
          readProperties( config, config_props );
          build(config_props);
        }
        catch( const sg_exception& )
        {
          SG_LOG( SG_SYSTEMS, SG_ALERT, "Failed to load systems system model: "
                  << config );
        }

    } else {
        SG_LOG( SG_SYSTEMS, SG_WARN,
                "No systems model specified for this model!");
    }

}

FGSystemMgr::~FGSystemMgr ()
{
}

bool FGSystemMgr::build (SGPropertyNode* config_props)
{
    SGPropertyNode *node;
    int i;

    int count = config_props->nChildren();
    for ( i = 0; i < count; ++i ) {
        node = config_props->getChild(i);
        string name = node->getName();
        std::ostringstream temp;
        temp << i;
        if ( name == "electrical" ) {
            set_subsystem( "electrical" + temp.str(),
                           new FGElectricalSystem( node ) );
        } else if ( name == "pitot" ) {
            set_subsystem( "system" + temp.str(), 
                           new PitotSystem( node ) );
        } else if ( name == "static" ) {
            set_subsystem( "system" + temp.str(), 
                           new StaticSystem( node ) );
        } else if ( name == "vacuum" ) {
            set_subsystem( "system" + temp.str(), 
                           new VacuumSystem( node ) );
        } else {
            SG_LOG(SG_SYSTEMS, SG_ALERT, "Ignoring unknown system: " << name);
        }
    }
    return true;
}


// Register the subsystem.
SGSubsystemMgr::Registrant<FGSystemMgr> registrantFGSystemMgr(
    SGSubsystemMgr::FDM);

// end of system_manager.cxx
