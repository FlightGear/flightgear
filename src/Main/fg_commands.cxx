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
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/sound/soundmgr_openal.hxx>
#include <simgear/timing/sg_time.hxx>
#include <simgear/misc/interpolator.hxx>
#include <simgear/io/HTTPRequest.hxx>

#include <FDM/flight.hxx>
#include <GUI/gui.h>
#include <GUI/new_gui.hxx>
#include <GUI/dialog.hxx>
#include <Aircraft/replay.hxx>
#include <Scenery/scenery.hxx>
#include <Scripting/NasalSys.hxx>
#include <Sound/sample_queue.hxx>
#include <Airports/xmlloader.hxx>
#include <Network/HTTPClient.hxx>
#include <Viewer/viewmgr.hxx>
#include <Viewer/viewer.hxx>
#include <Environment/presets.hxx>

#include "fg_init.hxx"
#include "fg_io.hxx"
#include "fg_os.hxx"
#include "fg_commands.hxx"
#include "fg_props.hxx"
#include "globals.hxx"
#include "logger.hxx"
#include "util.hxx"
#include "main.hxx"

#include <boost/scoped_array.hpp>

using std::string;
using std::ifstream;
using std::ofstream;



////////////////////////////////////////////////////////////////////////
// Static helper functions.
////////////////////////////////////////////////////////////////////////


static inline SGPropertyNode *
get_prop (const SGPropertyNode * arg)
{
    return fgGetNode(arg->getStringValue("property[0]", "/null"), true);
}

