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
#include <Main/globals.hxx>
#include <Scripting/NasalSys.hxx>

#include <osgGA/GUIEventAdapter>

#include <simgear/sg_inlines.h>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/CanvasWindow.hxx>
#include <simgear/canvas/elements/CanvasElement.hxx>
#include <simgear/canvas/elements/CanvasText.hxx>
#include <simgear/canvas/layout/BoxLayout.hxx>
#include <simgear/canvas/layout/NasalWidget.hxx>
#include <simgear/canvas/events/CustomEvent.hxx>
#include <simgear/canvas/events/KeyboardEvent.hxx>
#include <simgear/canvas/events/MouseEvent.hxx>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

extern naRef propNodeGhostCreate(naContext c, SGPropertyNode* n);

namespace sc = simgear::canvas;

template<class Element>
naRef elementGetNode(Element& element, naContext c)
{
  return propNodeGhostCreate(c, element.getProps());
}

typedef nasal::Ghost<sc::EventPtr> NasalEvent;
typedef nasal::Ghost<sc::CustomEventPtr> NasalCustomEvent;
typedef nasal::Ghost<sc::DeviceEventPtr> NasalDeviceEvent;
typedef nasal::Ghost<sc::KeyboardEventPtr> NasalKeyboardEvent;
typedef nasal::Ghost<sc::MouseEventPtr> NasalMouseEvent;

struct CustomEventDetailWrapper;
typedef SGSharedPtr<CustomEventDetailWrapper> CustomEventDetailPtr;
typedef nasal::Ghost<CustomEventDetailPtr> NasalCustomEventDetail;

typedef nasal::Ghost<simgear::PropertyBasedElementPtr> NasalPropertyBasedElement;
typedef nasal::Ghost<sc::CanvasPtr> NasalCanvas;
typedef nasal::Ghost<sc::ElementPtr> NasalElement;
typedef nasal::Ghost<sc::GroupPtr> NasalGroup;
typedef nasal::Ghost<sc::TextPtr> NasalText;

typedef nasal::Ghost<sc::LayoutItemRef> NasalLayoutItem;
typedef nasal::Ghost<sc::LayoutRef> NasalLayout;
typedef nasal::Ghost<sc::BoxLayoutRef> NasalBoxLayout;

typedef nasal::Ghost<sc::WindowPtr> NasalWindow;

naRef to_nasal_helper(naContext c, const osg::BoundingBox& bb)
{
  std::vector<float> bb_vec(4);
  bb_vec[0] = bb._min.x();
  bb_vec[1] = bb._min.y();
  bb_vec[2] = bb._max.x();
  bb_vec[3] = bb._max.y();

  return nasal::to_nasal(c, bb_vec);
}

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
  return ctx.to_nasal(requireCanvasMgr(ctx.c).createCanvas());
}

/**
 * Create new Window and get ghost for it.
 */
static naRef f_createWindow(const nasal::CallContext& ctx)
{
  return nasal::to_nasal<sc::WindowWeakPtr>
  (
    ctx.c,
    requireGUIMgr(ctx.c).createWindow( ctx.getArg<std::string>(0) )
  );
}

/**
 * Get ghost for existing Canvas.
 */
static naRef f_getCanvas(const nasal::CallContext& ctx)
{
  SGPropertyNode& props = *ctx.requireArg<SGPropertyNode*>(0);
  CanvasMgr& canvas_mgr = requireCanvasMgr(ctx.c);

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

  return ctx.to_nasal(canvas);
}

naRef f_canvasCreateGroup(sc::Canvas& canvas, const nasal::CallContext& ctx)
{
  return ctx.to_nasal( canvas.createGroup(ctx.getArg<std::string>(0)) );
}

/**
 * Get group containing all gui windows
 */
naRef f_getDesktop(naContext c, naRef me, int argc, naRef* args)
{
  return nasal::to_nasal(c, requireGUIMgr(c).getDesktop());
}

