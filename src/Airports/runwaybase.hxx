// runwaybase.hxx -- represent a runway or taxiway
//
// Written by James Turner, started December 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _FG_RUNWAY_BASE_HXX
#define _FG_RUNWAY_BASE_HXX

#include <simgear/compiler.h>

#include <simgear/math/sg_geodesy.hxx>

#include <Navaids/positioned.hxx>

#include <string>

/**
 * @class The base class for runways and taxiways. At present, FGTaxiway is
 * a direct instantiation of this class.
 */
class FGRunwayBase : public FGPositioned
{    
public:
  FGRunwayBase(Type aTy, const std::string& aIdent,
            const SGGeod& aGeod,
            const double heading, const double length,
            const double width,
            const int surface_code,
            bool index);
            
  /**
   * Retrieve a position on the extended centerline. Positive values
   * are in the direction of the runway heading, negative values are in the
   * opposited direction. 0.0 corresponds to the (non-displaced) threshold
   */
  SGGeod pointOnCenterline(double aOffset) const;
  SGGeod pointOffCenterline(double aOffset, double lateralOffset) const;
  
  double lengthFt() const
  { return _length; }
  
  double lengthM() const
  { return _length * SG_FEET_TO_METER; }
  
  double widthFt() const
  { return _width; }
  
  double widthM() const
  { return _width * SG_FEET_TO_METER; }
  
   /**
   * Runway heading in degrees.
   */
  double headingDeg() const
  { return _heading; }
  
  /**
   * Predicate to test if this runway has a hard surface. For the moment, this
   * means concrete or asphalt
   */
  bool isHardSurface() const;
  
  /**
   * Retrieve runway surface code, as define in Robin Peel's data
   */
  int surface() const 
  { return _surface_code; }
  
protected:
    
  double _heading;
  double _length;
  double _width;

  /** surface, as defined by:
   * http://www.x-plane.org/home/robinp/Apt810.htm#RwySfcCodes
   */
  int _surface_code;
};

// for the moment, taxiways are simply a concrete RunwayBase
class FGTaxiway : public FGRunwayBase
{
public:
  FGTaxiway(const std::string& aIdent,
            const SGGeod& aGeod,
            const double heading, const double length,
            const double width,
            const int surface_code);
};

#endif // _FG_RUNWAY_BASE_HXX
