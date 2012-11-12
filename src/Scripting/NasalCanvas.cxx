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

#include <memory>
#include <string.h>

#include "NasalCanvas.hxx"
#include <Canvas/canvas_mgr.hxx>
#include <Main/globals.hxx>

//#include <boost/python.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/make_shared.hpp>
#include <osgGA/GUIEventAdapter>

#include <simgear/sg_inlines.h>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/elements/CanvasElement.hxx>

#include <simgear/nasal/cppbind/from_nasal.hxx>
#include <simgear/nasal/cppbind/to_nasal.hxx>
#include <simgear/nasal/cppbind/NasalHash.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>

extern naRef propNodeGhostCreate(naContext c, SGPropertyNode* n);

//void initCanvasPython()
//{
//  using namespace boost::python;
//  class_<simgear::canvas::Canvas>("Canvas");
//}

namespace sc = simgear::canvas;

naRef canvasGetNode(naContext c, sc::Canvas* canvas)
{
  return propNodeGhostCreate(c, canvas->getProps());
}

struct Base
{
  int getInt() const
  {
    return 8;
  }
};

struct Test:
  public Base
{
  Test(): x(1) {}
  int x;
  void setX(int x_) { x = x_; }
  naRef test(int argc, naRef* args)
  {
    return naNil();
  }
};

typedef nasal::Ghost<sc::CanvasPtr> NasalCanvas;

void initCanvas(naContext c)
{

  NasalCanvas::init("Canvas")
    .member("_node_ghost", &canvasGetNode)
    .member("size_x", &sc::Canvas::getSizeX)
    .member("size_y", &sc::Canvas::getSizeY);
  nasal::Ghost<sc::ElementPtr>::init("canvas.Element");
  nasal::Ghost<sc::GroupPtr>::init("canvas.Group")
    .bases<sc::ElementPtr>();

  nasal::Ghost<Base>::init("BaseClass")
    .member("int", &Base::getInt);
  nasal::Ghost<Test>::init("TestClass")
    .bases<Base>()
    .member("x", &Test::setX)
    .method<&Test::test>("test");
}

#if 0
/**
 * Class for exposing C++ objects to Nasal
 */
template<class T, class Derived>
class NasalObject
{
  public:
    // TODO use variadic template when supporting C++11
    template<class A1>
    static naRef create( naContext c, const A1& a1 )
    {
      return makeGhost(c, new T(a1));
    }

    template<class A1, class A2>
    static naRef create( naContext c, const A1& a1,
                                      const A2& a2 )
    {
      return makeGhost(c, new T(a1, a2));
    }

    template<class A1, class A2, class A3>
    static naRef create( naContext c, const A1& a1,
                                      const A2& a2,
                                      const A3& a3 )
    {
      return makeGhost(c, new T(a1, a2, a3));
    }

    template<class A1, class A2, class A3, class A4>
    static naRef create( naContext c, const A1& a1,
                                      const A2& a2,
                                      const A3& a3,
                                      const A4& a4 )
    {
      return makeGhost(c, new T(a1, a2, a3, a4));
    }

    template<class A1, class A2, class A3, class A4, class A5>
    static naRef create( naContext c, const A1& a1,
                                      const A2& a2,
                                      const A3& a3,
                                      const A4& a4,
                                      const A5& a5 )
    {
      return makeGhost(c, new T(a1, a2, a3, a4, a5));
    }

    // TODO If you need more arguments just do some copy&paste :)

    static Derived& getInstance()
    {
      static Derived instance;
      return instance;
    }

    void setParent(const naRef& parent)
    {
      // TODO check if we need to take care of reference counting/gc
      _parents.resize(1);
      _parents[0] = parent;
    }

  protected:

    // TODO switch to boost::/std::function (with C++11 lambdas this can make
    //      adding setters easier and shorter)
    typedef naRef (Derived::*getter_t)(naContext, const T&);
    typedef std::map<std::string, getter_t> MemberMap;

    const std::string   _ghost_name;
    std::vector<naRef>  _parents;
    MemberMap           _members;

    NasalObject(const std::string& ghost_name):
      _ghost_name( ghost_name )
    {
      _ghost_type.destroy = &destroyGhost;
      _ghost_type.name = _ghost_name.c_str();
      _ghost_type.get_member = &Derived::getMember;
      _ghost_type.set_member = 0;

      _members["parents"] = &NasalObject::getParents;
    }

    naRef getParents(naContext c, const T&)
    {
      naRef parents = naNewVector(c);
      for(size_t i = 0; i < _parents.size(); ++i)
        naVec_append(parents, _parents[i]);
      return parents;
    }

    static naRef makeGhost(naContext c, void *ptr)
    {
      std::cout << "create  " << ptr << std::endl;
      return naNewGhost2(c, &(getInstance()._ghost_type), ptr);
    }

    static void destroyGhost(void *ptr)
    {
      std::cout << "destroy " << ptr << std::endl;
      delete (T*)ptr;
    }

