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

#include <map>

#include <simgear/debug/logstream.hxx>
#include <Main/bfi.hxx>
#include <Time/fg_time.hxx>

#include "panel.hxx"
#include "steam.hxx"
#include "radiostack.hxx"

FG_USING_STD(map);

				// Macros for instrument sizes
				// (these aren't used consistently
				// anyway, and should probably be
				// removed).
#define SIX_X 200
#define SIX_Y 345
#define SIX_W 128
#define SIX_SPACING (SIX_W + 5)
#define SMALL_W 112

#define createTexture(a) FGTextureManager::createTexture(a)



////////////////////////////////////////////////////////////////////////
// Static functions for obtaining settings.
//
// These are all temporary, and should be moved somewhere else
// as soon as convenient.
////////////////////////////////////////////////////////////////////////

static char * panelGetTime ()
{
  static char buf[1024];	// FIXME: not thread-safe
  struct tm * t = FGTime::cur_time_params->getGmt();
  sprintf(buf, " %.2d:%.2d:%.2d",
	  t->tm_hour, t->tm_min, t->tm_sec);
  return buf;
}

static bool panelGetNAV1TO ()
{
  if (current_radiostack->get_nav1_inrange()) {
    double heading = current_radiostack->get_nav1_heading();
    double radial = current_radiostack->get_nav1_radial();
    double var = FGBFI::getMagVar();
    if (current_radiostack->get_nav1_loc()) {
      double offset = fabs(heading - radial);
      return (offset<= 8.0 || offset >= 352.0);
    } else {
      double offset =
	fabs(heading - var - radial);
      return (offset <= 20.0 || offset >= 340.0);
    }
  } else {
    return false;
  }
}

static bool panelGetNAV1FROM ()
{
  if (current_radiostack->get_nav1_inrange()) {
    double heading = current_radiostack->get_nav1_heading();
    double radial = current_radiostack->get_nav1_radial();
    double var = FGBFI::getMagVar();
    if (current_radiostack->get_nav1_loc()) {
      double offset = fabs(heading - radial);
      return (offset >= 172.0 && offset<= 188.0);
    } else {
      double offset =
	fabs(heading - var - radial);
      return (offset >= 160.0 && offset <= 200.0);
    }
  } else {
    return false;
  }
}

static bool panelGetNAV2TO ()
{
  if (current_radiostack->get_nav2_inrange()) {
    double heading = current_radiostack->get_nav2_heading();
    double radial = current_radiostack->get_nav2_radial();
    double var = FGBFI::getMagVar();
    if (current_radiostack->get_nav2_loc()) {
      double offset = fabs(heading - radial);
      return (offset<= 8.0 || offset >= 352.0);
    } else {
      double offset =
	fabs(heading - var - radial);
      return (offset <= 20.0 || offset >= 340.0);
    }
  } else {
    return false;
  }
}

static bool panelGetNAV2FROM ()
{
  if (current_radiostack->get_nav2_inrange()) {
    double heading = current_radiostack->get_nav2_heading();
    double radial = current_radiostack->get_nav2_radial();
    double var = FGBFI::getMagVar();
    if (current_radiostack->get_nav2_loc()) {
      double offset = fabs(heading - radial);
      return (offset >= 172.0 && offset<= 188.0);
    } else {
      double offset =
	fabs(heading - var - radial);
      return (offset >= 160.0 && offset <= 200.0);
    }
  } else {
    return false;
  }
}



////////////////////////////////////////////////////////////////////////
// Cropped textures, with string keys.
////////////////////////////////////////////////////////////////////////

				// External representation of data.
struct TextureData
{
  const char * name;		// the cropped texture's name
  const char * path;		// the source texture file

				// the rectangular area containing
				// the texture (0.0-1.0)
  float xMin, yMin, xMax, yMax;
};


				// This table is temporarily hard-coded,
				// but soon it will be initialized from
				// an XML file at runtime.
