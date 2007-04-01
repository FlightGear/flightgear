
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <plib/ul.h>

#include <simgear/nasal/nasal.h>
#include <simgear/props/props.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/interpolator.hxx>
#include <simgear/structure/commands.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "NasalSys.hxx"

// Read and return file contents in a single buffer.  Note use of
// stat() to get the file size.  This is a win32 function, believe it
// or not. :) Note the REALLY IMPORTANT use of the "b" flag to fopen.
// Text mode brain damage will kill us if we're trying to do bytewise
// I/O.
static char* readfile(const char* file, int* lenOut)
{
    struct stat data;
    if(stat(file, &data) != 0) return 0;
    FILE* f = fopen(file, "rb");
    if(!f) return 0;
    char* buf = new char[data.st_size];
    *lenOut = fread(buf, 1, data.st_size, f);
    fclose(f);
    if(*lenOut != data.st_size) {
        // Shouldn't happen, but warn anyway since it represents a
        // platform bug and not a typical runtime error (missing file,
        // etc...)
        SG_LOG(SG_NASAL, SG_ALERT,
               "ERROR in Nasal initialization: " <<
               "short count returned from fread() of " << file <<
               ".  Check your C library!");
        delete[] buf;
        return 0;
    }
    return buf;
}

FGNasalSys::FGNasalSys()
{
    _context = 0;
    _globals = naNil();
    _gcHash = naNil();
    _nextGCKey = 0; // Any value will do
    _callCount = 0;
    _purgeListeners = false;
}

// Does a naCall() in a new context.  Wrapped here to make lock
// tracking easier.  Extension functions are called with the lock, but
// we have to release it before making a new naCall().  So rather than
// drop the lock in every extension function that might call back into
// Nasal, we keep a stack depth counter here and only unlock/lock
// around the naCall if it isn't the first one.
naRef FGNasalSys::call(naRef code, naRef locals)
{
    naContext ctx = naNewContext();
    if(_callCount) naModUnlock();
    _callCount++;
    naRef result = naCall(ctx, code, 0, 0, naNil(), locals);
    if(naGetError(ctx))
        logError(ctx);
    _callCount--;
    if(_callCount) naModLock();
    naFreeContext(ctx);
    return result;
}

FGNasalSys::~FGNasalSys()
{
    map<int, FGNasalListener *>::iterator it, end = _listener.end();
    for(it = _listener.begin(); it != end; ++it)
        delete it->second;

    naFreeContext(_context);
    _globals = naNil();
}

bool FGNasalSys::parseAndRun(const char* sourceCode)
{
    naRef code = parse("FGNasalSys::parseAndRun()", sourceCode,
                       strlen(sourceCode));
    if(naIsNil(code))
        return false;
    call(code, naNil());
    return true;
}

FGNasalScript* FGNasalSys::parseScript(const char* src, const char* name)
{
    FGNasalScript* script = new FGNasalScript();
    script->_gcKey = -1; // important, if we delete it on a parse
    script->_nas = this; // error, don't clobber a real handle!

    char buf[256];
    if(!name) {
        sprintf(buf, "FGNasalScript@%p", script);
        name = buf;
    }

    script->_code = parse(name, src, strlen(src));
    if(naIsNil(script->_code)) {
        delete script;
        return 0;
    }

    script->_gcKey = gcSave(script->_code);
    return script;
}

// Utility.  Sets a named key in a hash by C string, rather than nasal
// string object.
void FGNasalSys::hashset(naRef hash, const char* key, naRef val)
{
    naRef s = naNewString(_context);
    naStr_fromdata(s, (char*)key, strlen(key));
    naHash_set(hash, s, val);
}

// The get/setprop functions accept a *list* of strings and walk
// through the property tree with them to find the appropriate node.
// This allows a Nasal object to hold onto a property path and use it
// like a node object, e.g. setprop(ObjRoot, "size-parsecs", 2.02).  This
// is the utility function that walks the property tree.
// Future enhancement: support integer arguments to specify array
// elements.
static SGPropertyNode* findnode(naContext c, naRef* vec, int len)
{
    SGPropertyNode* p = globals->get_props();
    try {
        for(int i=0; i<len; i++) {
            naRef a = vec[i];
            if(!naIsString(a)) return 0;
            p = p->getNode(naStr_data(a));
            if(p == 0) return 0;
        }
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
        return 0;
    }
    return p;
}