    static const char* getMember(naContext c, void* g, naRef field, naRef* out)
    {
      typename MemberMap::iterator getter =
        getInstance()._members.find(naStr_data(field));

      if( getter == getInstance()._members.end() )
        return 0;

      *out = (getInstance().*getter->second)(c, *static_cast<T*>(g));
      return "";
    }

  private:

    naGhostType _ghost_type;

};

typedef osg::ref_ptr<osgGA::GUIEventAdapter> GUIEventPtr;

class NasalCanvasEvent:
  public NasalObject<GUIEventPtr, NasalCanvasEvent>
{
  public:

    NasalCanvasEvent():
      NasalObject("CanvasEvent")
    {
      _members["type"] = &NasalCanvasEvent::getEventType;
    }

    naRef getEventType(naContext c, const GUIEventPtr& event)
    {
#define RET_EVENT_STR(type, str)\
  case osgGA::GUIEventAdapter::type:\
    return nasal::to_nasal(c, str);

      switch( event->getEventType() )
      {
        RET_EVENT_STR(PUSH,         "push");
        RET_EVENT_STR(RELEASE,      "release");
        RET_EVENT_STR(DOUBLECLICK,  "double-click");
        RET_EVENT_STR(DRAG,         "drag");
        RET_EVENT_STR(MOVE,         "move");
        RET_EVENT_STR(SCROLL,       "scroll");
        RET_EVENT_STR(KEYUP,        "key-up");
        RET_EVENT_STR(KEYDOWN,      "key-down");

#undef RET_EVENT_STR

        default:
          return naNil();
      }
    }
};
#endif
#if 0
static const char* eventGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  osgGA::GUIEventAdapter* gea = (osgGA::GUIEventAdapter*) g;

  if (!strcmp(fieldName, "windowX")) *out = naNum(gea->getWindowX());
  else if (!strcmp(fieldName, "windowY")) *out = naNum(gea->getWindowY());
  else if (!strcmp(fieldName, "time")) *out = naNum(gea->getTime());
  else if (!strcmp(fieldName, "button")) *out = naNum(gea->getButton());
  else {
    return 0;
  }

  return "";
}

static naRef f_element_addButtonCallback(naContext c, naRef me, int argc, naRef* args)
{
  simgear::canvas::Element* e = elementGhost(me);
  if (!e) {
    naRuntimeError(c, "element.addButtonCallback called on non-canvas-element object");
  }
  
  return naNil();
}

static naRef f_element_addDragCallback(naContext c, naRef me, int argc, naRef* args)
{
  simgear::canvas::Element* e = elementGhost(me);
  if (!e) {
    naRuntimeError(c, "element.addDragCallback called on non-canvas-element object");
  }
  
  return naNil();
}

static naRef f_element_addMoveCallback(naContext c, naRef me, int argc, naRef* args)
{
  simgear::canvas::Element* e = elementGhost(me);
  if (!e) {
    naRuntimeError(c, "element.addMoveCallback called on non-canvas-element object");
  }
  
  return naNil();
}

static naRef f_element_addScrollCallback(naContext c, naRef me, int argc, naRef* args)
{
  simgear::canvas::Element* e = elementGhost(me);
  if (!e) {
    naRuntimeError(c, "element.addScrollCallback called on non-canvas-element object");
  }
  
  return naNil();
}
#endif

static naRef f_createCanvas(naContext c, naRef me, int argc, naRef* args)
{
  std::cout << "f_createCanvas" << std::endl;

  CanvasMgr* canvas_mgr =
    static_cast<CanvasMgr*>(globals->get_subsystem("Canvas"));
  if( !canvas_mgr )
    return naNil();

  return NasalCanvas::create(c, canvas_mgr->createCanvas());
}

naRef initNasalCanvas(naRef globals, naContext c, naRef gcSave)
{
      /*naNewHash(c);
    hashset(c, gcSave, "canvasProto", canvasPrototype);
  
    hashset(c, canvasPrototype, "getElement", naNewFunc(c, naNewCCode(c, f_canvas_getElement)));*/
    // set any event methods
  
#if 0
    elementPrototype = naNewHash(c);
    hashset(c, gcSave, "elementProto", elementPrototype);
    
    hashset(c, elementPrototype, "addButtonCallback", naNewFunc(c, naNewCCode(c, f_element_addButtonCallback)));
    hashset(c, elementPrototype, "addDragCallback", naNewFunc(c, naNewCCode(c, f_element_addDragCallback)));
    hashset(c, elementPrototype, "addMoveCallback", naNewFunc(c, naNewCCode(c, f_element_addMoveCallback)));
    hashset(c, elementPrototype, "addScrollCallback", naNewFunc(c, naNewCCode(c, f_element_addScrollCallback)));
#endif
  nasal::Hash globals_module(globals, c),
              canvas_module = globals_module.createHash("canvas");

  canvas_module.set("_new", f_createCanvas);
  canvas_module.set("testClass", nasal::Ghost<Test>::f_create);

  initCanvas(c);

  return naNil();
}
