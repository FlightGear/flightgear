// FGMouseInput.cxx -- handle user input from mouse devices
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

#include "FGMouseInput.hxx"
#include "Main/globals.hxx"

using std::ios_base;

void ActivePickCallbacks::init( int b, const osgGA::GUIEventAdapter* ea )
{
  // Get the list of hit callbacks. Take the first callback that
  // accepts the mouse button press and ignore the rest of them
  // That is they get sorted by distance and by scenegraph depth.
  // The nearest one is the first one and the deepest
  // (the most specialized one in the scenegraph) is the first.
  std::vector<SGSceneryPick> pickList;
  if (globals->get_renderer()->pick(pickList, ea)) {
    std::vector<SGSceneryPick>::const_iterator i;
    for (i = pickList.begin(); i != pickList.end(); ++i) {
      if (i->callback->buttonPressed(b, i->info)) {
          (*this)[b].push_back(i->callback);
          return;
      }
    }
  }
}

void ActivePickCallbacks::update( double dt )
{
  // handle repeatable mouse press events
  for( iterator mi = begin(); mi != end(); ++mi ) {
    std::list<SGSharedPtr<SGPickCallback> >::iterator li;
    for (li = mi->second.begin(); li != mi->second.end(); ++li) {
      (*li)->update(dt);
    }
  }
}


#include <plib/pu.h>
#include <Model/panelnode.hxx>
#include <Cockpit/panel.hxx>
////////////////////////////////////////////////////////////////////////
// The Mouse Input Implementation
////////////////////////////////////////////////////////////////////////

const FGMouseInput::MouseCursorMap FGMouseInput::mouse_cursor_map[] = {
    { "none", MOUSE_CURSOR_NONE },
    { "inherit", MOUSE_CURSOR_POINTER },
    { "wait", MOUSE_CURSOR_WAIT },
    { "crosshair", MOUSE_CURSOR_CROSSHAIR },
    { "left-right", MOUSE_CURSOR_LEFTRIGHT },
    { 0, 0 }
};

FGMouseInput * FGMouseInput::mouseInput = NULL;

FGMouseInput::FGMouseInput() :
  haveWarped(false),
  xSizeNode(fgGetNode("/sim/startup/xsize", false ) ),
  ySizeNode(fgGetNode("/sim/startup/ysize", false ) ),
  xAccelNode(fgGetNode("/devices/status/mice/mouse/accel-x", true ) ),
  yAccelNode(fgGetNode("/devices/status/mice/mouse/accel-y", true ) ),
  hideCursorNode(fgGetNode("/sim/mouse/hide-cursor", true ) ),
  cursorTimeoutNode(fgGetNode("/sim/mouse/cursor-timeout-sec", true ) )
{
  if( mouseInput == NULL )
    mouseInput = this;
}

FGMouseInput::~FGMouseInput()
{
  if( mouseInput == this )
    mouseInput = NULL;
}

void FGMouseInput::init()
{
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing mouse bindings");
  string module = "";

  SGPropertyNode * mouse_nodes = fgGetNode("/input/mice");
  if (mouse_nodes == 0) {
    SG_LOG(SG_INPUT, SG_WARN, "No mouse bindings (/input/mice)!!");
    mouse_nodes = fgGetNode("/input/mice", true);
  }

  int j;
  for (int i = 0; i < MAX_MICE; i++) {
    SGPropertyNode * mouse_node = mouse_nodes->getChild("mouse", i, true);
    mouse &m = bindings[i];

                                // Grab node pointers
    std::ostringstream buf;
    buf <<  "/devices/status/mice/mouse[" << i << "]/mode";
    m.mode_node = fgGetNode(buf.str().c_str());
    if (m.mode_node == NULL) {
      m.mode_node = fgGetNode(buf.str().c_str(), true);
      m.mode_node->setIntValue(0);
    }
    for (j = 0; j < MAX_MOUSE_BUTTONS; j++) {
      buf.seekp(ios_base::beg);
      buf << "/devices/status/mice/mouse["<< i << "]/button[" << j << "]";
      m.mouse_button_nodes[j] = fgGetNode(buf.str().c_str(), true);
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
      m.modes[j].buttons = new FGButton[MAX_MOUSE_BUTTONS];
      std::ostringstream buf;
      for (k = 0; k < MAX_MOUSE_BUTTONS; k++) {
        buf.seekp(ios_base::beg);
        buf << "mouse button " << k;
        SG_LOG(SG_INPUT, SG_DEBUG, "Initializing mouse button " << k);
        m.modes[j].buttons[k].init( mode_node->getChild("button", k), buf.str(), module );
      }

                                // Read the axis bindings for this mode
      read_bindings(mode_node->getChild("x-axis", 0, true), m.modes[j].x_bindings, KEYMOD_NONE, module );
      read_bindings(mode_node->getChild("y-axis", 0, true), m.modes[j].y_bindings, KEYMOD_NONE, module );
    }
  }

  fgRegisterMouseClickHandler(mouseClickHandler);
  fgRegisterMouseMotionHandler(mouseMotionHandler);
}

