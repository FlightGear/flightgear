// fg_commands.cxx - internal FGFS commands.

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>		// strcmp()

#include <simgear/compiler.h>

#include <string>
#include <fstream>

#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/timing/sg_time.hxx>
#include <Network/RemoteXMLRequest.hxx>

#include <FDM/flight.hxx>
#include <Aircraft/replay.hxx>
#include <Scripting/NasalSys.hxx>
#include <Airports/xmlloader.hxx>
#include <Network/HTTPClient.hxx>
#include <Viewer/viewmgr.hxx>
#include <Viewer/view.hxx>
#include <Environment/presets.hxx>
#include <Navaids/NavDataCache.hxx>
#include <GUI/gui.h>
#include <Main/sentryIntegration.hxx>

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_os.hxx"
#include "fg_commands.hxx"
#include "fg_props.hxx"
#include "globals.hxx"
#include "logger.hxx"
#include "util.hxx"
#include "main.hxx"
#include "positioninit.hxx"

#if FG_HAVE_GPERFTOOLS
# include <google/profiler.h>
#endif

using std::string;
using std::ifstream;
using std::ofstream;


////////////////////////////////////////////////////////////////////////
// Static helper functions.
////////////////////////////////////////////////////////////////////////


static inline SGPropertyNode *
get_prop (const SGPropertyNode * arg, SGPropertyNode * root)
{
    if (root != nullptr)
    {
        SGPropertyNode *rv = nullptr;
        rv = root->getNode(arg->getStringValue("property[0]", "/null"), true);
        if (rv == nullptr)
        {
            rv = root->getNode(arg->getStringValue("property[0]", "/null"), true);
            return fgGetNode(arg->getStringValue("property[0]", "/null"), true);
        }
        return rv;
    }
    return fgGetNode(arg->getStringValue("property[0]", "/null"), true);
}

static inline SGPropertyNode *
get_prop2 (const SGPropertyNode * arg, SGPropertyNode * root)
{
    if (root != nullptr)
    {
        SGPropertyNode *rv = nullptr;
        rv = root->getNode(arg->getStringValue("property[1]", "/null"), true);
        if (rv == nullptr)
        {
            rv = root->getNode(arg->getStringValue("property[1]", "/null"), true);
            return fgGetNode(arg->getStringValue("property[1]", "/null"), true);
        }
        return rv;
    }
    return fgGetNode(arg->getStringValue("property[1]", "/null"), true);
}


/**
 * Get a double value and split it as required.
 */
static void
split_value (double full_value, const char * mask,
             double * unmodifiable, double * modifiable)
{
    if (!strcmp("integer", mask)) {
        *modifiable = (full_value < 0 ? ceil(full_value) : floor (full_value));
        *unmodifiable = full_value - *modifiable;
    } else if (!strcmp("decimal", mask)) {
        *unmodifiable = (full_value < 0 ? ceil(full_value) : floor(full_value));
        *modifiable = full_value - *unmodifiable;
    } else {
        if (strcmp("all", mask))
            SG_LOG(SG_GENERAL, SG_ALERT, "Bad value " << mask << " for mask;"
                   << " assuming 'all'");
        *unmodifiable = 0;
        *modifiable = full_value;
    }
}


/**
 * Clamp or wrap a value as specified.
 */
static void
limit_value (double * value, const SGPropertyNode * arg)
{
    const SGPropertyNode * min_node = arg->getChild("min");
    const SGPropertyNode * max_node = arg->getChild("max");

    bool wrap = arg->getBoolValue("wrap");

    if (min_node == 0 || max_node == 0)
        wrap = false;

    if (wrap) {                 // wrap such that min <= x < max
        double min_val = min_node->getDoubleValue();
        double max_val = max_node->getDoubleValue();
        double resolution = arg->getDoubleValue("resolution");
        if (resolution > 0.0) {
            // snap to (min + N*resolution), taking special care to handle imprecision
            int n = (int)floor((*value - min_val) / resolution + 0.5);
            int steps = (int)floor((max_val - min_val) / resolution + 0.5);
            SG_NORMALIZE_RANGE(n, 0, steps);
            *value = min_val + resolution * n;
        } else {
            // plain circular wrapping
            SG_NORMALIZE_RANGE(*value, min_val, max_val);
        }
    } else {                    // clamp such that min <= x <= max
        if ((min_node != 0) && (*value < min_node->getDoubleValue()))
            *value = min_node->getDoubleValue();
        else if ((max_node != 0) && (*value > max_node->getDoubleValue()))
            *value = max_node->getDoubleValue();
    }
}

