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
#include <GL/glut.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/fgpath.hxx>
#include <Main/options.hxx>
#include <Main/views.hxx>
#include <Main/bfi.hxx>
#include <Objects/texload.h>
#include <Autopilot/autopilot.hxx>
#include <Time/fg_time.hxx>

#include "cockpit.hxx"
#include "panel.hxx"
#include "hud.hxx"
#include "steam.hxx"

extern fgAPDataPtr APDataGlobal;

#define SIX_X 200
#define SIX_Y 345
#define SIX_W 128
#define SIX_SPACING (SIX_W + 5)
#define SMALL_W 112



////////////////////////////////////////////////////////////////////////
// Static functions for obtaining settings.
//
// These should be replaced with functions from a global facade,
// or BFI (Big Friendly Interface).
////////////////////////////////////////////////////////////////////////

static char * panelGetTime (char * buf)
{
  struct tm * t = FGTime::cur_time_params->getGmt();
  sprintf(buf, " %.2d:%.2d:%.2d",
	  t->tm_hour, t->tm_min, t->tm_sec);
  return buf;
}



////////////////////////////////////////////////////////////////////////
// Static factory functions to create textured gauges.
//
// These will be replaced first with a giant table, and then with
// configuration files read from an external source, but for now
// they're hard-coded.
////////////////////////////////////////////////////////////////////////


#define createTexture(a) FGTextureManager::createTexture(a)

/**
 * Construct an airspeed indicator for a single-engine prop.
 */
static FGPanelInstrument *
createAirspeedIndicator (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Layer 0: gauge background.
  inst->addLayer(0, createTexture("Textures/Panel/airspeed.rgb"));

				// Layer 1: needle.
				// Rotates with airspeed.
  inst->addLayer(1, createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGSteam::get_ASI_kias,
			  30.0, 220.0, 36.0 / 20.0, -54.0);
  return inst;
}


/**
 * Construct an artificial horizon.
 */
