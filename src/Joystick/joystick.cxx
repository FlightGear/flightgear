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

#ifdef WIN32
static const int MAX_JOYSTICKS = 2;
#else
static const int MAX_JOYSTICKS = 10;
#endif
static const int MAX_AXES = _JS_MAX_AXES;
static const int MAX_BUTTONS = 10;


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
static const char * buttonNames[] = {
    "button0", "button1", "button2", "button3", "button4",
    "button5", "button6", "button7", "button8", "button9"
};


/**
 * Settings for a single axis.
 */
struct axis {
    axis () : value(0), offset(0.0), factor(1.0),
	last_value(9999999), tolerance(0.002) {}
    SGValue * value;
    float offset;
    float factor;
    float last_value;
    float tolerance;
};


/**
 * Settings for a single button.
 */
struct button {
    enum Action {
	TOGGLE,
	SWITCH,
	ADJUST
    };
    button () : value(0), step(0.0), action(ADJUST), isRepeatable(true),
	lastState(-1) {}
    SGValue * value;
    float step;
    Action action;
    bool isRepeatable;
    int lastState;
};


/**
 * Settings for a single joystick.
 */
struct joystick {
    virtual ~joystick () {
	delete js;
	delete axes;
	delete buttons;
    }
    int naxes;
    int nbuttons;
    jsJoystick * js;
    axis * axes;
    button * buttons;
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
    SGPropertyList &props = current_properties;

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

    // Default axis 2 to rudder
    if (!props.getValue("/input/js0/axis2/control")) {
	props.setStringValue("/input/js0/axis2/control", "/controls/rudder");
	props.setFloatValue("/input/js0/axis2/dead-band", 0.1);
    }

    // Default axis 3 to throttle
    // We need to fiddle with the offset
    // and factor to make it work
    if (!props.getValue("/input/js0/axis3/control")) {
	props.setStringValue("/input/js0/axis3/control", "/controls/throttle");
	props.setFloatValue("/input/js0/axis3/dead-band", 0.0);
	props.setFloatValue("/input/js0/axis3/offset", -1.0);
	props.setFloatValue("/input/js0/axis3/factor", -0.5);
    }

    // Default button 0 to all brakes
    if (!props.getValue("/input/js0/button0/control")) {
	props.setStringValue("/input/js0/button0/action", "switch");
	props.setStringValue("/input/js0/button0/control", "/controls/brake");
	props.setFloatValue("/input/js0/button0/step", 1.0);
	props.setFloatValue("/input/js0/button0/repeatable", false);
    }

    // Default button 1 to left brake.
    if (!props.getValue("/input/js0/button1/control")) {
	props.setStringValue("/input/js0/button1/action", "switch");
	props.setStringValue("/input/js0/button1/control", "/controls/left-brake");
	props.setFloatValue("/input/js0/button1/step", 1.0);
	props.setFloatValue("/input/js0/button1/repeatable", false);
    }

    // Default button 2 to right brake.
    if (!props.getValue("/input/js0/button2/control")) {
	props.setStringValue("/input/js0/button2/action", "switch");
	props.setStringValue("/input/js0/button2/control",
			     "/controls/right-brake");
	props.setFloatValue("/input/js0/button2/step", 1.0);
	props.setFloatValue("/input/js0/button2/repeatable", false);
    }

    // Default buttons 3 and 4 to elevator trim
    if (!props.getValue("/input/js0/button3/control")) {
	props.setStringValue("/input/js0/button3/action", "adjust");
	props.setStringValue("/input/js0/button3/control",
			     "/controls/elevator-trim");
	props.setFloatValue("/input/js0/button3/step", 0.001);
	props.setBoolValue("/input/js0/button3/repeatable", true);
    }
    if (!props.getValue("/input/js0/button4/control")) {
	props.setStringValue("/input/js0/button4/action", "adjust");
	props.setStringValue("/input/js0/button4/control",
			     "/controls/elevator-trim");
	props.setFloatValue("/input/js0/button4/step", -0.001);
	props.setBoolValue("/input/js0/button4/repeatable", true);
    }

