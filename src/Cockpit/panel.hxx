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
////////////////////////////////////////////////////////////////////////

class FGPanel
{
public:

  FGPanel ();
  virtual ~FGPanel ();

				// transfer pointer ownership!!!
  virtual void addInstrument (FGPanelInstrument * instrument);
  virtual void init (int x, int y, int finx, int finy);
  virtual void update () const;
  
  virtual bool getVisibility () const;
  virtual void setVisibility (bool visibility);

  virtual bool doMouseAction (int button, int updown, int x, int y);

private:
  bool _initialized;
  bool _visibility;
  typedef vector<FGPanelInstrument *> instrument_list_type;
  int _x, _y, _w, _h;
  int _panel_h;
  ssgTexture * _bg;
				// List of instruments in panel.
  instrument_list_type _instruments;
};



////////////////////////////////////////////////////////////////////////
// Base class for user action types.
////////////////////////////////////////////////////////////////////////

class FGPanelAction
{
public:
  virtual void doAction () = 0;
};



////////////////////////////////////////////////////////////////////////
// Adjustment action.
////////////////////////////////////////////////////////////////////////

class FGAdjustAction : public FGPanelAction
{
public:
  typedef double (*getter_type)();
  typedef void (*setter_type)(double);

  FGAdjustAction (getter_type getter, setter_type setter, double increment,
		  double min, double max, bool wrap=false);
  virtual ~FGAdjustAction ();
  virtual void doAction ();
private:
  getter_type _getter;
  setter_type _setter;
  double _increment;
  double _min;
  double _max;
  bool _wrap;
};



////////////////////////////////////////////////////////////////////////
// Instrument base class.
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
  virtual void addAction (int x, int y, int w, int h,
			  FGPanelAction * action);

				// Coordinates relative to centre.
  virtual bool doMouseAction (int button, int updown, int x, int y);

protected:
  int _x, _y, _w, _h;
  typedef struct {
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
// A single layer of an instrument.
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
  FGInstrumentLayer (int w, int h, int z);
  virtual ~FGInstrumentLayer ();

  virtual void draw () const = 0;
  virtual void transform () const;

  virtual void addTransformation (transform_type type, transform_func func,
				  double min, double max,
				  double factor = 1.0, double offset = 0.0);

protected:
  int _w, _h, _z;

  typedef struct {
    transform_type type;
    transform_func func;
    double min;
    double max;
    double factor;
    double offset;
  } transformation;
  typedef vector<transformation *> transformation_list;
  transformation_list _transformations;
};



////////////////////////////////////////////////////////////////////////
// An instrument composed of layered textures.
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
  virtual void addLayer (FGInstrumentLayer *layer);
  virtual void addLayer (int i, ssgTexture * texture);
  virtual void addTransformation (int layer,
				  FGInstrumentLayer::transform_type type,
				  FGInstrumentLayer::transform_func func,
				  double min, double max,
				  double factor = 1.0, double offset = 0.0);
  virtual void addTransformation (int layer,
				  FGInstrumentLayer::transform_type type,
				  double offset) {
    addTransformation(layer, type, 0, 0.0, 0.0, 1.0, offset);
  }

protected:
  layer_list _layers;
};



////////////////////////////////////////////////////////////////////////
// A textured layer of an instrument.
////////////////////////////////////////////////////////////////////////

/**
 * A textured layer of an instrument.
 *
 * This is a type of layer designed to hold a texture; normally,
 * the texture's background should be transparent so that
 * other layers or the panel background show through.
 */
class FGTexturedInstrumentLayer : public FGInstrumentLayer
{
public:
  FGTexturedInstrumentLayer (ssgTexture * texture,
			     int w, int h, int z);
  virtual ~FGTexturedInstrumentLayer ();

  virtual void draw () const;

  virtual void setTexture (ssgTexture * texture) { _texture = texture; }

private:
  ssgTexture * _texture;
};



////////////////////////////////////////////////////////////////////////
// A text layer of an instrument.
////////////////////////////////////////////////////////////////////////

class FGCharInstrumentLayer : public FGInstrumentLayer
{
public:
  typedef char * (*text_func)(char *);
  FGCharInstrumentLayer (text_func func,
			 int w, int h, int z);
  virtual ~FGCharInstrumentLayer ();

  virtual void draw () const;
  virtual void setColor (float r, float g, float b);
  virtual void setPointSize (float size);
  virtual void setFont (fntFont * font);

private:
  text_func _func;
  float _color[4];
				// FIXME: need only one globally
  mutable fntRenderer _renderer;
  mutable char _buf[1024];
};



////////////////////////////////////////////////////////////////////////
// The current panel, if any.
////////////////////////////////////////////////////////////////////////

extern FGPanel current_panel;



#endif // __PANEL_HXX

// end of panel.hxx