static bool
compare_values (SGPropertyNode * value1, SGPropertyNode * value2)
{
    switch (value1->getType()) {
    case simgear::props::BOOL:
        return (value1->getBoolValue() == value2->getBoolValue());
    case simgear::props::INT:
        return (value1->getIntValue() == value2->getIntValue());
    case simgear::props::LONG:
        return (value1->getLongValue() == value2->getLongValue());
    case simgear::props::FLOAT:
        return (value1->getFloatValue() == value2->getFloatValue());
    case simgear::props::DOUBLE:
        return (value1->getDoubleValue() == value2->getDoubleValue());
    default:
        return !strcmp(value1->getStringValue(), value2->getStringValue());
    }
}



////////////////////////////////////////////////////////////////////////
// Command implementations.
////////////////////////////////////////////////////////////////////////


/**
 * Built-in command: do nothing.
 */
static bool
do_null (const SGPropertyNode * arg, SGPropertyNode * root)
{
  return true;
}

/**
 * Built-in command: run a Nasal script.
 */
static bool
do_nasal (const SGPropertyNode * arg, SGPropertyNode * root)
{
    return ((FGNasalSys*)globals->get_subsystem("nasal"))->handleCommand(arg, root);
}

/**
 * Built-in command: replay the FDR buffer
 */
static bool
do_replay (const SGPropertyNode * arg, SGPropertyNode * root)
{
    FGReplay *r = (FGReplay *)(globals->get_subsystem( "replay" ));
    return r->start();
}

/**
 * Built-in command: pause/unpause the sim
 */
static bool
do_pause (const SGPropertyNode * arg, SGPropertyNode * root)
{
    bool forcePause = arg->getBoolValue("force-pause", false );
    bool forcePlay = arg->getBoolValue("force-play", false );

    bool paused = fgGetBool("/sim/freeze/master",true) || fgGetBool("/sim/freeze/clock",true);

    if(forcePause) paused = false;
    if(forcePlay) paused = true;

    if (paused && (fgGetInt("/sim/freeze/replay-state",0)>0))
    {
        do_replay(NULL, nullptr);
    }
    else
    {
        fgSetBool("/sim/freeze/master",!paused);
        fgSetBool("/sim/freeze/clock",!paused);
    }

    syncPausePopupState();
    return true;
}

/**
 * Built-in command: load flight.
 *
 * file (optional): the name of the file to load (relative to current
 *   directory).  Defaults to "fgfs.sav"
 */
static bool
do_load (const SGPropertyNode * arg, SGPropertyNode * root)
{
    SGPath file(arg->getStringValue("file", "fgfs.sav"));

    if (file.extension() != "sav")
        file.concat(".sav");

    SGPath validated_path = fgValidatePath(file, false);
    if (validated_path.isNull()) {
        SG_LOG(SG_IO, SG_ALERT, "load: reading '" << file << "' denied "
                "(unauthorized access)");
        return false;
    }

    sg_ifstream input(validated_path);
    if (input.good() && fgLoadFlight(input)) {
        input.close();
        SG_LOG(SG_INPUT, SG_INFO, "Restored flight from " << file);
        return true;
    } else {
        SG_LOG(SG_INPUT, SG_WARN, "Cannot load flight from " << file);
        return false;
    }
}


