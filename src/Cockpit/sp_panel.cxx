//  sp_panel.cxx - default, 2D single-engine prop instrument panel
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

#include <simgear/debug/logstream.hxx>
#include <Main/bfi.hxx>
#include <Time/fg_time.hxx>

#include "panel.hxx"
#include "steam.hxx"

				// Macros for instrument sizes
				// (these aren't used consistently
				// anyway, and should probably be
				// removed).
#define SIX_X 200
#define SIX_Y 345
#define SIX_W 128
#define SIX_SPACING (SIX_W + 5)
#define SMALL_W 112



////////////////////////////////////////////////////////////////////////
// Static functions for obtaining settings.
////////////////////////////////////////////////////////////////////////

static char * panelGetTime ()
{
  static char buf[1024];	// FIXME: not thread-safe
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
  inst->addLayer(createTexture("Textures/Panel/airspeed.rgb"));

				// Layer 1: needle.
				// Rotates with airspeed.
  inst->addLayer(createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
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
  inst->addLayer(createTexture("Textures/Panel/horizon-bg.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getRoll,
			  -360.0, 360.0, -1.0, 0.0);

				// Layer 1: floating horizon
				// moves with roll and pitch
  inst->addLayer(createTexture("Textures/Panel/horizon-float.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getRoll,
			  -360.0, 360.0, -1.0, 0.0);
  inst->addTransformation(FGInstrumentLayer::YSHIFT,
			  FGBFI::getPitch,
			  -20.0, 20.0, -(1.5 / 160.0) * SIX_W, 0.0);

				// Layer 2: rim
				// moves with roll only
  inst->addLayer(createTexture("Textures/Panel/horizon-rim.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getRoll,
			  -360.0, 360.0, -1.0, 0.0);

				// Layer 3: glass front of gauge
				// fixed, with markings
  inst->addLayer(createTexture("Textures/Panel/horizon-fg.rgb"));

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
  inst->addLayer(createTexture("Textures/Panel/altimeter.rgb"));

				// Layer 1: hundreds needle (long)
				// moves with altitude
  inst->addLayer(createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGSteam::get_ALT_ft,
			  0.0, 100000.0, 360.0 / 1000.0, 0.0);

				// Layer 2: thousands needle (short)
				// moves with altitude
  inst->addLayer(createTexture("Textures/Panel/short-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGSteam::get_ALT_ft,
			  0.0, 100000.0, 360.0 / 10000.0, 0.0);

				// Layer 3: ten thousands bug (outside)
				// moves with altitude
  inst->addLayer(createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
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
  inst->addLayer(createTexture("Textures/Panel/turn-bg.rgb"));

				// Layer 1: little plane
				// moves with roll
  inst->addLayer(createTexture("Textures/Panel/turn.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGSteam::get_TC_std,
			  -2.5, 2.5, 20.0, 0.0);

				// Layer 2: little ball
				// moves with slip/skid
  inst->addLayer(createTexture("Textures/Panel/ball.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGSteam::get_TC_rad,
			  -0.1, 0.1, -450.0, 0.0);

  return inst;
}


/**
 * Construct a gyro compass.
 */
static FGPanelInstrument *
createGyroCompass (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Action: move heading bug
  inst->addAction(0, SIX_W/2 - SIX_W/5, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getAPHeading,
				     FGBFI::setAPHeading,
				     -1.0, -360.0, 360.0, true));
  inst->addAction(0, SIX_W/2 - SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getAPHeading,
				     FGBFI::setAPHeading,
				     1.0, -360.0, 360.0, true));
  inst->addAction(1, SIX_W/2 - SIX_W/5, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getAPHeading,
				     FGBFI::setAPHeading,
				     -5.0, -360.0, 360.0, true));
  inst->addAction(1, SIX_W/2 - SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getAPHeading,
				     FGBFI::setAPHeading,
				     5.0, -360.0, 360.0, true));

				// Layer 0: compass background
				// rotates with heading
  inst->addLayer(createTexture("Textures/Panel/gyro-bg.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGSteam::get_MH_deg,
			  -720.0, 720.0, -1.0, 0.0);

				// Layer 1: heading bug
				// rotates with heading and AP heading
  inst->addLayer(createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGSteam::get_MH_deg,
			  -720.0, 720.0, -1.0, 0.0);
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getAPHeading,
			  -720.0, 720.0, 1.0, 0.0);

				// Layer 2: fixed center
  inst->addLayer(createTexture("Textures/Panel/gyro-fg.rgb"));

				// Layer 3: heading knob
				// rotates with AP heading
  inst->addLayer(new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
				     SIX_W/4, SIX_W/4, 0.0, 0.25, 0.25, 0.5));
  inst->addTransformation(FGInstrumentLayer::XSHIFT, SIX_W/2 - 10); 
  inst->addTransformation(FGInstrumentLayer::YSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(FGInstrumentLayer::ROTATION,
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
  inst->addLayer(createTexture("Textures/Panel/vertical.rgb"));

				// Layer 1: needle
				// moves with vertical velocity
  inst->addLayer(createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
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
  inst->addLayer(createTexture("Textures/Panel/rpm.rgb"));

				// Layer 1: long needle
				// FIXME: moves with throttle (for now)
  inst->addLayer(createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
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
  inst->addLayer(createTexture("Textures/Panel/flaps.rgb"));

				// Layer 1: long needle
				// shifted over, rotates with flap position
  inst->addLayer(createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::XSHIFT,
			  -(SMALL_W / 4) + (SMALL_W / 16));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getFlaps,
			  0.0, 1.0, 120.0, 30.0);

  return inst;
}

static FGPanelInstrument *
createChronometer (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SMALL_W, SMALL_W);

				// Layer 0: gauge background
  inst->addLayer(createTexture("Textures/Panel/clock.rgb"));

				// Layer 1: text
				// displays current GMT
  FGTextLayer * text = new FGTextLayer(SMALL_W, SMALL_W);
  text->addChunk(new FGTextLayer::Chunk(panelGetTime));
  text->setPointSize(14);
  text->setColor(0.2, 0.2, 0.2);
  inst->addLayer(text);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, SMALL_W * -0.38);
  inst->addTransformation(FGInstrumentLayer::YSHIFT, SMALL_W * -0.06);

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
  inst->addLayer(createTexture("Textures/Panel/controls.rgb"));

				// Layer 1: bug
				// moves left-right with aileron
  inst->addLayer(createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(FGInstrumentLayer::XSHIFT, FGBFI::getAileron,
			  -1.0, 1.0, SMALL_W * .75 / 2.0, 0.0);

				// Layer 2: bug
				// moves left-right with rudder
  inst->addLayer(createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION, 180.0);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, FGBFI::getRudder,
			  -1.0, 1.0, -SMALL_W * .75 / 2.0, 0.0);

				// Layer 3: bug
				// moves up-down with elevator trim
  inst->addLayer(createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION, 270.0);
  inst->addTransformation(FGInstrumentLayer::YSHIFT,
			  -SMALL_W * (3.0 / 8.0));
  inst->addTransformation(FGInstrumentLayer::XSHIFT, FGBFI::getElevatorTrim,
			  -1.0, 1.0, SMALL_W * .75 / 2.0, 0.0);

				// Layer 4: bug
				// moves up-down with elevator
  inst->addLayer(createTexture("Textures/Panel/bug.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION, 90.0);
  inst->addTransformation(FGInstrumentLayer::YSHIFT,
			  -SMALL_W * (3.0 / 8.0));
  inst->addTransformation(FGInstrumentLayer::XSHIFT, FGBFI::getElevator,
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

				// Action: change selected radial.
  inst->addAction(0, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV1SelRadial,
				     FGBFI::setNAV1SelRadial,
				     1.0, 0.0, 360.0, true));
  inst->addAction(0, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV1SelRadial,
				     FGBFI::setNAV1SelRadial,
				     -1.0, 0.0, 360.0, true));
  inst->addAction(1, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV1SelRadial,
				     FGBFI::setNAV1SelRadial,
				     5.0, 0.0, 360.0, true));
  inst->addAction(1, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV1SelRadial,
				     FGBFI::setNAV1SelRadial,
				     -5.0, 0.0, 360.0, true));

				// Layer 0: background
  inst->addLayer(createTexture("Textures/Panel/gyro-bg.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getNAV1SelRadial,
			  -360.0, 360.0, -1.0, 0.0);

				// Layer 1: left-right needle.
  inst->addLayer(createTexture("Textures/Panel/nav-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::XSHIFT,
			  FGSteam::get_HackVOR1_deg,
			  -10.0, 10.0, SIX_W / 40.0, 0.0);

				// Layer 2: glidescope needle
  inst->addLayer(createTexture("Textures/Panel/nav-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::YSHIFT,
			  FGSteam::get_HackGS_deg,
			  -1.0, 1.0, SIX_W / 5.0, 0.0);
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  90 );

				// Layer 3: face with markings
  inst->addLayer(createTexture("Textures/Panel/nav-face.rgb"));

				// Layer 4: heading knob
				// rotates with selected radial
  FGTexturedLayer * layer =
    new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
			SIX_W/4, SIX_W/4, 0.0, 0.5, 0.25, 0.75);
  inst->addLayer(layer);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(FGInstrumentLayer::YSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getNAV1SelRadial,
			  -360.0, 360.0, 1.0, 0.0);

  return inst;
}


/**
 * Construct a NAV2 gauge.
 */
static FGPanelInstrument *
createNAV2 (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Action: change selected radial.
  inst->addAction(0, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV2SelRadial,
				     FGBFI::setNAV2SelRadial,
				     1.0, 0.0, 360.0, true));
  inst->addAction(0, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV2SelRadial,
				     FGBFI::setNAV2SelRadial,
				     -1.0, 0.0, 360.0, true));
  inst->addAction(1, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV2SelRadial,
				     FGBFI::setNAV2SelRadial,
				     5.0, 0.0, 360.0, true));
  inst->addAction(1, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getNAV2SelRadial,
				     FGBFI::setNAV2SelRadial,
				     -5.0, 0.0, 360.0, true));

				// Layer 0: background
  inst->addLayer(createTexture("Textures/Panel/gyro-bg.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getNAV2SelRadial,
			  -360.0, 360.0, -1.0, 0.0);

				// Layer 1: left-right needle.
  inst->addLayer(createTexture("Textures/Panel/nav-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::XSHIFT,
			  FGSteam::get_HackVOR2_deg,
			  -10.0, 10.0, SIX_W / 40.0, 0.0);
//   inst->addTransformation(FGInstrumentLayer::YSHIFT,
// 			  -SIX_W / 4.4 );

				// Layer 2: face with markings.
  inst->addLayer(createTexture("Textures/Panel/nav-face.rgb"));

				// Layer 3: heading knob
				// rotates with selected radial
  FGTexturedLayer * layer =
    new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
			SIX_W/4, SIX_W/4, 0.0, 0.5, 0.25, 0.75);
  inst->addLayer(layer);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(FGInstrumentLayer::YSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getNAV2SelRadial,
			  -360.0, 360.0, 1.0, 0.0);

  return inst;
}


/**
 * Construct an ADF gauge.
 */
static FGPanelInstrument *
createADF (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W, SIX_W);

				// Action: change selected rotation.
  inst->addAction(0, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getADFRotation,
				     FGBFI::setADFRotation,
				     -1.0, 0.0, 360.0, true));
  inst->addAction(0, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getADFRotation,
				     FGBFI::setADFRotation,
				     1.0, 0.0, 360.0, true));
  inst->addAction(1, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getADFRotation,
				     FGBFI::setADFRotation,
				     -5.0, 0.0, 360.0, true));
  inst->addAction(1, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5,
		  new FGAdjustAction(FGBFI::getADFRotation,
				     FGBFI::setADFRotation,
				     5.0, 0.0, 360.0, true));

				// Layer 0: background
  inst->addLayer(createTexture("Textures/Panel/gyro-bg.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getADFRotation,
			  0.0, 360.0, 1.0, 0.0);

				// Layer 1: Direction needle.
  inst->addLayer(createTexture("Textures/Panel/long-needle.rgb"));
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGSteam::get_HackADF_deg,
			  -720.0, 720.0, 1.0, 0.0);

				// Layer 2: heading knob
				// rotates with selected radial
  FGTexturedLayer * layer =
    new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
			SIX_W/4, SIX_W/4, 0.0, 0.75, 0.25, 1.0);
  inst->addLayer(layer);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(FGInstrumentLayer::YSHIFT, -SIX_W/2 + 10); 
  inst->addTransformation(FGInstrumentLayer::ROTATION,
			  FGBFI::getADFRotation,
			  -360.0, 360.0, -1.0, 0.0);
  return inst;
}


