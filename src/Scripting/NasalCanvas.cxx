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
#include <boost/function.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/make_shared.hpp>
#include <osgGA/GUIEventAdapter>

#include <simgear/sg_inlines.h>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/elements/CanvasElement.hxx>

static naRef elementPrototype;
static naRef propsNodePrototype;

extern naRef propNodeGhostCreate(naContext c, SGPropertyNode* n);

static void hashset(naContext c, naRef hash, const char* key, naRef val)
{
  naRef s = naNewString(c);
  naStr_fromdata(s, (char*)key, strlen(key));
  naHash_set(hash, s, val);
}

template<class T>
typename boost::enable_if<boost::is_arithmetic<T>, naRef>::type
convertToNasal(T val)
{
  return naNum(val);
}

//void initCanvasPython()
//{
//  using namespace boost::python;
//  class_<simgear::canvas::Canvas>("Canvas");
//}

namespace sc = simgear::canvas;

naRef canvasGetTexture(naContext c, sc::Canvas* canvas)
{
  //{ parents : [Node], _g : node }
  naRef parents = naNewVector(c);
  naVec_append(parents, propsNodePrototype);

  naRef node = naNewHash(c);
  hashset(c, node, "parents", parents);
  hashset(c, node, "_g", propNodeGhostCreate(c, canvas->getProps()));

  return node;
}

namespace nasal
{

  template<class T>
  struct class_traits
  {
    typedef boost::false_type::type is_shared;
    typedef T raw_type;
  };

  template<class T>
  struct class_traits<boost::shared_ptr<T> >
  {
    typedef boost::true_type::type is_shared;
    typedef T raw_type;
  };

  template<class T>
  struct class_traits<osg::ref_ptr<T> >
  {
    typedef boost::true_type::type is_shared;
    typedef T raw_type;
  };

  template<class T>
  struct class_traits<SGSharedPtr<T> >
  {
    typedef boost::true_type::type is_shared;
    typedef T raw_type;
  };

  template<class T>
  struct SharedPointerPolicy
  {
    typedef typename class_traits<T>::raw_type raw_type;

    /**
     * Create a shared pointer on the heap to handle the reference counting for
     * the passed shared pointer while it is used in Nasal space.
     */
    static T* createInstance(const T& ptr)
    {
      return new T(ptr);
    }

    static raw_type* getRawPtr(void* ptr)
    {
      return static_cast<T*>(ptr)->get();
    }
  };

  template<class T>
  struct RawPointerPolicy
  {
    typedef typename class_traits<T>::raw_type raw_type;

    static T* createInstance()
    {
      return new T();
    }

    static raw_type* getRawPtr(void* ptr)
    {
      BOOST_STATIC_ASSERT((boost::is_same<raw_type, T>::value));
      return static_cast<T*>(ptr);
    }
  };

  class class_metadata
  {
    public:
      void addNasalBase(const naRef& parent)
      {
        assert( naIsHash(parent) );
        _parents.push_back(parent);
      }

    protected:
      const std::string             _name;
      naGhostType                   _ghost_type;
      std::vector<class_metadata*>  _base_classes;
      std::vector<naRef>            _parents;

      explicit class_metadata(const std::string& name):
        _name(name)
      {

      }

      void addBaseClass(class_metadata* base)
      {
        assert(base);
        std::cout << _name << ": addBase(" << base->_name << ")" << std::endl;
        _base_classes.push_back(base);
      }

      naRef getParents(naContext c)
      {
        naRef parents = naNewVector(c);
        for(size_t i = 0; i < _parents.size(); ++i)
          naVec_append(parents, _parents[i]);
        return parents;
      }
  };