    // Default buttons 5 and 6 to flaps
    if (!props.getValue("/input/js0/button5/control")) {
	props.setStringValue("/input/js0/button5/action", "adjust");
	props.setStringValue("/input/js0/button5/control", "/controls/flaps");
	props.setFloatValue("/input/js0/button5/step", -0.34);
	props.setBoolValue("/input/js0/button5/repeatable", false);
    }
    if (!props.getValue("/input/js0/button6/control")) {
	props.setStringValue("/input/js0/button6/action", "adjust");
	props.setStringValue("/input/js0/button6/control", "/controls/flaps");
	props.setFloatValue("/input/js0/button6/step", 0.34);
	props.setBoolValue("/input/js0/button6/repeatable", false);
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
#ifdef WIN32
	JOYCAPS jsCaps ;
	joyGetDevCaps( i, &jsCaps, sizeof(jsCaps) );
	int nbuttons = jsCaps.wNumButtons;
#else
	int nbuttons = MAX_BUTTONS;
#endif
	
	int naxes = js->getNumAxes();
	joysticks[i].naxes = naxes;
	joysticks[i].nbuttons = nbuttons;

	FG_LOG(FG_INPUT, FG_INFO, "Initializing joystick " << i);
	seen_joystick = true;

	// Set up range arrays
	float minRange[naxes];
	float maxRange[naxes];
	float center[naxes];

	// Initialize with default values
	js->getMinRange(minRange);
	js->getMaxRange(maxRange);
	js->getCenter(center);

	// Allocate axes and buttons
	joysticks[i].axes = new axis[naxes];
	joysticks[i].buttons = new button[nbuttons];


	//
	// Initialize the axes.
	//
	for (int j = 0; j < naxes; j++) {
	    axis &a = joysticks[i].axes[j];

	    string base = "/input/";
	    base += jsNames[i];
	    base += '/';
	    base += axisNames[j];
	    FG_LOG(FG_INPUT, FG_INFO, "  Axis " << j << ':');

	    // Control property
	    string name = base;
	    name += "/control";
	    SGValue * value = current_properties.getValue(name);
	    if (value == 0) {
		FG_LOG(FG_INPUT, FG_INFO, "    no control defined");
		continue;
	    }
	    const string &control = value->getStringValue();
	    a.value = current_properties.getValue(control, true);
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
		a.offset = value->getFloatValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    offset is " << a.offset);


	    // Factor
	    name = base;
	    name += "/factor";
	    value = current_properties.getValue(name);
	    if (value != 0)
		a.factor = value->getFloatValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    factor is " << a.factor);


	    // Tolerance
	    name = base;
	    name += "/tolerance";
	    value = current_properties.getValue(name);
	    if (value != 0)
		a.tolerance = value->getFloatValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    tolerance is " << a.tolerance);


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


	//
	// Initialize the buttons.
	//
	for (int j = 0; j < nbuttons; j++) {
	    button &b = joysticks[i].buttons[j];
      
	    string base = "/input/";
	    base += jsNames[i];
	    base += '/';
	    base += buttonNames[j];
	    FG_LOG(FG_INPUT, FG_INFO, "  Button " << j << ':');

	    // Control property
	    string name = base;
	    name += "/control";
	    cout << "Trying name " << name << endl;
	    SGValue * value = current_properties.getValue(name);
	    if (value == 0) {
		FG_LOG(FG_INPUT, FG_INFO, "    no control defined");
		continue;
	    }
	    const string &control = value->getStringValue();
	    b.value = current_properties.getValue(control, true);
	    FG_LOG(FG_INPUT, FG_INFO, "    using control " << control);

	    // Step
	    name = base;
	    name += "/step";
	    value = current_properties.getValue(name);
	    if (value != 0)
		b.step = value->getFloatValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    step is " << b.step);

	    // Type
	    name = base;
	    name += "/action";
	    value = current_properties.getValue(name);
	    string action = "adjust";
	    if (value != 0)
		action = value->getStringValue();
	    if (action == "toggle") {
		b.action = button::TOGGLE;
		b.isRepeatable = false;
	    } else if (action == "switch") {
		b.action = button::SWITCH;
		b.isRepeatable = false;
	    } else if (action == "adjust") {
		b.action = button::ADJUST;
		b.isRepeatable = true;
	    } else {
		FG_LOG(FG_INPUT, FG_ALERT, "    unknown action " << action);
		action = "adjust";
		b.action = button::ADJUST;
		b.isRepeatable = true;
	    }
	    FG_LOG(FG_INPUT, FG_INFO, "    action is " << action);

	    // Repeatability.
	    name = base;
	    name += "/repeatable";
	    value = current_properties.getValue(name);
	    if (value != 0)
		b.isRepeatable = value->getBoolValue();
	    FG_LOG(FG_INPUT, FG_INFO, (b.isRepeatable ?
				       "    repeatable" : "    not repeatable"));
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
	float axis_values[joysticks[i].naxes];
	if (js->notWorking()) {
	    continue;
	}

	js->read(&buttons, axis_values);

	//
	// Axes
	//
	for (int j = 0; j < joysticks[i].naxes; j++) {
	    bool flag = true;
	    axis &a = joysticks[i].axes[j];

	    // If the axis hasn't changed, don't
	    // force the value.
	    if (fabs(axis_values[j] - a.last_value) <= a.tolerance)
		continue;
	    else
		a.last_value = axis_values[j];

	    if (a.value)
		flag = a.value->setDoubleValue((axis_values[j] + a.offset) *
					       a.factor);
	    if (!flag)
		FG_LOG(FG_INPUT, FG_ALERT, "Failed to set value for joystick "
		       << i << ", axis " << j);
	}

	//
	// Buttons
	//
	for (int j = 0; j < joysticks[i].nbuttons; j++) {
	    bool flag;
	    button &b = joysticks[i].buttons[j];
	    if (b.value == 0)
		continue;

	    // Button is on.
	    if ((buttons & (1 << j)) > 0) {
		// Repeating?
		if (b.lastState == 1 && !b.isRepeatable)
		    continue;

		switch (b.action) {
		case button::TOGGLE:
		    if (b.step != 0.0) {
			if (b.value->getDoubleValue() == 0.0)
			    flag = b.value->setDoubleValue(b.step);
			else
			    flag = b.value->setDoubleValue(0.0);
		    } else {
			if (b.value->getBoolValue())
			    flag = b.value->setBoolValue(false);
			else
			    flag = b.value->setBoolValue(true);
		    }
		    break;
		case button::SWITCH:
		    flag = b.value->setDoubleValue(b.step);
		    break;
		case button::ADJUST:
		    if (!b.value->setDoubleValue(b.value->getDoubleValue() + b.step))
			FG_LOG(FG_INPUT, FG_ALERT, "Failed to set value for joystick "
			       << i << ", axis " << j);
		    break;
		default:
		    flag = false;
		    break;
		}
		b.lastState = 1;

		// Button is off
	    } else {
		// Repeating?
		if (b.lastState == 0 && !b.isRepeatable)
		    continue;

		switch (b.action) {
		case button::TOGGLE:
		    // no op
		    break;
		case button::SWITCH:
		    flag = b.value->setDoubleValue(0.0);
		    break;
		case button::ADJUST:
		    // no op
		    break;
		default:
		    flag = false;
		    break;
		}

		b.lastState = 0;
	    }
	    if (!flag)
		FG_LOG(FG_INPUT, FG_ALERT, "Failed to set value for "
		       << jsNames[i] << ' ' << buttonNames[j]);
	}
    }

    return true;
}

// end of joystick.cxx

