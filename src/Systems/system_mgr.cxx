// system_mgr.cxx - manage aircraft systems.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/sg_inlines.h>

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/util.hxx>

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
    set_subsystem( "electrical", new FGElectricalSystem );

    config_props = new SGPropertyNode;

    SGPropertyNode *path_n = fgGetNode("/sim/systems/path");

    if (path_n) {
        SGPath config( globals->get_fg_root() );
        config.append( path_n->getStringValue() );

        SG_LOG( SG_ALL, SG_INFO, "Reading systems from "
                << config.str() );
        try {
            readProperties( config.str(), config_props );

            if ( build() ) {
                enabled = true;
            } else {
                SG_LOG( SG_ALL, SG_ALERT,
                        "Detected an internal inconsistancy in the systems");
                SG_LOG( SG_ALL, SG_ALERT,
                        " system specification file.  See earlier errors for" );
                SG_LOG( SG_ALL, SG_ALERT,
                        " details.");
                exit(-1);
            }        
        } catch (const sg_exception& exc) {
            SG_LOG( SG_ALL, SG_ALERT, "Failed to load systems system model: "
                    << config.str() );
        }

    } else {
        SG_LOG( SG_ALL, SG_WARN,
                "No systems model specified for this model!");
    }

    delete config_props;
}

FGSystemMgr::~FGSystemMgr ()
{
}

bool FGSystemMgr::build ()
{
    SGPropertyNode *node;
    int i;

    int count = config_props->nChildren();
    for ( i = 0; i < count; ++i ) {
        node = config_props->getChild(i);
        string name = node->getName();
        std::ostringstream temp;
        temp << i;
        if ( name == "pitot" ) {
            set_subsystem( "system" + temp.str(), 
                           new PitotSystem( node ) );
        } else if ( name == "static" ) {
            set_subsystem( "system" + temp.str(), 
                           new StaticSystem( node ) );
        } else if ( name == "vacuum" ) {
            set_subsystem( "system" + temp.str(), 
                           new VacuumSystem( node ) );
        } else {
            SG_LOG( SG_ALL, SG_ALERT, "Unknown top level section: " 
                    << name );
            return false;
        }
    }
    return true;
}

// end of system_manager.cxx