  /**
   * Class for exposing C++ objects to Nasal
   */
  template<class T>
  class class_:
    public class_metadata,
    protected boost::mpl::if_< typename class_traits<T>::is_shared,
                               SharedPointerPolicy<T>,
                               RawPointerPolicy<T> >::type
  {
    private:
      typedef typename class_traits<T>::raw_type raw_type;

    public:

      typedef boost::function<naRef(naContext c, raw_type*)> getter_t;
      typedef std::map<std::string, getter_t> GetterMap;

      static class_& init(const std::string& name)
      {
        getSingletonHolder().reset( new class_(name) );
        return *getSingletonPtr();
      }

      static naRef f_create(naContext c, naRef me, int argc, naRef* args)
      {
        return create(c);
      }

      static naRef create( naContext c )
      {
        return makeGhost(c, class_::createInstance());
      }

      // TODO use variadic template when supporting C++11
      template<class A1>
      static naRef create( naContext c, const A1& a1 )
      {
        return makeGhost(c, class_::createInstance(a1));
      }

      class_& bases(const naRef& parent)
      {
        addNasalBase(parent);
        return *this;
      }

      template<class Base>
      typename boost::enable_if
        < boost::is_convertible<Base, class_metadata>,
          class_
        >::type&
      bases()
      {
        Base* base = Base::getSingletonPtr();
        addBaseClass( base );

        // Replace any getter that is not available in the current class.
        // TODO check if this is the correct behavior of function overriding
        const typename Base::GetterMap& base_getter = base->getGetter();
        for( typename Base::GetterMap::const_iterator getter =
               base_getter.begin();
               getter != base_getter.end();
             ++getter )
        {
          if( _getter.find(getter->first) == _getter.end() )
            _getter.insert( *getter );
        }

        return *this;
      }

      template<class Type>
      typename boost::enable_if_c
        < !boost::is_convertible< Type, class_metadata>::value,
          class_
        >::type&
      bases()
      {
        return bases< class_<Type> >();
      }

      template<class Var>
      class_& def( const std::string& field,
                   Var (raw_type::*getter)() const )
      {
        _getter[field] = boost::bind( &convertToNasal<Var>,
                                      boost::bind(getter, _2) );
        return *this;
      }

      class_& def_readonly( const std::string& field,
                            getter_t getter )
      {
        _getter[field] = getter;
        return *this;
      }

      /**
       * Data member
       */
      template<class Var>
      class_& def_readonly( const std::string& field,
                            Var raw_type::*var )
      {
        _getter[field] = boost::bind( &convertToNasal<Var>,
                                      boost::bind(var, _2) );
        return *this;
      }

      static class_* getSingletonPtr()
      {
        return getSingletonHolder().get();
      }

      const GetterMap& getGetter() const
      {
        return _getter;
      }

    private:

      typedef std::auto_ptr<class_> class_ptr;
      GetterMap _getter;

      explicit class_(const std::string& name):
        class_metadata( name )
      {
        _ghost_type.destroy = &destroyGhost;
        _ghost_type.name = _name.c_str();
        _ghost_type.get_member = &getMember;
        _ghost_type.set_member = 0;
      }

      static class_ptr& getSingletonHolder()
      {
        static class_ptr instance;
        return instance;
      }

      static naRef makeGhost(naContext c, void *ptr)
      {
        std::cout << "makeGhost    " << ptr << std::endl;
        return naNewGhost2(c, &getSingletonPtr()->_ghost_type, ptr);
      }

      static void destroyGhost(void *ptr)
      {
        std::cout << "destroyGhost " << ptr << std::endl;
        delete (T*)ptr;
      }

      static const char* getMember(naContext c, void* g, naRef field, naRef* out)
      {
        const std::string key = naStr_data(field);
        if( key == "parents" )
        {
          *out = getSingletonPtr()->getParents(c);
          return "";
        }

        typename GetterMap::iterator getter =
          getSingletonPtr()->_getter.find(key);

        if( getter == getSingletonPtr()->_getter.end() )
          return 0;

        *out = getter->second(c, class_::getRawPtr(g));
        return "";
      }
  };
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
};

typedef nasal::class_<sc::CanvasPtr> NasalCanvas;

void initCanvas()
{

  NasalCanvas::init("Canvas")
    .def_readonly("texture", &canvasGetTexture)
    .def("size_x", &sc::Canvas::getSizeX)
    .def("size_y", &sc::Canvas::getSizeY);
  nasal::class_<sc::ElementPtr>::init("canvas.Element");
  nasal::class_<sc::GroupPtr>::init("canvas.Group")
    .bases<sc::ElementPtr>();

  nasal::class_<Base>::init("BaseClass")
    .def("int", &Base::getInt);
  nasal::class_<Test>::init("TestClass")
    .bases<Base>()
    .def_readonly("x", &Test::x);
}

/**
 * A Nasal Hash
 */
class Hash
{
  public:

