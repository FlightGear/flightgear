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
#include <simgear/misc/sg_path.hxx>

#include <Main/globals.hxx>
#include <Main/fg_props.hxx>
#include <Main/viewmgr.hxx>
#include <Objects/texload.h>
#include <Time/light.hxx>

#include "hud.hxx"
#include "panel.hxx"

#define WIN_X 0
#define WIN_Y 0
#define WIN_W 1024
#define WIN_H 768

#ifdef NONE
#pragma warn A sloppy coder has defined NONE as a macro!!!
#undef NONE
#endif



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
// Global functions.
////////////////////////////////////////////////////////////////////////

bool
fgPanelVisible ()
{
  return ((current_panel != 0) &&
	  (current_panel->getVisibility()) &&
	  (globals->get_viewmgr()->get_current() == 0) &&
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
    SG_LOG( SG_COCKPIT, SG_DEBUG,
            "Texture " << relativePath << " does not yet exist" );
    SGPath tpath(globals->get_fg_root());
    tpath.append(relativePath);
    texture = new ssgTexture((char *)tpath.c_str(), false, false);
    _textureMap[relativePath] = texture;
    if (_textureMap[relativePath] == 0) 
      SG_LOG( SG_COCKPIT, SG_ALERT, "Texture *still* doesn't exist" );
    SG_LOG( SG_COCKPIT, SG_DEBUG, "Created texture " << relativePath
            << " handle=" << texture->getHandle() );
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
FGPanel::FGPanel ()
  : _mouseDown(false),
    _mouseInstrument(0),
    _width(WIN_W), _height(int(WIN_H * 0.5768 + 1)),
    _x_offset(0), _y_offset(0), _view_height(int(WIN_H * 0.4232)),
    _bound(false),
    _xsize_node(fgGetNode("/sim/startup/xsize", true)),
    _ysize_node(fgGetNode("/sim/startup/ysize", true))
{
  setVisibility(fgPanelVisible());
}


/**
 * Destructor.
 */
FGPanel::~FGPanel ()
{
  if (_bound)
    unbind();
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
 * Initialize the panel.
 */
void
FGPanel::init ()
{
  // NO-OP
}


/**
 * Bind panel properties.
 */
void
FGPanel::bind ()
{
  fgTie("/sim/panel/visibility", &_visibility);
  fgSetArchivable("/sim/panel/visibility");
  fgTie("/sim/panel/x-offset", &_x_offset);
  fgSetArchivable("/sim/panel/x-offset");
  fgTie("/sim/panel/y-offset", &_y_offset);
  fgSetArchivable("/sim/panel/y-offset");
  _bound = true;
}


/**
 * Unbind panel properties.
 */
void
FGPanel::unbind ()
{
  fgUntie("/sim/panel/visibility");
  fgUntie("/sim/panel/x-offset");
  fgUntie("/sim/panel/y-offset");
  _bound = false;
}


/**
 * Update the panel.
 */
void
FGPanel::update ()
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
    float aspect_adjust = get_aspect_adjust(_xsize_node->getIntValue(),
                                            _ysize_node->getIntValue());
    if (aspect_adjust <1.0)
        update(WIN_X, int(WIN_W * aspect_adjust), WIN_Y, WIN_H);
    else
        update(WIN_X, WIN_W, WIN_Y, int(WIN_H / aspect_adjust));
}


void
FGPanel::update (GLfloat winx, GLfloat winw, GLfloat winy, GLfloat winh)
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(winx, winx + winw, winy, winy + winh); /* right side up */
  // gluOrtho2D(winx + winw, winx, winy + winh, winy); /* up side down */

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
  if ( cur_light_params.sun_angle * SGD_RADIANS_TO_DEGREES < 95.0 ) {
      glColor4fv( cur_light_params.scene_diffuse );
  } else {
      glColor4f(0.7, 0.2, 0.2, 1.0);
  }
  glBindTexture(GL_TEXTURE_2D, _bg->getHandle());
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glBegin(GL_POLYGON);
  glTexCoord2f(0.0, 0.0); glVertex3f(WIN_X, WIN_Y, 0);
  glTexCoord2f(1.0, 0.0); glVertex3f(WIN_X + _width, WIN_Y, 0);
  glTexCoord2f(1.0, 1.0); glVertex3f(WIN_X + _width, WIN_Y + _height, 0);
  glTexCoord2f(0.0, 1.0); glVertex3f(WIN_X, WIN_Y + _height, 0);
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
  if (offset <= 0 && offset >= -_width + WIN_W)
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
				// FIXME: this same code appears in update()
  int xsize = _xsize_node->getIntValue();
  int ysize = _ysize_node->getIntValue();
  float aspect_adjust = get_aspect_adjust(xsize, ysize);

				// Note a released button and return
  // cerr << "Doing mouse action\n";
  if (updown == 1) {
    _mouseDown = false;
    _mouseInstrument = 0;
    return true;
  }

				// Scale for the real window size.
  if (aspect_adjust < 1.0) {
    x = int(((float)x / xsize) * WIN_W * aspect_adjust);
    y = int(WIN_H - ((float(y) / ysize) * WIN_H));
  } else {
    x = int(((float)x / xsize) * WIN_W);
    y = int((WIN_H - ((float(y) / ysize) * WIN_H)) / aspect_adjust);
  }

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
  for (unsigned int i = 0; i < _bindings.size(); i++)
    delete _bindings[i];
}

FGPanelAction::~FGPanelAction ()
{
}

void
FGPanelAction::addBinding (FGBinding * binding)
{
  _bindings.push_back(binding);
}

void
FGPanelAction::doAction ()
{
  if (test()) {
    int nBindings = _bindings.size();
    for (int i = 0; i < nBindings; i++) {
      _bindings[i]->fire();
    }
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
  if (test()) {
    action_list_type::iterator it = _actions.begin();
    action_list_type::iterator last = _actions.end();
    for ( ; it != last; it++) {
      if ((*it)->inArea(button, x, y)) {
	(*it)->doAction();
	return true;
      }
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
  if (test()) {
    for (int i = 0; i < (int)_layers.size(); i++) {
      glPushMatrix();
      glTranslatef(0.0, 0.0, (i / 100.0) + 0.1);
      _layers[i]->draw();
      glPopMatrix();
    }
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
    if (t->test()) {
      float val = (t->node == 0 ? 0.0 : t->node->getFloatValue());
      if (val < t->min) {
	val = t->min;
      } else if (val > t->max) {
	val = t->max;
      }
      if(t->table==0) {
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
FGGroupLayer::draw ()
{
  if (test()) {
    int nLayers = _layers.size();
    for (int i = 0; i < nLayers; i++)
      _layers[i]->draw();
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
  if (test()) {
    int w2 = _w / 2;
    int h2 = _h / 2;
    
    transform();
    glBindTexture(GL_TEXTURE_2D, _texture.getTexture()->getHandle());
    glBegin(GL_POLYGON);
    
				// From Curt: turn on the panel
				// lights after sundown.
    if ( cur_light_params.sun_angle * SGD_RADIANS_TO_DEGREES < 95.0 ) {
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
  if (test()) {
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

FGTextLayer::Chunk::Chunk (ChunkType type, const SGPropertyNode * node,
			   const string &fmt, float mult)
  : _type(type), _fmt(fmt), _mult(mult)
{
  if (_fmt == "") {
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
    switch (_type) {
    case TEXT:
      sprintf(_buf, _fmt.c_str(), _text.c_str());
      return _buf;
    case TEXT_VALUE:
      sprintf(_buf, _fmt.c_str(), _node->getStringValue().c_str());
      break;
    case DOUBLE_VALUE:
      sprintf(_buf, _fmt.c_str(), _node->getFloatValue() * _mult);
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

FGSwitchLayer::FGSwitchLayer (int w, int h, const SGPropertyNode * node,
			      FGInstrumentLayer * layer1,
			      FGInstrumentLayer * layer2)
  : FGInstrumentLayer(w, h), _node(node), _layer1(layer1), _layer2(layer2)
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
  if (test()) {
    transform();
    if (_node->getBoolValue()) {
      _layer1->draw();
    } else {
      _layer2->draw();
    }
  }
}


// end of panel.cxx
