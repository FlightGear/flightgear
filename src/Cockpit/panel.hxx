//  panel.hxx - default, 2D single-engine prop instrument panel
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

#ifndef __PANEL_HXX
#define __PANEL_HXX

#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H          
#  include <windows.h>
#endif

#include <GL/glut.h>
#include <plib/ssg.h>

#include <vector>
#include <map>
#include <plib/fnt.h>

FG_USING_STD(vector);
FG_USING_STD(map);

class FGPanelInstrument;



////////////////////////////////////////////////////////////////////////
// Texture manager (should migrate out into FGFS).
//
// This class ensures that no texture is loaded more than once.
////////////////////////////////////////////////////////////////////////

class FGTextureManager
{
public:
  static ssgTexture * createTexture(const char * relativePath);
private:
  static map<const char *,ssgTexture *>_textureMap;
};


////////////////////////////////////////////////////////////////////////
// Instrument panel class.
//
// The panel is a container that has a background texture and holds
// zero or more instruments.  The panel will order the instruments to
// redraw themselves when necessary, and will pass mouse clicks on to
// the appropriate instruments for processing.
////////////////////////////////////////////////////////////////////////

class FGPanel
{
public:

  FGPanel (int x, int y, int w, int h);
  virtual ~FGPanel ();

				// transfer pointer ownership!!!
  virtual void addInstrument (FGPanelInstrument * instrument);

				// Update the panel (every frame).
  virtual void update () const;

				// Background texture.
  virtual void setBackground (ssgTexture * texture);

				// Make the panel visible or invisible.
  virtual bool getVisibility () const;
  virtual void setVisibility (bool visibility);

				// Handle a mouse click.
  virtual bool doMouseAction (int button, int updown, int x, int y);

private:
  bool _visibility;
  bool _mouseDown;
  int _mouseButton, _mouseX, _mouseY;
  mutable int _mouseDelay;
  FGPanelInstrument * _mouseInstrument;
  typedef vector<FGPanelInstrument *> instrument_list_type;
  int _x, _y, _w, _h;
  int _panel_h;
  ssgTexture * _bg;
				// List of instruments in panel.
  instrument_list_type _instruments;
};



////////////////////////////////////////////////////////////////////////
// Base class for user action types.
//
// Individual instruments can have actions associated with a mouse
// click in a rectangular area.  Current concrete classes are
// FGAdjustAction and FGSwapAction.
////////////////////////////////////////////////////////////////////////

class FGPanelAction
{
public:
  virtual void doAction () = 0;
};



////////////////////////////////////////////////////////////////////////
// Adjustment action.
//
// This is an action to increase or decrease an FGFS value by a certain
// increment within a certain range.  If the wrap flag is true, the
// value will wrap around if it goes below min or above max; otherwise,
// it will simply stop at min or max.
////////////////////////////////////////////////////////////////////////

class FGAdjustAction : public FGPanelAction
{
public:
  typedef double (*getter_type)();
  typedef void (*setter_type)(double);

  FGAdjustAction (getter_type getter, setter_type setter, float increment,
		  float min, float max, bool wrap=false);
  virtual ~FGAdjustAction ();
  virtual void doAction ();

private:
  getter_type _getter;
  setter_type _setter;
  float _increment;
  float _min;
  float _max;
  bool _wrap;
};



////////////////////////////////////////////////////////////////////////
// Swap action.
//
// This is an action to swap two values.  It's currently used in the
// navigation radios.
////////////////////////////////////////////////////////////////////////

class FGSwapAction : public FGPanelAction
{
public:
  typedef double (*getter_type)();
  typedef void (*setter_type)(double);

  FGSwapAction (getter_type getter1, setter_type setter1,
		getter_type getter2, setter_type setter2);
  virtual ~FGSwapAction ();
  virtual void doAction ();

private:
  getter_type _getter1, _getter2;
  setter_type _setter1, _setter2;
};



////////////////////////////////////////////////////////////////////////
// Toggle action.
//
// This is an action to toggle a boolean value.
////////////////////////////////////////////////////////////////////////