/**
 * Built-in command: save flight.
 *
 * file (optional): the name of the file to save (relative to the
 * current directory).  Defaults to "fgfs.sav".
 */
static bool
do_save (const SGPropertyNode * arg, SGPropertyNode * root)
{
    SGPath file(arg->getStringValue("file", "fgfs.sav"));

    if (file.extension() != "sav")
        file.concat(".sav");

    SGPath validated_path = fgValidatePath(file, true);
    if (validated_path.isNull()) {
        SG_LOG(SG_IO, SG_ALERT, "save: writing '" << file << "' denied "
                "(unauthorized access)");
        return false;
    }

    bool write_all = arg->getBoolValue("write-all", false);
    SG_LOG(SG_INPUT, SG_INFO, "Saving flight");
    sg_ofstream output(validated_path);
    if (output.good() && fgSaveFlight(output, write_all)) {
        output.close();
        SG_LOG(SG_INPUT, SG_INFO, "Saved flight to " << file);
        return true;
    } else {
        SG_LOG(SG_INPUT, SG_ALERT, "Cannot save flight to " << file);
        return false;
    }
}

/**
 * Built-in command: save flight recorder tape.
 *
 */
static bool
do_save_tape (const SGPropertyNode * arg, SGPropertyNode * root)
{
    FGReplay* replay = (FGReplay*) globals->get_subsystem("replay");
    replay->saveTape(arg);

    return true;
}
/**
 * Built-in command: load flight recorder tape.
 *
 */
static bool
do_load_tape (const SGPropertyNode * arg, SGPropertyNode * root)
{
    FGReplay* replay = (FGReplay*) globals->get_subsystem("replay");
    replay->loadTape(arg);

    return true;
}

static void
do_view_next(bool do_it)
{
  // Only switch view if really requested to do so (and not for example while
  // reset/reposition where /command/view/next is set to false)
  if( do_it )
  {
    globals->get_current_view()->setHeadingOffset_deg(0.0);
    globals->get_viewmgr()->next_view();
  }
}

static void
do_view_prev(bool do_it)
{
  if( do_it )
  {
    globals->get_current_view()->setHeadingOffset_deg(0.0);
    globals->get_viewmgr()->prev_view();
  }
}

/**
 * Built-in command: cycle view.
 */
static bool
do_view_cycle (const SGPropertyNode * arg, SGPropertyNode * root)
{
  globals->get_current_view()->setHeadingOffset_deg(0.0);
  globals->get_viewmgr()->next_view();
  return true;
}


/**
 * Built-in command: view-push.
 */
static bool
do_view_push (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SG_LOG(SG_GENERAL, SG_ALERT, "do_view_push() called");
  globals->get_viewmgr()->view_push();
  return true;
}


/**
 * Built-in command: clone view.
 */
static bool
do_view_clone (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SG_LOG(SG_GENERAL, SG_ALERT, "do_view_clone() called");
  globals->get_viewmgr()->clone_current_view();
  return true;
}


/**
 * Built-in command: view last pair.
 */
static bool
do_view_last_pair (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SG_LOG(SG_GENERAL, SG_ALERT, "do_view_last_pair() called");
  globals->get_viewmgr()->clone_last_pair();
  return true;
}


/**
 * Built-in command: double view last pair.
 */
static bool
do_view_last_pair_double (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SG_LOG(SG_GENERAL, SG_ALERT, "do_view_last_pair_double() called");
  globals->get_viewmgr()->clone_last_pair_double();
  return true;
}

/**
 * Built-in command: toggle a bool property value.
 *
 * property: The name of the property to toggle.
 */
static bool
do_property_toggle (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SGPropertyNode * prop = get_prop(arg, root);
  return prop->setBoolValue(!prop->getBoolValue());
}


/**
 * Built-in command: assign a value to a property.
 *
 * property: the name of the property to assign.
 * value: the value to assign; or
 * property[1]: the property to copy from.
 */