static FGPanelInstrument *
createHorizon (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Layer 0: coloured background
				// moves with roll only
  inst->addLayer(0, createTexture("Textures/Panel/horizon-bg.rgb"));
  inst->addTransformation(0, FGInstrumentLayer::ROTATION,
			  FGBFI::getRoll,
			  -360.0, 360.0, -1.0, 0.0);

				// Layer 1: floating horizon
				// moves with roll and pitch
  inst->addLayer(1, createTexture("Textures/Panel/horizon-float.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGBFI::getRoll,
			  -360.0, 360.0, -1.0, 0.0);
  inst->addTransformation(1, FGInstrumentLayer::YSHIFT,
			  FGBFI::getPitch,
			  -20.0, 20.0, -(1.5 / 160.0) * SIX_W, 0.0);

				// Layer 2: rim
				// moves with roll only
  inst->addLayer(2, createTexture("Textures/Panel/horizon-rim.rgb"));
  inst->addTransformation(2, FGInstrumentLayer::ROTATION,
			  FGBFI::getRoll,
			  -360.0, 360.0, -1.0, 0.0);

				// Layer 3: glass front of gauge
				// fixed, with markings
  inst->addLayer(3, createTexture("Textures/Panel/horizon-fg.rgb"));

  return inst;
}


/**
 * Construct an altimeter.
 */
static FGPanelInstrument *
createAltimeter (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Layer 0: gauge background
  inst->addLayer(0, createTexture("Textures/Panel/altimeter.rgb"));

				// Layer 1: hundreds needle (long)
				// moves with altitude
  inst->addLayer(1, createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGSteam::get_ALT_ft,
			  0.0, 100000.0, 360.0 / 1000.0, 0.0);

				// Layer 2: thousands needle (short)
				// moves with altitude
  inst->addLayer(2, createTexture("Textures/Panel/short-needle.rgb"));
  inst->addTransformation(2, FGInstrumentLayer::ROTATION,
			  FGSteam::get_ALT_ft,
			  0.0, 100000.0, 360.0 / 10000.0, 0.0);

				// Layer 3: ten thousands bug (outside)
				// moves with altitude
  inst->addLayer(3, createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(3, FGInstrumentLayer::ROTATION,
			  FGSteam::get_ALT_ft,
			  0.0, 100000.0, 360.0 / 100000.0, 0.0);

  return inst;
}


/**
 * Construct a turn coordinator.
 */
static FGPanelInstrument *
createTurnCoordinator (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Layer 0: background
  inst->addLayer(0, createTexture("Textures/Panel/turn-bg.rgb"));

				// Layer 1: little plane
				// moves with roll
  inst->addLayer(1, createTexture("Textures/Panel/turn.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGSteam::get_TC_radps,
			  -30.0, 30.0, 1.0, 0.0);

				// Layer 2: little ball
				// moves with slip/skid
  inst->addLayer(2, createTexture("Textures/Panel/ball.rgb"));
  inst->addTransformation(2, FGInstrumentLayer::ROTATION,
			  FGSteam::get_TC_rad,
			  -0.1, 0.1, 450.0, 0.0);

  return inst;
}


/**
 * Construct a gyro compass.
 */
static FGPanelInstrument *
createGyroCompass (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Action: move bug counter-clockwise
  inst->addAction(SIX_W/2 - SIX_W/5, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getAPHeading,
				     FGBFI::setAPHeading,
				     -1.0, 0.0, 360.0, true));

				// Action: move bug clockwise
  inst->addAction(SIX_W/2 - SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getAPHeading,
				     FGBFI::setAPHeading,
				     1.0, 0.0, 360.0, true));

				// Layer 0: compass background
				// rotates with heading
  inst->addLayer(0, createTexture("Textures/Panel/gyro-bg.rgb"));
  inst->addTransformation(0, FGInstrumentLayer::ROTATION,
			  FGSteam::get_DG_deg,
			  -360.0, 360.0, -1.0, 0.0);

				// Layer 1: heading bug
				// rotates with heading and AP heading
  inst->addLayer(1, createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGSteam::get_DG_deg,
			  -360.0, 360.0, -1.0, 0.0);
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGBFI::getAPHeading,
			  -360.0, 360.0, 1.0, 0.0);

				// Layer 2: fixed center
  inst->addLayer(2, createTexture("Textures/Panel/gyro-fg.rgb"));

				// Layer 3: heading knob
				// rotates with AP heading
  inst->addLayer(3, createTexture("Textures/Panel/heading-knob.rgb"));
  inst->addTransformation(3, FGInstrumentLayer::XSHIFT, SIX_W/2 - 10); 
  inst->addTransformation(3, FGInstrumentLayer::YSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(3, FGInstrumentLayer::ROTATION,
			  FGBFI::getAPHeading,
			  -360.0, 360.0, 1.0, 0.0);

  return inst;
}


/**
 * Construct a vertical velocity indicator.
 */
static FGPanelInstrument *
createVerticalVelocity (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Layer 0: gauge background
  inst->addLayer(0, createTexture("Textures/Panel/vertical.rgb"));

				// Layer 1: needle
				// moves with vertical velocity
  inst->addLayer(1, createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGSteam::get_VSI_fps,
			  -2000.0, 2000.0, 42.0/500.0, 270.0);

  return inst;
}


/**
 * Construct an RPM gauge.
 */
static FGPanelInstrument *
createRPMGauge (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SMALL_W, SMALL_W);

				// Layer 0: gauge background
  inst->addLayer(0, createTexture("Textures/Panel/rpm.rgb"));

				// Layer 1: long needle
				// FIXME: moves with throttle (for now)
  inst->addLayer(1, createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGBFI::getThrottle,
			  0.0, 100.0, 300.0, -150.0);

  return inst;
}


/**
 * Construct a flap position indicator.
 */
static FGPanelInstrument *
createFlapIndicator (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SMALL_W, SMALL_W);

				// Layer 0: gauge background
  inst->addLayer(0, createTexture("Textures/Panel/flaps.rgb"));

				// Layer 1: long needle
				// shifted over, rotates with flap position
  inst->addLayer(1, createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::XSHIFT,
			  -(SMALL_W / 4) + (SMALL_W / 16));
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGBFI::getFlaps,
			  0.0, 1.0, 120.0, 30.0);

  return inst;
}

