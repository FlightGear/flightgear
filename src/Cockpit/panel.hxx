//  panel.hxx -- instrument panel defines and prototypes
// 
//  Written by Friedemann Reinhard, started June 1998.
//
//  Major code reorganization by David Megginson, November 1999.
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


#ifndef _PANEL_HXX
#define _PANEL_HXX


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
#include <XGL/xgl.h>

class FGInstrument;		// FIXME: rearrange to avoid this?


/**
 * Top-level class to hold an instance of a panel.
 */
class FGPanel{

public:
  static FGPanel *OurPanel;	// current_panel would be better

				// FIXME: a few other classes have a
				// dependency on this information; it
				// would be nice to fix that.
  GLuint panel_tex_id[2];

  FGPanel();
  virtual ~FGPanel ();
  virtual float get_height(void) { return height; }
  virtual void ReInit( int x, int y, int finx, int finy);
  virtual void Update(void);

private:

  int height;
  int width;
  GLubyte *background;
  GLubyte *imag;
  int imag_width, imag_height;
  GLubyte *img;
  int img_width, img_height;
				// The instruments on the panel.
  FGInstrument * horizonIndicator;
  FGInstrument * turnCoordinator;
  FGInstrument * rpmIndicator;
  FGInstrument * airspeedIndicator;
  FGInstrument * verticalSpeedIndicator;
  FGInstrument * altimeter;
  FGInstrument * altimeter2;
};


/**
 * Abstract base class for all panel instruments.
 */
class FGInstrument{

public:
  FGInstrument (void) {}
  virtual ~FGInstrument (void) {}
  virtual void Init(void) = 0;
  virtual void Render(void) = 0;

protected:
  float XPos;
  float YPos;
};


/**
 * Instrument: the artificial horizon.
 */
class FGHorizon : public FGInstrument 
{

public:
  FGHorizon (float inXPos, float inYPos);
  virtual ~FGHorizon (void);
  virtual void Init (void);
  virtual void Render (void);
        
private:
  float texXPos;
  float texYPos;
  float radius;
  float bottom;   // tell the program the offset between midpoint and bottom 
  float top;      // guess what ;-)
  float vertices[180][2];
  float normals[180][3];
  float texCoord[180][2];
};


/**
 * Instrument: the turn co-ordinator.
 */
class FGTurnCoordinator : public FGInstrument 
{

public:
  FGTurnCoordinator (float inXPos, float inYPos);
  virtual ~FGTurnCoordinator (void);
  virtual void Init (void);
  virtual void Render(void);
  
private:
  float PlaneTexXPos;
  float PlaneTexYPos;
  float alpha;
  float PlaneAlpha;
  float alphahist[2];
  float rollhist[2];
  float BallXPos;
  float BallYPos;
  float BallTexXPos;
  float BallTexYPos;
  float BallRadius;
  GLfloat vertices[72];
  static GLfloat wingArea[];
  static GLfloat elevatorArea[];
  static GLfloat rudderArea[];
};


/**
 * Abstract base class for gauges with needles and textured backgrounds.
 *
 * The airspeed indicator, vertical speed indicator, altimeter, and RPM 
 * gauge are all derived from this class.
 */
class FGTexInstrument : public FGInstrument 
{
public:
  FGTexInstrument (void);
  virtual ~FGTexInstrument ();
  virtual void Init(void);
  virtual void Render(void);
  
protected:
  virtual void CreatePointer(void);
  virtual void UpdatePointer(void);
  virtual double getValue () const = 0;

  float radius;
  float length;
  float width;
  float angle;
  float tape[2];
  float value1;
  float value2;
  float alpha1;
  float alpha2;
  float textureXPos;
  float textureYPos;
  GLfloat vertices[20];
};


/**
 * Instrument: the airspeed indicator.
 */
class FGAirspeedIndicator : public FGTexInstrument
{
public:
  FGAirspeedIndicator (int x, int y);
  virtual ~FGAirspeedIndicator ();

protected:
  double getValue () const;
};


/**
 * Instrument: the vertical speed indicator.
 */
class FGVerticalSpeedIndicator : public FGTexInstrument
{
public:
  FGVerticalSpeedIndicator (int x, int y);
  virtual ~FGVerticalSpeedIndicator ();

protected:
  double getValue () const;
};


/**
 * Instrument: the altimeter (big hand?)
 */
class FGAltimeter : public FGTexInstrument
{
public:
  FGAltimeter (int x, int y);
  virtual ~FGAltimeter ();

protected:
  double getValue () const;
};


/**
 * Instrument: the altimeter (little hand?)
 */
class FGAltimeter2 : public FGTexInstrument
{
public:
  FGAltimeter2 (int x, int y);
  virtual ~FGAltimeter2 ();

protected:
  double getValue () const;
};


/**
 * Instrument: the RPM gauge (actually manifold pressure right now).
 */
class FGRPMIndicator : public FGTexInstrument
{
public:
  FGRPMIndicator (int x, int y);
  virtual ~FGRPMIndicator ();

protected:
  double getValue () const;
};


				// FIXME: move to FGPanel, somehow
void fgEraseArea(GLfloat *array, int NumVerti, GLfloat texXPos,                                  GLfloat texYPos, GLfloat XPos, GLfloat YPos,                                    int Texid, float ScaleFactor);

#endif // _PANEL_HXX 
