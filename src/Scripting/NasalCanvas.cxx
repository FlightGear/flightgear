// NasalCanvas.cxx -- expose Canvas classes to Nasal
//
// Written by James Turner, started 2012.
//
// Copyright (C) 2012 James Turner
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
#  include "config.h"
#endif

#include "NasalCanvas.hxx"
#include <Canvas/canvas_mgr.hxx>
#include <Canvas/gui_mgr.hxx>
#include <Canvas/window.hxx>
#include <Main/globals.hxx>
#include <Scripting/NasalSys.hxx>

#include <osgGA/GUIEventAdapter>

#include <simgear/sg_inlines.h>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/elements/CanvasElement.hxx>
#include <simgear/canvas/elements/CanvasText.hxx>
#include <simgear/canvas/MouseEvent.hxx>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

extern naRef propNodeGhostCreate(naContext c, SGPropertyNode* n);

namespace sc = simgear::canvas;

template<class Element>
naRef elementGetNode(naContext c, Element& element)
{
  return propNodeGhostCreate(c, element.getProps());
}

typedef nasal::Ghost<sc::EventPtr> NasalEvent;
typedef nasal::Ghost<sc::MouseEventPtr> NasalMouseEvent;
typedef nasal::Ghost<sc::CanvasPtr> NasalCanvas;
typedef nasal::Ghost<sc::ElementPtr> NasalElement;
typedef nasal::Ghost<sc::GroupPtr> NasalGroup;
typedef nasal::Ghost<sc::TextPtr> NasalText;
typedef nasal::Ghost<canvas::WindowWeakPtr> NasalWindow;

SGPropertyNode* from_nasal_helper(naContext c, naRef ref, SGPropertyNode**)
{
  SGPropertyNode* props = ghostToPropNode(ref);
  if( !props )
    naRuntimeError(c, "Not a SGPropertyNode ghost.");

  return props;
}

CanvasMgr& requireCanvasMgr(naContext c)
{
  CanvasMgr* canvas_mgr =
    static_cast<CanvasMgr*>(globals->get_subsystem("Canvas"));
  if( !canvas_mgr )
    naRuntimeError(c, "Failed to get Canvas subsystem");

  return *canvas_mgr;
}

GUIMgr& requireGUIMgr(naContext c)
{
  GUIMgr* mgr =
    static_cast<GUIMgr*>(globals->get_subsystem("CanvasGUI"));
  if( !mgr )
    naRuntimeError(c, "Failed to get CanvasGUI subsystem");

  return *mgr;
}

/**
 * Create new Canvas and get ghost for it.
 */
static naRef f_createCanvas(const nasal::CallContext& ctx)
{
  return NasalCanvas::create(ctx.c, requireCanvasMgr(ctx.c).createCanvas());
}

/**
 * Create new Window and get ghost for it.
 */
static naRef f_createWindow(const nasal::CallContext& ctx)
{
  return NasalWindow::create
  (
    ctx.c,
    requireGUIMgr(ctx.c).createWindow( ctx.getArg<std::string>(0) )
  );
}

/**
 * Get ghost for existing Canvas.
 */
static naRef f_getCanvas(naContext c, naRef me, int argc, naRef* args)
{
  nasal::CallContext ctx(c, argc, args);
  SGPropertyNode& props = *ctx.requireArg<SGPropertyNode*>(0);
  CanvasMgr& canvas_mgr = requireCanvasMgr(c);

  sc::CanvasPtr canvas;
  if( canvas_mgr.getPropertyRoot() == props.getParent() )
  {
    // get a canvas specified by its root node
    canvas = canvas_mgr.getCanvas( props.getIndex() );
    if( !canvas || canvas->getProps() != &props )
      return naNil();
  }
  else
  {
    // get a canvas by name
    if( props.hasValue("name") )
      canvas = canvas_mgr.getCanvas( props.getStringValue("name") );
    else if( props.hasValue("index") )
      canvas = canvas_mgr.getCanvas( props.getIntValue("index") );
  }

  return NasalCanvas::create(c, canvas);
}

naRef f_canvasCreateGroup(sc::Canvas& canvas, const nasal::CallContext& ctx)
{
  return NasalGroup::create
  (
    ctx.c,
    canvas.createGroup( ctx.getArg<std::string>(0) )
  );
}

