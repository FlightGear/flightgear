// fg_commands.cxx - internal FGFS commands.

#include "fg_commands.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/commands.hxx>
#include <simgear/misc/props.hxx>

#include <GUI/gui.h>

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
  fgIOShutdownAll();
  exit(0);
  return true;
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
  case SGValue::BOOL:
    return node->setBoolValue(arg->getBoolValue("value"));
  case SGValue::INT:
    return node->setIntValue(arg->getIntValue("value"));
  case SGValue::LONG:
    return node->setLongValue(arg->getLongValue("value"));
  case SGValue::FLOAT:
    return node->setFloatValue(arg->getFloatValue("value"));
  case SGValue::DOUBLE:
    return node->setDoubleValue(arg->getDoubleValue("value"));
  case SGValue::STRING:
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
  case SGValue::BOOL:
    if (arg->getBoolValue("step"))
      return node->setBoolValue(!node->getBoolValue());
    else
      return true;
  case SGValue::INT:
    return node->setIntValue(node->getIntValue()
			     + arg->getIntValue("step"));
  case SGValue::LONG:
    return node->setLongValue(node->getLongValue()
			      + arg->getLongValue("step"));
  case SGValue::FLOAT:
    return node->setFloatValue(node->getFloatValue()
			       + arg->getFloatValue("step"));
  case SGValue::DOUBLE:
    return node->setDoubleValue(node->getDoubleValue()
				+ arg->getDoubleValue("step"));
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
  "null", do_null,
  "exit", do_exit,
  "view-cycle", do_view_cycle,
  "screen-capture", do_screen_capture,
  "property-toggle", do_property_toggle,
  "property-assign", do_property_assign,
  "property-adjust", do_property_adjust,
  "property-swap", do_property_swap,
  "property-scale", do_property_scale,
  0, 0				// zero-terminated
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
