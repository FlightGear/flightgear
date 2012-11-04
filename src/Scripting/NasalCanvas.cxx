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

#include <string.h>

#include "NasalCanvas.hxx"

#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <osgGA/GUIEventAdapter>

#include <simgear/sg_inlines.h>

#include <simgear/canvas/Canvas.hxx>
#include <simgear/canvas/elements/element.hxx>

static naRef canvasPrototype;
static naRef elementPrototype;
static naRef eventPrototype;

static void canvasGhostDestroy(void* g);
static void elementGhostDestroy(void* g);
static void eventGhostDestroy(void* g);

static const char* canvasGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType CanvasGhostType = { canvasGhostDestroy, "canvas", canvasGhostGetMember, 0 };

static const char* elementGhostGetMember(naContext c, void* g, naRef field, naRef* out);
static void elementGhostSetMember(naContext c, void* g, naRef field, naRef value);
naGhostType ElementGhostType = { elementGhostDestroy, "canvas.element", 
    elementGhostGetMember, elementGhostSetMember };

static const char* eventGhostGetMember(naContext c, void* g, naRef field, naRef* out);
naGhostType EventGhostType = { eventGhostDestroy, "ga-event", eventGhostGetMember, 0 };

static void hashset(naContext c, naRef hash, const char* key, naRef val)
{
  naRef s = naNewString(c);
  naStr_fromdata(s, (char*)key, strlen(key));
  naHash_set(hash, s, val);
}

static naRef stringToNasal(naContext c, const std::string& s)
{
    return naStr_fromdata(naNewString(c),
                   const_cast<char *>(s.c_str()), 
                   s.length());
}

static naRef eventTypeToNasal(naContext c, osgGA::GUIEventAdapter::EventType ty)
{
  switch (ty) {
  case osgGA::GUIEventAdapter::PUSH: return stringToNasal(c, "push");
  case osgGA::GUIEventAdapter::RELEASE: return stringToNasal(c, "release");
  case osgGA::GUIEventAdapter::DOUBLECLICK: return stringToNasal(c, "double-click");
  case osgGA::GUIEventAdapter::DRAG: return stringToNasal(c, "drag");
  case osgGA::GUIEventAdapter::MOVE: return stringToNasal(c, "move");
  case osgGA::GUIEventAdapter::SCROLL: return stringToNasal(c, "scroll");
  case osgGA::GUIEventAdapter::KEYUP: return stringToNasal(c, "key-up");
  case osgGA::GUIEventAdapter::KEYDOWN: return stringToNasal(c, "key-down");

  default:
      break;
  }
  
  return naNil();
}

static simgear::canvas::Element* elementGhost(naRef r)
{
  if (naGhost_type(r) == &ElementGhostType)
    return (simgear::canvas::Element*) naGhost_ptr(r);
  return 0;
}

static simgear::canvas::Canvas* canvasGhost(naRef r)
{
  if (naGhost_type(r) == &CanvasGhostType)
    return (simgear::canvas::Canvas*) naGhost_ptr(r);
  return 0;
}

static void elementGhostDestroy(void* g)
{
}

static void canvasGhostDestroy(void* g)
{
}

static void eventGhostDestroy(void* g)
{
    osgGA::GUIEventAdapter* gea = static_cast<osgGA::GUIEventAdapter*>(g);
    gea->unref();
}

static const char* eventGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  osgGA::GUIEventAdapter* gea = (osgGA::GUIEventAdapter*) g;
  
  if (!strcmp(fieldName, "parents")) {
    *out = naNewVector(c);
    naVec_append(*out, eventPrototype);
  } else if (!strcmp(fieldName, "type")) *out = eventTypeToNasal(c, gea->getEventType());
  else if (!strcmp(fieldName, "windowX")) *out = naNum(gea->getWindowX());
  else if (!strcmp(fieldName, "windowY")) *out = naNum(gea->getWindowY());
  else if (!strcmp(fieldName, "time")) *out = naNum(gea->getTime());
  else if (!strcmp(fieldName, "button")) *out = naNum(gea->getButton());
  else {
    return 0;
  }
  
  return "";
}

static const char* canvasGhostGetMember(naContext c, void* g, naRef field, naRef* out)
{
  const char* fieldName = naStr_data(field);
  simgear::canvas::Canvas* cvs = (simgear::canvas::Canvas*) g;
  
  if (!strcmp(fieldName, "parents")) {
    *out = naNewVector(c);
    naVec_append(*out, canvasPrototype);
  } else if (!strcmp(fieldName, "sizeX")) *out = naNum(cvs->getSizeX());
  else if (!strcmp(fieldName, "sizeY")) *out = naNum(cvs->getSizeY());
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
  simgear::canvas::Canvas* cvs = canvasGhost(me);
  if (!cvs) {
    naRuntimeError(c, "canvas.getElement called on non-canvas object");
  }
  
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

static naRef f_canvas(naContext c, naRef me, int argc, naRef* args)
{
  return naNil();
}

// Table of extension functions.  Terminate with zeros.
static struct { const char* name; naCFunction func; } funcs[] = {
  { "getCanvas", f_canvas },
  { 0, 0 }
};

naRef initNasalCanvas(naRef globals, naContext c, naRef gcSave)
{
    canvasPrototype = naNewHash(c);
    hashset(c, gcSave, "canvasProto", canvasPrototype);
  
    hashset(c, canvasPrototype, "getElement", naNewFunc(c, naNewCCode(c, f_canvas_getElement)));
    
    eventPrototype = naNewHash(c);
    hashset(c, gcSave, "eventProto", eventPrototype);
    // set any event methods
  
    elementPrototype = naNewHash(c);
    hashset(c, gcSave, "elementProto", elementPrototype);
    
    hashset(c, elementPrototype, "addButtonCallback", naNewFunc(c, naNewCCode(c, f_element_addButtonCallback)));
    hashset(c, elementPrototype, "addDragCallback", naNewFunc(c, naNewCCode(c, f_element_addDragCallback)));
    hashset(c, elementPrototype, "addMoveCallback", naNewFunc(c, naNewCCode(c, f_element_addMoveCallback)));
    hashset(c, elementPrototype, "addScrollCallback", naNewFunc(c, naNewCCode(c, f_element_addScrollCallback)));
      
    for(int i=0; funcs[i].name; i++) {
      hashset(c, globals, funcs[i].name,
      naNewFunc(c, naNewCCode(c, funcs[i].func)));
    }
  
  return naNil();
}
