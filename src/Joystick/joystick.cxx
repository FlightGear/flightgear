// joystick.cxx -- joystick support
//
// Original module written by Curtis Olson, started October 1998.
// Completely rewritten by David Megginson, July 2000.
//
// Copyright (C) 1998 - 2000  Curtis L. Olson - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <string>

#include <simgear/misc/props.hxx>
#include <simgear/debug/logstream.hxx>
#include <plib/js.h>
#include "joystick.hxx"

using std::string;

static const int MAX_JOYSTICKS = 10;
static const int MAX_AXES = 10;


/**
 * Property names for joysticks and axes.
 */
static const char * jsNames[] = {
  "js0", "js1", "js2", "js3", "js4",
  "js5", "js6", "js7", "js8", "js9"
};
static const char * axisNames[] = {
  "axis0", "axis1", "axis2", "axis3", "axis4",
  "axis5", "axis6", "axis7", "axis8", "axis9"
};


/**
 * Settings for a single axis.
 */
struct axis {
  axis () : value(0), offset(0.0), factor(1.0),
	    last_value(9999999), tolerance(0.002) {}
  FGValue * value;
  float offset;
  float factor;
  float last_value;
  float tolerance;
};


/**
 * Settings for a single joystick.
 */
struct joystick {
  virtual ~joystick () { delete js; delete axes; }
  jsJoystick * js;
  axis * axes;
};


/**
 * Array of joystick settings.
 */
static joystick joysticks[MAX_JOYSTICKS];


/**
 * Set up default values if properties are not specified.
 */
static void
setupDefaults ()
{
  FGPropertyList &props = current_properties;

				// Default axis 0 to aileron
  if (!props.getValue("/input/js0/axis0/control")) {
    props.setStringValue("/input/js0/axis0/control", "/controls/aileron");
    props.setFloatValue("/input/js0/axis0/dead-band", 0.1);
  }

				// Default axis 1 to elevator
  if (!props.getValue("/input/js0/axis1/control")) {
    props.setStringValue("/input/js0/axis1/control", "/controls/elevator");
    props.setFloatValue("/input/js0/axis1/dead-band", 0.1);
    props.setFloatValue("/input/js0/axis1/factor", -1.0);
  }

				// Default axis 2 to throttle
				// We need to fiddle with the offset
				// and factor to make it work
  if (!props.getValue("/input/js0/axis2/control")) {
    props.setStringValue("/input/js0/axis2/control", "/controls/throttle");
    props.setFloatValue("/input/js0/axis2/dead-band", 0.0);
    props.setFloatValue("/input/js0/axis2/offset", -1.0);
    props.setFloatValue("/input/js0/axis2/factor", -0.5);
  }

				// Default axis 3 to rudder
  if (!props.getValue("/input/js0/axis3/control")) {
    props.setStringValue("/input/js0/axis3/control", "/controls/rudder");
    props.setFloatValue("/input/js0/axis3/dead-band", 0.3);
  }
}


/**
 * Initialize any joysticks found.
 */
