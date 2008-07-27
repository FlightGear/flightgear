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

#include <string>
#include <vector>

#include "gnnode.hxx"

using std::string;
using std::vector;

class FGTaxiRoute;


class FGParking : public FGTaxiNode {
private:
  double heading;
  double radius;
  string parkingName;
  string type;
  string airlineCodes;
 
  bool available;
  int pushBackPoint;
  FGTaxiRoute *pushBackRoute;

public:
  FGParking() :
      heading(0),
      radius(0),
      available(true),
      pushBackPoint(0),
      pushBackRoute(0)
  {
  };

  FGParking(const FGParking &other) :
      FGTaxiNode   (other),
      heading      (other.heading),
      radius       (other.radius),
      parkingName  (other.parkingName),
      type         (other.type),
      airlineCodes (other.airlineCodes),
      available    (other.available),
      pushBackPoint(other.pushBackPoint),
      pushBackRoute(other.pushBackRoute)
  {
  };


  FGParking& operator =(const FGParking &other)
  {
      FGTaxiNode::operator=(other);
      heading      = other.heading;
      radius       = other.radius;
      parkingName  = other.parkingName;
      type         = other.type;
      airlineCodes = other.airlineCodes;
      available    = other.available;
      pushBackPoint= other.pushBackPoint;
      pushBackRoute= other.pushBackRoute;
      return *this;
  };
  ~FGParking();
//   FGParking(double lat,
// 	    double lon,
// 	    double hdg,
// 	    double rad,
// 	    int idx,
// 	    const string& name,
// 	    const string& tpe,
// 	    const string& codes);

  void setHeading  (double hdg)  { heading     = hdg;  };
  void setRadius   (double rad)  { radius      = rad;  };

  void setName     (const string& name) { parkingName = name; };
  void setType     (const string& tpe)  { type        = tpe;  };
  void setCodes    (const string& codes){ airlineCodes= codes;};

  void setPushBackRoute(FGTaxiRoute *val) { pushBackRoute = val; };
  void setPushBackPoint(int val)          { pushBackPoint = val; };

  bool isAvailable ()         { return available;};
  void setAvailable(bool val) { available = val; };
  
  double getHeading  () { return heading;     };
  double getRadius   () { return radius;      };

  string getType     () { return type;        };
  string getCodes    () { return airlineCodes;};
  string getName     () { return parkingName; };

  FGTaxiRoute * getPushBackRoute () { return pushBackRoute; };

  int getPushBackPoint () { return pushBackPoint; };

  bool operator< (const FGParking &other) const {
    return radius < other.radius; };
};

typedef vector<FGParking> FGParkingVec;
typedef vector<FGParking>::iterator FGParkingVecIterator;
typedef vector<FGParking>::const_iterator FGParkingVecConstIterator;

#endif
