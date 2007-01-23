#ifndef __NASALSYS_HXX
#define __NASALSYS_HXX

#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/nasal/nasal.h>
#include <simgear/scene/model/model.hxx>

#include <map>
SG_USING_STD(map);


class FGNasalScript;
class FGNasalListener;

class FGNasalSys : public SGSubsystem
{
public:
    FGNasalSys();
    virtual ~FGNasalSys();
    virtual void init();
    virtual void update(double dt);

    // Loads a nasal script from an external file and inserts it as a
    // global module of the specified name.
    void loadModule(SGPath file, const char* moduleName);

    // Simple hook to run arbitrary source code.  Returns a bool to
    // indicate successful execution.  Does *not* return any Nasal
    // values, because handling garbage-collected objects from C space
    // is deep voodoo and violates the "simple hook" idea.
    bool parseAndRun(const char* sourceCode);

    // Slightly more complicated hook to get a handle to a precompiled
    // Nasal script that can be invoked via a call() method.  The
    // caller is expected to delete the FGNasalScript returned from
    // this function.  The "name" argument specifies the "file name"
    // for the source code that will be printed in Nasal stack traces
    // on error.
    FGNasalScript* parseScript(const char* src, const char* name=0);

    // Implementation of the settimer extension function
    void setTimer(int argc, naRef* args);

    // Implementation of the setlistener extension function
    naRef setListener(int argc, naRef* args);
    naRef removeListener(naContext c, int argc, naRef* args);

    // Returns a ghost wrapper for the current _cmdArg
    naRef cmdArgGhost();

    // Callbacks for command and timer bindings
    virtual bool handleCommand(const SGPropertyNode* arg);

    void createModule(const char* moduleName, const char* fileName,
                    const char* src, int len, const SGPropertyNode* arg=0);

    void deleteModule(const char* moduleName);

    naRef call(naRef code, naRef locals);

private:
    friend class FGNasalScript;
    friend class FGNasalListener;

    //
    // FGTimer subclass for handling Nasal timer callbacks.
    // See the implementation of the settimer() extension function for
    // more notes.
    //
    struct NasalTimer {
        virtual void timerExpired();
        virtual ~NasalTimer() {}
        naRef handler;
        int gcKey;
        FGNasalSys* nasal;
    };

    // Listener
    map<int, FGNasalListener *> _listener;
    static int _listenerId;
    bool _purgeListeners;

    void loadPropertyScripts();
    void hashset(naRef hash, const char* key, naRef val);
    void logError(naContext);
    naRef parse(const char* filename, const char* buf, int len);
    naRef genPropsModule();
    naRef propNodeGhost(SGPropertyNode* handle);

    // This mechanism is here to allow naRefs to be passed to
    // locations "outside" the interpreter.  Normally, such a
    // reference would be garbage collected unexpectedly.  By passing
    // it to gcSave and getting a key/handle, it can be cached in a
    // globals.__gcsave hash.  Be sure to release it with gcRelease
    // when done.
    int gcSave(naRef r);
    void gcRelease(int key);

    naContext _context;
    naRef _globals;

    SGPropertyNode_ptr _cmdArg;

    int _nextGCKey;
    naRef _gcHash;
    int _callCount;

    public: void handleTimer(NasalTimer* t);
};


class FGNasalScript {
public:
    ~FGNasalScript() { _nas->gcRelease(_gcKey); }

    bool call() {
        naRef n = naNil();
        naCall(_nas->_context, _code, 0, &n, naNil(), naNil());
        return naGetError(_nas->_context) == 0;
    }

private:
    friend class FGNasalSys;
    naRef _code;
    int _gcKey;
    FGNasalSys* _nas;
};


class FGNasalListener : public SGPropertyChangeListener {
public:
    FGNasalListener(SGPropertyNode_ptr node, naRef handler,
                    FGNasalSys* nasal, int key);

    ~FGNasalListener();
    void valueChanged(SGPropertyNode* node);

private:
    friend class FGNasalSys;
    SGPropertyNode_ptr _node;
    naRef _handler;
    int _gcKey;
    FGNasalSys* _nas;
    unsigned int _active;
    bool _dead;
};


class FGNasalModelData : public SGModelData {
public:
    FGNasalModelData() : _unload(0) {}
    ~FGNasalModelData();
    void modelLoaded(const string& path, SGPropertyNode *prop, ssgBranch *);

private:
    string _module;
    SGConstPropertyNode_ptr _unload;
};

#endif // __NASALSYS_HXX