// getprop() extension function.  Concatenates its string arguments as
// property names and returns the value of the specified property.  Or
// nil if it doesn't exist.
static naRef f_getprop(naContext c, naRef me, int argc, naRef* args)
{
    const SGPropertyNode* p = findnode(c, args, argc);
    if(!p) return naNil();

    switch(p->getType()) {
    case SGPropertyNode::BOOL:   case SGPropertyNode::INT:
    case SGPropertyNode::LONG:   case SGPropertyNode::FLOAT:
    case SGPropertyNode::DOUBLE:
        return naNum(p->getDoubleValue());

    case SGPropertyNode::STRING:
    case SGPropertyNode::UNSPECIFIED:
        {
            naRef nastr = naNewString(c);
            const char* val = p->getStringValue();
            naStr_fromdata(nastr, (char*)val, strlen(val));
            return nastr;
        }
    case SGPropertyNode::ALIAS: // <--- FIXME, recurse?
    default:
        return naNil();
    }
}

// setprop() extension function.  Concatenates its string arguments as
// property names and sets the value of the specified property to the
// final argument.
static naRef f_setprop(naContext c, naRef me, int argc, naRef* args)
{
#define BUFLEN 1024
    char buf[BUFLEN + 1];
    buf[BUFLEN] = 0;
    char* p = buf;
    int buflen = BUFLEN;
    for(int i=0; i<argc-1; i++) {
        naRef s = naStringValue(c, args[i]);
        if(naIsNil(s)) return naNil();
        strncpy(p, naStr_data(s), buflen);
        p += naStr_len(s);
        buflen = BUFLEN - (p - buf);
        if(i < (argc-2) && buflen > 0) {
            *p++ = '/';
            buflen--;
        }
    }

    SGPropertyNode* props = globals->get_props();
    naRef val = args[argc-1];
    try {
        if(naIsString(val)) props->setStringValue(buf, naStr_data(val));
        else {
            naRef n = naNumValue(val);
            if(naIsNil(n))
                naRuntimeError(c, "setprop() value is not string or number");
            props->setDoubleValue(buf, n.num);
        }
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
    }
    return naNil();
#undef BUFLEN
}

// print() extension function.  Concatenates and prints its arguments
// to the FlightGear log.  Uses the highest log level (SG_ALERT), to
// make sure it appears.  Is there better way to do this?
static naRef f_print(naContext c, naRef me, int argc, naRef* args)
{
    string buf;
    int n = argc;
    for(int i=0; i<n; i++) {
        naRef s = naStringValue(c, args[i]);
        if(naIsNil(s)) continue;
        buf += naStr_data(s);
    }
    SG_LOG(SG_GENERAL, SG_ALERT, buf);
    return naNum(buf.length());
}

// fgcommand() extension function.  Executes a named command via the
// FlightGear command manager.  Takes a single property node name as
// an argument.
static naRef f_fgcommand(naContext c, naRef me, int argc, naRef* args)
{
    if(argc < 2 || !naIsString(args[0]) || !naIsGhost(args[1]))
        naRuntimeError(c, "bad arguments to fgcommand()");
    naRef cmd = args[0], props = args[1];
    SGPropertyNode_ptr* node = (SGPropertyNode_ptr*)naGhost_ptr(props);
    return naNum(globals->get_commands()->execute(naStr_data(cmd), *node));
}

// settimer(func, dt, simtime) extension function.  Falls through to
// FGNasalSys::setTimer().  See there for docs.
static naRef f_settimer(naContext c, naRef me, int argc, naRef* args)
{
    FGNasalSys* nasal = (FGNasalSys*)globals->get_subsystem("nasal");
    nasal->setTimer(argc, args);
    return naNil();
}

// setlistener(func, property, bool) extension function.  Falls through to
// FGNasalSys::setListener().  See there for docs.
static naRef f_setlistener(naContext c, naRef me, int argc, naRef* args)
{
    FGNasalSys* nasal = (FGNasalSys*)globals->get_subsystem("nasal");
    return nasal->setListener(c, argc, args);
}

// removelistener(int) extension function. Falls through to
// FGNasalSys::removeListener(). See there for docs.
static naRef f_removelistener(naContext c, naRef me, int argc, naRef* args)
{
    FGNasalSys* nasal = (FGNasalSys*)globals->get_subsystem("nasal");
    return nasal->removeListener(c, argc, args);
}

