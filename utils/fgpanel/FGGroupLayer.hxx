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

#ifndef FGGROUPLAYER_HXX
#define FGGROUPLAYER_HXX

#include "FGInstrumentLayer.hxx"

/**
 * An instrument layer containing a group of sublayers.
 *
 * This class is useful for gathering together a group of related
 * layers, either to hold in an external file or to work under
 * the same condition.
 */
class FGGroupLayer : public FGInstrumentLayer {
public:
  FGGroupLayer ();
  virtual ~FGGroupLayer ();
  virtual void draw ();
  // transfer pointer ownership
  virtual void addLayer (FGInstrumentLayer * const layer);
protected:
  vector <FGInstrumentLayer *> m_layers;
};

#endif
