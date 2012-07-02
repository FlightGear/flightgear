//  panel.cxx - default, 2D single-engine prop instrument panel
//
//  Written by David Megginson, started January 2000.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//  $Id$

//JVK
// On 2D panels all instruments include light sources were in night displayed
// with a red mask (instrument light). It is not correct for light sources
// (bulbs). There is added new layer property "emissive" (boolean) (only for
// textured layers).
// If a layer has to shine set it in the "instrument_def_file.xml" inside the
// <layer> tag by adding <emissive>true</emissive> tag. When omitted the default
// value is for backward compatibility set to false.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "panel.hxx"

#include <stdio.h>	// sprintf
#include <string.h>
#include <iostream>
#include <cassert>

#include <osg/CullFace>
#include <osg/Depth>
#include <osg/Material>
#include <osg/Matrixf>
#include <osg/TexEnv>
#include <osg/PolygonOffset>

#include <simgear/compiler.h>

#include <plib/fnt.h>

#include <boost/foreach.hpp>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/model.hxx>
#include <osg/GLU>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Viewer/viewmgr.hxx>
#include <Viewer/viewer.hxx>
#include <Time/light.hxx>
#include <GUI/FGFontCache.hxx>	
#include <Instrumentation/dclgps.hxx>

#define WIN_X 0
#define WIN_Y 0
#define WIN_W 1024
#define WIN_H 768

// The number of polygon-offset "units" to place between layers.  In
// principle, one is supposed to be enough.  In practice, I find that
// my hardware/driver requires many more.
#define POFF_UNITS 8

const double MOUSE_ACTION_REPEAT_DELAY = 0.5; // 500msec initial delay
const double MOUSE_ACTION_REPEAT_INTERVAL = 0.1; // 10Hz repeat rate

using std::map;

////////////////////////////////////////////////////////////////////////
// Local functions.
////////////////////////////////////////////////////////////////////////


/**
 * Calculate the aspect adjustment for the panel.
 */
static float
get_aspect_adjust (int xsize, int ysize)
{
  float ideal_aspect = float(WIN_W) / float(WIN_H);
  float real_aspect = float(xsize) / float(ysize);
  return (real_aspect / ideal_aspect);
}

////////////////////////////////////////////////////////////////////////
// Implementation of FGTextureManager.
////////////////////////////////////////////////////////////////////////

map<string,osg::ref_ptr<osg::Texture2D> > FGTextureManager::_textureMap;

osg::Texture2D*
FGTextureManager::createTexture (const string &relativePath, bool staticTexture)
{
  osg::Texture2D* texture = _textureMap[relativePath].get();
  if (texture == 0) {
    SGPath tpath = globals->resolve_aircraft_path(relativePath);
    texture = SGLoadTexture2D(staticTexture, tpath);

    _textureMap[relativePath] = texture;
    if (!_textureMap[relativePath].valid()) 
      SG_LOG( SG_COCKPIT, SG_ALERT, "Texture *still* doesn't exist" );
    SG_LOG( SG_COCKPIT, SG_DEBUG, "Created texture " << relativePath );
  }

  return texture;
}


void FGTextureManager::addTexture(const string &relativePath,
                                  osg::Texture2D* texture)
{
    _textureMap[relativePath] = texture;
}

////////////////////////////////////////////////////////////////////////
// Implementation of FGCropped Texture.
////////////////////////////////////////////////////////////////////////


FGCroppedTexture::FGCroppedTexture ()
  : _path(""), _texture(0),
    _minX(0.0), _minY(0.0), _maxX(1.0), _maxY(1.0)
{
}


FGCroppedTexture::FGCroppedTexture (const string &path,
				    float minX, float minY,
				    float maxX, float maxY)
  : _path(path), _texture(0),
    _minX(minX), _minY(minY), _maxX(maxX), _maxY(maxY)
{
}


FGCroppedTexture::~FGCroppedTexture ()
{
}


osg::StateSet*
FGCroppedTexture::getTexture ()
{
  if (_texture == 0) {
    _texture = new osg::StateSet;
    _texture->setTextureAttribute(0, FGTextureManager::createTexture(_path));
    _texture->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    _texture->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::MODULATE));
  }
  return _texture.get();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPanel.
