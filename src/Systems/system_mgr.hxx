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
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Manage aircraft systems.
 *
 * In the initial draft, the systems present are hard-coded, but they
 * will soon be configurable for individual aircraft.
 */
class FGSystemMgr : public SGSubsystemGroup
{
public:

    FGSystemMgr ();
    virtual ~FGSystemMgr ();
};

#endif // __SYSTEM_MGR_HXX