class FGToggleAction : public FGPanelAction
{
public:
  typedef bool (*getter_type)();
  typedef void (*setter_type)(bool);

  FGToggleAction (getter_type getter, setter_type setter);
  virtual ~FGToggleAction ();
  virtual void doAction ();

private:
  getter_type _getter;
  setter_type _setter;
};



////////////////////////////////////////////////////////////////////////
// Abstract base class for a panel instrument.
//
// A panel instrument consists of zero or more actions, associated
// with mouse clicks in rectangular areas.  Currently, the only
// concrete class derived from this is FGLayeredInstrument, but others
// may show up in the future (some complex instruments could be 
// entirely hand-coded, for example).
////////////////////////////////////////////////////////////////////////

class FGPanelInstrument
{
public:
  FGPanelInstrument ();
  FGPanelInstrument (int x, int y, int w, int h);
  virtual ~FGPanelInstrument ();

  virtual void draw () const = 0;

  virtual void setPosition(int x, int y);
  virtual void setSize(int w, int h);

  virtual int getXPos () const;
  virtual int getYPos () const;
  virtual int getWidth () const;
  virtual int getHeight () const;

				// Coordinates relative to centre.
				// Transfer pointer ownership!!
  virtual void addAction (int button, int x, int y, int w, int h,
			  FGPanelAction * action);

				// Coordinates relative to centre.
  virtual bool doMouseAction (int button, int x, int y);

protected:
  int _x, _y, _w, _h;
  typedef struct {
    int button;
    int x;
    int y;
    int w;
    int h;
    FGPanelAction * action;
  } inst_action;
  typedef vector<inst_action> action_list_type;
  action_list_type _actions;
};



////////////////////////////////////////////////////////////////////////
// Abstract base class for an instrument layer.
//
// The FGLayeredInstrument class builds up instruments by using layers
// of textures or text.  Each layer can have zero or more
// transformations applied to it: for example, a needle layer can
// rotate to show the altitude or airspeed.
////////////////////////////////////////////////////////////////////////

/**
 * A single layer of a multi-layered instrument.
 *
 * Each layer can be subject to a series of transformations based
 * on current FGFS instrument readings: for example, a texture
 * representing a needle can rotate to show the airspeed.
 */
class FGInstrumentLayer
{
public:
  typedef enum {
    XSHIFT,
    YSHIFT,
    ROTATION
  } transform_type;

  typedef double (*transform_func)();


  FGInstrumentLayer ();
  FGInstrumentLayer (int w, int h);
  virtual ~FGInstrumentLayer ();

  virtual void draw () const = 0;
  virtual void transform () const;

  virtual void addTransformation (transform_type type, transform_func func,
				  float min, float max,
				  float factor = 1.0, float offset = 0.0);

protected:
  int _w, _h;

  typedef struct {
    transform_type type;
    transform_func func;
    float min;
    float max;
    float factor;
    float offset;
  } transformation;
  typedef vector<transformation *> transformation_list;
  transformation_list _transformations;
};



////////////////////////////////////////////////////////////////////////
// An instrument composed of layers.
//
// This class represents an instrument which is simply a series of
// layers piled one on top of the other, each one undergoing its own
// set of transformations.  For example, one layer can represent
// the instrument's face (which doesn't move), while the next layer
// can represent a needle that rotates depending on an FGFS variable.
////////////////////////////////////////////////////////////////////////


/**
 * An instrument constructed of multiple layers.
 *
 * Each individual layer can be rotated or shifted to correspond
 * to internal FGFS instrument readings.
 */
class FGLayeredInstrument : public FGPanelInstrument
{
public:
  typedef vector<FGInstrumentLayer *> layer_list;
  FGLayeredInstrument (int x, int y, int w, int h);
  virtual ~FGLayeredInstrument ();

  virtual void draw () const;

