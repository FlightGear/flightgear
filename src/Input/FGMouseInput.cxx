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

#include <boost/foreach.hpp>
#include <osgGA/GUIEventAdapter>

#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/timing/timestamp.hxx>
#include <simgear/scene/model/SGPickAnimation.hxx>

#include "FGButton.hxx"
#include "Main/globals.hxx"
#include <Viewer/renderer.hxx>
#include <plib/pu.h>
#include <Model/panelnode.hxx>
#include <Cockpit/panel.hxx>
#include <Viewer/FGEventHandler.hxx>
#include <GUI/MouseCursor.hxx>

using std::ios_base;

const int MAX_MICE = 1;
const int MAX_MOUSE_BUTTONS = 8;

typedef std::vector<SGSceneryPick> SGSceneryPicks;
typedef SGSharedPtr<SGPickCallback> SGPickCallbackPtr;
typedef std::list<SGPickCallbackPtr> SGPickCallbackList;

////////////////////////////////////////////////////////////////////////

/**
 * List of currently pressed mouse button events
 */
class ActivePickCallbacks:
  public std::map<int, SGPickCallbackList>
{
  public:
    void update( double dt, unsigned int keyModState );
    void init( int button, const osgGA::GUIEventAdapter* ea );
};


void ActivePickCallbacks::init( int button, const osgGA::GUIEventAdapter* ea )
{
  osg::Vec2d windowPos;
  flightgear::eventToWindowCoords(ea, windowPos.x(), windowPos.y());
    
  // Get the list of hit callbacks. Take the first callback that
  // accepts the mouse button press and ignore the rest of them
  // That is they get sorted by distance and by scenegraph depth.
  // The nearest one is the first one and the deepest
  // (the most specialized one in the scenegraph) is the first.
  SGSceneryPicks pickList;
  if (!globals->get_renderer()->pick(pickList, windowPos)) {
    return;
  }

  SGSceneryPicks::const_iterator i;
  for (i = pickList.begin(); i != pickList.end(); ++i) {
    if (i->callback->buttonPressed(button, *ea, i->info)) {
        (*this)[button].push_back(i->callback);
        return;
    }
  }
}

void ActivePickCallbacks::update( double dt, unsigned int keyModState )
{
  // handle repeatable mouse press events
  for( iterator mi = begin(); mi != end(); ++mi ) {
    SGPickCallbackList::iterator li;
    for (li = mi->second.begin(); li != mi->second.end(); ++li) {
      (*li)->update(dt, keyModState);
    }
  }
}

////////////////////////////////////////////////////////////////////////


/**
 * Settings for a mouse mode.
 */
struct mouse_mode {
    mouse_mode ();
    virtual ~mouse_mode ();
    FGMouseCursor::Cursor cursor;
    bool constrained;
    bool pass_through;
    FGButton * buttons;
    SGBindingList x_bindings[KEYMOD_MAX];
    SGBindingList y_bindings[KEYMOD_MAX];
};


/**
 * Settings for a mouse.
 */
struct mouse {
    mouse ();
    virtual ~mouse ();
    int x, y;
    SGPropertyNode_ptr mode_node;
    SGPropertyNode_ptr mouse_button_nodes[MAX_MOUSE_BUTTONS];
    int nModes;
    int current_mode;
    
    SGTimeStamp timeSinceLastMove;
    mouse_mode * modes;
};

static
const SGSceneryPick*
getPick( const SGSceneryPicks& pick_list,
         const SGPickCallback* cb )
{
  for(size_t i = 0; i < pick_list.size(); ++i)
    if( pick_list[i].callback == cb )
      return &pick_list[i];

  return 0;
}

////////////////////////////////////////////////////////////////////////

