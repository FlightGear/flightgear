// options.cxx -- class to handle command line options
//
// Written by Curtis Olson, started April 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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
//
// $Id$


#include <config.h>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/misc/sg_dir.hxx>

#include <cJSON.h>

#include <cmath>        // rint()
#include <cstdio>
#include <cstdlib>      // atof(), atoi()
#include <cstring>      // strcmp()
#include <algorithm>
#include <map>

#include <iostream>
#include <string>
#include <sstream>

#include <simgear/math/sg_random.h>
#include <simgear/props/props_io.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/sound/soundmgr.hxx>
#include <simgear/misc/strutils.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Aircraft/replay.hxx>
#include <Aircraft/initialstate.hxx>

#include <GUI/gui.h>
#include <GUI/MessageBox.hxx>

#if defined(HAVE_QT)
#include <GUI/QtLauncher.hxx>
#include <GUI/SetupRootDialog.hxx>
#endif

#include <AIModel/AIManager.hxx>
#include <Add-ons/AddonManager.hxx>
#include <Main/locale.hxx>
#include <Navaids/NavDataCache.hxx>
#include "globals.hxx"
#include "fg_init.hxx"
#include "fg_os.hxx"
#include "fg_props.hxx"
#include "options.hxx"
#include "util.hxx"
#include "main.hxx"
#include "locale.hxx"
#include <Viewer/view.hxx>
#include <Viewer/viewmgr.hxx>
#include <Environment/presets.hxx>
#include <Network/http/httpd.hxx>
#include "AircraftDirVisitorBase.hxx"

#include <osg/Version>
#include <flightgearBuildId.h>
#include <simgear/version.h>

using std::string;
using std::sort;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::cin;

using namespace flightgear;

#define NEW_DEFAULT_MODEL_HZ 120

static flightgear::Options* shared_instance = nullptr;

static double
atof( const string& str )
{
    return ::atof( str.c_str() );
}

static int
atoi( const string& str )
{
    return ::atoi( str.c_str() );
}

static int fgSetupProxy( const char *arg );

/**
 * Set a few fail-safe default property values.
 *
 * These should all be set in $FG_ROOT/defaults.xml, but just
 * in case, we provide some initial sane values here. This method
 * should be invoked *before* reading any init files.
 */
void fgSetDefaults ()
{

    // Position (deliberately out of range)
    fgSetDouble("/position/longitude-deg", 9999.0);
    fgSetDouble("/position/latitude-deg", 9999.0);
    fgSetDouble("/position/altitude-ft", -9999.0);

    // Orientation
    fgSetDouble("/orientation/heading-deg", 9999.0);
    fgSetDouble("/orientation/roll-deg", 0.0);
    fgSetDouble("/orientation/pitch-deg", 0.424);

    // Velocities
    fgSetDouble("/velocities/uBody-fps", 0.0);
    fgSetDouble("/velocities/vBody-fps", 0.0);
    fgSetDouble("/velocities/wBody-fps", 0.0);
    fgSetDouble("/velocities/speed-north-fps", 0.0);
    fgSetDouble("/velocities/speed-east-fps", 0.0);
    fgSetDouble("/velocities/speed-down-fps", 0.0);
    fgSetDouble("/velocities/airspeed-kt", 0.0);
    fgSetDouble("/velocities/mach", 0.0);

    // Presets
    fgSetDouble("/sim/presets/longitude-deg", 9999.0);
    fgSetDouble("/sim/presets/latitude-deg", 9999.0);
    fgSetDouble("/sim/presets/altitude-ft", -9999.0);

    fgSetDouble("/sim/presets/heading-deg", 9999.0);
    fgSetDouble("/sim/presets/roll-deg", 0.0);
    fgSetDouble("/sim/presets/pitch-deg", 0.424);

    fgSetString("/sim/presets/speed-set", "knots");
    fgSetDouble("/sim/presets/airspeed-kt", 0.0);
    fgSetDouble("/sim/presets/mach", 0.0);
    fgSetDouble("/sim/presets/uBody-fps", 0.0);
    fgSetDouble("/sim/presets/vBody-fps", 0.0);
    fgSetDouble("/sim/presets/wBody-fps", 0.0);
    fgSetDouble("/sim/presets/speed-north-fps", 0.0);
    fgSetDouble("/sim/presets/speed-east-fps", 0.0);
    fgSetDouble("/sim/presets/speed-down-fps", 0.0);
    fgSetDouble("/sim/presets/offset-distance-nm", 0.0);

    fgSetBool("/sim/presets/runway-requested", false);

    fgSetBool("/sim/presets/onground", true);
    fgSetBool("/sim/presets/trim", false);

    // Miscellaneous
    fgSetBool("/sim/startup/splash-screen", true);
    // we want mouse-pointer to have an undefined value if nothing is
    // specified so we can do the right thing for voodoo-1/2 cards.
    // fgSetString("/sim/startup/mouse-pointer", "disabled");
    fgSetBool("/controls/flight/auto-coordination", false);
    fgSetString("/sim/logging/priority", "alert");

    // Features
    fgSetBool("/sim/hud/color/antialiased", false);
    fgSetBool("/sim/hud/enable3d[1]", true);
    fgSetBool("/sim/hud/visibility[1]", false);
    fgSetBool("/sim/panel/visibility", true);
    fgSetBool("/sim/sound/enabled", true);
    fgSetBool("/sim/sound/working", true);
    fgSetBool("/sim/fgcom/enabled", false);

    // Flight Model options
    fgSetString("/sim/flight-model", "jsb");
    fgSetString("/sim/aero", "c172");
    fgSetInt("/sim/model-hz", NEW_DEFAULT_MODEL_HZ);
    fgSetDouble("/sim/speed-up", 1.0);

    // Scenery
    fgSetString("/sim/scenery/engine", "tilecache");

    // ( scenery = pagedLOD )
    fgSetString("/sim/scenery/lod-levels", "1 3 5 7 9");
    fgSetString("/sim/scenery/lod-res", "1");
    fgSetString("/sim/scenery/lod-texturing", "bluemarble");

    // Rendering options
    fgSetString("/sim/rendering/fog", "nicest");
    fgSetBool("/environment/clouds/status", true);
    fgSetBool("/sim/startup/fullscreen", false);
    fgSetBool("/sim/rendering/shading", true);
    fgTie( "/sim/rendering/filtering", SGGetTextureFilter, SGSetTextureFilter, false);
    fgSetInt("/sim/rendering/filtering", 1);
    fgSetBool("/sim/rendering/wireframe", false);
    fgSetBool("/sim/rendering/horizon-effect", false);
    fgSetBool("/sim/rendering/distance-attenuation", false);
    fgSetBool("/sim/rendering/specular-highlight", true);
    fgSetString("/sim/rendering/materials-file", "materials.xml");
    fgSetInt("/sim/startup/xsize", 1024);
    fgSetInt("/sim/startup/ysize", 768);
    fgSetInt("/sim/rendering/bits-per-pixel", 32);
    fgSetString("/sim/view-mode", "pilot");
    fgSetDouble("/sim/current-view/heading-offset-deg", 0);

    // HUD options
    fgSetString("/sim/startup/units", "feet");
    fgSetString("/sim/hud/frame-stat-type", "tris");

    // Time options
    fgSetInt("/sim/startup/time-offset", 0);
    fgSetString("/sim/startup/time-offset-type", "system-offset");
    fgSetLong("/sim/time/cur-time-override", 0);

    // Freeze options
    fgSetBool("/sim/freeze/master", false);
    fgSetBool("/sim/freeze/position", false);
    fgSetBool("/sim/freeze/clock", false);
    fgSetBool("/sim/freeze/fuel", false);

    fgSetString("/sim/multiplay/callsign", "callsign");
    fgSetString("/sim/multiplay/rxhost", "");
    fgSetString("/sim/multiplay/txhost", "");
    fgSetInt("/sim/multiplay/rxport", 0);
    fgSetInt("/sim/multiplay/txport", 0);

    SGPropertyNode* v = globals->get_props()->getNode("/sim/version", true);
    v->setValueReadOnly("flightgear", FLIGHTGEAR_VERSION);
    v->setValueReadOnly("simgear", SG_STRINGIZE(SIMGEAR_VERSION));
    v->setValueReadOnly("openscenegraph", osgGetVersion());
#if OSG_VERSION_LESS_THAN(3,5,2)
    v->setValueReadOnly("openscenegraph-thread-safe-reference-counting",
                         osg::Referenced::getThreadSafeReferenceCounting());
#endif
    v->setValueReadOnly("revision", REVISION);
    v->setValueReadOnly("build-number", JENKINS_BUILD_NUMBER);
    v->setValueReadOnly("build-id", JENKINS_BUILD_ID);
    v->setValueReadOnly("hla-support", bool(FG_HAVE_HLA));
    v->setValueReadOnly("build-type", FG_BUILD_TYPE);

    char* envp = ::getenv( "http_proxy" );
    if( envp != nullptr )
      fgSetupProxy( envp );
}

///////////////////////////////////////////////////////////////////////////////
// helper object to implement the --show-aircraft command.
// resides here so we can share the fgFindAircraftInDir template above,
// and hence ensure this command lists exectly the same aircraft as the normal
// loading path.
class ShowAircraft : public AircraftDirVistorBase
{
public:
    ShowAircraft()
    {
        _minStatus = getNumMaturity(fgGetString("/sim/aircraft-min-status", "all"));
    }


    void show(const vector<SGPath> & path_list)
    {
		for (vector<SGPath>::const_iterator p = path_list.begin();
			 p != path_list.end(); ++p)
			visitDir(*p, 0);

        simgear::requestConsole(); // ensure console is shown on Windows

        std::sort(_aircraft.begin(), _aircraft.end(),
                  [](const std::string& lhs, const std::string& rhs) {
                      return strcasecmp(lhs.c_str(), rhs.c_str()) < 0 ? 1 : 0;
                  });

        cout << "Available aircraft:" << endl;
        for ( unsigned int i = 0; i < _aircraft.size(); i++ ) {
            cout << _aircraft[i] << endl;
        }
    }

private:
    virtual VisitResult visit(const SGPath& path)
    {
        SGPropertyNode root;
        try {
            readProperties(path, &root);
        } catch (sg_exception& ) {
            return VISIT_CONTINUE;
        }

        int maturity = 0;
        string descStr("   ");
        descStr += path.file();
        // trim common suffix from file names
        int nPos = descStr.rfind("-set.xml");
        if (nPos == (int)(descStr.size() - 8)) {
            descStr.resize(nPos);
        }

        SGPropertyNode *node = root.getNode("sim");
        if (node) {
            SGPropertyNode* desc = node->getNode("description");
            // if a status tag is found, read it in
            if (node->hasValue("status")) {
                maturity = getNumMaturity(node->getStringValue("status"));
            }

            if (desc) {
                if (descStr.size() <= 27+3) {
                    descStr.append(29+3-descStr.size(), ' ');
                } else {
                    descStr += '\n';
                    descStr.append( 32, ' ');
                }
                descStr += desc->getStringValue();
            }
        } // of have 'sim' node

        if (maturity >= _minStatus) {
            _aircraft.push_back(descStr);
        }

        return VISIT_CONTINUE;
    }


    int getNumMaturity(const char * str)
    {
        // Changes should also be reflected in $FG_ROOT/options.xml
        const char* levels[] = {"alpha","beta","early-production","production"};

        if (!strcmp(str, "all")) {
            return 0;
        }

        for (size_t i=0; i<(sizeof(levels)/sizeof(levels[0]));i++)
            if (strcmp(str,levels[i])==0)
                return i;

        return 0;
    }

    int _minStatus;
    string_list _aircraft;
};

/*
 * Search in the current directory, and in on directory deeper
 * for <aircraft>-set.xml configuration files and show the aircaft name
 * and the contents of the<description> tag in a sorted manner.
 *
 * @parampath the directory to search for configuration files
 */
void fgShowAircraft(const vector<SGPath> &path_list)
{
    ShowAircraft s;
    s.show(path_list);

#ifdef _MSC_VER
    cout << "Hit a key to continue..." << endl;
    std::cin.get();
#endif
}


static bool
parse_wind (const string &wind, double * min_hdg, double * max_hdg,
	    double * speed, double * gust)
{
  string::size_type pos = wind.find('@');
  if (pos == string::npos)
    return false;
  string dir = wind.substr(0, pos);
  string spd = wind.substr(pos+1);
  pos = dir.find(':');
  if (pos == string::npos) {
    *min_hdg = *max_hdg = atof(dir.c_str());
  } else {
    *min_hdg = atof(dir.substr(0,pos).c_str());
    *max_hdg = atof(dir.substr(pos+1).c_str());
  }
  pos = spd.find(':');
  if (pos == string::npos) {
    *speed = *gust = atof(spd.c_str());
  } else {
    *speed = atof(spd.substr(0,pos).c_str());
    *gust = atof(spd.substr(pos+1).c_str());
  }
  return true;
}

static bool
parseIntValue(char** ppParserPos, int* pValue,int min, int max, const char* field, const char* argument)
{
    if ( !strlen(*ppParserPos) )
        return true;

    char num[256];
    int i = 0;

    while ( isdigit((*ppParserPos)[0]) && (i<255) )
    {
        num[i] = (*ppParserPos)[0];
        (*ppParserPos)++;
        i++;
    }
    num[i] = '\0';

    switch ((*ppParserPos)[0])
    {
        case 0:
            break;
        case ':':
            (*ppParserPos)++;
            break;
        default:
            SG_LOG(SG_GENERAL, SG_ALERT, "Illegal character in time string for " << field << ": '" <<
                    (*ppParserPos)[0] << "'.");
            // invalid field - skip rest of string to avoid further errors
            while ((*ppParserPos)[0])
                (*ppParserPos)++;
            return false;
    }

    if (i<=0)
        return true;

    int value = atoi(num);
    if ((value < min)||(value > max))
    {
        SG_LOG(SG_GENERAL, SG_ALERT, "Invalid " << field << " in '" << argument <<
               "'. Valid range is " << min << "-" << max << ".");
        return false;
    }
    else
    {
        *pValue = value;
        return true;
    }
}

