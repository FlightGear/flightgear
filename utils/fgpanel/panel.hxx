//  panel.hxx - generic support classes for a 2D panel.
//
//  Written by David Megginson, started January 2000.
//  Adopted for standalone fgpanel application by Torsten Dreyer, August 2009
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

#ifndef __PANEL_HXX
#define __PANEL_HXX

#ifndef __cplusplus
# error This library requires C++
#endif


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/fnt.h>

#include <simgear/props/propsfwd.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/sg_inlines.h>
#include "FGTextureLoaderInterface.hxx"

class FGPanelInstrument;
using namespace std;

////////////////////////////////////////////////////////////////////////
// Texture management.
////////////////////////////////////////////////////////////////////////

class FGCroppedTexture;
typedef SGSharedPtr<FGCroppedTexture> FGCroppedTexture_ptr;
/**
 * Cropped texture (should migrate out into FGFS).
 *
 * This structure wraps an SSG texture with cropping information.
 */
class FGCroppedTexture : public SGReferenced
{
public:
  FGCroppedTexture (const string &path,
		  float _minX = 0.0, float _minY = 0.0,
		  float _maxX = 1.0, float _maxY = 1.0);

  virtual ~FGCroppedTexture ();

  virtual void setPath (const string &path) { _path = path; }

  virtual const string &getPath () const { return _path; }

  virtual void setCrop (float minX, float minY, float maxX, float maxY) {
    _minX = minX; _minY = minY; _maxX = maxX; _maxY = maxY;
  }

  static void registerTextureLoader( const string & extension, FGTextureLoaderInterface * loader ) {
    if( textureLoader.count( extension ) == 0 )
      textureLoader[extension] = loader;
  }

  virtual float getMinX () const { return _minX; }
  virtual float getMinY () const { return _minY; }
  virtual float getMaxX () const { return _maxX; }
  virtual float getMaxY () const { return _maxY; }
  GLuint getTexture() const { return _texture; }

  virtual void bind( bool doGLBind = true );

private:
  string _path;
  float _minX, _minY, _maxX, _maxY;

  GLuint _texture;
  static GLuint current_bound_texture;
  static map<string,GLuint> cache;
  static map<string,FGTextureLoaderInterface*> textureLoader;
};



////////////////////////////////////////////////////////////////////////
// Top-level panel.
////////////////////////////////////////////////////////////////////////


/**
 * Instrument panel class.
 *
 * The panel is a container that has a background texture and holds
 * zero or more instruments.  The panel will order the instruments to
 * redraw themselves when necessary, and will pass mouse clicks on to
 * the appropriate instruments for processing.
 */
class FGPanel : public SGSubsystem
{
public:

  FGPanel ( SGPropertyNode_ptr root );
  virtual ~FGPanel ();

				// Update the panel (every frame).
  virtual void init ();
  virtual void bind ();
  virtual void unbind ();
//  virtual void draw ();
  virtual void update (double dt);

				// transfer pointer ownership!!!
  virtual void addInstrument (FGPanelInstrument * instrument);

				// Background texture.
  virtual void setBackground (FGCroppedTexture_ptr texture);
  inline void setBackgroundWidth( double d ) {
    _bg_width = d;
  }

  inline void setBackgroundHeight( double d ) {
    _bg_height = d;
  }

				// Background multiple textures.
  virtual void setMultiBackground (FGCroppedTexture_ptr texture , int idx);

				// Full width of panel.
  virtual void setWidth (int width) { _width = width; }
  virtual int getWidth () const { return _width; }

				// Full height of panel.
  virtual void setHeight (int height) { _height = height; }
  virtual int getHeight () const { return _height; }

private:

  typedef vector<FGPanelInstrument *> instrument_list_type;
  int _width;
  int _height;

  SGPropertyNode_ptr _root;
  SGPropertyNode_ptr _flipx;
  SGPropertyNode_ptr _rotate;

  FGCroppedTexture_ptr _bg;
  double _bg_width;
  double _bg_height;
  FGCroppedTexture_ptr _mbg[8];
				// List of instruments in panel.
  instrument_list_type _instruments;

  GLuint initDisplayList;

  GLuint getInitDisplayList();
};



////////////////////////////////////////////////////////////////////////
// Transformations.
////////////////////////////////////////////////////////////////////////


/**
 * A transformation for a layer.
 */
class FGPanelTransformation : public SGConditional
{
public:

  enum Type {
    XSHIFT,
    YSHIFT,
    ROTATION
  };

  FGPanelTransformation ();
  virtual ~FGPanelTransformation ();

  Type type;
  SGConstPropertyNode_ptr node;
  float min;
  float max;
  bool has_mod;
  float mod;
  float factor;
  float offset;
  SGInterpTable * table;
};




////////////////////////////////////////////////////////////////////////
// Layers
////////////////////////////////////////////////////////////////////////


/**
 * A single layer of a multi-layered instrument.
 *
 * Each layer can be subject to a series of transformations based
 * on current FGFS instrument readings: for example, a texture
 * representing a needle can rotate to show the airspeed.
 */
class FGInstrumentLayer : public SGConditional
{
public:

  FGInstrumentLayer (int w = -1, int h = -1);
  virtual ~FGInstrumentLayer ();

  virtual void draw () = 0;
  virtual void transform () const;

  virtual int getWidth () const { return _w; }
  virtual int getHeight () const { return _h; }
  virtual void setWidth (int w) { _w = w; }
  virtual void setHeight (int h) { _h = h; }

