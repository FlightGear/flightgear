// fg_props.cxx -- support for FlightGear properties.
//
// Written by David Megginson, started 2000.
//
// Copyright (C) 2000, 2001 David Megginson - david@megginson.com
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>

#include <simgear/magvar/magvar.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/particles.hxx>
#include <simgear/sound/soundmgr_openal.hxx>

#include <GUI/gui.h>

#include "globals.hxx"
#include "fg_props.hxx"


static bool winding_ccw = true; // FIXME: temporary

static bool frozen = false;	// FIXME: temporary

using std::string;

////////////////////////////////////////////////////////////////////////
// Default property bindings (not yet handled by any module).
////////////////////////////////////////////////////////////////////////

struct LogClassMapping {
  sgDebugClass c;
  string name;
  LogClassMapping(sgDebugClass cc, string nname) { c = cc; name = nname; }
};

LogClassMapping log_class_mappings [] = {
  LogClassMapping(SG_NONE, "none"),
  LogClassMapping(SG_TERRAIN, "terrain"),
  LogClassMapping(SG_ASTRO, "astro"),
  LogClassMapping(SG_FLIGHT, "flight"),
  LogClassMapping(SG_INPUT, "input"),
  LogClassMapping(SG_GL, "gl"),
  LogClassMapping(SG_VIEW, "view"),
  LogClassMapping(SG_COCKPIT, "cockpit"),
  LogClassMapping(SG_GENERAL, "general"),
  LogClassMapping(SG_MATH, "math"),
  LogClassMapping(SG_EVENT, "event"),
  LogClassMapping(SG_AIRCRAFT, "aircraft"),
  LogClassMapping(SG_AUTOPILOT, "autopilot"),
  LogClassMapping(SG_IO, "io"),
  LogClassMapping(SG_CLIPPER, "clipper"),
  LogClassMapping(SG_NETWORK, "network"),
  LogClassMapping(SG_INSTR, "instrumentation"),
  LogClassMapping(SG_ATC, "atc"),
  LogClassMapping(SG_NASAL, "nasal"),
  LogClassMapping(SG_SYSTEMS, "systems"),
  LogClassMapping(SG_AI, "ai"),
  LogClassMapping(SG_ENVIRONMENT, "environment"),
  LogClassMapping(SG_SOUND, "sound"),
  LogClassMapping(SG_UNDEFD, "")
};


/**
 * Get the logging classes.
 */
// XXX Making the result buffer be global is a band-aid that hopefully
// delays its destruction 'til after its last use.
namespace
{
string loggingResult;
}

static const char *
getLoggingClasses ()
{
  sgDebugClass classes = logbuf::get_log_classes();
  loggingResult.clear();
  for (int i = 0; log_class_mappings[i].c != SG_UNDEFD; i++) {
    if ((classes&log_class_mappings[i].c) > 0) {
      if (!loggingResult.empty())
	loggingResult += '|';
      loggingResult += log_class_mappings[i].name;
    }
  }
  return loggingResult.c_str();
}


static void
addLoggingClass (const string &name)
{
  sgDebugClass classes = logbuf::get_log_classes();
  for (int i = 0; log_class_mappings[i].c != SG_UNDEFD; i++) {
    if (name == log_class_mappings[i].name) {
      logbuf::set_log_classes(sgDebugClass(classes|log_class_mappings[i].c));
      return;
    }
  }
  SG_LOG(SG_GENERAL, SG_WARN, "Unknown logging class: " << name);
}


/**
 * Set the logging classes.
 */
void
setLoggingClasses (const char * c)
{
  string classes = c;
  logbuf::set_log_classes(SG_NONE);

  if (classes == "none") {
    SG_LOG(SG_GENERAL, SG_INFO, "Disabled all logging classes");
    return;
  }

  if (classes.empty() || classes == "all") { // default
    logbuf::set_log_classes(SG_ALL);
    SG_LOG(SG_GENERAL, SG_INFO, "Enabled all logging classes: "
	   << getLoggingClasses());
    return;
  }

  string rest = classes;
  string name = "";
  string::size_type sep = rest.find('|');
  if (sep == string::npos)
    sep = rest.find(',');
  while (sep != string::npos) {
    name = rest.substr(0, sep);
    rest = rest.substr(sep+1);
    addLoggingClass(name);
    sep = rest.find('|');
    if (sep == string::npos)
      sep = rest.find(',');
  }
  addLoggingClass(rest);
  SG_LOG(SG_GENERAL, SG_INFO, "Set logging classes to "
	 << getLoggingClasses());
}


