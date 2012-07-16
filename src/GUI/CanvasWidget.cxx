/*
 * CanvasWidget.cxx
 *
 *  Created on: 03.07.2012
 *      Author: tom
 */

#include "CanvasWidget.hxx"

#include <Canvas/canvas_mgr.hxx>
#include <Main/fg_os.hxx>      // fgGetKeyModifiers()
#include <Scripting/NasalSys.hxx>

//------------------------------------------------------------------------------
CanvasWidget::CanvasWidget( int x, int y,
                            int width, int height,
                            SGPropertyNode* props,
                            const std::string& module ):
  puObject(x, y, width, height),
  _canvas_mgr( dynamic_cast<CanvasMgr*>(globals->get_subsystem("Canvas")) ),
  _tex_id(0),
  _no_tex_cnt(0)
{
  if( !_canvas_mgr )
  {
    SG_LOG(SG_GENERAL, SG_ALERT, "CanvasWidget: failed to get canvas manager!");
    return;
  }

  // Get the first unused canvas slot
  SGPropertyNode* canvas_root = fgGetNode("/canvas", true);
  for(int index = 0;; ++index)
  {
    if( !canvas_root->getChild("texture", index) )
    {
      int view[2] = {
        // Get canvas viewport size. If not specified use the widget dimensions
        props->getIntValue("view[0]", width),
        props->getIntValue("view[1]", height)
      };
      _canvas = canvas_root->getChild("texture", index, true);
      _canvas->setIntValue("size[0]", view[0] * 2); // use higher resolution
      _canvas->setIntValue("size[1]", view[1] * 2); // for antialias
      _canvas->setIntValue("view[0]", view[0]);
      _canvas->setIntValue("view[1]", view[1]);
      _canvas->setBoolValue("render-always", true);
      _canvas->setStringValue( "name",
                               props->getStringValue("name", "gui-anonymous") );
      SGPropertyNode* input = _canvas->getChild("input", 0, true);
      _mouse_x = input->getChild("mouse-x", 0, true);
      _mouse_y = input->getChild("mouse-y", 0, true);
      _mouse_down = input->getChild("mouse-down", 0, true);
      _mouse_drag = input->getChild("mouse-drag", 0, true);

      SGPropertyNode *nasal = props->getNode("nasal");
      if( !nasal )
        break;

      FGNasalSys *nas =
        dynamic_cast<FGNasalSys*>(globals->get_subsystem("nasal"));
      if( !nas )
        SG_LOG( SG_GENERAL,
                SG_ALERT,
                "CanvasWidget: Failed to get nasal subsystem!" );

      const std::string file = std::string("__canvas:")
                             + _canvas->getStringValue("name");

      SGPropertyNode *load = nasal->getNode("load");
      if( load )
      {
        const char *s = load->getStringValue();
        nas->handleCommand(module.c_str(), file.c_str(), s, _canvas);
      }
      break;
    }
  }
}

//------------------------------------------------------------------------------
CanvasWidget::~CanvasWidget()
{
  if( _canvas )
    _canvas->getParent()
           ->removeChild(_canvas->getName(), _canvas->getIndex(), false);
}

//------------------------------------------------------------------------------
void CanvasWidget::doHit(int button, int updown, int x, int y)
{
  puObject::doHit(button, updown, x, y);

  // CTRL allows resizing and SHIFT allows moving the window
  if( fgGetKeyModifiers() & (KEYMOD_CTRL | KEYMOD_SHIFT) )
    return;

  _mouse_x->setIntValue(x - abox.min[0]);
  _mouse_y->setIntValue(abox.max[1] - y);

  if( updown == PU_DRAG )
    _mouse_drag->setIntValue(button);
  else if( updown == PU_DOWN )
    _mouse_down->setIntValue(button);

  if( button != active_mouse_button )
    return;

  if (updown == PU_UP)
    puDeactivateWidget();
  else if (updown == PU_DOWN)
    puSetActiveWidget(this, x, y);
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

  _canvas->setIntValue("view[0]", w);
  _canvas->setIntValue("view[1]", h);
}

//------------------------------------------------------------------------------
void CanvasWidget::draw(int dx, int dy)
{
  if( !_tex_id )
  {
    _tex_id = _canvas_mgr->getCanvasTexId(_canvas->getIndex());

    // Normally we should be able to get the texture after one frame. I don't
    // know if there are circumstances where it can take longer, so we don't
    // log a warning message until we have tried a few times.
    if( !_tex_id )
    {
      if( ++_no_tex_cnt == 5 )
        SG_LOG(SG_GENERAL, SG_WARN, "CanvasWidget: failed to get texture!");
      return;
    }
    else
    {
      if( _no_tex_cnt >= 5 )
        SG_LOG
        (
          SG_GENERAL,
          SG_INFO,
          "CanvasWidget: got texture after " << _no_tex_cnt << " tries."
        );
      _no_tex_cnt = 0;
    }
  }

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, _tex_id);
  glBegin( GL_QUADS );
    glColor3f(1,1,1);
    glTexCoord2f(0,0); glVertex2f(dx + abox.min[0], dy + abox.min[1]);
    glTexCoord2f(1,0); glVertex2f(dx + abox.max[0], dy + abox.min[1]);
    glTexCoord2f(1,1); glVertex2f(dx + abox.max[0], dy + abox.max[1]);
    glTexCoord2f(0,1); glVertex2f(dx + abox.min[0], dy + abox.max[1]);
  glEnd();
  glDisable(GL_TEXTURE_2D);
}
