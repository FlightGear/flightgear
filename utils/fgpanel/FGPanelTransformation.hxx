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

#ifndef FGPANELTRANSFORMATION_HXX
#define FGPANELTRANSFORMATION_HXX

#include <simgear/math/interpolater.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>

/**
 * A transformation for a layer.
 */
class FGPanelTransformation : public SGConditional {
public:
  enum Type {
    XSHIFT,
    YSHIFT,
    ROTATION
  };

  FGPanelTransformation ();
  virtual ~FGPanelTransformation ();

  Type type;
  SGConstPropertyNode_ptr node;
  float min;
  float max;
  bool has_mod;
  float mod;
  float factor;
  float offset;
  SGInterpTable *table;
};

#endif
