
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // gettimeofday
#endif

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

#include <simgear/nasal/nasal.h>
#include <simgear/props/props.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/SimpleMarkdown.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/debug/BufferedLogCallback.hxx>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>

#include "NasalSGPath.hxx"
#include "NasalSys.hxx"
#include "NasalSys_private.hxx"
#include "NasalAircraft.hxx"
#include "NasalModelData.hxx"
#include "NasalPositioned.hxx"
#include "NasalCanvas.hxx"
#include "NasalClipboard.hxx"
#include "NasalCondition.hxx"
#include "NasalHTTP.hxx"
#include "NasalString.hxx"

#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Main/fg_props.hxx>

using std::map;
using std::string;
using std::vector;

void postinitNasalGUI(naRef globals, naContext c);

static FGNasalSys* nasalSys = 0;

// Listener class for loading Nasal modules on demand
class FGNasalModuleListener : public SGPropertyChangeListener
{
public:
    FGNasalModuleListener(SGPropertyNode* node);

    virtual void valueChanged(SGPropertyNode* node);

private:
    SGPropertyNode_ptr _node;
};

FGNasalModuleListener::FGNasalModuleListener(SGPropertyNode* node) : _node(node)
{
}

void FGNasalModuleListener::valueChanged(SGPropertyNode*)
{
    if (_node->getBoolValue("enabled",false)&&
        !_node->getBoolValue("loaded",true))
    {
        nasalSys->loadPropertyScripts(_node);
    }
}

//////////////////////////////////////////////////////////////////////////


class TimerObj : public SGReferenced
{
public:
  TimerObj(FGNasalSys* sys, naRef f, naRef self, double interval) :
    _sys(sys),
    _func(f),
    _self(self),
    _isRunning(false),
    _interval(interval),
    _singleShot(false)
  {
    char nm[128];
    snprintf(nm, 128, "nasal-timer-%p", this);
    _name = nm;
    _gcRoot =  sys->gcSave(f);
    _gcSelf = sys->gcSave(self);
  }
  
  virtual ~TimerObj()
  {
    stop();
    _sys->gcRelease(_gcRoot);
    _sys->gcRelease(_gcSelf);
  }
  
  bool isRunning() const { return _isRunning; }
    
  void stop()
  {
    if (_isRunning) {
      globals->get_event_mgr()->removeTask(_name);
      _isRunning = false;
    }
  }
  
  void start()
  {
    if (_isRunning) {
      return;
    }
    
    _isRunning = true;
    if (_singleShot) {
      globals->get_event_mgr()->addEvent(_name, this, &TimerObj::invoke, _interval);
    } else {
      globals->get_event_mgr()->addTask(_name, this, &TimerObj::invoke,
                                        _interval, _interval /* delay */);
    }
  }
  
  // stop and then start -
  void restart(double newInterval)
  {
    _interval = newInterval;
    stop();
    start();
  }
  
  void invoke()
  {
    if( _singleShot )
      // Callback may restart the timer, so update status before callback is
      // called (Prevent warnings of deleting not existing tasks from the
      // event manager).
      _isRunning = false;

    naRef *args = NULL;
    _sys->callMethod(_func, _self, 0, args, naNil() /* locals */);
  }
  
  void setSingleShot(bool aSingleShot)
  {
    _singleShot = aSingleShot;
  }
  
  bool isSingleShot() const
  { return _singleShot; }
private:
  std::string _name;
  FGNasalSys* _sys;
  naRef _func, _self;
  int _gcRoot, _gcSelf;
  bool _isRunning;
  double _interval;
  bool _singleShot;
};

typedef SGSharedPtr<TimerObj> TimerObjRef;
typedef nasal::Ghost<TimerObjRef> NasalTimerObj;

///////////////////////////////////////////////////////////////////////////

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

FGNasalSys::FGNasalSys() :
    _inited(false)
{
    nasalSys = this;
    _context = 0;
    _globals = naNil();
    _string = naNil();
    _wrappedNodeFunc = naNil();
    
    _log = new simgear::BufferedLogCallback(SG_NASAL, SG_INFO);
    _log->truncateAt(255);
    sglog().addCallback(_log);

    naSetErrorHandler(&logError);
}

// Utility.  Sets a named key in a hash by C string, rather than nasal
// string object.
void FGNasalSys::hashset(naRef hash, const char* key, naRef val)
{
    naRef s = naNewString(_context);
    naStr_fromdata(s, (char*)key, strlen(key));
    naHash_set(hash, s, val);
}

void FGNasalSys::globalsSet(const char* key, naRef val)
{
  hashset(_globals, key, val);
}

naRef FGNasalSys::call(naRef code, int argc, naRef* args, naRef locals)
{
  return callMethod(code, naNil(), argc, args, locals);
}

naRef FGNasalSys::callWithContext(naContext ctx, naRef code, int argc, naRef* args, naRef locals)
{
  return callMethodWithContext(ctx, code, naNil(), argc, args, locals);
}

// Does a naCall() in a new context.  Wrapped here to make lock
// tracking easier.  Extension functions are called with the lock, but
// we have to release it before making a new naCall().  So rather than
// drop the lock in every extension function that might call back into
// Nasal, we keep a stack depth counter here and only unlock/lock
// around the naCall if it isn't the first one.

naRef FGNasalSys::callMethod(naRef code, naRef self, int argc, naRef* args, naRef locals)
{
  return naCallMethod(code, self, argc, args, locals);
}

naRef FGNasalSys::callMethodWithContext(naContext ctx, naRef code, naRef self, int argc, naRef* args, naRef locals)
{
  return naCallMethodCtx(ctx, code, self, argc, args, locals);
}

