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
}

FGNasalSys::~FGNasalSys()
{
    // Nasal doesn't have a "destroy context" API yet. :(
    // Not a problem for a global subsystem that will never be
    // destroyed.  And the context is actually a global, so no memory
    // is technically leaked (although the GC pool memory obviously
    // won't be freed).
    _context = 0;
    _globals = naNil();
}

bool FGNasalSys::parseAndRun(const char* sourceCode)
{
    naRef code = parse("FGNasalSys::parseAndRun()", sourceCode,
                       strlen(sourceCode));
    if(naIsNil(code))
        return false;

    naCall(_context, code, naNil(), naNil(), naNil());

    if(!naGetError(_context)) return true;
    logError();
    return false;
}

FGNasalScript* FGNasalSys::parseScript(const char* src, const char* name)
{
    FGNasalScript* script = new FGNasalScript();
    script->_gcKey = -1; // important, if we delete it on a parse
    script->_nas = this; // error, don't clobber a real handle!

    char buf[256];
    if(!name) {
        sprintf(buf, "FGNasalScript@%8.8x", (int)script);
        name = buf;
    }

    script->_code = parse(name, src);
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
static SGPropertyNode* findnode(naContext c, naRef vec, int len)
{
    SGPropertyNode* p = globals->get_props();
    for(int i=0; i<len; i++) {
        naRef a = naVec_get(vec, i);
        if(!naIsString(a)) return 0;
        p = p->getNode(naStr_data(a));
        if(p == 0) return 0;
    }
    return p;
}

// getprop() extension function.  Concatenates its string arguments as
// property names and returns the value of the specified property.  Or
// nil if it doesn't exist.
static naRef f_getprop(naContext c, naRef args)
{
    const SGPropertyNode* p = findnode(c, args, naVec_size(args));
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
static naRef f_setprop(naContext c, naRef args)
{
#define BUFLEN 1024
    int argc = naVec_size(args);
    char buf[BUFLEN + 1];
    buf[BUFLEN] = 0;
    char* p = buf;
    int buflen = BUFLEN;
    for(int i=0; i<argc-1; i++) {
        naRef s = naStringValue(c, naVec_get(args, i));
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
    naRef val = naVec_get(args, argc-1);
    if(naIsString(val)) props->setStringValue(buf, naStr_data(val));
    else                props->setDoubleValue(buf, naNumValue(val).num);
    return naNil();
#undef BUFLEN
}

// print() extension function.  Concatenates and prints its arguments
// to the FlightGear log.  Uses the highest log level (SG_ALERT), to
// make sure it appears.  Is there better way to do this?
static naRef f_print(naContext c, naRef args)
{
#define BUFLEN 1024
    char buf[BUFLEN + 1];
    buf[BUFLEN] = 0; // extra nul to handle strncpy brain damage
    buf[0] = 0; // Zero-length in case there are no arguments
    char* p = buf;
    int buflen = BUFLEN;
    int n = naVec_size(args);
    for(int i=0; i<n; i++) {
        naRef s = naStringValue(c, naVec_get(args, i));
        if(naIsNil(s)) continue;
        strncpy(p, naStr_data(s), buflen);
        p += naStr_len(s);
        buflen = BUFLEN - (p - buf);
        if(buflen <= 0) break;
    }
    SG_LOG(SG_GENERAL, SG_ALERT, buf);
    return naNil();
#undef BUFLEN
}

// fgcommand() extension function.  Executes a named command via the
// FlightGear command manager.  Takes a single property node name as
// an argument.
static naRef f_fgcommand(naContext c, naRef args)
{
    naRef cmd = naVec_get(args, 0);
    naRef props = naVec_get(args, 1);
    if(!naIsString(cmd) || !naIsGhost(props)) return naNil();
    SGPropertyNode_ptr* node = (SGPropertyNode_ptr*)naGhost_ptr(props);
    globals->get_commands()->execute(naStr_data(cmd), *node);
    return naNil();

}

// settimer(func, dt, simtime) extension function.  Falls through to
// FGNasalSys::setTimer().  See there for docs.
static naRef f_settimer(naContext c, naRef args)
{
    FGNasalSys* nasal = (FGNasalSys*)globals->get_subsystem("nasal");
    nasal->setTimer(args);
    return naNil();
}

// Returns a ghost handle to the argument to the currently executing
// command
static naRef f_cmdarg(naContext c, naRef args)
{
    FGNasalSys* nasal = (FGNasalSys*)globals->get_subsystem("nasal");
    return nasal->cmdArgGhost();
}

// Sets up a property interpolation.  The first argument is either a
// ghost (SGPropertyNode_ptr*) or a string (global property path) to
// interpolate.  The second argument is a vector of pairs of
// value/delta numbers.
static naRef f_interpolate(naContext c, naRef args)
{
    SGPropertyNode* node;
    naRef prop = naVec_get(args, 0);
    if(naIsString(prop)) node = fgGetNode(naStr_data(prop), true);
    else if(naIsGhost(prop)) node = *(SGPropertyNode_ptr*)naGhost_ptr(prop);
    else return naNil();

    naRef curve = naVec_get(args, 1);
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

static naRef f_rand(naContext c, naRef args)
{
    return naNum(sg_random());
}

// Table of extension functions.  Terminate with zeros.
static struct { char* name; naCFunction func; } funcs[] = {
    { "getprop",   f_getprop },
    { "setprop",   f_setprop },
    { "print",     f_print },
    { "_fgcommand", f_fgcommand },
    { "settimer",  f_settimer },
    { "_cmdarg",  f_cmdarg },
    { "_interpolate",  f_interpolate },
    { "rand",  f_rand },
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
    _globals = naStdLib(_context);
    naSave(_context, _globals);
    hashset(_globals, "globals", _globals);

    // Add in the math library under "math"
    hashset(_globals, "math", naMathLib(_context));

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
        readScriptFile(fullpath, file.base().c_str());
    }

    // Pull scripts out of the property tree, too
    loadPropertyScripts();
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

        const char* file = n->getStringValue("file");
        if(!n->hasChild("file")) file = 0; // Hrm...
        if(file) {
            SGPath p(globals->get_fg_root());
            p.append(file);
            readScriptFile(p, module);
        }
        
        const char* src = n->getStringValue("script");
        if(!n->hasChild("script")) src = 0; // Hrm...
        if(src)
            initModule(module, n->getPath(), src, strlen(src));

        if(!file && !src)
            SG_LOG(SG_NASAL, SG_ALERT, "Nasal error: " <<
                   "no <file> or <script> defined in " <<
                   "/nasal/" << module);
    }
}

// Logs a runtime error, with stack trace, to the FlightGear log stream
void FGNasalSys::logError()
{
    SG_LOG(SG_NASAL, SG_ALERT,
           "Nasal runtime error: " << naGetError(_context));
    SG_LOG(SG_NASAL, SG_ALERT,
           "  at " << naStr_data(naGetSourceFile(_context, 0)) <<
           ", line " << naGetLine(_context, 0));
    for(int i=1; i<naStackDepth(_context); i++)
        SG_LOG(SG_NASAL, SG_ALERT,
               "  called from: " << naStr_data(naGetSourceFile(_context, i)) <<
               ", line " << naGetLine(_context, i));
}

// Reads a script file, executes it, and places the resulting
// namespace into the global namespace under the specified module
// name.
void FGNasalSys::readScriptFile(SGPath file, const char* module)
{
    int len = 0;
    char* buf = readfile(file.c_str(), &len);
    if(!buf) {
        SG_LOG(SG_NASAL, SG_ALERT,
               "Nasal error: could not read script file " << file.c_str()
               << " into module " << module);
        return;
    }

    initModule(module, file.c_str(), buf, len);
    delete[] buf;
}

// Parse and run.  Save the local variables namespace, as it will
// become a sub-object of globals.
void FGNasalSys::initModule(const char* moduleName, const char* fileName,
                            const char* src, int len)
{
    if(len == 0) len = strlen(src);

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

    naCall(_context, code, naNil(), naNil(), locals);
    if(naGetError(_context)) {
        logError();
        return;
    }
    hashset(_globals, moduleName, locals);
}

naRef FGNasalSys::parse(const char* filename, const char* buf, int len)
{
    if(len == 0) len = strlen(buf);
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
    // Parse the Nasal source.  I'd love to use the property name of
    // the argument, but it's actually a *clone* of the original
    // location in the property tree.  arg->getPath() returns an empty
    // string.
    const char* nasal = arg->getStringValue("script");
    naRef code = parse("<command>", nasal);
    if(naIsNil(code)) return false;
    
    // Cache the command argument for inspection via cmdarg().  For
    // performance reasons, we won't bother with it if the invoked
    // code doesn't need it.
    _cmdArg = (SGPropertyNode*)arg;

    // Call it!
    naRef result = naCall(_context, code, naNil(), naNil(), naNil());
    if(!naGetError(_context)) return true;
    logError();
    return false;
}

// settimer(func, dt, simtime) extension function.  The first argument
// is a Nasal function to call, the second is a delta time (from now),
// in seconds.  The third, if present, is a boolean value indicating
// that "simulator" time (rather than real time) is to be used.
//
// Implementation note: the FGTimer objects don't live inside the
// garbage collector, so the Nasal handler functions have to be
// "saved" somehow lest they be inadvertently cleaned.  In this case,
// they are inserted into a globals.__gcsave hash and removed on
// expiration.
void FGNasalSys::setTimer(naRef args)
{
    // Extract the handler, delta, and simtime arguments:
    naRef handler = naVec_get(args, 0);
    if(!(naIsCode(handler) || naIsCCode(handler) || naIsFunc(handler)))
        return;

    naRef delta = naNumValue(naVec_get(args, 1));
    if(naIsNil(delta)) return;
    
    bool simtime = naTrue(naVec_get(args, 2)) ? true : false;

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
    naCall(_context, t->handler, naNil(), naNil(), naNil());
    if(naGetError(_context))
        logError();
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
