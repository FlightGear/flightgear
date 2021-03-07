// parking.hxx - A class to handle airport startup locations in
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

#ifndef _PARKING_HXX_
#define _PARKING_HXX_

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>

#include <string>

#include "gnnode.hxx"
#include <Airports/airports_fwd.hxx>

class FGParking : public FGTaxiNode
{
private:
  const double heading;
  const double radius;
  const std::string type;
  const std::string airlineCodes;
  FGTaxiNodeRef pushBackPoint;

  SG_DISABLE_COPY(FGParking);
public:
  static bool isType(FGPositioned::Type ty)
  { return (ty == FGPositioned::PARKING); }

  FGParking(int index,
            const SGGeod& pos,
            double heading, double radius,
            const std::string& name, const std::string& type,
            const std::string& codes);
    virtual ~FGParking() = default;
  
  double getHeading  () const { return heading;     };
  double getRadius   () const { return radius;      };

  std::string getType     () const { return type;        };
  std::string getCodes    () const { return airlineCodes;};
  std::string getName     () const { return ident(); };

  void setPushBackPoint(const FGTaxiNodeRef& node);
  FGTaxiNodeRef getPushBackPoint () { return pushBackPoint; };

  bool operator< (const FGParking &other) const {
    return radius < other.radius; };
};

#endif