/**
 * Create Nav-Com radio 1.
 */
static FGPanelInstrument *
createNavCom1 (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W*2, SIX_W/2);

				// Use the button to swap standby and active
				// NAV frequencies
  inst->addAction(0, SIX_W * .375, -SIX_W/4, SIX_W/4, SIX_W/4,
		  new FGSwapAction(FGBFI::getNAV1Freq,
				   FGBFI::setNAV1Freq,
				   FGBFI::getNAV1AltFreq,
				   FGBFI::setNAV1AltFreq));

				// Use the knob to tune the standby NAV
  inst->addAction(0, SIX_W-SIX_W/4, -SIX_W/4, SIX_W/8, SIX_W/4,
		  new FGAdjustAction(FGBFI::getNAV1AltFreq,
				     FGBFI::setNAV1AltFreq,
				     -0.05, 108.0, 117.95, true));
  inst->addAction(0, SIX_W-SIX_W/8, -SIX_W/4, SIX_W/8, SIX_W/4,
		  new FGAdjustAction(FGBFI::getNAV1AltFreq,
				     FGBFI::setNAV1AltFreq,
				     0.05, 108.0, 117.95, true));
  inst->addAction(1, SIX_W-SIX_W/4, -SIX_W/4, SIX_W/8, SIX_W/4,
		  new FGAdjustAction(FGBFI::getNAV1AltFreq,
				     FGBFI::setNAV1AltFreq,
				     -0.5, 108.0, 117.95, true));
  inst->addAction(1, SIX_W-SIX_W/8, -SIX_W/4, SIX_W/8, SIX_W/4,
		  new FGAdjustAction(FGBFI::getNAV1AltFreq,
				     FGBFI::setNAV1AltFreq,
				     0.5, 108.0, 117.95, true));

				// Layer 0: background
  FGTexturedLayer * layer =
    new FGTexturedLayer(createTexture("Textures/Panel/radios.rgb"),
			SIX_W*2, SIX_W/2, 0.0, 0.75, 1.0, 1.0);
  inst->addLayer(layer);

				// Layer 1: NAV frequencies
  FGTextLayer * text = new FGTextLayer(SIX_W*2, SMALL_W/2);
  text->addChunk(new FGTextLayer::Chunk(FGBFI::getNAV1Freq, "%.2f"));
  text->addChunk(new FGTextLayer::Chunk(FGBFI::getNAV1AltFreq, "%7.2f"));
  text->setPointSize(14);
  text->setColor(1.0, 0.5, 0.0);
  inst->addLayer(text);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, 3);
  inst->addTransformation(FGInstrumentLayer::YSHIFT, 5);

  return inst;
}


