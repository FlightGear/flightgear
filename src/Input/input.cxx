// input.cxx -- handle user input from various sources.
//
// Written by David Megginson, started May 2001.
//
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
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/compiler.h>

#include <math.h>
#include <ctype.h>
#include <sstream>

#include STL_FSTREAM
#include STL_STRING
#include <vector>

#include <simgear/compiler.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>

#include <Aircraft/aircraft.hxx>
#include <Autopilot/xmlauto.hxx>
#include <Cockpit/hud.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include <GUI/gui.h>
#include <Model/panelnode.hxx>
#include <Scripting/NasalSys.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include "input.hxx"

#include <Scenery/scenery.hxx>
#include <Main/renderer.hxx>
#include <plib/ssg.h>
#include <simgear/math/sg_geodesy.hxx>

SG_USING_STD(ifstream);
SG_USING_STD(ostringstream);
SG_USING_STD(string);
SG_USING_STD(vector);

void mouseClickHandler(int button, int updown, int x, int y);
void mouseMotionHandler(int x, int y);
void keyHandler(int key, int keymod, int mousex, int mousey);


////////////////////////////////////////////////////////////////////////
// Local variables.
////////////////////////////////////////////////////////////////////////

static FGInput * default_input = 0;


////////////////////////////////////////////////////////////////////////
// Local functions.
////////////////////////////////////////////////////////////////////////

static bool
getModShift ()
{
  return bool(fgGetKeyModifiers() & KEYMOD_SHIFT);
}

static bool
getModCtrl ()
{
  return bool(fgGetKeyModifiers() & KEYMOD_CTRL);
}

static bool
getModAlt ()
{
  return bool(fgGetKeyModifiers() & KEYMOD_ALT);
}


////////////////////////////////////////////////////////////////////////
// Implementation of FGBinding.
////////////////////////////////////////////////////////////////////////

FGBinding::FGBinding ()
  : _command(0),
    _arg(new SGPropertyNode),
    _setting(0)
{
}

FGBinding::FGBinding (const SGPropertyNode * node)
  : _command(0),
    _arg(0),
    _setting(0)
{
  read(node);
}

FGBinding::~FGBinding ()
{
  if (_arg && _arg->getParent())
    _arg->getParent()->removeChild(_arg->getName(), _arg->getIndex(), false);
}

void
FGBinding::read (const SGPropertyNode * node)
{
  const SGPropertyNode * conditionNode = node->getChild("condition");
  if (conditionNode != 0)
    setCondition(sgReadCondition(globals->get_props(), conditionNode));

  _command_name = node->getStringValue("command", "");
  if (_command_name.empty()) {
    SG_LOG(SG_INPUT, SG_WARN, "No command supplied for binding.");
    _command = 0;
    return;
  }

  _arg = (SGPropertyNode *)node;
  _setting = 0;
}

void
FGBinding::fire () const
{
  if (test()) {
    if (_command == 0)
      _command = globals->get_commands()->getCommand(_command_name);
    if (_command == 0) {
      SG_LOG(SG_INPUT, SG_WARN, "No command attached to binding");
    } else if (!(*_command)(_arg)) {
      SG_LOG(SG_INPUT, SG_ALERT, "Failed to execute command "
             << _command_name);
    }
  }
}

void
FGBinding::fire (double offset, double max) const
{
  if (test()) {
    _arg->setDoubleValue("offset", offset/max);
    fire();
  }
}

