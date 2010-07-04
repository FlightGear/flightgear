// instrument_mgr.hxx - manage aircraft instruments.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#ifndef __INSTRUMENT_MGR_HXX
#define __INSTRUMENT_MGR_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>


/**
 * Manage aircraft instruments.
 *
 * In the initial draft, the instruments present are hard-coded, but they
 * will soon be configurable for individual aircraft.
 */
class FGInstrumentMgr : public SGSubsystemGroup
{
public:

    FGInstrumentMgr ();
    virtual ~FGInstrumentMgr ();
    
    virtual void init();
    virtual void reinit();
private:
    bool build (SGPropertyNode* config_props);
    
    bool _explicitGps;
    
    std::vector<std::string> _instruments;
};

#endif // __INSTRUMENT_MGR_HXX
