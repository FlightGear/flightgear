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

#include STL_STRING
#include <vector>


SG_USING_STD(string);
SG_USING_STD(vector);

double processPosition(const string& pos);

class FGParking {
private:
  double latitude;
  double longitude;
  double heading;
  double radius;
  int index;
  string parkingName;
  string type;
  string airlineCodes;
 
  bool available;

  

public:
  FGParking() { available = true;};
  //FGParking(FGParking &other);
  FGParking(double lat,
	    double lon,
	    double hdg,
	    double rad,
	    int idx,
	    const string& name,
	    const string& tpe,
	    const string& codes);
  void setLatitude (const string& lat)  { latitude    = processPosition(lat);  };
  void setLongitude(const string& lon)  { longitude   = processPosition(lon);  };
  void setHeading  (double hdg)  { heading     = hdg;  };
  void setRadius   (double rad)  { radius      = rad;  };
  void setIndex    (int    idx)  { index       = idx;  };
  void setName     (const string& name) { parkingName = name; };
  void setType     (const string& tpe)  { type        = tpe;  };
  void setCodes    (const string& codes){ airlineCodes= codes;};

  bool isAvailable ()         { return available;};
  void setAvailable(bool val) { available = val; };
  
  double getLatitude () { return latitude;    };
  double getLongitude() { return longitude;   };
  double getHeading  () { return heading;     };
  double getRadius   () { return radius;      };
  int    getIndex    () { return index;       };
  string getType     () { return type;        };
  string getCodes    () { return airlineCodes;};
  string getName     () { return parkingName; };

  bool operator< (const FGParking &other) const {return radius < other.radius; };
};

typedef vector<FGParking> FGParkingVec;
typedef vector<FGParking>::iterator FGParkingVecIterator;
typedef vector<FGParking>::const_iterator FGParkingVecConstIterator;

#endif
