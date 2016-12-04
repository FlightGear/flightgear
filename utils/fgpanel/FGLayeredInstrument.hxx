//
//  Written by David Megginson, started January 2000.
//  Adopted for standalone fgpanel application by Torsten Dreyer, August 2009
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

#ifndef FGLAYEREDINSTRUMENT_HXX
#define FGLAYEREDINSTRUMENT_HXX

#include "FGCroppedTexture.hxx"
#include "FGInstrumentLayer.hxx"
#include "FGPanelInstrument.hxx"

using namespace std;

/**
 * An instrument constructed of multiple layers.
 *
 * Each individual layer can be rotated or shifted to correspond
 * to internal FGFS instrument readings.
 */
class FGLayeredInstrument : public FGPanelInstrument {
public:
  FGLayeredInstrument (const int x, const int y, const int w, const int h);
  virtual ~FGLayeredInstrument ();

  virtual void draw ();

  // Transfer pointer ownership!!
  virtual int addLayer (FGInstrumentLayer * const layer);
  virtual int addLayer (const FGCroppedTexture_ptr texture, const int w = -1, const int h = -1);

  // Transfer pointer ownership!!
  virtual void addTransformation (FGPanelTransformation * const transformation);

private:
  typedef vector <FGInstrumentLayer *> layer_list;
  layer_list m_layers;
};

#endif