void
FGBinding::fire (double setting) const
{
  if (test()) {
                                // A value is automatically added to
                                // the args
    if (_setting == 0)          // save the setting node for efficiency
      _setting = _arg->getChild("setting", 0, true);
    _setting->setDoubleValue(setting);
    fire();
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput.
////////////////////////////////////////////////////////////////////////


FGInput::FGInput ()
{
    if (default_input == 0)
        default_input = this;
}

FGInput::~FGInput ()
{
    if (default_input == this)
        default_input = 0;
}

void
FGInput::init ()
{
  _init_keyboard();
  _init_joystick();
  _init_mouse();

  fgRegisterKeyHandler(keyHandler);
  fgRegisterMouseClickHandler(mouseClickHandler);
  fgRegisterMouseMotionHandler(mouseMotionHandler);
}

void
FGInput::reinit ()
{
    init();
}

void
FGInput::postinit ()
{
  _postinit_joystick();
  _postinit_keyboard();
}

void
FGInput::bind ()
{
  fgTie("/devices/status/keyboard/shift", getModShift);
  fgTie("/devices/status/keyboard/ctrl", getModCtrl);
  fgTie("/devices/status/keyboard/alt", getModAlt);
}

void
FGInput::unbind ()
{
  fgUntie("/devices/status/keyboard/shift");
  fgUntie("/devices/status/keyboard/ctrl");
  fgUntie("/devices/status/keyboard/alt");
}

void 
FGInput::update (double dt)
{
  _update_keyboard();
  _update_joystick(dt);
  _update_mouse(dt);
}

void
FGInput::suspend ()
{
    // NO-OP
}

void
FGInput::resume ()
{
    // NO-OP
}

bool
FGInput::is_suspended () const
{
    return false;
}

void
FGInput::makeDefault (bool status)
{
    if (status)
        default_input = this;
    else if (default_input == this)
        default_input = 0;
}

void
FGInput::doKey (int k, int modifiers, int x, int y)
{
                                // Sanity check.
  if (k < 0 || k >= MAX_KEYS) {
    SG_LOG(SG_INPUT, SG_WARN, "Key value " << k << " out of range");
    return;
  }

  button &b = _key_bindings[k];

                                // Key pressed.
  if (!(modifiers & KEYMOD_RELEASED)) {
    SG_LOG( SG_INPUT, SG_DEBUG, "User pressed key " << k
            << " with modifiers " << modifiers );
    if (!b.last_state || b.is_repeatable) {
      const binding_list_t &bindings = _find_key_bindings(k, modifiers);

      for (unsigned int i = 0; i < bindings.size(); i++)
        bindings[i]->fire();
      b.last_state = 1;
    }
  }
                                // Key released.
  else {
    SG_LOG(SG_INPUT, SG_DEBUG, "User released key " << k
           << " with modifiers " << modifiers);
    if (b.last_state) {
      const binding_list_t &bindings = _find_key_bindings(k, modifiers);
      for (unsigned int i = 0; i < bindings.size(); i++)
        bindings[i]->fire();
      b.last_state = 0;
    } else {
      if (k >= 1 && k <= 26) {
        if (_key_bindings[k + '@'].last_state)
          doKey(k + '@', KEYMOD_RELEASED, x, y);
        if (_key_bindings[k + '`'].last_state)
          doKey(k + '`', KEYMOD_RELEASED, x, y);
      } else if (k >= 'A' && k <= 'Z') {
        if (_key_bindings[k - '@'].last_state)
          doKey(k - '@', KEYMOD_RELEASED, x, y);
        if (_key_bindings[tolower(k)].last_state)
          doKey(tolower(k), KEYMOD_RELEASED, x, y);
      } else if (k >= 'a' && k <= 'z') {
        if (_key_bindings[k - '`'].last_state)
          doKey(k - '`', KEYMOD_RELEASED, x, y);
        if (_key_bindings[toupper(k)].last_state)
          doKey(toupper(k), KEYMOD_RELEASED, x, y);
      }
    }
  }
}

void
FGInput::doMouseClick (int b, int updown, int x, int y)
{
  int modifiers = fgGetKeyModifiers();

  mouse &m = _mouse_bindings[0];
  mouse_mode &mode = m.modes[m.current_mode];

                                // Let the property manager know.
  if (b >= 0 && b < MAX_MOUSE_BUTTONS)
    m.mouse_button_nodes[b]->setBoolValue(updown == MOUSE_BUTTON_DOWN);

                                // Pass on to PUI and the panel if
                                // requested, and return if one of
                                // them consumes the event.
  if (mode.pass_through) {
    if (puMouse(b, updown, x, y))
      return;
    else if ((globals->get_current_panel() != 0) &&
             globals->get_current_panel()->getVisibility() &&
             globals->get_current_panel()->doMouseAction(b, updown, x, y))
      return;
    else if (fgHandle3DPanelMouseEvent(b, updown, x, y))
      return;
    else {
      // pui and the panel didn't want the click event so compute a
      // terrain intersection point corresponding to the mouse click
      // and be happy.
      FGScenery* scenery = globals->get_scenery();
      SGVec3d start, dir, hit;
      if (!b && updown == MOUSE_BUTTON_DOWN
          && FGRenderer::getPickInfo(start, dir, x, y)
          && scenery->get_cart_ground_intersection(start, dir, hit)) {

        SGGeod geod = SGGeod::fromCart(hit);
        SGPropertyNode *c = fgGetNode("/sim/input/click", true);
        c->setDoubleValue("longitude-deg", geod.getLongitudeDeg());
        c->setDoubleValue("latitude-deg", geod.getLatitudeDeg());
        c->setDoubleValue("elevation-m", geod.getElevationM());
        c->setDoubleValue("elevation-ft", geod.getElevationFt());

        fgSetBool("/sim/signals/click", 1);
      }
    }
  }

                                // OK, PUI and the panel didn't want the click
  if (b >= MAX_MOUSE_BUTTONS) {
    SG_LOG(SG_INPUT, SG_ALERT, "Mouse button " << b
           << " where only " << MAX_MOUSE_BUTTONS << " expected");
    return;
  }

  _update_button(m.modes[m.current_mode].buttons[b], modifiers, 0 != updown, x, y);
}

void
FGInput::doMouseMotion (int x, int y)
{
  // Don't call fgGetKeyModifiers() here, until we are using a
  // toolkit that supports getting the mods from outside a key
  // callback.  Glut doesn't.
  int modifiers = KEYMOD_NONE;

  int xsize = fgGetInt("/sim/startup/xsize", 800);
  int ysize = fgGetInt("/sim/startup/ysize", 600);

  mouse &m = _mouse_bindings[0];

  if (m.current_mode < 0 || m.current_mode >= m.nModes) {
      m.x = x;
      m.y = y;
      return;
  }
  mouse_mode &mode = m.modes[m.current_mode];

                                // Pass on to PUI if requested, and return
                                // if PUI consumed the event.
  if (mode.pass_through && puMouse(x, y)) {
      m.x = x;
      m.y = y;
      return;
  }

                                // OK, PUI didn't want the event,
                                // so we can play with it.
  if (x != m.x) {
    int delta = x - m.x;
    for (unsigned int i = 0; i < mode.x_bindings[modifiers].size(); i++)
      mode.x_bindings[modifiers][i]->fire(double(delta), double(xsize));
  }
  if (y != m.y) {
    int delta = y - m.y;
    for (unsigned int i = 0; i < mode.y_bindings[modifiers].size(); i++)
      mode.y_bindings[modifiers][i]->fire(double(delta), double(ysize));
  }

                                // Constrain the mouse if requested
  if (mode.constrained) {
    bool need_warp = false;
    if (x <= (xsize * .25) || x >= (xsize * .75)) {
      x = int(xsize * .5);
      need_warp = true;
    }

    if (y <= (ysize * .25) || y >= (ysize * .75)) {
      y = int(ysize * .5);
      need_warp = true;
    }

    if (need_warp)
      fgWarpMouse(x, y);
  }

  if (m.x != x)
      fgSetInt("/devices/status/mice/mouse/x", m.x = x);

  if (m.y != y)
      fgSetInt("/devices/status/mice/mouse/y", m.y = y);
}

void
FGInput::_init_keyboard ()
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing key bindings");
  _module = "__kbd";
  SGPropertyNode * key_nodes = fgGetNode("/input/keyboard");
  if (key_nodes == 0) {
    SG_LOG(SG_INPUT, SG_WARN, "No key bindings (/input/keyboard)!!");
    key_nodes = fgGetNode("/input/keyboard", true);
  }

  vector<SGPropertyNode_ptr> keys = key_nodes->getChildren("key");
  for (unsigned int i = 0; i < keys.size(); i++) {
    int index = keys[i]->getIndex();
    SG_LOG(SG_INPUT, SG_DEBUG, "Binding key " << index);

    _key_bindings[index].bindings->clear();
    _key_bindings[index].is_repeatable = keys[i]->getBoolValue("repeatable");
    _key_bindings[index].last_state = 0;
    _read_bindings(keys[i], _key_bindings[index].bindings, KEYMOD_NONE);
  }
}


void
FGInput::_scan_joystick_dir(SGPath *path, SGPropertyNode* node, int *index)
{
  ulDir *dir = ulOpenDir(path->c_str());
  if (dir) {
    ulDirEnt* dent;
    while ((dent = ulReadDir(dir)) != 0) {
      if (dent->d_name[0] == '.')
        continue;

      SGPath p(path->str());
      p.append(dent->d_name);
      _scan_joystick_dir(&p, node, index);
    }
    ulCloseDir(dir);

  } else if (path->extension() == "xml") {
    SG_LOG(SG_INPUT, SG_DEBUG, "Reading joystick file " << path->str());
    SGPropertyNode *n = node->getChild("js-named", (*index)++, true);
    readProperties(path->str(), n);
    n->setStringValue("source", path->c_str());
  }
}


void
FGInput::_init_joystick ()
{
  jsInit();
                                // TODO: zero the old bindings first.
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing joystick bindings");
  SGPropertyNode * js_nodes = fgGetNode("/input/joysticks", true);

  // read all joystick xml files into /input/joysticks/js_named[1000++]
  SGPath path(globals->get_fg_root());
  path.append("Input/Joysticks");
  int js_named_index = 1000;
  _scan_joystick_dir(&path, js_nodes, &js_named_index);

  // build name->node map with each <name> (reverse order)
  map<string, SGPropertyNode_ptr> jsmap;
  vector<SGPropertyNode_ptr> js_named = js_nodes->getChildren("js-named");

  for (int k = (int)js_named.size() - 1; k >= 0; k--) {
    SGPropertyNode *n = js_named[k];
    vector<SGPropertyNode_ptr> names = n->getChildren("name");
    if (names.size() && (n->getChildren("axis").size() || n->getChildren("button").size()))
      for (unsigned int j = 0; j < names.size(); j++)
        jsmap[names[j]->getStringValue()] = n;
  }

  // set up js[] nodes
  for (int i = 0; i < MAX_JOYSTICKS; i++) {
    jsJoystick * js = new jsJoystick(i);
    _joystick_bindings[i].js = js;

    if (js->notWorking()) {
      SG_LOG(SG_INPUT, SG_DEBUG, "Joystick " << i << " not found");
      continue;
    }

    const char * name = js->getName();
    SGPropertyNode_ptr js_node = js_nodes->getChild("js", i);

    if (js_node) {
      SG_LOG(SG_INPUT, SG_INFO, "Using existing bindings for joystick " << i);

    } else {
      SG_LOG(SG_INPUT, SG_INFO, "Looking for bindings for joystick \"" << name << '"');
      SGPropertyNode_ptr named;

      if ((named = jsmap[name])) {
        string source = named->getStringValue("source", "user defined");
        SG_LOG(SG_INPUT, SG_INFO, "... found joystick: " << source);

      } else if ((named = jsmap["default"])) {
        string source = named->getStringValue("source", "user defined");
        SG_LOG(SG_INPUT, SG_INFO, "No config found for joystick \"" << name
            << "\"\nUsing default: \"" << source << '"');

      } else {
        throw sg_throwable(string("No joystick configuration file with "
            "<name>default</name> entry found!"));
      }

      js_node = js_nodes->getChild("js", i, true);
      copyProperties(named, js_node);
      js_node->setStringValue("id", name);
    }
  }

  // get rid of unused config nodes
  js_nodes->removeChildren("js-named", false);
}


void
FGInput::_postinit_keyboard()
{
  FGNasalSys *nasalsys = (FGNasalSys *)globals->get_subsystem("nasal");
  SGPropertyNode *key_nodes = fgGetNode("/input/keyboard", true);
  vector<SGPropertyNode_ptr> nasal = key_nodes->getChildren("nasal");
  for (unsigned int j = 0; j < nasal.size(); j++) {
    nasal[j]->setStringValue("module", _module.c_str());
    nasalsys->handleCommand(nasal[j]);
  }
}


void
FGInput::_postinit_joystick()
{
  FGNasalSys *nasalsys = (FGNasalSys *)globals->get_subsystem("nasal");
  SGPropertyNode *js_nodes = fgGetNode("/input/joysticks");

  for (int i = 0; i < MAX_JOYSTICKS; i++) {
    SGPropertyNode_ptr js_node = js_nodes->getChild("js", i);
    jsJoystick *js = _joystick_bindings[i].js;
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
    _joystick_bindings[i].naxes = naxes;
    _joystick_bindings[i].nbuttons = nbuttons;

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
    _joystick_bindings[i].axes = new axis[naxes];
    _joystick_bindings[i].buttons = new button[nbuttons];

    //
    // Initialize nasal groups.
    //
    ostringstream str;
    str << "__js" << i;
    _module = str.str();
    nasalsys->createModule(_module.c_str(), _module.c_str(), "", 0);

    vector<SGPropertyNode_ptr> nasal = js_node->getChildren("nasal");
    unsigned int j;
    for (j = 0; j < nasal.size(); j++) {
      nasal[j]->setStringValue("module", _module.c_str());
      nasalsys->handleCommand(nasal[j]);
    }

    //
    // Initialize the axes.
    //
    vector<SGPropertyNode_ptr> axes = js_node->getChildren("axis");
    size_t nb_axes = axes.size();
    for (j = 0; j < nb_axes; j++ ) {
      const SGPropertyNode * axis_node = axes[j];
      const SGPropertyNode * num_node = axis_node->getChild("number");
      int n_axis = axis_node->getIndex();
      if (num_node != 0) {
          n_axis = num_node->getIntValue(TGT_PLATFORM, -1);

          // Silently ignore platforms that are not specified within the
          // <number></number> section
          if (n_axis < 0)
             continue;
      }

      if (n_axis >= naxes) {
          SG_LOG(SG_INPUT, SG_DEBUG, "Dropping bindings for axis " << n_axis);
          continue;
      }
      axis &a = _joystick_bindings[i].axes[n_axis];

      js->setDeadBand(n_axis, axis_node->getDoubleValue("dead-band", 0.0));

      a.tolerance = axis_node->getDoubleValue("tolerance", 0.002);
      minRange[n_axis] = axis_node->getDoubleValue("min-range", minRange[n_axis]);
      maxRange[n_axis] = axis_node->getDoubleValue("max-range", maxRange[n_axis]);
      center[n_axis] = axis_node->getDoubleValue("center", center[n_axis]);

      _read_bindings(axis_node, a.bindings, KEYMOD_NONE);

      // Initialize the virtual axis buttons.
      _init_button(axis_node->getChild("low"), a.low, "low");
      a.low_threshold = axis_node->getDoubleValue("low-threshold", -0.9);

      _init_button(axis_node->getChild("high"), a.high, "high");
      a.high_threshold = axis_node->getDoubleValue("high-threshold", 0.9);
      a.interval_sec = axis_node->getDoubleValue("interval-sec",0.0);
      a.last_dt = 0.0;
    }

    //
    // Initialize the buttons.
    //
    vector<SGPropertyNode_ptr> buttons = js_node->getChildren("button");
    char buf[32];
    for (j = 0; j < buttons.size() && j < nbuttons; j++) {
      const SGPropertyNode * button_node = buttons[j];
      const SGPropertyNode * num_node = button_node->getChild("number");
      size_t n_but = button_node->getIndex();
      if (num_node != 0) {
          n_but = num_node->getIntValue(TGT_PLATFORM,n_but);
      }

      if (n_but >= nbuttons) {
          SG_LOG(SG_INPUT, SG_DEBUG, "Dropping bindings for button " << n_but);
          continue;
      }

      sprintf(buf, "%d", n_but);
      SG_LOG(SG_INPUT, SG_DEBUG, "Initializing button " << n_but);
      _init_button(button_node,
                   _joystick_bindings[i].buttons[n_but],
                   buf);

      // get interval-sec property
      button &b = _joystick_bindings[i].buttons[n_but];
      if (button_node != 0) {
        b.interval_sec = button_node->getDoubleValue("interval-sec",0.0);
        b.last_dt = 0.0;
      }
    }

    js->setMinRange(minRange);
    js->setMaxRange(maxRange);
    js->setCenter(center);
  }
}


// 
// Map of all known cursor names
// This used to contain all the Glut cursors, but those are
// not defined by other toolkits.  It now supports only the cursor
// images we actually use, in the interest of portability.  Someday,
// it would be cool to write an OpenGL cursor renderer, with the
// cursors defined as textures referenced in the property tree.  This
// list could then be eliminated. -Andy
//
static struct {
  const char * name;
  int cursor;
} mouse_cursor_map[] = {
  { "none", MOUSE_CURSOR_NONE },
  { "inherit", MOUSE_CURSOR_POINTER },
  { "wait", MOUSE_CURSOR_WAIT },
  { "crosshair", MOUSE_CURSOR_CROSSHAIR },
  { "left-right", MOUSE_CURSOR_LEFTRIGHT },
  { 0, 0 }
};

void
FGInput::_init_mouse ()
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing mouse bindings");
  _module = "";

  SGPropertyNode * mouse_nodes = fgGetNode("/input/mice");
  if (mouse_nodes == 0) {
    SG_LOG(SG_INPUT, SG_WARN, "No mouse bindings (/input/mice)!!");
    mouse_nodes = fgGetNode("/input/mice", true);
  }

  int j;
  for (int i = 0; i < MAX_MICE; i++) {
    SGPropertyNode * mouse_node = mouse_nodes->getChild("mouse", i, true);
    mouse &m = _mouse_bindings[i];

                                // Grab node pointers
    char buf[64];
    sprintf(buf, "/devices/status/mice/mouse[%d]/mode", i);
    m.mode_node = fgGetNode(buf);
    if (m.mode_node == NULL) {
      m.mode_node = fgGetNode(buf, true);
      m.mode_node->setIntValue(0);
    }
    for (j = 0; j < MAX_MOUSE_BUTTONS; j++) {
      sprintf(buf, "/devices/status/mice/mouse[%d]/button[%d]", i, j);
      m.mouse_button_nodes[j] = fgGetNode(buf, true);
      m.mouse_button_nodes[j]->setBoolValue(false);
    }

                                // Read all the modes
    m.nModes = mouse_node->getIntValue("mode-count", 1);
    m.modes = new mouse_mode[m.nModes];

    for (int j = 0; j < m.nModes; j++) {
      int k;

                                // Read the mouse cursor for this mode
      SGPropertyNode * mode_node = mouse_node->getChild("mode", j, true);
      const char * cursor_name =
        mode_node->getStringValue("cursor", "inherit");
      m.modes[j].cursor = MOUSE_CURSOR_POINTER;
      for (k = 0; mouse_cursor_map[k].name != 0; k++) {
        if (!strcmp(mouse_cursor_map[k].name, cursor_name)) {
          m.modes[j].cursor = mouse_cursor_map[k].cursor;
          break;
        }
      }

                                // Read other properties for this mode
      m.modes[j].constrained = mode_node->getBoolValue("constrained", false);
      m.modes[j].pass_through = mode_node->getBoolValue("pass-through", false);

                                // Read the button bindings for this mode
      m.modes[j].buttons = new button[MAX_MOUSE_BUTTONS];
      char buf[32];
      for (k = 0; k < MAX_MOUSE_BUTTONS; k++) {
        sprintf(buf, "mouse button %d", k);
        SG_LOG(SG_INPUT, SG_DEBUG, "Initializing mouse button " << k);
        _init_button(mode_node->getChild("button", k),
                     m.modes[j].buttons[k],
                     buf);
      }

                                // Read the axis bindings for this mode
      _read_bindings(mode_node->getChild("x-axis", 0, true),
                     m.modes[j].x_bindings,
                     KEYMOD_NONE);
      _read_bindings(mode_node->getChild("y-axis", 0, true),
                     m.modes[j].y_bindings,
                     KEYMOD_NONE);
    }
  }
}


