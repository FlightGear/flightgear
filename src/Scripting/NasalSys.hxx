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

    // Simple hook to run arbitrary source code.  Returns a bool to
    // indicate successful execution.  Does *not* return any Nasal
    // values, because handling garbage-collected objects from C space
    // is deep voodoo and violates the "simple hook" idea.
    bool parseAndRun(const char* sourceCode);

    // Implementation of the settimer extension function
    void setTimer(naRef args);

    // Returns a ghost wrapper for the current _cmdArg
    naRef cmdArgGhost();
    
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

    void loadPropertyScripts();
    void initModule(const char* moduleName, const char* fileName,
                    const char* src, int len);
    void readScriptFile(SGPath file, const char* lib);
    void hashset(naRef hash, const char* key, naRef val);
    void logError();
    naRef parse(const char* filename, const char* buf, int len);
    naRef genPropsModule();
    naRef propNodeGhost(SGPropertyNode* handle);

    naContext _context;
    naRef _globals;
    naRef _timerHash;

    SGPropertyNode* _cmdArg;

    int _nextTimerHashKey;

public:
         void handleTimer(NasalTimer* t);
};

#endif // __NASALSYS_HXX