TextureData textureData[] = {
{"compassFront", "Textures/Panel/misc-1.rgb",
  48.0/128.0, 0.0, 1.0, 24.0/128.0},
{"airspeedBG", "Textures/Panel/faces-2.rgb",
   0, 0.5, 0.5, 1.0},
{"longNeedle", "Textures/Panel/misc-1.rgb",
   102.0/128.0, 100.0/128.0, 107.0/128.0, 1.0},
{"horizonBG", "Textures/Panel/faces-2.rgb",
   0.5, 0.5, 1.0, 1.0},
{"horizonFloat", "Textures/Panel/misc-1.rgb",
   15.0/32.0, 54.0/128.0, 28.0/32.0, 87.0/128.0},
{"horizonRim", "Textures/Panel/faces-2.rgb",
   0, 0, 0.5, 0.5},
{"horizonFront", "Textures/Panel/faces-2.rgb",
   0.5, 0.0, 1.0, 0.5},
{"altimeterBG", "Textures/Panel/faces-1.rgb",
   0.5, 0.5, 1.0, 1.0},
{"shortNeedle", "Textures/Panel/misc-1.rgb",
   107.0/128.0, 110.0/128.0, 113.0/128.0, 1.0},
{"bug", "Textures/Panel/misc-1.rgb",
   108.0/128.0, 104.0/128.0, 112.0/128.0, 108.0/128.0},
{"turnBG", "Textures/Panel/faces-1.rgb",
   0.5, 0.0, 1.0, 0.5},
{"turnPlane", "Textures/Panel/misc-1.rgb",
   0.0, 3.0/8.0, 3.0/8.0, 0.5},
{"turnBall", "Textures/Panel/misc-1.rgb",
   108.0/128.0, 100.0/128.0, 112.0/128.0, 104.0/128.0},
{"compassBG", "Textures/Panel/faces-1.rgb",
   0.0, 0.5, 0.5, 1.0},
{"compassCenter", "Textures/Panel/misc-1.rgb",
   15.0/32.0, 11.0/16.0, 25.0/32.0, 1.0},
{"headingKnob", "Textures/Panel/misc-1.rgb",
   0, 64.0/128.0, 21.0/128.0, 85.0/128.0},
{"knob", "Textures/Panel/misc-1.rgb",
   79.0/128.0, 31.0/128.0, 101.0/128.0, 53.0/128.0},
{"verticalBG", "Textures/Panel/faces-1.rgb",
   0.0, 0.0, 0.5, 0.5},
{"rpmBG", "Textures/Panel/faces-3.rgb",
   0.0, 0.5, 0.5, 1.0},
{"flapsBG", "Textures/Panel/faces-3.rgb",
   0.5, 0.5, 1.0, 1.0},
{"clockBG", "Textures/Panel/faces-3.rgb",
   0.5, 0.0, 1.0, 0.5},
{"controlsBG", "Textures/Panel/faces-3.rgb",
   0.0, 0.0, 0.5, 0.5},
{"navFG", "Textures/Panel/misc-1.rgb",
   0, 0, 0.25, 5.0/16.0},
{"obsKnob", "Textures/Panel/misc-1.rgb",
   0.0, 86.0/128.0, 21.0/128.0, 107.0/128.0},
{"toFlag", "Textures/Panel/misc-1.rgb",
   120.0/128.0, 74.0/128.0, 1.0, 80.0/128.0},
{"fromFlag", "Textures/Panel/misc-1.rgb",
   120.0/128.0, 80.0/128.0, 1.0, 86.0/128.0},
{"offFlag", "Textures/Panel/misc-1.rgb",
   120.0/128.0, 0.5, 1.0, 70.0/128.0},
{"navNeedle", "Textures/Panel/misc-1.rgb",
   56.0/128.0, 0.5, 58.0/128.0, 1.0},
{"adfNeedle", "Textures/Panel/misc-1.rgb",
   120.0/128.0, 88.0/128.0, 1.0, 1.0},
{"adfKnob", "Textures/Panel/misc-1.rgb",
   0.0, 107.0/128.0, 21.0/128.0, 1.0},
{"adfPlane", "Textures/Panel/misc-1.rgb",
   102.0/128.0, 32.0/128.0, 1.0, 48.0/128.0},
{"adfFace", "Textures/Panel/faces-4.rgb",
   0.0, 0.5, 0.5, 1.0},
{"navRadioBG", "Textures/Panel/radios-1.rgb",
   0.0, 0.75, 1.0, 1.0},
{"adfRadioBG", "Textures/Panel/radios-1.rgb",
   0.0, 0.5, 1.0, 0.75},
{"autopilotBG", "Textures/Panel/radios-1.rgb",
   0.0, 0.375, 1.0, 0.5},
{"hdgButtonOn", "Textures/Panel/misc-1.rgb",
   39.0/128.0, 118.0/128.0, 54.0/128.0, 128.0/128.0},
{"hdgButtonOff", "Textures/Panel/misc-1.rgb",
   22.0/128.0, 118.0/128.0, 37.0/128.0, 128.0/128.0},
{"navButtonOn", "Textures/Panel/misc-1.rgb",
   39.0/128.0, 106.0/128.0, 54.0/128.0, 116.0/128.0},
{"navButtonOff", "Textures/Panel/misc-1.rgb",
   22.0/128.0, 106.0/128.0, 37.0/128.0, 116.0/128.0},
{"altButtonOn", "Textures/Panel/misc-1.rgb",
   39.0/128.0, 82.0/128.0, 54.0/128.0, 92.0/128.0},
{"altButtonOff", "Textures/Panel/misc-1.rgb",
   22.0/128.0, 82.0/128.0, 37.0/128.0, 92.0/128.0},
{"dmeBG", "Textures/Panel/radios-1.rgb",
   0.0, 0.25, 0.375, 0.375},
{"compassRibbon", "Textures/Panel/compass-ribbon.rgb",
   0.0, 0.0, 1.0, 1.0},
{0, 0}
};

				// Internal representation of
				// data.
