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

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>

#include <string>
#include <vector>
#include <memory> // for std::auto_ptr

#include "gnnode.hxx"

class FGTaxiRoute;


class FGParking : public FGTaxiNode
{
private:
  double heading;
  double radius;
  std::string parkingName;
  std::string type;
  std::string airlineCodes;
 
  bool available;
  int pushBackPoint;
  std::auto_ptr<FGTaxiRoute> pushBackRoute;

  SG_DISABLE_COPY(FGParking);
public:
  FGParking(PositionedID aGuid, int index, const SGGeod& pos,
            double heading, double radius,
            const std::string& name, const std::string& type,
            const std::string& codes);
  virtual ~FGParking();

  void setHeading  (double hdg)  { heading     = hdg;  };
  void setRadius   (double rad)  { radius      = rad;  };

  void setName     (const std::string& name) { parkingName = name; };
  void setType     (const std::string& tpe)  { type        = tpe;  };
  void setCodes    (const std::string& codes){ airlineCodes= codes;};

  void setPushBackRoute(std::auto_ptr<FGTaxiRoute> val) { pushBackRoute = val; };
  void setPushBackPoint(int val)          { pushBackPoint = val; };

  bool isAvailable ()   const { return available;};
  void setAvailable(bool val) { available = val; };
  
  double getHeading  () const { return heading;     };
  double getRadius   () const { return radius;      };

  std::string getType     () const { return type;        };
  std::string getCodes    () const { return airlineCodes;};
  std::string getName     () const { return parkingName; };

  FGTaxiRoute * getPushBackRoute () { return pushBackRoute.get(); };

  int getPushBackPoint () { return pushBackPoint; };

  bool operator< (const FGParking &other) const {
    return radius < other.radius; };
};

typedef std::vector<FGParking*> FGParkingVec;
typedef FGParkingVec::iterator FGParkingVecIterator;
typedef FGParkingVec::const_iterator FGParkingVecConstIterator;

#endif
