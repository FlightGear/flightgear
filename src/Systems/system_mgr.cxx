// system_mgr.cxx - manage aircraft systems.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#include "system_mgr.hxx"
#include "electrical.hxx"
#include "pitot.hxx"
#include "static.hxx"
#include "vacuum.hxx"
#include "submodel.hxx"


FGSystemMgr::FGSystemMgr ()
{
    set_subsystem( "electrical", new FGElectricalSystem );
    set_subsystem( "pitot", new PitotSystem );
    set_subsystem( "static", new StaticSystem );
    set_subsystem( "vacuum-l", new VacuumSystem(0) );
    set_subsystem( "vacuum-r", new VacuumSystem(1) );
    set_subsystem( "submodel", new SubmodelSystem() );
}

FGSystemMgr::~FGSystemMgr ()
{
}

// end of system_manager.cxx