/**
 * Create Nav-Com radio 2.
 */
static FGPanelInstrument *
createNavCom2 (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W*2, SIX_W/2);

				// Use the button to swap standby and active
				// NAV frequencies
  inst->addAction(0, SIX_W * .375, -SIX_W/4, SIX_W/4, SIX_W/4,
		  new FGSwapAction(FGBFI::getNAV2Freq,
				   FGBFI::setNAV2Freq,
				   FGBFI::getNAV2AltFreq,
				   FGBFI::setNAV2AltFreq));

				// Use the knob to tune the standby NAV
  inst->addAction(0, SIX_W-SIX_W/4, -SIX_W/4, SIX_W/8, SIX_W/4,
		  new FGAdjustAction(FGBFI::getNAV2AltFreq,
				     FGBFI::setNAV2AltFreq,
				     -0.05, 108.0, 117.95, true));
  inst->addAction(0, SIX_W-SIX_W/8, -SIX_W/4, SIX_W/8, SIX_W/4,
		  new FGAdjustAction(FGBFI::getNAV2AltFreq,
				     FGBFI::setNAV2AltFreq,
				     0.05, 108.0, 117.95, true));
  inst->addAction(1, SIX_W-SIX_W/4, -SIX_W/4, SIX_W/8, SIX_W/4,
		  new FGAdjustAction(FGBFI::getNAV2AltFreq,
				     FGBFI::setNAV2AltFreq,
				     -0.5, 108.0, 117.95, true));
  inst->addAction(1, SIX_W-SIX_W/8, -SIX_W/4, SIX_W/8, SIX_W/4,
		  new FGAdjustAction(FGBFI::getNAV2AltFreq,
				     FGBFI::setNAV2AltFreq,
				     0.5, 108.0, 117.95, true));

				// Layer 0: background
  FGTexturedLayer * layer =
    new FGTexturedLayer(createTexture("Textures/Panel/radios.rgb"),
			SIX_W*2, SIX_W/2, 0.0, 0.75, 1.0, 1.0);
  inst->addLayer(layer);

				// Layer 1: NAV frequencies
  FGTextLayer * text = new FGTextLayer(SIX_W*2, SIX_W/2);
  text->addChunk(new FGTextLayer::Chunk(FGBFI::getNAV2Freq, "%.2f"));
  text->addChunk(new FGTextLayer::Chunk(FGBFI::getNAV2AltFreq, "%7.2f"));
  text->setPointSize(14);
  text->setColor(1.0, 0.5, 0.0);
  inst->addLayer(text);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, 3);
  inst->addTransformation(FGInstrumentLayer::YSHIFT, 5);

  return inst;
}