class FGMouseInput::FGMouseInputPrivate : public SGPropertyChangeListener
{
public:
    FGMouseInputPrivate() :
        initialized(false),
        haveWarped(false),
        xSizeNode(fgGetNode("/sim/startup/xsize", false ) ),
        ySizeNode(fgGetNode("/sim/startup/ysize", false ) ),
        xAccelNode(fgGetNode("/devices/status/mice/mouse/accel-x", true ) ),
        yAccelNode(fgGetNode("/devices/status/mice/mouse/accel-y", true ) ),
        mouseXNode(fgGetNode("/devices/status/mice/mouse/x", true)),
        mouseYNode(fgGetNode("/devices/status/mice/mouse/y", true))
    {
        tooltipTimeoutDone = false;
        hoverPickScheduled = false;
        tooltipsEnabled = false;
        
        fgGetNode("/sim/mouse/hide-cursor", true )->addChangeListener(this, true);
        fgGetNode("/sim/mouse/cursor-timeout-sec", true )->addChangeListener(this, true);
        fgGetNode("/sim/mouse/right-button-mode-cycle-enabled", true)->addChangeListener(this, true);
        fgGetNode("/sim/mouse/tooltip-delay-msec", true)->addChangeListener(this, true);
        fgGetNode("/sim/mouse/click-shows-tooltip", true)->addChangeListener(this, true);
        fgGetNode("/sim/mouse/tooltips-enabled", true)->addChangeListener(this, true);
        fgGetNode("/sim/mouse/drag-sensitivity", true)->addChangeListener(this, true);
        fgGetNode("/sim/mouse/invert-mouse-wheel", true)->addChangeListener(this, true);
    }
  
    void centerMouseCursor(mouse& m)
    {    
      // center the cursor
      m.x = (xSizeNode ? xSizeNode->getIntValue() : 800) / 2;
      m.y = (ySizeNode ? ySizeNode->getIntValue() : 600) / 2;
      fgWarpMouse(m.x, m.y);
      haveWarped = true;
    }
  
