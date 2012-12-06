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

    GLuint              _tex_id;    //<! OpenGL texture id if canvas
    size_t              _no_tex_cnt;//<! Count since how many frames we were not
                                    //   able to get the texture (for debugging)
    simgear::canvas::CanvasPtr _canvas;
    SGPropertyNode     *_mouse_x,
                       *_mouse_y,
                       *_mouse_down,
                       *_mouse_drag;

    float _last_x,
          _last_y;

    static SGPropertyNode_ptr _time;
};

#endif /* CANVASWIDGET_HXX_ */
