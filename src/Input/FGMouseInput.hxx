// FGMouseInput.hxx -- handle user input from mouse devices
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


#ifndef _FGMOUSEINPUT_HXX
#define _FGMOUSEINPUT_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif

#include "FGCommonInput.hxx"
#include "FGButton.hxx"

#include <map>
#include <list>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/scene/util/SGPickCallback.hxx>
#include <Viewer/renderer.hxx>

/**
  * List of currently pressed mouse button events
  */
class ActivePickCallbacks : public std::map<int, std::list<SGSharedPtr<SGPickCallback> > > {
public:
  void update( double dt );
  void init( int b, const osgGA::GUIEventAdapter* ea );
};


////////////////////////////////////////////////////////////////////////
// The Mouse Input Class
////////////////////////////////////////////////////////////////////////
class FGMouseInput : public SGSubsystem,FGCommonInput {
public:
  FGMouseInput();
  virtual ~FGMouseInput();

  virtual void init();
  virtual void update( double dt );

  static const int MAX_MICE = 1;
  static const int MAX_MOUSE_BUTTONS = 8;

private:
  void doMouseClick (int b, int updown, int x, int y, bool mainWindow, const osgGA::GUIEventAdapter* ea);
  void doMouseMotion (int x, int y);
  static FGMouseInput * mouseInput;
  static void mouseClickHandler(int button, int updown, int x, int y, bool mainWindow, const osgGA::GUIEventAdapter*);
  static void mouseMotionHandler(int x, int y);

  ActivePickCallbacks activePickCallbacks;
  /**
   * Settings for a mouse mode.
   */
  struct mouse_mode {
    mouse_mode ();
    virtual ~mouse_mode ();
    int cursor;
    bool constrained;
    bool pass_through;
    FGButton * buttons;
    binding_list_t x_bindings[KEYMOD_MAX];
    binding_list_t y_bindings[KEYMOD_MAX];
  };


  /**
   * Settings for a mouse.
   */
  struct mouse {
    mouse ();
    virtual ~mouse ();
    int x;
    int y;
    int save_x;
    int save_y;
    SGPropertyNode_ptr mode_node;
    SGPropertyNode_ptr mouse_button_nodes[MAX_MOUSE_BUTTONS];
    int nModes;
    int current_mode;
    double timeout;
    mouse_mode * modes;
  };

  // 
  // Map of all known cursor names
  // This used to contain all the Glut cursors, but those are
  // not defined by other toolkits.  It now supports only the cursor
  // images we actually use, in the interest of portability.  Someday,
  // it would be cool to write an OpenGL cursor renderer, with the
  // cursors defined as textures referenced in the property tree.  This
  // list could then be eliminated. -Andy
  //
  const static struct MouseCursorMap {
    const char * name;
    int cursor;
  } mouse_cursor_map[];

  mouse bindings[MAX_MICE];
  
  bool haveWarped;

  SGPropertyNode_ptr xSizeNode;
  SGPropertyNode_ptr ySizeNode;
  SGPropertyNode_ptr xAccelNode;
  SGPropertyNode_ptr yAccelNode;
  SGPropertyNode_ptr hideCursorNode;
  SGPropertyNode_ptr cursorTimeoutNode;
};

#endif
