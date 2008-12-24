// runways.hxx -- a simple class to manage airport runway info
//
// Written by Curtis Olson, started August 2000.
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


#ifndef _FG_RUNWAYS_HXX
#define _FG_RUNWAYS_HXX

#include <simgear/compiler.h>

#include <simgear/math/sg_geodesy.hxx>

#include "Navaids/positioned.hxx"

#include <string>

// forward decls
class FGAirport;

class FGRunway : public FGPositioned
{  
  FGAirport* _airport; ///< owning airport
  bool _reciprocal;
public:
  
  FGRunway(FGAirport* aAirport, const std::string& rwy_no,
            const SGGeod& aGeod,
            const double heading, const double length,
            const double width,
            const double displ_thresh,
            const double stopway,
            const int surface_code,
            const bool reciprocal);
  
  /**
   * given a runway identifier (06, 18L, 31R) compute the identifier for the
   * reciprocal heading runway (24, 36R, 13L) string.
   */
  static std::string reverseIdent(const std::string& aRunayIdent);
    
  /**
   * score this runway according to the specified weights. Used by
   * FGAirport::findBestRunwayForHeading
   */
  double score(double aLengthWt, double aWidthWt, double aSurfaceWt) const;

  /**
   * Test if this runway is the reciprocal. This allows users who iterate
   * over runways to avoid counting runways twice, if desired.
   */
  bool isReciprocal() const
  { return _reciprocal; }

  /**
   * Test if this is a taxiway or not
   */
  bool isTaxiway() const;
  
  /**
   * Get the runway threshold point - this is syntatic sugar, equivalent to
   * calling pointOnCenterline(0.0);
   */
  SGGeod threshold() const;
  
  /**
   * Get the (possibly displaced) threshold point.
   */
  SGGeod displacedThreshold() const;
  
  /**
   * Get the opposite threshold - this is equivalent to calling
   * pointOnCenterline(lengthFt());
   */
  SGGeod reverseThreshold() const;
  
  /**
   * Retrieve a position on the extended runway centerline. Positive values
   * are in the direction of the runway heading, negative values are in the
   * opposited direction. 0.0 corresponds to the (non-displaced) threshold
   */
  SGGeod pointOnCenterline(double aOffset) const;
  
  /**
   * Runway length in ft
   */
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
   * Airport this runway is located at
   */
  FGAirport* airport() const
  { return _airport; }
  
  // FIXME - should die once airport / runway creation is cleaned up
  void setAirport(FGAirport* aAirport)
  { _airport = aAirport; }
  
  int surface() const 
  { return _surface_code; }
  
  double _displ_thresh;
  double _stopway;

  double _heading;
  double _length;
  double _width;
  
  int _surface_code;
};

#endif // _FG_RUNWAYS_HXX
