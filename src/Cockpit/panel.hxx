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
#include <hash_map>
#include <plib/fnt.h>

FG_USING_STD(vector);
FG_USING_STD(hash_map);

class FGPanelInstrument;


////////////////////////////////////////////////////////////////////////
// Instrument panel class.
////////////////////////////////////////////////////////////////////////

class FGPanel
{
public:

  FGPanel ();
  virtual ~FGPanel ();

  virtual ssgTexture * createTexture (const char * relativePath);

				// Legacy interface from old panel.
  static FGPanel * OurPanel;
  virtual float get_height () const;
  virtual void ReInit (int x, int y, int finx, int finy);
  virtual void Update () const;

private:

  typedef vector<FGPanelInstrument *> instrument_list_type;

  int _x, _y, _w, _h;
  int _panel_h;

  ssgTexture * _bg;

				// Internalization table.
  hash_map<const char *,ssgTexture *> _textureMap;

				// List of instruments in panel.
  instrument_list_type _instruments;
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

protected:
  int _x, _y, _w, _h;
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



#endif // __PANEL_HXX

// end of panel.hxx


