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

#include <simgear/compiler.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>                     
#endif

#include <math.h>

#include <string>

#include <Main/fg_props.hxx>

#include <simgear/debug/logstream.hxx>
#include <plib/js.h>

#include "joystick.hxx"

FG_USING_STD(string);
FG_USING_STD(cout);

#ifdef WIN32
static const int MAX_JOYSTICKS = 2;
#else
static const int MAX_JOYSTICKS = 10;
#endif
static const int MAX_AXES = _JS_MAX_AXES;
static const int MAX_BUTTONS = 32;


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
    "button5", "button6", "button7", "button8", "button9",
    "button10", "button11", "button12", "button13", "button14",
    "button15", "button16", "button17", "button18", "button19",
    "button20", "button21", "button22", "button23", "button24",
    "button25", "button26", "button27", "button28", "button29",
    "button30", "button31"
};


/**
 * trim capture
 */
struct trimcapture {
    float tolerance;
    bool captured;
    string name;
    SGValue* value;
};   


/**
 * Settings for a single axis.
 */
struct axis {
    axis () :
	value(0),
	offset(0.0),
	factor(1.0),
	last_value(9999999),
	tolerance(0.002),
	capture(NULL)
    { }
    SGValue * value;
    float offset;
    float factor;
    float last_value;
    float tolerance;
    trimcapture* capture;
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
#ifndef macintosh
	delete axes;
	delete buttons;
#else
	delete[] axes;
	delete[] buttons;
#endif
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

SGValue *trimmed;


/**
 * Initialize any joysticks found.
 */
int
fgJoystickInit() 
{
    bool seen_joystick = false;

    FG_LOG(FG_INPUT, FG_INFO, "Initializing joysticks");

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
	if (nbuttons > MAX_BUTTONS) nbuttons = MAX_BUTTONS;
#else
	int nbuttons = MAX_BUTTONS;
#endif
	
	int naxes = js->getNumAxes();
	if (naxes > MAX_AXES) naxes = MAX_AXES;
	joysticks[i].naxes = naxes;
	joysticks[i].nbuttons = nbuttons;

	FG_LOG(FG_INPUT, FG_INFO, "Initializing joystick " << i);
	seen_joystick = true;

	// Set up range arrays
	float *minRange = new float[naxes];
	float *maxRange = new float[naxes];
	float *center = new float[naxes];

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
	int j;
	for (j = 0; j < naxes; j++) {
	    axis &a = joysticks[i].axes[j];

	    string base = "/input/";
	    base += jsNames[i];
	    base += '/';
	    base += axisNames[j];
	    FG_LOG(FG_INPUT, FG_INFO, "  Axis " << j << ':');

	    // Control property
	    string name = base;
	    name += "/control";
	    SGValue * value = fgGetValue(name);
	    if (value == 0) {
		FG_LOG(FG_INPUT, FG_INFO, "    no control defined");
		continue;
	    }
	    const string &control = value->getStringValue();
	    a.value = fgGetValue(control, true);
	    FG_LOG(FG_INPUT, FG_INFO, "    using control " << control);

	    // Dead band
	    name = base;
	    name += "/dead-band";
	    value = fgGetValue(name);
	    if (value != 0)
		js->setDeadBand(j, value->getDoubleValue());
	    FG_LOG(FG_INPUT, FG_INFO, "    dead-band is " << js->getDeadBand(j));

	    // Offset
	    name = base;
	    name += "/offset";
	    value = fgGetValue(name);
	    if (value != 0)
		a.offset = value->getDoubleValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    offset is " << a.offset);


	    // Factor
	    name = base;
	    name += "/factor";
	    value = fgGetValue(name);
	    if (value != 0)
		a.factor = value->getDoubleValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    factor is " << a.factor);


	    // Tolerance
	    name = base;
	    name += "/tolerance";
	    value = fgGetValue(name);
	    if (value != 0)
		a.tolerance = value->getDoubleValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    tolerance is " << a.tolerance);


	    // Saturation
	    name = base;
	    name += "/saturation";
	    value = fgGetValue(name);
	    if (value != 0)
		js->setSaturation(j, value->getDoubleValue());
	    FG_LOG(FG_INPUT, FG_INFO, "    saturation is " << js->getSaturation(j));

	    // Minimum range
	    name = base;
	    name += "/min-range";
	    value = fgGetValue(name);
	    if (value != 0)
		minRange[j] = value->getDoubleValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    min-range is " << minRange[j]);

	    // Maximum range
	    name = base;
	    name += "/max-range";
	    value = fgGetValue(name);
	    if (value != 0)
		maxRange[j] = value->getDoubleValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    max-range is " << maxRange[j]);

	    // Center
	    name = base;
	    name += "/center";
	    value = fgGetValue(name);
	    if (value != 0)
		center[j] = value->getDoubleValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    center is " << center[j]);
      
            // Capture
	    name = base;
	    name += "/capture";
	    value = fgGetValue(name);
	    if ( value != 0 ) {
	        string trimname = "/fdm/trim"
		    + control.substr(control.rfind("/"),control.length());
		if ( fgHasValue(trimname) ) {	
		    a.capture = new trimcapture;
		    a.capture->tolerance = value->getDoubleValue();
		    a.capture->captured = false;
	 	    a.capture->name = control;
		    a.capture->value = fgGetValue(trimname);
	            FG_LOG(FG_INPUT, FG_INFO, "    capture is " 
			   << value->getDoubleValue() );
                } else {
		    a.capture = NULL;
		    FG_LOG(FG_INPUT, FG_INFO, "    capture is " 	
			   << "unsupported by FDM" );
		}				
	   }		 	        
	}


	//
	// Initialize the buttons.
	//
	for (j = 0; j < nbuttons; j++) {
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
	    SGValue * value = fgGetValue(name);
	    if (value == 0) {
		FG_LOG(FG_INPUT, FG_INFO, "    no control defined");
		continue;
	    }
	    const string &control = value->getStringValue();
	    b.value = fgGetValue(control, true);
	    FG_LOG(FG_INPUT, FG_INFO, "    using control " << control);

	    // Step
	    name = base;
	    name += "/step";
	    value = fgGetValue(name);
	    if (value != 0)
		b.step = value->getDoubleValue();
	    FG_LOG(FG_INPUT, FG_INFO, "    step is " << b.step);

	    // Type
	    name = base;
	    name += "/action";
	    value = fgGetValue(name);
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
	    value = fgGetValue(name);
	    if (value != 0)
		b.isRepeatable = value->getBoolValue();
	    FG_LOG(FG_INPUT, FG_INFO, (b.isRepeatable ?
				       "    repeatable" : "    not repeatable"));
	}

	js->setMinRange(minRange);
	js->setMaxRange(maxRange);
	js->setCenter(center);

	//-dw- clean up
	delete minRange;
	delete maxRange;
	delete center;
    }

