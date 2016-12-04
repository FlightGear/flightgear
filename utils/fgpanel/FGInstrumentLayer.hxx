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

#ifndef FGINSTRUMENTLAYER_HXX
#define FGINSTRUMENTLAYER_HXX

#include <vector>
#include <simgear/props/condition.hxx>

#include "FGPanelTransformation.hxx"

using namespace std;

/**
 * A single layer of a multi-layered instrument.
 *
 * Each layer can be subject to a series of transformations based
 * on current FGFS instrument readings: for example, a texture
 * representing a needle can rotate to show the airspeed.
 */
class FGInstrumentLayer : public SGConditional {
public:
  FGInstrumentLayer (const int w = -1, const int h = -1);
  virtual ~FGInstrumentLayer ();

  virtual void draw () = 0;
  virtual void transform () const;

  virtual int getWidth () const;
  virtual int getHeight () const;
  virtual void setWidth (const int w);
  virtual void setHeight (const int h);

  // Transfer pointer ownership!!
  // DEPRECATED
  virtual void addTransformation (FGPanelTransformation * const transformation);

protected:
  int m_w, m_h;

  typedef vector <FGPanelTransformation *> transformation_list;
  transformation_list m_transformations;
};

#endif
