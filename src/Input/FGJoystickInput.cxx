// FGJoystickInput.cxx -- handle user input from joystick devices
//
// Written by Torsten Dreyer, started August 2009
// Based on work from David Megginson, started May 2001.
//
// Copyright (C) 2009 Torsten Dreyer, Torsten (at) t3r _dot_ de
// Copyright (C) 2001 David Megginson, david@megginson.com
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

#include "FGJoystickInput.hxx"

#include <simgear/props/props_io.hxx>
#include "FGDeviceConfigurationMap.hxx"
#include <Main/fg_props.hxx>
#include <Scripting/NasalSys.hxx>
#include <boost/foreach.hpp>

using simgear::PropertyList;

FGJoystickInput::axis::axis ()
  : last_value(9999999),
    tolerance(0.002),
    low_threshold(-0.9),
    high_threshold(0.9),
    interval_sec(0),
    last_dt(0)
{
}

FGJoystickInput::axis::~axis ()
{
}

FGJoystickInput::joystick::joystick ()
  : jsnum(0),
    js(0),
    naxes(0),
    nbuttons(0),
    axes(0),
    buttons(0),
    predefined(true)
{
}

FGJoystickInput::joystick::~joystick ()
{
  //  delete js? no, since js == this - and we're in the destructor already.
  delete[] axes;
  delete[] buttons;
  jsnum = 0;
  js = NULL;
  naxes = 0;
  nbuttons = 0;
  axes = NULL;
  buttons = NULL;
}


FGJoystickInput::FGJoystickInput()
{
}

FGJoystickInput::~FGJoystickInput()
{
    _remove(true);
}

void FGJoystickInput::_remove(bool all)
{
    SGPropertyNode * js_nodes = fgGetNode("/input/joysticks", true);

    for (int i = 0; i < MAX_JOYSTICKS; i++)
    {
        // do not remove predefined joysticks info on reinit
        if ((all)||(!bindings[i].predefined))
            js_nodes->removeChild("js", i, false);
        if (bindings[i].js)
            delete bindings[i].js;
        bindings[i].js = NULL;
    }
}

void FGJoystickInput::init()
{
  jsInit();
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing joystick bindings");
  SGPropertyNode_ptr js_nodes = fgGetNode("/input/joysticks", true);

  FGDeviceConfigurationMap configMap("Input/Joysticks", js_nodes, "js-named");

  for (int i = 0; i < MAX_JOYSTICKS; i++) {
    jsJoystick * js = new jsJoystick(i);
    bindings[i].js = js;

    if (js->notWorking()) {
      SG_LOG(SG_INPUT, SG_DEBUG, "Joystick " << i << " not found");
      continue;
    }

    const char * name = js->getName();
    SGPropertyNode_ptr js_node = js_nodes->getChild("js", i);

    if (js_node) {
      SG_LOG(SG_INPUT, SG_INFO, "Using existing bindings for joystick " << i);

    } else {
      bindings[i].predefined = false;
      SG_LOG(SG_INPUT, SG_INFO, "Looking for bindings for joystick \"" << name << '"');
      SGPropertyNode_ptr named;

      if ((named = configMap[name])) {
        string source = named->getStringValue("source", "user defined");
        SG_LOG(SG_INPUT, SG_INFO, "... found joystick: " << source);

      } else if ((named = configMap["default"])) {
        string source = named->getStringValue("source", "user defined");
        SG_LOG(SG_INPUT, SG_INFO, "No config found for joystick \"" << name
            << "\"\nUsing default: \"" << source << '"');

      } else {
        SG_LOG(SG_INPUT, SG_WARN, "No joystick configuration file with <name>" << name << "</name> entry found!");
      }

      js_node = js_nodes->getChild("js", i, true);
      copyProperties(named, js_node);
      js_node->setStringValue("id", name);
    }
  }
}

void FGJoystickInput::reinit() {
  SG_LOG(SG_INPUT, SG_DEBUG, "Re-Initializing joystick bindings");
  _remove(false);
  FGJoystickInput::init();
  FGJoystickInput::postinit();
}

