// parking.cxx - Implementation of a class to manage aircraft parking in
// FlightGear. This code is intended to be used by AI code and
// initial user-startup location selection.
//
// Written by Durk Talsma, started December 2004.
//
// Copyright (C) 2004 Durk Talsma.
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

#include <config.h>

#include <simgear/compiler.h>

#include <string>

#include "parking.hxx"

/*********************************************************************************
 * FGParking
 ********************************************************************************/

FGParking::FGParking(int index,
                     const SGGeod& pos,
                     double aHeading, double aRadius,
                     const std::string& name,
                     const std::string& aType,
                     const std::string& codes) :
  FGTaxiNode(FGPositioned::PARKING, index, pos, false, 0, name),
  heading(aHeading),
  radius(aRadius),
  type(aType),
  airlineCodes(codes)
{
}

void FGParking::setPushBackPoint(const FGTaxiNodeRef &node)
{
    pushBackPoint = node;
}