static FGPanelInstrument *
createChronometer (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SMALL_W, SMALL_W);

				// Layer 0: gauge background
  inst->addLayer(0, createTexture("Textures/Panel/clock.rgb"));

				// Layer 1: text
				// displays current GMT
  FGCharInstrumentLayer * text =
    new FGCharInstrumentLayer(panelGetTime,
			      SMALL_W, SMALL_W, 1);
  text->setPointSize(14);
  text->setColor(0.2, 0.2, 0.2);
  inst->addLayer(text);
  inst->addTransformation(1, FGInstrumentLayer::XSHIFT, SMALL_W * -0.38);
  inst->addTransformation(1, FGInstrumentLayer::YSHIFT, SMALL_W * -0.06);

  return inst;
}


/**
 * Construct control-position indicators.
 */
static FGPanelInstrument *
createControls (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SMALL_W, SMALL_W);

				// Layer 0: gauge background
  inst->addLayer(0, createTexture("Textures/Panel/controls.rgb"));

				// Layer 1: bug
				// moves left-right with aileron
  inst->addLayer(1, createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::XSHIFT, FGBFI::getAileron,
			  -1.0, 1.0, SMALL_W * .75 / 2.0, 0.0);

				// Layer 2: bug
				// moves left-right with rudder
  inst->addLayer(2, createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(2, FGInstrumentLayer::ROTATION, 180.0);
  inst->addTransformation(2, FGInstrumentLayer::XSHIFT, FGBFI::getRudder,
			  -1.0, 1.0, -SMALL_W * .75 / 2.0, 0.0);

				// Layer 3: bug
				// moves up-down with elevator trim
  inst->addLayer(3, createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(3, FGInstrumentLayer::ROTATION, 270.0);
  inst->addTransformation(3, FGInstrumentLayer::YSHIFT,
			  -SMALL_W * (3.0 / 8.0));
  inst->addTransformation(3, FGInstrumentLayer::XSHIFT, FGBFI::getElevatorTrim,
			  -1.0, 1.0, SMALL_W * .75 / 2.0, 0.0);

				// Layer 4: bug
				// moves up-down with elevator
  inst->addLayer(4, createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(4, FGInstrumentLayer::ROTATION, 90.0);
  inst->addTransformation(4, FGInstrumentLayer::YSHIFT,
			  -SMALL_W * (3.0 / 8.0));
  inst->addTransformation(4, FGInstrumentLayer::XSHIFT, FGBFI::getElevator,
			  -1.0, 1.0, -SMALL_W * .75 / 2.0, 0.0);

  return inst;
}


/**
 * Construct a NAV1 gauge (hardwired).
 */
static FGPanelInstrument *
createNAV1 (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Action: increase selected radial
  inst->addAction(SIX_W/2 - SIX_W/5, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV1SelRadial,
				     FGBFI::setNAV1SelRadial,
				     1.0, 0.0, 360.0, true));

				// Action: decrease selected radial
  inst->addAction(SIX_W/2 - SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV1SelRadial,
				     FGBFI::setNAV1SelRadial,
				     -1.0, 0.0, 360.0, true));

				// Layer 0: background
  inst->addLayer(0, createTexture("Textures/Panel/gyro-bg.rgb"));
  inst->addTransformation(0, FGInstrumentLayer::ROTATION,
			  FGBFI::getNAV1SelRadial,
			  -360.0, 360.0, -1.0, 0.0);

				// Layer 1: left-right needle.
  inst->addLayer(1, createTexture("Textures/Panel/nav-needle.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::XSHIFT,
			  FGSteam::get_HackVOR1_deg,
			  -10.0, 10.0, SIX_W / 40.0, 0.0);

				// Layer 2: glidescope needle
  inst->addLayer(2, createTexture("Textures/Panel/nav-needle.rgb"));
  inst->addTransformation(2, FGInstrumentLayer::YSHIFT,
			  FGSteam::get_HackGS_deg,
			  -1.0, 1.0, SIX_W / 5.0, 0.0);
  inst->addTransformation(2, FGInstrumentLayer::ROTATION,
			  90 );

				// Layer 3: face with markings
  inst->addLayer(3, createTexture("Textures/Panel/nav-face.rgb"));

				// Layer 4: heading knob
				// rotates with selected radial
  inst->addLayer(4, createTexture("Textures/Panel/heading-knob.rgb"));
  inst->addTransformation(4, FGInstrumentLayer::XSHIFT, SIX_W/2 - 10); 
  inst->addTransformation(4, FGInstrumentLayer::YSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(4, FGInstrumentLayer::ROTATION,
			  FGBFI::getNAV1SelRadial,
			  -360.0, 360.0, -1.0, 0.0);

  return inst;
}


/**
 * Construct a NAV2 gauge.
 */
static FGPanelInstrument *
createNAV2 (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Action: increase selected radial
  inst->addAction(SIX_W/2 - SIX_W/5, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV2SelRadial,
				     FGBFI::setNAV2SelRadial,
				     1.0, 0.0, 360.0, true));

				// Action: decrease selected radial
  inst->addAction(SIX_W/2 - SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV2SelRadial,
				     FGBFI::setNAV2SelRadial,
				     -1.0, 0.0, 360.0, true));

				// Layer 0: background
  inst->addLayer(0, createTexture("Textures/Panel/gyro-bg.rgb"));
  inst->addTransformation(0, FGInstrumentLayer::ROTATION,
			  FGBFI::getNAV2SelRadial,
			  -360.0, 360.0, -1.0, 0.0);

				// Layer 1: left-right needle.
  inst->addLayer(1, createTexture("Textures/Panel/nav-needle.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::XSHIFT,
			  FGSteam::get_HackVOR2_deg,
			  -10.0, 10.0, SIX_W / 40.0, 0.0);
//   inst->addTransformation(1, FGInstrumentLayer::YSHIFT,
// 			  -SIX_W / 4.4 );

				// Layer 2: face with markings.
  inst->addLayer(2, createTexture("Textures/Panel/nav-face.rgb"));

				// Layer 3: heading knob
				// rotates with selected radial
  inst->addLayer(3, createTexture("Textures/Panel/heading-knob.rgb"));
  inst->addTransformation(3, FGInstrumentLayer::XSHIFT, SIX_W/2 - 10); 
  inst->addTransformation(3, FGInstrumentLayer::YSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(3, FGInstrumentLayer::ROTATION,
			  FGBFI::getNAV2SelRadial,
			  -360.0, 360.0, -1.0, 0.0);

  return inst;
}


/**
 * Construct an ADF gauge (hardwired).
 */
static FGPanelInstrument *
createADF (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Action: increase selected rotation
  inst->addAction(SIX_W/2 - SIX_W/5, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getADFRotation,
				     FGBFI::setADFRotation,
				     1.0, 0.0, 360.0, true));

				// Action: decrease selected rotation
  inst->addAction(SIX_W/2 - SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getADFRotation,
				     FGBFI::setADFRotation,
				     -1.0, 0.0, 360.0, true));

				// Layer 0: background
  inst->addLayer(0, createTexture("Textures/Panel/gyro-bg.rgb"));
  inst->addTransformation(0, FGInstrumentLayer::ROTATION,
			  FGBFI::getADFRotation,
			  0.0, 360.0, 1.0, 0.0);

				// Layer 1: Direction needle.
  inst->addLayer(1, createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(1, FGInstrumentLayer::ROTATION,
			  FGSteam::get_HackADF_deg,
			  -720.0, 720.0, 1.0, 0.0);

				// Layer 2: heading knob
				// rotates with selected radial
  inst->addLayer(2, createTexture("Textures/Panel/heading-knob.rgb"));
  inst->addTransformation(2, FGInstrumentLayer::XSHIFT, SIX_W/2 - 10); 
  inst->addTransformation(2, FGInstrumentLayer::YSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(2, FGInstrumentLayer::ROTATION,
			  FGBFI::getADFRotation,
			  -360.0, 360.0, -1.0, 0.0);
  return inst;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextureManager.
////////////////////////////////////////////////////////////////////////

map<const char *,ssgTexture *> FGTextureManager::_textureMap;

ssgTexture *
FGTextureManager::createTexture (const char * relativePath)
{
  ssgTexture *texture;

  texture = _textureMap[relativePath];
  if (texture == 0) {
    FGPath tpath(current_options.get_fg_root());
    tpath.append(relativePath);
    texture = new ssgTexture((char *)tpath.c_str(), false, false);
    _textureMap[relativePath] = texture;
    cerr << "Created texture " << relativePath
	 << " handle=" << texture->getHandle() << endl;
  }

  return texture;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGPanel.
////////////////////////////////////////////////////////////////////////

FGPanel current_panel;

FGPanel::FGPanel ()
  : _initialized(false),
    _visibility(false)
{
}

FGPanel::~FGPanel ()
{
  instrument_list_type::iterator current = _instruments.begin();
  instrument_list_type::iterator last = _instruments.end();
  
  for ( ; current != last; ++current) {
    delete *current;
    *current = 0;
  }
}

void
FGPanel::addInstrument (FGPanelInstrument * instrument)
{
  _instruments.push_back(instrument);
}

void
FGPanel::init (int x, int y, int finx, int finy)
{
  _x = x;
  _y = y;
  _w = finx - x;
  _h = finy - y;
  _panel_h = (int)((finy - y) * 0.5768 + 1);

				// Don't reconstruct all of the
				// instruments.
  if (_initialized)
    return;

  x = SIX_X;
  y = SIX_Y;

  _bg = createTexture("Textures/Panel/panel-bg.rgb");

				// Chronometer alone at side
  x = SIX_X - SIX_SPACING - 8;
  addInstrument(createChronometer(x, y));

				// Top row
  x = SIX_X;
  addInstrument(createAirspeedIndicator(x, y));
  x += SIX_SPACING;
  addInstrument(createHorizon(x, y));
  x += SIX_SPACING;
  addInstrument(createAltimeter(x, y));
  x += SIX_SPACING + 20;
  addInstrument(createNAV1(x, y));

				// Middle row
  x = SIX_X;
  y -= SIX_SPACING;
  addInstrument(createTurnCoordinator(x, y));
  x += SIX_SPACING;
  addInstrument(createGyroCompass(x, y));
  x += SIX_SPACING;
  addInstrument(createVerticalVelocity(x, y));
  x += SIX_SPACING + 20;
  addInstrument(createNAV2(x, y));

				// Bottom row
  x = SIX_X;
  y -= SIX_SPACING + 10;
  addInstrument(createControls(x, y));
  x += SIX_SPACING;
  addInstrument(createFlapIndicator(x, y));
  x += SIX_SPACING;
  addInstrument(createRPMGauge(x, y));
  x += SIX_SPACING + 20;
  y += 10;
  addInstrument(createADF(x, y));
}

void
FGPanel::update () const
{
				// Do nothing if the panel isn't visible.
  if (!_visibility)
    return;

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(_x, _x + _w, _y, _y + _h);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

				// Draw the background
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glEnable(GL_ALPHA_TEST);
  glEnable(GL_COLOR_MATERIAL);
  glColor4f(1.0, 1.0, 1.0, 1.0);
  glBindTexture(GL_TEXTURE_2D, _bg->getHandle());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glBegin(GL_POLYGON);
  glTexCoord2f(0.0, 0.0); glVertex3f(_x, _y, 0);
  glTexCoord2f(10.0, 0.0); glVertex3f(_x + _w, _y, 0);
  glTexCoord2f(10.0, 5.0); glVertex3f(_x + _w, _y + _panel_h, 0);
  glTexCoord2f(0.0, 5.0); glVertex3f(_x, _y + _panel_h, 0);
  glEnd();

				// Draw the instruments.
  instrument_list_type::const_iterator current = _instruments.begin();
  instrument_list_type::const_iterator end = _instruments.end();

  for ( ; current != end; current++) {
    FGPanelInstrument * instr = *current;
    glLoadIdentity();
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

void
FGPanel::setVisibility (bool visibility)
{
  _visibility = visibility;
}

bool
FGPanel::getVisibility () const
{
  return _visibility;
}

bool
FGPanel::doMouseAction (int button, int updown, int x, int y)
{
				// For now, ignore the release
  if (updown == 1) 
    return true;

  x = (int)(((float)x / current_view.get_winWidth()) * _w);
  y = (int)(_h - (((float)y / current_view.get_winHeight()) * _h));

  for (int i = 0; i < _instruments.size(); i++) {
    FGPanelInstrument *inst = _instruments[i];
    int ix = inst->getXPos();
    int iy = inst->getYPos();
    int iw = inst->getWidth() / 2;
    int ih = inst->getHeight() / 2;
    if (x >= ix - iw && x < ix + iw && y >= iy - ih && y < iy + ih) {
      cout << "Do mouse action for component " << i << '\n';
      return inst->doMouseAction(button, updown, x - ix, y - iy);
    }
  }
  cout << "Did not click on an instrument\n";
  return false;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGAdjustAction.
////////////////////////////////////////////////////////////////////////

FGAdjustAction::FGAdjustAction (getter_type getter, setter_type setter,
				double increment, double min, double max,
				bool wrap=false)
  : _getter(getter), _setter(setter), _increment(increment),
    _min(min), _max(max), _wrap(wrap)
{
}

FGAdjustAction::~FGAdjustAction ()
{
}

void
FGAdjustAction::doAction ()
{
  double value = (*_getter)();
  cout << "Do action; value=" << value << '\n';
  value += _increment;
  if (value < _min) {
    value = (_wrap ? _max : _min);
  } else if (value > _max) {
    value = (_wrap ? _min : _max);
  }
  cout << "New value is " << value << '\n';
  (*_setter)(value);
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
  action_list_type::iterator it = _actions.begin();
  action_list_type::iterator last = _actions.end();
  for ( ; it != last; it++) {
    delete it->action;
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
FGPanelInstrument::addAction (int x, int y, int w, int h,
			      FGPanelAction * action)
{
  FGPanelInstrument::inst_action act;
  act.x = x;
  act.y = y;
  act.w = w;
  act.h = h;
  act.action = action;
  _actions.push_back(act);
}

				// Coordinates relative to centre.
bool
FGPanelInstrument::doMouseAction (int button, int updown, int x, int y)
{
  action_list_type::iterator it = _actions.begin();
  action_list_type::iterator last = _actions.end();
  cout << "Mouse action at " << x << ',' << y << '\n';
  for ( ; it != last; it++) {
    cout << "Trying action at " << it->x << ',' << it->y << ','
	 << it->w <<',' << it->h << '\n';
    if (x >= it->x && x < it->x + it->w && y >= it->y && y < it->y + it->h) {
      it->action->doAction();
      return true;
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
  // FIXME: free layers
}

void
FGLayeredInstrument::draw () const
{
  layer_list::const_iterator it = _layers.begin();
  layer_list::const_iterator last = _layers.end();
  while (it != last) {
    (*it)->draw();
    it++;
  }
}

void
FGLayeredInstrument::addLayer (FGInstrumentLayer *layer)
{
  _layers.push_back(layer);
}

void
FGLayeredInstrument::addLayer (int layer, ssgTexture * texture)
{
  addLayer(new FGTexturedInstrumentLayer(texture, _w, _h, layer));
}

void
FGLayeredInstrument::addTransformation (int layer,
					FGInstrumentLayer::transform_type type,
					FGInstrumentLayer::transform_func func,
					double min, double max,
					double factor, double offset)
{
  _layers[layer]->addTransformation(type, func, min, max, factor, offset);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGInstrumentLayer.
////////////////////////////////////////////////////////////////////////

FGInstrumentLayer::FGInstrumentLayer (int w, int h, int z)
  : _w(w),
    _h(h),
    _z(z)
{
}

FGInstrumentLayer::~FGInstrumentLayer ()
{
  transformation_list::iterator it = _transformations.begin();
  transformation_list::iterator end = _transformations.end();
  while (it != end) {
    delete *it;
    it++;
  }
}

void
FGInstrumentLayer::transform () const
{
  glTranslatef(0.0, 0.0, (_z / 100.0) + 0.1);

  transformation_list::const_iterator it = _transformations.begin();
  transformation_list::const_iterator last = _transformations.end();
  while (it != last) {
    transformation *t = *it;
    double value = (t->func == 0 ? 0.0 : (*(t->func))());
    if (value < t->min) {
      value = t->min;
    } else if (value > t->max) {
      value = t->max;
    }
    value = value * t->factor + t->offset;

    switch (t->type) {
    case XSHIFT:
      glTranslatef(value, 0.0, 0.0);
      break;
    case YSHIFT:
      glTranslatef(0.0, value, 0.0);
      break;
    case ROTATION:
      glRotatef(-value, 0.0, 0.0, 1.0);
      break;
    }
    it++;
  }
}

void
FGInstrumentLayer::addTransformation (transform_type type,
				      transform_func func,
				      double min, double max,
				      double factor, double offset)
{
  transformation *t = new transformation;
  t->type = type;
  t->func = func;
  t->min = min;
  t->max = max;
  t->factor = factor;
  t->offset = offset;
  _transformations.push_back(t);
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTexturedInstrumentLayer.
////////////////////////////////////////////////////////////////////////

// FGTexturedInstrumentLayer::FGTexturedInstrumentLayer (const char *tname,
// 						      int w, int h, int z)
//   : FGInstrumentLayer(w, h, z)
// {
//   setTexture(tname);
// }

FGTexturedInstrumentLayer::FGTexturedInstrumentLayer (ssgTexture * texture,
						      int w, int h, int z)
  : FGInstrumentLayer(w, h, z)
{
  setTexture(texture);
}

FGTexturedInstrumentLayer::~FGTexturedInstrumentLayer ()
{
}

void
FGTexturedInstrumentLayer::draw () const
{
  int w2 = _w / 2;
  int h2 = _h / 2;

  glPushMatrix();
  transform();
  glBindTexture(GL_TEXTURE_2D, _texture->getHandle());
  glBegin(GL_POLYGON);
				// FIXME: is this really correct
				// for layering?
  glTexCoord2f(0.0, 0.0); glVertex2f(-w2, -h2);
  glTexCoord2f(1.0, 0.0); glVertex2f(w2, -h2);
  glTexCoord2f(1.0, 1.0); glVertex2f(w2, h2);
  glTexCoord2f(0.0, 1.0); glVertex2f(-w2, h2);
  glEnd();
  glPopMatrix();
}

// void
// FGTexturedInstrumentLayer::setTexture (const char *textureName)
// {
//   FGPath tpath(current_options.get_fg_root());
//   tpath.append(textureName);
//   ssgTexture * texture = new ssgTexture((char *)tpath.c_str(), false, false);
//   setTexture(texture);
// }



////////////////////////////////////////////////////////////////////////
// Implementation of FGCharInstrumentLayer.
////////////////////////////////////////////////////////////////////////

FGCharInstrumentLayer::FGCharInstrumentLayer (text_func func,
					      int w, int h, int z)
  : FGInstrumentLayer(w, h, z),
    _func(func)
{
  _renderer.setFont(guiFntHandle);
  _renderer.setPointSize(14);
  _color[0] = _color[1] = _color[2] = 0.0;
  _color[3] = 1.0;
}

FGCharInstrumentLayer::~FGCharInstrumentLayer ()
{
}

void
FGCharInstrumentLayer::draw () const
{
  glPushMatrix();
  glColor4fv(_color);
  transform();
  _renderer.begin();
  _renderer.start3f(0, 0, 0);
  _renderer.puts((*_func)(_buf));
  _renderer.end();
  glColor4f(1.0, 1.0, 1.0, 1.0);	// FIXME
  glPopMatrix();
}

void
FGCharInstrumentLayer::setColor (float r, float g, float b)
{
  _color[0] = r;
  _color[1] = g;
  _color[2] = b;
  _color[3] = 1.0;
}

void
FGCharInstrumentLayer::setPointSize (const float size)
{
  _renderer.setPointSize(size);
}

void
FGCharInstrumentLayer::setFont(fntFont * font)
{
  _renderer.setFont(font);
}



// end of panel.cxx
