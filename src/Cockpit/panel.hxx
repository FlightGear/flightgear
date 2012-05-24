//  panel.hxx - generic support classes for a 2D panel.
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

#ifndef __PANEL_HXX
#define __PANEL_HXX

#include <osg/ref_ptr>
#include <osg/StateSet>
#include <osg/Texture2D>

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>
#include <simgear/props/propertyObject.hxx>

#include <simgear/structure/SGBinding.hxx>
#include <simgear/math/interpolater.hxx>
#include <simgear/timing/timestamp.hxx>

#include <cmath>
#include <vector>
#include <map>

#include <Main/fg_props.hxx>

class FGPanelInstrument;
class fntFont;
class DCLGPS;
class IntRect;

////////////////////////////////////////////////////////////////////////
// Texture management.
////////////////////////////////////////////////////////////////////////


/**
 * Texture manager (should migrate out into FGFS).
 *
 * This class ensures that no texture is loaded more than once.
 */
class FGTextureManager
{
public:
  static osg::Texture2D* createTexture(const std::string &relativePath,
                                       bool staticTexture = true);
  static void addTexture(const std::string &relativePath, osg::Texture2D* texture);
private:
  static std::map<std::string,osg::ref_ptr<osg::Texture2D> > _textureMap;
};


/**
 * Cropped texture (should migrate out into FGFS).
 *
 * This structure wraps an SSG texture with cropping information.
 */
class FGCroppedTexture
{
public:

  FGCroppedTexture ();
  FGCroppedTexture (const std::string &path,
		  float _minX = 0.0, float _minY = 0.0,
		  float _maxX = 1.0, float _maxY = 1.0);
  virtual ~FGCroppedTexture ();

  virtual void setPath (const std::string &path) { _path = path; }

  virtual const std::string &getPath () const { return _path; }

  virtual osg::StateSet* getTexture ();

  virtual void setCrop (float minX, float minY, float maxX, float maxY) {
    _minX = minX; _minY = minY; _maxX = maxX; _maxY = maxY;
  }

  virtual float getMinX () const { return _minX; }
  virtual float getMinY () const { return _minY; }
  virtual float getMaxX () const { return _maxX; }
  virtual float getMaxY () const { return _maxY; }


private:
  std::string _path;
  osg::ref_ptr<osg::StateSet> _texture;
  float _minX, _minY, _maxX, _maxY;
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
class FGPanel : public SGReferenced
{
public:

  FGPanel ();
  virtual ~FGPanel ();

  virtual void draw (osg::State& state);
 
  virtual void updateMouseDelay(double dt);

				// transfer pointer ownership!!!
  virtual void addInstrument (FGPanelInstrument * instrument);

				// Background texture.
  virtual void setBackground (osg::Texture2D* texture);

				// Background multiple textures.
  virtual void setMultiBackground (osg::Texture2D* texture, int idx);

  virtual void setWidth (int width) { _width = width; }
  virtual int getWidth () const { return _width; }

				// Full height of panel.
  virtual void setHeight (int height) { _height = height; }
  virtual int getHeight () const { return _height; }

				// X-offset
  virtual void setXOffset (int offset);
  virtual int getXOffset () const { return _x_offset->getIntValue(); }

				// Y-offset.
  virtual void setYOffset (int offset);
  virtual int getYOffset () const { return _y_offset->getIntValue(); }

				// View height.
 // virtual void setViewHeight (int height) { _view_height = height; }
 // virtual int getViewHeight () const { return _view_height; }

  /**
   * find the actual logical extend of the panel, including all instruments
   * and actions.
   */
  void getLogicalExtent(int &x0, int& y0, int& x1, int &y1);
  
				// Handle a mouse click.
  virtual bool doMouseAction (int button, int updown, int x, int y);
  virtual bool doLocalMouseAction(int button, int updown, int x, int y);

  virtual void setDepthTest (bool enable);

  bool getAutohide(void) const { return _autohide; };
  void setAutohide(bool enable) { _autohide = enable; };
  
  double getAspectScale() const;

private:
  void setupVirtualCockpit();
  void cleanupVirtualCockpit();

  mutable bool _mouseDown;
  mutable int _mouseButton, _mouseX, _mouseY;
  double _mouseActionRepeat;
    
  mutable FGPanelInstrument * _mouseInstrument;
  typedef std::vector<FGPanelInstrument *> instrument_list_type;
  int _width;
  int _height;
 // int _view_height;

  SGPropertyNode_ptr _x_offset;
  SGPropertyNode_ptr _y_offset;
  SGPropertyNode_ptr _jitter;
  SGPropertyNode_ptr _flipx;

  SGConstPropertyNode_ptr _xsize_node;
  SGConstPropertyNode_ptr _ysize_node;
  
  osg::ref_ptr<osg::StateSet> _bg;
  osg::ref_ptr<osg::StateSet> _mbg[8];
				// List of instruments in panel.
  instrument_list_type _instruments;
  bool _enable_depth_test;
  bool _autohide;
  
  SGPropObjBool _drawPanelHotspots;
};



////////////////////////////////////////////////////////////////////////
// Actions
////////////////////////////////////////////////////////////////////////


/**
 * Class for user actions.
 *
 * The actions are command bindings, like bindings for the keyboard
 * or joystick, but they are tied to specific mouse actions in
 * rectangular areas of the panel.
 */
class FGPanelAction : public SGConditional
{
public:
  FGPanelAction ();
  FGPanelAction (int button, int x, int y, int w, int h, bool repeatable);
  virtual ~FGPanelAction ();