/**
 * Create ADF radio.
 */
static FGPanelInstrument *
createADFRadio (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W*2, SIX_W/2);

				// Use the knob to tune the standby NAV
  inst->addAction(0, SIX_W * 0.7, -SIX_W * 0.07, SIX_W * 0.09, SIX_W * 0.14,
		  new FGAdjustAction(FGBFI::getADFFreq,
				     FGBFI::setADFFreq,
				     -1.0, 100.0, 1299, true));
  inst->addAction(0, SIX_W * 0.79, -SIX_W * 0.07, SIX_W * 0.09, SIX_W * 0.14,
		  new FGAdjustAction(FGBFI::getADFFreq,
				     FGBFI::setADFFreq,
				     1.0, 100.0, 1299, true));
  inst->addAction(1, SIX_W * 0.7, -SIX_W * 0.07, SIX_W * 0.09, SIX_W * 0.14,
		  new FGAdjustAction(FGBFI::getADFFreq,
				     FGBFI::setADFFreq,
				     -25.0, 100.0, 1299, true));
  inst->addAction(1, SIX_W * 0.79, -SIX_W * 0.07, SIX_W * 0.09, SIX_W * 0.14,
		  new FGAdjustAction(FGBFI::getADFFreq,
				     FGBFI::setADFFreq,
				     25.0, 100.0, 1299, true));

				// Layer 0: background
  FGTexturedLayer * layer =
    new FGTexturedLayer(createTexture("Textures/Panel/radios.rgb"),
			SIX_W*2, SIX_W/2, 0.0, 0.5, 1.0, 0.75);
  inst->addLayer(layer);

				// Layer: ADF frequency
  FGTextLayer * text = new FGTextLayer(SIX_W*2, SIX_W/2);
  text->addChunk(new FGTextLayer::Chunk(FGBFI::getADFFreq, "%4.0f"));
  text->setPointSize(14);
  text->setColor(1.0, 0.5, 0.0);
  inst->addLayer(text);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -SIX_W + 18);

  return inst;
}