static bool
do_property_assign (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SGPropertyNode * prop = get_prop(arg,root);
  const SGPropertyNode * value = arg->getNode("value");

  if (value != 0)
      return prop->setUnspecifiedValue(value->getStringValue());
  else
  {
      const SGPropertyNode * prop2 = get_prop2(arg,root);
      if (prop2)
          return prop->setUnspecifiedValue(prop2->getStringValue());
      else
          return false;
  }
}


/**
 * Built-in command: increment or decrement a property value.
 *
 * If the 'step' argument is present, it will be used; otherwise,
 * the command uses 'offset' and 'factor', usually from the mouse.
 *
 * property: the name of the property to increment or decrement.
 * step: the amount of the increment or decrement (default: 0).
 * offset: offset from the current setting (used for the mouse; multiplied
 *         by factor)
 * factor: scaling amount for the offset (defaults to 1).
 * min: the minimum allowed value (default: no minimum).
 * max: the maximum allowed value (default: no maximum).
 * mask: 'integer' to apply only to the left of the decimal point,
 *       'decimal' to apply only to the right of the decimal point,
 *       or 'all' to apply to the whole number (the default).
 * wrap: true if the value should be wrapped when it passes min or max;
 *       both min and max must be present for this to work (default:
 *       false).
 */
static bool
do_property_adjust (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SGPropertyNode * prop = get_prop(arg,root);

  double amount = 0;
  if (arg->hasValue("step"))
      amount = arg->getDoubleValue("step");
  else
      amount = (arg->getDoubleValue("factor", 1.0)
                * arg->getDoubleValue("offset"));

  double unmodifiable, modifiable;
  split_value(prop->getDoubleValue(), arg->getStringValue("mask", "all"),
              &unmodifiable, &modifiable);
  modifiable += amount;
  limit_value(&modifiable, arg);

  prop->setDoubleValue(unmodifiable + modifiable);

  return true;
}


/**
 * Built-in command: multiply a property value.
 *
 * property: the name of the property to multiply.
 * factor: the amount by which to multiply.
 * min: the minimum allowed value (default: no minimum).
 * max: the maximum allowed value (default: no maximum).
 * mask: 'integer' to apply only to the left of the decimal point,
 *       'decimal' to apply only to the right of the decimal point,
 *       or 'all' to apply to the whole number (the default).
 * wrap: true if the value should be wrapped when it passes min or max;
 *       both min and max must be present for this to work (default:
 *       false).
 */
static bool
do_property_multiply (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SGPropertyNode * prop = get_prop(arg,root);
  double factor = arg->getDoubleValue("factor", 1.0);

  double unmodifiable, modifiable;
  split_value(prop->getDoubleValue(), arg->getStringValue("mask", "all"),
              &unmodifiable, &modifiable);
  modifiable *= factor;
  limit_value(&modifiable, arg);

  prop->setDoubleValue(unmodifiable + modifiable);

  return true;
}


/**
 * Built-in command: swap two property values.
 *
 * property[0]: the name of the first property.
 * property[1]: the name of the second property.
 */
static bool
do_property_swap (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SGPropertyNode * prop1 = get_prop(arg,root);
  SGPropertyNode * prop2 = get_prop2(arg,root);

				// FIXME: inefficient
  const string & tmp = prop1->getStringValue();
  return (prop1->setUnspecifiedValue(prop2->getStringValue()) &&
	  prop2->setUnspecifiedValue(tmp.c_str()));
}


/**
 * Built-in command: Set a property to an axis or other moving input.
 *
 * property: the name of the property to set.
 * setting: the current input setting, usually between -1.0 and 1.0.
 * offset: the offset to shift by, before applying the factor.
 * factor: the factor to multiply by (use negative to reverse).
 */
