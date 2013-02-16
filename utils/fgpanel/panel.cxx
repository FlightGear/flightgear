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
//  $Id: panel.cxx,v 1.44 2006/09/05 20:28:48 curt Exp $

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <stdio.h>	// sprintf
#include <string.h>

#include <simgear/compiler.h>

#if defined (SG_MAC)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <plib/fnt.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include "panel.hxx"
#include "ApplicationProperties.hxx"
////////////////////////////////////////////////////////////////////////
// Local functions.
////////////////////////////////////////////////////////////////////////

class FGDummyTextureLoader : public FGTextureLoaderInterface {
public:
  virtual GLuint loadTexture( const string & filename );
};

GLuint FGDummyTextureLoader::loadTexture( const string & filename )
{
  GLuint _texture = 0;
  glGenTextures( 1, &_texture );
  glBindTexture( GL_TEXTURE_2D, _texture );

//  glTexEnvi       ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ) ;
//  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) ;
//  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR ) ;
//  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ) ;
//  glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT ) ;

  GLubyte image[ 2 * 2 * 3 ] ;

  /* Red and white chequerboard */
  image [ 0 ] = 255 ; image [ 1 ] =  0  ; image [ 2 ] =  0  ;
  image [ 3 ] = 255 ; image [ 4 ] = 255 ; image [ 5 ] = 255 ;
  image [ 6 ] = 255 ; image [ 7 ] = 255 ; image [ 8 ] = 255 ;
  image [ 9 ] = 255 ; image [ 10] =  0  ; image [ 11] =  0  ;

  glTexImage2D(GL_TEXTURE_2D,0, GL_RGB, 2, 2, 0,
        GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*) image);

  return _texture;
}

////////////////////////////////////////////////////////////////////////
// Implementation of FGCropped Texture.
////////////////////////////////////////////////////////////////////////

GLuint FGCroppedTexture::current_bound_texture = 0;
map<string,GLuint> FGCroppedTexture::cache;
map<string,FGTextureLoaderInterface*> FGCroppedTexture::textureLoader;
static FGDummyTextureLoader dummyTextureLoader;

FGCroppedTexture::FGCroppedTexture (const string &path,
				    float minX, float minY,
				    float maxX, float maxY)
  : _path(path),
    _minX(minX), _minY(minY), _maxX(maxX), _maxY(maxY), _texture(0)
{
}

FGCroppedTexture::~FGCroppedTexture ()
{
}

void FGCroppedTexture::bind( bool doGLBind )
{
  if( _texture == 0 ) {
    SG_LOG( SG_COCKPIT, SG_DEBUG, "First bind of texture " << _path );
    if( cache.count(_path) > 0 ) {
      _texture = cache[_path];
      SG_LOG( SG_COCKPIT, SG_DEBUG, "Using texture " << _path << " from cache (#" << _texture << ")" );
    } else {
      SGPath tpath = ApplicationProperties::GetRootPath(_path.c_str());
      string extension = tpath.extension();
      FGTextureLoaderInterface * loader = &dummyTextureLoader;
      if( textureLoader.count( extension ) == 0 ) {
        SG_LOG( SG_COCKPIT, SG_ALERT, "Can't handle textures of type " << extension );
      } else {
        loader = textureLoader[extension];
      }

      _texture = loader->loadTexture( tpath.c_str() );
      SG_LOG( SG_COCKPIT, SG_DEBUG, "Texture " << tpath.c_str() << " loaded from file as #" << _texture );
      
      cache[_path] = _texture;
    }
  }

  if( !doGLBind || current_bound_texture == _texture )
    return;

  glBindTexture( GL_TEXTURE_2D, _texture );
  current_bound_texture = _texture;
}


////////////////////////////////////////////////////////////////////////
// Implementation of FGPanel.
////////////////////////////////////////////////////////////////////////

/**
 * Constructor.
 */
FGPanel::FGPanel ( SGPropertyNode_ptr root)
  : _root(root),
    _flipx(root->getNode("/sim/panel/flip-x", true)),
    _rotate(root->getNode("/sim/panel/rotate-deg", true)),
    _bg_width(1.0), _bg_height(1.0),
    initDisplayList(0)
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


/**
 * Initialize the panel.
 */
void
FGPanel::init ()
{
}


/**
 * Bind panel properties.
 */
void
FGPanel::bind ()
{
}


/**
 * Unbind panel properties.
 */
void
FGPanel::unbind ()
{
}