map<const char *,CroppedTexture> tex;



/**
 * Ugly kludge to delay texture loading.
 */
class MyTexturedLayer : public FGTexturedLayer
{
public:
  MyTexturedLayer (const char * name, int w = -1, int h = -1)
    : FGTexturedLayer(w, h), _name(name) {}

  virtual void draw () const;

private:
  const char * _name;
};

void
MyTexturedLayer::draw () const {
  MyTexturedLayer * me = (MyTexturedLayer *)this;
  if (me->getTexture() == 0) {
    CroppedTexture &t = tex[_name];
    me->setTexture(t.texture);
    me->setTextureCoords(t.minX, t.minY, t.maxX, t.maxY);
  }
  FGTexturedLayer::draw();
}



/**
 * Populate the textureMap from the data table.
 */
static void
setupTextures ()
{
  for (int i = 0; textureData[i].name; i++) {
    tex[textureData[i].name] =
      CroppedTexture(textureData[i].path, textureData[i].xMin,
		     textureData[i].yMin, textureData[i].xMax,
		     textureData[i].yMax);
  }
}



////////////////////////////////////////////////////////////////////////
// Special class for magnetic compass ribbon layer.
////////////////////////////////////////////////////////////////////////

class MagRibbon : public MyTexturedLayer
{
public:
  MagRibbon (int w, int h);
  virtual ~MagRibbon () {}

  virtual void draw () const;
};

MagRibbon::MagRibbon (int w, int h)
  : MyTexturedLayer("compassRibbon", w, h)
{
}

void
MagRibbon::draw () const
{
  double heading = FGSteam::get_MH_deg();
  double xoffset, yoffset;

  while (heading >= 360.0) {
    heading -= 360.0;
  }
  while (heading < 0.0) {
    heading += 360.0;
  }

  if (heading >= 60.0 && heading <= 180.0) {
    xoffset = heading / 240.0;
    yoffset = 0.75;
  } else if (heading >= 150.0 && heading <= 270.0) {
    xoffset = (heading - 90.0) / 240.0;
    yoffset = 0.50;
  } else if (heading >= 240.0 && heading <= 360.0) {
    xoffset = (heading - 180.0) / 240.0;
    yoffset = 0.25;
  } else {
    if (heading < 270.0)
      heading += 360.0;
    xoffset = (heading - 270.0) / 240.0;
    yoffset = 0.0;
  }

  xoffset = 1.0 - xoffset;
				// Adjust to put the number in the centre
  xoffset -= 0.25;

  setTextureCoords(xoffset, yoffset, xoffset + 0.5, yoffset + 0.25);
  MyTexturedLayer::draw();
}



////////////////////////////////////////////////////////////////////////
// Instruments.
////////////////////////////////////////////////////////////////////////

struct ActionData
{
  FGPanelAction * action;
  int button, x, y, w, h;
};

struct TransData
{
  enum Type {
    End = 0,
    Rotation,
    XShift,
    YShift
  };
  Type type;
  FGInstrumentLayer::transform_func func;
  float min, max, factor, offset;
};

struct LayerData
{
  FGInstrumentLayer * layer;
  TransData transformations[16];
};

struct InstrumentData
{
  const char * name;
  int x, y, w, h;
  ActionData actions[16];
  LayerData layers[16];
};