// parse a time string ([+/-]%f[:%f[:%f]]) into hours
static double
parse_time(const string& time_in) {
    char *time_str, num[256];
    double hours, minutes, seconds;
    double result = 0.0;
    int sign = 1;
    int i;

    time_str = (char *)time_in.c_str();

    // printf("parse_time(): %s\n", time_str);

    // check for sign
    if ( strlen(time_str) ) {
	if ( time_str[0] == '+' ) {
	    sign = 1;
	    time_str++;
	} else if ( time_str[0] == '-' ) {
	    sign = -1;
	    time_str++;
	}
    }
    // printf("sign = %d\n", sign);

    // get hours
    if ( strlen(time_str) ) {
	i = 0;
	while ( (time_str[0] != ':') && (time_str[0] != '\0') ) {
	    num[i] = time_str[0];
	    time_str++;
	    i++;
	}
	if ( time_str[0] == ':' ) {
	    time_str++;
	}
	num[i] = '\0';
	hours = atof(num);
	// printf("hours = %.2lf\n", hours);

	result += hours;
    }

    // get minutes
    if ( strlen(time_str) ) {
	i = 0;
	while ( (time_str[0] != ':') && (time_str[0] != '\0') ) {
	    num[i] = time_str[0];
	    time_str++;
	    i++;
	}
	if ( time_str[0] == ':' ) {
	    time_str++;
	}
	num[i] = '\0';
	minutes = atof(num);
	// printf("minutes = %.2lf\n", minutes);

	result += minutes / 60.0;
    }

    // get seconds
    if ( strlen(time_str) ) {
	i = 0;
	while ( (time_str[0] != ':') && (time_str[0] != '\0') ) {
	    num[i] = time_str[0];
	    time_str++;
	    i++;
	}
	num[i] = '\0';
	seconds = atof(num);
	// printf("seconds = %.2lf\n", seconds);

	result += seconds / 3600.0;
    }

    SG_LOG( SG_GENERAL, SG_INFO, " parse_time() = " << sign * result );

    return(sign * result);
}

// parse a date string (yyyy:mm:dd:hh:mm:ss) into a time_t (seconds)
static long int
parse_date( const string& date, const char* timeType)
{
    struct tm gmt,*pCurrentTime;
    int year,month,day,hour,minute,second;
    char *argument, *date_str;

    SGTime CurrentTime;
    CurrentTime.update(SGGeod(),0,0);

    // FIXME This should obtain system/aircraft/GMT time depending on timeType
    pCurrentTime = CurrentTime.getGmt();

    // initialize all fields with current time
    year   = pCurrentTime->tm_year + 1900;
    month  = pCurrentTime->tm_mon + 1;
    day    = pCurrentTime->tm_mday;
    hour   = pCurrentTime->tm_hour;
    minute = pCurrentTime->tm_min;
    second = pCurrentTime->tm_sec;

    argument = (char *)date.c_str();
    date_str = argument;

    // start with parsing year
    if (!strlen(date_str) ||
        !parseIntValue(&date_str,&year,0,9999,"year",argument))
    {
        return -1;
    }

    if (year < 1970)
    {
        SG_LOG(SG_GENERAL, SG_ALERT, "Invalid year '" << year << "'. Use 1970 or later.");
        return -1;
    }

    parseIntValue(&date_str, &month,  1, 12, "month",  argument);
    parseIntValue(&date_str, &day,    1, 31, "day",    argument);
    parseIntValue(&date_str, &hour,   0, 23, "hour",   argument);
    parseIntValue(&date_str, &minute, 0, 59, "minute", argument);
    parseIntValue(&date_str, &second, 0, 59, "second", argument);

    gmt.tm_sec  = second;
    gmt.tm_min  = minute;
    gmt.tm_hour = hour;
    gmt.tm_mday = day;
    gmt.tm_mon  = month - 1;
    gmt.tm_year = year -1900;
    gmt.tm_isdst = 0; // ignore daylight savings time for the moment

    time_t theTime = sgTimeGetGMT( gmt.tm_year, gmt.tm_mon, gmt.tm_mday,
                                   gmt.tm_hour, gmt.tm_min, gmt.tm_sec );

    SG_LOG(SG_GENERAL, SG_INFO, "Configuring startup time to " << ctime(&theTime));

    return (theTime);
}


// parse angle in the form of [+/-]ddd:mm:ss into degrees
static double
parse_degree( const string& degree_str) {
    double result = parse_time( degree_str );

    // printf("Degree = %.4f\n", result);

    return(result);
}


// parse time offset string into seconds
static long int
parse_time_offset( const string& time_str) {
   long int result;

    // printf("time offset = %s\n", time_str);

#ifdef HAVE_RINT
    result = (int)rint(parse_time(time_str) * 3600.0);
#else
    result = (int)(parse_time(time_str) * 3600.0);
#endif

    // printf("parse_time_offset(): %d\n", result);

    return( result );
}


// Parse --fov=x.xx type option
static double
parse_fov( const string& arg ) {
    double fov = atof(arg);

    if ( fov < FG_FOV_MIN ) { fov = FG_FOV_MIN; }
    if ( fov > FG_FOV_MAX ) { fov = FG_FOV_MAX; }

    fgSetDouble("/sim/view[0]/config/default-field-of-view-deg", fov);

    // printf("parse_fov(): result = %.4f\n", fov);

    return fov;
}


// Parse I/O channel option
//
// Format is "--protocol=medium,direction,hz,medium_options,..."
//
//   protocol = { native, nmea, garmin, AV400, AV400Sim, fgfs, rul, pve, etc. }
//   medium = { serial, socket, file, etc. }
//   direction = { in, out, bi }
//   hz = number of times to process channel per second (floating
//        point values are ok.
//
// Serial example "--nmea=serial,dir,hz,device,baud" where
//
//  device = OS device name of serial line to be open()'ed
//  baud = {300, 1200, 2400, ..., 230400}
//
// Socket example "--native=socket,dir,hz,machine,port,style" where
//
//  machine = machine name or ip address if client (leave empty if server)
//  port = port, leave empty to let system choose
//  style = tcp or udp
//
// File example "--garmin=file,dir,hz,filename" where
//
//  filename = file system file name

static bool
add_channel( const string& type, const string& channel_str ) {
    // This check is necessary to prevent fgviewer from segfaulting when given
    // weird options. (It doesn't run the full initialization)
    if(!globals->get_channel_options_list())
    {
        SG_LOG(SG_GENERAL, SG_ALERT, "Option " << type << "=" << channel_str
                                     << " ignored.");
        return false;
    }
    SG_LOG(SG_GENERAL, SG_INFO, "Channel string = " << channel_str );
    globals->get_channel_options_list()->push_back( type + "," + channel_str );
    return true;
}

static void
clearLocation ()
{
    fgSetString("/sim/presets/airport-id", "");
    fgSetString("/sim/presets/vor-id", "");
    fgSetString("/sim/presets/ndb-id", "");
    fgSetString("/sim/presets/carrier", "");
    fgSetString("/sim/presets/parkpos", "");
    fgSetString("/sim/presets/carrier-position", "");
    fgSetString("/sim/presets/fix", "");
}

/*
 Using --addon=/foo/bar does:
   - register the add-on with the AddonManager (enabling, among other things,
     add-on-specific resources for simgear::ResourceManager);
   - load /foo/bar/addon-config.xml into the Global Property Tree;
   - add /foo/bar to the list of aircraft paths to provide read access:
   - set various properties related to the add-on under /addons;
   - load /foo/bar/addon-main.nas into namespace __addon[ADDON_ID]__
     (see $FG_ROOT/Nasal/addons.nas);
   - call the main() function defined in that file.

 For more details, see $FG_ROOT/Docs/README.add-ons.
*/
static int
fgOptAddon(const char *arg)
{
  const SGPath addonPath = SGPath::fromUtf8(arg);
  const auto& addonManager = addons::AddonManager::instance();

  try {
    addonManager->registerAddon(addonPath);
  } catch (const sg_exception &e) {
    string msg = "Error registering an add-on: " + e.getFormattedMessage();
    SG_LOG(SG_GENERAL, SG_ALERT, msg);
    flightgear::fatalMessageBoxThenExit(
      "FlightGear", "Unable to register an add-on.", msg);
  }

  return FG_OPTIONS_OK;
}

static int
fgOptAdditionalDataDir(const char* arg)
{
    const SGPath dataPath = SGPath::fromUtf8(arg);
    if (!dataPath.exists()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "--data path not found:'" << dataPath << "'");
        flightgear::fatalMessageBoxWithoutExit(
            "FlightGear", "Data path not found: '" + dataPath.utf8Str() + "'.");
        return FG_OPTIONS_EXIT;
    }

    globals->append_data_path(dataPath, false /* = before FG_ROOT */);
    return FG_OPTIONS_OK;
}