GLuint FGPanel::getInitDisplayList()
{
  if( initDisplayList != 0 ) return initDisplayList;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if ( _flipx->getBoolValue() ) {
    gluOrtho2D( _width, 0, _height, 0 ); /* up side down */
  } else {
    gluOrtho2D( 0, _width, 0, _height ); /* right side up */
  }

  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClear( GL_COLOR_BUFFER_BIT);

  // save some state
  glPushAttrib( GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT
                | GL_TEXTURE_BIT | GL_PIXEL_MODE_BIT | GL_CULL_FACE 
                | GL_DEPTH_BUFFER_BIT );

  // Draw the background
  glEnable(GL_TEXTURE_2D);

  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_ALPHA_TEST);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glDisable(GL_DEPTH_TEST);

  if (_bg != NULL) {
    _bg->bind();
//    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex2f(0, 0);
    glTexCoord2f(_bg_width, 0.0); glVertex2f(_width, 0);
    glTexCoord2f(_bg_width, _bg_height); glVertex2f(_width, _height);
    glTexCoord2f(0.0, _bg_height); glVertex2f(0, _height);
    glEnd();
  } else if( _mbg[0] != NULL ) {
    for (int i = 0; i < 4; i ++) {
      // top row of textures...(1,3,5,7)
      _mbg[i*2]->bind();
//      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glBegin(GL_QUADS);
      glTexCoord2f(0.0, 0.0); glVertex2f(i*_width/4,     _height/2);
      glTexCoord2f(1.0, 0.0); glVertex2f((i+1)*_width/4, _height/2);
      glTexCoord2f(1.0, 1.0); glVertex2f((i+1)*_width/4, _height);
      glTexCoord2f(0.0, 1.0); glVertex2f(i*_width/4,     _height);
      glEnd();
      // bottom row of textures...(2,4,6,8)
      _mbg[i*2+1]->bind();
//      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glBegin(GL_QUADS);
      glTexCoord2f(0.0, 0.0); glVertex2f( i*_width/4,     0);
      glTexCoord2f(1.0, 0.0); glVertex2f( (i+1)*_width/4, 0);
      glTexCoord2f(1.0, 1.0); glVertex2f( (i+1)*_width/4, _height/2);
      glTexCoord2f(0.0, 1.0); glVertex2f( i*_width/4,     _height/2);
      glEnd();
    }
  } else {
    float c[4];
    glGetFloatv( GL_CURRENT_COLOR, c );
    glColor4f( 0.0, 0.0, 0.0, 1.0 );
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(_width, 0);
    glVertex2f(_width, _height);
    glVertex2f(0, _height);
    glEnd();
    glColor4fv( c );
  }


  return initDisplayList;  
}

void
FGPanel::update (double dt)
{
  /*glCallList*/(getInitDisplayList());

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
    instr->draw();

    glPopMatrix();
  }

  glDisable(GL_CLIP_PLANE0);
  glDisable(GL_CLIP_PLANE1);
  glDisable(GL_CLIP_PLANE2);
  glDisable(GL_CLIP_PLANE3);

  // restore some original state
  glPopAttrib();
}

#if 0
/**
 * Update the panel.
 */
void
FGPanel::update (double dt)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if ( _flipx->getBoolValue() ) {
    gluOrtho2D( _width, 0, _height, 0 ); /* up side down */
  } else {
    gluOrtho2D( 0, _width, 0, _height ); /* right side up */
  }

  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  draw();
}

void FGPanel::draw()
{
  glClear( GL_COLOR_BUFFER_BIT);

  // save some state
  glPushAttrib( GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT
                | GL_TEXTURE_BIT | GL_PIXEL_MODE_BIT | GL_CULL_FACE 
                | GL_DEPTH_BUFFER_BIT );

  // Draw the background
  glEnable(GL_TEXTURE_2D);

  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_ALPHA_TEST);
  glEnable(GL_COLOR_MATERIAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glDisable(GL_DEPTH_TEST);

  if (_bg != NULL) {
    _bg->bind();
//    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex2f(0, 0);
    glTexCoord2f(_bg_width, 0.0); glVertex2f(_width, 0);
    glTexCoord2f(_bg_width, _bg_height); glVertex2f(_width, _height);
    glTexCoord2f(0.0, _bg_height); glVertex2f(0, _height);
    glEnd();
  } else if( _mbg[0] != NULL ) {
    for (int i = 0; i < 4; i ++) {
      // top row of textures...(1,3,5,7)
      _mbg[i*2]->bind();
//      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glBegin(GL_QUADS);
      glTexCoord2f(0.0, 0.0); glVertex2f(i*_width/4,     _height/2);
      glTexCoord2f(1.0, 0.0); glVertex2f((i+1)*_width/4, _height/2);
      glTexCoord2f(1.0, 1.0); glVertex2f((i+1)*_width/4, _height);
      glTexCoord2f(0.0, 1.0); glVertex2f(i*_width/4,     _height);
      glEnd();
      // bottom row of textures...(2,4,6,8)
      _mbg[i*2+1]->bind();
//      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glBegin(GL_QUADS);
      glTexCoord2f(0.0, 0.0); glVertex2f( i*_width/4,     0);
      glTexCoord2f(1.0, 0.0); glVertex2f( (i+1)*_width/4, 0);
      glTexCoord2f(1.0, 1.0); glVertex2f( (i+1)*_width/4, _height/2);
      glTexCoord2f(0.0, 1.0); glVertex2f( i*_width/4,     _height/2);
      glEnd();
    }
  } else {
    float c[4];
    glGetFloatv( GL_CURRENT_COLOR, c );
    glColor4f( 0.0, 0.0, 0.0, 1.0 );
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(_width, 0);
    glVertex2f(_width, _height);
    glVertex2f(0, _height);
    glEnd();
    glColor4fv( c );
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
    instr->draw();

    glPopMatrix();
  }

  glDisable(GL_CLIP_PLANE0);
  glDisable(GL_CLIP_PLANE1);
  glDisable(GL_CLIP_PLANE2);
  glDisable(GL_CLIP_PLANE3);

  // restore some original state
  glPopAttrib();
}
#endif