////////////////////////////////////////////////////////////////////////

static fntRenderer text_renderer;
static sgVec4 panel_color;
static sgVec4 emissive_panel_color = {1,1,1,1};

/**
 * Constructor.
 */
FGPanel::FGPanel ()
  : _mouseDown(false),
    _mouseInstrument(0),
    _width(WIN_W), _height(int(WIN_H * 0.5768 + 1)),
    _x_offset(fgGetNode("/sim/panel/x-offset", true)),
    _y_offset(fgGetNode("/sim/panel/y-offset", true)),
    _jitter(fgGetNode("/sim/panel/jitter", true)),
    _flipx(fgGetNode("/sim/panel/flip-x", true)),
    _xsize_node(fgGetNode("/sim/startup/xsize", true)),
    _ysize_node(fgGetNode("/sim/startup/ysize", true)),
    _enable_depth_test(false),
    _autohide(true),
    _drawPanelHotspots("/sim/panel-hotspots")
{
}


/**
 * Destructor.
 */
FGPanel::~FGPanel ()
{
  for (instrument_list_type::iterator it = _instruments.begin();
       it != _instruments.end();
       it++) {
    delete *it;
    *it = 0;
  }
}


/**
 * Add an instrument to the panel.
 */
void
FGPanel::addInstrument (FGPanelInstrument * instrument)
{
  _instruments.push_back(instrument);
}

double
FGPanel::getAspectScale() const
{
  // set corner-coordinates correctly
  
  int xsize = _xsize_node->getIntValue();
  int ysize = _ysize_node->getIntValue();
  float aspect_adjust = get_aspect_adjust(xsize, ysize);
  
  if (aspect_adjust < 1.0)
    return ysize / (double) WIN_H;
  else
    return xsize /(double) WIN_W;  
}

/**
 * Handle repeatable mouse events.  Called from update() and from
 * fgUpdate3DPanels().  This functionality needs to move into the
 * input subsystem.  Counting a tick every two frames is clumsy...
 */
void FGPanel::updateMouseDelay(double dt)
{
  if (!_mouseDown) {
    return;
  }

  _mouseActionRepeat -= dt;
  while (_mouseActionRepeat < 0.0) {
    _mouseActionRepeat += MOUSE_ACTION_REPEAT_INTERVAL;
    _mouseInstrument->doMouseAction(_mouseButton, 0, _mouseX, _mouseY);
    
  }
}