InstrumentData instruments[] =
{

  {"magcompass", 512, 459, SIX_W, SIX_W/2, {}, {
    {new MagRibbon(int(SIX_W*0.8), int(SIX_W*0.2))},
    {new MyTexturedLayer("compassFront", SIX_W, SIX_W*(24.0/80.0))}
  }},

  {"airspeed", SIX_X, SIX_Y, SIX_W, SIX_W, {}, {
    {new MyTexturedLayer("airspeedBG", -1, -1)},
    {new MyTexturedLayer("longNeedle", int(SIX_W*(5.0/64.0)),
       int(SIX_W*(7.0/16.0))), {
      {TransData::Rotation, FGSteam::get_ASI_kias,
	 30.0, 220.0, 36.0/20.0, -54.0},
      {TransData::YShift, 0,
	 0.0, 0.0, 0.0, SIX_W*(12.0/64.0)}
    }}
  }},

  {"horizon", SIX_X + SIX_SPACING, SIX_Y, SIX_W, SIX_W, {}, {
    {new MyTexturedLayer("horizonBG", -1, -1), {
      {TransData::Rotation, FGBFI::getRoll, -360.0, 360.0, -1.0, 0.0}
    }},
    {new MyTexturedLayer("horizonFloat",
       int(SIX_W * (13.0/16.0)), int(SIX_W * (33.0/64.0))), {
      {TransData::Rotation, FGBFI::getRoll, -360.0, 360.0, -1.0, 0.0},
      {TransData::YShift, FGBFI::getPitch,
	 -20.0, 20.0, -(1.5/160.0)*SIX_W, 0.0}
    }},
    {new MyTexturedLayer("horizonRim", -1, -1), {
      {TransData::Rotation, FGBFI::getRoll, -360.0, 360.0, -1.0, 0.0}
    }},
    {new MyTexturedLayer("horizonFront", -1, -1)}
  }},

  {"altimeter", SIX_X + SIX_SPACING + SIX_SPACING, SIX_Y, SIX_W, SIX_W, {}, {
    {new MyTexturedLayer("altimeterBG", -1, -1)},
    {new MyTexturedLayer("longNeedle",
       int(SIX_W*(5.0/64.0)), int(SIX_W*(7.0/16.0))), {
      {TransData::Rotation, FGSteam::get_ALT_ft,
	 0.0, 100000.0, 360.0/1000.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, SIX_W*(12.0/64.0)}
    }},
    {new MyTexturedLayer("shortNeedle",
       int(SIX_W*(6.0/64.0)), int(SIX_W*(18.0/64.0))), {
      {TransData::Rotation, FGSteam::get_ALT_ft,
	 0.0, 100000.0, 360.0/10000.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, SIX_W/8.0}
    }},
    {new MyTexturedLayer("bug", int(SIX_W*(4.0/64.0)), int(SIX_W*(4.0/64.0))), {
      {TransData::Rotation, FGSteam::get_ALT_ft,
	 0.0, 100000.0, 360.0/100000.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, SIX_W/2.0-4}
    }},
  }},

  {"turn", SIX_X, SIX_Y-SIX_SPACING, SIX_W, SIX_W, {}, {
    {new MyTexturedLayer("turnBG", -1, -1)},
    {new MyTexturedLayer("turnPlane", int(SIX_W * 0.75), int(SIX_W * 0.25)), {
      {TransData::Rotation, FGSteam::get_TC_std, -2.5, 2.5, 20.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, int(SIX_W * 0.0625)}
    }},
    {new MyTexturedLayer("turnBall",
       int(SIX_W * (4.0/64.0)), int(SIX_W * (4.0/64.0))), {
      {TransData::Rotation, FGSteam::get_TC_rad,
	 -0.1, 0.1, -450.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -(SIX_W/4)+4}
    }}
  }},

  {"verticalVelocity", SIX_X+SIX_SPACING+SIX_SPACING, SIX_Y-SIX_SPACING,
    SIX_W, SIX_W, {}, {
    {new MyTexturedLayer("verticalBG", -1, -1)},
    {new MyTexturedLayer("longNeedle",
       int(SIX_W*(5.0/64.0)), int(SIX_W*(7.0/16.0))), {
      {TransData::Rotation, FGSteam::get_VSI_fps,
	 -2000.0, 2000.0, 42.0/500.0, 270.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, SIX_W*12.0/64.0}
    }}
  }},

  {"controls", SIX_X, SIX_Y-(SIX_SPACING*2), SMALL_W, SMALL_W, {}, {
    {new MyTexturedLayer("controlsBG", -1, -1)},
    {new MyTexturedLayer("bug", int(SIX_W*4.0/64.0), int(SIX_W*4.0/64.0)), {
      {TransData::XShift, FGBFI::getAileron, -1.0, 1.0, SMALL_W*0.75/2.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, (SIX_W/2.0)-12.0}
    }},
    {new MyTexturedLayer("bug", int(SIX_W*4.0/64.0), int(SIX_W*4.0/64.0)), {
      {TransData::Rotation, 0, 0.0, 0.0, 0.0, 180.0},
      {TransData::XShift, FGBFI::getRudder, -1.0, 1.0, -SMALL_W*0.75/2.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, SIX_W/2.0-12.0}
    }},
    {new MyTexturedLayer("bug", int(SIX_W*4.0/64.0), int(SIX_W*4.0/64.0)), {
      {TransData::Rotation, 0, 0.0, 0.0, 0.0, 270.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -SMALL_W*3.0/8.0},
      {TransData::XShift, FGBFI::getElevatorTrim,
	 -1.0, 1.0, SMALL_W*0.75/2.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, (SIX_W/2.0)-12.0}
    }},
    {new MyTexturedLayer("bug", int(SIX_W*4.0/64.0), int(SIX_W*4.0/64.0)), {
      {TransData::Rotation, 0, 0.0, 0.0, 0.0, 90.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -SMALL_W*(3.0/8.0)},
      {TransData::XShift, FGBFI::getElevator,
	 -1.0, 1.0, -SMALL_W*0.75/2.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, (SIX_W/2.0)-12.0}
    }}
  }},

  {"flaps", SIX_X+SIX_SPACING, SIX_Y-(SIX_SPACING*2), SMALL_W, SMALL_W, {}, {
    {new MyTexturedLayer("flapsBG", -1, -1)},
    {new MyTexturedLayer("longNeedle",
       int(SIX_W*(5.0/64.0)), int(SIX_W*(7.0/16.0))), {
      {TransData::XShift, 0, 0.0, 0.0, 0.0, -(SMALL_W/4) + (SMALL_W/16)},
      {TransData::Rotation, FGBFI::getFlaps, 0.0, 1.0, 120.0, 30.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, SIX_W*12.0/64.0}
    }}
  }},

  {"rpm", SIX_X+(SIX_SPACING*2), SIX_Y-(SIX_SPACING*2), SMALL_W, SMALL_W, {}, {
    {new MyTexturedLayer("rpmBG", -1, -1)},
    {new MyTexturedLayer("longNeedle",
       int(SIX_W*(5.0/64.0)), int(SIX_W*(7.0/16.0))), {
      {TransData::Rotation, FGBFI::getThrottle, 0.0, 100.0, 300.0, -150.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, SIX_W*12.0/64.0}
    }}
  }},

  {"gyro", SIX_X+SIX_SPACING, SIX_Y-SIX_SPACING, SIX_W, SIX_W, {
    {new FGAdjustAction(FGBFI::getAPHeadingMag, FGBFI::setAPHeadingMag,
			-1.0, -360.0, 360.0, true),
       0, SIX_W/2-SIX_W/5, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getAPHeadingMag, FGBFI::setAPHeadingMag,
			1.0, -360.0, 360.0, true),
       0, SIX_W/2 - SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getAPHeadingMag, FGBFI::setAPHeadingMag,
			-5.0, -360.0, 360.0, true),
       1, SIX_W/2 - SIX_W/5, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getAPHeadingMag, FGBFI::setAPHeadingMag,
			5.0, -360.0, 360.0, true),
       1, SIX_W/2 - SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5}
