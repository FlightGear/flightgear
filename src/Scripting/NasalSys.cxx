// Copyright (C) 2013  James Turner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "config.h"

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // gettimeofday
#endif

#include <algorithm>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

#include <simgear/nasal/nasal.h>
#include <simgear/nasal/iolib.h>
#include <simgear/props/props.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/misc/SimpleMarkdown.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/debug/BufferedLogCallback.hxx>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/timing/timestamp.hxx>

#include "NasalAddons.hxx"
#include "NasalAircraft.hxx"
#include "NasalCanvas.hxx"
#include "NasalClipboard.hxx"
#include "NasalCondition.hxx"
#include "NasalFlightPlan.hxx"
#include "NasalHTTP.hxx"
#include "NasalModelData.hxx"
#include "NasalPositioned.hxx"
#include "NasalSGPath.hxx"
#include "NasalString.hxx"
#include "NasalSys.hxx"
#include "NasalSys_private.hxx"
#include "NasalUnitTesting.hxx"

#include <Main/globals.hxx>
#include <Main/util.hxx>
#include <Main/fg_props.hxx>
#include <Main/sentryIntegration.hxx>

using std::map;
using std::string;
using std::vector;

void postinitNasalGUI(naRef globals, naContext c);

static FGNasalSys* nasalSys = nullptr;

// this is used by the test-suite to simplify
// how much Nasal modules we load by default
bool global_nasalMinimalInit = false;


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
  TimerObj(Context *c, FGNasalSys* sys, naRef f, naRef self, double interval) :
    _sys(sys),
    _func(f),
    _self(self),
    _interval(interval)
  {
    char nm[256];
    if (c) {
        snprintf(nm, 128, "maketimer-[%p]-%s:%d", (void*)this, naStr_data(naGetSourceFile(c, 0)), naGetLine(c, 0));
    }
    else {
        snprintf(nm, 128, "maketimer-%p", this);
    }
    _name = nm;
    _gcRoot =  naGCSave(f);
    _gcSelf = naGCSave(self);
    sys->addPersistentTimer(this);
  }

  virtual ~TimerObj()
  {
    stop();
    naGCRelease(_gcRoot);
    naGCRelease(_gcSelf);
      _sys->removePersistentTimer(this);
  }

  bool isRunning() const { return _isRunning; }

  void stop()
  {
    if (_isRunning) {
      globals->get_event_mgr()->removeTask(_name);
      _isRunning = false;
    }
  }

  bool isSimTime() const { return _isSimTime; }

  void setSimTime(bool value)
  {
      if (_isRunning) {
          SG_LOG(SG_NASAL, SG_WARN, "can't change type of running timer!");
          return;
      }

      _isSimTime = value;
  }

  void start()
  {
    if (_isRunning) {
      return;
    }

    _isRunning = true;
    if (_singleShot) {
      globals->get_event_mgr()->addEvent(_name, this, &TimerObj::invoke, _interval, _isSimTime);
    } else {
      globals->get_event_mgr()->addTask(_name, this, &TimerObj::invoke,
                                        _interval, _interval /* delay */,
                                        _isSimTime);
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

    naRef *args = nullptr;
    _sys->callMethod(_func, _self, 0, args, naNil() /* locals */);
  }

  void setSingleShot(bool aSingleShot)
  {
    _singleShot = aSingleShot;
  }

  bool isSingleShot() const
  { return _singleShot; }

  const std::string& name() const
  { return _name; }
private:
  std::string _name;
  FGNasalSys* _sys;
  naRef _func, _self;
  int _gcRoot, _gcSelf;
  bool _isRunning = false;
  double _interval;
  bool _singleShot = false;
  bool _isSimTime = false;
};

typedef SGSharedPtr<TimerObj> TimerObjRef;
typedef nasal::Ghost<TimerObjRef> NasalTimerObj;