    trimmed = fgGetValue("/fdm/trim/trimmed");
    
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
    float js_val, diff;
    float *axis_values = new float[MAX_AXES];

    for (int i = 0; i < MAX_JOYSTICKS; i++) {
	jsJoystick * js = joysticks[i].js;
	// float *axis_values = new float[joysticks[i].naxes];
	if (js->notWorking()) {
	    continue;
	}

	js->read(&buttons, axis_values);

	//
	// Axes
	//
	int j;
	for (j = 0; j < joysticks[i].naxes; j++) {
	    bool flag = true;
	    axis &a = joysticks[i].axes[j];

	    if ( a.capture && trimmed->getBoolValue() ) {
	    	// if the model has been trimmed then capture the
	        // joystick. When a trim succeeds, the above
	        // is true for one frame only.
		a.capture->captured = false;
	        FG_LOG( FG_GENERAL, FG_INFO, "Successful trim, capture is " <<
			"enabled on " << a.capture->name );
	    } 

	    // If the axis hasn't changed, don't
	    // force the value.
	    if (fabs(axis_values[j] - a.last_value) <= a.tolerance)
		continue;
	    else
		a.last_value = axis_values[j];
	    
	    if ( a.value ) {
	        js_val = ( axis_values[j] + a.offset ) * a.factor; 
	        
		if ( a.capture && !a.capture->captured ) {
		    diff = js_val - a.capture->value->getDoubleValue();
                    FG_LOG( FG_GENERAL, FG_INFO, a.capture->name
			    << " capture: " << diff );
		    if ( fabs( diff ) < a.capture->tolerance ) {
			flag = a.value->setDoubleValue( js_val );
			a.capture->captured = true;
			FG_LOG(FG_GENERAL,FG_INFO, a.capture->name 
			       << " captured." );
		    }
		} else {
		    flag = a.value->setDoubleValue( js_val );
                }
	    }
	    	
	    if (!flag)
		FG_LOG(FG_INPUT, FG_ALERT, "Failed to set value for joystick "
		       << i << ", axis " << j);
	}

	//
	// Buttons
	//
	for (j = 0; j < joysticks[i].nbuttons; j++) {
	    bool flag = false;
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
		    flag = b.value->setDoubleValue(b.value->getDoubleValue() +
						   b.step);
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
		case button::ADJUST:
		case button::TOGGLE:
		    // no op
		    flag = true;
		    break;
		case button::SWITCH:
		    flag = b.value->setDoubleValue(0.0);
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

    // -dw- cleanup 
    delete axis_values;

    return true;
}

// end of joystick.cxx