				// Transfer pointer ownership!!
  virtual int addLayer (FGInstrumentLayer *layer);
  virtual int addLayer (ssgTexture * texture,
			int w = -1, int h = -1,
			float texX1 = 0.0, float texY1 = 0.0,
			float texX2 = 1.0, float texY2 = 1.0);
  virtual void addTransformation (FGInstrumentLayer::transform_type type,
				  FGInstrumentLayer::transform_func func,
				  float min, float max,
				  float factor = 1.0, float offset = 0.0);
  virtual void addTransformation (FGInstrumentLayer::transform_type type,
				  float offset);

protected:
  layer_list _layers;
};



////////////////////////////////////////////////////////////////////////
// A textured layer of an instrument.
//
// This is a layer holding a single texture.  Normally, the texture's
// backgound should be transparent so that lower layers and the panel
// background can show through.
////////////////////////////////////////////////////////////////////////

class FGTexturedLayer : public FGInstrumentLayer
{
public:
  FGTexturedLayer (ssgTexture * texture, int w, int h,
		   float texX1 = 0.0, float texY1 = 0.0,
		   float texX2 = 1.0, float texY2 = 1.0);
  virtual ~FGTexturedLayer ();

  virtual void draw () const;

  virtual void setTexture (ssgTexture * texture) { _texture = texture; }
  virtual void setTextureCoords (float x1, float y1, float x2, float y2) {
    _texX1 = x1; _texY1 = y1; _texX2 = x2; _texY2 = y2;
  }

protected:
  
  virtual void setTextureCoords (float x1, float y1,
				 float x2, float y2) const {
    _texX1 = x1; _texY1 = y1; _texX2 = x2; _texY2 = y2;
  }


private:
  ssgTexture * _texture;
  mutable float _texX1, _texY1, _texX2, _texY2;
};



////////////////////////////////////////////////////////////////////////
// A text layer of an instrument.
//
// This is a layer holding a string of static and/or generated text.
// It is useful for instruments that have text displays, such as
// a chronometer, GPS, or NavCom radio.
////////////////////////////////////////////////////////////////////////

class FGTextLayer : public FGInstrumentLayer
{
public:
  typedef char * (*text_func)();
  typedef double (*double_func)();
  typedef enum ChunkType {
    TEXT,
    TEXT_FUNC,
    DOUBLE_FUNC
  };

  class Chunk {
  public:
    Chunk (char * text, char * fmt = "%s");
    Chunk (text_func func, char * fmt = "%s");
    Chunk (double_func func, char * fmt = "%.2f", float mult = 1.0);

    char * getValue () const;
  private:
    ChunkType _type;
    union {
      char * _text;
      text_func _tfunc;
      double_func _dfunc;
    } _value;
    char * _fmt;
    float _mult;
    mutable char _buf[1024];
  };

  FGTextLayer (int w, int h);
  virtual ~FGTextLayer ();

  virtual void draw () const;

				// Transfer pointer!!
  virtual void addChunk (Chunk * chunk);
  virtual void setColor (float r, float g, float b);
  virtual void setPointSize (float size);
  virtual void setFont (fntFont * font);

private:
  typedef vector<Chunk *> chunk_list;
  chunk_list _chunks;
  float _color[4];
				// FIXME: need only one globally
  mutable fntRenderer _renderer;
};



////////////////////////////////////////////////////////////////////////
// A layer that switches between two other layers.
////////////////////////////////////////////////////////////////////////

class FGSwitchLayer : public FGInstrumentLayer
{
public:
  typedef bool (*switch_func)();

				// Transfer pointers!!
  FGSwitchLayer (int w, int h, switch_func func,
		 FGInstrumentLayer * layer1,
		 FGInstrumentLayer * layer2);
  virtual ~FGSwitchLayer ();

  virtual void draw () const;

private:
  switch_func _func;
  FGInstrumentLayer * _layer1, * _layer2;
};



////////////////////////////////////////////////////////////////////////
// The current panel, if any.
////////////////////////////////////////////////////////////////////////

extern FGPanel * current_panel;



#endif // __PANEL_HXX

// end of panel.hxx