// Returns a ghost handle to the argument to the currently executing
// command
static naRef f_cmdarg(naContext c, naRef me, int argc, naRef* args)
{
    FGNasalSys* nasal = (FGNasalSys*)globals->get_subsystem("nasal");
    return nasal->cmdArgGhost();
}

// Sets up a property interpolation.  The first argument is either a
// ghost (SGPropertyNode_ptr*) or a string (global property path) to
// interpolate.  The second argument is a vector of pairs of
// value/delta numbers.
static naRef f_interpolate(naContext c, naRef me, int argc, naRef* args)
{
    SGPropertyNode* node;
    naRef prop = argc > 0 ? args[0] : naNil();
    if(naIsString(prop)) node = fgGetNode(naStr_data(prop), true);
    else if(naIsGhost(prop)) node = *(SGPropertyNode_ptr*)naGhost_ptr(prop);
    else return naNil();

    naRef curve = argc > 1 ? args[1] : naNil();
    if(!naIsVector(curve)) return naNil();
    int nPoints = naVec_size(curve) / 2;
    double* values = new double[nPoints];
    double* deltas = new double[nPoints];
    for(int i=0; i<nPoints; i++) {
        values[i] = naNumValue(naVec_get(curve, 2*i)).num;
        deltas[i] = naNumValue(naVec_get(curve, 2*i+1)).num;
    }

    ((SGInterpolator*)globals->get_subsystem("interpolator"))
        ->interpolate(node, nPoints, values, deltas);

    return naNil();
}

// This is a better RNG than the one in the default Nasal distribution
// (which is based on the C library rand() implementation). It will
// override.
static naRef f_rand(naContext c, naRef me, int argc, naRef* args)
{
    return naNum(sg_random());
}

static naRef f_srand(naContext c, naRef me, int argc, naRef* args)
{
    sg_srandom_time();
    return naNum(0);
}

// Return an array listing of all files in a directory
static naRef f_directory(naContext c, naRef me, int argc, naRef* args)
{
    if(argc != 1 || !naIsString(args[0]))
        naRuntimeError(c, "bad arguments to directory()");
    naRef ldir = args[0];
    ulDir* dir = ulOpenDir(naStr_data(args[0]));
    if(!dir) return naNil();
    naRef result = naNewVector(c);
    ulDirEnt* dent;
    while((dent = ulReadDir(dir)))
        naVec_append(result, naStr_fromdata(naNewString(c), dent->d_name,
                                            strlen(dent->d_name)));
    ulCloseDir(dir);
    return result;
}

// Table of extension functions.  Terminate with zeros.
static struct { char* name; naCFunction func; } funcs[] = {
    { "getprop",   f_getprop },
    { "setprop",   f_setprop },
    { "print",     f_print },
    { "_fgcommand", f_fgcommand },
    { "settimer",  f_settimer },
    { "_setlistener", f_setlistener },
    { "removelistener", f_removelistener },
    { "_cmdarg",  f_cmdarg },
    { "_interpolate",  f_interpolate },
    { "rand",  f_rand },
    { "srand",  f_srand },
    { "directory", f_directory },
    { 0, 0 }
};

naRef FGNasalSys::cmdArgGhost()
{
    return propNodeGhost(_cmdArg);
}

void FGNasalSys::init()
{
    int i;

    _context = naNewContext();

    // Start with globals.  Add it to itself as a recursive
    // sub-reference under the name "globals".  This gives client-code
    // write access to the namespace if someone wants to do something
    // fancy.
    _globals = naInit_std(_context);
    naSave(_context, _globals);
    hashset(_globals, "globals", _globals);

    hashset(_globals, "math", naInit_math(_context));
    hashset(_globals, "bits", naInit_bits(_context));
    hashset(_globals, "io", naInit_io(_context));
    hashset(_globals, "thread", naInit_thread(_context));
    hashset(_globals, "utf8", naInit_utf8(_context));

    // Add our custom extension functions:
    for(i=0; funcs[i].name; i++)
        hashset(_globals, funcs[i].name,
                naNewFunc(_context, naNewCCode(_context, funcs[i].func)));

    // And our SGPropertyNode wrapper
    hashset(_globals, "props", genPropsModule());

    // Make a "__gcsave" hash to hold the naRef objects which get
    // passed to handles outside the interpreter (to protect them from
    // begin garbage-collected).
    _gcHash = naNewHash(_context);
    hashset(_globals, "__gcsave", _gcHash);

    // Now load the various source files in the Nasal directory
    SGPath p(globals->get_fg_root());
    p.append("Nasal");
    ulDirEnt* dent;
    ulDir* dir = ulOpenDir(p.c_str());
    while(dir && (dent = ulReadDir(dir)) != 0) {
        SGPath fullpath(p);
        fullpath.append(dent->d_name);
        SGPath file(dent->d_name);
        if(file.extension() != "nas") continue;
        loadModule(fullpath, file.base().c_str());
    }
    ulCloseDir(dir);

    // set signal and remove node to avoid restoring at reinit
    const char *s = "nasal-dir-initialized";
    SGPropertyNode *signal = fgGetNode("/sim/signals", true);
    signal->setBoolValue(s, true);
    signal->removeChildren(s);

    // Pull scripts out of the property tree, too
    loadPropertyScripts();
}

