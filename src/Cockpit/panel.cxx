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
#include <GL/glut.h>
#include <simgear/xgl.h>

#include <simgear/logstream.hxx>
#include <simgear/fgpath.hxx>

#include <Main/options.hxx>
#include <Objects/texload.h>
#include <Autopilot/autopilot.hxx>

#include "cockpit.hxx"
#include "panel.hxx"

#define SIX_X 200
#define SIX_Y 345
#define SIX_W 128
#define SIX_SPACING (SIX_W + 5)
#define SMALL_W 112



////////////////////////////////////////////////////////////////////////
// Implementation of FGPanel.
////////////////////////////////////////////////////////////////////////

FGPanel * FGPanel::OurPanel = 0;

FGPanel::FGPanel ()
{
  if (OurPanel == 0) {
    OurPanel = this;
  } else {
    FG_LOG(FG_GENERAL, FG_ALERT, "Multiple panels");
    exit(-1);
  }

  int x = SIX_X;
  int y = SIX_Y;

  FGPath tpath(current_options.get_fg_root());
  tpath.append("Textures/Panel/panel-bg.rgb");
  _bg = new ssgTexture((char *)tpath.c_str(), false, false);

  _airspeed = new FGAirspeedIndicator(x, y);
  x += SIX_SPACING;
  _horizon = new FGHorizon(x, y);
  x += SIX_SPACING;
  _altimeter = new FGAltimeter(x, y);
  x = SIX_X;
  y -= SIX_SPACING;
  _coordinator = new FGTurnCoordinator(x, y);
  x += SIX_SPACING;
  _gyro = new FGGyroCompass(x, y);
  x += SIX_SPACING;
  _vertical = new FGVerticalVelocity(x, y);

  y -= SIX_SPACING;
  _rpm = new FGRPMGauge(x, y);

  x -= SIX_SPACING;
  _flaps = new FGFlapIndicator(x, y);
}

FGPanel::~FGPanel ()
{
  OurPanel = 0;
  delete _airspeed;
  delete _horizon;
  delete _altimeter;
  delete _coordinator;
  delete _gyro;
  delete _vertical;
  delete _rpm;
  delete _flaps;
}

float
FGPanel::get_height () const
{
  return _panel_h;
}

void
FGPanel::ReInit (int x, int y, int finx, int finy)
{
  _x = x;
  _y = y;
  _w = finx - x;
  _h = finy - y;
  _panel_h = (int)((finy - y) * 0.5768 + 1);
}