    void constrainMouse(int x, int y)
    {
        int new_x=x,new_y=y;
        int xsize = xSizeNode ? xSizeNode->getIntValue() : 800;
        int ysize = ySizeNode ? ySizeNode->getIntValue() : 600;
        
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
        }
    }

    void scheduleHoverPick(const osg::Vec2d& windowPos)
    {
      hoverPickScheduled = true;
      hoverPos = windowPos;
    }
  
    void doHoverPick(const osg::Vec2d& windowPos)
    {
        FGMouseCursor::Cursor cur = FGMouseCursor::CURSOR_ARROW;
        bool explicitCursor = false;
        bool didPick = false;

        SGPickCallback::Priority priority = SGPickCallback::PriorityScenery;
        SGSceneryPicks pickList;
        globals->get_renderer()->pick(pickList, windowPos);

        SGSceneryPicks::const_iterator i;
        for( i = pickList.begin(); i != pickList.end(); ++i )
        {
            bool done = i->callback->hover(windowPos, i->info);
            std::string curName(i->callback->getCursor());
            if (!curName.empty()) {
                explicitCursor = true;
                cur = FGMouseCursor::cursorFromString(curName.c_str());
            }
            
            // if the callback is of higher prioirty (lower enum index),
            // record that.
            if (i->callback->getPriority() < priority) {
                priority = i->callback->getPriority();
            }

            if (done) {
                didPick = true;
                break;
            }
        } // of picks iteration

        // Check if any pick from the previous iteration has disappeared. If so
        // notify the callback that the mouse has left its element.
        for( i = _previous_picks.begin(); i != _previous_picks.end(); ++i )
        {
          if( !getPick(pickList, i->callback) )
            i->callback->mouseLeave(windowPos);
        }
        _previous_picks = pickList;
      
        if (!explicitCursor && (priority == SGPickCallback::PriorityPanel)) {
            cur = FGMouseCursor::CURSOR_HAND;
        }
        
        FGMouseCursor::instance()->setCursor(cur);
        if (!didPick) {
          SGPropertyNode_ptr args(new SGPropertyNode);
          globals->get_commands()->execute("update-hover", args);

        }
    }
    
    void doMouseMoveWithCallbacks(const osgGA::GUIEventAdapter* ea)
    {
        FGMouseCursor::Cursor cur = FGMouseCursor::CURSOR_CLOSED_HAND;
        
        osg::Vec2d windowPos;
        flightgear::eventToWindowCoords(ea, windowPos.x(), windowPos.y());

        SGSceneryPicks pickList;
        if( !globals->get_renderer()->pick(pickList, windowPos) )
          return;

        for( ActivePickCallbacks::iterator mi = activePickCallbacks.begin();
                                           mi != activePickCallbacks.end();
                                         ++mi )
        {
          SGPickCallbackList::iterator li;
          for( li = mi->second.begin(); li != mi->second.end(); ++li )
          {
            const SGSceneryPick* pick = getPick(pickList, *li);
            (*li)->mouseMoved(*ea, pick ? &pick->info : 0);

            std::string curName((*li)->getCursor());
            if( !curName.empty() )
              cur = FGMouseCursor::cursorFromString(curName.c_str());
          }
        }

        FGMouseCursor::instance()->setCursor(cur);
    }

    // implement the property-change-listener interfacee
    virtual void valueChanged( SGPropertyNode * node )
    {
        if (node->getNameString() == "drag-sensitivity") {
            SGKnobAnimation::setDragSensitivity(node->getDoubleValue());
        } else if (node->getNameString() == "invert-mouse-wheel") {
            SGKnobAnimation::setAlternateMouseWheelDirection(node->getBoolValue());
        } else if (node->getNameString() == "hide-cursor") {
            hideCursor = node->getBoolValue();
        } else if (node->getNameString() == "cursor-timeout-sec") {
            cursorTimeoutMsec = node->getDoubleValue() * 1000;
        } else if (node->getNameString() == "tooltip-delay-msec") {
            tooltipDelayMsec = node->getIntValue();
        } else if (node->getNameString() == "right-button-mode-cycle-enabled") {
            rightClickModeCycle = node->getBoolValue();
        } else if (node->getNameString() == "click-shows-tooltip") {
            clickTriggersTooltip = node->getBoolValue();
        } else if (node->getNameString() == "tooltips-enabled") {
            tooltipsEnabled = node->getBoolValue();
        }
    }
    
    ActivePickCallbacks activePickCallbacks;
    SGSceneryPicks _previous_picks;

    mouse mice[MAX_MICE];
    
    bool initialized;
    bool hideCursor, haveWarped;
    bool tooltipTimeoutDone;
    bool clickTriggersTooltip;
    int tooltipDelayMsec, cursorTimeoutMsec;
    bool rightClickModeCycle;
    bool tooltipsEnabled;
    
    SGPropertyNode_ptr xSizeNode;
    SGPropertyNode_ptr ySizeNode;
    SGPropertyNode_ptr xAccelNode;
    SGPropertyNode_ptr yAccelNode;
    SGPropertyNode_ptr mouseXNode, mouseYNode;
  
    bool hoverPickScheduled;
    osg::Vec2d hoverPos;
};


////////////////////////////////////////////////////////////////////////
// The Mouse Input Implementation
////////////////////////////////////////////////////////////////////////

static FGMouseInput* global_mouseInput = NULL;

static void mouseClickHandler(int button, int updown, int x, int y, bool mainWindow, const osgGA::GUIEventAdapter* ea)
{
    if(global_mouseInput)
        global_mouseInput->doMouseClick(button, updown, x, y, mainWindow, ea);
}

static void mouseMotionHandler(int x, int y, const osgGA::GUIEventAdapter* ea)
{
    if (global_mouseInput != 0)
        global_mouseInput->doMouseMotion(x, y, ea);
}



FGMouseInput::FGMouseInput() :
  d(new FGMouseInputPrivate)
{
    global_mouseInput = this;
}

FGMouseInput::~FGMouseInput()
{
    global_mouseInput = NULL;
}

