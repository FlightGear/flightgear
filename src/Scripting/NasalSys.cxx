
#ifdef HAVE_CONFIG_H
#  include "config.h"
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
#include <simgear/misc/interpolator.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/event_mgr.hxx>

#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/util.hxx>
#include <Scenery/scenery.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/procedure.hxx>

#include "NasalSys.hxx"

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
    nasalSys = this;
    _context = 0;
    _globals = naNil();
    _gcHash = naNil();
    _nextGCKey = 0; // Any value will do
    _callCount = 0;
}

// Does a naCall() in a new context.  Wrapped here to make lock
// tracking easier.  Extension functions are called with the lock, but
// we have to release it before making a new naCall().  So rather than
// drop the lock in every extension function that might call back into
// Nasal, we keep a stack depth counter here and only unlock/lock
// around the naCall if it isn't the first one.
naRef FGNasalSys::call(naRef code, int argc, naRef* args, naRef locals)
{
    naContext ctx = naNewContext();
    if(_callCount) naModUnlock();
    _callCount++;
    naRef result = naCall(ctx, code, argc, args, naNil(), locals);
    if(naGetError(ctx))
        logError(ctx);
    _callCount--;
    if(_callCount) naModLock();
    naFreeContext(ctx);
    return result;
}

FGNasalSys::~FGNasalSys()
{
    nasalSys = 0;
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
    call(code, 0, 0, naNil());
    return true;
}

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
    using namespace simgear;
    const SGPropertyNode* p = findnode(c, args, argc);
    if(!p) return naNil();

    switch(p->getType()) {
    case props::BOOL:   case props::INT:
    case props::LONG:   case props::FLOAT:
    case props::DOUBLE:
        {
        double dv = p->getDoubleValue();
        if (osg::isNaN(dv)) {
          SG_LOG(SG_GENERAL, SG_ALERT, "Nasal getprop: property " << p->getPath() << " is NaN");
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
#define BUFLEN 1024
    char buf[BUFLEN + 1];
    buf[BUFLEN] = 0;
    char* p = buf;
    int buflen = BUFLEN;
    if(argc < 2) naRuntimeError(c, "setprop() expects at least 2 arguments");
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
    bool result = false;
    try {
        if(naIsString(val)) result = props->setStringValue(buf, naStr_data(val));
        else {
            naRef n = naNumValue(val);
            if(naIsNil(n))
                naRuntimeError(c, "setprop() value is not string or number");
                
            if (osg::isNaN(n.num)) {
                naRuntimeError(c, "setprop() passed a NaN");
            }
            
            result = props->setDoubleValue(buf, n.num);
        }
    } catch (const string& err) {
        naRuntimeError(c, (char *)err.c_str());
    }
    return naNum(result);
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
    naRef cmd = argc > 0 ? args[0] : naNil();
    naRef props = argc > 1 ? args[1] : naNil();
    if(!naIsString(cmd) || (!naIsNil(props) && !naIsGhost(props)))
        naRuntimeError(c, "bad arguments to fgcommand()");
    SGPropertyNode_ptr tmp, *node;
    if(!naIsNil(props))
        node = (SGPropertyNode_ptr*)naGhost_ptr(props);
    else {
        tmp = new SGPropertyNode();
        node = &tmp;
    }
    return naNum(globals->get_commands()->execute(naStr_data(cmd), *node));
}

// settimer(func, dt, simtime) extension function.  Falls through to
// FGNasalSys::setTimer().  See there for docs.
static naRef f_settimer(naContext c, naRef me, int argc, naRef* args)
{
    nasalSys->setTimer(c, argc, args);
    return naNil();
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

    ((SGInterpolator*)globals->get_subsystem_mgr()
        ->get_group(SGSubsystemMgr::INIT)->get_subsystem("interpolator"))
        ->interpolate(node, nPoints, values, deltas);

    delete[] values;
    delete[] deltas;
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

// Convert a cartesian point to a geodetic lat/lon/altitude.
static naRef f_carttogeod(naContext c, naRef me, int argc, naRef* args)
{
    double lat, lon, alt, xyz[3];
    if(argc != 3) naRuntimeError(c, "carttogeod() expects 3 arguments");
    for(int i=0; i<3; i++)
        xyz[i] = naNumValue(args[i]).num;
    sgCartToGeod(xyz, &lat, &lon, &alt);
    lat *= SG_RADIANS_TO_DEGREES;
    lon *= SG_RADIANS_TO_DEGREES;
    naRef vec = naNewVector(c);
    naVec_append(vec, naNum(lat));
    naVec_append(vec, naNum(lon));
    naVec_append(vec, naNum(alt));
    return vec;
}

// Convert a geodetic lat/lon/altitude to a cartesian point.
static naRef f_geodtocart(naContext c, naRef me, int argc, naRef* args)
{
    if(argc != 3) naRuntimeError(c, "geodtocart() expects 3 arguments");
    double lat = naNumValue(args[0]).num * SG_DEGREES_TO_RADIANS;
    double lon = naNumValue(args[1]).num * SG_DEGREES_TO_RADIANS;
    double alt = naNumValue(args[2]).num;
    double xyz[3];
    sgGeodToCart(lat, lon, alt, xyz);
    naRef vec = naNewVector(c);
    naVec_append(vec, naNum(xyz[0]));
    naVec_append(vec, naNum(xyz[1]));
    naVec_append(vec, naNum(xyz[2]));
    return vec;
}

// For given geodetic point return array with elevation, and a material data
// hash, or nil if there's no information available (tile not loaded). If
// information about the material isn't available, then nil is returned instead
// of the hash.
static naRef f_geodinfo(naContext c, naRef me, int argc, naRef* args)
{
#define HASHSET(s,l,n) naHash_set(matdata, naStr_fromdata(naNewString(c),s,l),n)
    if(argc < 2 || argc > 3)
        naRuntimeError(c, "geodinfo() expects 2 or 3 arguments: lat, lon [, maxalt]");
    double lat = naNumValue(args[0]).num;
    double lon = naNumValue(args[1]).num;
    double elev = argc == 3 ? naNumValue(args[2]).num : 10000;
    const SGMaterial *mat;
    SGGeod geod = SGGeod::fromDegM(lon, lat, elev);
    if(!globals->get_scenery()->get_elevation_m(geod, elev, &mat))
        return naNil();
    naRef vec = naNewVector(c);
    naVec_append(vec, naNum(elev));
    naRef matdata = naNil();
    if(mat) {
        matdata = naNewHash(c);
        naRef names = naNewVector(c);
        const vector<string> n = mat->get_names();
        for(unsigned int i=0; i<n.size(); i++)
            naVec_append(names, naStr_fromdata(naNewString(c),
                    const_cast<char*>(n[i].c_str()), n[i].size()));
        HASHSET("names", 5, names);
        HASHSET("solid", 5, naNum(mat->get_solid()));
        HASHSET("friction_factor", 15, naNum(mat->get_friction_factor()));
        HASHSET("rolling_friction", 16, naNum(mat->get_rolling_friction()));
        HASHSET("load_resistance", 15, naNum(mat->get_load_resistance()));
        HASHSET("bumpiness", 9, naNum(mat->get_bumpiness()));
        HASHSET("light_coverage", 14, naNum(mat->get_light_coverage()));
    }
    naVec_append(vec, matdata);
    return vec;
#undef HASHSET
}


class AirportInfoFilter : public FGAirport::AirportFilter
{
public:
    AirportInfoFilter() : type(FGPositioned::AIRPORT) {
    }

    virtual FGPositioned::Type minType() const {
        return type;
    }

    virtual FGPositioned::Type maxType() const {
        return type;
    }

    FGPositioned::Type type;
};

// Returns data hash for particular or nearest airport of a <type>, or nil
// on error.
//
// airportinfo(<id>);                   e.g. "KSFO"
// airportinfo(<type>);                 type := ("airport"|"seaport"|"heliport")
// airportinfo()                        same as  airportinfo("airport")
// airportinfo(<lat>, <lon> [, <type>]);
static naRef f_airportinfo(naContext c, naRef me, int argc, naRef* args)
{
    static SGConstPropertyNode_ptr latn = fgGetNode("/position/latitude-deg", true);
    static SGConstPropertyNode_ptr lonn = fgGetNode("/position/longitude-deg", true);
    SGGeod pos;
    FGAirport* apt = NULL;

    if(argc >= 2 && naIsNum(args[0]) && naIsNum(args[1])) {
        pos = SGGeod::fromDeg(args[1].num, args[0].num);
        args += 2;
        argc -= 2;
    } else {
        pos = SGGeod::fromDeg(lonn->getDoubleValue(), latn->getDoubleValue());
    }

    double maxRange = 10000.0; // expose this? or pick a smaller value?

    AirportInfoFilter filter; // defaults to airports only

    if(argc == 0) {
        // fall through and use AIRPORT
    } else if(argc == 1 && naIsString(args[0])) {
        const char *s = naStr_data(args[0]);
        if(!strcmp(s, "airport")) filter.type = FGPositioned::AIRPORT;
        else if(!strcmp(s, "seaport")) filter.type = FGPositioned::SEAPORT;
        else if(!strcmp(s, "heliport")) filter.type = FGPositioned::HELIPORT;
        else {
            // user provided an <id>, hopefully
            apt = FGAirport::findByIdent(s);
            if (!apt) {
                // return nil here, but don't raise a runtime error; this is a
                // legitamate way to validate an ICAO code, for example in a
                // dialog box or similar.
                return naNil();
            }
        }
    } else {
        naRuntimeError(c, "airportinfo() with invalid function arguments");
        return naNil();
    }

    if(!apt) {
        apt = FGAirport::findClosest(pos, maxRange, &filter);
        if(!apt) return naNil();
    }

    string id = apt->ident();
    string name = apt->name();

    // set runway hash
    naRef rwys = naNewHash(c);
    for(unsigned int r=0; r<apt->numRunways(); ++r) {
        FGRunway* rwy(apt->getRunwayByIndex(r));

        naRef rwyid = naStr_fromdata(naNewString(c),
                      const_cast<char *>(rwy->ident().c_str()),
                      rwy->ident().length());

        naRef rwydata = naNewHash(c);
#define HASHSET(s,l,n) naHash_set(rwydata, naStr_fromdata(naNewString(c),s,l),n)
        HASHSET("id", 2, rwyid);
        HASHSET("lat", 3, naNum(rwy->latitude()));
        HASHSET("lon", 3, naNum(rwy->longitude()));
        HASHSET("heading", 7, naNum(rwy->headingDeg()));
        HASHSET("length", 6, naNum(rwy->lengthM()));
        HASHSET("width", 5, naNum(rwy->widthM()));
        HASHSET("threshold", 9, naNum(rwy->displacedThresholdM()));
        HASHSET("stopway", 7, naNum(rwy->stopwayM()));
        
        if (rwy->ILS()) {
          HASHSET("ils_frequency_mhz", 17, naNum(rwy->ILS()->get_freq() / 100.0));
        }
        
        std::vector<flightgear::SID*> sids(rwy->getSIDs());
        naRef sidVec = naNewVector(c);
        
        for (unsigned int s=0; s < sids.size(); ++s) {
          naRef procId = naStr_fromdata(naNewString(c),
                    const_cast<char *>(sids[s]->ident().c_str()),
                    sids[s]->ident().length());
          naVec_append(sidVec, procId);
        }
        HASHSET("sids", 4, sidVec); 
        
        std::vector<flightgear::STAR*> stars(rwy->getSTARs());
        naRef starVec = naNewVector(c);
      
        for (unsigned int s=0; s < stars.size(); ++s) {
          naRef procId = naStr_fromdata(naNewString(c),
                    const_cast<char *>(stars[s]->ident().c_str()),
                    stars[s]->ident().length());
          naVec_append(starVec, procId);
        }
        HASHSET("stars", 5, starVec); 

#undef HASHSET
        naHash_set(rwys, rwyid, rwydata);
    }

    // set airport hash
    naRef aptdata = naNewHash(c);
#define HASHSET(s,l,n) naHash_set(aptdata, naStr_fromdata(naNewString(c),s,l),n)
    HASHSET("id", 2, naStr_fromdata(naNewString(c),
            const_cast<char *>(id.c_str()), id.length()));
    HASHSET("name", 4, naStr_fromdata(naNewString(c),
            const_cast<char *>(name.c_str()), name.length()));
    HASHSET("lat", 3, naNum(apt->getLatitude()));
    HASHSET("lon", 3, naNum(apt->getLongitude()));
    HASHSET("elevation", 9, naNum(apt->getElevation() * SG_FEET_TO_METER));
    HASHSET("has_metar", 9, naNum(apt->getMetar()));
    HASHSET("runways", 7, rwys);
#undef HASHSET
    return aptdata;
}


// Table of extension functions.  Terminate with zeros.
static struct { const char* name; naCFunction func; } funcs[] = {
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
    { "abort", f_abort },
    { "directory", f_directory },
    { "resolvepath", f_resolveDataPath },
    { "parsexml", f_parsexml },
    { "systime", f_systime },
    { "carttogeod", f_carttogeod },
    { "geodtocart", f_geodtocart },
    { "geodinfo", f_geodinfo },
    { "airportinfo", f_airportinfo },
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
    signal->removeChildren(s, false);

    // Pull scripts out of the property tree, too
    loadPropertyScripts();
}

void FGNasalSys::update(double)
{
    if(!_dead_listener.empty()) {
        vector<FGNasalListener *>::iterator it, end = _dead_listener.end();
        for(it = _dead_listener.begin(); it != end; ++it) delete *it;
        _dead_listener.clear();
    }

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
    // sort scripts, avoid loading sequence effects due to file system's
    // random directory order
    std::sort(scripts.begin(), scripts.end(), pathSortPredicate);

    for (unsigned int i=0; i<scripts.size(); ++i) {
      SGPath fullpath(scripts[i]);
      SGPath file = fullpath.file();
      loadModule(fullpath, file.base().c_str());
    }
}

// Create module with list of scripts
void FGNasalSys::addModule(string moduleName, simgear::PathList scripts)
{
    if (scripts.size()>0)
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
            }
            ok &= loadModule(p, module);
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
    naRef code = parse(fileName, src, len);
    if(naIsNil(code))
        return false;

    // See if we already have a module hash to use.  This allows the
    // user to, for example, add functions to the built-in math
    // module.  Make a new one if necessary.
    naRef locals;
    naRef modname = naNewString(_context);
    naStr_fromdata(modname, (char*)moduleName, strlen(moduleName));
    if(!naHash_get(_globals, modname, &locals))
        locals = naNewHash(_context);

    _cmdArg = (SGPropertyNode*)cmdarg;

    call(code, argc, args, locals);
    hashset(_globals, moduleName, locals);
    return true;
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
    naRef code = parse(arg->getPath(true).c_str(), nasal, strlen(nasal));
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

    call(code, 0, 0, locals);
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

// setlistener(<property>, <func> [, <initial=0> [, <persistent=1>]])
// Attaches a callback function to a property (specified as a global
// property path string or a SGPropertyNode_ptr* ghost). If the third,
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
    else if(naIsGhost(prop)) node = *(SGPropertyNode_ptr*)naGhost_ptr(prop);
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

    node->addChangeListener(nl, init);

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
    SG_LOG(SG_NASAL, SG_DEBUG, "trigger listener #" << _id);
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



// FGNasalModelData class.  If sgLoad3DModel() is called with a pointer to
// such a class, then it lets modelLoaded() run the <load> script, and the
// destructor the <unload> script. The latter happens when the model branch
// is removed from the scene graph.

unsigned int FGNasalModelData::_module_id = 0;

void FGNasalModelData::modelLoaded(const string& path, SGPropertyNode *prop,
                                   osg::Node *)
{
    if(!prop)
        return;
    SGPropertyNode *nasal = prop->getNode("nasal");
    if(!nasal)
        return;

    SGPropertyNode *load = nasal->getNode("load");
    _unload = nasal->getNode("unload");
    if(!load && !_unload)
        return;

    std::stringstream m;
    m << "__model" << _module_id++;
    _module = m.str();

    const char *s = load ? load->getStringValue() : "";

    naRef arg[2];
    arg[0] = nasalSys->propNodeGhost(_root);
    arg[1] = nasalSys->propNodeGhost(prop);
    nasalSys->createModule(_module.c_str(), path.c_str(), s, strlen(s),
                           _root, 2, arg);
}

FGNasalModelData::~FGNasalModelData()
{
    if(_module.empty())
        return;

    if(!nasalSys) {
        SG_LOG(SG_NASAL, SG_WARN, "Trying to run an <unload> script "
                "without Nasal subsystem present.");
        return;
    }

    if(_unload) {
        const char *s = _unload->getStringValue();
        nasalSys->createModule(_module.c_str(), _module.c_str(), s, strlen(s), _root);
    }
    nasalSys->deleteModule(_module.c_str());
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