//     {new FGAdjustAction(FGSteam::get_DG_err, FGSteam::set_DG_err,
// 			-1.0, -360.0, 360.0, true),
//        0, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5}
//     {new FGAdjustAction(FGSteam::get_DG_err, FGSteam::set_DG_err,
// 			1.0, -360.0, 360.0, true),
//        0, -SIX_W/2+SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5}
//     {new FGAdjustAction(FGSteam::get_DG_err, FGSteam::set_DG_err,
// 			-5.0, -360.0, 360.0, true),
//        1, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5}
//     {new FGAdjustAction(FGSteam::get_DG_err, FGSteam::set_DG_err,
// 			5.0, -360.0, 360.0, true),
//        1, -SIX_W/2+SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5}
  }, {
    {new MyTexturedLayer("compassBG", -1, -1), {
      {TransData::Rotation, FGSteam::get_DG_deg, -720.0, 720.0, -1.0, 0.0}
    }},
    {new MyTexturedLayer("bug",
			 int(SIX_W*(4.0/64.0)), int(SIX_W*(4.0/64.0))), {
      {TransData::Rotation, FGSteam::get_DG_deg, -720.0, 720.0, -1.0, 0.0},
      {TransData::Rotation, FGBFI::getAPHeadingMag, -720.0, 720.0, -1.0, 0.0},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, (SIX_W/2.0)-4}
    }},
    {new MyTexturedLayer("compassCenter", int(SIX_W*0.625), int(SIX_W*0.625))},
    {new MyTexturedLayer("headingKnob",
       int(SIX_W*(21.0/112.0)), int(SIX_W*(21.0/112.0))), {
      {TransData::XShift, 0, 0.0, 0.0, 0.0, SIX_W/2-10},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -SIX_W/2+10},
      {TransData::Rotation, FGBFI::getAPHeadingMag, -360.0, 360.0, 1.0, 0.0}
    }},
    {new MyTexturedLayer("knob",
			 int(SIX_W*(22.0/112.0)), int(SIX_W*(22.0/112.0))), {
      {TransData::XShift, 0, 0.0, 0.0, 0.0, -SIX_W/2+10},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -SIX_W/2+10}
    }}

  }},

  {"chronometer", SIX_X-SIX_SPACING-8, SIX_Y, SMALL_W, SMALL_W, {}, {
    {new MyTexturedLayer("clockBG")},
    {new FGTextLayer(SMALL_W, SMALL_W,
		     new FGTextLayer::Chunk(panelGetTime)), {
      {TransData::XShift, 0, 0.0, 0.0, 0.0, SMALL_W*-0.38},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, SMALL_W*-0.06}
    }}
  }},

  {"nav1", SIX_X+(SIX_SPACING*3)+20, SIX_Y, SIX_W, SIX_W, {
    {new FGAdjustAction(FGBFI::getNAV1SelRadial, FGBFI::setNAV1SelRadial,
			1.0, 0.0, 360.0, true),
       0, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getNAV1SelRadial, FGBFI::setNAV1SelRadial,
			-1.0, 0.0, 360.0, true),
       0, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getNAV1SelRadial, FGBFI::setNAV1SelRadial,
			5.0, 0.0, 360.0, true),
       1, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getNAV1SelRadial, FGBFI::setNAV1SelRadial,
			-5.0, 0.0, 360.0, true),
       1, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5}       
  }, {
    {new MyTexturedLayer("compassBG"), {
      {TransData::Rotation, FGBFI::getNAV1SelRadial,
	 -360.0, 360.0, -1.0, 0.0}
    }},
    {new MyTexturedLayer("navFG", SIX_W/2, int(SIX_W*(5.0/8.0)))},
    {new MyTexturedLayer("obsKnob", int(SIX_W*(21.0/112.0)),
			 int(SIX_W*(21.0/112.0))), {
      {TransData::XShift, 0, 0.0, 0.0, 0.0, -SIX_W/2+10},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -SIX_W/2+10},
      {TransData::Rotation, FGBFI::getNAV1SelRadial, -360.0, 360.0, 1.0, 0.0}
    }},
    {new FGSwitchLayer(SIX_W/8, SIX_W/8, panelGetNAV1TO,
		       new MyTexturedLayer("toFlag", SIX_W/8, SIX_W/8),
		       new FGSwitchLayer(SIX_W/8, SIX_W/8, panelGetNAV1FROM,
					 new MyTexturedLayer("fromFlag",
							     SIX_W/8,
							     SIX_W/8),
					 new MyTexturedLayer("offFlag",
							     SIX_W/8,
							     SIX_W/8))), {
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -int(SIX_W*0.1875)}
    }},
    {new MyTexturedLayer("navNeedle", SIX_W/32, SIX_W/2), {
      {TransData::XShift, FGSteam::get_HackVOR1_deg,
	 -10.0, 10.0, SIX_W/40.0, 0.0}
    }},
    {new MyTexturedLayer("navNeedle", SIX_W/32, SIX_W/2), {
      {TransData::YShift, FGSteam::get_HackGS_deg, -1.0, 1.0, SIX_W/5.0, 0.0},
      {TransData::Rotation, 0, 0.0, 0.0, 0.0, 90}
    }}
  }},

  {"nav2", SIX_X+(SIX_SPACING*3)+20, SIX_Y-SIX_SPACING, SIX_W, SIX_W, {
    {new FGAdjustAction(FGBFI::getNAV2SelRadial, FGBFI::setNAV2SelRadial,
			1.0, 0.0, 360.0, true),
       0, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getNAV2SelRadial, FGBFI::setNAV2SelRadial,
			-1.0, 0.0, 360.0, true),
       0, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getNAV2SelRadial, FGBFI::setNAV2SelRadial,
			5.0, 0.0, 360.0, true),
       1, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getNAV2SelRadial, FGBFI::setNAV2SelRadial,
			-5.0, 0.0, 360.0, true),
       1, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5}       
  }, {
    {new MyTexturedLayer("compassBG"), {
      {TransData::Rotation, FGBFI::getNAV2SelRadial,
	 -360.0, 360.0, -1.0, 0.0}
    }},
    {new MyTexturedLayer("navFG", SIX_W/2, int(SIX_W*(5.0/8.0)))},
    {new MyTexturedLayer("obsKnob", int(SIX_W*(21.0/112.0)),
			 int(SIX_W*(21.0/112.0))), {
      {TransData::XShift, 0, 0.0, 0.0, 0.0, -SIX_W/2+10},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -SIX_W/2+10},
      {TransData::Rotation, FGBFI::getNAV2SelRadial, -360.0, 360.0, 1.0, 0.0}
    }},
    {new FGSwitchLayer(SIX_W/8, SIX_W/8, panelGetNAV2TO,
		       new MyTexturedLayer("toFlag", SIX_W/8, SIX_W/8),
		       new FGSwitchLayer(SIX_W/8, SIX_W/8, panelGetNAV2FROM,
					 new MyTexturedLayer("fromFlag",
							     SIX_W/8,
							     SIX_W/8),
					 new MyTexturedLayer("offFlag",
							     SIX_W/8,
							     SIX_W/8))), {
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -int(SIX_W*0.1875)}
    }},
    {new MyTexturedLayer("navNeedle", SIX_W/32, SIX_W/2), {
      {TransData::XShift, FGSteam::get_HackVOR2_deg,
	 -10.0, 10.0, SIX_W/40.0, 0.0}
    }}
  }},

  {"adf", SIX_X+(SIX_SPACING*3)+20, SIX_Y-(SIX_SPACING*2), SIX_W, SIX_W, {
    {new FGAdjustAction(FGBFI::getADFRotation, FGBFI::setADFRotation,
			-1.0, 0.0, 360.0, true),
       0, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getADFRotation, FGBFI::setADFRotation,
			1.0, 0.0, 360.0, true),
       0, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getADFRotation, FGBFI::setADFRotation,
			-5.0, 0.0, 360.0, true),
       1, -SIX_W/2, -SIX_W/2, SIX_W/10, SIX_W/5},
    {new FGAdjustAction(FGBFI::getADFRotation, FGBFI::setADFRotation,
			5.0, 0.0, 360.0, true),
       1, -SIX_W/2 + SIX_W/10, -SIX_W/2, SIX_W/10, SIX_W/5}
  },{
    {new MyTexturedLayer("compassBG"), {
      {TransData::Rotation, FGBFI::getADFRotation, 0.0, 360.0, 1.0, 0.0}
    }},
    {new MyTexturedLayer("adfFace", -1, -1), {}},
    {new MyTexturedLayer("adfNeedle", SIX_W/8, int(SIX_W*0.625)), {
      {TransData::Rotation, FGSteam::get_HackADF_deg, -720.0, 720.0, 1.0, 0.0}
    }},
    {new MyTexturedLayer("adfKnob", int(SIX_W*(21.0/112.0)),
			 int(SIX_W*(21.0/112.0))), {
      {TransData::XShift, 0, 0.0, 0.0, 0.0, -SIX_W/2+10},
      {TransData::YShift, 0, 0.0, 0.0, 0.0, -SIX_W/2+10},
      {TransData::Rotation, FGBFI::getADFRotation, 0.0, 360.0, 1.0, 0.0}
    }}
  }},

  {0}

};