void
FGPanel::Update () const
{
  // glPushAttrib(GL_ALL_ATTRIB_BITS);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(_x, _x + _w, _y, _y + _h);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

				// Draw the background
  glDisable(GL_LIGHTING);
  glEnable(GL_TEXTURE_2D);
  if ( _bg->getHandle() >= 0 ) {
      glBindTexture(GL_TEXTURE_2D, _bg->getHandle());
  } else {
      cout << "invalid texture handle in FGPanel::Update()" << endl;
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glBegin(GL_POLYGON);
  glTexCoord2f(0.0, 0.0); glVertex3f(_x, _y, 0);
  glTexCoord2f(10.0, 0.0); glVertex3f(_x + _w, _y, 0);
  glTexCoord2f(10.0, 5.0); glVertex3f(_x + _w, _y + _panel_h, 0);
  glTexCoord2f(0.0, 5.0); glVertex3f(_x, _y + _panel_h, 0);
  glEnd();

				// Draw the instruments.
  glLoadIdentity();
  glTranslated(_airspeed->getXPos(), _airspeed->getYPos(), 0);
  _airspeed->draw();

  glLoadIdentity();
  glTranslated(_horizon->getXPos(), _horizon->getYPos(), 0);
  _horizon->draw();

  glLoadIdentity();
  glTranslated(_altimeter->getXPos(), _altimeter->getYPos(), 0);
  _altimeter->draw();

  glLoadIdentity();
  glTranslated(_coordinator->getXPos(), _coordinator->getYPos(), 0);
  _coordinator->draw();

  glLoadIdentity();
  glTranslated(_gyro->getXPos(), _gyro->getYPos(), 0);
  _gyro->draw();

  glLoadIdentity();
  glTranslated(_vertical->getXPos(), _vertical->getYPos(), 0);
  _vertical->draw();

  glLoadIdentity();
  glTranslated(_rpm->getXPos(), _rpm->getYPos(), 0);
  _rpm->draw();

  glLoadIdentity();
  glTranslated(_flaps->getXPos(), _flaps->getYPos(), 0);
  _flaps->draw();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // glPopAttrib();

//   ssgForceBasicState();
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



////////////////////////////////////////////////////////////////////////
// Implementation of FGTexturedInstrument.
////////////////////////////////////////////////////////////////////////

FGTexturedInstrument::FGTexturedInstrument (int x, int y, int w, int h)
  : FGPanelInstrument(x, y, w, h)
{
  for (int i = 0; i < MAX_LAYERS; i++) {
    _layers[i] = false;
  }
}

FGTexturedInstrument::~FGTexturedInstrument ()
{
  // FIXME: maybe free textures
}

void
FGTexturedInstrument::addLayer (int layer, const char * textureName)
{
  FGPath tpath(current_options.get_fg_root());
  tpath.append(textureName);
  ssgTexture * texture = new ssgTexture((char *)tpath.c_str(), false, false);
  cerr << "Loaded texture " << textureName << endl;
  addLayer(layer, texture);
}

void
FGTexturedInstrument::addLayer (int layer, ssgTexture * texture)
{
  _layers[layer] = true;
  _textures[layer] = texture;
  _rotation[layer] = 0.0;
  _xoffset[layer] = _yoffset[layer] = 0;
  _xcenter[layer] = _ycenter[layer] = 0;
}

void
FGTexturedInstrument::setLayerCenter (int layer, int x, int y)
{
  _xcenter[layer] = x;
  _ycenter[layer] = y;
}

void
FGTexturedInstrument::setLayerRot (int layer, double rotation) const
{
  _rotation[layer] = rotation;
}

void
FGTexturedInstrument::setLayerOffset (int layer, int xoff, int yoff) const
{
  _xoffset[layer] = xoff;
  _yoffset[layer] = yoff;
}

bool
FGTexturedInstrument::hasLayer (int layer) const
{
  return _layers[layer];
}

void
FGTexturedInstrument::draw () const
{
  glEnable(GL_TEXTURE_2D);

  int i;
  int w2 = _w / 2;
  int h2 = _h / 2;

  glMatrixMode(GL_MODELVIEW);
  for (i = 0; i < MAX_LAYERS; i++) {
    if (hasLayer(i)) {
      glPushMatrix();
      glTranslated(_xcenter[i], _ycenter[i], 0);
      glRotatef(0.0 - _rotation[i], 0.0, 0.0, 1.0);
      glTranslated(_xoffset[i], _yoffset[i], 0);
      if ( _textures[i]->getHandle() >= 0 ) {
	  glBindTexture(GL_TEXTURE_2D, _textures[i]->getHandle());
      } else {
	  cout << "invalid texture handle in FGTexturedInstrument::draw()" 
	       << endl;
      }
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      glBegin(GL_POLYGON);
				// FIXME: is this really correct
				// for layering?
      glTexCoord2f(0.0, 0.0); glVertex3f(-w2, -h2, i / 100.0 + 0.1);
      glTexCoord2f(1.0, 0.0); glVertex3f(w2, -h2, i / 100.0 + 0.1);
      glTexCoord2f(1.0, 1.0); glVertex3f(w2, h2, i / 100.0 + 0.1);
      glTexCoord2f(0.0, 1.0); glVertex3f(-w2, h2, i / 100.0 + 0.1);
      glEnd();
      glPopMatrix();
    }
  }

  glDisable(GL_TEXTURE_2D);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAirspeedIndicator.
////////////////////////////////////////////////////////////////////////

FGAirspeedIndicator::FGAirspeedIndicator (int x, int y)
  : FGTexturedInstrument(x, y, SIX_W, SIX_W)
{
  addLayer(0, "Textures/Panel/airspeed.rgb");
  addLayer(1, "Textures/Panel/long-needle.rgb");
}

FGAirspeedIndicator::~FGAirspeedIndicator ()
{
}

void
FGAirspeedIndicator::draw () const
{
  double speed = get_speed();
  if (speed < 30.0) {
    speed = 30.0;
  } else if (speed > 220.0) {
    speed = 220.0;
  }
  double angle = speed / 20.0 * 36.0 - 54.0;
  setLayerRot(1, angle);
  FGTexturedInstrument::draw();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGHorizon.
////////////////////////////////////////////////////////////////////////

FGHorizon::FGHorizon (int x, int y)
  : FGTexturedInstrument(x, y, SIX_W, SIX_W)
{
  addLayer(0, "Textures/Panel/horizon-bg.rgb");
  addLayer(1, "Textures/Panel/horizon-float.rgb");
  addLayer(2, "Textures/Panel/horizon-rim.rgb");
  addLayer(3, "Textures/Panel/horizon-fg.rgb");
}

FGHorizon::~FGHorizon ()
{
}

void
FGHorizon::draw () const
{
  double rot = get_roll() * RAD_TO_DEG;
  double pitch = get_pitch() * RAD_TO_DEG;
  if (pitch > 20)
    pitch = 20;
  else if (pitch < -20)
    pitch = -20;
  int yoffset = 0 - (pitch * ((1.5 / 160.0) * _h));
  setLayerRot(0, 0 - rot);
  setLayerRot(1, 0 - rot);
  setLayerOffset(1, 0, yoffset);
  setLayerRot(2, 0 - rot);
  FGTexturedInstrument::draw();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAltimeter.
////////////////////////////////////////////////////////////////////////

// TODO: add 10,000 bug

FGAltimeter::FGAltimeter (int x, int y)
  : FGTexturedInstrument(x, y, SIX_W, SIX_W)
{
  addLayer(0, "Textures/Panel/altimeter.rgb");
  addLayer(1, "Textures/Panel/long-needle.rgb");
  addLayer(2, "Textures/Panel/short-needle.rgb");
  addLayer(3, "Textures/Panel/bug.rgb");
}

FGAltimeter::~FGAltimeter ()
{
}

void
FGAltimeter::draw () const
{
  long altitude = get_altitude();
  setLayerRot(1, (altitude % 1000) / 1000.0 * 360.0);
  setLayerRot(2, (altitude % 10000) / 10000.0 * 360.0);
  setLayerRot(3, (altitude % 100000) / 100000.0 * 360.0);
  FGTexturedInstrument::draw();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTurnCoordinator.
////////////////////////////////////////////////////////////////////////

// TODO: add slip/skid ball

FGTurnCoordinator::FGTurnCoordinator (int x, int y)
  : FGTexturedInstrument(x, y, SIX_W, SIX_W)
{
  addLayer(0, "Textures/Panel/turn-bg.rgb");
  addLayer(1, "Textures/Panel/turn.rgb");
  addLayer(2, "Textures/Panel/ball.rgb");
}

FGTurnCoordinator::~FGTurnCoordinator ()
{
}

void
FGTurnCoordinator::draw () const
{
				// Set little plane
				// FIXME: this should be turn, maybe
  double rot = get_roll() * RAD_TO_DEG;
  if (rot > 30.0)
    rot = 30.0;
  else if (rot < -30.0)
    rot = -30.0;
  setLayerRot(1, rot);

				// Set ball
				// FIXME: totally bogus values
  double slip = get_sideslip() * 450;
  if (slip > 45) {
    slip = 45;
  } else if (slip < -45) {
    slip = -45;
  }
  setLayerRot(2, 0 - slip);

  FGTexturedInstrument::draw();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGGyroCompass.
////////////////////////////////////////////////////////////////////////

// TODO: add heading bug

FGGyroCompass::FGGyroCompass (int x, int y)
  : FGTexturedInstrument(x, y, SIX_W, SIX_W)
{
  addLayer(0, "Textures/Panel/gyro-bg.rgb");
  addLayer(1, "Textures/Panel/bug.rgb");
  addLayer(2, "Textures/Panel/gyro-fg.rgb");
}

FGGyroCompass::~FGGyroCompass ()
{
}

void
FGGyroCompass::draw () const
{
  setLayerRot(0, 0.0 - get_heading());
  setLayerRot(1, 0.0 - get_heading() + fgAPget_TargetHeading());
  FGTexturedInstrument::draw();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGVerticalVelocity.
////////////////////////////////////////////////////////////////////////

FGVerticalVelocity::FGVerticalVelocity (int x, int y)
  : FGTexturedInstrument(x, y, SIX_W, SIX_W)
{
  addLayer(0, "Textures/Panel/vertical.rgb");
  addLayer(1, "Textures/Panel/long-needle.rgb");
}

FGVerticalVelocity::~FGVerticalVelocity ()
{
}

void
FGVerticalVelocity::draw () const
{
  double climb = get_climb_rate() / 500.0;
  if (climb < -4.0) {
    climb = -4.0;
  } else if (climb > 4.0) {
    climb = 4.0;
  }
  double rot = (climb * 42.0) + 270.0;
				// FIXME: why inverted?
  setLayerRot(1, rot);
  FGTexturedInstrument::draw();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGRPMGauge.
////////////////////////////////////////////////////////////////////////

FGRPMGauge::FGRPMGauge (int x, int y)
  : FGTexturedInstrument(x, y, SMALL_W, SMALL_W)
{
  addLayer(0, "Textures/Panel/rpm.rgb");
  addLayer(1, "Textures/Panel/long-needle.rgb");
}

FGRPMGauge::~FGRPMGauge ()
{
}

void
FGRPMGauge::draw () const
{
  double rot = get_throttleval() * 300 - 150;
  setLayerRot(1, rot);
  FGTexturedInstrument::draw();
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGFlapIndicator.
////////////////////////////////////////////////////////////////////////

FGFlapIndicator::FGFlapIndicator (int x, int y)
  : FGTexturedInstrument(x, y, SMALL_W, SMALL_W)
{
  addLayer(0, "Textures/Panel/flaps.rgb");
  addLayer(1, "Textures/Panel/long-needle.rgb");
  setLayerCenter(1, 0 - (SMALL_W / 4) + (SMALL_W / 16), 0);
}

FGFlapIndicator::~FGFlapIndicator ()
{
}

void
FGFlapIndicator::draw () const
{
  double rot = controls.get_flaps() * 120 + 30;
  setLayerRot(1, rot);
  FGTexturedInstrument::draw();
}



// end of panel.cxx
