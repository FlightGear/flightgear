// fg_commands.cxx - internal FGFS commands.

#include <string.h>		// strcmp()

#include <simgear/compiler.h>
#include <simgear/misc/exception.hxx>

#include STL_STRING
#include STL_FSTREAM

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/commands.hxx>
#include <simgear/misc/props.hxx>

#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include <FDM/flight.hxx>
#include <GUI/gui.h>
#include <GUI/new_gui.hxx>
#include <Scenery/tilemgr.hxx>
#include <Time/tmp.hxx>

#include "fg_init.hxx"
#include "fg_commands.hxx"

SG_USING_STD(string);
#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(ifstream);
SG_USING_STD(ofstream);
#endif

#include "fg_props.hxx"
#include "fg_io.hxx"
#include "globals.hxx"
#include "viewmgr.hxx"



////////////////////////////////////////////////////////////////////////
// Static helper functions.
////////////////////////////////////////////////////////////////////////


/**
 * Template function to wrap a value.
 */
template <class T>
static inline void
do_wrap (T * value, T min, T max)
{
    if (min >= max) {           // basic sanity check
        *value = min;
    } else {
        T range = max - min;
        while (*value < min)
            *value += range;
        while (*value > max)
            *value -= range;
    }
}

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
 * Built-in command: exit FlightGear.
 *
 * TODO: show a confirm dialog.
 */
static bool
do_exit (const SGPropertyNode * arg)
{
  SG_LOG(SG_INPUT, SG_ALERT, "Program exit requested.");
  ConfirmExitDialog();
  return true;
}


/**
 * Built-in command: load flight.
 *
 * file (optional): the name of the file to load (relative to current
 * directory).  Defaults to "fgfs.sav".
 */