void FGMouseInput::update ( double dt )
{
  double cursorTimeout = cursorTimeoutNode ? cursorTimeoutNode->getDoubleValue() : 10.0;

  mouse &m = bindings[0];
  int mode =  m.mode_node->getIntValue();
  if (mode != m.current_mode) {
    m.current_mode = mode;
    m.timeout = cursorTimeout;
    if (mode >= 0 && mode < m.nModes) {
      fgSetMouseCursor(m.modes[mode].cursor);
      m.x = (xSizeNode ? xSizeNode->getIntValue() : 800) / 2;
      m.y = (ySizeNode ? ySizeNode->getIntValue() : 600) / 2;
      fgWarpMouse(m.x, m.y);
      haveWarped = true;
    } else {
      SG_LOG(SG_INPUT, SG_DEBUG, "Mouse mode " << mode << " out of range");
      fgSetMouseCursor(MOUSE_CURSOR_POINTER);
    }
  }

  if ( hideCursorNode ==NULL || hideCursorNode->getBoolValue() ) {
      if ( m.x != m.save_x || m.y != m.save_y ) {
          m.timeout = cursorTimeout;
          if (fgGetMouseCursor() == MOUSE_CURSOR_NONE)
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

  activePickCallbacks.update( dt );
}

FGMouseInput::mouse::mouse ()
  : x(-1),
    y(-1),
    save_x(-1),
    save_y(-1),
    nModes(1),
    current_mode(0),
    timeout(0),
    modes(NULL)
{
}

FGMouseInput::mouse::~mouse ()
{
  delete [] modes;
}

FGMouseInput::mouse_mode::mouse_mode ()
  : cursor(MOUSE_CURSOR_POINTER),
    constrained(false),
    pass_through(false),
    buttons(NULL)
{
}

FGMouseInput::mouse_mode::~mouse_mode ()
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

void FGMouseInput::doMouseClick (int b, int updown, int x, int y, bool mainWindow, const osgGA::GUIEventAdapter* ea)
{
  int modifiers = fgGetKeyModifiers();

  mouse &m = bindings[0];
  mouse_mode &mode = m.modes[m.current_mode];

                                // Let the property manager know.
  if (b >= 0 && b < MAX_MOUSE_BUTTONS)
    m.mouse_button_nodes[b]->setBoolValue(updown == MOUSE_BUTTON_DOWN);

                                // Pass on to PUI and the panel if
                                // requested, and return if one of
                                // them consumes the event.

  if (updown != MOUSE_BUTTON_DOWN) {
    // Execute the mouse up event in any case, may be we should
    // stop processing here?
    while (!activePickCallbacks[b].empty()) {
      activePickCallbacks[b].front()->buttonReleased();
      activePickCallbacks[b].pop_front();
    }
  }

  if (mode.pass_through) {
    // remove once PUI uses standard picking mechanism
    if (0 <= x && 0 <= y && puMouse(b, updown, x, y))
      return;
    else {
      // pui didn't want the click event so compute a
      // scenegraph intersection point corresponding to the mouse click
      if (updown == MOUSE_BUTTON_DOWN) {
        activePickCallbacks.init( b, ea );
      }
    }
  }

  // OK, PUI and the panel didn't want the click
  if (b >= MAX_MOUSE_BUTTONS) {
    SG_LOG(SG_INPUT, SG_ALERT, "Mouse button " << b
           << " where only " << MAX_MOUSE_BUTTONS << " expected");
    return;
  }

  m.modes[m.current_mode].buttons[b].update( modifiers, 0 != updown, x, y);
}

void FGMouseInput::doMouseMotion (int x, int y)
{
  // Don't call fgGetKeyModifiers() here, until we are using a
  // toolkit that supports getting the mods from outside a key
  // callback.  Glut doesn't.
  int modifiers = KEYMOD_NONE;

  int xsize = xSizeNode ? xSizeNode->getIntValue() : 800;
  int ysize = ySizeNode ? ySizeNode->getIntValue() : 600;

  mouse &m = bindings[0];

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
  
  if (haveWarped)
  {
      // don't fire mouse-movement events at the first update after warping the mouse,
      // just remember the new mouse position
      haveWarped = false;
  }
  else
  {
                                // OK, PUI didn't want the event,
                                // so we can play with it.
      if (x != m.x) {
        int delta = x - m.x;
        xAccelNode->setIntValue( delta );
        for (unsigned int i = 0; i < mode.x_bindings[modifiers].size(); i++)
          mode.x_bindings[modifiers][i]->fire(double(delta), double(xsize));
      }
      if (y != m.y) {
        int delta = y - m.y;
        yAccelNode->setIntValue( -delta );
        for (unsigned int i = 0; i < mode.y_bindings[modifiers].size(); i++)
          mode.y_bindings[modifiers][i]->fire(double(delta), double(ysize));
      }
  }
                                // Constrain the mouse if requested
  if (mode.constrained) {
    int new_x=x,new_y=y;
    
    bool need_warp = false;
    if (x <= (xsize * .25) || x >= (xsize * .75)) {
      new_x = int(xsize * .5);
      need_warp = true;
    }

    if (y <= (ysize * .25) || y >= (ysize * .75)) {
      new_y = int(ysize * .5);
      need_warp = true;
    }

    if (need_warp)
    {
      fgWarpMouse(new_x, new_y);
      haveWarped = true;
      SG_LOG(SG_INPUT, SG_DEBUG, "Mouse warp: " << x << ", " << y << " => " << new_x << ", " << new_y);
    }
  }

  if (m.x != x)
      fgSetInt("/devices/status/mice/mouse/x", m.x = x);

  if (m.y != y)
      fgSetInt("/devices/status/mice/mouse/y", m.y = y);
}

void FGMouseInput::mouseClickHandler(int button, int updown, int x, int y, bool mainWindow, const osgGA::GUIEventAdapter* ea)
{
    if(mouseInput)
      mouseInput->doMouseClick(button, updown, x, y, mainWindow, ea);
}

void FGMouseInput::mouseMotionHandler(int x, int y)
{
    if (mouseInput != 0)
        mouseInput->doMouseMotion(x, y);
}


