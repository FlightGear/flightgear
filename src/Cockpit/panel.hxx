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
#include <simgear/xgl.h>

#include <plib/ssg.h>

class FGPanelInstrument;



////////////////////////////////////////////////////////////////////////
// Instrument panel class.
////////////////////////////////////////////////////////////////////////

class FGPanel
{
public:
  FGPanel ();
  virtual ~FGPanel ();

				// Legacy interface from old panel.
  static FGPanel * OurPanel;
  virtual float get_height () const;
  virtual void ReInit (int x, int y, int finx, int finy);
  virtual void Update () const;

private:
  int _x, _y, _w, _h;
  int _panel_h;

  ssgTexture * _bg;

  const FGPanelInstrument * _airspeed;
  const FGPanelInstrument * _horizon;
  const FGPanelInstrument * _altimeter;
  const FGPanelInstrument * _coordinator;
  const FGPanelInstrument * _gyro;
  const FGPanelInstrument * _vertical;
  const FGPanelInstrument * _flaps;
  const FGPanelInstrument * _rpm;
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
// An instrument composed of layered textures.
////////////////////////////////////////////////////////////////////////

class FGTexturedInstrument : public FGPanelInstrument
{
public:
  static const int MAX_LAYERS = 8;
  FGTexturedInstrument (int x, int y, int w, int h);
  virtual ~FGTexturedInstrument ();

  virtual void addLayer (int layer, const char * textureName);
  virtual void addLayer (int layer, ssgTexture * texture);
  virtual void setLayerCenter (int layer, int x, int y);
  virtual void setLayerRot (int layer, double rotation) const;
  virtual void setLayerOffset (int layer, int xoffset, int yoffset) const;
  virtual bool hasLayer (int layer) const;

  virtual void draw () const;
protected:
  bool _layers[MAX_LAYERS];
  mutable int _xcenter[MAX_LAYERS];
  mutable int _ycenter[MAX_LAYERS];
  mutable double _rotation[MAX_LAYERS];
  mutable int _xoffset[MAX_LAYERS];
  mutable int _yoffset[MAX_LAYERS];
  ssgTexture * _textures[MAX_LAYERS];
};



////////////////////////////////////////////////////////////////////////
// Airspeed indicator.
////////////////////////////////////////////////////////////////////////

class FGAirspeedIndicator : public FGTexturedInstrument
{
public:
  FGAirspeedIndicator (int x, int y);
  virtual ~FGAirspeedIndicator ();
  virtual void draw () const;
};



////////////////////////////////////////////////////////////////////////
// Artificial Horizon.
////////////////////////////////////////////////////////////////////////

class FGHorizon : public FGTexturedInstrument
{
public:
  FGHorizon (int x, int y);
  virtual ~FGHorizon ();
  virtual void draw () const;
};



////////////////////////////////////////////////////////////////////////
// Altimeter.
////////////////////////////////////////////////////////////////////////

class FGAltimeter : public FGTexturedInstrument
{
public:
  FGAltimeter (int x, int y);
  virtual ~FGAltimeter ();
  virtual void draw () const;
};



////////////////////////////////////////////////////////////////////////
// Turn Co-ordinator.
////////////////////////////////////////////////////////////////////////

class FGTurnCoordinator : public FGTexturedInstrument
{
public:
  FGTurnCoordinator (int x, int y);
  virtual ~FGTurnCoordinator ();
  virtual void draw () const;
};



////////////////////////////////////////////////////////////////////////
// Gyro Compass.
////////////////////////////////////////////////////////////////////////

class FGGyroCompass : public FGTexturedInstrument
{
public:
  FGGyroCompass (int x, int y);
  virtual ~FGGyroCompass ();
  virtual void draw () const;
};



////////////////////////////////////////////////////////////////////////
// Vertical velocity indicator.
////////////////////////////////////////////////////////////////////////

class FGVerticalVelocity : public FGTexturedInstrument
{
public:
  FGVerticalVelocity (int x, int y);
  virtual ~FGVerticalVelocity ();
  virtual void draw () const;
};



////////////////////////////////////////////////////////////////////////
// RPM gauge.
////////////////////////////////////////////////////////////////////////

class FGRPMGauge : public FGTexturedInstrument
{
public:
  FGRPMGauge (int x, int y);
  virtual ~FGRPMGauge ();
  virtual void draw () const;
};



////////////////////////////////////////////////////////////////////////
// Flap position indicator.
////////////////////////////////////////////////////////////////////////

class FGFlapIndicator : public FGTexturedInstrument
{
public:
  FGFlapIndicator (int x, int y);
  virtual ~FGFlapIndicator ();
  virtual void draw () const;
};



#endif // __PANEL_HXX

// end of panel.hxx


