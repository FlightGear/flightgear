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
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Objects/texload.h>

#include "hud.hxx"
#include "panel.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextureManager.
////////////////////////////////////////////////////////////////////////

map<const char *,ssgTexture *> FGTextureManager::_textureMap;

ssgTexture *
FGTextureManager::createTexture (const char * relativePath)
{
  ssgTexture *texture;

  texture = _textureMap[relativePath];
  if (texture == 0) {
    FGPath tpath(current_options.get_fg_root());
    tpath.append(relativePath);
    texture = new ssgTexture((char *)tpath.c_str(), false, false);
    _textureMap[relativePath] = texture;
    cerr << "Created texture " << relativePath
	 << " handle=" << texture->getHandle() << endl;
  }

  return texture;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPanel.
////////////////////////////////////////////////////////////////////////

FGPanel * current_panel = NULL;

FGPanel::FGPanel (int x, int y, int w, int h)
  : _mouseDown(false),
    _mouseInstrument(0),
    _x(x), _y(y), _w(w), _h(h)
{
  setVisibility(current_options.get_panel_status());
  _panel_h = (int)(h * 0.5768 + 1);
}

FGPanel::~FGPanel ()
{
  instrument_list_type::iterator current = _instruments.begin();
  instrument_list_type::iterator last = _instruments.end();
  
  for ( ; current != last; ++current) {
    delete *current;
    *current = 0;
  }
}

void
FGPanel::addInstrument (FGPanelInstrument * instrument)
{
  _instruments.push_back(instrument);
}

void
FGPanel::update () const
{
				// Do nothing if the panel isn't visible.
  if (!_visibility)
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
  gluOrtho2D(_x, _x + _w, _y, _y + _h);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

				// Draw the background
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glEnable(GL_ALPHA_TEST);
  glEnable(GL_COLOR_MATERIAL);
  glColor4f(1.0, 1.0, 1.0, 1.0);
  glBindTexture(GL_TEXTURE_2D, _bg->getHandle());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glBegin(GL_POLYGON);
  glTexCoord2f(0.0, 0.0); glVertex3f(_x, _y, 0);
  glTexCoord2f(10.0, 0.0); glVertex3f(_x + _w, _y, 0);
  glTexCoord2f(10.0, 5.0); glVertex3f(_x + _w, _y + _panel_h, 0);
  glTexCoord2f(0.0, 5.0); glVertex3f(_x, _y + _panel_h, 0);
  glEnd();

				// Draw the instruments.
  instrument_list_type::const_iterator current = _instruments.begin();
  instrument_list_type::const_iterator end = _instruments.end();

  for ( ; current != end; current++) {
    FGPanelInstrument * instr = *current;
    glLoadIdentity();
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

void
FGPanel::setVisibility (bool visibility)
{
  _visibility = visibility;
}

bool
FGPanel::getVisibility () const
{
  return _visibility;
}

void
FGPanel::setBackground (ssgTexture * texture)
{
  _bg = texture;
}

bool
FGPanel::doMouseAction (int button, int updown, int x, int y)
{
				// Note a released button and return
  if (updown == 1) {
    _mouseDown = false;
    _mouseInstrument = 0;
    return true;
  }

  x = (int)(((float)x / current_view.get_winWidth()) * _w);
  y = (int)(_h - (((float)y / current_view.get_winHeight()) * _h));

  for (int i = 0; i < _instruments.size(); i++) {
    FGPanelInstrument *inst = _instruments[i];
    int ix = inst->getXPos();
    int iy = inst->getYPos();
    int iw = inst->getWidth() / 2;
    int ih = inst->getHeight() / 2;
    if (x >= ix - iw && x < ix + iw && y >= iy - ih && y < iy + ih) {
//       cout << "Do mouse action for component " << i << '\n';
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
//   cout << "Did not click on an instrument\n";
  return false;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAdjustAction.
////////////////////////////////////////////////////////////////////////

FGAdjustAction::FGAdjustAction (getter_type getter, setter_type setter,
				float increment, float min, float max,
				bool wrap=false)
  : _getter(getter), _setter(setter), _increment(increment),
    _min(min), _max(max), _wrap(wrap)
{
}

FGAdjustAction::~FGAdjustAction ()
{
}

void
FGAdjustAction::doAction ()
{
  float value = (*_getter)();
//   cout << "Do action; value=" << value << '\n';
  value += _increment;
  if (value < _min) {
    value = (_wrap ? _max : _min);
  } else if (value > _max) {
    value = (_wrap ? _min : _max);
  }
//   cout << "New value is " << value << '\n';
  (*_setter)(value);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGSwapAction.
////////////////////////////////////////////////////////////////////////

FGSwapAction::FGSwapAction (getter_type getter1, setter_type setter1,
			    getter_type getter2, setter_type setter2)
  : _getter1(getter1), _setter1(setter1),
    _getter2(getter2), _setter2(setter2)
{
}

FGSwapAction::~FGSwapAction ()
{
}

void
FGSwapAction::doAction ()
{
  float value = (*_getter1)();
  (*_setter1)((*_getter2)());
  (*_setter2)(value);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGToggleAction.
////////////////////////////////////////////////////////////////////////

FGToggleAction::FGToggleAction (getter_type getter, setter_type setter)
  : _getter(getter), _setter(setter)
{
}

FGToggleAction::~FGToggleAction ()
{
}

void
FGToggleAction::doAction ()
{
  (*_setter)(!((*_getter)()));
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
  action_list_type::iterator it = _actions.begin();
  action_list_type::iterator last = _actions.end();
  for ( ; it != last; it++) {
    delete it->action;
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
FGPanelInstrument::addAction (int button, int x, int y, int w, int h,
			      FGPanelAction * action)
{
  FGPanelInstrument::inst_action act;
  act.button = button;
  act.x = x;
  act.y = y;
  act.w = w;
  act.h = h;
  act.action = action;
  _actions.push_back(act);
}

				// Coordinates relative to centre.
bool
FGPanelInstrument::doMouseAction (int button, int x, int y)
{
  action_list_type::iterator it = _actions.begin();
  action_list_type::iterator last = _actions.end();
//   cout << "Mouse action at " << x << ',' << y << '\n';
  for ( ; it != last; it++) {
//     cout << "Trying action at " << it->x << ',' << it->y << ','
// 	 << it->w <<',' << it->h << '\n';
    if (button == it->button &&
	x >= it->x && x < it->x + it->w && y >= it->y && y < it->y + it->h) {
      it->action->doAction();
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
  // FIXME: free layers
}

void
FGLayeredInstrument::draw () const
{
  for (int i = 0; i < _layers.size(); i++) {
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
  _layers.push_back(layer);
  return n;
}

int
FGLayeredInstrument::addLayer (ssgTexture * texture,
			       int w = -1, int h = -1,
			       float texX1 = 0.0, float texY1 = 0.0,
			       float texX2 = 1.0, float texY2 = 1.0)
{
  if (w == -1)
    w = _w;
  if (h == -1)
    h = _h;
  FGTexturedLayer * layer = new FGTexturedLayer(texture, w, h);
  layer->setTextureCoords(texX1, texY1, texX2, texY2);
  return addLayer(layer);
}

void
FGLayeredInstrument::addTransformation (FGInstrumentLayer::transform_type type,
					FGInstrumentLayer::transform_func func,
					float min, float max,
					float factor, float offset)
{
  int layer = _layers.size() - 1;
  _layers[layer]->addTransformation(type, func, min, max, factor, offset);
}

void
FGLayeredInstrument::addTransformation (FGInstrumentLayer::transform_type type,
					float offset)
{
  addTransformation(type, 0, 0.0, 0.0, 1.0, offset);
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
  transformation_list::iterator it = _transformations.begin();
  transformation_list::iterator end = _transformations.end();
  while (it != end) {
    delete *it;
    it++;
  }
}

void
FGInstrumentLayer::transform () const
{
  transformation_list::const_iterator it = _transformations.begin();
  transformation_list::const_iterator last = _transformations.end();
  while (it != last) {
    transformation *t = *it;
    float value = (t->func == 0 ? 0.0 : (*(t->func))());
    if (value < t->min) {
      value = t->min;
    } else if (value > t->max) {
      value = t->max;
    }
    value = value * t->factor + t->offset;

    switch (t->type) {
    case XSHIFT:
      glTranslatef(value, 0.0, 0.0);
      break;
    case YSHIFT:
      glTranslatef(0.0, value, 0.0);
      break;
    case ROTATION:
      glRotatef(-value, 0.0, 0.0, 1.0);
      break;
    }
    it++;
  }
}

void
FGInstrumentLayer::addTransformation (transform_type type,
				      transform_func func,
				      float min, float max,
				      float factor, float offset)
{
  transformation *t = new transformation;
  t->type = type;
  t->func = func;
  t->min = min;
  t->max = max;
  t->factor = factor;
  t->offset = offset;
  _transformations.push_back(t);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTexturedLayer.
////////////////////////////////////////////////////////////////////////

FGTexturedLayer::FGTexturedLayer (ssgTexture * texture, int w, int h,
				  float texX1 = 0.0, float texY1 = 0.0,
				  float texX2 = 1.0, float texY2 = 1.0)
  : FGInstrumentLayer(w, h),
    _texX1(texX1), _texY1(texY1), _texX2(texX2), _texY2(texY2)
{
  setTexture(texture);
}

FGTexturedLayer::~FGTexturedLayer ()
{
}

void
FGTexturedLayer::draw () const
{
  int w2 = _w / 2;
  int h2 = _h / 2;

  transform();
  glBindTexture(GL_TEXTURE_2D, _texture->getHandle());
  glBegin(GL_POLYGON);

  glTexCoord2f(_texX1, _texY1); glVertex2f(-w2, -h2);
  glTexCoord2f(_texX2, _texY1); glVertex2f(w2, -h2);
  glTexCoord2f(_texX2, _texY2); glVertex2f(w2, h2);
  glTexCoord2f(_texX1, _texY2); glVertex2f(-w2, h2);
  glEnd();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextLayer.
////////////////////////////////////////////////////////////////////////

FGTextLayer::FGTextLayer (int w, int h)
  : FGInstrumentLayer(w, h)
{
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
FGTextLayer::draw () const
{
  glPushMatrix();
  glColor4fv(_color);
  transform();
  _renderer.setFont(guiFntHandle);
  _renderer.setPointSize(14);
  _renderer.begin();
  _renderer.start3f(0, 0, 0);

				// Render each of the chunks.
  chunk_list::const_iterator it = _chunks.begin();
  chunk_list::const_iterator last = _chunks.end();
  for ( ; it != last; it++) {
    _renderer.puts((*it)->getValue());
  }

  _renderer.end();
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
FGTextLayer::setPointSize (const float size)
{
  _renderer.setPointSize(size);
}

void
FGTextLayer::setFont(fntFont * font)
{
  _renderer.setFont(font);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextLayer::Chunk.
////////////////////////////////////////////////////////////////////////

FGTextLayer::Chunk::Chunk (char * text, char * fmt = "%s")
  : _type(FGTextLayer::TEXT), _fmt(fmt)
{
  _value._text = text;
}

FGTextLayer::Chunk::Chunk (text_func func, char * fmt = "%s")
  : _type(FGTextLayer::TEXT_FUNC), _fmt(fmt)
{
  _value._tfunc = func;
}

FGTextLayer::Chunk::Chunk (double_func func, char * fmt = "%.2f",
			   float mult = 1.0)
  : _type(FGTextLayer::DOUBLE_FUNC), _fmt(fmt), _mult(mult)
{
  _value._dfunc = func;
}

char *
FGTextLayer::Chunk::getValue () const
{
  switch (_type) {
  case TEXT:
    sprintf(_buf, _fmt, _value._text);
    return _buf;
  case TEXT_FUNC:
    sprintf(_buf, _fmt, (*(_value._tfunc))());
    break;
  case DOUBLE_FUNC:
    sprintf(_buf, _fmt, (*(_value._dfunc))() * _mult);
    break;
  }
  return _buf;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGSwitchLayer.
////////////////////////////////////////////////////////////////////////

FGSwitchLayer::FGSwitchLayer (int w, int h, switch_func func,
			      FGInstrumentLayer * layer1,
			      FGInstrumentLayer * layer2)
  : FGInstrumentLayer(w, h), _func(func), _layer1(layer1), _layer2(layer2)
{
}

FGSwitchLayer::~FGSwitchLayer ()
{
  delete _layer1;
  delete _layer2;
}

void
FGSwitchLayer::draw () const
{
  transform();
  if ((*_func)()) {
    _layer1->draw();
  } else {
    _layer2->draw();
  }
}


// end of panel.cxx