void
FGPanel::draw(osg::State& state)
{
    
  // In 3D mode, it's possible that we are being drawn exactly on top
  // of an existing polygon.  Use an offset to prevent z-fighting.  In
  // 2D mode, this is a no-op.
  static osg::ref_ptr<osg::StateSet> panelStateSet;
  if (!panelStateSet.valid()) {
    panelStateSet = new osg::StateSet;
    panelStateSet->setAttributeAndModes(new osg::PolygonOffset(-1, -POFF_UNITS));
    panelStateSet->setTextureAttribute(0, new osg::TexEnv);

    // Draw the background
    panelStateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    panelStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    panelStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    panelStateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
  
    osg::Material* material = new osg::Material;
    material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1, 1, 1, 1));
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1, 1, 1, 1));
    material->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4(0, 0, 0, 1));
    panelStateSet->setAttribute(material);
    
    panelStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    panelStateSet->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK));
    panelStateSet->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL));
  }
  if ( _enable_depth_test )
    panelStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
  else
    panelStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
  state.pushStateSet(panelStateSet.get());
  state.apply();
  state.setActiveTextureUnit(0);
  state.setClientActiveTextureUnit(0);

  FGLight *l = (FGLight *)(globals->get_subsystem("lighting"));
  sgCopyVec4( panel_color, l->scene_diffuse().data());
  if ( fgGetDouble("/systems/electrical/outputs/instrument-lights") > 1.0 ) {
      if ( panel_color[0] < 0.7 ) panel_color[0] = 0.7;
      if ( panel_color[1] < 0.2 ) panel_color[1] = 0.2;
      if ( panel_color[2] < 0.2 ) panel_color[2] = 0.2;
  }
  glColor4fv( panel_color );
  if (_bg != 0) {
    state.pushStateSet(_bg.get());
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);

    glBegin(GL_POLYGON);
    glTexCoord2f(0.0, 0.0); glVertex2f(WIN_X, WIN_Y);
    glTexCoord2f(1.0, 0.0); glVertex2f(WIN_X + _width, WIN_Y);
    glTexCoord2f(1.0, 1.0); glVertex2f(WIN_X + _width, WIN_Y + _height);
    glTexCoord2f(0.0, 1.0); glVertex2f(WIN_X, WIN_Y + _height);
    glEnd();
    state.popStateSet();
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);

  } else {
    for (int i = 0; i < 4; i ++) {
      // top row of textures...(1,3,5,7)
      state.pushStateSet(_mbg[i*2].get());
      state.apply();
      state.setActiveTextureUnit(0);
      state.setClientActiveTextureUnit(0);

      glBegin(GL_POLYGON);
      glTexCoord2f(0.0, 0.0); glVertex2f(WIN_X + (_width/4) * i, WIN_Y + (_height/2));
      glTexCoord2f(1.0, 0.0); glVertex2f(WIN_X + (_width/4) * (i+1), WIN_Y + (_height/2));
      glTexCoord2f(1.0, 1.0); glVertex2f(WIN_X + (_width/4) * (i+1), WIN_Y + _height);
      glTexCoord2f(0.0, 1.0); glVertex2f(WIN_X + (_width/4) * i, WIN_Y + _height);
      glEnd();
      state.popStateSet();
      state.apply();
      state.setActiveTextureUnit(0);
      state.setClientActiveTextureUnit(0);

      // bottom row of textures...(2,4,6,8)
      state.pushStateSet(_mbg[i*2+1].get());
      state.apply();
      state.setActiveTextureUnit(0);
      state.setClientActiveTextureUnit(0);

      glBegin(GL_POLYGON);
      glTexCoord2f(0.0, 0.0); glVertex2f(WIN_X + (_width/4) * i, WIN_Y);
      glTexCoord2f(1.0, 0.0); glVertex2f(WIN_X + (_width/4) * (i+1), WIN_Y);
      glTexCoord2f(1.0, 1.0); glVertex2f(WIN_X + (_width/4) * (i+1), WIN_Y + (_height/2));
      glTexCoord2f(0.0, 1.0); glVertex2f(WIN_X + (_width/4) * i, WIN_Y + (_height/2));
      glEnd();
      state.popStateSet();
      state.apply();
      state.setActiveTextureUnit(0);
      state.setClientActiveTextureUnit(0);

    }
  }

  // Draw the instruments.
  // Syd Adams: added instrument clipping
  instrument_list_type::const_iterator current = _instruments.begin();
  instrument_list_type::const_iterator end = _instruments.end();

  GLdouble blx[4]={1.0,0.0,0.0,0.0};
  GLdouble bly[4]={0.0,1.0,0.0,0.0};
  GLdouble urx[4]={-1.0,0.0,0.0,0.0};
  GLdouble ury[4]={0.0,-1.0,0.0,0.0};

  for ( ; current != end; current++) {
    FGPanelInstrument * instr = *current;
    glPushMatrix();
    glTranslated(instr->getXPos(), instr->getYPos(), 0);

    int ix= instr->getWidth();
    int iy= instr->getHeight();
    glPushMatrix();
    glTranslated(-ix/2,-iy/2,0);
    glClipPlane(GL_CLIP_PLANE0,blx);
    glClipPlane(GL_CLIP_PLANE1,bly);
    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);

    glTranslated(ix,iy,0);
    glClipPlane(GL_CLIP_PLANE2,urx);
    glClipPlane(GL_CLIP_PLANE3,ury);
    glEnable(GL_CLIP_PLANE2);
    glEnable(GL_CLIP_PLANE3);
    glPopMatrix();
    instr->draw(state);

    glPopMatrix();
  }

  glDisable(GL_CLIP_PLANE0);
  glDisable(GL_CLIP_PLANE1);
  glDisable(GL_CLIP_PLANE2);
  glDisable(GL_CLIP_PLANE3);

  state.popStateSet();
  state.apply();
  state.setActiveTextureUnit(0);
  state.setClientActiveTextureUnit(0);


  // Draw yellow "hotspots" if directed to.  This is a panel authoring
  // feature; not intended to be high performance or to look good.
  if ( _drawPanelHotspots ) {
    static osg::ref_ptr<osg::StateSet> hotspotStateSet;
    if (!hotspotStateSet.valid()) {
      hotspotStateSet = new osg::StateSet;
      hotspotStateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::OFF);
      hotspotStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    }

    state.pushStateSet(hotspotStateSet.get());
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);


    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_COLOR_MATERIAL);
    glColor3f(1, 1, 0);
    
    for ( unsigned int i = 0; i < _instruments.size(); i++ )
      _instruments[i]->drawHotspots(state);

  // disable drawing of panel extents for the 2.8 release, since it
  // confused use of hot-spot drawing in tutorials.
