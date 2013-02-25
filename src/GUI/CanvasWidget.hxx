/*
 * CanvasWidget.hxx
 *
 *  Created on: 03.07.2012
 *      Author: tom
 */

#ifndef CANVASWIDGET_HXX_
#define CANVASWIDGET_HXX_

#include <Main/fg_props.hxx>
#include <plib/pu.h>

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

    static SGPropertyNode_ptr _time,
                              _view_height;
};

#endif /* CANVASWIDGET_HXX_ */
