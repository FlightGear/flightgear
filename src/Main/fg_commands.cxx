// fg_commands.cxx - internal FGFS commands.

#include <simgear/compiler.h>

#include STL_STRING
#include STL_FSTREAM

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/commands.hxx>
#include <simgear/misc/props.hxx>

#include <GUI/gui.h>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include <Scenery/tilemgr.hxx>
#include <Time/tmp.hxx>

#include "fg_commands.hxx"

SG_USING_STD(string);
#if !defined(SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(ifstream);
SG_USING_STD(ofstream);
#endif

#include "fg_props.hxx"
#include "fg_io.hxx"
#include "globals.hxx"


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
  SG_LOG(SG_INPUT, SG_INFO, "Saving flight");
  ofstream output(file.c_str());
  if (output.good() && fgSaveFlight(output)) {
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
 * Built-in command: (re)load preferences.
 *
 * path (optional): the file name to load the panel from (relative
 * to FG_ROOT). Defaults to "preferences.xml".
 */
static bool
do_preferences_load (const SGPropertyNode * arg)
{
  const string &path = arg->getStringValue("path", "preferences.xml");
  SGPath props_path(globals->get_fg_root());
  props_path.append(path);
  SG_LOG(SG_INPUT, SG_INFO, "Reading global preferences from "
	 << props_path.str());
  if (!readProperties(props_path.str(), globals->get_props())) {
    SG_LOG(SG_INPUT, SG_ALERT, "Failed to reread global preferences");
    return false;
  } else {
    SG_LOG(SG_INPUT, SG_INFO, "Successfully read global preferences.");
    return true;
  }
}


/**
 * Built-in command: cycle view.
 */
static bool
do_view_cycle (const SGPropertyNode * arg)
{
  globals->get_current_view()->set_view_offset(0.0);
  globals->set_current_view(globals->get_viewmgr()->next_view());
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
  bool freeze = globals->get_freeze();
  SG_LOG(SG_INPUT, SG_INFO, "ReIniting TileCache");
  if ( !freeze ) 
    globals->set_freeze( true );
  BusyCursor(0);
  if ( global_tile_mgr.init() ) {
    // Load the local scenery data
    global_tile_mgr.update(fgGetDouble("/position/longitude"),
			   fgGetDouble("/position/latitude"));
  } else {
    SG_LOG( SG_GENERAL, SG_ALERT, 
	    "Error in Tile Manager initialization!" );
    exit(-1);
  }
  BusyCursor(1);
  if ( !freeze )
    globals->set_freeze( false );
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
  const string & propname = arg->getStringValue("property", "");
  if (propname == "")
    return false;

  SGPropertyNode * node = fgGetNode(propname);
  return node->setBoolValue(!node->getBoolValue());
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
  const string & propname = arg->getStringValue("property", "");
  if (propname == "")
    return false;

  SGPropertyNode * node = fgGetNode(propname, true);

  switch (node->getType()) {
  case SGPropertyNode::BOOL:
    return node->setBoolValue(arg->getBoolValue("value"));
  case SGPropertyNode::INT:
    return node->setIntValue(arg->getIntValue("value"));
  case SGPropertyNode::LONG:
    return node->setLongValue(arg->getLongValue("value"));
  case SGPropertyNode::FLOAT:
    return node->setFloatValue(arg->getFloatValue("value"));
  case SGPropertyNode::DOUBLE:
    return node->setDoubleValue(arg->getDoubleValue("value"));
  case SGPropertyNode::STRING:
    return node->setStringValue(arg->getStringValue("value"));
  default:
    return node->setUnknownValue(arg->getStringValue("value"));
  }
}


/**
 * Built-in command: increment or decrement a property value.
 *
 * property: the name of the property to increment or decrement.
 * step: the amount of the increment or decrement.
 */
static bool
do_property_adjust (const SGPropertyNode * arg)
{
  const string & propname = arg->getStringValue("property", "");
  if (propname == "")
    return false;

  SGPropertyNode * node = fgGetNode(propname, true);

  switch (node->getType()) {
  case SGPropertyNode::BOOL:
    if (arg->getBoolValue("step"))
      return node->setBoolValue(!node->getBoolValue());
    else
      return true;
  case SGPropertyNode::INT:
    return node->setIntValue(node->getIntValue()
			     + arg->getIntValue("step"));
  case SGPropertyNode::LONG:
    return node->setLongValue(node->getLongValue()
			      + arg->getLongValue("step"));
  case SGPropertyNode::FLOAT:
    return node->setFloatValue(node->getFloatValue()
			       + arg->getFloatValue("step"));
  case SGPropertyNode::DOUBLE:
  case SGPropertyNode::UNKNOWN:
    return node->setDoubleValue(node->getDoubleValue()
				+ arg->getDoubleValue("step"));
  default:			// doesn't make sense with strings
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
  const string & propname = arg->getStringValue("property", "");
  if (propname == "")
    return false;

  SGPropertyNode * node = fgGetNode(propname, true);

  switch (node->getType()) {
  case SGPropertyNode::BOOL:
    return node->setBoolValue(node->getBoolValue() &&
			      arg->getBoolValue("factor"));
  case SGPropertyNode::INT:
    return node->setIntValue(int(node->getIntValue()
				 * arg->getDoubleValue("factor")));
  case SGPropertyNode::LONG:
    return node->setLongValue(long(node->getLongValue()
				   * arg->getDoubleValue("factor")));
  case SGPropertyNode::FLOAT:
    return node->setFloatValue(float(node->getFloatValue()
				     * arg->getDoubleValue("factor")));
  case SGPropertyNode::DOUBLE:
  case SGPropertyNode::UNKNOWN:
    return node->setDoubleValue(node->getDoubleValue()
				* arg->getDoubleValue("factor"));
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
  const string &propname1 = arg->getStringValue("property[0]", "");
  const string &propname2 = arg->getStringValue("property[1]", "");
  if (propname1 == "" || propname2 == "")
    return false;

  SGPropertyNode * node1 = fgGetNode(propname1, true);
  SGPropertyNode * node2 = fgGetNode(propname2, true);
  const string & tmp = node1->getStringValue();
  return (node1->setUnknownValue(node2->getStringValue()) &&
	  node2->setUnknownValue(tmp));
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
  const string &propname = arg->getStringValue("property");
  double setting = arg->getDoubleValue("setting", 0.0);
  double offset = arg->getDoubleValue("offset", 0.0);
  double factor = arg->getDoubleValue("factor", 1.0);
  return fgSetDouble(propname, (setting + offset) * factor);
}



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
}

// end of fg_commands.hxx