/**
 * Set the panel's background texture.
 */
void
FGPanel::setBackground (FGCroppedTexture_ptr texture)
{
  _bg = texture;
}

/**
 * Set the panel's multiple background textures.
 */
void
FGPanel::setMultiBackground (FGCroppedTexture_ptr texture, int idx)
{
  _bg = 0;
  _mbg[idx] = texture;
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
  if (!test())
    return;
  
  for (int i = 0; i < (int)_layers.size(); i++) {
    glPushMatrix();
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
FGLayeredInstrument::addLayer (FGCroppedTexture_ptr texture, int w, int h)
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
FGGroupLayer::draw ()
{
  if (test()) {
    transform();
    int nLayers = _layers.size();
    for (int i = 0; i < nLayers; i++)
      _layers[i]->draw( );
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


FGTexturedLayer::FGTexturedLayer (FGCroppedTexture_ptr texture, int w, int h)
  : FGInstrumentLayer(w, h),
    _emissive(false),
    displayList(0)
{
  setTexture(texture);
}


FGTexturedLayer::~FGTexturedLayer ()
{
}

GLuint
FGTexturedLayer::getDisplayList()
{
  if( displayList != 0 )
    return displayList;

  int w2 = _w / 2;
  int h2 = _h / 2;

  _texture->bind( false );
  displayList = glGenLists(1);
  glNewList(displayList,GL_COMPILE_AND_EXECUTE);
    glBindTexture( GL_TEXTURE_2D, _texture->getTexture() );
    glBegin(GL_QUADS);
      glTexCoord2f(_texture->getMinX(), _texture->getMinY()); glVertex2f(-w2, -h2);
      glTexCoord2f(_texture->getMaxX(), _texture->getMinY()); glVertex2f(w2, -h2);
      glTexCoord2f(_texture->getMaxX(), _texture->getMaxY()); glVertex2f(w2, h2);
      glTexCoord2f(_texture->getMinX(), _texture->getMaxY()); glVertex2f(-w2, h2);
    glEnd();
  glEndList();

  return displayList;
}

void
FGTexturedLayer::draw ( )
{
  if (test()) {
    transform();
    glCallList(getDisplayList());
  }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextLayer.
////////////////////////////////////////////////////////////////////////

fntRenderer FGTextLayer::text_renderer;

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
FGTextLayer::draw ()
{
  if (test()) {
    float c[4];
    glGetFloatv( GL_CURRENT_COLOR, c );
    glColor4fv(_color);
    transform();

    text_renderer.setFont(ApplicationProperties::fontCache.getTexFont(_font_name.c_str()));
    if (!text_renderer.getFont())
    {
        SG_LOG( SG_COCKPIT, SG_ALERT, "Missing font file: " << _font_name );
        return;
    }

    text_renderer.setPointSize(_pointSize);
    text_renderer.begin();
    text_renderer.start3f(0, 0, 0);

    _now.stamp();
    long diff = (_now - _then).toUSecs();

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
    glColor4fv( c );
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
}


void
FGTextLayer::setFont(fntFont * font)
{
  FGTextLayer::text_renderer.setFont(font);
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
FGSwitchLayer::draw ()
{
  if (test()) {
    transform();
    int nLayers = _layers.size();
    for (int i = 0; i < nLayers; i++) {
      if (_layers[i]->test()) {
          _layers[i]->draw();
          return;
      }
    }
  }
}


// end of panel.cxx