naRef f_setInputFocus(const nasal::CallContext& ctx)
{
  requireGUIMgr(ctx.c).setInputFocus(ctx.requireArg<sc::WindowPtr>(0));
  return naNil();
}

naRef f_grabPointer(const nasal::CallContext& ctx)
{
  return ctx.to_nasal(
    requireGUIMgr(ctx.c).grabPointer(ctx.requireArg<sc::WindowPtr>(0))
  );
}

naRef f_ungrabPointer(const nasal::CallContext& ctx)
{
  requireGUIMgr(ctx.c).ungrabPointer(ctx.requireArg<sc::WindowPtr>(0));
  return naNil();
}

static naRef f_groupCreateChild(sc::Group& group, const nasal::CallContext& ctx)
{
  return ctx.to_nasal( group.createChild( ctx.requireArg<std::string>(0),
                                          ctx.getArg<std::string>(1) ) );
}

static sc::ElementPtr f_groupGetChild(sc::Group& group, SGPropertyNode* node)
{
  return group.getChild(node);
}

static void propElementSetData( simgear::PropertyBasedElement& el,
                                const std::string& name,
                                naContext c,
                                naRef ref )
{
  if( naIsNil(ref) )
    return el.removeDataProp(name);

  std::string val = nasal::from_nasal<std::string>(c, ref);

  char* end = NULL;

  long val_long = strtol(val.c_str(), &end, 10);
  if( !*end )
    return el.setDataProp(name, val_long);

  double val_double = strtod(val.c_str(), &end);
  if( !*end )
    return el.setDataProp(name, val_double);

  el.setDataProp(name, val);
}

/**
 * Accessor for HTML5 data properties.
 *
 * # set single property:
 * el.data("myKey", 5);
 *
 * # set multiple properties
 * el.data({myProp1: 12, myProp2: "test"});
 *
 * # get value of properties
 * el.data("myKey");   # 5
 * el.data("myProp2"); # "test"
 *
 * # remove a single property
 * el.data("myKey", nil);
 *
 * # remove multiple properties
 * el.data({myProp1: nil, myProp2: nil});
 *
 * # set and remove multiple properties
 * el.data({newProp: "some text...", removeProp: nil});
 *
 *
 * @see http://api.jquery.com/data/
 */
static naRef f_propElementData( simgear::PropertyBasedElement& el,
                                const nasal::CallContext& ctx )
{
  if( ctx.isHash(0) )
  {
    // Add/delete properties given as hash
    nasal::Hash obj = ctx.requireArg<nasal::Hash>(0);
    for(nasal::Hash::iterator it = obj.begin(); it != obj.end(); ++it)
      propElementSetData(el, it->getKey(), ctx.c, it->getValue<naRef>());

    return ctx.to_nasal(&el);
  }

  std::string name = ctx.getArg<std::string>(0);
  if( !name.empty() )
  {
    if( ctx.argc == 1 )
    {
      // name + additional argument -> add/delete property
      SGPropertyNode* node = el.getDataProp<SGPropertyNode*>(name);
      if( !node )
        return naNil();

      return ctx.to_nasal( node->getStringValue() );
    }
    else
    {
      // only name -> get property
      propElementSetData(el, name, ctx.c, ctx.requireArg<naRef>(1));
      return ctx.to_nasal(&el);
    }
  }

  return naNil();
}

static naRef f_createCustomEvent(const nasal::CallContext& ctx)
{
  std::string const& type = ctx.requireArg<std::string>(0);
  if( type.empty() )
    return naNil();

  bool bubbles = false;
  simgear::StringMap detail;
  if( ctx.isHash(1) )
  {
    nasal::Hash const& cfg = ctx.requireArg<nasal::Hash>(1);
    naRef na_detail = cfg.get("detail");
    if( naIsHash(na_detail) )
      detail = ctx.from_nasal<simgear::StringMap>(na_detail);
    bubbles = cfg.get<bool>("bubbles");
  }

  return ctx.to_nasal(
    sc::CustomEventPtr(new sc::CustomEvent(type, bubbles, detail))
  );
}

