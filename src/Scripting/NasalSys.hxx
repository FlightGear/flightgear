#ifndef __NASALSYS_HXX
#define __NASALSYS_HXX

#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/nasal/nasal.h>

class FGNasalSys : public SGSubsystem
{
public:
    FGNasalSys();
    virtual ~FGNasalSys();
    virtual void init();
    virtual void update(double dt) { /* noop */ }

    virtual bool handleCommand(const SGPropertyNode* arg);

    // Implementation of the settimer extension function
    void setTimer(naRef args);

private:
    //
    // FGTimer subclass for handling Nasal timer callbacks.
    // See the implementation of the settimer() extension function for
    // more notes.
    //
    struct NasalTimer {
        virtual void timerExpired();
        naRef handler;
        int hashKey;
        FGNasalSys* nasal;
    };

    void readScriptFile(SGPath file, const char* lib);
    void hashset(naRef hash, const char* key, naRef val);
    void logError();
    naRef parse(const char* filename, const char* buf, int len);
    void handleTimer(NasalTimer* t);

    naContext _context;
    naRef _globals;
    naRef _timerHash;

    int _nextTimerHashKey;
};

#endif // __NASALSYS_HXX