				// Getters.
  virtual int getButton () const { return _button; }
  virtual int getX () const { return _x; }
  virtual int getY () const { return _y; }
  virtual int getWidth () const { return _w; }
  virtual int getHeight () const { return _h; }

				// Setters.

				// transfer pointer ownership
  virtual void addBinding (SGBinding * binding, int updown);
  virtual void setButton (int button) { _button = button; }
  virtual void setX (int x) { _x = x; }
  virtual void setY (int y) { _y = y; }
  virtual void setWidth (int w) { _w = w; }
  virtual void setHeight (int h) { _h = h; }

				// Check whether we're in the area.
  virtual bool inArea (int button, int x, int y)
  {
    return (button == _button &&
	    x >= _x &&
	    x < _x + _w &&
	    y >= _y &&
	    y < _y + _h);
  }

				// Perform the action.
  virtual bool doAction (int updown);

private:
  typedef std::vector<SGBinding *> binding_list_t;

  int _button;
  int _x;
  int _y;
  int _w;
  int _h;
  bool _repeatable;
  int _last_state;
  binding_list_t _bindings[2];
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

  virtual void draw (osg::State& state) = 0;
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

  typedef std::vector<FGPanelTransformation *> transformation_list;
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

  virtual void draw (osg::State& state) = 0;
  virtual void drawHotspots(osg::State& state);

  virtual void setPosition(int x, int y);
  virtual void setSize(int w, int h);

  virtual int getXPos () const;
  virtual int getYPos () const;
  virtual int getWidth () const;
  virtual int getHeight () const;

				// Coordinates relative to centre.
				// Transfer pointer ownership!!
  virtual void addAction (FGPanelAction * action);

				// Coordinates relative to centre.
  virtual bool doMouseAction (int button, int updown, int x, int y);

  void extendRect(IntRect& r) const;
protected:
  int _x, _y, _w, _h;
  typedef std::vector<FGPanelAction *> action_list_type;
  action_list_type _actions;
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

  virtual void draw (osg::State& state);

				// Transfer pointer ownership!!
  virtual int addLayer (FGInstrumentLayer *layer);
  virtual int addLayer (const FGCroppedTexture &texture, int w = -1, int h = -1);

				// Transfer pointer ownership!!
  virtual void addTransformation (FGPanelTransformation * transformation);

protected:
  typedef std::vector<FGInstrumentLayer *> layer_list;
  layer_list _layers;
};


/**
 * An empty-shell instrument that exists soley in
 * order to redirect commands from the panel to a
 * complex instrument inherited from SGSubsystem.
 *
 * Currently the only complex instrument is the KLN89,
 * which we've hardwired this to for now.
 */
class FGSpecialInstrument : public FGPanelInstrument
{
public:
  FGSpecialInstrument(DCLGPS* sb);
  //FGSpecialInstrument (int x, int y, int w, int h);
  virtual ~FGSpecialInstrument ();

  virtual void draw (osg::State& state);
  
protected:
  DCLGPS* complex;
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
  virtual void draw (osg::State& state);
				// transfer pointer ownership
  virtual void addLayer (FGInstrumentLayer * layer);
protected:
  std::vector<FGInstrumentLayer *> _layers;
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
  FGTexturedLayer (const FGCroppedTexture &texture, int w = -1, int h = -1);
  virtual ~FGTexturedLayer ();

  virtual void draw (osg::State& state);

  virtual void setTexture (const FGCroppedTexture &texture) {
    _texture = texture;
  }
  virtual const FGCroppedTexture &getTexture () const { return _texture; }
  virtual FGCroppedTexture *getTexture() { return &_texture; }

  void setEmissive(bool e) { _emissive = e; }

private:
  FGCroppedTexture _texture;
  bool _emissive;
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
    Chunk (const std::string &text, const std::string &fmt = "%s");
    Chunk (ChunkType type, const SGPropertyNode * node,
	   const std::string &fmt = "", float mult = 1.0, float offs = 0.0,
           bool truncation = false);

    const char * getValue () const;
  private:
    ChunkType _type;
    std::string _text;
    SGConstPropertyNode_ptr _node;
    std::string _fmt;
    float _mult;
    float _offs;
    bool _trunc;
    mutable char _buf[1024];
  };

  FGTextLayer (int w = -1, int h = -1);
  virtual ~FGTextLayer ();

  virtual void draw (osg::State& state);

				// Transfer pointer!!
  virtual void addChunk (Chunk * chunk);
  virtual void setColor (float r, float g, float b);
  virtual void setPointSize (float size);
  virtual void setFontName ( const std::string &name );
  virtual void setFont (fntFont * font);

private:

  void recalc_value () const;

  typedef std::vector<Chunk *> chunk_list;
  chunk_list _chunks;
  float _color[4];

  float _pointSize;
  mutable std::string _font_name;
  mutable std::string _value;
  mutable SGTimeStamp _then;
  mutable SGTimeStamp _now;
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
  virtual void draw (osg::State& state);

};
#endif // __PANEL_HXX

// end of panel.hxx