static inline SGPropertyNode *
get_prop2 (const SGPropertyNode * arg)
{
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
do_null (const SGPropertyNode * arg)
{
  return true;
}

/**
 * Built-in command: run a Nasal script.
 */
static bool
do_nasal (const SGPropertyNode * arg)
{
    return ((FGNasalSys*)globals->get_subsystem("nasal"))->handleCommand(arg);
}

/**
 * Built-in command: exit FlightGear.
 *
 * status: the exit status to return to the operating system (defaults to 0)
 */
static bool
do_exit (const SGPropertyNode * arg)
{
    SG_LOG(SG_INPUT, SG_INFO, "Program exit requested.");
    fgSetBool("/sim/signals/exit", true);
    globals->saveUserSettings();
    fgOSExit(arg->getIntValue("status", 0));
    return true;
}


/**
 * Reset FlightGear (Shift-Escape or Menu->File->Reset)
 */
static bool
do_reset (const SGPropertyNode * arg)
{
    fgReInitSubsystems();
    return true;
}


/**
 * Built-in command: reinitialize one or more subsystems.
 *
 * subsystem[*]: the name(s) of the subsystem(s) to reinitialize; if
 * none is specified, reinitialize all of them.
 */
static bool
do_reinit (const SGPropertyNode * arg)
{
    bool result = true;

    vector<SGPropertyNode_ptr> subsystems = arg->getChildren("subsystem");
    if (subsystems.size() == 0) {
        globals->get_subsystem_mgr()->reinit();
    } else {
        for ( unsigned int i = 0; i < subsystems.size(); i++ ) {
            const char * name = subsystems[i]->getStringValue();
            SGSubsystem * subsystem = globals->get_subsystem(name);
            if (subsystem == 0) {
                result = false;
                SG_LOG( SG_GENERAL, SG_ALERT,
                        "Subsystem " << name << " not found" );
            } else {
                subsystem->reinit();
            }
        }
    }

    globals->get_event_mgr()->reinit();

    return result;
}

#if 0
  //
  // these routines look useful ??? but are never used in the code ???
  //

/**
 * Built-in command: suspend one or more subsystems.
 *
 * subsystem[*] - the name(s) of the subsystem(s) to suspend.
 */
static bool
do_suspend (const SGPropertyNode * arg)
{
    bool result = true;

    vector<SGPropertyNode_ptr> subsystems = arg->getChildren("subsystem");
    for ( unsigned int i = 0; i < subsystems.size(); i++ ) {
        const char * name = subsystems[i]->getStringValue();
        SGSubsystem * subsystem = globals->get_subsystem(name);
        if (subsystem == 0) {
            result = false;
            SG_LOG(SG_GENERAL, SG_ALERT, "Subsystem " << name << " not found");
        } else {
            subsystem->suspend();
        }
    }
    return result;
}

/**
 * Built-in command: suspend one or more subsystems.
 *
 * subsystem[*] - the name(s) of the subsystem(s) to suspend.
 */
static bool
do_resume (const SGPropertyNode * arg)
{
    bool result = true;

    vector<SGPropertyNode_ptr> subsystems = arg->getChildren("subsystem");
    for ( unsigned int i = 0; i < subsystems.size(); i++ ) {
        const char * name = subsystems[i]->getStringValue();
        SGSubsystem * subsystem = globals->get_subsystem(name);
        if (subsystem == 0) {
            result = false;
            SG_LOG(SG_GENERAL, SG_ALERT, "Subsystem " << name << " not found");
        } else {
            subsystem->resume();
        }
    }
    return result;
}

#endif

/**
 * Built-in command: replay the FDR buffer
 */
static bool
do_replay (const SGPropertyNode * arg)
{
    FGReplay *r = (FGReplay *)(globals->get_subsystem( "replay" ));
    return r->start();
}

/**
 * Built-in command: pause/unpause the sim
 */
static bool
do_pause (const SGPropertyNode * arg)
{
    bool paused = fgGetBool("/sim/freeze/master",true) || fgGetBool("/sim/freeze/clock",true);
    if (paused && (fgGetInt("/sim/freeze/replay-state",0)>0))
    {
        do_replay(NULL);
    }
    else
    {
        fgSetBool("/sim/freeze/master",!paused);
        fgSetBool("/sim/freeze/clock",!paused);
    }
    return true;
}

/**
 * Built-in command: load flight.
 *
 * file (optional): the name of the file to load (relative to current
 *   directory).  Defaults to "fgfs.sav"
 */
static bool
do_load (const SGPropertyNode * arg)
{
    string file = arg->getStringValue("file", "fgfs.sav");
    if (file.size() < 4 || file.substr(file.size() - 4) != ".sav")
        file += ".sav";

    if (!fgValidatePath(file.c_str(), false)) {
        SG_LOG(SG_IO, SG_ALERT, "load: reading '" << file << "' denied "
                "(unauthorized access)");
        return false;
    }

    ifstream input(file.c_str());
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
do_save (const SGPropertyNode * arg)
{
    string file = arg->getStringValue("file", "fgfs.sav");
    if (file.size() < 4 || file.substr(file.size() - 4) != ".sav")
        file += ".sav";

    if (!fgValidatePath(file.c_str(), false)) {
        SG_LOG(SG_IO, SG_ALERT, "save: writing '" << file << "' denied "
                "(unauthorized access)");
        return false;
    }

    bool write_all = arg->getBoolValue("write-all", false);
    SG_LOG(SG_INPUT, SG_INFO, "Saving flight");
    ofstream output(file.c_str());
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
 * Built-in command: (re)load the panel.
 *
 * path (optional): the file name to load the panel from 
 * (relative to FG_ROOT).  Defaults to the value of /sim/panel/path,
 * and if that's unspecified, to "Panels/Default/default.xml".
 */
static bool
do_panel_load (const SGPropertyNode * arg)
{
  string panel_path = arg->getStringValue("path");
  if (!panel_path.empty()) {
    // write to the standard property, which will force a load
    fgSetString("/sim/panel/path", panel_path.c_str());
  }
  
  return true;
}

/**
 * Built-in command: (re)load preferences.
 *
 * path (optional): the file name to load the panel from (relative
 * to FG_ROOT). Defaults to "preferences.xml".
 */
static bool
do_preferences_load (const SGPropertyNode * arg)
{
  try {
    fgLoadProps(arg->getStringValue("path", "preferences.xml"),
                globals->get_props());
  } catch (const sg_exception &e) {
    guiErrorMessage("Error reading global preferences: ", e);
    return false;
  }
  SG_LOG(SG_INPUT, SG_INFO, "Successfully read global preferences.");
  return true;
}

static void
do_view_next( bool )
{
    globals->get_current_view()->setHeadingOffset_deg(0.0);
    globals->get_viewmgr()->next_view();
}

static void
do_view_prev( bool )
{
    globals->get_current_view()->setHeadingOffset_deg(0.0);
    globals->get_viewmgr()->prev_view();
}

/**
 * Built-in command: cycle view.
 */
static bool
do_view_cycle (const SGPropertyNode * arg)
{
  globals->get_current_view()->setHeadingOffset_deg(0.0);
  globals->get_viewmgr()->next_view();
  return true;
}

/**
 * Built-in command: capture screen.
 */
static bool
do_screen_capture (const SGPropertyNode * arg)
{
  return fgDumpSnapShot();
}

static bool
do_reload_shaders (const SGPropertyNode*)
{
    simgear::reload_shaders();
    return true;
}

static bool
do_dump_scene_graph (const SGPropertyNode*)
{
    fgDumpSceneGraph();
    return true;
}

static bool
do_dump_terrain_branch (const SGPropertyNode*)
{
    fgDumpTerrainBranch();

    double lon_deg = fgGetDouble("/position/longitude-deg");
    double lat_deg = fgGetDouble("/position/latitude-deg");
    SGGeod geodPos = SGGeod::fromDegFt(lon_deg, lat_deg, 0.0);
    SGVec3d zero = SGVec3d::fromGeod(geodPos);

    SG_LOG(SG_INPUT, SG_INFO, "Model parameters:");
    SG_LOG(SG_INPUT, SG_INFO, "Center: " << zero.x() << ", " << zero.y() << ", " << zero.z() );
    SG_LOG(SG_INPUT, SG_INFO, "Rotation: " << lat_deg << ", " << lon_deg );

    return true;
}

static bool
do_print_visible_scene_info(const SGPropertyNode*)
{
    fgPrintVisibleSceneInfoCommand();
    return true;
}

/**
 * Built-in command: hires capture screen.
 */
static bool
do_hires_screen_capture (const SGPropertyNode * arg)
{
  fgHiResDump();
  return true;
}


/**
 * Reload the tile cache.
 */
static bool
do_tile_cache_reload (const SGPropertyNode * arg)
{
    static const SGPropertyNode *master_freeze
	= fgGetNode("/sim/freeze/master");
    bool freeze = master_freeze->getBoolValue();
    SG_LOG(SG_INPUT, SG_INFO, "ReIniting TileCache");
    if ( !freeze ) {
	fgSetBool("/sim/freeze/master", true);
    }

    globals->get_subsystem("tile-manager")->reinit();

    if ( !freeze ) {
	fgSetBool("/sim/freeze/master", false);
    }
    return true;
}


#if 0
These do_set_(some-environment-parameters) are deprecated and no longer 
useful/functional - Torsten Dreyer, January 2011
/**
 * Set the sea level outside air temperature and assigning that to all
 * boundary and aloft environment layers.
 */
static bool
do_set_sea_level_degc ( double temp_sea_level_degc)
{
    SGPropertyNode *node, *child;

    // boundary layers
    node = fgGetNode( "/environment/config/boundary" );
    if ( node != NULL ) {
      int i = 0;
      while ( ( child = node->getNode( "entry", i ) ) != NULL ) {
	child->setDoubleValue( "temperature-sea-level-degc",
			       temp_sea_level_degc );
	++i;
      }
    }

    // aloft layers
    node = fgGetNode( "/environment/config/aloft" );
    if ( node != NULL ) {
      int i = 0;
      while ( ( child = node->getNode( "entry", i ) ) != NULL ) {
	child->setDoubleValue( "temperature-sea-level-degc",
			       temp_sea_level_degc );
	++i;
      }
    }

    return true;
}

static bool
do_set_sea_level_degc (const SGPropertyNode * arg)
{
    return do_set_sea_level_degc( arg->getDoubleValue("temp-degc", 15.0) );
}


/**
 * Set the outside air temperature at the "current" altitude by first
 * calculating the corresponding sea level temp, and assigning that to
 * all boundary and aloft environment layers.
 */
static bool
do_set_oat_degc (const SGPropertyNode * arg)
{
    double oat_degc = arg->getDoubleValue("temp-degc", 15.0);
    // check for an altitude specified in the arguments, otherwise use
    // current aircraft altitude.
    const SGPropertyNode *altitude_ft = arg->getChild("altitude-ft");
    if ( altitude_ft == NULL ) {
        altitude_ft = fgGetNode("/position/altitude-ft");
    }

    FGEnvironment dummy;	// instantiate a dummy so we can leech a method
    dummy.set_elevation_ft( altitude_ft->getDoubleValue() );
    dummy.set_temperature_degc( oat_degc );
    return do_set_sea_level_degc( dummy.get_temperature_sea_level_degc());
}

/**
 * Set the sea level outside air dewpoint and assigning that to all
 * boundary and aloft environment layers.
 */
static bool
do_set_dewpoint_sea_level_degc (double dewpoint_sea_level_degc)
{

    SGPropertyNode *node, *child;

    // boundary layers
    node = fgGetNode( "/environment/config/boundary" );
    if ( node != NULL ) {
      int i = 0;
      while ( ( child = node->getNode( "entry", i ) ) != NULL ) {
	child->setDoubleValue( "dewpoint-sea-level-degc",
			       dewpoint_sea_level_degc );
	++i;
      }
    }

    // aloft layers
    node = fgGetNode( "/environment/config/aloft" );
    if ( node != NULL ) {
      int i = 0;
      while ( ( child = node->getNode( "entry", i ) ) != NULL ) {
	child->setDoubleValue( "dewpoint-sea-level-degc",
			       dewpoint_sea_level_degc );
	++i;
      }
    }

    return true;
}

static bool
do_set_dewpoint_sea_level_degc (const SGPropertyNode * arg)
{
    return do_set_dewpoint_sea_level_degc(arg->getDoubleValue("dewpoint-degc", 5.0));
}

/**
 * Set the outside air dewpoint at the "current" altitude by first
 * calculating the corresponding sea level dewpoint, and assigning
 * that to all boundary and aloft environment layers.
 */
static bool
do_set_dewpoint_degc (const SGPropertyNode * arg)
{
    double dewpoint_degc = arg->getDoubleValue("dewpoint-degc", 5.0);

    // check for an altitude specified in the arguments, otherwise use
    // current aircraft altitude.
    const SGPropertyNode *altitude_ft = arg->getChild("altitude-ft");
    if ( altitude_ft == NULL ) {
        altitude_ft = fgGetNode("/position/altitude-ft");
    }

    FGEnvironment dummy;	// instantiate a dummy so we can leech a method
    dummy.set_elevation_ft( altitude_ft->getDoubleValue() );
    dummy.set_dewpoint_degc( dewpoint_degc );
    return do_set_dewpoint_sea_level_degc(dummy.get_dewpoint_sea_level_degc());
}
#endif

/**
 * Built-in command: toggle a bool property value.
 *
 * property: The name of the property to toggle.
 */
static bool
do_property_toggle (const SGPropertyNode * arg)
{
  SGPropertyNode * prop = get_prop(arg);
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
do_property_assign (const SGPropertyNode * arg)
{
  SGPropertyNode * prop = get_prop(arg);
  const SGPropertyNode * value = arg->getNode("value");

  if (value != 0)
      return prop->setUnspecifiedValue(value->getStringValue());
  else
  {
      const SGPropertyNode * prop2 = get_prop2(arg);
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
do_property_adjust (const SGPropertyNode * arg)
{
  SGPropertyNode * prop = get_prop(arg);

  double amount = 0;
  if (arg->hasValue("step"))
      amount = arg->getDoubleValue("step");
  else
      amount = (arg->getDoubleValue("factor", 1)
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
do_property_multiply (const SGPropertyNode * arg)
{
  SGPropertyNode * prop = get_prop(arg);
  double factor = arg->getDoubleValue("factor", 1);

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
do_property_swap (const SGPropertyNode * arg)
{
  SGPropertyNode * prop1 = get_prop(arg);
  SGPropertyNode * prop2 = get_prop2(arg);

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
do_property_scale (const SGPropertyNode * arg)
{
  SGPropertyNode * prop = get_prop(arg);
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
do_property_cycle (const SGPropertyNode * arg)
{
    SGPropertyNode * prop = get_prop(arg);
    vector<SGPropertyNode_ptr> values = arg->getChildren("value");
    int selection = -1;
    int nSelections = values.size();

    if (nSelections < 1) {
        SG_LOG(SG_GENERAL, SG_ALERT, "No values for property-cycle");
        return false;
    }

                                // Try to find the current selection
    for (int i = 0; i < nSelections; i++) {
        if (compare_values(prop, values[i])) {
            selection = i + 1;
            break;
        }
    }

                                // Default or wrap to the first selection
    if (selection < 0 || selection >= nSelections)
        selection = 0;

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
do_property_randomize (const SGPropertyNode * arg)
{
    SGPropertyNode * prop = get_prop(arg);
    double min = arg->getDoubleValue("min", DBL_MIN);
    double max = arg->getDoubleValue("max", DBL_MAX);
    prop->setDoubleValue(sg_random() * (max - min) + min);
    return true;
}

/**
 * Built-in command: interpolate a property value over time
 *
 * property: the name of the property value to interpolate.
 * value[0..n] any number of constant values to interpolate
 * time[0..n]  time between each value, number of time elements must
 *          match those of value elements
 * -or-
 * property[1..n] any number of target values taken from named properties
 * time[0..n]     time between each value, number of time elements must
 *                match those of property elements minus one
 */
static bool
do_property_interpolate (const SGPropertyNode * arg)
{
    SGPropertyNode * prop = get_prop(arg);

    simgear::PropertyList valueNodes = arg->getChildren( "value" );
    simgear::PropertyList timeNodes = arg->getChildren( "time" );

    boost::scoped_array<double> value;
    boost::scoped_array<double> time;

    if( valueNodes.size() > 0 ) {
        // must match
        if( timeNodes.size() != valueNodes.size() )
            return false;

        value.reset( new double[valueNodes.size()] );
        for( simgear::PropertyList::size_type n = 0; n < valueNodes.size(); n++ ) {
            value[n] = valueNodes[n]->getDoubleValue();
        }
    } else {
        valueNodes = arg->getChildren("property");
        // must have one more property node
        if( valueNodes.size() - 1 != timeNodes.size() )
          return false;

        value.reset( new double[valueNodes.size()-1] );
        for( simgear::PropertyList::size_type n = 0; n < valueNodes.size()-1; n++ ) {
            value[n] = fgGetNode(valueNodes[n+1]->getStringValue(), "/null")->getDoubleValue();
        }

    }

    time.reset( new double[timeNodes.size()] );
    for( simgear::PropertyList::size_type n = 0; n < timeNodes.size(); n++ ) {
        time[n] = timeNodes[n]->getDoubleValue();
    }

    ((SGInterpolator*)globals->get_subsystem_mgr()
      ->get_group(SGSubsystemMgr::INIT)->get_subsystem("interpolator"))
      ->interpolate(prop, timeNodes.size(), value.get(), time.get() );

    return true;
}

/**
 * Built-in command: reinit the data logging system based on the
 * current contents of the /logger tree.
 */
static bool
do_data_logging_commit (const SGPropertyNode * arg)
{
    FGLogger *log = (FGLogger *)globals->get_subsystem("logger");
    log->reinit();
    return true;
}

/**
 * Built-in command: Add a dialog to the GUI system.  Does *not*
 * display the dialog.  The property node should have the same format
 * as a dialog XML configuration.  It must include:
 *
 * name: the name of the GUI dialog for future reference.
 */
static bool
do_dialog_new (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");

    // Note the casting away of const: this is *real*.  Doing a
    // "dialog-apply" command later on will mutate this property node.
    // I'm not convinced that this isn't the Right Thing though; it
    // allows client to create a node, pass it to dialog-new, and get
    // the values back from the dialog by reading the same node.
    // Perhaps command arguments are not as "const" as they would
    // seem?
    gui->newDialog((SGPropertyNode*)arg);
    return true;
}

/**
 * Built-in command: Show an XML-configured dialog.
 *
 * dialog-name: the name of the GUI dialog to display.
 */
static bool
do_dialog_show (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    gui->showDialog(arg->getStringValue("dialog-name"));
    return true;
}


/**
 * Built-in Command: Hide the active XML-configured dialog.
 */
static bool
do_dialog_close (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    if(arg->hasValue("dialog-name"))
        return gui->closeDialog(arg->getStringValue("dialog-name"));
    return gui->closeActiveDialog();
}


/**
 * Update a value in the active XML-configured dialog.
 *
 * object-name: The name of the GUI object(s) (all GUI objects if omitted).
 */
static bool
do_dialog_update (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    FGDialog * dialog;
    if (arg->hasValue("dialog-name"))
        dialog = gui->getDialog(arg->getStringValue("dialog-name"));
    else
        dialog = gui->getActiveDialog();

    if (dialog != 0) {
        dialog->updateValues(arg->getStringValue("object-name"));
        return true;
    } else {
        return false;
    }
}

static bool
do_open_browser (const SGPropertyNode * arg)
{
    string path;
    if (arg->hasValue("path"))
        path = arg->getStringValue("path");
    else
    if (arg->hasValue("url"))
        path = arg->getStringValue("url");
    else
        return false;

    return openBrowser(path);
}

/**
 * Apply a value in the active XML-configured dialog.
 *
 * object-name: The name of the GUI object(s) (all GUI objects if omitted).
 */
static bool
do_dialog_apply (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    FGDialog * dialog;
    if (arg->hasValue("dialog-name"))
        dialog = gui->getDialog(arg->getStringValue("dialog-name"));
    else
        dialog = gui->getActiveDialog();

    if (dialog != 0) {
        dialog->applyValues(arg->getStringValue("object-name"));
        return true;
    } else {
        return false;
    }
}


/**
 * Redraw GUI (applying new widget colors). Doesn't reload the dialogs,
 * unlike reinit().
 */
static bool
do_gui_redraw (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem("gui");
    gui->redraw();
    return true;
}


/**
 * Adds model to the scenery. The path to the added branch (/models/model[*])
 * is returned in property "property".
 */
static bool
do_add_model (const SGPropertyNode * arg)
{
    SGPropertyNode * model = fgGetNode("models", true);
    int i;
    for (i = 0; model->hasChild("model",i); i++);
    model = model->getChild("model", i, true);
    copyProperties(arg, model);
    if (model->hasValue("elevation-m"))
        model->setDoubleValue("elevation-ft", model->getDoubleValue("elevation-m")
                * SG_METER_TO_FEET);
    model->getNode("load", true);
    model->removeChildren("load");
    const_cast<SGPropertyNode *>(arg)->setStringValue("property", model->getPath());
    return true;
}


/**
 * Set mouse cursor coordinates and cursor shape.
 */
static bool
do_set_cursor (const SGPropertyNode * arg)
{
    if (arg->hasValue("x") || arg->hasValue("y")) {
        SGPropertyNode *mx = fgGetNode("/devices/status/mice/mouse/x", true);
        SGPropertyNode *my = fgGetNode("/devices/status/mice/mouse/y", true);
        int x = arg->getIntValue("x", mx->getIntValue());
        int y = arg->getIntValue("y", my->getIntValue());
        fgWarpMouse(x, y);
        mx->setIntValue(x);
        my->setIntValue(y);
    }

    SGPropertyNode *cursor = const_cast<SGPropertyNode *>(arg)->getNode("cursor", true);
    if (cursor->getType() != simgear::props::NONE)
        fgSetMouseCursor(cursor->getIntValue());

    cursor->setIntValue(fgGetMouseCursor());
    return true;
}


/**
 * Built-in command: play an audio message (i.e. a wav file) This is
 * fire and forget.  Call this once per message and it will get dumped
 * into a queue.  Messages are played sequentially so they do not
 * overlap.
 */
static bool
do_play_audio_sample (const SGPropertyNode * arg)
{
    string path = arg->getStringValue("path");
    string file = arg->getStringValue("file");
    float volume = arg->getFloatValue("volume");
    // cout << "playing " << path << " / " << file << endl;
    try {
        static FGSampleQueue *queue = 0;
        if ( !queue ) {
           SGSoundMgr *smgr = globals->get_soundmgr();
           queue = new FGSampleQueue(smgr, "chatter");
           queue->tie_to_listener();
        }

        SGSoundSample *msg = new SGSoundSample(file.c_str(), path);
        msg->set_volume( volume );
        queue->add( msg );

        return true;

    } catch (const sg_io_exception&) {
        SG_LOG(SG_GENERAL, SG_ALERT, "play-audio-sample: "
                "failed to load" << path << '/' << file);
        return false;
    }
}

/**
 * Built-in command: commit presets (read from in /sim/presets/)
 */
static bool
do_presets_commit (const SGPropertyNode * arg)
{
    if (fgGetBool("/sim/initialized", false)) {
      fgReInitSubsystems();
    } else {
      // Nasal can trigger this during initial init, which confuses
      // the logic in ReInitSubsystems, since initial state has not been
      // saved at that time. Short-circuit everything here.
      fgInitPosition();
    }
    
    return true;
}

/**
 * Built-in command: set log level (0 ... 7)
 */
static bool
do_log_level (const SGPropertyNode * arg)
{
   sglog().setLogLevels( SG_ALL, (sgDebugPriority)arg->getIntValue() );

   return true;
}

/*
static bool
do_decrease_visibility (const SGPropertyNode * arg)
{
    Environment::Presets::VisibilitySingleton::instance()->adjust( 0.9 );
    return true;
}
 
static bool
do_increase_visibility (const SGPropertyNode * arg)
{
    Environment::Presets::VisibilitySingleton::instance()->adjust( 1.1 );
    return true;
}
*/
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
do_load_xml_to_proptree(const SGPropertyNode * arg)
{
    SGPath file(arg->getStringValue("filename"));
    if (file.str().empty())
        return false;

    if (file.extension() != "xml")
        file.concat(".xml");
    
    std::string icao = arg->getStringValue("icao");
    if (icao.empty()) {
        if (file.isRelative()) {
          SGPath absPath = globals->resolve_maybe_aircraft_path(file.str());
          if (!absPath.isNull())
              file = absPath;
          else
          {
              SG_LOG(SG_IO, SG_ALERT, "loadxml: Cannot find XML property file '"  
                          << file.str() << "'.");
              return false;
          }
        }
    } else {
        if (!XMLLoader::findAirportData(icao, file.str(), file)) {
          SG_LOG(SG_IO, SG_INFO, "loadxml: failed to find airport data for "
            << file.str() << " at ICAO:" << icao);
          return false;
        }
    }
    
    if (!fgValidatePath(file.c_str(), false)) {
        SG_LOG(SG_IO, SG_ALERT, "loadxml: reading '" << file.str() << "' denied "
                "(unauthorized access)");
        return false;
    }

    SGPropertyNode *targetnode;
    if (arg->hasValue("targetnode"))
        targetnode = fgGetNode(arg->getStringValue("targetnode"), true);
    else
        targetnode = const_cast<SGPropertyNode *>(arg)->getNode("data", true);

    try {
        readProperties(file.c_str(), targetnode, true);
    } catch (const sg_exception &e) {
        SG_LOG(SG_IO, SG_WARN, "loadxml: " << e.getFormattedMessage());
        return false;
    }

    return true;
}

class RemoteXMLRequest : public simgear::HTTP::Request
{
public:
    SGPropertyNode_ptr _complete;
    SGPropertyNode_ptr _status;
    SGPropertyNode_ptr _failed;
    SGPropertyNode_ptr _target;
    string propsData;
    
    RemoteXMLRequest(const std::string& url, SGPropertyNode* targetNode) : 
        simgear::HTTP::Request(url),
        _target(targetNode)
    {
    }
    
    void setCompletionProp(SGPropertyNode_ptr p)
    {
        _complete = p;
    }
    
    void setStatusProp(SGPropertyNode_ptr p)
    {
        _status = p;
    }
    
    void setFailedProp(SGPropertyNode_ptr p)
    {
        _failed = p;
    }
protected:
    virtual void gotBodyData(const char* s, int n)
    {
        propsData += string(s, n);
    }
    
    virtual void responseComplete()
    {
        int response = responseCode();
        bool failed = false;
        if (response == 200) {
            try {
                const char* buffer = propsData.c_str();
                readProperties(buffer, propsData.size(), _target, true);
            } catch (const sg_exception &e) {
                SG_LOG(SG_IO, SG_WARN, "parsing XML from remote, failed: " << e.getFormattedMessage());
                failed = true;
                response = 406; // 'not acceptable', anything better?
            }
        } else {
            failed = true;
        }
    // now the response data is output, signal Nasal / listeners
        if (_complete) _complete->setBoolValue(true);
        if (_status) _status->setIntValue(response);
        if (_failed) _failed->setBoolValue(failed);
    }
};


static bool
do_load_xml_from_url(const SGPropertyNode * arg)
{
    std::string url(arg->getStringValue("url"));
    if (url.empty())
        return false;
        
    SGPropertyNode *targetnode;
    if (arg->hasValue("targetnode"))
        targetnode = fgGetNode(arg->getStringValue("targetnode"), true);
    else
        targetnode = const_cast<SGPropertyNode *>(arg)->getNode("data", true);
    
    RemoteXMLRequest* req = new RemoteXMLRequest(url, targetnode);
    
// connect up optional reporting properties
    if (arg->hasValue("complete")) 
        req->setCompletionProp(fgGetNode(arg->getStringValue("complete"), true));
    if (arg->hasValue("failure")) 
        req->setFailedProp(fgGetNode(arg->getStringValue("failure"), true));
    if (arg->hasValue("status")) 
        req->setStatusProp(fgGetNode(arg->getStringValue("status"), true));
        
    FGHTTPClient::instance()->makeRequest(req);
    
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
do_save_xml_from_proptree(const SGPropertyNode * arg)
{
    SGPath file(arg->getStringValue("filename"));
    if (file.str().empty())
        return false;

    if (file.extension() != "xml")
        file.concat(".xml");

    if (!fgValidatePath(file.c_str(), true)) {
        SG_LOG(SG_IO, SG_ALERT, "savexml: writing to '" << file.str() << "' denied "
                "(unauthorized access)");
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
        writeProperties (file.c_str(), sourcenode, true);
    } catch (const sg_exception &e) {
        SG_LOG(SG_IO, SG_WARN, "savexml: " << e.getFormattedMessage());
        return false;
    }

    return true;
}

static bool
do_press_cockpit_button (const SGPropertyNode *arg)
{
  const char *prefix = arg->getStringValue("prefix");

  if (arg->getBoolValue("guarded") && fgGetDouble((string(prefix) + "-guard").c_str()) < 1)
    return true;

  string prop = string(prefix) + "-button";
  double value;

  if (arg->getBoolValue("latching"))
    value = fgGetDouble(prop.c_str()) > 0 ? 0 : 1;
  else
    value = 1;

  fgSetDouble(prop.c_str(), value);
  fgSetBool(arg->getStringValue("discrete"), value > 0);

  return true;
}

static bool
do_release_cockpit_button (const SGPropertyNode *arg)
{
  const char *prefix = arg->getStringValue("prefix");

  if (arg->getBoolValue("guarded")) {
    string prop = string(prefix) + "-guard";
    if (fgGetDouble(prop.c_str()) < 1) {
      fgSetDouble(prop.c_str(), 1);
      return true;
    }
  }

  if (! arg->getBoolValue("latching")) {
    fgSetDouble((string(prefix) + "-button").c_str(), 0);
    fgSetBool(arg->getStringValue("discrete"), false);
  }

  return true;
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
    { "exit", do_exit },
    { "reset", do_reset },
    { "reinit", do_reinit },
    { "suspend", do_reinit },
    { "resume", do_reinit },
    { "pause", do_pause },
    { "load", do_load },
    { "save", do_save },
    { "panel-load", do_panel_load },
    { "preferences-load", do_preferences_load },
    { "view-cycle", do_view_cycle },
    { "screen-capture", do_screen_capture },
    { "hires-screen-capture", do_hires_screen_capture },
    { "tile-cache-reload", do_tile_cache_reload },
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
    { "dialog-new", do_dialog_new },
    { "dialog-show", do_dialog_show },
    { "dialog-close", do_dialog_close },
    { "dialog-update", do_dialog_update },
    { "dialog-apply", do_dialog_apply },
    { "open-browser", do_open_browser },
    { "gui-redraw", do_gui_redraw },
    { "add-model", do_add_model },
    { "set-cursor", do_set_cursor },
    { "play-audio-sample", do_play_audio_sample },
    { "presets-commit", do_presets_commit },
    { "log-level", do_log_level },
    { "replay", do_replay },
    /*
    { "decrease-visibility", do_decrease_visibility },
    { "increase-visibility", do_increase_visibility },
    */
    { "loadxml", do_load_xml_to_proptree},
    { "savexml", do_save_xml_from_proptree },
    { "xmlhttprequest", do_load_xml_from_url },
    { "press-cockpit-button", do_press_cockpit_button },
    { "release-cockpit-button", do_release_cockpit_button },
    { "dump-scenegraph", do_dump_scene_graph },
    { "dump-terrainbranch", do_dump_terrain_branch },
    { "print-visible-scene", do_print_visible_scene_info },
    { "reload-shaders", do_reload_shaders },

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
  SG_LOG(SG_GENERAL, SG_BULK, "Initializing basic built-in commands:");
  for (int i = 0; built_ins[i].name != 0; i++) {
    SG_LOG(SG_GENERAL, SG_BULK, "  " << built_ins[i].name);
    globals->get_commands()->addCommand(built_ins[i].name,
					built_ins[i].command);
  }

  typedef bool (*dummy)();
  fgTie( "/command/view/next", dummy(0), do_view_next );
  fgTie( "/command/view/prev", dummy(0), do_view_prev );
}

// end of fg_commands.cxx
