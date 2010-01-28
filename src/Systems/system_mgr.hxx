// system_mgr.hxx - manage aircraft systems.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __SYSTEM_MGR_HXX
#define __SYSTEM_MGR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Manage aircraft systems.
 *
 * Multiple aircraft systems can be configured for each aircraft.
 */
class FGSystemMgr : public SGSubsystemGroup
{
public:

    FGSystemMgr ();
    virtual ~FGSystemMgr ();
    bool build (SGPropertyNode* config_props);

private:
    bool enabled;

};

#endif // __SYSTEM_MGR_HXX