////////////////////////////////////////////////////////////////////////
// Static factory functions to create textured gauges.
//
// These will be replaced first with a giant table, and then with
// configuration files read from an external source, but for now
// they're hard-coded.
////////////////////////////////////////////////////////////////////////


/**
 * Create Nav-Com radio 1.
 */
static FGPanelInstrument *
createNavCom1 (int x, int y)
{
  FGLayeredInstrument * inst = new FGLayeredInstrument(x, y, SIX_W*2, SIX_W/2);

				// Use the button to swap standby and active
				// NAV frequencies
  inst->addAction(0, int(SIX_W * .375), -SIX_W/4, SIX_W/4, SIX_W/4,
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
  inst->addLayer(tex["navRadioBG"], SIX_W*2, SIX_W/2);

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
  inst->addAction(0, int(SIX_W * .375), -SIX_W/4, SIX_W/4, SIX_W/4,
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
  inst->addLayer(tex["navRadioBG"], SIX_W*2, SIX_W/2);

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
  inst->addAction(0, int(SIX_W * 0.7), int(-SIX_W * 0.07),
		  int(SIX_W * 0.09), int(SIX_W * 0.14),
		  new FGAdjustAction(FGBFI::getADFFreq,
				     FGBFI::setADFFreq,
				     -1.0, 100.0, 1299, true));
  inst->addAction(0, int(SIX_W * 0.79), int(-SIX_W * 0.07),
		  int(SIX_W * 0.09), int(SIX_W * 0.14),
		  new FGAdjustAction(FGBFI::getADFFreq,
				     FGBFI::setADFFreq,
				     1.0, 100.0, 1299, true));
  inst->addAction(1, int(SIX_W * 0.7), int(-SIX_W * 0.07),
		  int(SIX_W * 0.09), int(SIX_W * 0.14),
		  new FGAdjustAction(FGBFI::getADFFreq,
				     FGBFI::setADFFreq,
				     -25.0, 100.0, 1299, true));
  inst->addAction(1, int(SIX_W * 0.79), int(-SIX_W * 0.07),
		  int(SIX_W * 0.09), int(SIX_W * 0.14),
		  new FGAdjustAction(FGBFI::getADFFreq,
				     FGBFI::setADFFreq,
				     25.0, 100.0, 1299, true));

				// Layer 0: background
  inst->addLayer(tex["adfRadioBG"], SIX_W*2, SIX_W/2);

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
  inst->addAction(0, int(-SIX_W*0.6125), -SIX_W/16, SIX_W/4, SIX_W/8,
		  new FGToggleAction(FGBFI::getAPHeadingLock,
				     FGBFI::setAPHeadingLock));

				// Action: select NAV button
  inst->addAction(0, int(-SIX_W*0.3625), -SIX_W/16, SIX_W/4, SIX_W/8,
		  new FGToggleAction(FGBFI::getAPNAV1Lock,
				     FGBFI::setAPNAV1Lock));

				// Action: select ALT button
  inst->addAction(0, int(-SIX_W*0.1125), -SIX_W/16, SIX_W/4, SIX_W/8,
		  new FGToggleAction(FGBFI::getAPAltitudeLock,
				     FGBFI::setAPAltitudeLock));

				// Layer: AP background
  inst->addLayer(tex["autopilotBG"], SIX_W*2, SIX_W/4);

				// Display HDG button
  FGSwitchLayer * sw =
    new FGSwitchLayer(SIX_W/4, SIX_W/8, FGBFI::getAPHeadingLock,
		      new FGTexturedLayer(tex["hdgButtonOn"], SIX_W/4, SIX_W/8),
		      new FGTexturedLayer(tex["hdgButtonOff"], SIX_W/4, SIX_W/8));
  inst->addLayer(sw);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -SIX_W * 0.5);

				// Display NAV button
  sw = new FGSwitchLayer(SIX_W/4, SIX_W/8, FGBFI::getAPNAV1Lock,
			 new FGTexturedLayer(tex["navButtonOn"], SIX_W/4, SIX_W/8),
			 new FGTexturedLayer(tex["navButtonOff"], SIX_W/4, SIX_W/8));
  inst->addLayer(sw);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -SIX_W * 0.25);

				// Display ALT button
  sw = new FGSwitchLayer(SIX_W/4, SIX_W/8, FGBFI::getAPAltitudeLock,
			 new FGTexturedLayer(tex["altButtonOn"], SIX_W/4, SIX_W/8),
			 new FGTexturedLayer(tex["altButtonOff"], SIX_W/4, SIX_W/8));
  inst->addLayer(sw);

  return inst;
}