static bool
do_property_scale (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SGPropertyNode * prop = get_prop(arg,root);
  double setting = arg->getDoubleValue("setting");
  double offset = arg->getDoubleValue("offset", 0.0);
  double factor = arg->getDoubleValue("factor", 1.0);
  bool squared = arg->getBoolValue("squared", false);
  int power = arg->getIntValue("power", (squared ? 2 : 1));

  int sign = (setting < 0 ? -1 : 1);

  switch (power) {
  case 1:
      break;
  case 2:
      setting = setting * setting * sign;
      break;
  case 3:
      setting = setting * setting * setting;
      break;
  case 4:
      setting = setting * setting * setting * setting * sign;
      break;
  default:
      setting =  pow(setting, power);
      if ((power % 2) == 0)
          setting *= sign;
      break;
  }

  return prop->setDoubleValue((setting + offset) * factor);
}


/**
 * Built-in command: cycle a property through a set of values.
 *
 * If the current value isn't in the list, the cycle will
 * (re)start from the beginning.
 *
 * property: the name of the property to cycle.
 * value[*]: the list of values to cycle through.
 */
static bool
do_property_cycle (const SGPropertyNode * arg, SGPropertyNode * root)
{
    SGPropertyNode * prop = get_prop(arg,root);
    std::vector<SGPropertyNode_ptr> values = arg->getChildren("value");

    bool wrap = arg->getBoolValue("wrap", true);
    // compatible with knob/pick animations
    int offset = arg->getIntValue("offset", 1);

    int selection = -1;
    int nSelections = values.size();

    if (nSelections < 1) {
        SG_LOG(SG_GENERAL, SG_ALERT, "No values for property-cycle");
        return false;
    }

                                // Try to find the current selection
    for (int i = 0; i < nSelections; i++) {
        if (compare_values(prop, values[i])) {
            selection = i;
            break;
        }
    }

    if (selection < 0) { // default to first selection
        selection = 0;
    } else {
        selection += offset;
        if (wrap) {
            selection = (selection + nSelections) % nSelections;
        } else {
            SG_CLAMP_RANGE(selection, 0, nSelections - 1);
        }
    }

    prop->setUnspecifiedValue(values[selection]->getStringValue());
    return true;
}


/**
 * Built-in command: randomize a numeric property value.
 *
 * property: the name of the property value to randomize.
 * min: the minimum allowed value.
 * max: the maximum allowed value.
 */
static bool
do_property_randomize (const SGPropertyNode * arg, SGPropertyNode * root)
{
    SGPropertyNode * prop = get_prop(arg,root);
    double min = arg->getDoubleValue("min", DBL_MIN);
    double max = arg->getDoubleValue("max", DBL_MAX);
    prop->setDoubleValue(sg_random() * (max - min) + min);
    return true;
}

/**
 * Built-in command: interpolate a property value over time
 *
 * property:        the name of the property value to interpolate.
 * type:            the interpolation type ("numeric", "color", etc.)
 * easing:          name of easing function (see http://easings.net/)
 * value[0..n]      any number of constant values to interpolate
 * time/rate[0..n]  time between each value, number of time elements must
 *                  match those of value elements. Instead of time also rate can
 *                  be used which automatically calculates the time to change
 *                  the property value at the given speed.
 * -or-
 * property[1..n+1] any number of target values taken from named properties
 * time/rate[0..n]  time between each value, number of time elements must
 *                  match those of value elements. Instead of time also rate can
 *                  be used which automatically calculates the time to change
 *                  the property value at the given speed.
 */
