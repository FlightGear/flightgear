// Airports forward declarations
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "CanvasWidget.hxx"

#include <Canvas/canvas_mgr.hxx>
#include <Main/fg_os.hxx>      // fgGetKeyModifiers()
#include <Scripting/NasalSys.hxx>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/events/MouseEvent.hxx>

//------------------------------------------------------------------------------
CanvasWidget::CanvasWidget( int x, int y,
                            int width, int height,
                            SGPropertyNode* props,
                            const std::string& module ):
  puObject(x, y, width, height),
  _canvas_mgr( dynamic_cast<CanvasMgr*>(globals->get_subsystem("Canvas")) ),
  _last_x(0),
  _last_y(0),
  // automatically resize viewport of canvas if no size is given
  _auto_viewport( !props->hasChild("view") )
{
  if( !_canvas_mgr )
  {
    SG_LOG(SG_GENERAL, SG_ALERT, "CanvasWidget: failed to get canvas manager!");
    return;
  }

  _canvas = _canvas_mgr->createCanvas
  (
    props->getStringValue("name", "gui-anonymous")
  );

  int view[2] = {
    // Get canvas viewport size. If not specified use the widget dimensions
    props->getIntValue("view[0]", width),
    props->getIntValue("view[1]", height)
  };

  SGPropertyNode* cprops = _canvas->getProps();
  cprops->setIntValue("size[0]", view[0] * 2); // use higher resolution
  cprops->setIntValue("size[1]", view[1] * 2); // for antialias
  cprops->setIntValue("view[0]", view[0]);
  cprops->setIntValue("view[1]", view[1]);
  cprops->setBoolValue("render-always", true);
  cprops->setStringValue( "name",
                           props->getStringValue("name", "gui-anonymous") );

  SGPropertyNode *nasal = props->getNode("nasal");
  if( !nasal )
    return;

  FGNasalSys *nas = dynamic_cast<FGNasalSys*>(globals->get_subsystem("nasal"));
  if( !nas )
    SG_LOG( SG_GENERAL,
            SG_ALERT,
            "CanvasWidget: Failed to get nasal subsystem!" );

  const std::string file = std::string("__canvas:")
                         + cprops->getStringValue("name");

  SGPropertyNode *load = nasal->getNode("load");
  if( load )
  {
    const char *s = load->getStringValue();
    nas->handleCommand(module.c_str(), file.c_str(), s, cprops);
  }
}

//------------------------------------------------------------------------------
CanvasWidget::~CanvasWidget()
{
  if( _canvas )
    _canvas->destroy();
}

// Old versions of PUI are missing this defines...
#ifndef PU_SCROLL_UP_BUTTON
# define PU_SCROLL_UP_BUTTON     3
#endif
#ifndef PU_SCROLL_DOWN_BUTTON
# define PU_SCROLL_DOWN_BUTTON   4
#endif

//------------------------------------------------------------------------------
void CanvasWidget::doHit(int button, int updown, int x, int y)
{
  puObject::doHit(button, updown, x, y);

  // CTRL allows resizing and SHIFT allows moving the window
  if( fgGetKeyModifiers() & (KEYMOD_CTRL | KEYMOD_SHIFT) )
    return;

  namespace sc = simgear::canvas;
  sc::MouseEventPtr event(new sc::MouseEvent);

  if( !_time )
    _time = globals->get_props()->getNode("/sim/time/elapsed-sec");
  event->time = _time->getDoubleValue();

  if( !_view_height )
    _view_height = globals->get_props()->getNode("/sim/gui/canvas/size[1]");
  event->screen_pos.set(x, _view_height->getIntValue() - y);

  event->client_pos.set(x - abox.min[0], abox.max[1] - y);
  event->delta.set( event->getScreenX() - _last_x,
                    event->getScreenY() - _last_y );

  _last_x = event->getScreenX();
  _last_y = event->getScreenY();

  switch( button )
  {
    case PU_LEFT_BUTTON:
    case PU_MIDDLE_BUTTON:
    case PU_RIGHT_BUTTON:
      event->button = button;
      break;
    case PU_SCROLL_UP_BUTTON:
    case PU_SCROLL_DOWN_BUTTON:
      // Only let PU_DOWN trigger a scroll wheel event
      if( updown != PU_DOWN )
        return;

      event->type = sc::Event::WHEEL;
      event->delta.y() = button == PU_SCROLL_UP_BUTTON ? 1 : -1;

      _canvas->handleMouseEvent(event);

      return;
    default:
      SG_LOG(SG_INPUT, SG_WARN, "CanvasWidget: Unknown button: " << button);
      return;
  }

  switch( updown )
  {
    case PU_DOWN:
      event->type = sc::Event::MOUSE_DOWN;
      puSetActiveWidget(this, x, y);
      break;
    case PU_UP:
      event->type = sc::Event::MOUSE_UP;
      puDeactivateWidget();
      break;
    case PU_DRAG:
      event->type = sc::Event::DRAG;
      break;
    default:
      SG_LOG(SG_INPUT, SG_WARN, "CanvasWidget: Unknown updown: " << updown);
      return;
  }

  _canvas->handleMouseEvent(event);
}

//------------------------------------------------------------------------------
int CanvasWidget::checkKey(int key, int updown)
{
  return puObject::checkKey(key, updown);
}

//------------------------------------------------------------------------------
void CanvasWidget::setSize(int w, int h)
{
  puObject::setSize(w, h);

  if( _auto_viewport )
  {
    _canvas->getProps()->setIntValue("view[0]", w);
    _canvas->getProps()->setIntValue("view[1]", h);
  }
}

//------------------------------------------------------------------------------
void CanvasWidget::draw(int dx, int dy)
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, _canvas_mgr->getCanvasTexId(_canvas));
  glBegin( GL_QUADS );
    glColor3f(1,1,1);
    glTexCoord2f(0,0); glVertex2f(dx + abox.min[0], dy + abox.min[1]);
    glTexCoord2f(1,0); glVertex2f(dx + abox.max[0], dy + abox.min[1]);
    glTexCoord2f(1,1); glVertex2f(dx + abox.max[0], dy + abox.max[1]);
    glTexCoord2f(0,1); glVertex2f(dx + abox.min[0], dy + abox.max[1]);
  glEnd();
  glDisable(GL_TEXTURE_2D);
}