void FGMouseInput::init()
{
    if (d->initialized) {
        SG_LOG(SG_INPUT, SG_WARN, "Duplicate init of FGMouseInput");

        return;
    }
    
    d->initialized = true;
    
  SG_LOG(SG_INPUT, SG_DEBUG, "Initializing mouse bindings");
  std::string module = "";

  SGPropertyNode * mouse_nodes = fgGetNode("/input/mice");
  if (mouse_nodes == 0) {
    SG_LOG(SG_INPUT, SG_WARN, "No mouse bindings (/input/mice)!!");
    mouse_nodes = fgGetNode("/input/mice", true);
  }

  int j;
  for (int i = 0; i < MAX_MICE; i++) {
    SGPropertyNode * mouse_node = mouse_nodes->getChild("mouse", i, true);
    mouse &m = d->mice[i];

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
      SGPropertyNode * mode_node = mouse_node->getChild("mode", j, true);

    // Read the mouse cursor for this mode
      m.modes[j].cursor = FGMouseCursor::cursorFromString(mode_node->getStringValue("cursor", "inherit"));
        
      // Read other properties for this mode
      m.modes[j].constrained = mode_node->getBoolValue("constrained", false);
      m.modes[j].pass_through = mode_node->getBoolValue("pass-through", false);

      // Read the button bindings for this mode
      m.modes[j].buttons = new FGButton[MAX_MOUSE_BUTTONS];
      std::ostringstream buf;
      for (k = 0; k < MAX_MOUSE_BUTTONS; k++) {
        buf.seekp(ios_base::beg);
        buf << "mouse button " << k;
        m.modes[j].buttons[k].init( mode_node->getChild("button", k), buf.str(), module );
      }

      // Read the axis bindings for this mode
      read_bindings(mode_node->getChild("x-axis", 0, true), m.modes[j].x_bindings, KEYMOD_NONE, module );
      read_bindings(mode_node->getChild("y-axis", 0, true), m.modes[j].y_bindings, KEYMOD_NONE, module );
      
      if (mode_node->hasChild("x-axis-ctrl")) {
        read_bindings(mode_node->getChild("x-axis-ctrl"), m.modes[j].x_bindings, KEYMOD_CTRL, module );
      }
      if (mode_node->hasChild("x-axis-shift")) {
        read_bindings(mode_node->getChild("x-axis-shift"), m.modes[j].x_bindings, KEYMOD_SHIFT, module );
      }
      if (mode_node->hasChild("x-axis-ctrl-shift")) {
        read_bindings(mode_node->getChild("x-axis-ctrl-shift"), m.modes[j].x_bindings, KEYMOD_CTRL|KEYMOD_SHIFT, module );
      }
      
      if (mode_node->hasChild("y-axis-ctrl")) {
        read_bindings(mode_node->getChild("y-axis-ctrl"), m.modes[j].y_bindings, KEYMOD_CTRL, module );
      }
      if (mode_node->hasChild("y-axis-shift")) {
        read_bindings(mode_node->getChild("y-axis-shift"), m.modes[j].y_bindings, KEYMOD_SHIFT, module );
      }
      if (mode_node->hasChild("y-axis-ctrl-shift")) {
        read_bindings(mode_node->getChild("y-axis-ctrl-shift"), m.modes[j].y_bindings, KEYMOD_CTRL|KEYMOD_SHIFT, module );
      }
    } // of modes iteration
  }

  fgRegisterMouseClickHandler(mouseClickHandler);
  fgRegisterMouseMotionHandler(mouseMotionHandler);
}