FGPanelInstrument * 
createDME (int x, int y)
{
  FGLayeredInstrument * inst =
    new FGLayeredInstrument(x, y, int(SIX_W * 0.75), int(SIX_W * 0.25));

				// Layer: background
  inst->addLayer(tex["dmeBG"]);

				// Layer: current distance

  FGTextLayer * text1 = new FGTextLayer(SIX_W/2, SIX_W/4);
  text1->addChunk(new FGTextLayer::Chunk(FGBFI::getNAV1DistDME, "%05.1f",
					 METER_TO_NM));
  text1->setPointSize(12);
  text1->setColor(1.0, 0.5, 0.0);

  FGTextLayer * text2 = new FGTextLayer(SIX_W/2, SIX_W/4);
  text2->addChunk(new FGTextLayer::Chunk("---.-"));
  text2->setPointSize(12);
  text2->setColor(1.0, 0.5, 0.0);

  FGSwitchLayer * sw =
    new FGSwitchLayer(SIX_W/2, SIX_W/4, FGBFI::getNAV1DMEInRange,
		      text1, text2);

  inst->addLayer(sw);
  inst->addTransformation(FGInstrumentLayer::XSHIFT, -20);
  inst->addTransformation(FGInstrumentLayer::YSHIFT, -6);

  return inst;
}