void FGNasalSys::update(double)
{
    if(_purgeListeners) {
        _purgeListeners = false;
        map<int, FGNasalListener *>::iterator it;
        for(it = _listener.begin(); it != _listener.end();) {
            FGNasalListener *nl = it->second;
            if(nl->_dead) {
                _listener.erase(it++);
                delete nl;
            } else {
                ++it;
            }
        }
    }
}

// Loads the scripts found under /nasal in the global tree
void FGNasalSys::loadPropertyScripts()
{
    SGPropertyNode* nasal = globals->get_props()->getNode("nasal");
    if(!nasal) return;

    for(int i=0; i<nasal->nChildren(); i++) {
        SGPropertyNode* n = nasal->getChild(i);

        const char* module = n->getName();
        if(n->hasChild("module"))
            module = n->getStringValue("module");

        // allow multiple files to be specified within in a single
        // Nasal module tag
        int j = 0;
        SGPropertyNode *fn;
        bool file_specified = false;
        while ( (fn = n->getChild("file", j)) != NULL ) {
            file_specified = true;
            const char* file = fn->getStringValue();
            SGPath p(globals->get_fg_root());
            p.append(file);
            loadModule(p, module);
            j++;
        }

        // Old code which only allowed a single file to be specified per module
        /*
        const char* file = n->getStringValue("file");
        if(!n->hasChild("file")) file = 0; // Hrm...
        if(file) {
            SGPath p(globals->get_fg_root());
            p.append(file);
            loadModule(p, module);
        }
        */
        
        const char* src = n->getStringValue("script");
        if(!n->hasChild("script")) src = 0; // Hrm...
        if(src)
            createModule(module, n->getPath(), src, strlen(src));

        if(!file_specified && !src)
            SG_LOG(SG_NASAL, SG_ALERT, "Nasal error: " <<
                   "no <file> or <script> defined in " <<
                   "/nasal/" << module);
    }
}

// Logs a runtime error, with stack trace, to the FlightGear log stream
void FGNasalSys::logError(naContext context)
{
    SG_LOG(SG_NASAL, SG_ALERT,
           "Nasal runtime error: " << naGetError(context));
    SG_LOG(SG_NASAL, SG_ALERT,
           "  at " << naStr_data(naGetSourceFile(context, 0)) <<
           ", line " << naGetLine(context, 0));
    for(int i=1; i<naStackDepth(context); i++)
        SG_LOG(SG_NASAL, SG_ALERT,
               "  called from: " << naStr_data(naGetSourceFile(context, i)) <<
               ", line " << naGetLine(context, i));
}

// Reads a script file, executes it, and places the resulting
// namespace into the global namespace under the specified module
// name.
void FGNasalSys::loadModule(SGPath file, const char* module)
{
    int len = 0;
    char* buf = readfile(file.c_str(), &len);
    if(!buf) {
        SG_LOG(SG_NASAL, SG_ALERT,
               "Nasal error: could not read script file " << file.c_str()
               << " into module " << module);
        return;
    }

    createModule(module, file.c_str(), buf, len);
    delete[] buf;
}