void FGJoystickInput::postinit()
{
  FGNasalSys *nasalsys = (FGNasalSys *)globals->get_subsystem("nasal");
  SGPropertyNode_ptr js_nodes = fgGetNode("/input/joysticks");

  for (int i = 0; i < MAX_JOYSTICKS; i++) {
    SGPropertyNode_ptr js_node = js_nodes->getChild("js", i);
    jsJoystick *js = bindings[i].js;
    if (!js_node || js->notWorking())
      continue;

#ifdef WIN32
    JOYCAPS jsCaps ;
    joyGetDevCaps( i, &jsCaps, sizeof(jsCaps) );
    unsigned int nbuttons = jsCaps.wNumButtons;
    if (nbuttons > MAX_JOYSTICK_BUTTONS) nbuttons = MAX_JOYSTICK_BUTTONS;
#else
    unsigned int nbuttons = MAX_JOYSTICK_BUTTONS;
#endif

    int naxes = js->getNumAxes();
    if (naxes > MAX_JOYSTICK_AXES) naxes = MAX_JOYSTICK_AXES;
    bindings[i].naxes = naxes;
    bindings[i].nbuttons = nbuttons;

    SG_LOG(SG_INPUT, SG_DEBUG, "Initializing joystick " << i);

                                // Set up range arrays
    float minRange[MAX_JOYSTICK_AXES];
    float maxRange[MAX_JOYSTICK_AXES];
    float center[MAX_JOYSTICK_AXES];

                                // Initialize with default values
    js->getMinRange(minRange);
    js->getMaxRange(maxRange);
    js->getCenter(center);

                                // Allocate axes and buttons
    bindings[i].axes = new axis[naxes];
    bindings[i].buttons = new FGButton[nbuttons];

    //
    // Initialize nasal groups.
    //
    std::ostringstream str;
    str << "__js" << i;
    string module = str.str();
    nasalsys->createModule(module.c_str(), module.c_str(), "", 0);

    PropertyList nasal = js_node->getChildren("nasal");
    unsigned int j;
    for (j = 0; j < nasal.size(); j++) {
      nasal[j]->setStringValue("module", module.c_str());
      nasalsys->handleCommand(nasal[j]);
    }

    //
    // Initialize the axes.
    //
    PropertyList axes = js_node->getChildren("axis");
    size_t nb_axes = axes.size();
    for (j = 0; j < nb_axes; j++ ) {
      const SGPropertyNode * axis_node = axes[j];
      const SGPropertyNode * num_node = axis_node->getChild("number");
      int n_axis = axis_node->getIndex();
      if (num_node != 0) {
          n_axis = num_node->getIntValue(TGT_PLATFORM, -1);

        #ifdef SG_MAC
          // Mac falls back to Unix by default - avoids specifying many
          // duplicate <mac> entries in joystick config files
          if (n_axis < 0) {
              n_axis = num_node->getIntValue("unix", -1);
          }
        #endif
          
          // Silently ignore platforms that are not specified within the
          // <number></number> section
          if (n_axis < 0) {
              continue;
          }
      }

      if (n_axis >= naxes) {
          SG_LOG(SG_INPUT, SG_DEBUG, "Dropping bindings for axis " << n_axis);
          continue;
      }
      axis &a = bindings[i].axes[n_axis];

      js->setDeadBand(n_axis, axis_node->getDoubleValue("dead-band", 0.0));

      a.tolerance = axis_node->getDoubleValue("tolerance", 0.002);
      minRange[n_axis] = axis_node->getDoubleValue("min-range", minRange[n_axis]);
      maxRange[n_axis] = axis_node->getDoubleValue("max-range", maxRange[n_axis]);
      center[n_axis] = axis_node->getDoubleValue("center", center[n_axis]);

      read_bindings(axis_node, a.bindings, KEYMOD_NONE, module );

      // Initialize the virtual axis buttons.
      a.low.init(axis_node->getChild("low"), "low", module );
      a.low_threshold = axis_node->getDoubleValue("low-threshold", -0.9);

      a.high.init(axis_node->getChild("high"), "high", module );
      a.high_threshold = axis_node->getDoubleValue("high-threshold", 0.9);
      a.interval_sec = axis_node->getDoubleValue("interval-sec",0.0);
      a.last_dt = 0.0;
    }

    //
    // Initialize the buttons.
    //
    PropertyList buttons = js_node->getChildren("button");
    BOOST_FOREACH( SGPropertyNode * button_node, buttons ) {
      size_t n_but = button_node->getIndex();

      const SGPropertyNode * num_node = button_node->getChild("number");
      if (NULL != num_node)
          n_but = num_node->getIntValue(TGT_PLATFORM,n_but);

      if (n_but >= nbuttons) {
          SG_LOG(SG_INPUT, SG_DEBUG, "Dropping bindings for button " << n_but);
          continue;
      }

      std::ostringstream buf;
      buf << (unsigned)n_but;

      SG_LOG(SG_INPUT, SG_DEBUG, "Initializing button " << buf.str());
      FGButton &b = bindings[i].buttons[n_but];
      b.init(button_node, buf.str(), module );
      // get interval-sec property
      b.interval_sec = button_node->getDoubleValue("interval-sec",0.0);
      b.last_dt = 0.0;
    }

    js->setMinRange(minRange);
    js->setMaxRange(maxRange);
    js->setCenter(center);
  }
}

void FGJoystickInput::update( double dt )
{
  float axis_values[MAX_JOYSTICK_AXES];
  int modifiers = fgGetKeyModifiers();
  int buttons;

  for (int i = 0; i < MAX_JOYSTICKS; i++) {

    jsJoystick * js = bindings[i].js;
    if (js == 0 || js->notWorking())
      continue;

    js->read(&buttons, axis_values);
    if (js->notWorking()) // If js is disconnected
      continue;

                                // Fire bindings for the axes.
    for (int j = 0; j < bindings[i].naxes; j++) {
      axis &a = bindings[i].axes[j];

                                // Do nothing if the axis position
                                // is unchanged; only a change in
                                // position fires the bindings.
                                // But only if there are bindings
      if (fabs(axis_values[j] - a.last_value) > a.tolerance
         && a.bindings[KEYMOD_NONE].size() > 0 ) {
        a.last_value = axis_values[j];
        for (unsigned int k = 0; k < a.bindings[KEYMOD_NONE].size(); k++)
          a.bindings[KEYMOD_NONE][k]->fire(axis_values[j]);
      }

                                // do we have to emulate axis buttons?
      a.last_dt += dt;
      if(a.last_dt >= a.interval_sec) {
        if (a.low.bindings[modifiers].size())
          bindings[i].axes[j].low.update( modifiers, axis_values[j] < a.low_threshold );

        if (a.high.bindings[modifiers].size())
          bindings[i].axes[j].high.update( modifiers, axis_values[j] > a.high_threshold );

        a.last_dt -= a.interval_sec;
      }
    }

                                // Fire bindings for the buttons.
    for (int j = 0; j < bindings[i].nbuttons; j++) {
      FGButton &b = bindings[i].buttons[j];
      b.last_dt += dt;
      if(b.last_dt >= b.interval_sec) {
        bindings[i].buttons[j].update( modifiers, (buttons & (1u << j)) > 0 );
        b.last_dt -= b.interval_sec;
      }
    }
  }
}