    /**
     * Initialize from an existing Nasal Hash
     *
     * @param hash  Existing Nasal Hash
     * @param c     Nasal context
     */
    Hash(const naRef& hash, naContext c):
      _hash(hash),
      _context(c)
    {
      assert( naIsHash(_hash) );
    }

    void set(const std::string& name, naRef val)
    {
      naHash_set(_hash, stringToNasal(name), val);
    }

    void set(const std::string& name, naCFunction func)
    {
      set(name, naNewFunc(_context, naNewCCode(_context, func)));
    }

    void set(const std::string& name, const std::string& str)
    {
      set(name, stringToNasal(str));
    }

    void set(const std::string& name, double num)
    {
      set(name, naNum(num));
    }

    /**
     * Create a new child hash (module)
     *
     * @param name  Name of the new hash inside this hash
     */
    Hash createHash(const std::string& name)
    {
      naRef hash = naNewHash(_context);
      set(name, hash);
      return Hash(hash, _context);
    }

  protected:
    naRef _hash;
    naContext _context;

    naRef stringToNasal(const std::string& str)
    {
      naRef s = naNewString(_context);
      naStr_fromdata(s, str.c_str(), str.size());
      return s;
    }
};

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
    typedef std::map<std::string, getter_t> GetterMap;

    const std::string   _ghost_name;
    std::vector<naRef>  _parents;
    GetterMap           _getter;

    NasalObject(const std::string& ghost_name):
      _ghost_name( ghost_name )
    {
      _ghost_type.destroy = &destroyGhost;
      _ghost_type.name = _ghost_name.c_str();
      _ghost_type.get_member = &Derived::getMember;
      _ghost_type.set_member = 0;

      _getter["parents"] = &NasalObject::getParents;
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
      typename GetterMap::iterator getter =
        getInstance()._getter.find(naStr_data(field));

      if( getter == getInstance()._getter.end() )
        return 0;

      *out = (getInstance().*getter->second)(c, *static_cast<T*>(g));
      return "";
    }

  private:

    naGhostType _ghost_type;

};

static naRef stringToNasal(naContext c, const std::string& s)
{
    return naStr_fromdata(naNewString(c),
                   const_cast<char *>(s.c_str()),
                   s.length());
}

typedef osg::ref_ptr<osgGA::GUIEventAdapter> GUIEventPtr;