#ifdef DRAW_PANEL_EXTENTS
    glColor3f(0, 1, 1);

    int x0, y0, x1, y1;
    getLogicalExtent(x0, y0, x1, y1);
    
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x1, y1);
    glVertex2f(x0, y1);
    glEnd();
#endif
    
    glPopAttrib();

    state.popStateSet();
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);

  }
}

/**
 * Set the panel's background texture.
 */
void
FGPanel::setBackground (osg::Texture2D* texture)
{
  osg::StateSet* stateSet = new osg::StateSet;
  stateSet->setTextureAttribute(0, texture);
  stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
  stateSet->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::MODULATE));
  _bg = stateSet;
}

/**
 * Set the panel's multiple background textures.
 */
void
FGPanel::setMultiBackground (osg::Texture2D* texture, int idx)
{
  _bg = 0;

  osg::StateSet* stateSet = new osg::StateSet;
  stateSet->setTextureAttribute(0, texture);
  stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
  stateSet->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::MODULATE));
  _mbg[idx] = stateSet;
}

/**
 * Set the panel's x-offset.
 */
void
FGPanel::setXOffset (int offset)
{
  if (offset <= 0 && offset >= -_width + WIN_W)
    _x_offset->setIntValue( offset );
}


/**
 * Set the panel's y-offset.
 */
void
FGPanel::setYOffset (int offset)
{
  if (offset <= 0 && offset >= -_height)
    _y_offset->setIntValue( offset );
}

/**
 * Handle a mouse action in panel-local (not screen) coordinates.
 * Used by the 3D panel code in Model/panelnode.cxx, in situations
 * where the panel doesn't control its own screen location.
 */
bool
FGPanel::doLocalMouseAction(int button, int updown, int x, int y)
{
  // Note a released button and return
  if (updown == 1) {
    if (_mouseInstrument != 0)
        _mouseInstrument->doMouseAction(_mouseButton, 1, _mouseX, _mouseY);
    _mouseDown = false;
    _mouseInstrument = 0;
    return false;
  }

  // Search for a matching instrument.
  for (int i = 0; i < (int)_instruments.size(); i++) {
    FGPanelInstrument *inst = _instruments[i];
    int ix = inst->getXPos();
    int iy = inst->getYPos();
    int iw = inst->getWidth() / 2;
    int ih = inst->getHeight() / 2;
    if (x >= ix - iw && x < ix + iw && y >= iy - ih && y < iy + ih) {
      _mouseDown = true;
      _mouseActionRepeat = MOUSE_ACTION_REPEAT_DELAY;
      _mouseInstrument = inst;
      _mouseButton = button;
      _mouseX = x - ix;
      _mouseY = y - iy;
      // Always do the action once.
      return _mouseInstrument->doMouseAction(_mouseButton, 0,
                                             _mouseX, _mouseY);
    }
  }
  return false;
}

/**
 * Perform a mouse action.
 */