static void f_timerObj_setSimTime(TimerObj& timer, naContext c, naRef value)
{
    if (timer.isRunning()) {
        naRuntimeError(c, "Timer is running, cannot change type between real/sim time");
        return;
    }

    timer.setSimTime(nasal::from_nasal<bool>(c, value));
}
////////////////////
/// Timestamp - used to provide millisecond based timing of operations. See SGTimeStamp
////
/// 0.373404us to perform elapsedUSec from Nasal : Tested i7-4790K, Win64
class TimeStampObj : public SGReferenced
{
public:
    TimeStampObj(Context *c)
    {
        timestamp.stamp();
    }

    virtual ~TimeStampObj() = default;

    void stamp()
    {
        timestamp.stamp();
    }
    double elapsedMSec()
    {
        return timestamp.elapsedMSec();
    }
    double elapsedUSec()
    {
        return timestamp.elapsedUSec();
    }
private:
    SGTimeStamp timestamp;
};

typedef SGSharedPtr<TimeStampObj> TimeStampObjRef;
typedef nasal::Ghost<TimeStampObjRef> NasalTimeStampObj;

///////////////////////////////////////////////////////////////////////////

FGNasalSys::FGNasalSys() :
    _inited(false)
{
     nasalSys = this;
    _context = 0;
    _globals = naNil();
    _string = naNil();
    _wrappedNodeFunc = naNil();

    _log.reset(new simgear::BufferedLogCallback(SG_NASAL, SG_INFO));
    _log->truncateAt(255);
    sglog().addCallback(_log.get());

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
    try {
        return naCallMethod(code, self, argc, args, locals);
    } catch (sg_exception& e) {
        SG_LOG(SG_NASAL, SG_DEV_ALERT, "caught exception invoking nasal method:" << e.what());
        return naNil();
    }
}

naRef FGNasalSys::callMethodWithContext(naContext ctx, naRef code, naRef self, int argc, naRef* args, naRef locals)
{
    try {
        return naCallMethodCtx(ctx, code, self, argc, args, locals);
    } catch (sg_exception& e) {
        SG_LOG(SG_NASAL, SG_DEV_ALERT, "caught exception invoking nasal method:" << e.what());
        string_list nasalStack;
        logNasalStack(ctx, nasalStack);
        flightgear::sentryReportNasalError(string{"Exception invoking nasal method:"} + e.what(), nasalStack);
        return naNil();
    }
}

FGNasalSys::~FGNasalSys()
{
    if (_inited) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Nasal was not shutdown");
    }
    sglog().removeCallback(_log.get());

    nasalSys = nullptr;
}

bool FGNasalSys::parseAndRunWithOutput(const std::string& source, std::string& output, std::string& errors)
{
    naContext ctx = naNewContext();
    naRef code = parse(ctx, "FGNasalSys::parseAndRun()", source.c_str(),
                       source.size(), errors);
    if(naIsNil(code)) {
        naFreeContext(ctx);
        return false;
    }
    naRef result = callWithContext(ctx, code, 0, 0, naNil());

    // if there was a result value, try to convert it to a string
    // value.
    if (!naIsNil(result)) {
        naRef s = naStringValue(ctx, result);
        if (!naIsNil(s)) {
            output = naStr_data(s);
        }
    }

    naFreeContext(ctx);
    return true;
}