// Parse and run.  Save the local variables namespace, as it will
// become a sub-object of globals.  The optional "arg" argument can be
// used to pass an associated property node to the module, which can then
// be accessed via cmdarg().  (This is, for example, used by XML dialogs.)
void FGNasalSys::createModule(const char* moduleName, const char* fileName,
                              const char* src, int len, const SGPropertyNode* arg)
{
    naRef code = parse(fileName, src, len);
    if(naIsNil(code))
        return;

    // See if we already have a module hash to use.  This allows the
    // user to, for example, add functions to the built-in math
    // module.  Make a new one if necessary.
    naRef locals;
    naRef modname = naNewString(_context);
    naStr_fromdata(modname, (char*)moduleName, strlen(moduleName));
    if(!naHash_get(_globals, modname, &locals))
        locals = naNewHash(_context);

    _cmdArg = (SGPropertyNode*)arg;

    call(code, locals);
    hashset(_globals, moduleName, locals);
}

void FGNasalSys::deleteModule(const char* moduleName)
{
    naRef modname = naNewString(_context);
    naStr_fromdata(modname, (char*)moduleName, strlen(moduleName));
    naHash_delete(_globals, modname);
}

naRef FGNasalSys::parse(const char* filename, const char* buf, int len)
{
    int errLine = -1;
    naRef srcfile = naNewString(_context);
    naStr_fromdata(srcfile, (char*)filename, strlen(filename));
    naRef code = naParseCode(_context, srcfile, 1, (char*)buf, len, &errLine);
    if(naIsNil(code)) {
        SG_LOG(SG_NASAL, SG_ALERT,
               "Nasal parse error: " << naGetError(_context) <<
               " in "<< filename <<", line " << errLine);
        return naNil();
    }

    // Bind to the global namespace before returning
    return naBindFunction(_context, code, _globals);
}

bool FGNasalSys::handleCommand(const SGPropertyNode* arg)
{
    const char* nasal = arg->getStringValue("script");
    const char* moduleName = arg->getStringValue("module");
    naRef code = parse(arg->getPath(true), nasal, strlen(nasal));
    if(naIsNil(code)) return false;

    // Commands can be run "in" a module.  Make sure that module
    // exists, and set it up as the local variables hash for the
    // command.
    naRef locals = naNil();
    if(moduleName[0]) {
        naRef modname = naNewString(_context);
        naStr_fromdata(modname, (char*)moduleName, strlen(moduleName));
        if(!naHash_get(_globals, modname, &locals)) {
            locals = naNewHash(_context);
            naHash_set(_globals, modname, locals);
        }
    }

    // Cache this command's argument for inspection via cmdarg().  For
    // performance reasons, we won't bother with it if the invoked
    // code doesn't need it.
    _cmdArg = (SGPropertyNode*)arg;

    call(code, locals);
    return true;
}

// settimer(func, dt, simtime) extension function.  The first argument
// is a Nasal function to call, the second is a delta time (from now),
// in seconds.  The third, if present, is a boolean value indicating
// that "real world" time (rather than simulator time) is to be used.
//
// Implementation note: the FGTimer objects don't live inside the
// garbage collector, so the Nasal handler functions have to be
// "saved" somehow lest they be inadvertently cleaned.  In this case,
// they are inserted into a globals.__gcsave hash and removed on
// expiration.
void FGNasalSys::setTimer(int argc, naRef* args)
{
    // Extract the handler, delta, and simtime arguments:
    naRef handler = argc > 0 ? args[0] : naNil();
    if(!(naIsCode(handler) || naIsCCode(handler) || naIsFunc(handler)))
        return;

    naRef delta = argc > 1 ? args[1] : naNil();
    if(naIsNil(delta)) return;

    bool simtime = (argc > 2 && naTrue(args[2])) ? false : true;

    // Generate and register a C++ timer handler
    NasalTimer* t = new NasalTimer;
    t->handler = handler;
    t->gcKey = gcSave(handler);
    t->nasal = this;

    globals->get_event_mgr()->addEvent("NasalTimer",
                                       t, &NasalTimer::timerExpired,
                                       delta.num, simtime);
}

void FGNasalSys::handleTimer(NasalTimer* t)
{
    call(t->handler, naNil());
    gcRelease(t->gcKey);
}

int FGNasalSys::gcSave(naRef r)
{
    int key = _nextGCKey++;
    naHash_set(_gcHash, naNum(key), r);
    return key;
}

void FGNasalSys::gcRelease(int key)
{
    naHash_delete(_gcHash, naNum(key));
}

void FGNasalSys::NasalTimer::timerExpired()
{
    nasal->handleTimer(this);
    delete this;
}