////////////////////////////////////////////////////////////////////////
// Construct a panel for a small, single-prop plane.
////////////////////////////////////////////////////////////////////////

FGPanel *
fgCreateSmallSinglePropPanel (int xpos, int ypos, int finx, int finy)
{
  int w = finx - xpos;
  int h = finy - ypos;
  FGPanel * panel = new FGPanel(xpos, ypos, w, h);
  int x, y;

  setupTextures();

				// Read gauges from data table.
  for (int i = 0; instruments[i].name; i++) {
    InstrumentData &gauge = instruments[i];
    FGLayeredInstrument * inst =
      new FGLayeredInstrument(gauge.x, gauge.y, gauge.w, gauge.h);

    for (int j = 0; gauge.actions[j].action; j++) {
      ActionData &action = gauge.actions[j];
      inst->addAction(action.button, action.x, action.y, action.w, action.h,
		      action.action);
    }

    for (int j = 0; gauge.layers[j].layer; j++) {
      LayerData &layer = gauge.layers[j];
//       inst->addLayer(tex[layer.textureName], layer.w, layer.h);
      inst->addLayer(layer.layer);

      for (int k = 0; layer.transformations[k].type; k++) {
	TransData &trans = layer.transformations[k];
	FGInstrumentLayer::transform_type type;
	switch (trans.type) {
	case TransData::Rotation:
	  type = FGInstrumentLayer::ROTATION;
	  break;
	case TransData::XShift:
	  type = FGInstrumentLayer::XSHIFT;
	  break;
	case TransData::YShift:
	  type = FGInstrumentLayer::YSHIFT;
	  break;
	default:
	  break;
	}
	if (trans.func) {
	  inst->addTransformation(type, trans.func,
				  trans.min, trans.max,
				  trans.factor, trans.offset);
	} else {
	  inst->addTransformation(type, trans.offset);
	}
      }

    }
    panel->addInstrument(inst);
  }

  x = SIX_X;
  y = SIX_Y;

				// Set the background texture
  panel->setBackground(createTexture("Textures/Panel/panel-bg.rgb"));

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