void
FGInput::_init_button (const SGPropertyNode * node,
                       button &b,
                       const string name)
{       
  if (node == 0) {
    SG_LOG(SG_INPUT, SG_DEBUG, "No bindings for button " << name);
  } else {
    b.is_repeatable = node->getBoolValue("repeatable", b.is_repeatable);
    
                // Get the bindings for the button
    _read_bindings(node, b.bindings, KEYMOD_NONE);
  }
}


void
FGInput::_update_keyboard ()
{
  // no-op
}


void
FGInput::_update_joystick (double dt)
{
  int modifiers = KEYMOD_NONE;  // FIXME: any way to get the real ones?
  int buttons;
  // float js_val, diff;
  float axis_values[MAX_JOYSTICK_AXES];

  int i;
  int j;

  for ( i = 0; i < MAX_JOYSTICKS; i++) {

    jsJoystick * js = _joystick_bindings[i].js;
    if (js == 0 || js->notWorking())
      continue;

    js->read(&buttons, axis_values);

                                // Fire bindings for the axes.
    for ( j = 0; j < _joystick_bindings[i].naxes; j++) {
      axis &a = _joystick_bindings[i].axes[j];
      
                                // Do nothing if the axis position
                                // is unchanged; only a change in
                                // position fires the bindings.
      if (fabs(axis_values[j] - a.last_value) > a.tolerance) {
//      SG_LOG(SG_INPUT, SG_DEBUG, "Axis " << j << " has moved");
        a.last_value = axis_values[j];
//      SG_LOG(SG_INPUT, SG_DEBUG, "There are "
//             << a.bindings[modifiers].size() << " bindings");
        for (unsigned int k = 0; k < a.bindings[modifiers].size(); k++)
          a.bindings[modifiers][k]->fire(axis_values[j]);
      }
     
                                // do we have to emulate axis buttons?
      a.last_dt += dt;
      if(a.last_dt >= a.interval_sec) {
        if (a.low.bindings[modifiers].size())
          _update_button(_joystick_bindings[i].axes[j].low,
                         modifiers,
                         axis_values[j] < a.low_threshold,
                         -1, -1);
      
        if (a.high.bindings[modifiers].size())
          _update_button(_joystick_bindings[i].axes[j].high,
                         modifiers,
                         axis_values[j] > a.high_threshold,
                         -1, -1);
         a.last_dt -= a.interval_sec;
      }
    }

                                // Fire bindings for the buttons.
    for (j = 0; j < _joystick_bindings[i].nbuttons; j++) {
      button &b = _joystick_bindings[i].buttons[j];
      b.last_dt += dt;
      if(b.last_dt >= b.interval_sec) {
        _update_button(_joystick_bindings[i].buttons[j],
                       modifiers,
                       (buttons & (1 << j)) > 0,
                       -1, -1);
        b.last_dt -= b.interval_sec;
      }
    }
  }
}