/**
 * Construct the autopilot.
 */

FGPanelInstrument *
createAP (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W*2, SIX_W/4);

				// Action: select HDG button
  inst->addAction(0, -SIX_W*0.6125, -SIX_W/16, SIX_W/4, SIX_W/8,
		  new FGToggleAction(FGBFI::getAPHeadingLock,
				     FGBFI::setAPHeadingLock));

				// Action: select NAV button
  inst->addAction(0, -SIX_W*0.3625, -SIX_W/16, SIX_W/4, SIX_W/8,
		  new FGToggleAction(FGBFI::getAPNAV1Lock,
				     FGBFI::setAPNAV1Lock));

				// Action: select ALT button
  inst->addAction(0, -SIX_W*0.1125, -SIX_W/16, SIX_W/4, SIX_W/8,
		  new FGToggleAction(FGBFI::getAPAltitudeLock,
				     FGBFI::setAPAltitudeLock));

				// Layer: AP background
  FGTexturedLayer * layer;
  layer = new FGTexturedLayer(createTexture("Textures/Panel/radios.rgb"),
			      SIX_W*2, SIX_W/4,
			      0.0, 0.375, 1.0, 0.5);
  inst->addLayer(layer);

				// Display HDG button
  FGTexturedLayer *l1 =
    new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
			SIX_W/4, SIX_W/8,
			7.0/16.0, 27.0/32.0, 9.0/16.0, 15.0/16.0);
  FGTexturedLayer *l2 =
    new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
			SIX_W/4, SIX_W/8,
			1.0/4.0, 27.0/32.0, 3.0/8.0, 15.0/16.0);
  FGSwitchLayer * sw =
    new FGSwitchLayer(SIX_W/4, SIX_W/8, FGBFI::getAPHeadingLock, l1, l2);
  inst->addLayer(sw);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -SIX_W * 0.5);

				// Display NAV button
  l1 =
    new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
			SIX_W/4, SIX_W/8,
			7.0/16.0, 3.0/4.0, 9.0/16.0, 27.0/32.0);
  l2 =
    new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
			SIX_W/4, SIX_W/8,
			1.0/4.0, 3.0/4.0, 3.0/8.0, 27.0/32.0);
  sw =
    new FGSwitchLayer(SIX_W/4, SIX_W/8, FGBFI::getAPNAV1Lock, l1, l2);
  inst->addLayer(sw);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -SIX_W * 0.25);

				// Display ALT button
  l1 =
    new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
			SIX_W/4, SIX_W/8,
			7.0/16.0, 9.0/16.0, 9.0/16.0, 21.0/32.0);
  l2 =
    new FGTexturedLayer(createTexture("Textures/Panel/knobs.rgb"),
			SIX_W/4, SIX_W/8,
			1.0/4.0, 9.0/16.0, 3.0/8.0, 21.0/32.0);
  sw =
    new FGSwitchLayer(SIX_W/4, SIX_W/8, FGBFI::getAPAltitudeLock, l1, l2);
  inst->addLayer(sw);

  return inst;
}

