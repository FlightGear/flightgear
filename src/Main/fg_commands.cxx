// fg_commands.cxx - internal FGFS commands.

#include <string.h>		// strcmp()

#include <simgear/compiler.h>
#include <simgear/misc/exception.hxx>

#include STL_STRING
#include STL_FSTREAM

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/commands.hxx>
#include <simgear/misc/props.hxx>
#include <simgear/sg_inlines.h>

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
SG_USING_STD(ifstream);
SG_USING_STD(ofstream);

#include "fg_props.hxx"
#include "fg_io.hxx"
#include "globals.hxx"
#include "viewmgr.hxx"



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

static void
do_view_next( bool )
{
    globals->get_current_view()->setHeadingOffset_deg(0.0);
    globals->get_viewmgr()->next_view();
    fix_hud_visibility();
    globals->get_tile_mgr()->refresh_view_timestamps();
}

static void
do_view_prev( bool )
{
    globals->get_current_view()->setHeadingOffset_deg(0.0);
    globals->get_viewmgr()->prev_view();
    fix_hud_visibility();
    globals->get_tile_mgr()->refresh_view_timestamps();
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
  globals->get_tile_mgr()->refresh_view_timestamps();
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
    if ( globals->get_tile_mgr()->init() ) {
	// Load the local scenery data
        double visibility_meters = fgGetDouble("/environment/visibility-m");
	globals->get_tile_mgr()->update( visibility_meters );
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
 * value: the value to assign; or
 * property[1]: the property to copy from.
 */
static bool
do_property_assign (const SGPropertyNode * arg)
{
  SGPropertyNode * prop = get_prop(arg);
  const SGPropertyNode * prop2 = get_prop2(arg);
  const SGPropertyNode * value = arg->getNode("value");

  if (value != 0)
      return prop->setUnspecifiedValue(value->getStringValue());
  else if (prop2)
      return prop->setUnspecifiedValue(prop2->getStringValue());
  else
      return false;
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

  if (squared)
    setting = (setting < 0 ? -1 : 1) * setting * setting;

  return prop->setDoubleValue((setting + offset) * factor);
}


/**
 * Built-in command: Show an XML-configured dialog.
 *
 * dialog-name: the name of the GUI dialog to display.
 */
static bool
do_dialog_show (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem_mgr()
        ->get_group(FGSubsystemMgr::INIT)->get_subsystem("gui");
    gui->display(arg->getStringValue("dialog-name"));
    return true;
}


/**
 * Built-in Command: Hide the active XML-configured dialog.
 */
static bool
do_dialog_close (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem_mgr()
        ->get_group(FGSubsystemMgr::INIT)->get_subsystem("gui");
    GUIWidget * widget = gui->getCurrentWidget();
    if (widget != 0) {
        delete widget;
        gui->setCurrentWidget(0);
        return true;
    } else {
        return false;
    }
}


/**
 * Update a value in the active XML-configured dialog.
 *
 * object-name: The name of the GUI object(s) (all GUI objects if omitted).
 */
static bool
do_dialog_update (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem_mgr()
        ->get_group(FGSubsystemMgr::INIT)->get_subsystem("gui");
    GUIWidget * widget = gui->getCurrentWidget();
    if (widget != 0) {
        if (arg->hasValue("object-name")) {
            gui->getCurrentWidget()
                ->updateValue(arg->getStringValue("object-name"));
        } else {
            gui->getCurrentWidget()->updateValues();
        }
        return true;
    } else {
        return false;
    }
}


/**
 * Apply a value in the active XML-configured dialog.
 *
 * object-name: The name of the GUI object(s) (all GUI objects if omitted).
 */
static bool
do_dialog_apply (const SGPropertyNode * arg)
{
    NewGUI * gui = (NewGUI *)globals->get_subsystem_mgr()
        ->get_group(FGSubsystemMgr::INIT)->get_subsystem("gui");
    GUIWidget * widget = gui->getCurrentWidget();
    if (widget != 0) {
        if (arg->hasValue("object-name")) {
            gui->getCurrentWidget()
                ->applyValue(arg->getStringValue("object-name"));
        } else {
            gui->getCurrentWidget()->applyValues();
        }
        return true;
    } else {
        return false;
    }
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

    globals->get_tile_mgr()->update( fgGetDouble("/environment/visibility-m") );

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
    { "dialog-show", do_dialog_show },
    { "dialog-close", do_dialog_close },
    { "dialog-show", do_dialog_show },
    { "dialog-update", do_dialog_update },
    { "dialog-apply", do_dialog_apply },
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