void
FGInput::_update_mouse ( double dt )
{
  mouse &m = _mouse_bindings[0];
  int mode =  m.mode_node->getIntValue();
  if (mode != m.current_mode) {
    m.current_mode = mode;
    m.timeout = fgGetDouble( "/sim/mouse/cursor-timeout-sec", 10.0 );
    if (mode >= 0 && mode < m.nModes) {
      fgSetMouseCursor(m.modes[mode].cursor);
      m.x = fgGetInt("/sim/startup/xsize", 800) / 2;
      m.y = fgGetInt("/sim/startup/ysize", 600) / 2;
      fgWarpMouse(m.x, m.y);
    } else {
      SG_LOG(SG_INPUT, SG_DEBUG, "Mouse mode " << mode << " out of range");
      fgSetMouseCursor(MOUSE_CURSOR_POINTER);
    }
  }

  if ( fgGetBool( "/sim/mouse/hide-cursor", true ) ) {
      if ( m.x != m.save_x || m.y != m.save_y ) {
          m.timeout = fgGetDouble( "/sim/mouse/cursor-timeout-sec", 10.0 );
          fgSetMouseCursor(m.modes[mode].cursor);
      } else {
          m.timeout -= dt;
          if ( m.timeout <= 0.0 ) {
              fgSetMouseCursor(MOUSE_CURSOR_NONE);
              m.timeout = 0.0;
          }
      }
      m.save_x = m.x;
      m.save_y = m.y;
  }
}