FGPanelInstrument * 
createDME (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y,
						       SIX_W * 0.75,
						       SIX_W * 0.25);

				// Layer: background
  FGTexturedLayer * layer =
    new FGTexturedLayer(createTexture("Textures/Panel/radios.rgb"),
			SIX_W * 0.75, SIX_W * 0.25,
			0.0, 0.25, 0.375, 0.375);
  inst->addLayer(layer);

				// Layer: current distance
  FGTextLayer * text = new FGTextLayer(SIX_W/2, SIX_W/4);
  text->addChunk(new FGTextLayer::Chunk(FGBFI::getNAV1DistDME, "%05.1f",
					METER_TO_NM));
  text->setPointSize(14);
  text->setColor(1.0, 0.5, 0.0);
  inst->addLayer(text);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -20);
  inst->addTransformation(FGInstrumentLayer::YSHIFT, -6);

  return inst;
}



////////////////////////////////////////////////////////////////////////
// Construct a panel for a small, single-prop plane.
////////////////////////////////////////////////////////////////////////

FGPanel *
fgCreateSmallSinglePropPanel (int x, int y, int finx, int finy)
{
  FGPanel * panel = new FGPanel(x, y, finx - x, finy - y);

  x = SIX_X;
  y = SIX_Y;

				// Set the background texture
  panel->setBackground(createTexture("Textures/Panel/panel-bg.rgb"));

				// Chronometer alone at side
  x = SIX_X - SIX_SPACING - 8;
  panel->addInstrument(createChronometer(x, y));

				// Top row
  x = SIX_X;
  panel->addInstrument(createAirspeedIndicator(x, y));
  x += SIX_SPACING;
  panel->addInstrument(createHorizon(x, y));
  x += SIX_SPACING;
  panel->addInstrument(createAltimeter(x, y));
  x += SIX_SPACING + 20;
  panel->addInstrument(createNAV1(x, y));

				// Middle row
  x = SIX_X;
  y -= SIX_SPACING;
  panel->addInstrument(createTurnCoordinator(x, y));
  x += SIX_SPACING;
  panel->addInstrument(createGyroCompass(x, y));
  x += SIX_SPACING;
  panel->addInstrument(createVerticalVelocity(x, y));
  x += SIX_SPACING + 20;
  panel->addInstrument(createNAV2(x, y));

				// Bottom row
  x = SIX_X;
  y -= SIX_SPACING + 10;
  panel->addInstrument(createControls(x, y));
  x += SIX_SPACING;
  panel->addInstrument(createFlapIndicator(x, y));
  x += SIX_SPACING;
  panel->addInstrument(createRPMGauge(x, y));
  x += SIX_SPACING + 20;
  y += 10;
  panel->addInstrument(createADF(x, y));

				// Radio stack
  x = SIX_X + (SIX_SPACING * 5);
  y = SIX_Y;
  panel->addInstrument(createDME(x, y));
  y -= (int)(SIX_W * 0.375);
  panel->addInstrument(createNavCom1(x, y));
  y -= SIX_W / 2;
  panel->addInstrument(createNavCom2(x, y));
  y -= SIX_W / 2;
  panel->addInstrument(createADFRadio(x, y));
  y -= (int)(SIX_W * 0.375);
  panel->addInstrument(createAP(x, y));

  return panel;
}

// end of sp_panel.cxx