bool
FGPanel::doMouseAction (int button, int updown, int x, int y)
{
				// FIXME: this same code appears in update()
  int xsize = _xsize_node->getIntValue();
  int ysize = _ysize_node->getIntValue();
  float aspect_adjust = get_aspect_adjust(xsize, ysize);

				// Scale for the real window size.
  if (aspect_adjust < 1.0) {
    x = int(((float)x / xsize) * WIN_W * aspect_adjust);
    y = int(WIN_H - ((float(y) / ysize) * WIN_H));
  } else {
    x = int(((float)x / xsize) * WIN_W);
    y = int((WIN_H - ((float(y) / ysize) * WIN_H)) / aspect_adjust);
  }

				// Adjust for offsets.
  x -= _x_offset->getIntValue();
  y -= _y_offset->getIntValue();

  // Having fixed up the coordinates, fall through to the local
  // coordinate handler.
  return doLocalMouseAction(button, updown, x, y);
} 

void FGPanel::setDepthTest (bool enable) {
    _enable_depth_test = enable;
}

class IntRect
{
  
public:
  IntRect() : 
    x0(std::numeric_limits<int>::max()), 
    y0(std::numeric_limits<int>::max()), 
    x1(std::numeric_limits<int>::min()), 
    y1(std::numeric_limits<int>::min()) 
  { }
  
  IntRect(int x, int y, int w, int h) :
    x0(x), y0(y), x1(x + w), y1( y + h)
  { 
    if (x1 < x0) {
      std::swap(x0, x1);
    }
    
    if (y1 < y0) {
      std::swap(y0, y1);
    }
    
    assert(x0 <= x1);
    assert(y0 <= y1);
  }
  
  void extend(const IntRect& r)
  {
    x0 = std::min(x0, r.x0);
    y0 = std::min(y0, r.y0);
    x1 = std::max(x1, r.x1);
    y1 = std::max(y1, r.y1);
  }
  
  int x0, y0, x1, y1;
};

void FGPanel::getLogicalExtent(int &x0, int& y0, int& x1, int &y1)
{  
  IntRect result;
  BOOST_FOREACH(FGPanelInstrument *inst, _instruments) {
    inst->extendRect(result);
  }
  
  x0 = result.x0;
  y0 = result.y0;
  x1 = result.x1;
  y1 = result.y1;
}

////////////////////////////////////////////////////////////////////////.
// Implementation of FGPanelAction.
////////////////////////////////////////////////////////////////////////

FGPanelAction::FGPanelAction ()
{
}

FGPanelAction::FGPanelAction (int button, int x, int y, int w, int h,
                              bool repeatable)
    : _button(button), _x(x), _y(y), _w(w), _h(h), _repeatable(repeatable)
{
}

FGPanelAction::~FGPanelAction ()
{
  for (unsigned int i = 0; i < 2; i++) {
      for (unsigned int j = 0; j < _bindings[i].size(); j++)
          delete _bindings[i][j];
  }
}

void
FGPanelAction::addBinding (SGBinding * binding, int updown)
{
  _bindings[updown].push_back(binding);
}

