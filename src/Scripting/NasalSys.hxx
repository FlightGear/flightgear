#ifndef __NASALSYS_HXX
#define __NASALSYS_HXX

#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/nasal/nasal.h>
#include <simgear/scene/model/modellib.hxx>
#include <simgear/xml/easyxml.hxx>
#include <simgear/threads/SGQueue.hxx>

#include <map>


class FGNasalScript;
class FGNasalListener;


/** Nasal model data container.
 * load and unload methods must be run in main thread (not thread-safe). */
class FGNasalModelData : public SGReferenced
{
public:
    /** Constructor to be run in an arbitrary thread. */
    FGNasalModelData(SGPropertyNode *root, const string& path, SGPropertyNode *prop,
                     SGPropertyNode* load, SGPropertyNode* unload) :
        _path(path),
        _root(root), _prop(prop),
        _load(load), _unload(unload)
     {
     }

    /** Load hook. Always call from inside the main loop. */
    void load();

    /** Unload hook. Always call from inside the main loop. */
    void unload();

private:
    static unsigned int _module_id;

    string _module, _path;
    SGPropertyNode_ptr _root, _prop;
    SGConstPropertyNode_ptr _load, _unload;
};

/** Thread-safe proxy for FGNasalModelData.
 * modelLoaded/destroy methods only register the requested
 * operation. Actual (un)loading of Nasal module is deferred
 * and done in the main loop. */
class FGNasalModelDataProxy : public simgear::SGModelData
{
public:
    FGNasalModelDataProxy(SGPropertyNode *root = 0) :
        _root(root), _data(0)
    {
    }

    ~FGNasalModelDataProxy();

    void modelLoaded(const string& path, SGPropertyNode *prop, osg::Node *);

protected:
    SGPropertyNode_ptr _root;
    SGSharedPtr<FGNasalModelData> _data;
};

class FGNasalSys : public SGSubsystem
{
public:
    FGNasalSys();
    virtual ~FGNasalSys();
    virtual void init();
    virtual void update(double dt);

    // Loads a nasal script from an external file and inserts it as a
    // global module of the specified name.
    bool loadModule(SGPath file, const char* moduleName);

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
    void setTimer(naContext c, int argc, naRef* args);

    // Implementation of the setlistener extension function
    naRef setListener(naContext c, int argc, naRef* args);
    naRef removeListener(naContext c, int argc, naRef* args);

    // Returns a ghost wrapper for the current _cmdArg
    naRef cmdArgGhost();

    // Callbacks for command and timer bindings
    virtual bool handleCommand(const SGPropertyNode* arg);

    bool createModule(const char* moduleName, const char* fileName,
                      const char* src, int len, const SGPropertyNode* cmdarg=0,
                      int argc=0, naRef*args=0);

    void deleteModule(const char* moduleName);

    naRef call(naRef code, int argc, naRef* args, naRef locals);
  
    naRef callMethod(naRef code, naRef self, int argc, naRef* args, naRef locals);
  
    naRef propNodeGhost(SGPropertyNode* handle);

    void registerToLoad(FGNasalModelData* data)   { _loadList.push(data);}
    void registerToUnload(FGNasalModelData* data) { _unloadList.push(data);}

    // can't call this 'globals' due to naming clash
    naRef nasalGlobals() const
    { return _globals; }
  
    naContext context() const
    { return _context; }
  
    // This mechanism is here to allow naRefs to be passed to
    // locations "outside" the interpreter.  Normally, such a
    // reference would be garbage collected unexpectedly.  By passing
    // it to gcSave and getting a key/handle, it can be cached in a
    // globals.__gcsave hash.  Be sure to release it with gcRelease
    // when done.
    int gcSave(naRef r);
    void gcRelease(int key);
private:
    friend class FGNasalScript;
    friend class FGNasalListener;
    friend class FGNasalModuleListener;

    SGLockedQueue<SGSharedPtr<FGNasalModelData> > _loadList;
    SGLockedQueue<SGSharedPtr<FGNasalModelData> > _unloadList;

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
    std::map<int, FGNasalListener *> _listener;
    vector<FGNasalListener *> _dead_listener;
    static int _listenerId;

    void loadPropertyScripts();
    void loadPropertyScripts(SGPropertyNode* n);
    void loadScriptDirectory(simgear::Dir nasalDir);
    void addModule(string moduleName, simgear::PathList scripts);
    void hashset(naRef hash, const char* key, naRef val);
    void logError(naContext);
    naRef parse(const char* filename, const char* buf, int len);
    naRef genPropsModule();

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
    FGNasalListener(SGPropertyNode* node, naRef code, FGNasalSys* nasal,
                    int key, int id, int init, int type);

    virtual ~FGNasalListener();
    virtual void valueChanged(SGPropertyNode* node);
    virtual void childAdded(SGPropertyNode* parent, SGPropertyNode* child);
    virtual void childRemoved(SGPropertyNode* parent, SGPropertyNode* child);

private:
    bool changed(SGPropertyNode* node);
    void call(SGPropertyNode* which, naRef mode);

    friend class FGNasalSys;
    SGPropertyNode_ptr _node;
    naRef _code;
    int _gcKey;
    int _id;
    FGNasalSys* _nas;
    int _init;
    int _type;
    unsigned int _active;
    bool _dead;
    long _last_int;
    double _last_float;
    string _last_string;
};


class NasalXMLVisitor : public XMLVisitor {
public:
    NasalXMLVisitor(naContext c, int argc, naRef* args);
    virtual ~NasalXMLVisitor() { naFreeContext(_c); }

    virtual void startElement(const char* tag, const XMLAttributes& a);
    virtual void endElement(const char* tag);
    virtual void data(const char* str, int len);
    virtual void pi(const char* target, const char* data);

private:
    void call(naRef func, int num, naRef a = naNil(), naRef b = naNil());
    naRef make_string(const char* s, int n = -1);

    naContext _c;
    naRef _start_element, _end_element, _data, _pi;
};

#endif // __NASALSYS_HXX