int
fgJoystickInit() 
{
  bool seen_joystick = false;

  FG_LOG(FG_INPUT, FG_INFO, "Initializing joysticks");

  setupDefaults();

  for (int i = 0; i < MAX_JOYSTICKS; i++) {
    jsJoystick * js = new jsJoystick(i);
    joysticks[i].js = js;
    if (js->notWorking()) {
      FG_LOG(FG_INPUT, FG_INFO, "Joystick " << i << " not found");
      continue;
    }

    FG_LOG(FG_INPUT, FG_INFO, "Initializing joystick " << i);
    seen_joystick = true;

				// Set up range arrays
    float minRange[js->getNumAxes()];
    float maxRange[js->getNumAxes()];
    float center[js->getNumAxes()];

				// Initialize with default values
    js->getMinRange(minRange);
    js->getMaxRange(maxRange);
    js->getCenter(center);

				// Allocate axes
    joysticks[i].axes = new axis[js->getNumAxes()];

    for (int j = 0; j < min(js->getNumAxes(), MAX_AXES); j++) {
      string base = "/input/";
      base += jsNames[i];
      base += '/';
      base += axisNames[j];
      FG_LOG(FG_INPUT, FG_INFO, "  Axis " << j << ':');

				// Control property
      string name = base;
      name += "/control";
      FGValue * value = current_properties.getValue(name);
      if (value == 0) {
	FG_LOG(FG_INPUT, FG_INFO, "    no control defined");
	continue;
      }
      const string &control = value->getStringValue();
      joysticks[i].axes[j].value = current_properties.getValue(control, true);
      FG_LOG(FG_INPUT, FG_INFO, "    using control " << control);

				// Dead band
      name = base;
      name += "/dead-band";
      value = current_properties.getValue(name);
      if (value != 0)
	js->setDeadBand(j, value->getFloatValue());
      FG_LOG(FG_INPUT, FG_INFO, "    dead-band is " << js->getDeadBand(j));

				// Offset
      name = base;
      name += "/offset";
      value = current_properties.getValue(name);
      if (value != 0)
	joysticks[i].axes[j].offset = value->getFloatValue();
      FG_LOG(FG_INPUT, FG_INFO, "    offset is "
	     << joysticks[i].axes[j].offset);


				// Factor
      name = base;
      name += "/factor";
      value = current_properties.getValue(name);
      if (value != 0)
	joysticks[i].axes[j].factor = value->getFloatValue();
      FG_LOG(FG_INPUT, FG_INFO, "    factor is "
	     << joysticks[i].axes[j].factor);


				// Tolerance
      name = base;
      name += "/tolerance";
      value = current_properties.getValue(name);
      if (value != 0)
	joysticks[i].axes[j].tolerance = value->getFloatValue();
      FG_LOG(FG_INPUT, FG_INFO, "    tolerance is "
	     << joysticks[i].axes[j].tolerance);


				// Saturation
      name = base;
      name += "/saturation";
      value = current_properties.getValue(name);
      if (value != 0)
	js->setSaturation(j, value->getFloatValue());
      FG_LOG(FG_INPUT, FG_INFO, "    saturation is " << js->getSaturation(j));

				// Minimum range
      name = base;
      name += "/min-range";
      value = current_properties.getValue(name);
      if (value != 0)
	minRange[j] = value->getFloatValue();
      FG_LOG(FG_INPUT, FG_INFO, "    min-range is " << minRange[j]);

				// Maximum range
      name = base;
      name += "/max-range";
      value = current_properties.getValue(name);
      if (value != 0)
	maxRange[j] = value->getFloatValue();
      FG_LOG(FG_INPUT, FG_INFO, "    max-range is " << maxRange[j]);

				// Center
      name = base;
      name += "/center";
      value = current_properties.getValue(name);
      if (value != 0)
	center[j] = value->getFloatValue();
      FG_LOG(FG_INPUT, FG_INFO, "    center is " << center[j]);
    }

    js->setMinRange(minRange);
    js->setMaxRange(maxRange);
    js->setCenter(center);
  }

  if (seen_joystick)
    FG_LOG(FG_INPUT, FG_INFO, "Done initializing joysticks");
  else
    FG_LOG(FG_INPUT, FG_ALERT, "No joysticks detected");

  return seen_joystick;
}


/**
 * Update property values based on the joystick state(s).
 */
int
fgJoystickRead()
{
  int buttons;

  for (int i = 0; i < MAX_JOYSTICKS; i++) {
    jsJoystick * js = joysticks[i].js;
    float axis_values[js->getNumAxes()];
    if (js->notWorking()) {
      continue;
    }

    js->read(&buttons, axis_values);

    for (int j = 0; j < min(MAX_AXES, js->getNumAxes()); j++) {

				// If the axis hasn't changed, don't
				// force the value.
      if (fabs(axis_values[j] - joysticks[i].axes[j].last_value) <=
	  joysticks[i].axes[j].tolerance)
	continue;
      else
	joysticks[i].axes[j].last_value = axis_values[j];

      FGValue * value = joysticks[i].axes[j].value;
      if (value) {
	if (!value->setDoubleValue((axis_values[j] +
				    joysticks[i].axes[j].offset) *
				   joysticks[i].axes[j].factor))
	  FG_LOG(FG_INPUT, FG_ALERT, "Failed to set value for joystick "
		 << i << ", axis " << j);
      }
    }
  }

  return true;			// FIXME
}

// end of joystick.cxx