/**
 * Get the logging priority.
 */
static const char *
getLoggingPriority ()
{
  switch (logbuf::get_log_priority()) {
  case SG_BULK:
    return "bulk";
  case SG_DEBUG:
    return "debug";
  case SG_INFO:
    return "info";
  case SG_WARN:
    return "warn";
  case SG_ALERT:
    return "alert";
  default:
    SG_LOG(SG_GENERAL, SG_WARN, "Internal: Unknown logging priority number: "
	   << logbuf::get_log_priority());
    return "unknown";
  }
}


/**
 * Set the logging priority.
 */
void
setLoggingPriority (const char * p)
{
  if (p == 0)
      return;
  string priority = p;
  if (priority == "bulk") {
    logbuf::set_log_priority(SG_BULK);
  } else if (priority == "debug") {
    logbuf::set_log_priority(SG_DEBUG);
  } else if (priority.empty() || priority == "info") { // default
    logbuf::set_log_priority(SG_INFO);
  } else if (priority == "warn") {
    logbuf::set_log_priority(SG_WARN);
  } else if (priority == "alert") {
    logbuf::set_log_priority(SG_ALERT);
  } else {
    SG_LOG(SG_GENERAL, SG_WARN, "Unknown logging priority " << priority);
  }
  SG_LOG(SG_GENERAL, SG_DEBUG, "Logging priority is " << getLoggingPriority());
}


/**
 * Return the current frozen state.
 */
static bool
getFreeze ()
{
  return frozen;
}


/**
 * Set the current frozen state.
 */
static void
setFreeze (bool f)
{
    frozen = f;

    // Stop sound on a pause
    SGSoundMgr *smgr = globals->get_soundmgr();
    if ( smgr != NULL ) {
        if ( f ) {
            smgr->suspend();
        } else if (fgGetBool("/sim/sound/working")) {
            smgr->resume();
        }
    }

    // Pause the particle system
    simgear::Particles::setFrozen(f);
}


/**
 * Return the number of milliseconds elapsed since simulation started.
 */
static double
getElapsedTime_sec ()
{
  return globals->get_sim_time_sec();
}


/**
 * Return the current Zulu time.
 */
static const char *
getDateString ()
{
  static char buf[64];		// FIXME
  
  SGTime * st = globals->get_time_params();
  if (!st) {
    buf[0] = 0;
    return buf;
  }
  
  struct tm * t = st->getGmt();
  sprintf(buf, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d",
	  t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
	  t->tm_hour, t->tm_min, t->tm_sec);
  return buf;
}


/**
 * Set the current Zulu time.
 */
static void
setDateString (const char * date_string)
{
  SGTime * st = globals->get_time_params();
  struct tm * current_time = st->getGmt();
  struct tm new_time;

				// Scan for basic ISO format
				// YYYY-MM-DDTHH:MM:SS
  int ret = sscanf(date_string, "%d-%d-%dT%d:%d:%d",
		   &(new_time.tm_year), &(new_time.tm_mon),
		   &(new_time.tm_mday), &(new_time.tm_hour),
		   &(new_time.tm_min), &(new_time.tm_sec));

				// Be pretty picky about this, so
				// that strange things don't happen
				// if the save file has been edited
				// by hand.
  if (ret != 6) {
    SG_LOG(SG_INPUT, SG_WARN, "Date/time string " << date_string
	   << " not in YYYY-MM-DDTHH:MM:SS format; skipped");
    return;
  }

				// OK, it looks like we got six
				// values, one way or another.
  new_time.tm_year -= 1900;
  new_time.tm_mon -= 1;
				// Now, tell flight gear to use
				// the new time.  This was far
				// too difficult, by the way.
  long int warp =
    mktime(&new_time) - mktime(current_time) + globals->get_warp();
    
  fgSetInt("/sim/time/warp", warp);
}

/**
 * Return the GMT as a string.
 */
static const char *
getGMTString ()
{
  static char buf[16];
  SGTime * st = globals->get_time_params();
  if (!st) {
    buf[0] = 0;
    return buf;
  }
  
  struct tm *t = st->getGmt();
  snprintf(buf, 16, "%.2d:%.2d:%.2d",
      t->tm_hour, t->tm_min, t->tm_sec);
  return buf;
}

/**
 * Return the magnetic variation
 */
