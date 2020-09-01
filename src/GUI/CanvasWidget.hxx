// CanvasWidget -- Using Canvas inside PUI dialogs
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef CANVASWIDGET_HXX_
#define CANVASWIDGET_HXX_

#include <Main/fg_props.hxx>
#include "FlightGear_pu.h"

#include <simgear/canvas/canvas_fwd.hxx>

class CanvasMgr;

class CanvasWidget:
  public puObject
{
  public:
    CanvasWidget( int x, int y,
                  int width, int height,
                  SGPropertyNode* props,
                  const std::string& module );
    virtual ~CanvasWidget();

    virtual void doHit (int button, int updown, int x, int y);
    virtual int checkKey(int key, int updown);

    virtual void setSize ( int w, int h );
    virtual void draw(int dx, int dy);

  private:

    CanvasMgr  *_canvas_mgr; // TODO maybe we should store this in some central
                             // location or make it static...

    simgear::canvas::CanvasPtr _canvas;

    float _last_x,
          _last_y;
    bool  _auto_viewport; //!< Set true to get the canvas view dimensions
                          //   automatically resized if the size of the widget
                          //   changes.

    SGPropertyNode_ptr _time,
                              _view_height;
};

#endif /* CANVASWIDGET_HXX_ */