static bool
do_property_interpolate (const SGPropertyNode * arg, SGPropertyNode * root)
{
  SGPropertyNode * prop = get_prop(arg,root);
  if( !prop )
    return false;

  simgear::PropertyList time_nodes = arg->getChildren("time");
  simgear::PropertyList rate_nodes = arg->getChildren("rate");

  if( !time_nodes.empty() && !rate_nodes.empty() )
    // mustn't specify time and rate
    return false;

  simgear::PropertyList::size_type num_times = time_nodes.empty()
                                             ? rate_nodes.size()
                                             : time_nodes.size();

  simgear::PropertyList value_nodes = arg->getChildren("value");
  if( value_nodes.empty() )
  {
    simgear::PropertyList prop_nodes = arg->getChildren("property");

    // must have one more property node
    if( prop_nodes.size() != num_times + 1 )
      return false;

    value_nodes.reserve(num_times);
    for( size_t i = 1; i < prop_nodes.size(); ++i )
      value_nodes.push_back( fgGetNode(prop_nodes[i]->getStringValue()) );
  }

  // must match
  if( value_nodes.size() != num_times )
    return false;

  double_list deltas;
  deltas.reserve(num_times);

  if( !time_nodes.empty() )
  {
    for( size_t i = 0; i < num_times; ++i )
      deltas.push_back( time_nodes[i]->getDoubleValue() );
  }
  else
  {
    for( size_t i = 0; i < num_times; ++i )
    {
      // TODO calculate delta based on property type
      double delta = value_nodes[i]->getDoubleValue()
                   - ( i > 0
                     ? value_nodes[i - 1]->getDoubleValue()
                     : prop->getDoubleValue()
                     );
      deltas.push_back( fabs(delta / rate_nodes[i]->getDoubleValue()) );
    }
  }

  return prop->interpolate
  (
    arg->getStringValue("type", "numeric"),
    value_nodes,
    deltas,
    arg->getStringValue("easing", "linear")
  );
}

/**
 * Built-in command: reinit the data logging system based on the
 * current contents of the /logger tree.
 */
static bool
do_data_logging_commit (const SGPropertyNode * arg, SGPropertyNode * root)
{
    FGLogger *log = (FGLogger *)globals->get_subsystem("logger");
    log->reinit();
    return true;
}

/**
 * Built-in command: set log level (0 ... 7)
 */
static bool
do_log_level (const SGPropertyNode * arg, SGPropertyNode * root)
{
   sglog().setLogLevels( SG_ALL, (sgDebugPriority)arg->getIntValue() );

   return true;
}

/**
 * An fgcommand to allow loading of xml files via nasal,
 * the xml file's structure will be made available within
 * a property tree node defined under argument "targetnode",
 * or in the given argument tree under "data" otherwise.
 *
 * @param filename a string to hold the complete path & filename of an XML file
 * @param targetnode a string pointing to a location within the property tree
 * where to store the parsed XML file. If <targetnode> is undefined, then the
 * file contents are stored under a node <data> in the argument tree.
 */

static bool
do_load_xml_to_proptree(const SGPropertyNode * arg, SGPropertyNode * root)
{
    SGPath file(arg->getStringValue("filename"));
    if (file.isNull())
        return false;

    if (file.extension() != "xml")
        file.concat(".xml");

    // some Nasal uses loadxml to also speculatively probe for existence of
    // files. This flag allows us not to be noisy in the logs, in that case.
    const bool quiet = arg->getBoolValue("quiet", false);

    std::string icao = arg->getStringValue("icao");
    if (icao.empty()) {
        if (file.isRelative()) {
          SGPath absPath = globals->resolve_maybe_aircraft_path(file.utf8Str());
          if (!absPath.isNull())
              file = absPath;
          else
          {
              if (!quiet) {
                  SG_LOG(SG_IO, SG_ALERT, "loadxml: Cannot find XML property file '" << file << "'.");
              }
              return false;
          }
        }
    } else {
        if (!XMLLoader::findAirportData(icao, file.utf8Str(), file)) {
            if (!quiet) {
                SG_LOG(SG_IO, SG_INFO, "loadxml: failed to find airport data for " << file << " at ICAO:" << icao);
            }
          return false;
        }
    }

    if (!file.exists()) {
        if (!quiet) {
            SG_LOG(SG_IO, SG_WARN, "loadxml: no such file:" << file);
        }
        return false;
    }

    SGPath validated_path = fgValidatePath(file, false);
    if (validated_path.isNull()) {
        SG_LOG(SG_IO, quiet ? SG_DEV_WARN : SG_ALERT, "loadxml: reading '" << file << "' denied "
                                                                                      "(unauthorized directory - authorization no longer follows symlinks; to authorize reading additional directories, pass them to --allow-nasal-read)");
        return false;
    }

    SGPropertyNode *targetnode;
    if (arg->hasValue("targetnode"))
        targetnode = fgGetNode(arg->getStringValue("targetnode"), true);
    else
        targetnode = const_cast<SGPropertyNode *>(arg)->getNode("data", true);

    // don't report Sentry errors for Nasal-loaded XML, since it makes
    // for very noisy reports
    flightgear::sentryThreadReportXMLErrors(false);
    try {
        readProperties(validated_path, targetnode, true);
    } catch (const sg_exception &e) {
        SG_LOG(SG_IO, quiet ? SG_DEV_WARN : SG_WARN, "loadxml exception: " << e.getFormattedMessage());
        flightgear::sentryThreadReportXMLErrors(true);
        return false;
    }
    flightgear::sentryThreadReportXMLErrors(true);

    return true;
}