void FGMouseInput::update ( double dt )
{
    if (!d->initialized) {
        SG_LOG(SG_INPUT, SG_WARN, "update of mouse before init");
    }
    
  mouse &m = d->mice[0];
  int mode =  m.mode_node->getIntValue();
  if (mode != m.current_mode) {
    // current mode has changed
    m.current_mode = mode;
    m.timeSinceLastMove.stamp();
      
    if (mode >= 0 && mode < m.nModes) {
      FGMouseCursor::instance()->setCursor(m.modes[mode].cursor);
      d->centerMouseCursor(m);
    } else {
      SG_LOG(SG_INPUT, SG_WARN, "Mouse mode " << mode << " out of range");
      FGMouseCursor::instance()->setCursor(FGMouseCursor::CURSOR_ARROW);
    }
  }

  if ((mode == 0) && d->hoverPickScheduled) {
    d->doHoverPick(d->hoverPos);
    d->hoverPickScheduled = false;
  }
  
  if ( !d->tooltipTimeoutDone &&
      d->tooltipsEnabled &&
      (m.timeSinceLastMove.elapsedMSec() > d->tooltipDelayMsec))
  {
      d->tooltipTimeoutDone = true;
      SGPropertyNode_ptr arg(new SGPropertyNode);
      globals->get_commands()->execute("tooltip-timeout", arg);
  }
  
  if ( d->hideCursor ) {
      if ( m.timeSinceLastMove.elapsedMSec() > d->cursorTimeoutMsec) {
          FGMouseCursor::instance()->hideCursorUntilMouseMove();
          m.timeSinceLastMove.stamp();
      }
  }
    
  d->activePickCallbacks.update( dt, fgGetKeyModifiers() );
}

mouse::mouse ()
  : x(-1),
    y(-1),
    nModes(1),
    current_mode(0),
    modes(NULL)
{
}

mouse::~mouse ()
{
  delete [] modes;
}

mouse_mode::mouse_mode ()
  : cursor(FGMouseCursor::CURSOR_ARROW),
    constrained(false),
    pass_through(false),
    buttons(NULL)
{
}

mouse_mode::~mouse_mode ()
{
                                // FIXME: memory leak
//   for (int i = 0; i < KEYMOD_MAX; i++) {
//     int j;
//     for (j = 0; i < x_bindings[i].size(); j++)
//       delete bindings[i][j];
//     for (j = 0; j < y_bindings[i].size(); j++)
//       delete bindings[i][j];
//   }
  if (buttons) {
    delete [] buttons;
  }
}

void FGMouseInput::doMouseClick (int b, int updown, int x, int y, bool mainWindow, const osgGA::GUIEventAdapter* ea)
{
    if (!d->initialized) {
        // can occur during reset
        return;
    }
    
  int modifiers = fgGetKeyModifiers();

  mouse &m = d->mice[0];
  mouse_mode &mode = m.modes[m.current_mode];
                                // Let the property manager know.
  if (b >= 0 && b < MAX_MOUSE_BUTTONS)
    m.mouse_button_nodes[b]->setBoolValue(updown == MOUSE_BUTTON_DOWN);

  if (!d->rightClickModeCycle && (b == 2)) {
    // in spring-loaded look mode, ignore right clicks entirely here
    return;
  }
  
  // Pass on to PUI and the panel if
  // requested, and return if one of
  // them consumes the event.

  osg::Vec2d windowPos;
  flightgear::eventToWindowCoords(ea, windowPos.x(), windowPos.y());

  SGSceneryPicks pickList;
  globals->get_renderer()->pick(pickList, windowPos);

  if( updown != MOUSE_BUTTON_DOWN )
  {
    // Execute the mouse up event in any case, may be we should
    // stop processing here?

    SGPickCallbackList& callbacks = d->activePickCallbacks[b];

    while( !callbacks.empty() )
    {
      SGPickCallbackPtr& cb = callbacks.front();
      const SGSceneryPick* pick = getPick(pickList, cb);
      cb->buttonReleased(ea->getModKeyMask(), *ea, pick ? &pick->info : 0);

      callbacks.pop_front();
    }
  }

  if (mode.pass_through) {
    // remove once PUI uses standard picking mechanism
    if (0 <= x && 0 <= y && puMouse(b, updown, x, y))
      return; // pui handled it

    // pui didn't want the click event so compute a
    // scenegraph intersection point corresponding to the mouse click
    if (updown == MOUSE_BUTTON_DOWN) {
      d->activePickCallbacks.init( b, ea );
        
      if (d->clickTriggersTooltip) {
            SGPropertyNode_ptr args(new SGPropertyNode);
            args->setStringValue("reason", "click");
            globals->get_commands()->execute("tooltip-timeout", args);
            d->tooltipTimeoutDone = true;
      }
    } else {
      // do a hover pick now, to fix up cursor
      d->doHoverPick(windowPos);
    } // mouse button was released
  } // of pass-through mode

  // OK, PUI and the panel didn't want the click
  if (b >= MAX_MOUSE_BUTTONS) {
    SG_LOG(SG_INPUT, SG_ALERT, "Mouse button " << b
           << " where only " << MAX_MOUSE_BUTTONS << " expected");
    return;
  }

  m.modes[m.current_mode].buttons[b].update( modifiers, 0 != updown, x, y);  
}