struct CustomEventDetailWrapper:
  public SGReferenced
{
  sc::CustomEventPtr _event;

  CustomEventDetailWrapper(const sc::CustomEventPtr& event):
    _event(event)
  {

  }

  bool _get( const std::string& key,
             std::string& value_out ) const
  {
    if( !_event )
      return false;

    simgear::StringMap::const_iterator it = _event->detail.find(key);
    if( it == _event->detail.end() )
      return false;

    value_out = it->second;
    return true;
  }

  bool _set( const std::string& key,
             const std::string& value )
  {
    if( !_event )
      return false;

    _event->detail[ key ] = value;
    return true;
  }
};

static naRef f_customEventGetDetail( sc::CustomEvent& event,
                                     naContext c )
{
  return nasal::to_nasal(
    c,
    CustomEventDetailPtr(new CustomEventDetailWrapper(&event))
  );
}

static naRef f_boxLayoutAddItem( sc::BoxLayout& box,
                                 const nasal::CallContext& ctx )
{
  box.addItem( ctx.requireArg<sc::LayoutItemRef>(0),
               ctx.getArg<int>(1),
               ctx.getArg<int>(2, sc::AlignFill) );
  return naNil();
}
static naRef f_boxLayoutInsertItem( sc::BoxLayout& box,
                                    const nasal::CallContext& ctx )
{
  box.insertItem( ctx.requireArg<int>(0),
                  ctx.requireArg<sc::LayoutItemRef>(1),
                  ctx.getArg<int>(2),
                  ctx.getArg<int>(3, sc::AlignFill) );
  return naNil();
}

static naRef f_boxLayoutAddStretch( sc::BoxLayout& box,
                                    const nasal::CallContext& ctx )
{
  box.addStretch( ctx.getArg<int>(0) );
  return naNil();
}
static naRef f_boxLayoutInsertStretch( sc::BoxLayout& box,
                                       const nasal::CallContext& ctx )
{
  box.insertStretch( ctx.requireArg<int>(0),
                     ctx.getArg<int>(1) );
  return naNil();
}

template<class Type, class Base>
static naRef f_newAsBase(const nasal::CallContext& ctx)
{
  return ctx.to_nasal<Base*>(new Type());
}