static bool
do_load_xml_from_url(const SGPropertyNode * arg, SGPropertyNode * root)
{
    FGHTTPClient* http = static_cast<FGHTTPClient*>(globals->get_subsystem("http"));
    if (!http) {
        SG_LOG(SG_IO, SG_ALERT, "xmlhttprequest: HTTP client not running");
        return false;
    }

    std::string url(arg->getStringValue("url"));
    if (url.empty())
        return false;

    SGPropertyNode *targetnode;
    if (arg->hasValue("targetnode"))
        targetnode = fgGetNode(arg->getStringValue("targetnode"), true);
    else
        targetnode = const_cast<SGPropertyNode *>(arg)->getNode("data", true);

    RemoteXMLRequest* req = new RemoteXMLRequest(url, targetnode);

    if (arg->hasChild("body"))
        req->setBodyData(arg->getChild("body"));

// connect up optional reporting properties
    if (arg->hasValue("complete"))
        req->setCompletionProp(fgGetNode(arg->getStringValue("complete"), true));
    if (arg->hasValue("failure"))
        req->setFailedProp(fgGetNode(arg->getStringValue("failure"), true));
    if (arg->hasValue("status"))
        req->setStatusProp(fgGetNode(arg->getStringValue("status"), true));

    http->makeRequest(req);
    return true;
}


/**
 * An fgcommand to allow saving of xml files via nasal,
 * the file's structure will be determined based on what's
 * encountered in the passed (source) property tree node
 *
 * @param filename a string to hold the complete path & filename of the (new)
 * XML file
 * @param sourcenode a string pointing to a location within the property tree
 * where to find the nodes that should be written recursively into an XML file
 * @param data if no sourcenode is given, then the file contents are taken from
 * the argument tree's "data" node.
 */

static bool
do_save_xml_from_proptree(const SGPropertyNode * arg, SGPropertyNode * root)
{
    SGPath file(arg->getStringValue("filename"));
    if (file.isNull())
        return false;

    if (file.extension() != "xml")
        file.concat(".xml");

    SGPath validated_path = fgValidatePath(file, true);
    if (validated_path.isNull()) {
        SG_LOG(SG_IO, SG_ALERT, "savexml: writing to '" << file << "' denied "
                "(unauthorized directory - authorization no longer follows symlinks)");
        return false;
    }

    SGPropertyNode *sourcenode;
    if (arg->hasValue("sourcenode"))
        sourcenode = fgGetNode(arg->getStringValue("sourcenode"), true);
    else if (arg->getNode("data", false))
        sourcenode = const_cast<SGPropertyNode *>(arg)->getNode("data");
    else
        return false;

    try {
        writeProperties (validated_path, sourcenode, true);
    } catch (const sg_exception &e) {
        SG_LOG(SG_IO, SG_WARN, "savexml: " << e.getFormattedMessage());
        return false;
    }

    return true;
}

// Optional profiling commands using gperftools:
// http://code.google.com/p/gperftools/