static double
getMagVar ()
{
  return globals->get_mag()->get_magvar() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Return the magnetic dip
 */
static double
getMagDip ()
{
  return globals->get_mag()->get_magdip() * SGD_RADIANS_TO_DEGREES;
}


/**
 * Return the current heading in degrees.
 */
static double
getHeadingMag ()
{
  double magheading = fgGetDouble("/orientation/heading-deg") - getMagVar();
  return SGMiscd::normalizePeriodic(0, 360, magheading );
}

/**
 * Return the current track in degrees.
 */
static double
getTrackMag ()
{
  double magtrack = fgGetDouble("/orientation/track-deg") - getMagVar();
  return SGMiscd::normalizePeriodic(0, 360, magtrack );
}

static bool
getWindingCCW ()
{
  return winding_ccw;
}

static void
setWindingCCW (bool state)
{
  winding_ccw = state;
  if ( winding_ccw )
    glFrontFace ( GL_CCW );
  else
    glFrontFace ( GL_CW );
}

static const char *
getLongitudeString ()
{
  static SGConstPropertyNode_ptr n = fgGetNode("/position/longitude-deg", true);
  static SGConstPropertyNode_ptr f = fgGetNode("/sim/lon-lat-format", true);
  static char buf[32];
  double d = n->getDoubleValue();
  int format = f->getIntValue();
  char c = d < 0.0 ? 'W' : 'E';

  if (format == 0) {
    snprintf(buf, 32, "%3.6f%c", d, c);

  } else if (format == 1) {
    // dd mm.mmm' (DMM-Format) -- uses a round-off factor tailored to the
    // required precision of the minutes field (three decimal places),
    // preventing minute values of 60.
    double deg = fabs(d) + 5.0E-4 / 60.0;
    double min = fabs(deg - int(deg)) * 60.0 - 4.999E-4;
    snprintf(buf, 32, "%d*%06.3f%c", int(d < 0.0 ? -deg : deg), min, c);

  } else {
    // mm'ss.s'' (DMS-Format) -- uses a round-off factor tailored to the
    // required precision of the seconds field (one decimal place),
    // preventing second values of 60.
    double deg = fabs(d) + 0.05 / 3600.0;
    double min = (deg - int(deg)) * 60.0;
    double sec = (min - int(min)) * 60.0 - 0.049;
    snprintf(buf, 32, "%d*%02d %04.1f%c", int(d < 0.0 ? -deg : deg),
        int(min), fabs(sec), c);
  }
  buf[31] = '\0';
  return buf;
}

static const char *
getLatitudeString ()
{
  static SGConstPropertyNode_ptr n = fgGetNode("/position/latitude-deg", true);
  static SGConstPropertyNode_ptr f = fgGetNode("/sim/lon-lat-format", true);
  static char buf[32];
  double d = n->getDoubleValue();
  int format = f->getIntValue();
  char c = d < 0.0 ? 'S' : 'N';

  if (format == 0) {
    snprintf(buf, 32, "%3.6f%c", d, c);

  } else if (format == 1) {
    double deg = fabs(d) + 5.0E-4 / 60.0;
    double min = fabs(deg - int(deg)) * 60.0 - 4.999E-4;
    snprintf(buf, 32, "%d*%06.3f%c", int(d < 0.0 ? -deg : deg), min, c);

  } else {
    double deg = fabs(d) + 0.05 / 3600.0;
    double min = (deg - int(deg)) * 60.0;
    double sec = (min - int(min)) * 60.0 - 0.049;
    snprintf(buf, 32, "%d*%02d %04.1f%c", int(d < 0.0 ? -deg : deg),
        int(min), fabs(sec), c);
  }
  buf[31] = '\0';
  return buf;
}




////////////////////////////////////////////////////////////////////////
// Tie the properties.
////////////////////////////////////////////////////////////////////////

FGProperties::FGProperties ()
{
}

FGProperties::~FGProperties ()
{
}

void
FGProperties::init ()
{
}

void
FGProperties::bind ()
{
  _tiedProperties.setRoot(globals->get_props());

  // Simulation
  _tiedProperties.Tie("/sim/logging/priority", getLoggingPriority, setLoggingPriority);
  _tiedProperties.Tie("/sim/logging/classes", getLoggingClasses, setLoggingClasses);
  _tiedProperties.Tie("/sim/freeze/master", getFreeze, setFreeze);

  _tiedProperties.Tie("/sim/time/elapsed-sec", getElapsedTime_sec);
  _tiedProperties.Tie("/sim/time/gmt", getDateString, setDateString);
  fgSetArchivable("/sim/time/gmt");
  _tiedProperties.Tie("/sim/time/gmt-string", getGMTString);

  // Position
  _tiedProperties.Tie("/position/latitude-string", getLatitudeString);
  _tiedProperties.Tie("/position/longitude-string", getLongitudeString);

  // Orientation
  _tiedProperties.Tie("/orientation/heading-magnetic-deg", getHeadingMag);
  _tiedProperties.Tie("/orientation/track-magnetic-deg", getTrackMag);

  _tiedProperties.Tie("/environment/magnetic-variation-deg", getMagVar);
  _tiedProperties.Tie("/environment/magnetic-dip-deg", getMagDip);

  // Misc. Temporary junk.
  _tiedProperties.Tie("/sim/temp/winding-ccw", getWindingCCW, setWindingCCW, false);
}

void
FGProperties::unbind ()
{
    _tiedProperties.Untie();
}

void
FGProperties::update (double dt)
{
    static SGPropertyNode_ptr offset = fgGetNode("/sim/time/local-offset", true);
    offset->setIntValue(globals->get_time_params()->get_local_offset());

    // utc date/time
    static SGPropertyNode_ptr uyear = fgGetNode("/sim/time/utc/year", true);
    static SGPropertyNode_ptr umonth = fgGetNode("/sim/time/utc/month", true);
    static SGPropertyNode_ptr uday = fgGetNode("/sim/time/utc/day", true);
    static SGPropertyNode_ptr uhour = fgGetNode("/sim/time/utc/hour", true);
    static SGPropertyNode_ptr umin = fgGetNode("/sim/time/utc/minute", true);
    static SGPropertyNode_ptr usec = fgGetNode("/sim/time/utc/second", true);
    static SGPropertyNode_ptr uwday = fgGetNode("/sim/time/utc/weekday", true);
    static SGPropertyNode_ptr udsec = fgGetNode("/sim/time/utc/day-seconds", true);

    struct tm *u = globals->get_time_params()->getGmt();
    uyear->setIntValue(u->tm_year + 1900);
    umonth->setIntValue(u->tm_mon + 1);
    uday->setIntValue(u->tm_mday);
    uhour->setIntValue(u->tm_hour);
    umin->setIntValue(u->tm_min);
    usec->setIntValue(u->tm_sec);
    uwday->setIntValue(u->tm_wday);

    udsec->setIntValue(u->tm_hour * 3600 + u->tm_min * 60 + u->tm_sec);


    // real local date/time
    static SGPropertyNode_ptr ryear = fgGetNode("/sim/time/real/year", true);
    static SGPropertyNode_ptr rmonth = fgGetNode("/sim/time/real/month", true);
    static SGPropertyNode_ptr rday = fgGetNode("/sim/time/real/day", true);
    static SGPropertyNode_ptr rhour = fgGetNode("/sim/time/real/hour", true);
    static SGPropertyNode_ptr rmin = fgGetNode("/sim/time/real/minute", true);
    static SGPropertyNode_ptr rsec = fgGetNode("/sim/time/real/second", true);
    static SGPropertyNode_ptr rwday = fgGetNode("/sim/time/real/weekday", true);

    time_t real = time(0);
    struct tm *r = localtime(&real);
    ryear->setIntValue(r->tm_year + 1900);
    rmonth->setIntValue(r->tm_mon + 1);
    rday->setIntValue(r->tm_mday);
    rhour->setIntValue(r->tm_hour);
    rmin->setIntValue(r->tm_min);
    rsec->setIntValue(r->tm_sec);
    rwday->setIntValue(r->tm_wday);
}



////////////////////////////////////////////////////////////////////////
// Save and restore.
////////////////////////////////////////////////////////////////////////


/**
 * Save the current state of the simulator to a stream.
 */
bool
fgSaveFlight (std::ostream &output, bool write_all)
{

  fgSetBool("/sim/presets/onground", false);
  fgSetArchivable("/sim/presets/onground");
  fgSetBool("/sim/presets/trim", false);
  fgSetArchivable("/sim/presets/trim");
  fgSetString("/sim/presets/speed-set", "UVW");
  fgSetArchivable("/sim/presets/speed-set");

  try {
    writeProperties(output, globals->get_props(), write_all);
  } catch (const sg_exception &e) {
    guiErrorMessage("Error saving flight: ", e);
    return false;
  }
  return true;
}


/**
 * Restore the current state of the simulator from a stream.
 */
bool
fgLoadFlight (std::istream &input)
{
  SGPropertyNode props;
  try {
    readProperties(input, &props);
  } catch (const sg_exception &e) {
    guiErrorMessage("Error reading saved flight: ", e);
    return false;
  }

  fgSetBool("/sim/presets/onground", false);
  fgSetBool("/sim/presets/trim", false);
  fgSetString("/sim/presets/speed-set", "UVW");

  copyProperties(&props, globals->get_props());
  // When loading a flight, make it the
  // new initial state.
  globals->saveInitialState();
  return true;
}


bool
fgLoadProps (const char * path, SGPropertyNode * props, bool in_fg_root, int default_mode)
{
    string fullpath;
    if (in_fg_root) {
        SGPath loadpath(globals->get_fg_root());
        loadpath.append(path);
        fullpath = loadpath.str();
    } else {
        fullpath = path;
    }

    try {
        readProperties(fullpath, props, default_mode);
    } catch (const sg_exception &e) {
        guiErrorMessage("Error reading properties: ", e);
        return false;
    }
    return true;
}



////////////////////////////////////////////////////////////////////////
// Property convenience functions.
////////////////////////////////////////////////////////////////////////

SGPropertyNode *
fgGetNode (const char * path, bool create)
{
  return globals->get_props()->getNode(path, create);
}

SGPropertyNode * 
fgGetNode (const char * path, int index, bool create)
{
  return globals->get_props()->getNode(path, index, create);
}

bool
fgHasNode (const char * path)
{
  return (fgGetNode(path, false) != 0);
}

void
fgAddChangeListener (SGPropertyChangeListener * listener, const char * path)
{
  fgGetNode(path, true)->addChangeListener(listener);
}

void
fgAddChangeListener (SGPropertyChangeListener * listener,
		     const char * path, int index)
{
  fgGetNode(path, index, true)->addChangeListener(listener);
}

bool
fgGetBool (const char * name, bool defaultValue)
{
  return globals->get_props()->getBoolValue(name, defaultValue);
}

int
fgGetInt (const char * name, int defaultValue)
{
  return globals->get_props()->getIntValue(name, defaultValue);
}

int
fgGetLong (const char * name, long defaultValue)
{
  return globals->get_props()->getLongValue(name, defaultValue);
}

float
fgGetFloat (const char * name, float defaultValue)
{
  return globals->get_props()->getFloatValue(name, defaultValue);
}

double
fgGetDouble (const char * name, double defaultValue)
{
  return globals->get_props()->getDoubleValue(name, defaultValue);
}

const char *
fgGetString (const char * name, const char * defaultValue)
{
  return globals->get_props()->getStringValue(name, defaultValue);
}

bool
fgSetBool (const char * name, bool val)
{
  return globals->get_props()->setBoolValue(name, val);
}

bool
fgSetInt (const char * name, int val)
{
  return globals->get_props()->setIntValue(name, val);
}

bool
fgSetLong (const char * name, long val)
{
  return globals->get_props()->setLongValue(name, val);
}

bool
fgSetFloat (const char * name, float val)
{
  return globals->get_props()->setFloatValue(name, val);
}

bool
fgSetDouble (const char * name, double val)
{
  return globals->get_props()->setDoubleValue(name, val);
}

bool
fgSetString (const char * name, const char * val)
{
  return globals->get_props()->setStringValue(name, val);
}

void
fgSetArchivable (const char * name, bool state)
{
  SGPropertyNode * node = globals->get_props()->getNode(name);
  if (node == 0)
    SG_LOG(SG_GENERAL, SG_DEBUG,
	   "Attempt to set archive flag for non-existant property "
	   << name);
  else
    node->setAttribute(SGPropertyNode::ARCHIVE, state);
}

void
fgSetReadable (const char * name, bool state)
{
  SGPropertyNode * node = globals->get_props()->getNode(name);
  if (node == 0)
    SG_LOG(SG_GENERAL, SG_DEBUG,
	   "Attempt to set read flag for non-existant property "
	   << name);
  else
    node->setAttribute(SGPropertyNode::READ, state);
}

void
fgSetWritable (const char * name, bool state)
{
  SGPropertyNode * node = globals->get_props()->getNode(name);
  if (node == 0)
    SG_LOG(SG_GENERAL, SG_DEBUG,
	   "Attempt to set write flag for non-existant property "
	   << name);
  else
    node->setAttribute(SGPropertyNode::WRITE, state);
}

void
fgUntie(const char * name)
{
  SGPropertyNode* node = globals->get_props()->getNode(name);
  if (!node) {
    SG_LOG(SG_GENERAL, SG_WARN, "fgUntie: unknown property " << name);
    return;
  }
  
  if (!node->isTied()) {
    return;
  }
  
  if (!node->untie()) {
    SG_LOG(SG_GENERAL, SG_WARN, "Failed to untie property " << name);
  }
}


// end of fg_props.cxx