FGNasalSys::~FGNasalSys()
{
    if (_inited) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Nasal was not shutdown");
    }
    nasalSys = 0;
}

bool FGNasalSys::parseAndRun(const char* sourceCode)
{
    naContext ctx = naNewContext();
    naRef code = parse(ctx, "FGNasalSys::parseAndRun()", sourceCode,
                       strlen(sourceCode));
    if(naIsNil(code)) {
        naFreeContext(ctx);
        return false;
    }
    callWithContext(ctx, code, 0, 0, naNil());
    naFreeContext(ctx);
    return true;
}

#if 0
FGNasalScript* FGNasalSys::parseScript(const char* src, const char* name)
{
    FGNasalScript* script = new FGNasalScript();
    script->_gcKey = -1; // important, if we delete it on a parse
    script->_nas = this; // error, don't clobber a real handle!

    char buf[256];
    if(!name) {
        sprintf(buf, "FGNasalScript@%p", (void *)script);
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
#endif

// The get/setprop functions accept a *list* of strings and walk
// through the property tree with them to find the appropriate node.
// This allows a Nasal object to hold onto a property path and use it
// like a node object, e.g. setprop(ObjRoot, "size-parsecs", 2.02).  This
// is the utility function that walks the property tree.
static SGPropertyNode* findnode(naContext c, naRef* vec, int len, bool create=false)
{
    SGPropertyNode* p = globals->get_props();
    try {
        for(int i=0; i<len; i++) {
            naRef a = vec[i];
            if(!naIsString(a)) {
                naRuntimeError(c, "bad argument to setprop/getprop path: expected a string");
            }
            naRef b = i < len-1 ? naNumValue(vec[i+1]) : naNil();
            if (!naIsNil(b)) {
                p = p->getNode(naStr_data(a), (int)b.num, create);
                i++;
            } else {
                p = p->getNode(naStr_data(a), create);
            }
            if(p == 0) return 0;
        }
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
    }
    return p;
}

// getprop() extension function.  Concatenates its string arguments as
// property names and returns the value of the specified property.  Or
// nil if it doesn't exist.
static naRef f_getprop(naContext c, naRef me, int argc, naRef* args)
{
    using namespace simgear;
    if (argc < 1) {
        naRuntimeError(c, "getprop() expects at least 1 argument");
    }
    const SGPropertyNode* p = findnode(c, args, argc, false);
    if(!p) return naNil();

    switch(p->getType()) {
    case props::BOOL:   case props::INT:
    case props::LONG:   case props::FLOAT:
    case props::DOUBLE:
        {
        double dv = p->getDoubleValue();
        if (SGMisc<double>::isNaN(dv)) {
          SG_LOG(SG_NASAL, SG_ALERT, "Nasal getprop: property " << p->getPath() << " is NaN");
          return naNil();
        }
        
        return naNum(dv);
        }
        
    case props::STRING:
    case props::UNSPECIFIED:
        {
            naRef nastr = naNewString(c);
            const char* val = p->getStringValue();
            naStr_fromdata(nastr, (char*)val, strlen(val));
            return nastr;
        }
    case props::ALIAS: // <--- FIXME, recurse?
    default:
        return naNil();
    }
}

// setprop() extension function.  Concatenates its string arguments as
// property names and sets the value of the specified property to the
// final argument.
static naRef f_setprop(naContext c, naRef me, int argc, naRef* args)
{
    if (argc < 2) {
        naRuntimeError(c, "setprop() expects at least 2 arguments");
    }
    naRef val = args[argc - 1];
    SGPropertyNode* p = findnode(c, args, argc-1, true);

    bool result = false;
    try {
        if(naIsString(val)) result = p->setStringValue(naStr_data(val));
        else {
            if(!naIsNum(val))
                naRuntimeError(c, "setprop() value is not string or number");
                
            if (SGMisc<double>::isNaN(val.num)) {
                naRuntimeError(c, "setprop() passed a NaN");
            }
            
            result = p->setDoubleValue(val.num);
        }
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
    }
    return naNum(result);
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
    SG_LOG(SG_NASAL, SG_ALERT, buf);
    return naNum(buf.length());
}

// logprint() extension function.  Same as above, all arguments after the
// first argument are concatenated. Argument 0 is the log-level, matching
// sgDebugPriority.
static naRef f_logprint(naContext c, naRef me, int argc, naRef* args)
{
  if (argc < 1)
    naRuntimeError(c, "no prioirty argument to logprint()");
  
  naRef priority = args[0];
  string buf;
  int n = argc;
  for(int i=1; i<n; i++) {
    naRef s = naStringValue(c, args[i]);
    if(naIsNil(s)) continue;
    buf += naStr_data(s);
  }
// use the nasal source file and line for the message location, since
// that's more useful than the location here!
  sglog().log(SG_NASAL, (sgDebugPriority)(int) priority.num,
               naStr_data(naGetSourceFile(c, 0)),
               naGetLine(c, 0), buf);
  return naNum(buf.length());
}


// fgcommand() extension function.  Executes a named command via the
// FlightGear command manager.  Takes a single property node name as
// an argument.
static naRef f_fgcommand(naContext c, naRef me, int argc, naRef* args)
{
    naRef cmd = argc > 0 ? args[0] : naNil();
    naRef props = argc > 1 ? args[1] : naNil();
    if(!naIsString(cmd) || (!naIsNil(props) && !naIsGhost(props)))
        naRuntimeError(c, "bad arguments to fgcommand()");
    SGPropertyNode_ptr node;
    if(!naIsNil(props))
        node = static_cast<SGPropertyNode*>(naGhost_ptr(props));
    else
        node = new SGPropertyNode;

    return naNum(globals->get_commands()->execute(naStr_data(cmd), node));
}

// settimer(func, dt, simtime) extension function.  Falls through to
// FGNasalSys::setTimer().  See there for docs.
static naRef f_settimer(naContext c, naRef me, int argc, naRef* args)
{
    nasalSys->setTimer(c, argc, args);
    return naNil();
}

static naRef f_makeTimer(naContext c, naRef me, int argc, naRef* args)
{
  if (!naIsNum(args[0])) {
    naRuntimeError(c, "bad interval argument to maketimer");
  }
    
  naRef func, self = naNil();
  if (naIsFunc(args[1])) {
    func = args[1];
  } else if ((argc == 3) && naIsFunc(args[2])) {
    self = args[1];
    func = args[2];
  }
  
  TimerObj* timerObj = new TimerObj(nasalSys, func, self, args[0].num);
  return nasal::to_nasal(c, timerObj);
}

// setlistener(func, property, bool) extension function.  Falls through to
// FGNasalSys::setListener().  See there for docs.
static naRef f_setlistener(naContext c, naRef me, int argc, naRef* args)
{
    return nasalSys->setListener(c, argc, args);
}

// removelistener(int) extension function. Falls through to
// FGNasalSys::removeListener(). See there for docs.
static naRef f_removelistener(naContext c, naRef me, int argc, naRef* args)
{
    return nasalSys->removeListener(c, argc, args);
}

// Returns a ghost handle to the argument to the currently executing
// command
static naRef f_cmdarg(naContext c, naRef me, int argc, naRef* args)
{
    return nasalSys->cmdArgGhost();
}

// Sets up a property interpolation.  The first argument is either a
// ghost (SGPropertyNode*) or a string (global property path) to
// interpolate.  The second argument is a vector of pairs of
// value/delta numbers.
static naRef f_interpolate(naContext c, naRef me, int argc, naRef* args)
{
  SGPropertyNode* node;
  naRef prop = argc > 0 ? args[0] : naNil();
  if(naIsString(prop)) node = fgGetNode(naStr_data(prop), true);
  else if(naIsGhost(prop)) node = static_cast<SGPropertyNode*>(naGhost_ptr(prop));
  else return naNil();

  naRef curve = argc > 1 ? args[1] : naNil();
  if(!naIsVector(curve)) return naNil();
  int nPoints = naVec_size(curve) / 2;

  simgear::PropertyList value_nodes;
  value_nodes.reserve(nPoints);
  double_list deltas;
  deltas.reserve(nPoints);

  for( int i = 0; i < nPoints; ++i )
  {
    SGPropertyNode* val = new SGPropertyNode;
    val->setDoubleValue(naNumValue(naVec_get(curve, 2*i)).num);
    value_nodes.push_back(val);
    deltas.push_back(naNumValue(naVec_get(curve, 2*i+1)).num);
  }

  node->interpolate("numeric", value_nodes, deltas, "linear");
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

static naRef f_abort(naContext c, naRef me, int argc, naRef* args)
{
    abort();
    return naNil();
}

// Return an array listing of all files in a directory
static naRef f_directory(naContext c, naRef me, int argc, naRef* args)
{
    if(argc != 1 || !naIsString(args[0]))
        naRuntimeError(c, "bad arguments to directory()");
    
    simgear::Dir d(SGPath(naStr_data(args[0])));
    if(!d.exists()) return naNil();
    naRef result = naNewVector(c);

    simgear::PathList paths = d.children(simgear::Dir::TYPE_FILE | simgear::Dir::TYPE_DIR);
    for (unsigned int i=0; i<paths.size(); ++i) {
      std::string p = paths[i].file();
      naVec_append(result, naStr_fromdata(naNewString(c), p.c_str(), p.size()));
    }
    
    return result;
}

/**
 * Given a data path, resolve it in FG_ROOT or an FG_AIRCRFT directory
 */
static naRef f_resolveDataPath(naContext c, naRef me, int argc, naRef* args)
{
    if(argc != 1 || !naIsString(args[0]))
        naRuntimeError(c, "bad arguments to resolveDataPath()");

    SGPath p = globals->resolve_maybe_aircraft_path(naStr_data(args[0]));
    const char* pdata = p.c_str();
    return naStr_fromdata(naNewString(c), const_cast<char*>(pdata), strlen(pdata));
}

static naRef f_findDataDir(naContext c, naRef me, int argc, naRef* args)
{
    if(argc != 1 || !naIsString(args[0]))
        naRuntimeError(c, "bad arguments to findDataDir()");
    
    SGPath p = globals->find_data_dir(naStr_data(args[0]));
    const char* pdata = p.c_str();
    return naStr_fromdata(naNewString(c), const_cast<char*>(pdata), strlen(pdata));
}

class NasalCommand : public SGCommandMgr::Command
{
public:
    NasalCommand(FGNasalSys* sys, naRef f, const std::string& name) :
        _sys(sys),
        _func(f),
        _name(name)
    {
        globals->get_commands()->addCommandObject(_name, this);
        _gcRoot =  sys->gcSave(f);
    }
    
    virtual ~NasalCommand()
    {
        _sys->gcRelease(_gcRoot);
    }
    
    virtual bool operator()(const SGPropertyNode* aNode)
    {
        _sys->setCmdArg(const_cast<SGPropertyNode*>(aNode));
        naRef args[1];
        args[0] = _sys->wrappedPropsNode(const_cast<SGPropertyNode*>(aNode));
    
        _sys->callMethod(_func, naNil(), 1, args, naNil() /* locals */);

        return true;
    }
    
private:
    FGNasalSys* _sys;
    naRef _func;
    int _gcRoot;
    std::string _name;
};

static naRef f_addCommand(naContext c, naRef me, int argc, naRef* args)
{
    if(argc != 2 || !naIsString(args[0]) || !naIsFunc(args[1]))
        naRuntimeError(c, "bad arguments to addcommand()");
    
    nasalSys->addCommand(args[1], naStr_data(args[0]));
    return naNil();
}

static naRef f_removeCommand(naContext c, naRef me, int argc, naRef* args)
{
    if ((argc < 1) || !naIsString(args[0]))
        naRuntimeError(c, "bad argument to removecommand()");
    
    globals->get_commands()->removeCommand(naStr_data(args[0]));
    return naNil();
}

// Parse XML file.
//     parsexml(<path> [, <start-tag> [, <end-tag> [, <data> [, <pi>]]]]);
//
// <path>      ... absolute path to an XML file
// <start-tag> ... callback function with two args: tag name, attribute hash
// <end-tag>   ... callback function with one arg:  tag name
// <data>      ... callback function with one arg:  data
// <pi>        ... callback function with two args: target, data
//                 (pi = "processing instruction")
// All four callback functions are optional and default to nil.
// The function returns nil on error, or the validated file name otherwise.
static naRef f_parsexml(naContext c, naRef me, int argc, naRef* args)
{
    if(argc < 1 || !naIsString(args[0]))
        naRuntimeError(c, "parsexml(): path argument missing or not a string");
    if(argc > 5) argc = 5;
    for(int i=1; i<argc; i++)
        if(!(naIsNil(args[i]) || naIsFunc(args[i])))
            naRuntimeError(c, "parsexml(): callback argument not a function");

    const char* file = fgValidatePath(naStr_data(args[0]), false);
    if(!file) {
        naRuntimeError(c, "parsexml(): reading '%s' denied "
                "(unauthorized access)", naStr_data(args[0]));
        return naNil();
    }
    std::ifstream input(file);
    NasalXMLVisitor visitor(c, argc, args);
    try {
        readXML(input, visitor);
    } catch (const sg_exception& e) {
        naRuntimeError(c, "parsexml(): file '%s' %s",
                file, e.getFormattedMessage().c_str());
        return naNil();
    }
    return naStr_fromdata(naNewString(c), const_cast<char*>(file), strlen(file));
}

/**
 * Parse very simple and small subset of markdown
 *
 * parse_markdown(src)
 */
static naRef f_parse_markdown(naContext c, naRef me, int argc, naRef* args)
{
  nasal::CallContext ctx(c, me, argc, args);
  return ctx.to_nasal(
    simgear::SimpleMarkdown::parse(ctx.requireArg<std::string>(0))
  );
}

/**
 * Create md5 hash from given string
 *
 * md5(str)
 */
static naRef f_md5(naContext c, naRef me, int argc, naRef* args)
{
  if( argc != 1 || !naIsString(args[0]) )
    naRuntimeError(c, "md5(): wrong type or number of arguments");

  return nasal::to_nasal(
    c,
    simgear::strutils::md5(naStr_data(args[0]), naStr_len(args[0]))
  );
}

// Return UNIX epoch time in seconds.
static naRef f_systime(naContext c, naRef me, int argc, naRef* args)
{
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    double t = (4294967296.0 * ft.dwHighDateTime + ft.dwLowDateTime);
    // Converts from 100ns units in 1601 epoch to unix epoch in sec
    return naNum((t * 1e-7) - 11644473600.0);
#else
    struct timeval td;
    gettimeofday(&td, 0);
    return naNum(td.tv_sec + 1e-6 * td.tv_usec);
#endif
}

// Table of extension functions.  Terminate with zeros.
static struct { const char* name; naCFunction func; } funcs[] = {
    { "getprop",   f_getprop },
    { "setprop",   f_setprop },
    { "print",     f_print },
    { "logprint",  f_logprint },
    { "_fgcommand", f_fgcommand },
    { "settimer",  f_settimer },
    { "maketimer", f_makeTimer },
    { "_setlistener", f_setlistener },
    { "removelistener", f_removelistener },
    { "addcommand", f_addCommand },
    { "removecommand", f_removeCommand },
    { "_cmdarg",  f_cmdarg },
    { "_interpolate",  f_interpolate },
    { "rand",  f_rand },
    { "srand",  f_srand },
    { "abort", f_abort },
    { "directory", f_directory },
    { "resolvepath", f_resolveDataPath },
    { "finddata", f_findDataDir },
    { "parsexml", f_parsexml },
    { "parse_markdown", f_parse_markdown },
    { "md5", f_md5 },
    { "systime", f_systime },
    { 0, 0 }
};

naRef FGNasalSys::cmdArgGhost()
{
    return propNodeGhost(_cmdArg);
}

void FGNasalSys::setCmdArg(SGPropertyNode* aNode)
{
    _cmdArg = aNode;
}

void FGNasalSys::init()
{
    if (_inited) {
        SG_LOG(SG_GENERAL, SG_ALERT, "duplicate init of Nasal");
    }
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

    // Add string methods
    _string = naInit_string(_context);
    naSave(_context, _string);
    initNasalString(_globals, _string, _context);

    initNasalPositioned(_globals, _context);
    initNasalPositioned_cppbind(_globals, _context);
    initNasalAircraft(_globals, _context);
    NasalClipboard::init(this);
    initNasalCanvas(_globals, _context);
    initNasalCondition(_globals, _context);
    initNasalHTTP(_globals, _context);
    initNasalSGPath(_globals, _context);
  
    NasalTimerObj::init("Timer")
      .method("start", &TimerObj::start)
      .method("stop", &TimerObj::stop)
      .method("restart", &TimerObj::restart)
      .member("singleShot", &TimerObj::isSingleShot, &TimerObj::setSingleShot)
      .member("isRunning", &TimerObj::isRunning);

    // Now load the various source files in the Nasal directory
    simgear::Dir nasalDir(SGPath(globals->get_fg_root(), "Nasal"));
    loadScriptDirectory(nasalDir);

    // Add modules in Nasal subdirectories to property tree
    simgear::PathList directories = nasalDir.children(simgear::Dir::TYPE_DIR+
            simgear::Dir::NO_DOT_OR_DOTDOT, "");
    for (unsigned int i=0; i<directories.size(); ++i) {
        simgear::Dir dir(directories[i]);
        simgear::PathList scripts = dir.children(simgear::Dir::TYPE_FILE, ".nas");
        addModule(directories[i].file(), scripts);
    }

    // set signal and remove node to avoid restoring at reinit
    const char *s = "nasal-dir-initialized";
    SGPropertyNode *signal = fgGetNode("/sim/signals", true);
    signal->setBoolValue(s, true);
    signal->removeChildren(s);

    if( !checkIOrules() )
    {
      SG_LOG(SG_NASAL, SG_ALERT, "Required IOrules check failed.");
      exit(-1);
    }

    // Pull scripts out of the property tree, too
    loadPropertyScripts();
  
    // now Nasal modules are loaded, we can do some delayed work
    postinitNasalPositioned(_globals, _context);
    postinitNasalGUI(_globals, _context);
    
    _inited = true;
}

void FGNasalSys::shutdown()
{
    if (!_inited) {
        return;
    }
    
    shutdownNasalPositioned();
    
    map<int, FGNasalListener *>::iterator it, end = _listener.end();
    for(it = _listener.begin(); it != end; ++it)
        delete it->second;
    _listener.clear();
    
    NasalCommandDict::iterator j = _commands.begin();
    for (; j != _commands.end(); ++j) {
        globals->get_commands()->removeCommand(j->first);
    }
    _commands.clear();
    
    std::vector<FGNasalModuleListener*>::iterator k = _moduleListeners.begin();
    for(; k!= _moduleListeners.end(); ++k)
        delete *k;
    _moduleListeners.clear();
    
    naClearSaved();
    
    _string = naNil(); // will be freed by _context
    naFreeContext(_context);
   
    //setWatchedRef(_globals);
    
    // remove the recursive reference in globals
    hashset(_globals, "globals", naNil());
    _globals = naNil();    
    
    naGC();
    _inited = false;
}

naRef FGNasalSys::wrappedPropsNode(SGPropertyNode* aProps)
{
    if (naIsNil(_wrappedNodeFunc)) {
        nasal::Hash props = getGlobals().get<nasal::Hash>("props");
        _wrappedNodeFunc = props.get("wrapNode");
    }
    
    naRef args[1];
    args[0] = propNodeGhost(aProps);
    naContext ctx = naNewContext();
    naRef wrapped = naCallMethodCtx(ctx, _wrappedNodeFunc, naNil(), 1, args, naNil());
    naFreeContext(ctx);
    return wrapped;
}

void FGNasalSys::update(double)
{
    if( NasalClipboard::getInstance() )
        NasalClipboard::getInstance()->update();

    if(!_dead_listener.empty()) {
        vector<FGNasalListener *>::iterator it, end = _dead_listener.end();
        for(it = _dead_listener.begin(); it != end; ++it) delete *it;
        _dead_listener.clear();
    }

    if (!_loadList.empty())
    {
        if( _delay_load )
          _delay_load = false;
        else
          // process Nasal load hook (only one per update loop to avoid excessive lags)
          _loadList.pop()->load();
    }
    else
    if (!_unloadList.empty())
    {
        // process pending Nasal unload hooks after _all_ load hooks were processed
        // (only unload one per update loop to avoid excessive lags)
        _unloadList.pop()->unload();
    }

    // Destroy all queued ghosts
    nasal::ghostProcessDestroyList();

    // The global context is a legacy thing.  We use dynamically
    // created contexts for naCall() now, so that we can call them
    // recursively.  But there are still spots that want to use it for
    // naNew*() calls, which end up leaking memory because the context
    // only clears out its temporary vector when it's *used*.  So just
    // junk it and fetch a new/reinitialized one every frame.  This is
    // clumsy: the right solution would use the dynamic context in all
    // cases and eliminate _context entirely.  But that's more work,
    // and this works fine (yes, they say "New" and "Free", but
    // they're very fast, just trust me). -Andy
    naFreeContext(_context);
    _context = naNewContext();
}

bool pathSortPredicate(const SGPath& p1, const SGPath& p2)
{
  return p1.file() < p2.file();
}

// Loads all scripts in given directory 
void FGNasalSys::loadScriptDirectory(simgear::Dir nasalDir)
{
    simgear::PathList scripts = nasalDir.children(simgear::Dir::TYPE_FILE, ".nas");
    // Note: simgear::Dir already reports file entries in a deterministic order,
    // so a fixed loading sequence is guaranteed (same for every user)
    for (unsigned int i=0; i<scripts.size(); ++i) {
      SGPath fullpath(scripts[i]);
      SGPath file = fullpath.file();
      loadModule(fullpath, file.base().c_str());
    }
}

// Create module with list of scripts
void FGNasalSys::addModule(string moduleName, simgear::PathList scripts)
{
    if (! scripts.empty())
    {
        SGPropertyNode* nasal = globals->get_props()->getNode("nasal");
        SGPropertyNode* module_node = nasal->getChild(moduleName,0,true);
        for (unsigned int i=0; i<scripts.size(); ++i) {
            SGPropertyNode* pFileNode = module_node->getChild("file",i,true);
            pFileNode->setStringValue(scripts[i].c_str());
        }
        if (!module_node->hasChild("enabled",0))
        {
            SGPropertyNode* node = module_node->getChild("enabled",0,true);
            node->setBoolValue(true);
            node->setAttribute(SGPropertyNode::USERARCHIVE,true);
        }
    }
}

// Loads the scripts found under /nasal in the global tree
void FGNasalSys::loadPropertyScripts()
{
    SGPropertyNode* nasal = globals->get_props()->getNode("nasal");
    if(!nasal) return;

    for(int i=0; i<nasal->nChildren(); i++)
    {
        SGPropertyNode* n = nasal->getChild(i);
        loadPropertyScripts(n);
    }
}

// Loads the scripts found under /nasal in the global tree
void FGNasalSys::loadPropertyScripts(SGPropertyNode* n)
{
    bool is_loaded = false;

    const char* module = n->getName();
    if(n->hasChild("module"))
        module = n->getStringValue("module");
    if (n->getBoolValue("enabled",true))
    {
        // allow multiple files to be specified within a single
        // Nasal module tag
        int j = 0;
        SGPropertyNode *fn;
        bool file_specified = false;
        bool ok=true;
        while((fn = n->getChild("file", j)) != NULL) {
            file_specified = true;
            const char* file = fn->getStringValue();
            SGPath p(file);
            if (!p.isAbsolute() || !p.exists())
            {
                p = globals->resolve_maybe_aircraft_path(file);
                if (p.isNull())
                {
                    SG_LOG(SG_NASAL, SG_ALERT, "Cannot find Nasal script '" <<
                            file << "' for module '" << module << "'.");
                }
            }
            ok &= p.isNull() ? false : loadModule(p, module);
            j++;
        }

        const char* src = n->getStringValue("script");
        if(!n->hasChild("script")) src = 0; // Hrm...
        if(src)
            createModule(module, n->getPath().c_str(), src, strlen(src));

        if(!file_specified && !src)
        {
            // module no longer exists - clear the archived "enable" flag
            n->setAttribute(SGPropertyNode::USERARCHIVE,false);
            SGPropertyNode* node = n->getChild("enabled",0,false);
            if (node)
                node->setAttribute(SGPropertyNode::USERARCHIVE,false);

            SG_LOG(SG_NASAL, SG_ALERT, "Nasal error: " <<
                    "no <file> or <script> defined in " <<
                    "/nasal/" << module);
        }
        else
            is_loaded = ok;
    }
    else
    {
        SGPropertyNode* enable = n->getChild("enabled");
        if (enable)
        {
            FGNasalModuleListener* listener = new FGNasalModuleListener(n);
            _moduleListeners.push_back(listener);
            enable->addChangeListener(listener, false);
        }
    }
    SGPropertyNode* loaded = n->getChild("loaded",0,true);
    loaded->setAttribute(SGPropertyNode::PRESERVE,true);
    loaded->setBoolValue(is_loaded);
}

// Logs a runtime error, with stack trace, to the FlightGear log stream
void FGNasalSys::logError(naContext context)
{
    SG_LOG(SG_NASAL, SG_ALERT, "Nasal runtime error: " << naGetError(context));
    int stack_depth = naStackDepth(context);
    if( stack_depth < 1 )
      return;
    SG_LOG(SG_NASAL, SG_ALERT,
           "  at " << naStr_data(naGetSourceFile(context, 0)) <<
           ", line " << naGetLine(context, 0));
    for(int i=1; i<stack_depth; i++)
        SG_LOG(SG_NASAL, SG_ALERT,
               "  called from: " << naStr_data(naGetSourceFile(context, i)) <<
               ", line " << naGetLine(context, i));
}

// Reads a script file, executes it, and places the resulting
// namespace into the global namespace under the specified module
// name.
bool FGNasalSys::loadModule(SGPath file, const char* module)
{
    int len = 0;
    char* buf = readfile(file.c_str(), &len);
    if(!buf) {
        SG_LOG(SG_NASAL, SG_ALERT,
               "Nasal error: could not read script file " << file.c_str()
               << " into module " << module);
        return false;
    }

    bool ok = createModule(module, file.c_str(), buf, len);
    delete[] buf;
    return ok;
}

// Parse and run.  Save the local variables namespace, as it will
// become a sub-object of globals.  The optional "arg" argument can be
// used to pass an associated property node to the module, which can then
// be accessed via cmdarg().  (This is, for example, used by XML dialogs.)
bool FGNasalSys::createModule(const char* moduleName, const char* fileName,
                              const char* src, int len,
                              const SGPropertyNode* cmdarg,
                              int argc, naRef* args)
{
    naContext ctx = naNewContext();
    naRef code = parse(ctx, fileName, src, len);
    if(naIsNil(code)) {
        naFreeContext(ctx);
        return false;
    }

    
    // See if we already have a module hash to use.  This allows the
    // user to, for example, add functions to the built-in math
    // module.  Make a new one if necessary.
    naRef locals;
    naRef modname = naNewString(ctx);
    naStr_fromdata(modname, (char*)moduleName, strlen(moduleName));
    if(!naHash_get(_globals, modname, &locals))
        locals = naNewHash(ctx);

    _cmdArg = (SGPropertyNode*)cmdarg;

    callWithContext(ctx, code, argc, args, locals);
    hashset(_globals, moduleName, locals);
    
    naFreeContext(ctx);
    return true;
}

void FGNasalSys::deleteModule(const char* moduleName)
{
    if (!_inited) {
        // can occur on shutdown due to us being shutdown first, but other
        // subsystems having Nasal objects.
        return;
    }
    
    naContext ctx = naNewContext();
    naRef modname = naNewString(ctx);
    naStr_fromdata(modname, (char*)moduleName, strlen(moduleName));
    naHash_delete(_globals, modname);
    naFreeContext(ctx);
}

naRef FGNasalSys::parse(naContext ctx, const char* filename, const char* buf, int len)
{
    int errLine = -1;
    naRef srcfile = naNewString(ctx);
    naStr_fromdata(srcfile, (char*)filename, strlen(filename));
    naRef code = naParseCode(ctx, srcfile, 1, (char*)buf, len, &errLine);
    if(naIsNil(code)) {
        SG_LOG(SG_NASAL, SG_ALERT,
               "Nasal parse error: " << naGetError(ctx) <<
               " in "<< filename <<", line " << errLine);
        return naNil();
    }

    // Bind to the global namespace before returning
    return naBindFunction(ctx, code, _globals);
}

bool FGNasalSys::handleCommand( const char* moduleName,
                                const char* fileName,
                                const char* src,
                                const SGPropertyNode* arg )
{
    naContext ctx = naNewContext();
    naRef code = parse(ctx, fileName, src, strlen(src));
    if(naIsNil(code)) {
        naFreeContext(ctx);
        return false;
    }

    // Commands can be run "in" a module.  Make sure that module
    // exists, and set it up as the local variables hash for the
    // command.
    naRef locals = naNil();
    if(moduleName[0]) {
        naRef modname = naNewString(ctx);
        naStr_fromdata(modname, (char*)moduleName, strlen(moduleName));
        if(!naHash_get(_globals, modname, &locals)) {
            locals = naNewHash(ctx);
            naHash_set(_globals, modname, locals);
        }
    }

    // Cache this command's argument for inspection via cmdarg().  For
    // performance reasons, we won't bother with it if the invoked
    // code doesn't need it.
    _cmdArg = (SGPropertyNode*)arg;

    callWithContext(ctx, code, 0, 0, locals);
    naFreeContext(ctx);
    return true;
}

bool FGNasalSys::handleCommand(const SGPropertyNode* arg)
{
  const char* src = arg->getStringValue("script");
  const char* moduleName = arg->getStringValue("module");

  return handleCommand( moduleName,
                        arg->getPath(true).c_str(),
                        src,
                        arg );
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
void FGNasalSys::setTimer(naContext c, int argc, naRef* args)
{
    // Extract the handler, delta, and simtime arguments:
    naRef handler = argc > 0 ? args[0] : naNil();
    if(!(naIsCode(handler) || naIsCCode(handler) || naIsFunc(handler))) {
        naRuntimeError(c, "settimer() with invalid function argument");
        return;
    }

    naRef delta = argc > 1 ? args[1] : naNil();
    if(naIsNil(delta)) {
        naRuntimeError(c, "settimer() with invalid time argument");
        return;
    }

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
    call(t->handler, 0, 0, naNil());
    gcRelease(t->gcKey);
}

int FGNasalSys::gcSave(naRef r)
{
    return naGCSave(r);
}

void FGNasalSys::gcRelease(int key)
{
    naGCRelease(key);
}

//------------------------------------------------------------------------------
bool FGNasalSys::checkIOrules()
{
  // Ensure IOrules and path validation are working properly by trying to
  // access a folder/file which should never be accessible.
  const char* no_access_path =
#ifdef _WIN32
    "Z:"
#endif
    "/do-not-access";

  bool success = true;

  // write access
  if( fgValidatePath(no_access_path, true) )
  {
    success = false;
    SG_LOG
    (
      SG_GENERAL,
      SG_ALERT,
      "Check your IOrules! (write to '" << no_access_path << "' is allowed)"
    );
  }

  // read access
  if( fgValidatePath(no_access_path, false) )
  {
    success = false;
    SG_LOG
    (
      SG_GENERAL,
      SG_ALERT,
      "Check your IOrules! (read from '" << no_access_path << "' is allowed)"
    );
  }

  return success;
}

//------------------------------------------------------------------------------
void FGNasalSys::NasalTimer::timerExpired()
{
    nasal->handleTimer(this);
    delete this;
}

int FGNasalSys::_listenerId = 0;

// setlistener(<property>, <func> [, <initial=0> [, <persistent=1>]])
// Attaches a callback function to a property (specified as a global
// property path string or a SGPropertyNode* ghost). If the third,
// optional argument (default=0) is set to 1, then the function is also
// called initially. If the fourth, optional argument is set to 0, then the
// function is only called when the property node value actually changes.
// Otherwise it's called independent of the value whenever the node is
// written to (default). The setlistener() function returns a unique
// id number, which is to be used as argument to the removelistener()
// function.
naRef FGNasalSys::setListener(naContext c, int argc, naRef* args)
{
    SGPropertyNode_ptr node;
    naRef prop = argc > 0 ? args[0] : naNil();
    if(naIsString(prop)) node = fgGetNode(naStr_data(prop), true);
    else if(naIsGhost(prop)) node = static_cast<SGPropertyNode*>(naGhost_ptr(prop));
    else {
        naRuntimeError(c, "setlistener() with invalid property argument");
        return naNil();
    }

    if(node->isTied())
        SG_LOG(SG_NASAL, SG_DEBUG, "Attaching listener to tied property " <<
                node->getPath());

    naRef code = argc > 1 ? args[1] : naNil();
    if(!(naIsCode(code) || naIsCCode(code) || naIsFunc(code))) {
        naRuntimeError(c, "setlistener() with invalid function argument");
        return naNil();
    }

    int init = argc > 2 && naIsNum(args[2]) ? int(args[2].num) : 0;
    int type = argc > 3 && naIsNum(args[3]) ? int(args[3].num) : 1;
    FGNasalListener *nl = new FGNasalListener(node, code, this,
            gcSave(code), _listenerId, init, type);

    node->addChangeListener(nl, init != 0);

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

    it->second->_dead = true;
    _dead_listener.push_back(it->second);
    _listener.erase(it);
    return naNum(_listener.size());
}

void FGNasalSys::registerToLoad(FGNasalModelData *data)
{
  if( _loadList.empty() )
    _delay_load = true;
  _loadList.push(data);
}

void FGNasalSys::registerToUnload(FGNasalModelData *data)
{
    _unloadList.push(data);
}

void FGNasalSys::addCommand(naRef func, const std::string& name)
{
    if (_commands.find(name) != _commands.end()) {
        SG_LOG(SG_NASAL, SG_WARN, "duplicate add of command:" << name);
        return;
    }
    
    NasalCommand* cmd = new NasalCommand(this, func, name);
    _commands[name] = cmd;
}

void FGNasalSys::removeCommand(const std::string& name)
{
    NasalCommandDict::iterator it = _commands.find(name);
    if (it == _commands.end()) {
        SG_LOG(SG_NASAL, SG_WARN, "remove of unknwon command:" << name);
        return;
    }

    // will delete the NasalCommand instance
    globals->get_commands()->removeCommand(name);
    _commands.erase(it);
}

//////////////////////////////////////////////////////////////////////////
// FGNasalListener class.

FGNasalListener::FGNasalListener(SGPropertyNode *node, naRef code,
                                 FGNasalSys* nasal, int key, int id,
                                 int init, int type) :
    _node(node),
    _code(code),
    _gcKey(key),
    _id(id),
    _nas(nasal),
    _init(init),
    _type(type),
    _active(0),
    _dead(false),
    _last_int(0L),
    _last_float(0.0)
{
    if(_type == 0 && !_init)
        changed(node);
}

FGNasalListener::~FGNasalListener()
{
    _node->removeChangeListener(this);
    _nas->gcRelease(_gcKey);
}

void FGNasalListener::call(SGPropertyNode* which, naRef mode)
{
    if(_active || _dead) return;
    _active++;
    naRef arg[4];
    arg[0] = _nas->propNodeGhost(which);
    arg[1] = _nas->propNodeGhost(_node);
    arg[2] = mode;                  // value changed, child added/removed
    arg[3] = naNum(_node != which); // child event?
    _nas->call(_code, 4, arg, naNil());
    _active--;
}

void FGNasalListener::valueChanged(SGPropertyNode* node)
{
    if(_type < 2 && node != _node) return;   // skip child events
    if(_type > 0 || changed(_node) || _init)
        call(node, naNum(0));

    _init = 0;
}

void FGNasalListener::childAdded(SGPropertyNode*, SGPropertyNode* child)
{
    if(_type == 2) call(child, naNum(1));
}

void FGNasalListener::childRemoved(SGPropertyNode*, SGPropertyNode* child)
{
    if(_type == 2) call(child, naNum(-1));
}

bool FGNasalListener::changed(SGPropertyNode* node)
{
    using namespace simgear;
    props::Type type = node->getType();
    if(type == props::NONE) return false;
    if(type == props::UNSPECIFIED) return true;

    bool result;
    switch(type) {
    case props::BOOL:
    case props::INT:
    case props::LONG:
        {
            long l = node->getLongValue();
            result = l != _last_int;
            _last_int = l;
            return result;
        }
    case props::FLOAT:
    case props::DOUBLE:
        {
            double d = node->getDoubleValue();
            result = d != _last_float;
            _last_float = d;
            return result;
        }
    default:
        {
            string s = node->getStringValue();
            result = s != _last_string;
            _last_string = s;
            return result;
        }
    }
}

// NasalXMLVisitor class: handles EasyXML visitor callback for parsexml()
//
NasalXMLVisitor::NasalXMLVisitor(naContext c, int argc, naRef* args) :
    _c(naSubContext(c)),
    _start_element(argc > 1 ? args[1] : naNil()),
    _end_element(argc > 2 ? args[2] : naNil()),
    _data(argc > 3 ? args[3] : naNil()),
    _pi(argc > 4 ? args[4] : naNil())
{
}

void NasalXMLVisitor::startElement(const char* tag, const XMLAttributes& a)
{
    if(naIsNil(_start_element)) return;
    naRef attr = naNewHash(_c);
    for(int i=0; i<a.size(); i++) {
        naRef name = make_string(a.getName(i));
        naRef value = make_string(a.getValue(i));
        naHash_set(attr, name, value);
    }
    call(_start_element, 2, make_string(tag), attr);
}

void NasalXMLVisitor::endElement(const char* tag)
{
    if(!naIsNil(_end_element)) call(_end_element, 1, make_string(tag));
}

void NasalXMLVisitor::data(const char* str, int len)
{
    if(!naIsNil(_data)) call(_data, 1, make_string(str, len));
}

void NasalXMLVisitor::pi(const char* target, const char* data)
{
    if(!naIsNil(_pi)) call(_pi, 2, make_string(target), make_string(data));
}

void NasalXMLVisitor::call(naRef func, int num, naRef a, naRef b)
{
    naRef args[2];
    args[0] = a;
    args[1] = b;
    naCall(_c, func, num, args, naNil(), naNil());
    if(naGetError(_c))
        naRethrowError(_c);
}

naRef NasalXMLVisitor::make_string(const char* s, int n)
{
    return naStr_fromdata(naNewString(_c), const_cast<char *>(s),
                          n < 0 ? strlen(s) : n);
}