#if !FG_HAVE_GPERFTOOLS
static void
no_profiling_support()
{
  SG_LOG
  (
    SG_GENERAL,
    SG_ALERT,
    "No profiling support! Install gperftools and reconfigure/rebuild fgfs."
  );
}
#endif

static bool
do_profiler_start(const SGPropertyNode *arg, SGPropertyNode *root)
{
#if FG_HAVE_GPERFTOOLS
  const char *filename = arg->getStringValue("filename", "fgfs.profile");
  ProfilerStart(filename);
  return true;
#else
  no_profiling_support();
  return false;
#endif
}

static bool
do_profiler_stop(const SGPropertyNode *arg, SGPropertyNode *root)
{
#if FG_HAVE_GPERFTOOLS
  ProfilerStop();
  return true;
#else
  no_profiling_support();
  return false;
#endif
}


////////////////////////////////////////////////////////////////////////
// Command setup.
////////////////////////////////////////////////////////////////////////


/**
 * Table of built-in commands.
 *
 * New commands do not have to be added here; any module in the application
 * can add a new command using globals->get_commands()->addCommand(...).
 */
static struct {
  const char * name;
  SGCommandMgr::command_t command;
} built_ins [] = {
    { "null", do_null },
    { "nasal", do_nasal },
    { "pause", do_pause },
    { "load", do_load },
    { "save", do_save },
    { "save-tape", do_save_tape },
    { "load-tape", do_load_tape },
    { "view-cycle", do_view_cycle },
    { "view-push", do_view_push },
    { "view-clone", do_view_clone },
    { "view-last-pair", do_view_last_pair },
    { "view-last-pair-double", do_view_last_pair_double },
    /*
    { "set-sea-level-air-temp-degc", do_set_sea_level_degc },
    { "set-outside-air-temp-degc", do_set_oat_degc },
    { "set-dewpoint-sea-level-air-temp-degc", do_set_dewpoint_sea_level_degc },
    { "set-dewpoint-temp-degc", do_set_dewpoint_degc },
    */
    { "property-toggle", do_property_toggle },
    { "property-assign", do_property_assign },
    { "property-adjust", do_property_adjust },
    { "property-multiply", do_property_multiply },
    { "property-swap", do_property_swap },
    { "property-scale", do_property_scale },
    { "property-cycle", do_property_cycle },
    { "property-randomize", do_property_randomize },
    { "property-interpolate", do_property_interpolate },
    { "data-logging-commit", do_data_logging_commit },
    { "log-level", do_log_level },
    { "replay", do_replay },
    /*
    { "decrease-visibility", do_decrease_visibility },
    { "increase-visibility", do_increase_visibility },
    */
    { "loadxml", do_load_xml_to_proptree},
    { "savexml", do_save_xml_from_proptree },
    { "xmlhttprequest", do_load_xml_from_url },

    { "profiler-start", do_profiler_start },
    { "profiler-stop",  do_profiler_stop },

    { 0, 0 }			// zero-terminated
};


/**
 * Initialize the default built-in commands.
 *
 * Other commands may be added by other parts of the application.
 */
void
fgInitCommands ()
{
  // set our property root as the implicit default root for the
  // command managr
  SGCommandMgr::instance()->setImplicitRoot(globals->get_props());
    
  SG_LOG(SG_GENERAL, SG_BULK, "Initializing basic built-in commands:");
  for (int i = 0; built_ins[i].name != 0; i++) {
    SG_LOG(SG_GENERAL, SG_BULK, "  " << built_ins[i].name);
    globals->get_commands()->addCommand(built_ins[i].name,
					built_ins[i].command);
  }

  typedef bool (*dummy)();
  fgTie( "/command/view/next", dummy(0), do_view_next );
  fgTie( "/command/view/prev", dummy(0), do_view_prev );

  globals->get_props()->setValueReadOnly( "/sim/debug/profiler-available",
                                          bool(FG_HAVE_GPERFTOOLS) );
}

// end of fg_commands.cxx
