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
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//  $Id$

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <string.h>

#include <plib/ssg.h>
#include <plib/fnt.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgpath.hxx>
#include <Main/globals.hxx>
#include <Main/options.hxx>
#include <Objects/texload.h>

#include "hud.hxx"
#include "panel.hxx"


bool
fgPanelVisible ()
{
  return ((current_options.get_panel_status()) &&
	  (current_options.get_view_mode() == fgOPTIONS::FG_VIEW_PILOT) &&
	  (globals->get_current_view()->get_view_offset() == 0.0));
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextureManager.
////////////////////////////////////////////////////////////////////////

map<string,ssgTexture *> FGTextureManager::_textureMap;

ssgTexture *
FGTextureManager::createTexture (const string &relativePath)
{
  ssgTexture * texture = _textureMap[relativePath];
  if (texture == 0) {
    cerr << "Texture " << relativePath << " does not yet exist" << endl;
    FGPath tpath(current_options.get_fg_root());
    tpath.append(relativePath);
    texture = new ssgTexture((char *)tpath.c_str(), false, false);
    _textureMap[relativePath] = texture;
    if (_textureMap[relativePath] == 0) 
      cerr << "Texture *still* doesn't exist" << endl;
    cerr << "Created texture " << relativePath
	 << " handle=" << texture->getHandle() << endl;
  }

  return texture;
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


ssgTexture *
FGCroppedTexture::getTexture ()
{
  if (_texture == 0) {
    _texture = FGTextureManager::createTexture(_path);
  }
  return _texture;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPanel.
////////////////////////////////////////////////////////////////////////

FGPanel * current_panel = NULL;
static fntRenderer text_renderer;


/**
 * Constructor.
 */
FGPanel::FGPanel (int window_x, int window_y, int window_w, int window_h)
  : _mouseDown(false),
    _mouseInstrument(0),
    _winx(window_x), _winy(window_y), _winw(window_w), _winh(window_h),
    _width(_winw), _height(int(_winh * 0.5768 + 1)),
    _x_offset(0), _y_offset(0), _view_height(int(_winh * 0.4232))
{
  setVisibility(fgPanelVisible());
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


/**
 * Update the panel.
 */
void
FGPanel::update () const
{
				// Do nothing if the panel isn't visible.
  if (!fgPanelVisible())
    return;

				// If the mouse is down, do something
  if (_mouseDown) {
    _mouseDelay--;
    if (_mouseDelay < 0) {
      _mouseInstrument->doMouseAction(_mouseButton, _mouseX, _mouseY);
      _mouseDelay = 2;
    }
  }

				// Now, draw the panel
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(_winx, _winx + _winw, _winy, _winy + _winh);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glTranslated(_x_offset, _y_offset, 0);

				// Draw the background
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glEnable(GL_ALPHA_TEST);
  glEnable(GL_COLOR_MATERIAL);
  // glColor4f(1.0, 1.0, 1.0, 1.0);
  if ( cur_light_params.sun_angle * RAD_TO_DEG < 95.0 ) {
      glColor4fv( cur_light_params.scene_diffuse );
  } else {
      glColor4f(0.7, 0.2, 0.2, 1.0);
  }
  glBindTexture(GL_TEXTURE_2D, _bg->getHandle());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glBegin(GL_POLYGON);
  glTexCoord2f(0.0, 0.0); glVertex3f(_winx, _winy, 0);
  glTexCoord2f(1.0, 0.0); glVertex3f(_winx + _width, _winy, 0);
  glTexCoord2f(1.0, 1.0); glVertex3f(_winx + _width, _winy + _height, 0);
  glTexCoord2f(0.0, 1.0); glVertex3f(_winx, _winy + _height, 0);
  glEnd();

				// Draw the instruments.
  instrument_list_type::const_iterator current = _instruments.begin();
  instrument_list_type::const_iterator end = _instruments.end();

  for ( ; current != end; current++) {
    FGPanelInstrument * instr = *current;
    glLoadIdentity();
    glTranslated(_x_offset, _y_offset, 0);
    glTranslated(instr->getXPos(), instr->getYPos(), 0);
    instr->draw();
  }

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  ssgForceBasicState();
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}


/**
 * Set the panel's visibility.
 */
void
FGPanel::setVisibility (bool visibility)
{
  _visibility = visibility;
}


/**
 * Return true if the panel is visible.
 */
bool
FGPanel::getVisibility () const
{
  return _visibility;
}


/**
 * Set the panel's background texture.
 */
void
FGPanel::setBackground (ssgTexture * texture)
{
  _bg = texture;
}


/**
 * Set the panel's x-offset.
 */
void
FGPanel::setXOffset (int offset)
{
  if (offset <= 0 && offset >= -_width + _winw)
    _x_offset = offset;
}


/**
 * Set the panel's y-offset.
 */
void
FGPanel::setYOffset (int offset)
{
  if (offset <= 0 && offset >= -_height)
    _y_offset = offset;
}


/**
 * Perform a mouse action.
 */
bool
FGPanel::doMouseAction (int button, int updown, int x, int y)
{

				// Note a released button and return
  // cerr << "Doing mouse action\n";
  if (updown == 1) {
    _mouseDown = false;
    _mouseInstrument = 0;
    return true;
  }

				// Scale for the real window size.
  x = int(((float)x / globals->get_current_view()->get_winWidth()) * _winw);
  y = int(_winh - (((float)y / globals->get_current_view()->get_winHeight())
		   * _winh));

				// Adjust for offsets.
  x -= _x_offset;
  y -= _y_offset;

				// Search for a matching instrument.
  for (int i = 0; i < (int)_instruments.size(); i++) {
    FGPanelInstrument *inst = _instruments[i];
    int ix = inst->getXPos();
    int iy = inst->getYPos();
    int iw = inst->getWidth() / 2;
    int ih = inst->getHeight() / 2;
    if (x >= ix - iw && x < ix + iw && y >= iy - ih && y < iy + ih) {
      _mouseDown = true;
      _mouseDelay = 20;
      _mouseInstrument = inst;
      _mouseButton = button;
      _mouseX = x - ix;
      _mouseY = y - iy;
				// Always do the action once.
      _mouseInstrument->doMouseAction(_mouseButton, _mouseX, _mouseY);
      return true;
    }
  }
  return false;
}



////////////////////////////////////////////////////////////////////////.
// Implementation of FGPanelAction.
////////////////////////////////////////////////////////////////////////

FGPanelAction::FGPanelAction ()
{
}

FGPanelAction::FGPanelAction (int button, int x, int y, int w, int h)
  : _button(button), _x(x), _y(y), _w(w), _h(h)
{
}

FGPanelAction::~FGPanelAction ()
{
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAdjustAction.
////////////////////////////////////////////////////////////////////////

FGAdjustAction::FGAdjustAction (int button, int x, int y, int w, int h,
				SGValue * value, float increment, 
				float min, float max, bool wrap)
  : FGPanelAction(button, x, y, w, h),
    _value(value), _increment(increment), _min(min), _max(max), _wrap(wrap)
{
}

FGAdjustAction::~FGAdjustAction ()
{
}

void
FGAdjustAction::doAction ()
{
  float val = _value->getFloatValue();
  val += _increment;
  if (val < _min) {
    val = (_wrap ? _max : _min);
  } else if (val > _max) {
    val = (_wrap ? _min : _max);
  }
  _value->setDoubleValue(val);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGSwapAction.
////////////////////////////////////////////////////////////////////////

FGSwapAction::FGSwapAction (int button, int x, int y, int w, int h,
			    SGValue * value1, SGValue * value2)
  : FGPanelAction(button, x, y, w, h), _value1(value1), _value2(value2)
{
}

FGSwapAction::~FGSwapAction ()
{
}

void
FGSwapAction::doAction ()
{
  float val = _value1->getFloatValue();
  _value1->setDoubleValue(_value2->getFloatValue());
  _value2->setDoubleValue(val);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGToggleAction.
////////////////////////////////////////////////////////////////////////

FGToggleAction::FGToggleAction (int button, int x, int y, int w, int h,
				SGValue * value)
  : FGPanelAction(button, x, y, w, h), _value(value)
{
}

FGToggleAction::~FGToggleAction ()
{
}

void
FGToggleAction::doAction ()
{
  _value->setBoolValue(!(_value->getBoolValue()));
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPanelTransformation.
////////////////////////////////////////////////////////////////////////

FGPanelTransformation::FGPanelTransformation ()
{
}

FGPanelTransformation::~FGPanelTransformation ()
{
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
FGPanelInstrument::addAction (FGPanelAction * action)
{
  _actions.push_back(action);
}

				// Coordinates relative to centre.
bool
FGPanelInstrument::doMouseAction (int button, int x, int y)
{
  action_list_type::iterator it = _actions.begin();
  action_list_type::iterator last = _actions.end();
  for ( ; it != last; it++) {
    if ((*it)->inArea(button, x, y)) {
      (*it)->doAction();
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
FGLayeredInstrument::draw ()
{
  for (int i = 0; i < (int)_layers.size(); i++) {
    glPushMatrix();
    glTranslatef(0.0, 0.0, (i / 100.0) + 0.1);
    _layers[i]->draw();
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
FGLayeredInstrument::addLayer (FGCroppedTexture &texture,
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
    float val = (t->value == 0 ? 0.0 : t->value->getFloatValue());
    if (val < t->min) {
      val = t->min;
    } else if (val > t->max) {
      val = t->max;
    }
    val = val * t->factor + t->offset;

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
    it++;
  }
}

void
FGInstrumentLayer::addTransformation (FGPanelTransformation * transformation)
{
  _transformations.push_back(transformation);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTexturedLayer.
////////////////////////////////////////////////////////////////////////


FGTexturedLayer::FGTexturedLayer (const FGCroppedTexture &texture, int w, int h)
  : FGInstrumentLayer(w, h)
{
  setTexture(texture);
}


FGTexturedLayer::~FGTexturedLayer ()
{
}


void
FGTexturedLayer::draw ()
{
  int w2 = _w / 2;
  int h2 = _h / 2;

  transform();
  glBindTexture(GL_TEXTURE_2D, _texture.getTexture()->getHandle());
  glBegin(GL_POLYGON);

				// From Curt: turn on the panel
				// lights after sundown.
  if ( cur_light_params.sun_angle * RAD_TO_DEG < 95.0 ) {
      glColor4fv( cur_light_params.scene_diffuse );
  } else {
      glColor4f(0.7, 0.2, 0.2, 1.0);
  }


  glTexCoord2f(_texture.getMinX(), _texture.getMinY()); glVertex2f(-w2, -h2);
  glTexCoord2f(_texture.getMaxX(), _texture.getMinY()); glVertex2f(w2, -h2);
  glTexCoord2f(_texture.getMaxX(), _texture.getMaxY()); glVertex2f(w2, h2);
  glTexCoord2f(_texture.getMinX(), _texture.getMaxY()); glVertex2f(-w2, h2);
  glEnd();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextLayer.
////////////////////////////////////////////////////////////////////////

FGTextLayer::FGTextLayer (int w, int h)
  : FGInstrumentLayer(w, h), _pointSize(14.0)
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
FGTextLayer::draw ()
{
  glPushMatrix();
  glColor4fv(_color);
  transform();
  text_renderer.setFont(guiFntHandle);
  text_renderer.setPointSize(_pointSize);
  text_renderer.begin();
  text_renderer.start3f(0, 0, 0);

  _now.stamp();
  if (_now - _then > 100000) {
    recalc_value();
    _then = _now;
  }
  text_renderer.puts((char *)(_value.c_str()));

  text_renderer.end();
  glColor4f(1.0, 1.0, 1.0, 1.0);	// FIXME
  glPopMatrix();
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
  if (_fmt == "") 
    _fmt = "%s";
}

FGTextLayer::Chunk::Chunk (ChunkType type, const SGValue * value,
			   const string &fmt, float mult)
  : _type(type), _fmt(fmt), _mult(mult)
{
  if (_fmt == "") {
    if (type == TEXT_VALUE)
      _fmt = "%s";
    else
      _fmt = "%.2f";
  }
  _value = value;
}

const char *
FGTextLayer::Chunk::getValue () const
{
  switch (_type) {
  case TEXT:
    sprintf(_buf, _fmt.c_str(), _text.c_str());
    return _buf;
  case TEXT_VALUE:
    sprintf(_buf, _fmt.c_str(), _value->getStringValue().c_str());
    break;
  case DOUBLE_VALUE:
    sprintf(_buf, _fmt.c_str(), _value->getFloatValue() * _mult);
    break;
  }
  return _buf;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGSwitchLayer.
////////////////////////////////////////////////////////////////////////

FGSwitchLayer::FGSwitchLayer (int w, int h, const SGValue * value,
			      FGInstrumentLayer * layer1,
			      FGInstrumentLayer * layer2)
  : FGInstrumentLayer(w, h), _value(value), _layer1(layer1), _layer2(layer2)
{
}

FGSwitchLayer::~FGSwitchLayer ()
{
  delete _layer1;
  delete _layer2;
}

void
FGSwitchLayer::draw ()
{
  transform();
  if (_value->getBoolValue()) {
    _layer1->draw();
  } else {
    _layer2->draw();
  }
}


// end of panel.cxx