static int
fgOptVOR( const char * arg )
{
    clearLocation();
    fgSetString("/sim/presets/vor-id", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptNDB( const char * arg )
{
    clearLocation();
    fgSetString("/sim/presets/ndb-id", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptCarrier( const char * arg )
{
    clearLocation();
    fgSetString("/sim/presets/carrier", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptCarrierPos( const char * arg )
{
    fgSetString("/sim/presets/carrier-position", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptParkpos( const char * arg )
{
    fgSetString("/sim/presets/parkpos", arg);
    fgSetBool("/sim/presets/parking-requested", true);
    return FG_OPTIONS_OK;
}

static int
fgOptFIX( const char * arg )
{
    clearLocation();
    fgSetString("/sim/presets/fix", arg);
    return FG_OPTIONS_OK;
}

static int
fgOptLon( const char *arg )
{
    clearLocation();
    fgSetDouble("/sim/presets/longitude-deg", parse_degree( arg ));
    fgSetDouble("/position/longitude-deg", parse_degree( arg ));
    return FG_OPTIONS_OK;
}

static int
fgOptLat( const char *arg )
{
    clearLocation();
    fgSetDouble("/sim/presets/latitude-deg", parse_degree( arg ));
    fgSetDouble("/position/latitude-deg", parse_degree( arg ));
    return FG_OPTIONS_OK;
}

static int
fgOptAltitude( const char *arg )
{
    fgSetBool("/sim/presets/onground", false);
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
        fgSetDouble("/sim/presets/altitude-ft", atof( arg ));
    else
        fgSetDouble("/sim/presets/altitude-ft",
		    atof( arg ) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptUBody( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "UVW");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/uBody-fps", atof( arg ));
    else
	fgSetDouble("/sim/presets/uBody-fps",
                    atof( arg ) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVBody( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "UVW");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/vBody-fps", atof( arg ));
    else
	fgSetDouble("/sim/presets/vBody-fps",
			    atof( arg ) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptWBody( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "UVW");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/wBody-fps", atof(arg));
    else
	fgSetDouble("/sim/presets/wBody-fps",
			    atof(arg) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVNorth( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "NED");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/speed-north-fps", atof( arg ));
    else
	fgSetDouble("/sim/presets/speed-north-fps",
			    atof( arg ) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVEast( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "NED");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/speed-east-fps", atof(arg));
    else
	fgSetDouble("/sim/presets/speed-east-fps",
		    atof(arg) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVDown( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "NED");
    if ( !strcmp(fgGetString("/sim/startup/units"), "feet") )
	fgSetDouble("/sim/presets/speed-down-fps", atof(arg));
    else
	fgSetDouble("/sim/presets/speed-down-fps",
			    atof(arg) * SG_METER_TO_FEET);
    return FG_OPTIONS_OK;
}

static int
fgOptVc( const char *arg )
{
    // fgSetString("/sim/presets/speed-set", "knots");
    // fgSetDouble("/velocities/airspeed-kt", atof(arg.substr(5)));
    fgSetString("/sim/presets/speed-set", "knots");
    fgSetDouble("/sim/presets/airspeed-kt", atof(arg));
    return FG_OPTIONS_OK;
}

static int
fgOptMach( const char *arg )
{
    fgSetString("/sim/presets/speed-set", "mach");
    fgSetDouble("/sim/presets/mach", atof(arg));
    return FG_OPTIONS_OK;
}

static int
fgOptRoc( const char *arg )
{
    fgSetDouble("/sim/presets/vertical-speed-fps", atof(arg)/60);
    return FG_OPTIONS_OK;
}

static int
fgOptFgScenery( const char *arg )
{
    globals->append_fg_scenery(SGPath::pathsFromUtf8(arg));
    return FG_OPTIONS_OK;
}

static int
fgOptEnhancedLighting( const char *arg )
{
    SG_LOG(SG_ALL,SG_ALERT,
           "the options --enable-enhanced-lighting and "
           "--disable-enhanced-lighting are no longer supported and will be "
           "removed in a future FlightGear version! Please do not use them");
    return FG_OPTIONS_EXIT;
}

static int
fgOptAllowNasalRead( const char *arg )
{
    PathList paths = SGPath::pathsFromUtf8(arg);
    if(paths.size() == 0) {
        SG_LOG(SG_GENERAL, SG_WARN, "--allow-nasal-read requires a list of directories to allow");
    }
    for( PathList::const_iterator it = paths.begin(); it != paths.end(); ++it ) {
        globals->append_read_allowed_paths(*it);
    }
    return FG_OPTIONS_OK;
}

static int
fgOptFov( const char *arg )
{
    parse_fov( arg );
    return FG_OPTIONS_OK;
}

static int
fgOptGeometry( const char *arg )
{
    bool geometry_ok = true;
    int xsize = 0, ysize = 0;
    string geometry = arg;
    string::size_type i = geometry.find('x');

    if (i != string::npos) {
        xsize = atoi(geometry.substr(0, i));
        ysize = atoi(geometry.substr(i+1));
    } else {
        geometry_ok = false;
    }

    if ( xsize <= 0 || ysize <= 0 ) {
        xsize = 640;
        ysize = 480;
        geometry_ok = false;
    }

    if ( !geometry_ok ) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Unknown geometry: " << geometry );
        SG_LOG( SG_GENERAL, SG_ALERT,
 	        "Setting geometry to " << xsize << 'x' << ysize << '\n');
    } else {
        SG_LOG( SG_GENERAL, SG_INFO,
	        "Setting geometry to " << xsize << 'x' << ysize << '\n');
        fgSetInt("/sim/startup/xsize", xsize);
        fgSetInt("/sim/startup/ysize", ysize);
    }
    return FG_OPTIONS_OK;
}

static int
fgOptBpp( const char *arg )
{
    string bits_per_pix = arg;
    if ( bits_per_pix == "16" ) {
	fgSetInt("/sim/rendering/bits-per-pixel", 16);
    } else if ( bits_per_pix == "24" ) {
	fgSetInt("/sim/rendering/bits-per-pixel", 24);
    } else if ( bits_per_pix == "32" ) {
	fgSetInt("/sim/rendering/bits-per-pixel", 32);
    } else {
	SG_LOG(SG_GENERAL, SG_ALERT, "Unsupported bpp " << bits_per_pix);
    }
    return FG_OPTIONS_OK;
}

static int
fgOptTimeOffset( const char *arg )
{
    fgSetLong("/sim/startup/time-offset",
                parse_time_offset( arg ));
    fgSetString("/sim/startup/time-offset-type", "system-offset");
    return FG_OPTIONS_OK;
}

static int
fgOptStartDateSys( const char *arg )
{
    long int theTime = parse_date( arg, "system" );
    if (theTime>=0)
    {
        fgSetLong("/sim/startup/time-offset",  theTime);
        fgSetString("/sim/startup/time-offset-type", "system");
    }
    return FG_OPTIONS_OK;
}

static int
fgOptStartDateLat( const char *arg )
{
    long int theTime = parse_date( arg, "latitude" );
    if (theTime>=0)
    {
        fgSetLong("/sim/startup/time-offset", theTime);
        fgSetString("/sim/startup/time-offset-type", "latitude");
    }
    return FG_OPTIONS_OK;
}

static int
fgOptStartDateGmt( const char *arg )
{
    long int theTime = parse_date( arg, "gmt" );
    if (theTime>=0)
    {
        fgSetLong("/sim/startup/time-offset", theTime);
        fgSetString("/sim/startup/time-offset-type", "gmt");
    }
    return FG_OPTIONS_OK;
}

static int
fgOptJpgHttpd( const char * arg )
{
  SG_LOG(SG_ALL,SG_ALERT,
   "the option --jpg-httpd is no longer supported! Please use --httpd instead."
   " URL for the screenshot within the new httpd is http://YourFgServer:xxxx/screenshot");
  return FG_OPTIONS_EXIT;
}

static int
fgOptHttpd( const char * arg )
{
    // port may be any valid address:port notation
    // like 127.0.0.1:8080
    // or just the port 8080
    string port = simgear::strutils::strip(string(arg));
    if( port.empty() ) return FG_OPTIONS_ERROR;
    fgSetString( string(flightgear::http::PROPERTY_ROOT).append("/options/listening-port").c_str(), port );
    return FG_OPTIONS_OK;
}

static int
fgSetupProxy( const char *arg )
{
    string options = simgear::strutils::strip( arg );
    string host, port, auth;
    string::size_type pos;

    // this is NURLP - NURLP is not an url parser
    if( simgear::strutils::starts_with( options, "http://" ) )
        options = options.substr( 7 );
    if( simgear::strutils::ends_with( options, "/" ) )
        options = options.substr( 0, options.length() - 1 );

    host = port = auth = "";
    if ((pos = options.find("@")) != string::npos)
        auth = options.substr(0, pos++);
    else
        pos = 0;

    host = options.substr(pos, options.size());
    if ((pos = host.find(":")) != string::npos) {
        port = host.substr(++pos, host.size());
        host.erase(--pos, host.size());
    }

    fgSetString("/sim/presets/proxy/host", host.c_str());
    fgSetString("/sim/presets/proxy/port", port.c_str());
    fgSetString("/sim/presets/proxy/authentication", auth.c_str());

    return FG_OPTIONS_OK;
}

static int
fgOptTraceRead( const char *arg )
{
    string name = arg;
    SG_LOG(SG_GENERAL, SG_INFO, "Tracing reads for property " << name);
    fgGetNode(name.c_str(), true)
	->setAttribute(SGPropertyNode::TRACE_READ, true);
    return FG_OPTIONS_OK;
}

static int
fgOptLogLevel( const char *arg )
{
    fgSetString("/sim/logging/priority", arg);
    setLoggingPriority(arg);

    return FG_OPTIONS_OK;
}

static int
fgOptLogClasses( const char *arg )
{
    fgSetString("/sim/logging/classes", arg);
    setLoggingClasses (arg);

    return FG_OPTIONS_OK;
}

static int
fgOptLogDir(const char* arg)
{
    SGPath dirPath;
    if (!strcmp(arg, "desktop")) {
        dirPath = SGPath::desktop();
    } else {
        dirPath = SGPath::fromUtf8(arg);
    }

    if (!dirPath.isDir()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "cannot find logging location " << dirPath);
        return FG_OPTIONS_ERROR;
    }

    if (!dirPath.canWrite()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "cannot write to logging location " << dirPath);
        return FG_OPTIONS_ERROR;
    }

    // generate the log file name
    SGPath logFile;
    {
        char fileNameBuffer[100];
        time_t now;
        time(&now);
        strftime(fileNameBuffer, 99, "FlightGear_%F", localtime(&now));

        unsigned int logsTodayCount = 0;
        while (true) {
            std::ostringstream os;
            os << fileNameBuffer << "_" << logsTodayCount++ << ".log";
            logFile = dirPath / os.str();
            if (!logFile.exists()) {
                break;
            }
        }
    }

    sglog().logToFile(logFile, sglog().get_log_classes(), sglog().get_log_priority());

    return FG_OPTIONS_OK;
}

static int
fgOptTraceWrite( const char *arg )
{
    string name = arg;
    SG_LOG(SG_GENERAL, SG_INFO, "Tracing writes for property " << name);
    fgGetNode(name.c_str(), true)
	->setAttribute(SGPropertyNode::TRACE_WRITE, true);
    return FG_OPTIONS_OK;
}

static int
fgOptViewOffset( const char *arg )
{
    // $$$ begin - added VS Renganathan, 14 Oct 2K
    // for multi-window outside window imagery
    string woffset = arg;
    double default_view_offset = 0.0;
    if ( woffset == "LEFT" ) {
	    default_view_offset = SGD_PI * 0.25;
    } else if ( woffset == "RIGHT" ) {
	default_view_offset = SGD_PI * 1.75;
    } else if ( woffset == "CENTER" ) {
	default_view_offset = 0.00;
    } else {
	default_view_offset = atof( woffset.c_str() ) * SGD_DEGREES_TO_RADIANS;
    }
    /* apparently not used (CLO, 11 Jun 2002)
        FGViewer *pilot_view =
	    (FGViewer *)globals->get_viewmgr()->get_view( 0 ); */
    // this will work without calls to the viewer...
    fgSetDouble( "/sim/current-view/heading-offset-deg",
                    default_view_offset  * SGD_RADIANS_TO_DEGREES );
    // $$$ end - added VS Renganathan, 14 Oct 2K
    return FG_OPTIONS_OK;
}

static int
fgOptVisibilityMeters( const char *arg )
{
    Environment::Presets::VisibilitySingleton::instance()->preset( atof( arg ) );
    return FG_OPTIONS_OK;
}

static int
fgOptVisibilityMiles( const char *arg )
{
    Environment::Presets::VisibilitySingleton::instance()->preset( atof( arg ) * 5280.0 * SG_FEET_TO_METER );
    return FG_OPTIONS_OK;
}

static int
fgOptMetar(const char *arg)
{
    // The given METAR string cannot be effective without disabling
    // real weather fetching.
    fgSetBool("/environment/realwx/enabled", false);
    // The user-supplied METAR string
    fgSetString("/environment/metar/data", arg);

    return FG_OPTIONS_OK;
}
static int
fgOptConsole(const char *arg)
{
    static bool already_done = false;
    if (!already_done)
    {
        already_done = true;
        simgear::requestConsole();
    }
    return FG_OPTIONS_OK;
}

static int
fgOptRandomWind( const char *arg )
{
    double min_hdg = sg_random() * 360.0;
    double max_hdg = min_hdg + (20 - sqrt(sg_random() * 400));
    double speed = sg_random() * sg_random() * 40;
    double gust = speed + (10 - sqrt(sg_random() * 100));
    Environment::Presets::WindSingleton::instance()->preset(min_hdg, max_hdg, speed, gust);
    return FG_OPTIONS_OK;
}

static int
fgOptWind( const char *arg )
{
    double min_hdg = 0.0, max_hdg = 0.0, speed = 0.0, gust = 0.0;
    if (!parse_wind( arg, &min_hdg, &max_hdg, &speed, &gust)) {
	SG_LOG( SG_GENERAL, SG_ALERT, "bad wind value " << arg );
	return FG_OPTIONS_ERROR;
    }
    Environment::Presets::WindSingleton::instance()->preset(min_hdg, max_hdg, speed, gust);
    return FG_OPTIONS_OK;
}

static int
fgOptTurbulence( const char *arg )
{
    Environment::Presets::TurbulenceSingleton::instance()->preset( atof(arg) );
    return FG_OPTIONS_OK;
}

static int
fgOptCeiling( const char *arg )
{
    double elevation, thickness;
    string spec = arg;
    string::size_type pos = spec.find(':');
    if (pos == string::npos) {
        elevation = atof(spec.c_str());
        thickness = 2000;
    } else {
        elevation = atof(spec.substr(0, pos).c_str());
        thickness = atof(spec.substr(pos + 1).c_str());
    }
    Environment::Presets::CeilingSingleton::instance()->preset( elevation, thickness );
    return FG_OPTIONS_OK;
}

static int
fgOptWp( const char *arg )
{
    string_list *waypoints = globals->get_initial_waypoints();
    if (!waypoints) {
        waypoints = new string_list;
        globals->set_initial_waypoints(waypoints);
    }
    waypoints->push_back(arg);
    return FG_OPTIONS_OK;
}

static bool
parse_colon (const string &s, double * val1, double * val2)
{
    string::size_type pos = s.find(':');
    if (pos == string::npos) {
        *val2 = atof(s);
        return false;
    } else {
        *val1 = atof(s.substr(0, pos).c_str());
        *val2 = atof(s.substr(pos+1).c_str());
        return true;
    }
}


static int
fgOptFailure( const char * arg )
{
    string a = arg;
    if (a == "pitot") {
        fgSetBool("/systems/pitot/serviceable", false);
    } else if (a == "static") {
        fgSetBool("/systems/static/serviceable", false);
    } else if (a == "vacuum") {
        fgSetBool("/systems/vacuum/serviceable", false);
    } else if (a == "electrical") {
        fgSetBool("/systems/electrical/serviceable", false);
    } else {
        SG_LOG(SG_INPUT, SG_ALERT, "Unknown failure mode: " << a);
        return FG_OPTIONS_ERROR;
    }

    return FG_OPTIONS_OK;
}


static int
fgOptNAV1( const char * arg )
{
    double radial, freq;
    if (parse_colon(arg, &radial, &freq))
        fgSetDouble("/instrumentation/nav[0]/radials/selected-deg", radial);
    fgSetDouble("/instrumentation/nav[0]/frequencies/selected-mhz", freq);
    return FG_OPTIONS_OK;
}

static int
fgOptNAV2( const char * arg )
{
    double radial, freq;
    if (parse_colon(arg, &radial, &freq))
        fgSetDouble("/instrumentation/nav[1]/radials/selected-deg", radial);
    fgSetDouble("/instrumentation/nav[1]/frequencies/selected-mhz", freq);
    return FG_OPTIONS_OK;
}

static int
fgOptADF( const char * arg )
{
    SG_LOG(SG_ALL,SG_ALERT,
           "the option --adf is no longer supported! Please use --adf1 "
           "instead.");
    return FG_OPTIONS_EXIT;
}

static int
fgOptADF1( const char * arg )
{
    double rot, freq;
    if (parse_colon(arg, &rot, &freq))
        fgSetDouble("/instrumentation/adf[0]/rotation-deg", rot);
    fgSetDouble("/instrumentation/adf[0]/frequencies/selected-khz", freq);
    return FG_OPTIONS_OK;
}

static int
fgOptADF2( const char * arg )
{
    double rot, freq;
    if (parse_colon(arg, &rot, &freq))
        fgSetDouble("/instrumentation/adf[1]/rotation-deg", rot);
    fgSetDouble("/instrumentation/adf[1]/frequencies/selected-khz", freq);
    return FG_OPTIONS_OK;
}

static int
fgOptDME( const char *arg )
{
    string opt = arg;
    if (opt == "nav1") {
        fgSetInt("/instrumentation/dme/switch-position", 1);
        fgSetString("/instrumentation/dme/frequencies/source",
                    "/instrumentation/nav[0]/frequencies/selected-mhz");
    } else if (opt == "nav2") {
        fgSetInt("/instrumentation/dme/switch-position", 3);
        fgSetString("/instrumentation/dme/frequencies/source",
                    "/instrumentation/nav[1]/frequencies/selected-mhz");
    } else {
        double frequency = atof(arg);
        if (frequency==0.0)
        {
            SG_LOG(SG_INPUT, SG_ALERT, "Invalid DME frequency: '" << arg << "'.");
            return FG_OPTIONS_ERROR;
        }
        fgSetInt("/instrumentation/dme/switch-position", 2);
        fgSetString("/instrumentation/dme/frequencies/source",
                    "/instrumentation/dme/frequencies/selected-mhz");
        fgSetDouble("/instrumentation/dme/frequencies/selected-mhz", frequency);
    }
    return FG_OPTIONS_OK;
}

static int
fgOptLivery( const char *arg )
{
    string opt = arg;
    string livery_path = "livery/" + opt;
    fgSetString("/sim/model/texture-path", livery_path.c_str() );
    return FG_OPTIONS_OK;
}

static int
fgOptScenario( const char *arg )
{
    SGPath path(arg);
    std::string name(arg);
    if (path.exists()) {
        if (path.isRelative()) {
            // make absolute
            path = simgear::Dir::current().path() / arg;
        }

        // create description node
        auto n = FGAIManager::registerScenarioFile(globals->get_props(), path);
        if (!n) {
            SG_LOG(SG_GENERAL, SG_WARN, "failed to read scenario file at:" << path);
            return FG_OPTIONS_ERROR;
        }

        // also set the /sim/ai/scenario entry so we load it on startup
        name = path.file_base();
    }

    // add the 'load it' node
    SGPropertyNode_ptr ai_node = fgGetNode( "/sim/ai", true );
    ai_node->addChild("scenario")->setStringValue(name);

    return FG_OPTIONS_OK;
}

static int
fgOptAirport( const char *arg )
{
    fgSetString("/sim/presets/airport-id", arg );
    fgSetBool("/sim/presets/airport-requested", true );
    return FG_OPTIONS_OK;
}

static int
fgOptRunway( const char *arg )
{
    fgSetString("/sim/presets/runway", arg );
    fgSetBool("/sim/presets/runway-requested", true );
    return FG_OPTIONS_OK;
}

static int
fgOptCallSign(const char * arg)
{
    int i;
    char callsign[11];
    strncpy(callsign,arg,10);
    callsign[10]=0;
    for (i=0;callsign[i];i++)
    {
        char c = callsign[i];
        if (c >= 'A' && c <= 'Z') continue;
        if (c >= 'a' && c <= 'z') continue;
        if (c >= '0' && c <= '9') continue;
        if (c == '-' || c == '_') continue;
        // convert any other illegal characters
        callsign[i]='-';
    }
    fgSetString("sim/multiplay/callsign", callsign );
    return FG_OPTIONS_OK;
}

static int
fgOptIgnoreAutosave(const char* arg)
{
    fgSetBool("/sim/startup/ignore-autosave", true);
    // don't overwrite autosave on exit
    fgSetBool("/sim/startup/save-on-exit", false);
    return FG_OPTIONS_OK;
}

static int
fgOptEnableFreeze(const char* arg)
{
    fgSetBool("/sim/freeze/master", true);
    fgSetBool("/sim/freeze/clock", true);
    return FG_OPTIONS_OK;
}

static int
fgOptDisableFreeze(const char* arg)
{
    fgSetBool("/sim/freeze/master", false);
    fgSetBool("/sim/freeze/clock", false);
    return FG_OPTIONS_OK;
}

// Set a property for the --prop: option. Syntax: --prop:[<type>:]<name>=<value>
// <type> can be "double" etc. but also only the first letter "d".
// Examples:  --prop:alpha=1  --prop:bool:beta=true  --prop:d:gamma=0.123
static int
fgOptSetProperty(const char* raw)
{
  string arg(raw);
  string::size_type pos = arg.find('=');
  if (pos == arg.npos || pos == 0 || pos + 1 == arg.size())
    return FG_OPTIONS_ERROR;

  string name = arg.substr(0, pos);
  string value = arg.substr(pos + 1);
  string type;
  pos = name.find(':');

  if (pos != name.npos && pos != 0 && pos + 1 != name.size()) {
    type = name.substr(0, pos);
    name = name.substr(pos + 1);
  }
  SGPropertyNode *n = fgGetNode(name.c_str(), true);

  bool writable = n->getAttribute(SGPropertyNode::WRITE);
  if (!writable)
    n->setAttribute(SGPropertyNode::WRITE, true);

  bool ret = false;
  if (type.empty())
    ret = n->setUnspecifiedValue(value.c_str());
  else if (type == "s" || type == "string")
    ret = n->setStringValue(value.c_str());
  else if (type == "d" || type == "double")
    ret = n->setDoubleValue(strtod(value.c_str(), 0));
  else if (type == "f" || type == "float")
    ret = n->setFloatValue(atof(value.c_str()));
  else if (type == "l" || type == "long")
    ret =  n->setLongValue(strtol(value.c_str(), 0, 0));
  else if (type == "i" || type == "int")
    ret =  n->setIntValue(atoi(value.c_str()));
  else if (type == "b" || type == "bool")
    ret =  n->setBoolValue(value == "true" || atoi(value.c_str()) != 0);

  if (!writable)
    n->setAttribute(SGPropertyNode::WRITE, false);
  return ret ? FG_OPTIONS_OK : FG_OPTIONS_ERROR;
}

static int
fgOptLoadTape(const char* arg)
{
  // load a flight recorder tape but wait until the fdm is initialized
  class DelayedTapeLoader : SGPropertyChangeListener {
  public:
    DelayedTapeLoader( const char * tape ) :
      _tape(SGPath::fromUtf8(tape))
    {
      SGPropertyNode_ptr n = fgGetNode("/sim/signals/fdm-initialized", true);
      n->addChangeListener( this );
    }

    virtual ~ DelayedTapeLoader() {}

    virtual void valueChanged(SGPropertyNode * node)
    {
      node->removeChangeListener( this );

      // tell the replay subsystem to load the tape
      FGReplay* replay = globals->get_subsystem<FGReplay>();
      SGPropertyNode_ptr arg = new SGPropertyNode();
      arg->setStringValue("tape", _tape.utf8Str() );
      arg->setBoolValue( "same-aircraft", 0 );
      if (!replay->loadTape(arg)) {
        // Force shutdown.
        SG_LOG(SG_GENERAL, SG_POPUP, "Exiting because unable to load fgtape: " << _tape.str());
        flightgear::modalMessageBox("Exiting because unable to load fgtape", _tape.str(), "");
        fgOSExit(1);
      }
      delete this; // commence suicide
    }
  private:
    SGPath _tape;

  };

  new DelayedTapeLoader(arg);
  return FG_OPTIONS_OK;
}

static int fgOptDisableGUI(const char*)
{
    globals->set_headless(true);
    return FG_OPTIONS_OK;
}

/*
   option       has_param type        property         b_param s_param  func

where:
 option    : name of the option
 has_param : option is --name=value if true or --name if false
 type      : OPTION_BOOL    - property is a boolean
             OPTION_STRING  - property is a string
             OPTION_DOUBLE  - property is a double
             OPTION_INT     - property is an integer
             OPTION_CHANNEL - name of option is the name of a channel
             OPTION_FUNC    - the option trigger a function
 property  :
 b_param   : if type==OPTION_BOOL,
             value set to the property (has_param is false for boolean)
 s_param   : if type==OPTION_STRING,
             value set to the property if has_param is false
 func      : function called if type==OPTION_FUNC. if has_param is true,
             the value is passed to the function as a string, otherwise,
             s_param is passed.

    For OPTION_DOUBLE and OPTION_INT, the parameter value is converted into a
    double or an integer and set to the property.

    For OPTION_CHANNEL, add_channel is called with the parameter value as the
    argument.
*/

enum OptionType { OPTION_BOOL = 0, OPTION_STRING, OPTION_DOUBLE, OPTION_INT, OPTION_CHANNEL, OPTION_FUNC, OPTION_IGNORE };
const int OPTION_MULTI = 1 << 17;

struct OptionDesc {
    const char *option;
    bool has_param;
    int type;
    const char *property;
    bool b_param;
    const char *s_param;
    int (*func)( const char * );
    } fgOptionArray[] = {

    {"language",                     true,  OPTION_IGNORE, "", false, "", 0 },
    {"console",                      false, OPTION_FUNC,   "", false, "", fgOptConsole },
    {"launcher",                     false, OPTION_IGNORE, "", false, "", 0 },
    {"enable-sentry",                false, OPTION_BOOL,   "/sim/startup/sentry-crash-reporting-enabled", true, "", nullptr },
    {"disable-sentry",               false, OPTION_BOOL,   "/sim/startup/sentry-crash-reporting-enabled", false, "", nullptr },
    {"allow-nasal-from-sockets",     false, OPTION_IGNORE, "", false, "", 0 },
    {"disable-rembrandt",            false, OPTION_BOOL,   "/sim/rendering/rembrandt/enabled", false, "", 0 },
    {"enable-rembrandt",             false, OPTION_BOOL,   "/sim/rendering/rembrandt/enabled", true, "", 0 },
    {"renderer",                     true,  OPTION_STRING, "/sim/rendering/rembrandt/renderer", false, "", 0 },
    {"compositor",                   true,  OPTION_STRING, "/sim/rendering/default-compositor", false, "", 0 },
    {"disable-splash-screen",        false, OPTION_BOOL,   "/sim/startup/splash-screen", false, "", 0 },
    {"enable-splash-screen",         false, OPTION_BOOL,   "/sim/startup/splash-screen", true, "", 0 },
    {"disable-mouse-pointer",        false, OPTION_STRING, "/sim/startup/mouse-pointer", false, "disabled", 0 },
    {"enable-mouse-pointer",         false, OPTION_STRING, "/sim/startup/mouse-pointer", false, "enabled", 0 },
    {"disable-random-objects",       false, OPTION_BOOL,   "/sim/rendering/random-objects", false, "", 0 },
    {"enable-random-objects",        false, OPTION_BOOL,   "/sim/rendering/random-objects", true, "", 0 },
    {"disable-random-vegetation",    false, OPTION_BOOL,   "/sim/rendering/random-vegetation", false, "", 0 },
    {"enable-random-vegetation",     false, OPTION_BOOL,   "/sim/rendering/random-vegetation", true, "", 0 },
    {"disable-random-buildings",     false, OPTION_BOOL,   "/sim/rendering/random-buildings", false, "", 0 },
    {"enable-random-buildings",      false, OPTION_BOOL,   "/sim/rendering/random-buildings", true, "", 0 },
    {"disable-real-weather-fetch",   false, OPTION_BOOL,   "/environment/realwx/enabled", false, "", 0 },
    {"enable-real-weather-fetch",    false, OPTION_BOOL,   "/environment/realwx/enabled", true,  "", 0 },
    {"metar",                        true,  OPTION_FUNC,   "", false, "", fgOptMetar },
    {"disable-ai-models",            false, OPTION_BOOL,   "/sim/ai/enabled", false, "", 0 },
    {"enable-ai-models",             false, OPTION_BOOL,   "/sim/ai/enabled", true, "", 0 },
    {"disable-ai-traffic",           false, OPTION_BOOL,   "/sim/traffic-manager/enabled", false, "", 0 },
    {"enable-ai-traffic",            false, OPTION_BOOL,   "/sim/traffic-manager/enabled", true,  "", 0 },
    {"disable-freeze",               false, OPTION_FUNC,   "", false, "", fgOptDisableFreeze },
    {"enable-freeze",                false, OPTION_FUNC,   "", true, "", fgOptEnableFreeze },
    {"disable-fuel-freeze",          false, OPTION_BOOL,   "/sim/freeze/fuel", false, "", 0 },
    {"enable-fuel-freeze",           false, OPTION_BOOL,   "/sim/freeze/fuel", true, "", 0 },
    {"disable-clock-freeze",         false, OPTION_BOOL,   "/sim/freeze/clock", false, "", 0 },
    {"enable-clock-freeze",          false, OPTION_BOOL,   "/sim/freeze/clock", true, "", 0 },
    {"disable-hud-3d",               false, OPTION_BOOL,   "/sim/hud/enable3d[1]", false, "", 0 },
    {"enable-hud-3d",                false, OPTION_BOOL,   "/sim/hud/enable3d[1]", true, "", 0 },
    {"disable-anti-alias-hud",       false, OPTION_BOOL,   "/sim/hud/color/antialiased", false, "", 0 },
    {"enable-anti-alias-hud",        false, OPTION_BOOL,   "/sim/hud/color/antialiased", true, "", 0 },
    {"disable-auto-coordination",    false, OPTION_BOOL,   "/controls/flight/auto-coordination", false, "", 0 },
    {"enable-auto-coordination",     false, OPTION_BOOL,   "/controls/flight/auto-coordination", true, "", 0 },
    {"browser-app",                  true,  OPTION_STRING, "/sim/startup/browser-app", false, "", 0 },
    {"disable-hud",                  false, OPTION_BOOL,   "/sim/hud/visibility[1]", false, "", 0 },
    {"enable-hud",                   false, OPTION_BOOL,   "/sim/hud/visibility[1]", true, "", 0 },
    {"disable-panel",                false, OPTION_BOOL,   "/sim/panel/visibility", false, "", 0 },
    {"enable-panel",                 false, OPTION_BOOL,   "/sim/panel/visibility", true, "", 0 },
    {"disable-sound",                false, OPTION_BOOL,   "/sim/sound/working", false, "", 0 },
    {"enable-sound",                 false, OPTION_BOOL,   "/sim/sound/working", true, "", 0 },
    {"sound-device",                 true,  OPTION_STRING, "/sim/sound/device-name", false, "", 0 },
    {"airport",                      true,  OPTION_FUNC,   "", false, "", fgOptAirport },
    {"runway",                       true,  OPTION_FUNC,   "", false, "", fgOptRunway },
    {"vor",                          true,  OPTION_FUNC,   "", false, "", fgOptVOR },
    {"vor-frequency",                true,  OPTION_DOUBLE, "/sim/presets/vor-freq", false, "", fgOptVOR },
    {"ndb",                          true,  OPTION_FUNC,   "", false, "", fgOptNDB },
    {"ndb-frequency",                true,  OPTION_DOUBLE, "/sim/presets/ndb-freq", false, "", fgOptVOR },
    {"carrier",                      true,  OPTION_FUNC,   "", false, "", fgOptCarrier },
    {"carrier-position",             true,  OPTION_FUNC,   "", false, "", fgOptCarrierPos },
    {"fix",                          true,  OPTION_FUNC,   "", false, "", fgOptFIX },
    {"offset-distance",              true,  OPTION_DOUBLE, "/sim/presets/offset-distance-nm", false, "", 0 },
    {"offset-azimuth",               true,  OPTION_DOUBLE, "/sim/presets/offset-azimuth-deg", false, "", 0 },
    {"lon",                          true,  OPTION_FUNC,   "", false, "", fgOptLon },
    {"lat",                          true,  OPTION_FUNC,   "", false, "", fgOptLat },
    {"altitude",                     true,  OPTION_FUNC,   "", false, "", fgOptAltitude },
    {"uBody",                        true,  OPTION_FUNC,   "", false, "", fgOptUBody },
    {"vBody",                        true,  OPTION_FUNC,   "", false, "", fgOptVBody },
    {"wBody",                        true,  OPTION_FUNC,   "", false, "", fgOptWBody },
    {"vNorth",                       true,  OPTION_FUNC,   "", false, "", fgOptVNorth },
    {"vEast",                        true,  OPTION_FUNC,   "", false, "", fgOptVEast },
    {"vDown",                        true,  OPTION_FUNC,   "", false, "", fgOptVDown },
    {"vc",                           true,  OPTION_FUNC,   "", false, "", fgOptVc },
    {"mach",                         true,  OPTION_FUNC,   "", false, "", fgOptMach },
    {"heading",                      true,  OPTION_DOUBLE, "/sim/presets/heading-deg", false, "", 0 },
    {"roll",                         true,  OPTION_DOUBLE, "/sim/presets/roll-deg", false, "", 0 },
    {"pitch",                        true,  OPTION_DOUBLE, "/sim/presets/pitch-deg", false, "", 0 },
    {"glideslope",                   true,  OPTION_DOUBLE, "/sim/presets/glideslope-deg", false, "", 0 },
    {"roc",                          true,  OPTION_FUNC,   "", false, "", fgOptRoc },
    {"fg-root",                      true,  OPTION_IGNORE, "", false, "", 0 },
    {"fg-scenery",                   true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptFgScenery },
    {"terrain-engine",               true,  OPTION_STRING, "/sim/scenery/engine", false, "tilecache", 0 },
    {"lod-levels",                   true,  OPTION_STRING, "/sim/scenery/lod-levels", false, "1 3 5 7", 0 },
    {"lod-res",                      true,  OPTION_STRING, "/sim/scenery/lod-res", false, "1", 0 },
    {"lod-texturing",                true,  OPTION_STRING, "/sim/scenery/lod-texturing", false, "bluemarble", 0 },
    {"lod-range-mult",               true,  OPTION_STRING, "/sim/scenery/lod-range-mult", false, "2", 0 },
    {"fg-aircraft",                  true,  OPTION_IGNORE | OPTION_MULTI,   "", false, "", 0 },
    {"fdm",                          true,  OPTION_STRING, "/sim/flight-model", false, "", 0 },
    {"aero",                         true,  OPTION_STRING, "/sim/aero", false, "", 0 },
    {"aircraft-dir",                 true,  OPTION_IGNORE, "", false, "", 0 },
    {"state",                        true,  OPTION_IGNORE, "", false, "", 0 },
    {"model-hz",                     true,  OPTION_INT,    "/sim/model-hz", false, "", 0 },
    {"max-fps",                      true,  OPTION_DOUBLE, "/sim/frame-rate-throttle-hz", false, "", 0 },
    {"speed",                        true,  OPTION_DOUBLE, "/sim/speed-up", false, "", 0 },
    {"trim",                         false, OPTION_BOOL,   "/sim/presets/trim", true, "", 0 },
    {"notrim",                       false, OPTION_BOOL,   "/sim/presets/trim", false, "", 0 },
    {"on-ground",                    false, OPTION_BOOL,   "/sim/presets/onground", true, "", 0 },
    {"in-air",                       false, OPTION_BOOL,   "/sim/presets/onground", false, "", 0 },
    {"disable-hold-short",           false, OPTION_BOOL,   "/sim/presets/mp-hold-short-override", true, "", 0 },
    {"fog-disable",                  false, OPTION_STRING, "/sim/rendering/fog", false, "disabled", 0 },
    {"fog-fastest",                  false, OPTION_STRING, "/sim/rendering/fog", false, "fastest", 0 },
    {"fog-nicest",                   false, OPTION_STRING, "/sim/rendering/fog", false, "nicest", 0 },
    {"disable-horizon-effect",       false, OPTION_BOOL,   "/sim/rendering/horizon-effect", false, "", 0 },
    {"enable-horizon-effect",        false, OPTION_BOOL,   "/sim/rendering/horizon-effect", true, "", 0 },
    {"disable-enhanced-lighting",    false, OPTION_FUNC,   "", false, "", fgOptEnhancedLighting },
    {"enable-enhanced-lighting",     false, OPTION_FUNC,   "", false, "", fgOptEnhancedLighting },
    {"disable-distance-attenuation", false, OPTION_BOOL,   "/sim/rendering/distance-attenuation", false, "", 0 },
    {"enable-distance-attenuation",  false, OPTION_BOOL,   "/sim/rendering/distance-attenuation", true, "", 0 },
    {"disable-specular-highlight",   false, OPTION_BOOL,   "/sim/rendering/specular-highlight", false, "", 0 },
    {"enable-specular-highlight",    false, OPTION_BOOL,   "/sim/rendering/specular-highlight", true, "", 0 },
    {"disable-clouds",               false, OPTION_BOOL,   "/environment/clouds/status", false, "", 0 },
    {"enable-clouds",                false, OPTION_BOOL,   "/environment/clouds/status", true, "", 0 },
    {"disable-clouds3d",             false, OPTION_BOOL,   "/sim/rendering/clouds3d-enable", false, "", 0 },
    {"enable-clouds3d",              false, OPTION_BOOL,   "/sim/rendering/clouds3d-enable", true, "", 0 },
    {"fov",                          true,  OPTION_FUNC,   "", false, "", fgOptFov },
    {"aspect-ratio-multiplier",      true,  OPTION_DOUBLE, "/sim/current-view/aspect-ratio-multiplier", false, "", 0 },
    {"disable-fullscreen",           false, OPTION_BOOL,   "/sim/startup/fullscreen", false, "", 0 },
    {"enable-fullscreen",            false, OPTION_BOOL,   "/sim/startup/fullscreen", true, "", 0 },
    {"disable-save-on-exit",         false, OPTION_BOOL,   "/sim/startup/save-on-exit", false, "", 0 },
    {"enable-save-on-exit",          false, OPTION_BOOL,   "/sim/startup/save-on-exit", true, "", 0 },
    {"read-only",                    false, OPTION_BOOL,   "/sim/fghome-readonly", true, "", 0 },
    {"ignore-autosave",              false, OPTION_FUNC,   "", false, "", fgOptIgnoreAutosave },
    {"restore-defaults",             false, OPTION_BOOL,   "/sim/startup/restore-defaults", true, "", 0 },
    {"shading-flat",                 false, OPTION_BOOL,   "/sim/rendering/shading", false, "", 0 },
    {"shading-smooth",               false, OPTION_BOOL,   "/sim/rendering/shading", true, "", 0 },
    {"texture-filtering",            false, OPTION_INT,    "/sim/rendering/filtering", 1, "", 0 },
    {"disable-wireframe",            false, OPTION_BOOL,   "/sim/rendering/wireframe", false, "", 0 },
    {"enable-wireframe",             false, OPTION_BOOL,   "/sim/rendering/wireframe", true, "", 0 },
    {"materials-file",               true,  OPTION_STRING, "/sim/rendering/materials-file", false, "", 0 },
    {"disable-terrasync",            false, OPTION_BOOL,   "/sim/terrasync/enabled", false, "", 0 },
    {"enable-terrasync",             false, OPTION_BOOL,   "/sim/terrasync/enabled", true, "", 0 },
    {"terrasync-dir",                true,  OPTION_IGNORE, "", false, "", 0 },
    {"download-dir",                 true,  OPTION_IGNORE, "", false, "", 0 },
    {"texture-cache-dir",            true,  OPTION_IGNORE, "", false, "", 0 },
    {"enable-texture-cache",         false, OPTION_BOOL,   "/sim/rendering/texture-cache/cache-enabled", true, "", 0 },
    {"disable-texture-cache",        false, OPTION_BOOL,   "/sim/rendering/texture-cache/cache-enabled", false, "", 0 },
    {"allow-nasal-read",             true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptAllowNasalRead },
    {"geometry",                     true,  OPTION_FUNC,   "", false, "", fgOptGeometry },
    {"bpp",                          true,  OPTION_FUNC,   "", false, "", fgOptBpp },
    {"units-feet",                   false, OPTION_STRING, "/sim/startup/units", false, "feet", 0 },
    {"units-meters",                 false, OPTION_STRING, "/sim/startup/units", false, "meters", 0 },
    {"timeofday",                    true,  OPTION_STRING, "/sim/startup/time-offset-type", false, "noon", 0 },
    {"season",                       true,  OPTION_STRING, "/sim/startup/season", false, "summer", 0 },
    {"time-offset",                  true,  OPTION_FUNC,   "", false, "", fgOptTimeOffset },
    {"time-match-real",              false, OPTION_STRING, "/sim/startup/time-offset-type", false, "system-offset", 0 },
    {"time-match-local",             false, OPTION_STRING, "/sim/startup/time-offset-type", false, "latitude-offset", 0 },
    {"start-date-sys",               true,  OPTION_FUNC,   "", false, "", fgOptStartDateSys },
    {"start-date-lat",               true,  OPTION_FUNC,   "", false, "", fgOptStartDateLat },
    {"start-date-gmt",               true,  OPTION_FUNC,   "", false, "", fgOptStartDateGmt },
    {"hud-tris",                     false, OPTION_STRING, "/sim/hud/frame-stat-type", false, "tris", 0 },
    {"hud-culled",                   false, OPTION_STRING, "/sim/hud/frame-stat-type", false, "culled", 0 },
    {"atcsim",                       true,  OPTION_CHANNEL, "", false, "dummy", 0 },
    {"atlas",                        true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"httpd",                        true,  OPTION_FUNC   , "", false, "", fgOptHttpd },
    {"jpg-httpd",                    true,  OPTION_FUNC,    "", false, "", fgOptJpgHttpd },
    {"native",                       true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"native-ctrls",                 true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"native-fdm",                   true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"native-gui",                   true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"opengc",                       true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"AV400",                        true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"AV400Sim",                     true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"AV400WSimA",                   true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"AV400WSimB",                   true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"flarm",                        true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"garmin",                       true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"igc",                          true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"nmea",                         true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"generic",                      true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"props",                        true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"telnet",                       true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
    {"pve",                          true,  OPTION_CHANNEL, "", false, "", 0 },
    {"ray",                          true,  OPTION_CHANNEL, "", false, "", 0 },
    {"rul",                          true,  OPTION_CHANNEL, "", false, "", 0 },
    {"joyclient",                    true,  OPTION_CHANNEL, "", false, "", 0 },
    {"jsclient",                     true,  OPTION_CHANNEL, "", false, "", 0 },
    {"proxy",                        true,  OPTION_FUNC,    "", false, "", fgSetupProxy },
    {"callsign",                     true,  OPTION_FUNC,    "", false, "", fgOptCallSign},
    {"multiplay",                    true,  OPTION_CHANNEL | OPTION_MULTI, "", false, "", 0 },
#if FG_HAVE_HLA
    {"hla",                          true,  OPTION_CHANNEL, "", false, "", 0 },
    {"hla-local",                    true,  OPTION_CHANNEL, "", false, "", 0 },
#endif
    {"trace-read",                   true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptTraceRead },
    {"trace-write",                  true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptTraceWrite },
    {"log-level",                    true,  OPTION_FUNC,   "", false, "", fgOptLogLevel },
    {"log-class",                    true,  OPTION_FUNC,   "", false, "", fgOptLogClasses },
    {"log-dir",                      true,  OPTION_FUNC | OPTION_MULTI, "", false, "", fgOptLogDir },
    {"view-offset",                  true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptViewOffset },
    {"visibility",                   true,  OPTION_FUNC,   "", false, "", fgOptVisibilityMeters },
    {"visibility-miles",             true,  OPTION_FUNC,   "", false, "", fgOptVisibilityMiles },
    {"random-wind",                  false, OPTION_FUNC,   "", false, "", fgOptRandomWind },
    {"wind",                         true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptWind },
    {"turbulence",                   true,  OPTION_FUNC,   "", false, "", fgOptTurbulence },
    {"ceiling",                      true,  OPTION_FUNC,   "", false, "", fgOptCeiling },
    {"wp",                           true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptWp },
    {"flight-plan",                  true,  OPTION_STRING,   "/autopilot/route-manager/file-path", false, "", NULL },
    {"config",                       true,  OPTION_IGNORE | OPTION_MULTI,   "", false, "", 0 },
    {"addon",                        true,  OPTION_FUNC | OPTION_MULTI, "", false, "", fgOptAddon },
    {"data",                         true,  OPTION_FUNC | OPTION_MULTI, "", false, "", fgOptAdditionalDataDir },
    {"aircraft",                     true,  OPTION_STRING, "/sim/aircraft", false, "", 0 },
    {"vehicle",                      true,  OPTION_STRING, "/sim/aircraft", false, "", 0 },
    {"failure",                      true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptFailure },
#ifdef ENABLE_IAX
    {"enable-fgcom",                 false, OPTION_BOOL,   "/sim/fgcom/enabled", true, "", 0 },
    {"disable-fgcom",                false, OPTION_BOOL,   "/sim/fgcom/enabled", false, "", 0 },
#endif
    {"com1",                         true,  OPTION_DOUBLE, "/instrumentation/comm[0]/frequencies/selected-mhz", false, "", 0 },
    {"com2",                         true,  OPTION_DOUBLE, "/instrumentation/comm[1]/frequencies/selected-mhz", false, "", 0 },
    {"nav1",                         true,  OPTION_FUNC,   "", false, "", fgOptNAV1 },
    {"nav2",                         true,  OPTION_FUNC,   "", false, "", fgOptNAV2 },
    {"adf", /*legacy*/               true,  OPTION_FUNC,   "", false, "", fgOptADF },
    {"adf1",                         true,  OPTION_FUNC,   "", false, "", fgOptADF1 },
    {"adf2",                         true,  OPTION_FUNC,   "", false, "", fgOptADF2 },
    {"dme",                          true,  OPTION_FUNC,   "", false, "", fgOptDME },
    {"min-status",                   true,  OPTION_STRING,  "/sim/aircraft-min-status", false, "all", 0 },
    {"livery",                       true,  OPTION_FUNC,   "", false, "", fgOptLivery },
    {"ai-scenario",                  true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptScenario },
    {"parking-id",                   true,  OPTION_FUNC,   "", false, "", fgOptParkpos },
    {"parkpos",                      true,  OPTION_FUNC,   "", false, "", fgOptParkpos },
    {"version",                      false, OPTION_IGNORE, "", false, "", 0 },
    {"json-report",                  false, OPTION_IGNORE, "", false, "", 0 },
    {"enable-fpe",                   false, OPTION_IGNORE, "", false, "", 0},
    {"fgviewer",                     false, OPTION_IGNORE, "", false, "", 0},
    {"no-default-config",            false, OPTION_IGNORE, "", false, "", 0},
    {"prop",                         true,  OPTION_FUNC | OPTION_MULTI,   "", false, "", fgOptSetProperty},
    {"load-tape",                    true,  OPTION_FUNC,   "", false, "", fgOptLoadTape },
    {"developer",                    true,  OPTION_IGNORE | OPTION_BOOL, "", false, "", nullptr },
    {"jsbsim-output-directive-file", true,  OPTION_STRING, "/sim/jsbsim/output-directive-file", false, "", nullptr },
    {"disable-gui",                  false, OPTION_FUNC, "", false, "", fgOptDisableGUI },
    {"composite-viewer",             true,  OPTION_INT,    "/sim/rendering/composite-viewer-enabled", "", "", nullptr},
    {nullptr,                        false, 0,             nullptr, false, nullptr, nullptr}
};


namespace flightgear
{

/**
 * internal storage of a value->option binding
 */
class OptionValue
{
public:
  OptionValue(OptionDesc* d, const string& v) :
    desc(d), value(v)
  {;}

  OptionDesc* desc;
  string value;
};

typedef std::vector<OptionValue> OptionValueVec;
typedef std::map<string, OptionDesc*> OptionDescDict;

class Options::OptionsPrivate
{
public:

  OptionValueVec::const_iterator findValue(const string& key) const
  {
    OptionValueVec::const_iterator it = values.begin();
    for (; it != values.end(); ++it) {
      if (!it->desc) {
        continue; // ignore markers
      }

      if (it->desc->option == key) {
        return it;
      }
    } // of set values iteration

    return it; // not found
  }

    OptionValueVec::iterator findValue(const string& key)
    {
        OptionValueVec::iterator it = values.begin();
        for (; it != values.end(); ++it) {
            if (!it->desc) {
                continue; // ignore markers
            }

            if (it->desc->option == key) {
                return it;
            }
        } // of set values iteration

        return it; // not found
    }

  OptionDesc* findOption(const string& key) const
  {
    OptionDescDict::const_iterator it = options.find(key);
    if (it == options.end()) {
      return NULL;
    }

    return it->second;
  }

  int processOption(OptionDesc* desc, const string& arg_value)
  {
    if (!desc) {
      return FG_OPTIONS_OK; // tolerate marker options
    }

    switch ( desc->type & 0xffff ) {
      case OPTION_BOOL:
        fgSetBool( desc->property, desc->b_param );
        break;
      case OPTION_STRING:
        if ( desc->has_param && !arg_value.empty() ) {
          fgSetString( desc->property, arg_value.c_str() );
        } else if ( !desc->has_param && arg_value.empty() ) {
          fgSetString( desc->property, desc->s_param );
        } else if ( desc->has_param ) {
          SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << desc->option << "' needs a parameter" );
          return FG_OPTIONS_ERROR;
        } else {
          SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << desc->option << "' does not have a parameter" );
          return FG_OPTIONS_ERROR;
        }
        break;
      case OPTION_DOUBLE:
        if ( !arg_value.empty() ) {
          fgSetDouble( desc->property, atof( arg_value ) );
        } else {
          SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << desc->option << "' needs a parameter" );
          return FG_OPTIONS_ERROR;
        }
        break;
      case OPTION_INT:
        if ( !arg_value.empty() ) {
          fgSetInt( desc->property, atoi( arg_value ) );
        } else {
          SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << desc->option << "' needs a parameter" );
          return FG_OPTIONS_ERROR;
        }
        break;
      case OPTION_CHANNEL:
        // XXX return value of add_channel should be checked?
        if ( desc->has_param && !arg_value.empty() ) {
          add_channel( desc->option, arg_value );
        } else if ( !desc->has_param && arg_value.empty() ) {
          add_channel( desc->option, desc->s_param );
        } else if ( desc->has_param ) {
          SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << desc->option << "' needs a parameter" );
          return FG_OPTIONS_ERROR;
        } else {
          SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << desc->option << "' does not have a parameter" );
          return FG_OPTIONS_ERROR;
        }
        break;
      case OPTION_FUNC:
        if ( desc->has_param && !arg_value.empty() ) {
          return desc->func( arg_value.c_str() );
        } else if ( !desc->has_param && arg_value.empty() ) {
          return desc->func( desc->s_param );
        } else if ( desc->has_param ) {
          SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << desc->option << "' needs a parameter" );
          return FG_OPTIONS_ERROR;
        }

        SG_LOG( SG_GENERAL, SG_ALERT, "Option '" << desc->option << "' does not have a parameter" );
        return FG_OPTIONS_ERROR;

      case OPTION_IGNORE:
        break;
    }

    return FG_OPTIONS_OK;
  }

  /**
   * insert a marker value into the values vector. This is necessary
   * when processing options, to ensure the correct ordering, where we scan
   * for marker values in reverse, and then forwards within each group.
   */
  void insertGroupMarker()
  {
    values.push_back(OptionValue(NULL, "-"));
  }

  /**
   * given a current iterator into the values, find the preceding group marker,
   * or return the beginning of the value vector.
   */
  OptionValueVec::const_iterator rfindGroup(OptionValueVec::const_iterator pos) const
  {
    while (--pos != values.begin()) {
      if (pos->desc == NULL) {
        return pos; // found a marker, we're done
      }
    }

    return pos;
  }

  // Return a pointer to a new JSON array node
  // (["/foo/bar", "/other/path", ...]) created from the given PathList.
  cJSON *createJSONArrayFromPathList(const PathList& pl) const
  {
    cJSON *resultNode = cJSON_CreateArray();
    cJSON *prevNode = nullptr;
    bool isFirst = true;

    for (const SGPath& path : pl) {
      cJSON *pathNode = cJSON_CreateString(path.utf8Str().c_str());

      if (isFirst) {
        isFirst = false;
        resultNode->child = pathNode;
      } else {
        prevNode->next = pathNode;
        pathNode->prev = prevNode;
      }

      prevNode = pathNode;
    }

    return resultNode;
  }

  bool showHelp,
    verbose,
    showAircraft,
    shouldLoadDefaultConfig;

  OptionDescDict options;
  OptionValueVec values;
  simgear::PathList configFiles;
  simgear::PathList propertyFiles;
};

Options* Options::sharedInstance()
{
  if (shared_instance == NULL) {
    shared_instance = new Options;
  }

  return shared_instance;
}

void Options::reset()
{
    if (shared_instance != nullptr) {
        delete shared_instance;
        shared_instance = nullptr;
    }
}

Options::Options() :
  p(new OptionsPrivate())
{
  p->showHelp = false;
  p->verbose = false;
  p->showAircraft = false;
  p->shouldLoadDefaultConfig = true;

// build option map
  OptionDesc *desc = &fgOptionArray[ 0 ];
  while ( desc->option != 0 ) {
    // REVIEW: Memory Leak - 15,768 bytes in 219 blocks are still reachable
    p->options[ desc->option ] = desc;
    ++desc;
  }
}

Options::~Options()
{
}

OptionResult Options::init(int argc, char** argv, const SGPath& appDataPath)
{
// first, process the command line
  bool inOptions = true;
  for (int i=1; i<argc; ++i) {
      // important : on first run after the Gatekeeper quarantine flag is
      // cleared, launchd passes us a null argument here. Avoid dying in
      // that case.
      if (!argv[i]) {
          continue;
      }

    if (inOptions && (argv[i][0] == '-')) {
      if (strcmp(argv[i], "--") == 0) { // end of options delimiter
        inOptions = false;
        continue;
      }

      int result = parseOption(argv[i], /* fromConfigFile */ false);
      processArgResult(result);
    } else {
    // XML properties file
        SGPath f = SGPath::fromUtf8(argv[i]);
      if (!f.exists()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "config file not found:" << f);
      } else {
        p->propertyFiles.push_back(f);
      }
    }
  } // of arguments iteration
  p->insertGroupMarker(); // command line is one group

  // establish log-level before anything else - otherwise it is not possible
  // to show extra (debug/info/warning) messages for the start-up phase.
  // Leave the simgear logstream default value of SG_ALERT if the argument is
  // not supplied.
  if (isOptionSet("log-level"))
      fgOptLogLevel(valueForOption("log-level").c_str());

  simgear::PathList::const_iterator i;
  for (i = p->configFiles.begin(); i != p->configFiles.end(); ++i) {
      readConfig(*i);
  }

  if (!p->shouldLoadDefaultConfig) {
      return setupRoot(argc, argv);
  }

// then config files
  SGPath config;

  if( !hostname.empty() ) {
    // Check for ~/.fgfsrc.hostname
    config = SGPath::home();
    config.append(".fgfsrc");
    config.concat( "." );
    config.concat( hostname );
    readConfig(config);
  }

// Check for ~/.fgfsrc
    config = SGPath::home();
    config.append(".fgfsrc");
    readConfig(config);

// check for a config file in app data
  SGPath appDataConfig(appDataPath);
  appDataConfig.append("fgfsrc");
  if (appDataConfig.exists()) {
    readConfig(appDataConfig);
  }

// setup FG_ROOT
  auto res = setupRoot(argc, argv);
  if (res != FG_OPTIONS_OK) {
      return res;
  }

// system.fgfsrc is disabled, as we no longer allow anything in fgdata to set
// fg-root/fg-home/fg-aircraft and hence control what files Nasal can access
  std::string nameForError = config.utf8Str();
  if( ! hostname.empty() ) {
    config = globals->get_fg_root();
    config.append( "system.fgfsrc" );
    config.concat( "." );
    config.concat( hostname );
    if (config.exists()) {
      flightgear::fatalMessageBoxThenExit(
        "Unsupported configuration",
        "You have a '" + config.utf8Str() + "' file, which is no longer "
        "processed for security reasons.",
        "If you created this file intentionally, please move it to '" +
        nameForError + "'.");
    }
  }

  config = globals->get_fg_root();
  config.append( "system.fgfsrc" );
  if (config.exists()) {
    flightgear::fatalMessageBoxThenExit(
      "Unsupported configuration",
      "You have a '" + config.utf8Str() + "' file, which is no longer "
      "processed for security reasons.",
      "If you created this file intentionally, please move it to '" +
      nameForError + "'.");
  }

  return FG_OPTIONS_OK;
}

void Options::initPaths()
{
    for (const string& pathOpt : valuesForOption("fg-aircraft")) {
        PathList paths = SGPath::pathsFromUtf8(pathOpt);
        globals->append_aircraft_paths(paths);
    }

    const char* envp = ::getenv("FG_AIRCRAFT");
    if (envp) {
        globals->append_aircraft_paths(SGPath::pathsFromEnv("FG_AIRCRAFT"));
    }

}

OptionResult Options::initAircraft()
{
  string aircraft;
  if (isOptionSet("aircraft")) {
    aircraft = valueForOption("aircraft");
  } else if (isOptionSet("vehicle")) {
    aircraft = valueForOption("vehicle");
  }

  if (!aircraft.empty()) {
    SG_LOG(SG_INPUT, SG_INFO, "aircraft = " << aircraft );
    fgSetString("/sim/aircraft", aircraft.c_str() );
  } else {
    SG_LOG(SG_INPUT, SG_INFO, "No user specified aircraft, using default" );
  }

  if (p->showAircraft) {
	PathList path_list;

    fgOptLogLevel( "alert" );

    // First place to check is the 'Aircraft' sub-directory in $FG_ROOT

      SGPath rootAircraft = globals->get_fg_root();
      rootAircraft.append("Aircraft");
	path_list.push_back(rootAircraft);

    // Additionally, aircraft may also be found in user-defined places
	// (via $FG_AIRCRAFT or with the '--fg-aircraft' option)
      PathList aircraft_paths = globals->get_aircraft_paths();

      path_list.insert(path_list.end(), aircraft_paths.begin(),
                       aircraft_paths.end());

    fgShowAircraft(path_list);
    // this is to indicate that we did show it
    return FG_OPTIONS_SHOW_AIRCRAFT;
  }

  if (isOptionSet("aircraft-dir")) {
    SGPath aircraftDirPath = SGPath::fromUtf8(valueForOption("aircraft-dir"));
    SGPath realAircraftPath = aircraftDirPath.realpath();
    globals->append_read_allowed_paths(realAircraftPath);

    // Set this now, so it's available in FindAndCacheAircraft. Use realpath()
    // as in FGGlobals::append_aircraft_path(), otherwise fgValidatePath()
    // will deny access to resources under this path if one of its components
    // is a symlink (which is not a problem, since it was given as is by the
    // user---this is very different from a symlink *under* the aircraft dir
    // or a scenery dir).
    fgSetString("/sim/aircraft-dir", realAircraftPath.utf8Str());
  }

    if (isOptionSet("state")) {
        std::string stateName = valueForOption("state");
        // can't validate this until the -set.xml is parsed
        fgSetString("/sim/aircraft-state", stateName);
    }
    
    return FG_OPTIONS_OK;
}

void Options::processArgResult(int result)
{
  if ((result == FG_OPTIONS_HELP) || (result == FG_OPTIONS_ERROR))
    p->showHelp = true;
  else if (result == FG_OPTIONS_VERBOSE_HELP)
    p->verbose = true;
  else if (result == FG_OPTIONS_SHOW_AIRCRAFT) {
    p->showAircraft = true;
  } else if (result == FG_OPTIONS_NO_DEFAULT_CONFIG) {
    p->shouldLoadDefaultConfig = false;
  } else if (result == FG_OPTIONS_SHOW_SOUND_DEVICES) {
    SGSoundMgr smgr;

    smgr.init();
    string vendor = smgr.get_vendor();
    string renderer = smgr.get_renderer();
    cout << renderer << " provided by " << vendor << endl;
    cout << endl << "No. Device" << endl;

    vector <std::string>devices = smgr.get_available_devices();
    for (vector <std::string>::size_type i=0; i<devices.size(); i++) {
      cout << i << ".  \"" << devices[i] << "\"" << endl;
    }
    devices.clear();
    smgr.stop();
    exit(0);
  } else if (result == FG_OPTIONS_EXIT) {
    exit(0);
  }
}

void Options::readConfig(const SGPath& path)
{
  sg_gzifstream in( path );
  if ( !in.is_open() ) {
    return;
  }

  SG_LOG( SG_GENERAL, SG_INFO, "Processing config file: " << path );

  in >> skipcomment;
  while ( ! in.eof() ) {
    string line;
    getline( in, line, '\n' );

    // catch extraneous (DOS) line ending character
    int i;
    for (i = line.length(); i > 0; i--)
      if (line[i - 1] > 32)
        break;
    line = line.substr( 0, i );

    if ( parseOption(line, /* fromConfigFile */ true) == FG_OPTIONS_ERROR ) {
      cerr << endl << "Config file parse error: " << path << " '"
      << line << "'" << endl;
	    p->showHelp = true;
    }
    in >> skipcomment;
  }

  p->insertGroupMarker(); // each config file is a group
}

int Options::parseOption(const string& s, bool fromConfigFile)
{
  if ((s == "--help") || (s=="-h")) {
    return FG_OPTIONS_HELP;
  } else if ( (s == "--verbose") || (s == "-v") ) {
    // verbose help/usage request
    return FG_OPTIONS_VERBOSE_HELP;
  } else if ((s == "--console") || (s == "-c")) {
      fgOptConsole(nullptr);
	  return FG_OPTIONS_OK;
  } else if (s.find("-psn") == 0) {
    // on Mac, when launched from the GUI, we are passed the ProcessSerialNumber
    // as an argument (and no others). Silently ignore the argument here.
    return FG_OPTIONS_OK;
  } else if ( s.find( "--show-aircraft") == 0) {
    return(FG_OPTIONS_SHOW_AIRCRAFT);
  } else if ( s.find( "--show-sound-devices") == 0) {
    return(FG_OPTIONS_SHOW_SOUND_DEVICES);
  } else if ( s.find( "--no-default-config") == 0) {
    return FG_OPTIONS_NO_DEFAULT_CONFIG;
  } else if ( s.find( "--prop:") == 0) {
    // property setting has a slightly different syntax, so fudge things
    OptionDesc* desc = p->findOption("prop");
    if (s.find("=", 7) == string::npos) { // no equals token
      SG_LOG(SG_GENERAL, SG_ALERT, "malformed property option:" << s);
      return FG_OPTIONS_ERROR;
    }

    p->values.push_back(OptionValue(desc, s.substr(7)));
    return FG_OPTIONS_OK;
  } else if ( s.find("--config=") == 0) {
    SGPath path = s.substr(9);
    if (path.extension() == "xml") {
        p->propertyFiles.push_back(path);
    } else if (fromConfigFile) {
      flightgear::fatalMessageBoxThenExit(
        "FlightGear",
        "Invalid use of the --config option.",
        "Sorry, it is currently not supported to load a configuration file "
        "using --config from another configuration file.\n\n"
        "Note: this does not apply to loading of XML PropertyList files "
        "with --config.");
    } else {                // the --config option comes from the command line
        p->configFiles.push_back(path);
    }

    return FG_OPTIONS_OK;
  } else if ( s.find( "--" ) == 0 ) {
    size_t eqPos = s.find( '=' );
    string key, value;
    if (eqPos == string::npos) {
      key = s.substr(2);
    } else {
      key = s.substr( 2, eqPos - 2 );
      value = s.substr( eqPos + 1);
    }

    return addOption(key, value);
  } else {
      flightgear::modalMessageBox("Unknown option", "Unknown command-line option: " + s);
    return FG_OPTIONS_ERROR;
  }
}

int Options::addOption(const string &key, const string &value)
{
    if (key == "config") {
        // occurs when the launcher adds --config options
        SGPath path(value);
        if (!path.exists()) {
            return FG_OPTIONS_ERROR;
        }

        if (path.extension() == "xml") {
            p->propertyFiles.push_back(path);
        } else {
            p->insertGroupMarker(); // begin a group for the config file
            readConfig(path);
        }

        return FG_OPTIONS_OK;
    }

  OptionDesc* desc = p->findOption(key);
  if (!desc) {
    flightgear::modalMessageBox("Unknown option", "Unknown command-line option: " + key);
    return FG_OPTIONS_ERROR;
  }

  if (!(desc->type & OPTION_MULTI)) {
    OptionValueVec::const_iterator it = p->findValue(key);
    if (it != p->values.end()) {
      SG_LOG(SG_GENERAL, SG_WARN, "multiple values forbidden for option:" << key << ", ignoring:" << value);
      return FG_OPTIONS_OK;
    }
  }

  p->values.push_back(OptionValue(desc, value));
  return FG_OPTIONS_OK;
}

int Options::setOption(const string &key, const string &value)
{
    OptionDesc* desc = p->findOption(key);
    if (!desc) {
        flightgear::modalMessageBox("Unknown option", "Unknown command-line option: " + key);
        return FG_OPTIONS_ERROR;
    }

    if (!(desc->type & OPTION_MULTI)) {
        OptionValueVec::iterator it = p->findValue(key);
        if (it != p->values.end()) {
            // remove existing valye
            p->values.erase(it);
        }
    }

    p->values.push_back(OptionValue(desc, value));
    return FG_OPTIONS_OK;
}

void Options::clearOption(const std::string& key)
{
    OptionValueVec::iterator it = p->findValue(key);
    for (; it != p->values.end(); it = p->findValue(key)) {
        p->values.erase(it);
    }
}

bool Options::isOptionSet(const string &key) const
{
  OptionValueVec::const_iterator it = p->findValue(key);
  return (it != p->values.end());
}

string Options::valueForOption(const string& key, const string& defValue) const
{
  OptionValueVec::const_iterator it = p->findValue(key);
  if (it == p->values.end()) {
    return defValue;
  }

  return it->value;
}

string_list Options::valuesForOption(const std::string& key) const
{
  string_list result;
  OptionValueVec::const_iterator it = p->values.begin();
  for (; it != p->values.end(); ++it) {
    if (!it->desc) {
      continue; // ignore marker values
    }

    if (it->desc->option == key) {
      result.push_back(it->value);
    }
  }

  return result;
}

SGPath defaultDownloadDir()
{
#if defined(SG_WINDOWS)
    SGPath p(SGPath::documents());
    if (p.isNull()) {
        SG_LOG(SG_IO, SG_ALERT, "Failed to locate user's Documents directory, will default to FG_HOME");
        // fall through to standard get_fg_home codepath
    }
    else {
        return p / "FlightGear";
    }
#endif

    return globals->get_fg_home();
}

SGPath defaultTextureCacheDir()
{
    return defaultDownloadDir() / "TextureCache";
}

OptionResult Options::processOptions()
{
  // establish locale before showing help (this selects the default locale,
  // when no explicit option was set)
  globals->get_locale()->selectLanguage(valueForOption("language").c_str());

  // now FG_ROOT is setup, process various command line options that bail us
  // out quickly, but rely on aircraft / root settings
  if (p->showHelp) {
    showUsage();
    return FG_OPTIONS_EXIT;
  }

  // processing order is complicated. We must process groups LIFO, but the
  // values *within* each group in FIFO order, to retain consistency with
  // older versions of FG, and existing user configs.
  // in practice this means system.fgfsrc must be *processed* before
  // .fgfsrc, which must be processed before the command line args, and so on.
  OptionValueVec::const_iterator groupEnd = p->values.end();

  while (groupEnd != p->values.begin()) {
    OptionValueVec::const_iterator groupBegin = p->rfindGroup(groupEnd);
  // run over the group in FIFO order
    OptionValueVec::const_iterator it;
    for (it = groupBegin; it != groupEnd; ++it) {
      int result = p->processOption(it->desc, it->value);
      switch(result)
      {
          case FG_OPTIONS_ERROR:
              showUsage();
              return FG_OPTIONS_ERROR;

          case FG_OPTIONS_EXIT:
              return FG_OPTIONS_EXIT;

          default:
              break;
      }
        if (it->desc) {
            SG_LOG(SG_GENERAL, SG_INFO, "\toption:" << it->desc->option << " = " << it->value);
        }
    }

    groupEnd = groupBegin;
  }

  for (const SGPath& file : p->propertyFiles) {
    SG_LOG(SG_GENERAL, SG_INFO,
           "Reading command-line property file " << file);
	  readProperties(file, globals->get_props());
  }

// now options are process, do supplemental fixup
  const char *envp = ::getenv( "FG_SCENERY" );
  if (envp) {
      globals->append_fg_scenery(SGPath::pathsFromEnv("FG_SCENERY"));
  }

    // Download dir fix-up
    SGPath downloadDir = SGPath::fromUtf8(valueForOption("download-dir"));
    if (downloadDir.isNull()) {
        downloadDir = defaultDownloadDir();
        SG_LOG(SG_GENERAL, SG_INFO,
               "Using default download dir: " << downloadDir);
    } else {
      SG_LOG(SG_GENERAL, SG_INFO,
             "Using explicit download dir: " << downloadDir);
    }

    simgear::Dir d(downloadDir);
    if (!d.exists()) {
      SG_LOG(SG_GENERAL, SG_INFO,
             "Creating download dir: " << downloadDir);
      d.create(0755);
    }

    // This is safe because the value of 'downloadDir' is trustworthy. In
    // particular, it can't be influenced by Nasal code, not even indirectly
    // via a Nasal-writable place such as the property tree.
    globals->set_download_dir(downloadDir);

    // Texture Cache directory handling
    SGPath textureCacheDir = SGPath::fromUtf8(valueForOption("texture-cache-dir"));
    if (textureCacheDir.isNull()) {
        textureCacheDir = defaultTextureCacheDir();
        SG_LOG(SG_GENERAL, SG_INFO,
            "Using default texture cache directory: " << textureCacheDir);
    }
    else {
        SG_LOG(SG_GENERAL, SG_INFO,
            "Using explicit texture cache directory: " << textureCacheDir);
    }

    simgear::Dir tcd(textureCacheDir);
    if (!tcd.exists()) {
        SG_LOG(SG_GENERAL, SG_INFO,
            "Creating texture cache directory: " << textureCacheDir);
        tcd.create(0755);
    }

    globals->set_texture_cache_dir(textureCacheDir);


    // TerraSync directory fixup
    SGPath terrasyncDir = SGPath::fromUtf8(valueForOption("terrasync-dir"));
    if (terrasyncDir.isNull()) {
      terrasyncDir = downloadDir / "TerraSync";
      // No “default” qualifier here, because 'downloadDir' may be non-default
      SG_LOG(SG_GENERAL, SG_INFO,
             "Using TerraSync dir: " << terrasyncDir);
    } else {
      SG_LOG(SG_GENERAL, SG_INFO,
             "Using explicit TerraSync dir: " << terrasyncDir);
    }

    d = simgear::Dir(terrasyncDir);
    if (!d.exists()) {
      SG_LOG(SG_GENERAL, SG_INFO,
             "Creating TerraSync dir: " << terrasyncDir);
      d.create(0755);
    }

    // This is safe because the value of 'terrasyncDir' is trustworthy. In
    // particular, it can't be influenced by Nasal code, not even indirectly
    // via a Nasal-writable place such as the property tree.
    globals->set_terrasync_dir(terrasyncDir);

    // check if we setup a scenery path so far
    bool addFGDataScenery = globals->get_fg_scenery().empty();

    // always add the terrasync location, regardless of whether terrasync
    // is enabled or not. This allows us to toggle terrasync on/off at
    // runtime and have things work as expected
    const PathList& scenery_paths(globals->get_fg_scenery());
    if (std::find(scenery_paths.begin(), scenery_paths.end(), terrasyncDir) == scenery_paths.end()) {
        // terrasync dir is not in the scenery paths, add it
        globals->append_fg_scenery(terrasyncDir);
    }

    if (addFGDataScenery) {
        // no scenery paths set at all, use the data in FG_ROOT
        // ensure this path is added last
        SGPath root(globals->get_fg_root());
        root.append("Scenery");
        globals->append_fg_scenery(root);
    }

  if (isOptionSet("json-report")) {
    printJSONReport();
    return FG_OPTIONS_EXIT;
  } else if (isOptionSet("version")) {
    showVersion();
    return FG_OPTIONS_EXIT;
  }

  return FG_OPTIONS_OK;
}

void Options::showUsage() const
{
  fgOptLogLevel( "alert" );

  FGLocale *locale = globals->get_locale();
  SGPropertyNode options_root;

  simgear::requestConsole(); // ensure console is shown on Windows
  cout << endl;

  try {
    fgLoadProps("options.xml", &options_root);
  } catch (const sg_exception &) {
    cout << "Unable to read the help file." << endl;
    cout << "Make sure the file options.xml is located in the FlightGear base directory," << endl;
    cout << "and the location of the base directory is specified by setting $FG_ROOT or" << endl;
    cout << "by adding --fg-root=path as a program argument." << endl;

    exit(-1);
  }

  SGPropertyNode *options = options_root.getNode("options");
  if (!options) {
    SG_LOG( SG_GENERAL, SG_ALERT,
           "Error reading options.xml: <options> element not found." );
    exit(-1);
  }

  if (!locale->loadResource("options"))
  {
      cout << "Unable to read the language resource." << endl;
      exit(-1);
  }

  std::string usage = locale->getLocalizedString(options->getStringValue("usage"), "options");
  if (!usage.empty()) {
    cout << usage << endl;
  }

  vector<SGPropertyNode_ptr>section = options->getChildren("section");
  for (unsigned int j = 0; j < section.size(); j++) {
    string msg = "";

    vector<SGPropertyNode_ptr>option = section[j]->getChildren("option");
    for (unsigned int k = 0; k < option.size(); k++) {

      SGPropertyNode *name = option[k]->getNode("name");
      SGPropertyNode *short_name = option[k]->getNode("short");
      SGPropertyNode *key = option[k]->getNode("key");
      SGPropertyNode *arg = option[k]->getNode("arg");
      bool brief = option[k]->getNode("brief") != 0;

      if ((brief || p->verbose) && name) {
        string tmp = name->getStringValue();

        if (key){
          tmp.append(":");
          tmp.append(key->getStringValue());
        }
        if (arg) {
          tmp.append("=");
          tmp.append(arg->getStringValue());
        }
        if (short_name) {
          tmp.append(", -");
          tmp.append(short_name->getStringValue());
        }

        if (tmp.size() <= 25) {
          msg+= "   --";
          msg += tmp;
          msg.append( 27-tmp.size(), ' ');
        } else {
          msg += "\n   --";
          msg += tmp + '\n';
          msg.append(32, ' ');
        }
        // There may be more than one <description> tag associated
        // with one option

        vector<SGPropertyNode_ptr> desc;
        desc = option[k]->getChildren("description");
        if (! desc.empty()) {
          for ( unsigned int l = 0; l < desc.size(); l++) {
            string t = desc[l]->getStringValue();

            // There may be more than one translation line.
            vector<SGPropertyNode_ptr>trans_desc = locale->getLocalizedStrings(t.c_str(),"options");
            for ( unsigned int m = 0; m < trans_desc.size(); m++ ) {
              string t_str = trans_desc[m]->getStringValue();

              if ((m > 0) || ((l > 0) && m == 0)) {
                msg.append( 32, ' ');
              }

              // If the string is too large to fit on the screen,
              // then split it up in several pieces.

              while ( t_str.size() > 47 ) {

                string::size_type m = t_str.rfind(' ', 47);
                msg += t_str.substr(0, m) + '\n';
                msg.append( 32, ' ');

                t_str.erase(t_str.begin(), t_str.begin() + m + 1);
              }
              msg += t_str + '\n';
            }
          }
        }
      }
    }

    std::string name = locale->getLocalizedString(section[j]->getStringValue("name"),"options");
    if (!msg.empty() && !name.empty()) {
      cout << endl << name << ":" << endl;
      cout << msg;
      msg.erase();
    }
  }

  if ( !p->verbose ) {
    std::string verbose_help = locale->getLocalizedString(options->getStringValue("verbose-help"),"options");
    if (!verbose_help.empty())
        cout << endl << verbose_help << endl;
  }
#ifdef _MSC_VER
  std::cout << "Hit a key to continue..." << std::endl;
  std::cin.get();
#endif
}

void Options::showVersion() const
{
    cout << "FlightGear version: " << FLIGHTGEAR_VERSION << endl;
    cout << "Revision: " << REVISION << endl;
    cout << "Build-Id: " << JENKINS_BUILD_ID << endl;
    cout << "Build-Type: " << FG_BUILD_TYPE << endl;
    cout << "FG_ROOT=" << globals->get_fg_root().utf8Str() << endl;
    cout << "FG_HOME=" << globals->get_fg_home().utf8Str() << endl;
    cout << "FG_SCENERY=";

    PathList scn = globals->get_fg_scenery();
    cout << SGPath::join(scn, SGPath::pathListSep) << endl;
    cout << "SimGear version: " << SG_STRINGIZE(SIMGEAR_VERSION) << endl;
    cout << "OSG version: " << osgGetVersion() << endl;
    cout << "PLIB version: " << PLIB_VERSION << endl;
}

// Print a report using JSON syntax on the standard output, encoded in UTF-8.
//
// The report format is versioned, don't forget to update it when making
// changes (see below).
void Options::printJSONReport() const
{
  cJSON *rootNode = cJSON_CreateObject();

  cJSON *metaNode = cJSON_CreateObject();
  cJSON_AddItemToObject(rootNode, "meta", metaNode);
  cJSON_AddStringToObject(metaNode, "type", "FlightGear JSON report");
  // When making compatible changes to the format (e.g., adding members to
  // JSON objects), only the minor version number should be increased.
  // Increase the major version number when a change is backward-incompatible
  // (such as the removal, renaming or semantic change of a member). Of
  // course, incompatible changes should only be considered as a last
  // recourse.
  cJSON_AddNumberToObject(metaNode, "format major version", 1);
  cJSON_AddNumberToObject(metaNode, "format minor version", 0);

  cJSON *generalNode = cJSON_CreateObject();
  cJSON_AddItemToObject(rootNode, "general", generalNode);
  cJSON_AddStringToObject(generalNode, "name", "FlightGear");
  cJSON_AddStringToObject(generalNode, "version", FLIGHTGEAR_VERSION);
  cJSON_AddStringToObject(generalNode, "build ID", JENKINS_BUILD_ID);
  cJSON_AddStringToObject(generalNode, "build type", FG_BUILD_TYPE);

  cJSON *configNode = cJSON_CreateObject();
  cJSON_AddItemToObject(rootNode, "config", configNode);
  cJSON_AddStringToObject(configNode, "FG_ROOT",
                          globals->get_fg_root().utf8Str().c_str());
  cJSON_AddStringToObject(configNode, "FG_HOME",
                          globals->get_fg_home().utf8Str().c_str());

  cJSON *sceneryPathsNode = p->createJSONArrayFromPathList(globals->get_fg_scenery());
  cJSON_AddItemToObject(configNode, "scenery paths", sceneryPathsNode);

  cJSON *aircraftPathsNode = p->createJSONArrayFromPathList(
    globals->get_aircraft_paths());
  cJSON_AddItemToObject(configNode, "aircraft paths", aircraftPathsNode);

  cJSON_AddStringToObject(configNode, "TerraSync directory",
                          globals->get_terrasync_dir().utf8Str().c_str());

  cJSON_AddStringToObject(configNode, "download directory",
                          globals->get_download_dir().utf8Str().c_str());

  cJSON_AddStringToObject(configNode, "autosave file",
                          globals->autosaveFilePath().utf8Str().c_str());

  // Get the ordered lists of apt.dat, fix.dat and nav.dat files used by the
  // NavCache
  NavDataCache* cache = NavDataCache::instance();
  if (!cache) {
    cache = NavDataCache::createInstance();
  }

  cJSON *navDataNode = cJSON_CreateObject();
  cJSON_AddItemToObject(rootNode, "navigation data", navDataNode);

  // Write each list to the JSON tree
  for (const auto& datType: {NavDataCache::DATFILETYPE_APT,
                             NavDataCache::DATFILETYPE_FIX,
                             NavDataCache::DATFILETYPE_NAV}) {
    // For this method, it doesn't matter if the cache is out-of-date
    const NavDataCache::DatFilesGroupInfo& datFilesInfo =
      cache->getDatFilesInfo(datType);
    cJSON *datPathsNode = p->createJSONArrayFromPathList(datFilesInfo.paths);
    string key = NavDataCache::datTypeStr[datType] + ".dat files";
    cJSON_AddItemToObject(navDataNode, key.c_str(), datPathsNode);
  }

  // Print the JSON tree to the standard output
  char *report = cJSON_Print(rootNode);
  cout << report << endl;
  cJSON_Delete(rootNode);
}

#if defined(__CYGWIN__)
SGPath Options::platformDefaultRoot() const
{
  return SGPath::fromUtf8("../data");
}

#elif defined(SG_WINDOWS)
SGPath Options::platformDefaultRoot() const
{
  return SGPath::fromUtf8("..\\data");
}
#elif defined(SG_MAC)
// platformDefaultRoot defined in CocoaHelpers.mm
#else
SGPath Options::platformDefaultRoot() const
{
    return SGPath::fromUtf8(PKGLIBDIR);
}

#endif

string_list Options::extractOptions() const
{
    string_list result;
    for (auto opt : p->values) {
        if (opt.desc == nullptr) {
            continue;
        }

        if (!strcmp(opt.desc->option,"prop")) {
            result.push_back("prop:" + opt.value);
        } else if (opt.value.empty()) {
            result.push_back(opt.desc->option);
        } else {
            result.push_back(std::string(opt.desc->option) + "=" + opt.value);
        }
    }

    return result;
}

OptionResult Options::setupRoot(int argc, char** argv)
{
    SGPath root(globals->get_fg_root());
    bool usingDefaultRoot = false;

    // root has already been set, so skip the fg_root setting and validation.
    if (!root.isNull()) {
        return FG_OPTIONS_OK;
    }

  if (isOptionSet("fg-root")) {
      root = SGPath::fromUtf8(valueForOption("fg-root")); // easy!
      SG_LOG(SG_GENERAL, SG_INFO, "set from command-line argument: fg_root = " << root );
  } else {
  // Next check if fg-root is set as an env variable
    char *envp = ::getenv( "FG_ROOT" );
    if ( envp != nullptr ) {
        root = SGPath::fromEnv("FG_ROOT");
        SG_LOG(SG_GENERAL, SG_INFO, "set from FG_ROOT env var: fg_root = " << root );
    } else {
#if defined(HAVE_QT)
        auto restoreResult = SetupRootDialog::restoreUserSelectedRoot(root);
        if (restoreResult == SetupRootDialog::UserExit) {
            return FG_OPTIONS_EXIT;
        } else if (restoreResult == SetupRootDialog::UseDefault) {
            root = SGPath{}; // clear any value, so we fall through in root.isNull() below
        }
#endif

        if (root.isNull()) {
            usingDefaultRoot = true;
            root = platformDefaultRoot();
            SG_LOG(SG_GENERAL, SG_INFO, "platform default fg_root = " << root );
        } else {
            SG_LOG(SG_GENERAL, SG_INFO, "Qt launcher set fg_root = " << root );
        }
    }
  }

  globals->set_fg_root(root);
    string base_version = fgBasePackageVersion(root);


#if defined(HAVE_QT)
    // only compare major and minor version, not the patch level.
    const int versionComp = simgear::strutils::compare_versions(FLIGHTGEAR_VERSION, base_version, 2);

    // note we never end up here if restoring a user selected root via
    // the Qt GUI, since that code pre-validates the path. But if we're using
    // a command-line, env-var or default root this check can fail and
    // we still want to use the GUI in that case
    if (versionComp != 0) {
        flightgear::initApp(argc, argv);
        bool ok = SetupRootDialog::runDialog(usingDefaultRoot);
        if (!ok) {
            return FG_OPTIONS_EXIT;
        }
    }
#else
    SG_UNUSED(usingDefaultRoot);

    // validate it
    if (base_version.empty()) {
        flightgear::fatalMessageBoxThenExit(
          "Base package not found",
          "Required data files not found, please check your installation.",
          "Looking for base-package files at: '" + root.str() + "'");
    }

    // only compare major and minor version, not the patch level.
    const int versionComp = simgear::strutils::compare_versions(FLIGHTGEAR_VERSION, base_version, 2);
    if (versionComp != 0) {
      flightgear::fatalMessageBoxThenExit(
        "Base package version mismatch",
        "Version check failed, please check your installation.",
        "Found data files for version '" + base_version + "' at '" +
        globals->get_fg_root().str() + "', version '" +
        std::string(FLIGHTGEAR_VERSION) + "' is required.");
  }
#endif
    return FG_OPTIONS_OK;
}

bool Options::shouldLoadDefaultConfig() const
{
  return p->shouldLoadDefaultConfig;
}

void Options::setShouldLoadDefaultConfig(bool load)
{
    p->shouldLoadDefaultConfig = load;
}

bool Options::checkForArg(int argc, char* argv[], const char* checkArg)
{
    for (int i = 0; i < argc; ++i) {
        char* arg = argv[i];
        if (arg == nullptr) {
            continue;
        }

        if (*arg != '-') { // we only care about args with a leading hypen
            continue;
        }

        arg++;
        if (*arg == '-') { // skip double hypens
            arg++;
        }

        if (strcmp(arg, checkArg) == 0) {
            return true;
        }
    }

    return false;
}

std::string Options::getArgValue(int argc, char* argv[], const char* checkArg)
{
    const auto len = strlen(checkArg);
    for (int i = 0; i < argc; ++i) {
        char* arg = argv[i];
        if (arg == nullptr) {
            continue;
        }

        if (strncmp(arg, checkArg, len) == 0) {
            const auto alen = strlen(arg);
            if ((alen - len) < 2)
                return {}; // no value after the =, or missing = entirely
            return std::string(arg + len + 1);
        }
    } // of args iteration

    return {};
}

} // of namespace flightgear