void FGMouseInput::processMotion(int x, int y, const osgGA::GUIEventAdapter* ea)
{
  if (!d->activePickCallbacks[0].empty()) {
    d->doMouseMoveWithCallbacks(ea);
    return;
  }
  
  mouse &m = d->mice[0];
  int modeIndex = m.current_mode;
  // are we in spring-loaded look mode?
  if (!d->rightClickModeCycle && m.nModes > 3) {
    if (m.mouse_button_nodes[2]->getBoolValue()) {
      // right mouse is down, force look mode
      modeIndex = 3;
    }
  }

  if (modeIndex == 0) {
    osg::Vec2d windowPos;
    flightgear::eventToWindowCoords(ea, windowPos.x(), windowPos.y());
    d->scheduleHoverPick(windowPos);
    // mouse has moved, so we may need to issue tooltip-timeout command again
    d->tooltipTimeoutDone = false;
  }
  
  mouse_mode &mode = m.modes[modeIndex];
  
  // Pass on to PUI if requested, and return
  // if PUI consumed the event.
  if (mode.pass_through && puMouse(x, y)) {
    return;
  }

  if (d->haveWarped)
  {
    // don't fire mouse-movement events at the first update after warping the mouse,
    // just remember the new mouse position
    d->haveWarped = false;
  }
  else
  {
    int modifiers = fgGetKeyModifiers();
    int xsize = d->xSizeNode ? d->xSizeNode->getIntValue() : 800;
    int ysize = d->ySizeNode ? d->ySizeNode->getIntValue() : 600;
      
    // OK, PUI didn't want the event,
    // so we can play with it.
    if (x != m.x) {
      int delta = x - m.x;
      d->xAccelNode->setIntValue( delta );
      for (unsigned int i = 0; i < mode.x_bindings[modifiers].size(); i++)
        mode.x_bindings[modifiers][i]->fire(double(delta), double(xsize));
    }
    if (y != m.y) {
      int delta = y - m.y;
      d->yAccelNode->setIntValue( -delta );
      for (unsigned int i = 0; i < mode.y_bindings[modifiers].size(); i++)
        mode.y_bindings[modifiers][i]->fire(double(delta), double(ysize));
    }
  }
  
  // Constrain the mouse if requested
  if (mode.constrained) {
    d->constrainMouse(x, y);
  }
}

void FGMouseInput::doMouseMotion (int x, int y, const osgGA::GUIEventAdapter* ea)
{
    if (!d->initialized) {
        // can occur during reset
        return;
    }
    
  mouse &m = d->mice[0];

  if (m.current_mode < 0 || m.current_mode >= m.nModes) {
      m.x = x;
      m.y = y;
      return;
  }

  m.timeSinceLastMove.stamp();
  FGMouseCursor::instance()->mouseMoved();

  // TODO Get rid of this as soon as soon as cursor hide timeout works globally
  if( ea->getHandled() )
    return;

  processMotion(x, y, ea);
    
  m.x = x;
  m.y = y;
  d->mouseXNode->setIntValue(x);
  d->mouseYNode->setIntValue(y);
}