int FGNasalSys::_listenerId = 0;

// setlistener(property, func, bool) extension function.  The first argument
// is either a ghost (SGPropertyNode_ptr*) or a string (global property
// path), the second is a Nasal function, the optional third one a bool.
// If the bool is true, then the listener is executed initially. The
// setlistener() function returns a unique id number, that can be used
// as argument to the removelistener() function.
naRef FGNasalSys::setListener(naContext c, int argc, naRef* args)
{
    SGPropertyNode_ptr node;
    naRef prop = argc > 0 ? args[0] : naNil();
    if(naIsString(prop)) node = fgGetNode(naStr_data(prop), true);
    else if(naIsGhost(prop)) node = *(SGPropertyNode_ptr*)naGhost_ptr(prop);
    else {
        naRuntimeError(c, "setlistener() with invalid property argument");
        return naNil();
    }

    if(node->isTied())
        SG_LOG(SG_NASAL, SG_DEBUG, "Attaching listener to tied property " <<
                node->getPath());

    naRef handler = argc > 1 ? args[1] : naNil();
    if(!(naIsCode(handler) || naIsCCode(handler) || naIsFunc(handler))) {
        naRuntimeError(c, "setlistener() with invalid function argument");
        return naNil();
    }

    bool initial = argc > 2 && naTrue(args[2]);

    FGNasalListener *nl = new FGNasalListener(node, handler, this,
            gcSave(handler));
    node->addChangeListener(nl, initial);

    _listener[_listenerId] = nl;
    return naNum(_listenerId++);
}

// removelistener(int) extension function. The argument is the id of
// a listener as returned by the setlistener() function.
naRef FGNasalSys::removeListener(naContext c, int argc, naRef* args)
{
    naRef id = argc > 0 ? args[0] : naNil();
    map<int, FGNasalListener *>::iterator it = _listener.find(int(id.num));

    if(!naIsNum(id) || it == _listener.end() || it->second->_dead) {
        naRuntimeError(c, "removelistener() with invalid listener id");
        return naNil();
    }

    FGNasalListener *nl = it->second;
    if(nl->_active) {
        nl->_dead = true;
        _purgeListeners = true;
        return naNum(-1);
    }

    _listener.erase(it);
    delete nl;
    return naNum(_listener.size());
}



// FGNasalListener class.

FGNasalListener::FGNasalListener(SGPropertyNode_ptr node, naRef handler,
                                 FGNasalSys* nasal, int key) :
    _node(node),
    _handler(handler),
    _gcKey(key),
    _nas(nasal),
    _active(0),
    _dead(false)
{
}

FGNasalListener::~FGNasalListener()
{
    _node->removeChangeListener(this);
    _nas->gcRelease(_gcKey);
}

void FGNasalListener::valueChanged(SGPropertyNode* node)
{
    // drop recursive listener calls
    if(_active || _dead)
        return;

    _active++;
    _nas->_cmdArg = node;
    _nas->call(_handler, naNil());
    _active--;
}




// FGNasalModelData class.  If sgLoad3DModel() is called with a pointer to
// such a class, then it lets modelLoaded() run the <load> script, and the
// destructor the <unload> script. The latter happens when the model branch
// is removed from the scene graph.

void FGNasalModelData::modelLoaded(const string& path, SGPropertyNode *prop,
                                   ssgBranch *)
{
    SGPropertyNode *n = prop->getNode("nasal"), *load;
    if(!n)
        return;

    load = n->getNode("load");
    _unload = n->getNode("unload");
    if(!load && !_unload)
        return;

    _module = path;
    const char *s = load ? load->getStringValue() : "";
    FGNasalSys *nas = (FGNasalSys *)globals->get_subsystem("nasal");
    nas->createModule(_module.c_str(), _module.c_str(), s, strlen(s), _props);
}

FGNasalModelData::~FGNasalModelData()
{
    if(_module.empty())
        return;

    FGNasalSys *nas = (FGNasalSys *)globals->get_subsystem("nasal");
    if(!nas) {
        SG_LOG(SG_NASAL, SG_ALERT, "Trying to run an <unload> script "
                "without Nasal subsystem present.");
        return;
    }

    if(_unload) {
        const char *s = _unload->getStringValue();
        nas->createModule(_module.c_str(), _module.c_str(), s, strlen(s), _props);
    }
    nas->deleteModule(_module.c_str());
}