				// Transfer pointer ownership!!
				// DEPRECATED
  virtual void addTransformation (FGPanelTransformation * transformation);

protected:
  int _w, _h;

  typedef vector<FGPanelTransformation *> transformation_list;
  transformation_list _transformations;
};



////////////////////////////////////////////////////////////////////////
// Instruments.
////////////////////////////////////////////////////////////////////////


/**
 * Abstract base class for a panel instrument.
 *
 * A panel instrument consists of zero or more actions, associated
 * with mouse clicks in rectangular areas.  Currently, the only
 * concrete class derived from this is FGLayeredInstrument, but others
 * may show up in the future (some complex instruments could be 
 * entirely hand-coded, for example).
 */
class FGPanelInstrument : public SGConditional
{
public:
  FGPanelInstrument ();
  FGPanelInstrument (int x, int y, int w, int h);
  virtual ~FGPanelInstrument ();

  virtual void draw () = 0;

  virtual void setPosition(int x, int y);
  virtual void setSize(int w, int h);

  virtual int getXPos () const;
  virtual int getYPos () const;
  virtual int getWidth () const;
  virtual int getHeight () const;

protected:
  int _x, _y, _w, _h;
};


/**
 * An instrument constructed of multiple layers.
 *
 * Each individual layer can be rotated or shifted to correspond
 * to internal FGFS instrument readings.
 */
class FGLayeredInstrument : public FGPanelInstrument
{
public:
  FGLayeredInstrument (int x, int y, int w, int h);
  virtual ~FGLayeredInstrument ();

  virtual void draw ();

				// Transfer pointer ownership!!
  virtual int addLayer (FGInstrumentLayer *layer);
  virtual int addLayer (FGCroppedTexture_ptr texture, int w = -1, int h = -1);

				// Transfer pointer ownership!!
  virtual void addTransformation (FGPanelTransformation * transformation);

protected:
  typedef vector<FGInstrumentLayer *> layer_list;
  layer_list _layers;
};


/**
 * An instrument layer containing a group of sublayers.
 *
 * This class is useful for gathering together a group of related
 * layers, either to hold in an external file or to work under
 * the same condition.
 */
class FGGroupLayer : public FGInstrumentLayer
{
public:
  FGGroupLayer ();
  virtual ~FGGroupLayer ();
  virtual void draw ();
				// transfer pointer ownership
  virtual void addLayer (FGInstrumentLayer * layer);
protected:
  vector<FGInstrumentLayer *> _layers;
};


/**
 * A textured layer of an instrument.
 *
 * This is a layer holding a single texture.  Normally, the texture's
 * backgound should be transparent so that lower layers and the panel
 * background can show through.
 */
class FGTexturedLayer : public FGInstrumentLayer
{
public:
  FGTexturedLayer (int w = -1, int h = -1) : FGInstrumentLayer(w, h) {}
  FGTexturedLayer (FGCroppedTexture_ptr texture, int w = -1, int h = -1);
  virtual ~FGTexturedLayer ();

  virtual void draw ();

  virtual void setTexture (FGCroppedTexture_ptr texture) {
    _texture = texture;
  }
  FGCroppedTexture_ptr getTexture() { return _texture; }

  void setEmissive(bool e) { _emissive = e; }

private:
  GLuint getDisplayList();

  FGCroppedTexture_ptr _texture;
  bool _emissive;
  GLuint displayList;
};


/**
 * A text layer of an instrument.
 *
 * This is a layer holding a string of static and/or generated text.
 * It is useful for instruments that have text displays, such as
 * a chronometer, GPS, or NavCom radio.
 */
class FGTextLayer : public FGInstrumentLayer
{
public:
  enum ChunkType {
    TEXT,
    TEXT_VALUE,
    DOUBLE_VALUE
  };

  class Chunk : public SGConditional
  {
  public:
    Chunk (const string &text, const string &fmt = "%s");
    Chunk (ChunkType type, const SGPropertyNode * node,
	   const string &fmt = "", float mult = 1.0, float offs = 0.0,
           bool truncation = false);

    const char * getValue () const;
  private:
    ChunkType _type;
    string _text;
    SGConstPropertyNode_ptr _node;
    string _fmt;
    float _mult;
    float _offs;
    bool _trunc;
    mutable char _buf[1024];
    
  };

  FGTextLayer (int w = -1, int h = -1);
  virtual ~FGTextLayer ();

  virtual void draw ();

				// Transfer pointer!!
  virtual void addChunk (Chunk * chunk);
  virtual void setColor (float r, float g, float b);
  virtual void setPointSize (float size);
  virtual void setFontName ( const string &name );
  virtual void setFont (fntFont * font);

private:

  void recalc_value () const;

  typedef vector<Chunk *> chunk_list;
  chunk_list _chunks;
  float _color[4];

  float _pointSize;
  mutable string _font_name;
  mutable string _value;
  mutable SGTimeStamp _then;
  mutable SGTimeStamp _now;

  static fntRenderer text_renderer;
};


/**
 * A group layer that switches among its children.
 *
 * The first layer that passes its condition will be drawn, and
 * any following layers will be ignored.
 */
class FGSwitchLayer : public FGGroupLayer
{
public:
				// Transfer pointers!!
  FGSwitchLayer ();
  virtual void draw ();

};


#endif // __PANEL_HXX

// end of panel.hxx