/**
 * Get group containing all gui windows
 */
naRef f_getDesktop(naContext c, naRef me, int argc, naRef* args)
{
  return NasalGroup::create(c, requireGUIMgr(c).getDesktop());
}

naRef f_elementGetTransformedBounds(sc::Element& el, const nasal::CallContext& ctx)
{
  osg::BoundingBox bb = el.getTransformedBounds( osg::Matrix::identity() );

  std::vector<float> bb_vec(4);
  bb_vec[0] = bb._min.x();
  bb_vec[1] = bb._min.y();
  bb_vec[2] = bb._max.x();
  bb_vec[3] = bb._max.y();

  return nasal::to_nasal(ctx.c, bb_vec);
}

naRef f_groupCreateChild(sc::Group& group, const nasal::CallContext& ctx)
{
  return NasalElement::create
  (
    ctx.c,
    group.createChild( ctx.requireArg<std::string>(0),
                       ctx.getArg<std::string>(1) )
  );
}

naRef f_groupGetChild(sc::Group& group, const nasal::CallContext& ctx)
{
  return NasalElement::create
  (
    ctx.c,
    group.getChild( ctx.requireArg<SGPropertyNode*>(0) )
  );
}

naRef f_groupGetElementById(sc::Group& group, const nasal::CallContext& ctx)
{
  return NasalElement::create
  (
    ctx.c,
    group.getElementById( ctx.requireArg<std::string>(0) )
  );
}

naRef to_nasal_helper(naContext c, const sc::ElementWeakPtr& el)
{
  return NasalElement::create(c, el.lock());
}

naRef initNasalCanvas(naRef globals, naContext c, naRef gcSave)
{
  NasalEvent::init("canvas.Event")
    .member("type", &sc::Event::getTypeString)
    .member("target", &sc::Event::getTarget)
    .member("currentTarget", &sc::Event::getCurrentTarget)
    .method("stopPropagation", &sc::Event::stopPropagation);
  NasalMouseEvent::init("canvas.MouseEvent")
    .bases<NasalEvent>()
    .member("screenX", &sc::MouseEvent::getScreenX)
    .member("screenY", &sc::MouseEvent::getScreenY)
    .member("clientX", &sc::MouseEvent::getClientX)
    .member("clientY", &sc::MouseEvent::getClientY)
    .member("localX", &sc::MouseEvent::getLocalX)
    .member("localY", &sc::MouseEvent::getLocalY)
    .member("deltaX", &sc::MouseEvent::getDeltaX)
    .member("deltaY", &sc::MouseEvent::getDeltaY)
    .member("click_count", &sc::MouseEvent::getCurrentClickCount);
  NasalCanvas::init("Canvas")
    .member("_node_ghost", &elementGetNode<sc::Canvas>)
    .member("size_x", &sc::Canvas::getSizeX)
    .member("size_y", &sc::Canvas::getSizeY)
    .method("_createGroup", &f_canvasCreateGroup)
    .method("_getGroup", &sc::Canvas::getGroup)
    .method("addEventListener", &sc::Canvas::addEventListener);
  NasalElement::init("canvas.Element")
    .member("_node_ghost", &elementGetNode<sc::Element>)
    .method("addEventListener", &sc::Element::addEventListener)
    .method("getTransformedBounds", &f_elementGetTransformedBounds);
  NasalGroup::init("canvas.Group")
    .bases<NasalElement>()
    .method("_createChild", &f_groupCreateChild)
    .method("_getChild", &f_groupGetChild)
    .method("_getElementById", &f_groupGetElementById);
  NasalText::init("canvas.Text")
    .bases<NasalElement>()
    .method("getNearestCursor", &sc::Text::getNearestCursor);

  NasalWindow::init("canvas.Window")
    .bases<NasalElement>()
    .member("_node_ghost", &elementGetNode<canvas::Window>)
    .method("_getCanvasDecoration", &canvas::Window::getCanvasDecoration);

  nasal::Hash globals_module(globals, c),
              canvas_module = globals_module.createHash("canvas");

  canvas_module.set("_newCanvasGhost", f_createCanvas);
  canvas_module.set("_newWindowGhost", f_createWindow);
  canvas_module.set("_getCanvasGhost", f_getCanvas);
  canvas_module.set("_getDesktopGhost", f_getDesktop);

  return naNil();
}