bool
FGPanelAction::doAction (int updown)
{
  if (test()) {
    if ((updown != _last_state) || (updown == 0 && _repeatable)) {
        int nBindings = _bindings[updown].size();
        for (int i = 0; i < nBindings; i++)
            _bindings[updown][i]->fire();
    }
    _last_state = updown;
    return true;
  } else {
    return false;
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPanelTransformation.
////////////////////////////////////////////////////////////////////////

FGPanelTransformation::FGPanelTransformation ()
  : table(0)
{
}

FGPanelTransformation::~FGPanelTransformation ()
{
  delete table;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPanelInstrument.
////////////////////////////////////////////////////////////////////////


FGPanelInstrument::FGPanelInstrument ()
{
  setPosition(0, 0);
  setSize(0, 0);
}

FGPanelInstrument::FGPanelInstrument (int x, int y, int w, int h)
{
  setPosition(x, y);
  setSize(w, h);
}

FGPanelInstrument::~FGPanelInstrument ()
{
  for (action_list_type::iterator it = _actions.begin();
       it != _actions.end();
       it++) {
    delete *it;
    *it = 0;
  }
}

void
FGPanelInstrument::drawHotspots(osg::State& state)
{
  for ( unsigned int i = 0; i < _actions.size(); i++ ) {
    FGPanelAction* a = _actions[i];
    float x1 = getXPos() + a->getX();
    float x2 = x1 + a->getWidth();
    float y1 = getYPos() + a->getY();
    float y2 = y1 + a->getHeight();

    glBegin(GL_LINE_LOOP);
    glVertex2f(x1, y1);
    glVertex2f(x1, y2);
    glVertex2f(x2, y2);
    glVertex2f(x2, y1);
    glEnd();
  }
}

void
FGPanelInstrument::setPosition (int x, int y)
{
  _x = x;
  _y = y;
}

void
FGPanelInstrument::setSize (int w, int h)
{
  _w = w;
  _h = h;
}

int
FGPanelInstrument::getXPos () const
{
  return _x;
}

int
FGPanelInstrument::getYPos () const
{
  return _y;
}

int
FGPanelInstrument::getWidth () const
{
  return _w;
}

int
FGPanelInstrument::getHeight () const
{
  return _h;
}

void
FGPanelInstrument::extendRect(IntRect& r) const
{
  IntRect instRect(_x, _y, _w, _h);
  r.extend(instRect);
  
  BOOST_FOREACH(FGPanelAction* act, _actions) {
    r.extend(IntRect(getXPos() + act->getX(), 
                     getYPos() + act->getY(),
                     act->getWidth(),
                     act->getHeight()
                     ));
  }
}

void
FGPanelInstrument::addAction (FGPanelAction * action)
{
  _actions.push_back(action);
}

				// Coordinates relative to centre.
bool
FGPanelInstrument::doMouseAction (int button, int updown, int x, int y)
{
  if (test()) {
    action_list_type::iterator it = _actions.begin();
    action_list_type::iterator last = _actions.end();
    for ( ; it != last; it++) {
      if ((*it)->inArea(button, x, y) &&
          (*it)->doAction(updown))
        return true;
    }
  }
  return false;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGLayeredInstrument.
////////////////////////////////////////////////////////////////////////

FGLayeredInstrument::FGLayeredInstrument (int x, int y, int w, int h)
  : FGPanelInstrument(x, y, w, h)
{
}

FGLayeredInstrument::~FGLayeredInstrument ()
{
  for (layer_list::iterator it = _layers.begin(); it != _layers.end(); it++) {
    delete *it;
    *it = 0;
  }
}

void
FGLayeredInstrument::draw (osg::State& state)
{
  if (!test())
    return;
  
  for (int i = 0; i < (int)_layers.size(); i++) {
    glPushMatrix();
    _layers[i]->draw(state);
    glPopMatrix();
  }
}

int
FGLayeredInstrument::addLayer (FGInstrumentLayer *layer)
{
  int n = _layers.size();
  if (layer->getWidth() == -1) {
    layer->setWidth(getWidth());
  }
  if (layer->getHeight() == -1) {
    layer->setHeight(getHeight());
  }
  _layers.push_back(layer);
  return n;
}

int
FGLayeredInstrument::addLayer (const FGCroppedTexture &texture,
			       int w, int h)
{
  return addLayer(new FGTexturedLayer(texture, w, h));
}

void
FGLayeredInstrument::addTransformation (FGPanelTransformation * transformation)
{
  int layer = _layers.size() - 1;
  _layers[layer]->addTransformation(transformation);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGSpecialInstrument.
////////////////////////////////////////////////////////////////////////

FGSpecialInstrument::FGSpecialInstrument (DCLGPS* sb)
  : FGPanelInstrument()
{
  complex = sb;
}

FGSpecialInstrument::~FGSpecialInstrument ()
{
}

void
FGSpecialInstrument::draw (osg::State& state)
{
  complex->draw(state);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInstrumentLayer.
////////////////////////////////////////////////////////////////////////

FGInstrumentLayer::FGInstrumentLayer (int w, int h)
  : _w(w),
    _h(h)
{
}

FGInstrumentLayer::~FGInstrumentLayer ()
{
  for (transformation_list::iterator it = _transformations.begin();
       it != _transformations.end();
       it++) {
    delete *it;
    *it = 0;
  }
}

void
FGInstrumentLayer::transform () const
{
  transformation_list::const_iterator it = _transformations.begin();
  transformation_list::const_iterator last = _transformations.end();
  while (it != last) {
    FGPanelTransformation *t = *it;
    if (t->test()) {
      float val = (t->node == 0 ? 0.0 : t->node->getFloatValue());

      if (t->has_mod)
          val = fmod(val, t->mod);
      if (val < t->min) {
	val = t->min;
      } else if (val > t->max) {
	val = t->max;
      }

      if (t->table==0) {
	val = val * t->factor + t->offset;
      } else {
	val = t->table->interpolate(val) * t->factor + t->offset;
      }
      
      switch (t->type) {
      case FGPanelTransformation::XSHIFT:
	glTranslatef(val, 0.0, 0.0);
	break;
      case FGPanelTransformation::YSHIFT:
	glTranslatef(0.0, val, 0.0);
	break;
      case FGPanelTransformation::ROTATION:
	glRotatef(-val, 0.0, 0.0, 1.0);
	break;
      }
    }
    it++;
  }
}

void
FGInstrumentLayer::addTransformation (FGPanelTransformation * transformation)
{
  _transformations.push_back(transformation);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGGroupLayer.
////////////////////////////////////////////////////////////////////////

FGGroupLayer::FGGroupLayer ()
{
}

FGGroupLayer::~FGGroupLayer ()
{
  for (unsigned int i = 0; i < _layers.size(); i++)
    delete _layers[i];
}

void
FGGroupLayer::draw (osg::State& state)
{
  if (test()) {
    transform();
    int nLayers = _layers.size();
    for (int i = 0; i < nLayers; i++)
      _layers[i]->draw(state);
  }
}

void
FGGroupLayer::addLayer (FGInstrumentLayer * layer)
{
  _layers.push_back(layer);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTexturedLayer.
////////////////////////////////////////////////////////////////////////


FGTexturedLayer::FGTexturedLayer (const FGCroppedTexture &texture, int w, int h)
  : FGInstrumentLayer(w, h),
    _emissive(false)
{
  setTexture(texture);
}


FGTexturedLayer::~FGTexturedLayer ()
{
}


void
FGTexturedLayer::draw (osg::State& state)
{
  if (test()) {
    int w2 = _w / 2;
    int h2 = _h / 2;
    
    transform();
    state.pushStateSet(_texture.getTexture());
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);

    glBegin(GL_POLYGON);

    if (_emissive) {
      glColor4fv( emissive_panel_color );
    } else {
				// From Curt: turn on the panel
				// lights after sundown.
      glColor4fv( panel_color );
    }

    glTexCoord2f(_texture.getMinX(), _texture.getMinY()); glVertex2f(-w2, -h2);
    glTexCoord2f(_texture.getMaxX(), _texture.getMinY()); glVertex2f(w2, -h2);
    glTexCoord2f(_texture.getMaxX(), _texture.getMaxY()); glVertex2f(w2, h2);
    glTexCoord2f(_texture.getMinX(), _texture.getMaxY()); glVertex2f(-w2, h2);
    glEnd();
    state.popStateSet();
    state.apply();
    state.setActiveTextureUnit(0);
    state.setClientActiveTextureUnit(0);

  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextLayer.
////////////////////////////////////////////////////////////////////////

FGTextLayer::FGTextLayer (int w, int h)
  : FGInstrumentLayer(w, h), _pointSize(14.0), _font_name("Helvetica.txf")
{
  _then.stamp();
  _color[0] = _color[1] = _color[2] = 0.0;
  _color[3] = 1.0;
}

FGTextLayer::~FGTextLayer ()
{
  chunk_list::iterator it = _chunks.begin();
  chunk_list::iterator last = _chunks.end();
  for ( ; it != last; it++) {
    delete *it;
  }
}

void
FGTextLayer::draw (osg::State& state)
{
  if (test()) {
    glColor4fv(_color);
    transform();

    FGFontCache *fc = globals->get_fontcache();
    fntFont* font = fc->getTexFont(_font_name.c_str());
    if (!font) {
        return; // don't crash on missing fonts
    }
    
    text_renderer.setFont(font);

    text_renderer.setPointSize(_pointSize);
    text_renderer.begin();
    text_renderer.start3f(0, 0, 0);

    _now.stamp();
    double diff = (_now - _then).toUSecs();

    if (diff > 100000 || diff < 0 ) {
      // ( diff < 0 ) is a sanity check and indicates our time stamp
      // difference math probably overflowed.  We can handle a max
      // difference of 35.8 minutes since the returned value is in
      // usec.  So if the panel is left off longer than that we can
      // over flow the math with it is turned back on.  This (diff <
      // 0) catches that situation, get's us out of trouble, and
      // back on track.
      recalc_value();
      _then = _now;
    }

    // Something is goofy.  The code in this file renders only CCW
    // polygons, and I have verified that the font code in plib
    // renders only CCW trianbles.  Yet they come out backwards.
    // Something around here or in plib is either changing the winding
    // order or (more likely) pushing a left-handed matrix onto the
    // stack.  But I can't find it; get out the chainsaw...
    glFrontFace(GL_CW);
    text_renderer.puts((char *)(_value.c_str()));
    glFrontFace(GL_CCW);

    text_renderer.end();
    glColor4f(1.0, 1.0, 1.0, 1.0);	// FIXME
  }
}

void
FGTextLayer::addChunk (FGTextLayer::Chunk * chunk)
{
  _chunks.push_back(chunk);
}

void
FGTextLayer::setColor (float r, float g, float b)
{
  _color[0] = r;
  _color[1] = g;
  _color[2] = b;
  _color[3] = 1.0;
}

void
FGTextLayer::setPointSize (float size)
{
  _pointSize = size;
}

void
FGTextLayer::setFontName(const string &name)
{
  _font_name = name + ".txf";
  FGFontCache *fc = globals->get_fontcache();
  fntFont* font = fc->getTexFont(_font_name.c_str());
  if (!font) {
      SG_LOG(SG_COCKPIT, SG_WARN, "unable to find font:" << name);
  }
}


void
FGTextLayer::setFont(fntFont * font)
{
  text_renderer.setFont(font);
}


void
FGTextLayer::recalc_value () const
{
  _value = "";
  chunk_list::const_iterator it = _chunks.begin();
  chunk_list::const_iterator last = _chunks.end();
  for ( ; it != last; it++) {
    _value += (*it)->getValue();
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextLayer::Chunk.
////////////////////////////////////////////////////////////////////////

FGTextLayer::Chunk::Chunk (const string &text, const string &fmt)
  : _type(FGTextLayer::TEXT), _fmt(fmt)
{
  _text = text;
  if (_fmt.empty()) 
    _fmt = "%s";
}

FGTextLayer::Chunk::Chunk (ChunkType type, const SGPropertyNode * node,
			   const string &fmt, float mult, float offs,
                           bool truncation)
  : _type(type), _fmt(fmt), _mult(mult), _offs(offs), _trunc(truncation)
{
  if (_fmt.empty()) {
    if (type == TEXT_VALUE)
      _fmt = "%s";
    else
      _fmt = "%.2f";
  }
  _node = node;
}

const char *
FGTextLayer::Chunk::getValue () const
{
  if (test()) {
    _buf[0] = '\0';
    switch (_type) {
    case TEXT:
      sprintf(_buf, _fmt.c_str(), _text.c_str());
      return _buf;
    case TEXT_VALUE:
      sprintf(_buf, _fmt.c_str(), _node->getStringValue());
      break;
    case DOUBLE_VALUE:
      double d = _offs + _node->getFloatValue() * _mult;
      if (_trunc)  d = (d < 0) ? -floor(-d) : floor(d);
      sprintf(_buf, _fmt.c_str(), d);
      break;
    }
    return _buf;
  } else {
    return "";
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGSwitchLayer.
////////////////////////////////////////////////////////////////////////

FGSwitchLayer::FGSwitchLayer ()
  : FGGroupLayer()
{
}

void
FGSwitchLayer::draw (osg::State& state)
{
  if (test()) {
    transform();
    int nLayers = _layers.size();
    for (int i = 0; i < nLayers; i++) {
      if (_layers[i]->test()) {
          _layers[i]->draw(state);
          return;
      }
    }
  }
}


// end of panel.cxx