void
FGInput::_update_button (button &b, int modifiers, bool pressed,
                         int x, int y)
{
  if (pressed) {
                                // The press event may be repeated.
    if (!b.last_state || b.is_repeatable) {
      SG_LOG( SG_INPUT, SG_DEBUG, "Button has been pressed" );
      for (unsigned int k = 0; k < b.bindings[modifiers].size(); k++)
        b.bindings[modifiers][k]->fire(x, y);
    }
  } else {
                                // The release event is never repeated.
    if (b.last_state) {
      SG_LOG( SG_INPUT, SG_DEBUG, "Button has been released" );
      for (unsigned int k = 0; k < b.bindings[modifiers|KEYMOD_RELEASED].size(); k++)
        b.bindings[modifiers|KEYMOD_RELEASED][k]->fire(x, y);
    }
  }
          
  b.last_state = pressed;
}  


void
FGInput::_read_bindings (const SGPropertyNode * node, 
                         binding_list_t * binding_list,
                         int modifiers)
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Reading all bindings");
  vector<SGPropertyNode_ptr> bindings = node->getChildren("binding");
  for (unsigned int i = 0; i < bindings.size(); i++) {
    const char *cmd = bindings[i]->getStringValue("command");
    SG_LOG(SG_INPUT, SG_DEBUG, "Reading binding " << cmd);

    if (!strcmp(cmd, "nasal") && !_module.empty())
      bindings[i]->setStringValue("module", _module.c_str());
    binding_list[modifiers].push_back(new FGBinding(bindings[i]));
  }

                                // Read nested bindings for modifiers
  if (node->getChild("mod-up") != 0)
    _read_bindings(node->getChild("mod-up"), binding_list,
                   modifiers|KEYMOD_RELEASED);

  if (node->getChild("mod-shift") != 0)
    _read_bindings(node->getChild("mod-shift"), binding_list,
                   modifiers|KEYMOD_SHIFT);

  if (node->getChild("mod-ctrl") != 0)
    _read_bindings(node->getChild("mod-ctrl"), binding_list,
                   modifiers|KEYMOD_CTRL);

  if (node->getChild("mod-alt") != 0)
    _read_bindings(node->getChild("mod-alt"), binding_list,
                   modifiers|KEYMOD_ALT);
}