static bool
do_load (const SGPropertyNode * arg)
{
  const string &file = arg->getStringValue("file", "fgfs.sav");
  ifstream input(file.c_str());
  if (input.good() && fgLoadFlight(input)) {
    input.close();
    SG_LOG(SG_INPUT, SG_INFO, "Restored flight from " << file);
    return true;
  } else {
    SG_LOG(SG_INPUT, SG_ALERT, "Cannot load flight from " << file);
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
  const string &file = arg->getStringValue("file", "fgfs.sav");
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
  string panel_path =
    arg->getStringValue("path",
			fgGetString("/sim/panel/path",
				    "Panels/Default/default.xml"));
  FGPanel * new_panel = fgReadPanel(panel_path);
  if (new_panel == 0) {
    SG_LOG(SG_INPUT, SG_ALERT,
	   "Error reading new panel from " << panel_path);
    return false;
  }
  SG_LOG(SG_INPUT, SG_INFO, "Loaded new panel from " << panel_path);
  current_panel->unbind();
  delete current_panel;
  current_panel = new_panel;
  current_panel->bind();
  return true;
}


/**
 * Built-in command: pass a mouse click to the panel.
 *
 * button: the mouse button number, zero-based.
 * is-down: true if the button is down, false if it is up.
 * x-pos: the x position of the mouse click.
 * y-pos: the y position of the mouse click.
 */
static bool
do_panel_mouse_click (const SGPropertyNode * arg)
{
  if (current_panel != 0)
    return current_panel
      ->doMouseAction(arg->getIntValue("button"),
		      arg->getBoolValue("is-down") ? PU_DOWN : PU_UP,
		      arg->getIntValue("x-pos"),
		      arg->getIntValue("y-pos"));
  else
    return false;
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
fix_hud_visibility()
{
  if ( !strcmp(fgGetString("/sim/flight-model"), "ada") ) {
      globals->get_props()->setBoolValue( "/sim/hud/visibility", true );
      if ( globals->get_viewmgr()->get_current() == 1 ) {
          globals->get_props()->setBoolValue( "/sim/hud/visibility", false );
      }
  }
}

void
do_view_next( bool )
{
    globals->get_current_view()->setHeadingOffset_deg(0.0);
    globals->get_viewmgr()->next_view();
    fix_hud_visibility();
  global_tile_mgr.refresh_view_timestamps();
}

void
do_view_prev( bool )
{
    globals->get_current_view()->setHeadingOffset_deg(0.0);
    globals->get_viewmgr()->prev_view();
    fix_hud_visibility();
  global_tile_mgr.refresh_view_timestamps();
}

/**
 * Built-in command: cycle view.
 */
static bool
do_view_cycle (const SGPropertyNode * arg)
{
  globals->get_current_view()->setHeadingOffset_deg(0.0);
  globals->get_viewmgr()->next_view();
  fix_hud_visibility();
  global_tile_mgr.refresh_view_timestamps();
//   fgReshape(fgGetInt("/sim/startup/xsize"), fgGetInt("/sim/startup/ysize"));
  return true;
}

/**
 * Built-in command: capture screen.
 */
static bool
do_screen_capture (const SGPropertyNode * arg)
{
  fgDumpSnapShot();
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
    // BusyCursor(0);
    if ( global_tile_mgr.init() ) {
	// Load the local scenery data
        double visibility_meters = fgGetDouble("/environment/visibility-m");
	global_tile_mgr.update( visibility_meters );
    } else {
	SG_LOG( SG_GENERAL, SG_ALERT, 
		"Error in Tile Manager initialization!" );
	exit(-1);
    }
    // BusyCursor(1);
    if ( !freeze ) {
	fgSetBool("/sim/freeze/master", false);
    }
    return true;
}


/**
 * Update the lighting manually.
 */
static bool
do_lighting_update (const SGPropertyNode * arg)
{
  fgUpdateSkyAndLightingParams();
  return true;
}


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
 * value: the value to assign.
 */
static bool
do_property_assign (const SGPropertyNode * arg)
{
  SGPropertyNode * prop = get_prop(arg);
  const SGPropertyNode * value = arg->getNode("value");

  switch (prop->getType()) {

  case SGPropertyNode::BOOL:
    return prop->setBoolValue(value->getBoolValue());

  case SGPropertyNode::INT:
    return prop->setIntValue(value->getIntValue());

  case SGPropertyNode::LONG:
    return prop->setLongValue(value->getLongValue());

  case SGPropertyNode::FLOAT:
    return prop->setFloatValue(value->getFloatValue());

  case SGPropertyNode::DOUBLE:
    return prop->setDoubleValue(value->getDoubleValue());

  case SGPropertyNode::STRING:
    return prop->setStringValue(value->getStringValue());

  default:
    return prop->setUnspecifiedValue(value->getStringValue());

  }
}


/**
 * Built-in command: increment or decrement a property value.
 *
 * property: the name of the property to increment or decrement.
 * step: the amount of the increment or decrement (default: 0).
 * offset: a normalized amount to offset by (if step is not present).
 * factor: the amount by which to multiply the offset (if step is not present).
 * min: the minimum allowed value (default: no minimum).
 * max: the maximum allowed value (default: no maximum).
 * wrap: true if the value should be wrapped when it passes min or max;
 *       both min and max must be present for this to work (default:
 *       false).
 */
static bool
do_property_adjust (const SGPropertyNode * arg)
{
  SGPropertyNode * prop = get_prop(arg);
  const SGPropertyNode * step = arg->getChild("step");
  const SGPropertyNode * offset = arg->getChild("offset");
  const SGPropertyNode * factor = arg->getChild("factor");
  const SGPropertyNode * min = arg->getChild("min");
  const SGPropertyNode * max = arg->getChild("max");
  bool wrap = arg->getBoolValue("wrap");

  if (min == 0 || max == 0)
      wrap = false;

  double amount = 0;
  if (step == 0)
    amount = offset->getDoubleValue() * factor->getDoubleValue();

  switch (prop->getType()) {

  case SGPropertyNode::BOOL: {
    bool value;
    if (step != 0)
      value = step->getBoolValue();
    else
      value = (0.0 != amount);
    if (value)
      return prop->setBoolValue(!prop->getBoolValue());
    else
      return true;
  }

  case SGPropertyNode::INT: {
    int value;
    if (step != 0)
      value = prop->getIntValue() + step->getIntValue();
    else
      value = prop->getIntValue() + int(amount);
    if (wrap) {
        do_wrap(&value, min->getIntValue(), max->getIntValue());
    } else {
        if (min != 0 && value < min->getIntValue())
            value = min->getIntValue();
        if (max != 0 && value > max->getIntValue())
            value = max->getIntValue();
    }
    return prop->setIntValue(value);
  }

  case SGPropertyNode::LONG: {
    long value;
    if (step != 0)
      value = prop->getLongValue() + step->getLongValue();
    else
      value = prop->getLongValue() + long(amount);
    if (wrap) {
        do_wrap(&value, min->getLongValue(), max->getLongValue());
    } else {
        if (min != 0 && value < min->getLongValue())
            value = min->getLongValue();
        if (max != 0 && value > max->getLongValue())
            value = max->getLongValue();
    }
    return prop->setLongValue(value);
  }

  case SGPropertyNode::FLOAT: {
    float value;
    if (step != 0)
      value = prop->getFloatValue() + step->getFloatValue();
    else
      value = prop->getFloatValue() + float(amount);
    if (wrap) {
        do_wrap(&value, min->getFloatValue(), max->getFloatValue());
    } else {
        if (min != 0 && value < min->getFloatValue())
            value = min->getFloatValue();
        if (max != 0 && value > max->getFloatValue())
            value = max->getFloatValue();
    }
    return prop->setFloatValue(value);
  }

  case SGPropertyNode::DOUBLE:
  case SGPropertyNode::UNSPECIFIED:
  case SGPropertyNode::NONE: {
    double value;
    if (step != 0)
      value = prop->getDoubleValue() + step->getDoubleValue();
    else
      value = prop->getDoubleValue() + amount;
    if (wrap) {
        do_wrap(&value, min->getDoubleValue(), max->getDoubleValue());
    } else {
        if (min != 0 && value < min->getDoubleValue())
            value = min->getDoubleValue();
        if (max != 0 && value > max->getDoubleValue())
            value = max->getDoubleValue();
    }
    return prop->setDoubleValue(value);
  }

  case SGPropertyNode::STRING: // doesn't make sense with strings
    SG_LOG(SG_INPUT, SG_ALERT, "Cannot adjust a string value");
    return false;

  default:
    SG_LOG(SG_INPUT, SG_ALERT, "Unknown value type");
    return false;

  }
}


/**
 * Built-in command: multiply a property value.
 *
 * property: the name of the property to multiply.
 * factor: the amount by which to multiply.
 */
static bool
do_property_multiply (const SGPropertyNode * arg)
{
  SGPropertyNode * prop = get_prop(arg);
  const SGPropertyNode * factor = arg->getChild("factor");

  if (factor == 0)
      return true;

  switch (prop->getType()) {

  case SGPropertyNode::BOOL:
    return prop->setBoolValue(prop->getBoolValue() && factor->getBoolValue());

  case SGPropertyNode::INT:
    return prop->setIntValue(int(prop->getIntValue()
                                 * factor->getDoubleValue()));

  case SGPropertyNode::LONG:
    return prop->setLongValue(long(prop->getLongValue()
				   * factor->getDoubleValue()));

  case SGPropertyNode::FLOAT:
    return prop->setFloatValue(float(prop->getFloatValue()
				     * factor->getDoubleValue()));

  case SGPropertyNode::DOUBLE:
  case SGPropertyNode::UNSPECIFIED:
  case SGPropertyNode::NONE:
    return prop->setDoubleValue(prop->getDoubleValue()
				* factor->getDoubleValue());

  default:			// doesn't make sense with strings
    return false;
  }
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
 * Set a property to an axis or other moving input.
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

  if (squared)
    setting = (setting < 0 ? -1 : 1) * setting * setting;

  return prop->setDoubleValue((setting + offset) * factor);
}

static bool
do_gui (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem_mgr()->get_group(FGSubsystemMgr::INIT)->get_subsystem("gui");
    gui->display(arg->getStringValue("name"));
    return true;
}


/**
 * Built-in command: commit presets (read from in /sim/presets/)
 */
static bool
do_presets_commit (const SGPropertyNode * arg)
{
    // unbind the current fdm state so property changes
    // don't get lost when we subsequently delete this fdm
    // and create a new one.
    cur_fdm_state->unbind();
        
    // set position from presets
    fgInitPosition();

    // BusyCursor(0);
    fgReInitSubsystems();

    global_tile_mgr.update( fgGetDouble("/environment/visibility-m") );

    if ( ! fgGetBool("/sim/presets/onground") ) {
        fgSetBool( "/sim/freeze/master", true );
        fgSetBool( "/sim/freeze/clock", true );
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
    { "exit", do_exit },
    { "load", do_load },
    { "save", do_save },
    { "panel-load", do_panel_load },
    { "panel-mouse-click", do_panel_mouse_click },
    { "preferences-load", do_preferences_load },
    { "view-cycle", do_view_cycle },
    { "screen-capture", do_screen_capture },
    { "tile-cache-reload", do_tile_cache_reload },
    { "lighting-update", do_lighting_update },
    { "property-toggle", do_property_toggle },
    { "property-assign", do_property_assign },
    { "property-adjust", do_property_adjust },
    { "property-multiply", do_property_multiply },
    { "property-swap", do_property_swap },
    { "property-scale", do_property_scale },
    { "gui", do_gui },
    { "presets_commit", do_presets_commit },
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
  SG_LOG(SG_GENERAL, SG_INFO, "Initializing basic built-in commands:");
  for (int i = 0; built_ins[i].name != 0; i++) {
    SG_LOG(SG_GENERAL, SG_INFO, "  " << built_ins[i].name);
    globals->get_commands()->addCommand(built_ins[i].name,
					built_ins[i].command);
  }

  typedef bool (*dummy)();
  fgTie( "/command/view/next", dummy(0), do_view_next );
  fgTie( "/command/view/prev", dummy(0), do_view_prev );
}

// end of fg_commands.cxx