naRef initNasalCanvas(naRef globals, naContext c)
{
  nasal::Hash globals_module(globals, c),
              canvas_module = globals_module.createHash("canvas");

  nasal::Object::setupGhost();

  //----------------------------------------------------------------------------
  // Events

  using osgGA::GUIEventAdapter;
  NasalEvent::init("canvas.Event")
    .member("type", &sc::Event::getTypeString)
    .member("target", &sc::Event::getTarget)
    .member("currentTarget", &sc::Event::getCurrentTarget)
    .member("defaultPrevented", &sc::Event::defaultPrevented)
    .method("stopPropagation", &sc::Event::stopPropagation)
    .method("preventDefault", &sc::Event::preventDefault);

  NasalCustomEvent::init("canvas.CustomEvent")
    .bases<NasalEvent>()
    .member("detail", &f_customEventGetDetail, &sc::CustomEvent::setDetail);
  NasalCustomEventDetail::init("canvas.CustomEventDetail")
    ._get(&CustomEventDetailWrapper::_get)
    ._set(&CustomEventDetailWrapper::_set);

  canvas_module.createHash("CustomEvent")
               .set("new", &f_createCustomEvent);

  NasalDeviceEvent::init("canvas.DeviceEvent")
    .bases<NasalEvent>()
    .member("modifiers", &sc::DeviceEvent::getModifiers)
    .member("ctrlKey", &sc::DeviceEvent::ctrlKey)
    .member("shiftKey", &sc::DeviceEvent::shiftKey)
    .member("altKey",  &sc::DeviceEvent::altKey)
    .member("metaKey",  &sc::DeviceEvent::metaKey);

  NasalKeyboardEvent::init("canvas.KeyboardEvent")
    .bases<NasalDeviceEvent>()
    .member("key", &sc::KeyboardEvent::key)
    .member("location", &sc::KeyboardEvent::location)
    .member("repeat", &sc::KeyboardEvent::repeat)
    .member("charCode", &sc::KeyboardEvent::charCode)
    .member("keyCode", &sc::KeyboardEvent::keyCode);

  NasalMouseEvent::init("canvas.MouseEvent")
    .bases<NasalDeviceEvent>()
    .member("screenX", &sc::MouseEvent::getScreenX)
    .member("screenY", &sc::MouseEvent::getScreenY)
    .member("clientX", &sc::MouseEvent::getClientX)
    .member("clientY", &sc::MouseEvent::getClientY)
    .member("localX", &sc::MouseEvent::getLocalX)
    .member("localY", &sc::MouseEvent::getLocalY)
    .member("deltaX", &sc::MouseEvent::getDeltaX)
    .member("deltaY", &sc::MouseEvent::getDeltaY)
    .member("button", &sc::MouseEvent::getButton)
    .member("buttons", &sc::MouseEvent::getButtonMask)
    .member("click_count", &sc::MouseEvent::getCurrentClickCount);

  //----------------------------------------------------------------------------
  // Canvas & elements

  NasalPropertyBasedElement::init("PropertyBasedElement")
    .method("data", &f_propElementData);
  NasalCanvas::init("Canvas")
    .bases<NasalPropertyBasedElement>()
    .bases<nasal::ObjectRef>()
    .member("_node_ghost", &elementGetNode<sc::Canvas>)
    .member("size_x", &sc::Canvas::getSizeX)
    .member("size_y", &sc::Canvas::getSizeY)
    .method("_createGroup", &f_canvasCreateGroup)
    .method("_getGroup", &sc::Canvas::getGroup)
    .method( "addEventListener",
             static_cast<bool (sc::Canvas::*)( const std::string&,
                                               const sc::EventListener& )>
             (&sc::Canvas::addEventListener) )
    .method("dispatchEvent", &sc::Canvas::dispatchEvent)
    .method("setLayout", &sc::Canvas::setLayout)
    .method("setFocusElement", &sc::Canvas::setFocusElement)
    .method("clearFocusElement", &sc::Canvas::clearFocusElement);

  canvas_module.set("_newCanvasGhost", f_createCanvas);
  canvas_module.set("_getCanvasGhost", f_getCanvas);

  NasalElement::init("canvas.Element")
    .bases<NasalPropertyBasedElement>()
    .member("_node_ghost", &elementGetNode<sc::Element>)
    .method("_getParent", &sc::Element::getParent)
    .method("_getCanvas", &sc::Element::getCanvas)
    .method("addEventListener", &sc::Element::addEventListener)
    .method("setFocus", &sc::Element::setFocus)
    .method("dispatchEvent", &sc::Element::dispatchEvent)
    .method("getBoundingBox", &sc::Element::getBoundingBox)
    .method("getTightBoundingBox", &sc::Element::getTightBoundingBox);

  NasalGroup::init("canvas.Group")
    .bases<NasalElement>()
    .method("_createChild", &f_groupCreateChild)
    .method( "_getChild", &f_groupGetChild)
    .method("_getElementById", &sc::Group::getElementById);
  NasalText::init("canvas.Text")
    .bases<NasalElement>()
    .method("heightForWidth", &sc::Text::heightForWidth)
    .method("maxWidth", &sc::Text::maxWidth)
    .method("lineCount", &sc::Text::lineCount)
    .method("lineLength", &sc::Text::lineLength)
    .method("getNearestCursor", &sc::Text::getNearestCursor)
    .method("getCursorPos", &sc::Text::getCursorPos);

  //----------------------------------------------------------------------------
  // Layouting

#define ALIGN_ENUM_MAPPING(key, val, comment) canvas_module.set(#key, sc::key);
#  include <simgear/canvas/layout/AlignFlag_values.hxx>
#undef ALIGN_ENUM_MAPPING

  void (sc::LayoutItem::*f_layoutItemSetContentsMargins)(int, int, int, int)
    = &sc::LayoutItem::setContentsMargins;

  NasalLayoutItem::init("canvas.LayoutItem")
    .method("getCanvas", &sc::LayoutItem::getCanvas)
    .method("setCanvas", &sc::LayoutItem::setCanvas)
    .method("getParent", &sc::LayoutItem::getParent)
    .method("setParent", &sc::LayoutItem::setParent)
    .method("setContentsMargins", f_layoutItemSetContentsMargins)
    .method("setContentsMargin", &sc::LayoutItem::setContentsMargin)
    .method("sizeHint", &sc::LayoutItem::sizeHint)
    .method("minimumSize", &sc::LayoutItem::minimumSize)
    .method("maximumSize", &sc::LayoutItem::maximumSize)
    .method("hasHeightForWidth", &sc::LayoutItem::hasHeightForWidth)
    .method("heightForWidth", &sc::LayoutItem::heightForWidth)
    .method("minimumHeightForWidth", &sc::LayoutItem::minimumHeightForWidth)
    .method("setAlignment", &sc::LayoutItem::setAlignment)
    .method("alignment", &sc::LayoutItem::alignment)
    .method("setVisible", &sc::LayoutItem::setVisible)
    .method("isVisible", &sc::LayoutItem::isVisible)
    .method("isExplicitlyHidden", &sc::LayoutItem::isExplicitlyHidden)
    .method("show", &sc::LayoutItem::show)
    .method("hide", &sc::LayoutItem::hide)
    .method("setGeometry", &sc::LayoutItem::setGeometry)
    .method("geometry", &sc::LayoutItem::geometry);
  sc::NasalWidget::setupGhost(canvas_module);

  NasalLayout::init("canvas.Layout")
    .bases<NasalLayoutItem>()
    .method("addItem", &sc::Layout::addItem)
    .method("setSpacing", &sc::Layout::setSpacing)
    .method("spacing", &sc::Layout::spacing)
    .method("count", &sc::Layout::count)
    .method("itemAt", &sc::Layout::itemAt)
    .method("takeAt", &sc::Layout::takeAt)
    .method("removeItem", &sc::Layout::removeItem)
    .method("clear", &sc::Layout::clear);

  NasalBoxLayout::init("canvas.BoxLayout")
    .bases<NasalLayout>()
    .method("addItem", &f_boxLayoutAddItem)
    .method("addSpacing", &sc::BoxLayout::addSpacing)
    .method("addStretch", &f_boxLayoutAddStretch)
    .method("insertItem", &f_boxLayoutInsertItem)
    .method("insertSpacing", &sc::BoxLayout::insertSpacing)
    .method("insertStretch", &f_boxLayoutInsertStretch)
    .method("setStretch", &sc::BoxLayout::setStretch)
    .method("setStretchFactor", &sc::BoxLayout::setStretchFactor)
    .method("stretch", &sc::BoxLayout::stretch);

  canvas_module.createHash("HBoxLayout")
               .set("new", &f_newAsBase<sc::HBoxLayout, sc::BoxLayout>);
  canvas_module.createHash("VBoxLayout")
               .set("new", &f_newAsBase<sc::VBoxLayout, sc::BoxLayout>);

  //----------------------------------------------------------------------------
  // Window

  NasalWindow::init("canvas.Window")
    .bases<NasalElement>()
    .bases<NasalLayoutItem>()
    .member("_node_ghost", &elementGetNode<sc::Window>)
    .method("_getCanvasDecoration", &sc::Window::getCanvasDecoration)
    .method("setLayout", &sc::Window::setLayout);

  canvas_module.set("_newWindowGhost", f_createWindow);
  canvas_module.set("_getDesktopGhost", f_getDesktop);
  canvas_module.set("setInputFocus", f_setInputFocus);
  canvas_module.set("grabPointer", f_grabPointer);
  canvas_module.set("ungrabPointer", f_ungrabPointer);

  return naNil();
}
