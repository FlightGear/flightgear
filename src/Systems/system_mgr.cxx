// system_mgr.cxx - manage aircraft systems.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#include "system_mgr.hxx"
#include "Vacuum/vacuum.hxx"


FGSystemMgr::FGSystemMgr ()
{
    // NO-OP
}

FGSystemMgr::~FGSystemMgr ()
{
    for (int i = 0; i < _systems.size(); i++) {
        delete _systems[i];
        _systems[i] = 0;
    }
}

void
FGSystemMgr::init ()
{
                                // TODO: replace with XML configuration
    _systems.push_back(new VacuumSystem);

                                // Initialize the individual systems
    for (int i = 0; i < _systems.size(); i++)
        _systems[i]->init();
}

void
FGSystemMgr::bind ()
{
    // NO-OP
}

void
FGSystemMgr::unbind ()
{
    // NO-OP
}

void
FGSystemMgr::update (double dt)
{
    for (int i = 0; i < _systems.size(); i++)
        _systems[i]->update(dt);
}

// end of system_manager.cxx
