// pavement.cxx - class to represent complex taxiway specified in v850 apt.dat 
//
// Copyright (C) 2009 Frederic Bouvier
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "pavement.hxx"

FGPavement::FGPavement(const std::string& aIdent, const SGGeod& aPos) :
  FGPositioned(PAVEMENT, aIdent, aPos)
{
  init(false); // FGPositioned::init
}

void FGPavement::addNode(const SGGeod &aPos, bool aClose)
{
  mNodes.push_back(new SimpleNode(aPos, aClose));
}

void FGPavement::addBezierNode(const SGGeod &aPos, const SGGeod &aCtrlPt, bool aClose)
{
  mNodes.push_back(new BezierNode(aPos, aCtrlPt, aClose));
}