const vector<FGBinding *> &
FGInput::_find_key_bindings (unsigned int k, int modifiers)
{
  unsigned char kc = (unsigned char)k;
  button &b = _key_bindings[k];

                                // Try it straight, first.
  if (b.bindings[modifiers].size() > 0)
    return b.bindings[modifiers];

                                // Alt-Gr is CTRL+ALT
  else if (modifiers&(KEYMOD_CTRL|KEYMOD_ALT))
    return _find_key_bindings(k, modifiers&~(KEYMOD_CTRL|KEYMOD_ALT));

                                // Try removing the control modifier
                                // for control keys.
  else if ((modifiers&KEYMOD_CTRL) && iscntrl(kc))
    return _find_key_bindings(k, modifiers&~KEYMOD_CTRL);

                                // Try removing shift modifier 
                                // for upper case or any punctuation
                                // (since different keyboards will
                                // shift different punctuation types)
  else if ((modifiers&KEYMOD_SHIFT) && (isupper(kc) || ispunct(kc)))
    return _find_key_bindings(k, modifiers&~KEYMOD_SHIFT);

                                // Try removing alt modifier for
                                // high-bit characters.
  else if ((modifiers&KEYMOD_ALT) && k >= 128 && k < 256)
    return _find_key_bindings(k, modifiers&~KEYMOD_ALT);

                                // Give up and return the empty vector.
  else
    return b.bindings[modifiers];
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::button.
////////////////////////////////////////////////////////////////////////

FGInput::button::button ()
  : is_repeatable(false),
    interval_sec(0),
    last_dt(0),
    last_state(0)
{
}

FGInput::button::~button ()
{
                                // FIXME: memory leak
//   for (int i = 0; i < KEYMOD_MAX; i++)
//     for (int j = 0; i < bindings[i].size(); j++)
//       delete bindings[i][j];
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::axis.
////////////////////////////////////////////////////////////////////////

FGInput::axis::axis ()
  : last_value(9999999),
    tolerance(0.002),
    low_threshold(-0.9),
    high_threshold(0.9),
    interval_sec(0),
    last_dt(0)
{
}

FGInput::axis::~axis ()
{
//   for (int i = 0; i < KEYMOD_MAX; i++)
//     for (int j = 0; i < bindings[i].size(); j++)
//       delete bindings[i][j];
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::joystick.
////////////////////////////////////////////////////////////////////////

FGInput::joystick::joystick ()
  : jsnum(0),
    js(0),
    naxes(0),
    nbuttons(0),
    axes(0),
    buttons(0)
{
}

FGInput::joystick::~joystick ()
{
//   delete js;
  delete[] axes;
  delete[] buttons;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::mouse_mode
////////////////////////////////////////////////////////////////////////

FGInput::mouse_mode::mouse_mode ()
  : cursor(MOUSE_CURSOR_POINTER),
    constrained(false),
    pass_through(false),
    buttons(0)
{
}

FGInput::mouse_mode::~mouse_mode ()
{
                                // FIXME: memory leak
//   for (int i = 0; i < KEYMOD_MAX; i++) {
//     int j;
//     for (j = 0; i < x_bindings[i].size(); j++)
//       delete bindings[i][j];
//     for (j = 0; j < y_bindings[i].size(); j++)
//       delete bindings[i][j];
//   }
  delete [] buttons;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInput::mouse
////////////////////////////////////////////////////////////////////////

FGInput::mouse::mouse ()
  : x(-1),
    y(-1),
    nModes(1),
    current_mode(0),
    modes(0)
{
}

FGInput::mouse::~mouse ()
{
  delete [] modes;
}

////////////////////////////////////////////////////////////////////////
// Implementation of OS callbacks.
////////////////////////////////////////////////////////////////////////

void keyHandler(int key, int keymod, int mousex, int mousey)
{
    if((keymod & KEYMOD_RELEASED) == 0)
        if(puKeyboard(key, PU_DOWN))
            return;

    if(default_input)
        default_input->doKey(key, keymod, mousex, mousey);
}

void mouseClickHandler(int button, int updown, int x, int y)
{
    if(default_input)
        default_input->doMouseClick(button, updown, x, y);
}

void mouseMotionHandler(int x, int y)
{
    if (default_input != 0)
        default_input->doMouseMotion(x, y);
}