bool FGNasalSys::parseAndRun(const std::string& source)
{
    std::string errors;
    std::string output;
    return parseAndRunWithOutput(source, output, errors);
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
// to the FlightGear log.  Uses the mandatory log level (SG_MANDATORY_INFO), to
// make sure it appears.  Users should be converted to use logprint() instead,
// and specify a level explicitly.
static naRef f_print(naContext c, naRef me, int argc, naRef* args)
{
    string buf;
    int n = argc;
    for(int i=0; i<n; i++) {
        naRef s = naStringValue(c, args[i]);
        if(naIsNil(s)) continue;
        buf += naStr_data(s);
    }
 
    /* Copy what SG_LOG() does, but use nasal file:line instead of
    our own __FILE__ and __LINE__. */
    if (sglog().would_log(SG_NASAL, SG_MANDATORY_INFO)) {
        int frame = 0;
        const char* file = naStr_data(naGetSourceFile(c, 0));
        if (simgear::strutils::ends_with( file, "/globals.nas")) {
            /* This generally means have been called by globals.nas's
            printf function; go one step up the stack so we give the
            file:line of the caller of printf, which is generally more
            useful. */
            frame += 1;
            file = naStr_data(naGetSourceFile(c, frame));
        }
        int line = naGetLine(c, frame);
        sglog().logCopyingFilename(SG_NASAL, SG_MANDATORY_INFO, file, line, buf);
    }

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
  sglog().logCopyingFilename(SG_NASAL, (sgDebugPriority)(int) priority.num,
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

    return naNum(globals->get_commands()->execute(naStr_data(cmd), node, nullptr));
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
  } else {
    naRuntimeError(c, "bad function/self arguments to maketimer");
  }

  TimerObj* timerObj = new TimerObj(c, nasalSys, func, self, args[0].num);
  return nasal::to_nasal(c, timerObj);
}

static naRef f_makeSingleShot(naContext c, naRef me, int argc, naRef* args)
{
    if (!naIsNum(args[0])) {
        naRuntimeError(c, "bad interval argument to makesingleshot");
    }

    naRef func, self = naNil();
    if (naIsFunc(args[1])) {
        func = args[1];
    } else if ((argc == 3) && naIsFunc(args[2])) {
        self = args[1];
        func = args[2];
    } else {
        naRuntimeError(c, "bad function/self arguments to makesingleshot");
    }

    auto timerObj = new TimerObj(c, nasalSys, func, self, args[0].num);
    timerObj->setSingleShot(true);
    timerObj->start();
    return nasal::to_nasal(c, timerObj);
}

static naRef f_maketimeStamp(naContext c, naRef me, int argc, naRef* args)
{
    TimeStampObj* timeStampObj = new TimeStampObj(c);
    return nasal::to_nasal(c, timeStampObj);
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

    SGPath dirname = fgValidatePath(SGPath::fromUtf8(naStr_data(args[0])), false);
    if(dirname.isNull()) {
        SG_LOG(SG_NASAL, SG_ALERT, "directory(): listing '" <<
        naStr_data(args[0]) << "' denied (unauthorized directory - authorization"
        " no longer follows symlinks; to authorize reading additional "
        "directories, pass them to --allow-nasal-read)");
        // to avoid breaking dialogs, pretend it doesn't exist rather than erroring out
        return naNil();
    }

    simgear::Dir d(dirname);
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
    std::string pdata = p.utf8Str();
    return naStr_fromdata(naNewString(c), const_cast<char*>(pdata.c_str()), pdata.length());
}

static naRef f_findDataDir(naContext c, naRef me, int argc, naRef* args)
{
    if(argc != 1 || !naIsString(args[0]))
        naRuntimeError(c, "bad arguments to findDataDir()");

    SGPath p = globals->findDataPath(naStr_data(args[0]));
    std::string pdata = p.utf8Str();
    return naStr_fromdata(naNewString(c), const_cast<char*>(pdata.c_str()), pdata.length());
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

    virtual bool operator()(const SGPropertyNode* aNode, SGPropertyNode * root)
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

    const string commandName(naStr_data(args[0]));
    bool ok = nasalSys->removeCommand(commandName);
    if (!ok) {
        return naNum(0);
    }
    return naNum(1);
}

static naRef f_open(naContext c, naRef me, int argc, naRef* args)
{
    FILE* f;
    naRef file = argc > 0 ? naStringValue(c, args[0]) : naNil();
    naRef mode = argc > 1 ? naStringValue(c, args[1]) : naNil();
    if(!naStr_data(file)) naRuntimeError(c, "bad argument to open()");
    const char* modestr = naStr_data(mode) ? naStr_data(mode) : "rb";
    SGPath filename = fgValidatePath(SGPath::fromUtf8(naStr_data(file)),
        strcmp(modestr, "rb") && strcmp(modestr, "r"));
    if(filename.isNull()) {
        SG_LOG(SG_NASAL, SG_ALERT, "open(): reading/writing '" <<
        naStr_data(file) << "' denied (unauthorized directory - authorization"
        " no longer follows symlinks; to authorize reading additional "
        "directories, pass them to --allow-nasal-read)");
        naRuntimeError(c, "open(): access denied (unauthorized directory)");
        return naNil();
    }

#if defined(SG_WINDOWS)
    std::wstring fp = filename.wstr();
    std::wstring wmodestr = simgear::strutils::convertUtf8ToWString(modestr);
    f = _wfopen(fp.c_str(), wmodestr.c_str());
#else
    std::string fp = filename.utf8Str();
    f = fopen(fp.c_str(), modestr);
#endif
    if(!f) naRuntimeError(c, strerror(errno));
    return naIOGhost(c, f);
}

static naRef ftype(naContext ctx, const SGPath& f)
{
    const char* t = "unk";
    if (f.isFile()) t = "reg";
    else if(f.isDir()) t = "dir";
    return naStr_fromdata(naNewString(ctx), t, strlen(t));
}

// io.stat with UTF-8 path support, replaces the default one in
// Nasal iolib.c which does not support UTF-8 paths
static naRef f_custom_stat(naContext ctx, naRef me, int argc, naRef* args)
{
    naRef pathArg = argc > 0 ? naStringValue(ctx, args[0]) : naNil();
    if (!naIsString(pathArg))
        naRuntimeError(ctx, "bad argument to stat()");

    const auto path = SGPath::fromUtf8(naStr_data(pathArg));
    if (!path.exists()) {
        return naNil();
    }

    const SGPath filename = fgValidatePath(path, false );
    if (filename.isNull()) {
        SG_LOG(SG_NASAL, SG_ALERT, "stat(): reading '" <<
        naStr_data(pathArg) << "' denied (unauthorized directory - authorization"
        " no longer follows symlinks; to authorize reading additional "
        "directories, pass them to --allow-nasal-read)");
        naRuntimeError(ctx, "stat(): access denied (unauthorized directory)");
        return naNil();
    }

    naRef result = naNewVector(ctx);
    naVec_setsize(ctx, result, 12);

    // every use in fgdata/Nasal only uses stat() to test existence of
    // files, not to check any of this information.
    int n = 0;
    naVec_set(result, n++, naNum(0)); // device
    naVec_set(result, n++, naNum(0)); // inode
    naVec_set(result, n++, naNum(0)); // mode
    naVec_set(result, n++, naNum(0)); // nlink
    naVec_set(result, n++, naNum(0)); // uid
    naVec_set(result, n++, naNum(0)); // guid
    naVec_set(result, n++, naNum(0)); // rdev
    naVec_set(result, n++, naNum(filename.sizeInBytes())); // size
    naVec_set(result, n++, naNum(0)); // atime
    naVec_set(result, n++, naNum(0)); // mtime
    naVec_set(result, n++, naNum(0)); // ctime
    naVec_set(result, n++, ftype(ctx, filename));
    return result;
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

    SGPath file = fgValidatePath(SGPath::fromUtf8(naStr_data(args[0])), false);
    if(file.isNull()) {
        SG_LOG(SG_NASAL, SG_ALERT, "parsexml(): reading '" <<
        naStr_data(args[0]) << "' denied (unauthorized directory - authorization"
        " no longer follows symlinks; to authorize reading additional "
        "directories, pass them to --allow-nasal-read)");
        naRuntimeError(c, "parsexml(): access denied (unauthorized directory)");
        return naNil();
    }

    NasalXMLVisitor visitor(c, argc, args);
    try {
        readXML(file, visitor);
    } catch (const sg_exception& e) {
        std::string fp = file.utf8Str();
        naRuntimeError(c, "parsexml(): file '%s' %s", fp.c_str(), e.getFormattedMessage().c_str());
        return naNil();
    }

    std::string fs = file.utf8Str();
    return naStr_fromdata(naNewString(c), fs.c_str(), fs.length());
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
static struct { const char* name; naCFunction func;
} funcs[] = {
    {"getprop", f_getprop},
    {"setprop", f_setprop},
    {"print", f_print},
    {"logprint", f_logprint},
    {"_fgcommand", f_fgcommand},
    {"settimer", f_settimer},
    {"maketimer", f_makeTimer},
    {"makesingleshot", f_makeSingleShot},
    {"_setlistener", f_setlistener},
    {"removelistener", f_removelistener},
    {"addcommand", f_addCommand},
    {"removecommand", f_removeCommand},
    {"_cmdarg", f_cmdarg},
    {"_interpolate", f_interpolate},
    {"rand", f_rand},
    {"srand", f_srand},
    {"abort", f_abort},
    {"directory", f_directory},
    {"resolvepath", f_resolveDataPath},
    {"finddata", f_findDataDir},
    {"parsexml", f_parsexml},
    {"parse_markdown", f_parse_markdown},
    {"md5", f_md5},
    {"systime", f_systime},
    {"maketimestamp", f_maketimeStamp},
    {0, 0}};

naRef FGNasalSys::cmdArgGhost()
{
    return propNodeGhost(_cmdArg);
}

void FGNasalSys::initLogLevelConstants()
{
    hashset(_globals, "LOG_BULK", naNum(SG_BULK));
    hashset(_globals, "LOG_WARN", naNum(SG_WARN));
    hashset(_globals, "LOG_DEBUG", naNum(SG_DEBUG));
    hashset(_globals, "LOG_INFO", naNum(SG_INFO));
    hashset(_globals, "LOG_ALERT", naNum(SG_ALERT));
    hashset(_globals, "DEV_WARN", naNum(SG_DEV_WARN));
    hashset(_globals, "DEV_ALERT", naNum(SG_DEV_ALERT));
    hashset(_globals, "MANDATORY_INFO", naNum(SG_MANDATORY_INFO));
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

    initLogLevelConstants();

    // Add our custom extension functions:
    for(i=0; funcs[i].name; i++)
        hashset(_globals, funcs[i].name,
                naNewFunc(_context, naNewCCode(_context, funcs[i].func)));
    nasal::Hash io_module = getGlobals().get<nasal::Hash>("io");
    io_module.set("open", f_open);
    io_module.set("stat", f_custom_stat);

    // And our SGPropertyNode wrapper
    hashset(_globals, "props", genPropsModule());

    // Add string methods
    _string = naInit_string(_context);
    naSave(_context, _string);
    initNasalString(_globals, _string, _context);

#if defined (BUILDING_TESTSUITE)
    initNasalUnitTestCppUnit(_globals, _context);
#else
    initNasalUnitTestInSim(_globals, _context);
#endif

    if (!global_nasalMinimalInit) {
        initNasalPositioned(_globals, _context);
        initNasalFlightPlan(_globals, _context);
        initNasalPositioned_cppbind(_globals, _context);
        initNasalAircraft(_globals, _context);
        NasalClipboard::init(this);
        initNasalCanvas(_globals, _context);
        initNasalCondition(_globals, _context);
        initNasalHTTP(_globals, _context);
        initNasalSGPath(_globals, _context);
    }

    NasalTimerObj::init("Timer")
      .method("start", &TimerObj::start)
      .method("stop", &TimerObj::stop)
      .method("restart", &TimerObj::restart)
      .member("singleShot", &TimerObj::isSingleShot, &TimerObj::setSingleShot)
      .member("simulatedTime", &TimerObj::isSimTime, &f_timerObj_setSimTime)
      .member("isRunning", &TimerObj::isRunning);

    NasalTimeStampObj::init("TimeStamp")
        .method("stamp", &TimeStampObj::stamp)
        .method("elapsedMSec", &TimeStampObj::elapsedMSec)
        .method("elapsedUSec", &TimeStampObj::elapsedUSec)
        ;

    // everything after here, skip if we're doing minimal init, so
    // we don'tload FG_DATA/Nasal or add-ons
    if (global_nasalMinimalInit) {
        _inited = true;
        return;
    }

    flightgear::addons::initAddonClassesForNasal(_globals, _context);

    // Now load the various source files in the Nasal directory
    simgear::Dir nasalDir(SGPath(globals->get_fg_root(), "Nasal"));
    loadScriptDirectory(nasalDir, globals->get_props()->getNode("/sim/nasal-load-priority"));

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
    shutdownNasalFlightPlan();
    shutdownNasalUnitTestInSim();

    for (auto l : _listener)
        delete l.second;
    _listener.clear();

    for (auto c : _commands) {
        globals->get_commands()->removeCommand(c.first);
    }
    _commands.clear();

    for(auto ml : _moduleListeners)
        delete ml;
    _moduleListeners.clear();

    for (auto t : _nasalTimers) {
        delete t;
    }
    _nasalTimers.clear();

    naClearSaved();

    _string = naNil(); // will be freed by _context
    naFreeContext(_context);

    //setWatchedRef(_globals);

    // remove the recursive reference in globals
    hashset(_globals, "globals", naNil());
    _globals = naNil();

    naGC();

    // Destroy all queued ghosts : important to ensure persistent timers are
    // destroyed now.
    nasal::ghostProcessDestroyList();

    if (!_persistentTimers.empty()) {
        SG_LOG(SG_NASAL, SG_DEV_WARN, "Extant persistent timer count:" << _persistentTimers.size());

        for (auto pt : _persistentTimers) {
            SG_LOG(SG_NASAL, SG_DEV_WARN, "Extant:" << pt << " : " << pt->name());
        }
    }

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

    std::for_each(_dead_listener.begin(), _dead_listener.end(),
                  []( FGNasalListener* l) { delete l; });
    _dead_listener.clear();

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

// Loads all scripts in given directory, with an optional partial ordering of
// files defined in loadorder.
void FGNasalSys::loadScriptDirectory(simgear::Dir nasalDir, SGPropertyNode* loadorder)
{
    simgear::PathList scripts = nasalDir.children(simgear::Dir::TYPE_FILE, ".nas");

    if (loadorder != nullptr && loadorder->hasChild("file")) {
      // Load any scripts defined in the loadorder in order, removing them from
      // the list so they don't get loaded twice.
      simgear::PropertyList files = loadorder->getChildren("file");

      auto loadAndErase = [ &scripts, &nasalDir, this ] (SGPropertyNode_ptr n) {
        SGPath p = SGPath(nasalDir.path(), n->getStringValue());
        auto script = std::find(scripts.begin(), scripts.end(), p);
        if (script != scripts.end()) {
          this->loadModule(p, p.file_base().c_str());
          scripts.erase(script);
        }
      };

      std::for_each(files.begin(), files.end(), loadAndErase);
    }

    // Load any remaining scripts.
    // Note: simgear::Dir already reports file entries in a deterministic order,
    // so a fixed loading sequence is guaranteed (same for every user)
    std::for_each(scripts.begin(), scripts.end(), [this](SGPath p) { this->loadModule(p, p.file_base().c_str()); });
}

// Create module with list of scripts
void FGNasalSys::addModule(string moduleName, simgear::PathList scripts)
{
    if (! scripts.empty())
    {
        SGPropertyNode* nasal = globals->get_props()->getNode("nasal", true);
        SGPropertyNode* module_node = nasal->getChild(moduleName,0,true);
        for (unsigned int i=0; i<scripts.size(); ++i) {
            SGPropertyNode* pFileNode = module_node->getChild("file",i,true);
            pFileNode->setStringValue(scripts[i].utf8Str());
        }
        if (!module_node->hasChild("enabled",0))
        {
            SGPropertyNode* node = module_node->getChild("enabled",0,true);
            node->setBoolValue(false);
            node->setAttribute(SGPropertyNode::USERARCHIVE,false);
            SG_LOG(SG_NASAL, SG_ALERT, "Nasal module " <<
            moduleName <<
            " present in FGDATA/Nasal but not configured in defaults.xml. " <<
            " Please add an entry to defaults.xml, and set "
            << node->getPath() <<
            "=true to load the module on-demand at runtime when required.");
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
    string errorMessage = naGetError(context);
    SG_LOG(SG_NASAL, SG_ALERT, "Nasal runtime error: " << errorMessage);

    string_list nasalStack;
    logNasalStack(context, nasalStack);
    flightgear::sentryReportNasalError(errorMessage, nasalStack);
}

void FGNasalSys::logNasalStack(naContext context, string_list& stack)
{
    const int stack_depth = naStackDepth(context);
    if (stack_depth < 1)
      return;

    stack.push_back(string{naStr_data(naGetSourceFile(context, 0))} +
                    ", line " + std::to_string(naGetLine(context, 0)));

    SG_LOG(SG_NASAL, SG_ALERT,
           "  at " << naStr_data(naGetSourceFile(context, 0)) <<
           ", line " << naGetLine(context, 0));

    for(int i=1; i<stack_depth; i++) {
        SG_LOG(SG_NASAL, SG_ALERT,
               "  called from: " << naStr_data(naGetSourceFile(context, i)) <<
               ", line " << naGetLine(context, i));

        stack.push_back(string{naStr_data(naGetSourceFile(context, i))} +
                        ", line " + std::to_string(naGetLine(context, i)));
    }
}

// Reads a script file, executes it, and places the resulting
// namespace into the global namespace under the specified module
// name.
bool FGNasalSys::loadModule(SGPath file, const char* module)
{
    if (!file.exists()) {
        SG_LOG(SG_NASAL, SG_ALERT, "Cannot load module, missing file:" << file);
        return false;
    }

    sg_ifstream file_in(file);
    string buf;
    while (!file_in.eof()) {
        char bytes[8192];
        file_in.read(bytes, 8192);
        buf.append(bytes, file_in.gcount());
    }
    file_in.close();
    auto pathStr = file.utf8Str();
    return createModule(module, pathStr.c_str(), buf.data(), buf.length());
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
    std::string errors;
    naRef code = parse(ctx, fileName, src, len, errors);
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
    if (naIsNil(_globals))
        return false;

    if (!naHash_get(_globals, modname, &locals))
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

naRef FGNasalSys::getModule(const char* moduleName)
{
    naRef mod = naHash_cget(_globals, (char*) moduleName);
    return mod;
}

naRef FGNasalSys::parse(naContext ctx, const char* filename,
                        const char* buf, int len,
                        std::string& errors)
{
    int errLine = -1;
    naRef srcfile = naNewString(ctx);
    naStr_fromdata(srcfile, (char*)filename, strlen(filename));
    naRef code = naParseCode(ctx, srcfile, 1, (char*)buf, len, &errLine);
    if(naIsNil(code)) {
        std::ostringstream errorMessageStream;
        errorMessageStream << "Nasal parse error: " << naGetError(ctx) <<
            " in "<< filename <<", line " << errLine;
        errors = errorMessageStream.str();
        SG_LOG(SG_NASAL, SG_ALERT, errors);

        return naNil();
    }

    // Bind to the global namespace before returning
    return naBindFunction(ctx, code, _globals);
}

bool FGNasalSys::handleCommand( const char* moduleName,
                                const char* fileName,
                                const char* src,
                                const SGPropertyNode* arg,
                                SGPropertyNode* root)
{
    naContext ctx = naNewContext();
    std::string errorMessage;
    naRef code = parse(ctx, fileName, src, strlen(src), errorMessage);
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

bool FGNasalSys::handleCommand(const SGPropertyNode * arg, SGPropertyNode * root)
{
  const char* src = arg->getStringValue("script");
  const char* moduleName = arg->getStringValue("module");

  return handleCommand( moduleName,
                        arg->getPath(true).c_str(),
                        src,
                        arg,
                        root);
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

    // A unique name for the timer based on the file name and line number of the function.
    std::string name = "settimer-";
    name.append(naStr_data(naGetSourceFile(c, 0)));
    name.append(":");
    name.append(std::to_string(naGetLine(c, 0)));

    // Generate and register a C++ timer handler
    NasalTimer* t = new NasalTimer(handler, this);
    _nasalTimers.push_back(t);
    globals->get_event_mgr()->addEvent(name,
                                       t, &NasalTimer::timerExpired,
                                       delta.num, simtime);
}

void FGNasalSys::handleTimer(NasalTimer* t)
{
    call(t->handler, 0, 0, naNil());
    auto it =  std::find(_nasalTimers.begin(), _nasalTimers.end(), t);
    assert(it != _nasalTimers.end());
    _nasalTimers.erase(it);
    delete t;
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

NasalTimer::NasalTimer(naRef h, FGNasalSys* sys) :
    handler(h), nasal(sys)
{
    assert(sys);
    gcKey = naGCSave(handler);
}

NasalTimer::~NasalTimer()
{
    naGCRelease(gcKey);
}

void NasalTimer::timerExpired()
{
    nasal->handleTimer(this);
    // note handleTimer calls delete on us, don't do anything
    // which requires 'this' to be valid here
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

    if (node->isTied()) {
        const auto isSafe = node->getAttribute(SGPropertyNode::LISTENER_SAFE);
        if (!isSafe) {
            SG_LOG(SG_NASAL, SG_DEV_ALERT, "ERROR: Cannot add listener to tied property " <<
                     node->getPath());
        }
    }

    naRef code = argc > 1 ? args[1] : naNil();
    if(!(naIsCode(code) || naIsCCode(code) || naIsFunc(code))) {
        naRuntimeError(c, "setlistener() with invalid function argument");
        return naNil();
    }

    int init = argc > 2 && naIsNum(args[2]) ? int(args[2].num) : 0; // do not trigger when created
    int type = argc > 3 && naIsNum(args[3]) ? int(args[3].num) : 1; // trigger will always be triggered when the property is written
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
    auto it = _listener.find(int(id.num));
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

bool FGNasalSys::removeCommand(const std::string& name)
{
    auto it = _commands.find(name);
    if (it == _commands.end()) {
        SG_LOG(SG_NASAL, SG_WARN, "remove of unknwon command:" << name);
        return false;
    }

    // will delete the NasalCommand instance
    bool ok = globals->get_commands()->removeCommand(name);
    _commands.erase(it);
    return ok;
}

void FGNasalSys::addPersistentTimer(TimerObj* pto)
{
    _persistentTimers.push_back(pto);
}

void FGNasalSys::removePersistentTimer(TimerObj* obj)
{
    auto it = std::find(_persistentTimers.begin(), _persistentTimers.end(), obj);
    assert(it != _persistentTimers.end());
    _persistentTimers.erase(it);
}

// Register the subsystem.
SGSubsystemMgr::Registrant<FGNasalSys> registrantFGNasalSys(
    SGSubsystemMgr::INIT);


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