class NasalCanvasEvent:
  public NasalObject<GUIEventPtr, NasalCanvasEvent>
{
  public:

    NasalCanvasEvent():
      NasalObject("CanvasEvent")
    {
      _getter["type"] = &NasalCanvasEvent::getEventType;
    }

    naRef getEventType(naContext c, const GUIEventPtr& event)
    {
#define RET_EVENT_STR(type, str)\
  case osgGA::GUIEventAdapter::type:\
    return stringToNasal(c, str);

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

//using simgear::canvas::CanvasPtr;
//
///**
// * Expose Canvas to Nasal
// */
//class NasalCanvas:
//  public NasalObject<CanvasPtr, NasalCanvas>
//{
//  public:
//
//    NasalCanvas():
//      NasalObject("Canvas")
//    {
//      _getter["texture"] = &NasalCanvas::getTexture;
//      _getter["size_x"] = &NasalCanvas::getSizeX;
//      _getter["size_y"] = &NasalCanvas::getSizeY;
//    }
//
//    static naRef f_create(naContext c, naRef me, int argc, naRef* args)
//    {
//      std::cout << "NasalCanvas::create" << std::endl;
//
//      CanvasMgr* canvas_mgr =
//        static_cast<CanvasMgr*>(globals->get_subsystem("Canvas"));
//      if( !canvas_mgr )
//        return naNil();
//
//      return create(c, canvas_mgr->createCanvas());
//    }
//
//    static naRef f_setPrototype(naContext c, naRef me, int argc, naRef* args)
//    {
//      if( argc != 1 || !naIsHash(args[0]) )
//        naRuntimeError(c, "Invalid argument(s)");
//
//      getInstance().setParent(args[0]);
//
//      return naNil();
//    }
//
//    naRef getTexture(naContext c, const CanvasPtr& canvas)
//    {
//      //{ parents : [Node], _g : node }
//      naRef parents = naNewVector(c);
//      naVec_append(parents, propsNodePrototype);
//
//      naRef node = naNewHash(c);
//      hashset(c, node, "parents", parents);
//      hashset(c, node, "_g", propNodeGhostCreate(c, canvas->getProps()));
//
//      return node;
//    }
//
//    naRef getSizeX(naContext c, const CanvasPtr& canvas)
//    {
//      return naNum(canvas->getSizeX());
//    }
//
//    naRef getSizeY(naContext c, const CanvasPtr& canvas)
//    {
//      return naNum(canvas->getSizeY());
//    }
//};

static void elementGhostDestroy(void* g);

static const char* elementGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static void elementGhostSetMember(naContext c, void* g, naRef field, naRef value);
naGhostType ElementGhostType = { elementGhostDestroy, "canvas.element", 
    elementGhostGetMember, elementGhostSetMember };

static simgear::canvas::Element* elementGhost(naRef r)
{
  if (naGhost_type(r) == &ElementGhostType)
    return (simgear::canvas::Element*) naGhost_ptr(r);
  return 0;
}

static void elementGhostDestroy(void* g)
{
}

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

static const char* elementGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  simgear::canvas::Element* e = (simgear::canvas::Element*) g;
  SG_UNUSED(e);
  
  if (!strcmp(fieldName, "parents")) {
    *out = naNewVector(c);
    naVec_append(*out, elementPrototype);
  } else {
    return 0;
  }
  
  return "";
}

static void elementGhostSetMember(naContext c, void* g, naRef field, naRef value)
{
  const char* fieldName = naStr_data(field);
  simgear::canvas::Element* e = (simgear::canvas::Element*) g;
  SG_UNUSED(fieldName);
  SG_UNUSED(e);
}


static naRef f_canvas_getElement(naContext c, naRef me, int argc, naRef* args)
{
//  simgear::canvas::Canvas* cvs = canvasGhost(me);
//  if (!cvs) {
//    naRuntimeError(c, "canvas.getElement called on non-canvas object");
//  }
  
  return naNil();
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

static naRef f_createCanvas(naContext c, naRef me, int argc, naRef* args)
{
  std::cout << "f_createCanvas" << std::endl;

  CanvasMgr* canvas_mgr =
    static_cast<CanvasMgr*>(globals->get_subsystem("Canvas"));
  if( !canvas_mgr )
    return naNil();

  return NasalCanvas::create(c, canvas_mgr->createCanvas());
}

static naRef f_setCanvasPrototype(naContext c, naRef me, int argc, naRef* args)
{
  if( argc != 1 || !naIsHash(args[0]) )
    naRuntimeError(c, "Invalid argument(s)");

  NasalCanvas::getSingletonPtr()->addNasalBase(args[0]);

  return naNil();
}

naRef initNasalCanvas(naRef globals, naContext c, naRef gcSave)
{
  naRef props_module = naHash_cget(globals, (char*)"props");
  if( naIsNil(props_module) )
    std::cerr << "No props module" << std::endl;

  propsNodePrototype = naHash_cget(props_module, (char*)"Node");
  if( naIsNil(propsNodePrototype) )
    std::cerr << "Failed to get props.Node" << std::endl;

      /*naNewHash(c);
    hashset(c, gcSave, "canvasProto", canvasPrototype);
  
    hashset(c, canvasPrototype, "getElement", naNewFunc(c, naNewCCode(c, f_canvas_getElement)));*/
    // set any event methods
  
    elementPrototype = naNewHash(c);
    hashset(c, gcSave, "elementProto", elementPrototype);
    
    hashset(c, elementPrototype, "addButtonCallback", naNewFunc(c, naNewCCode(c, f_element_addButtonCallback)));
    hashset(c, elementPrototype, "addDragCallback", naNewFunc(c, naNewCCode(c, f_element_addDragCallback)));
    hashset(c, elementPrototype, "addMoveCallback", naNewFunc(c, naNewCCode(c, f_element_addMoveCallback)));
    hashset(c, elementPrototype, "addScrollCallback", naNewFunc(c, naNewCCode(c, f_element_addScrollCallback)));

  Hash globals_module(globals, c),
       canvas_module = globals_module.createHash("canvas");

  canvas_module.set("_new", f_createCanvas);
  canvas_module.set("_setPrototype", f_setCanvasPrototype);
  canvas_module.set("testClass", nasal::class_<Test>::f_create);

  initCanvas();

  return naNil();
}
