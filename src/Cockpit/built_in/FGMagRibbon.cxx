//  FGMagRibbon.cxx - Built-in layer for the magnetic compass ribbon layer.
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "FGMagRibbon.hxx"


FGMagRibbon::FGMagRibbon (int w, int h)
  : FGTexturedLayer(w, h)
{
  FGCroppedTexture texture("Aircraft/Instruments/Textures/compass-ribbon.rgb");
  setTexture(texture);
  _magcompass_node =
      fgGetNode("/instrumentation/magnetic-compass/indicated-heading-deg",
                true);
}

void
FGMagRibbon::draw (osg::State& state)
{
  double heading = _magcompass_node->getDoubleValue();
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

  FGCroppedTexture *t = getTexture();
  t->setCrop(xoffset, yoffset, xoffset + 0.5, yoffset + 0.25);

  static osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet;
  stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

  state.pushStateSet(stateSet.get());
  state.apply();

  FGTexturedLayer::draw(state);

  state.popStateSet();
  state.apply();
}

// end of FGMagRibbon.cxx
