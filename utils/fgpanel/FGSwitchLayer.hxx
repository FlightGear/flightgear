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

#ifndef FGSWITCHLAYER_HXX
#define FGSWITCHLAYER_HXX

#include "FGGroupLayer.hxx"

/**
 * A group layer that switches among its children.
 *
 * The first layer that passes its condition will be drawn, and
 * any following layers will be ignored.
 */
class FGSwitchLayer : public FGGroupLayer {
public:
  // Transfer pointers!!
  FGSwitchLayer ();
  virtual void draw ();
};

#endif
