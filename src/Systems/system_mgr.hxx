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

#include <Main/fgfs.hxx>

#include <vector>

SG_USING_STD(vector);


/**
 * Manage aircraft systems.
 *
 * In the initial draft, the systems present are hard-coded, but they
 * will soon be configurable for individual aircraft.
 */
class FGSystemMgr : public FGSubsystem
{
public:

    FGSystemMgr ();
    virtual ~FGSystemMgr ();

    virtual void init ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double dt);

private:
    vector<FGSubsystem *> _systems;

};

#endif // __SYSTEM_MGR_HXX
