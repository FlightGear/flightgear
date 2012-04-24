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

#include <Airports/runwaybase.hxx>

// forward decls
class FGAirport;
class FGNavRecord;
class SGPropertyNode;

namespace flightgear {
  class SID;
  class STAR;
  class Approach;
}

class FGRunway : public FGRunwayBase
{
  FGAirport* _airport;
  bool _isReciprocal;
  FGRunway* _reciprocal;
  double _displ_thresh;
  double _stopway;
  FGNavRecord* _ils;
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
  { return _isReciprocal; }

  /**
   * Get the runway begining point - this is syntatic sugar, equivalent to
   * calling pointOnCenterline(0.0);
   */
  SGGeod begin() const;
  
  /**
   * Get the (possibly displaced) threshold point.
   */
  SGGeod threshold() const;
  
  /**
   * Get the 'far' end - this is equivalent to calling
   * pointOnCenterline(lengthFt());
   */
  SGGeod end() const;
  
  double displacedThresholdM() const
  { return _displ_thresh * SG_FEET_TO_METER; }
  
  double stopwayM() const
  { return _stopway * SG_FEET_TO_METER; }
  
  /**
   * Airport this runway is located at
   */
  FGAirport* airport() const
  { return _airport; }
  
  // FIXME - should die once airport / runway creation is cleaned up
  void setAirport(FGAirport* aAirport)
  { _airport = aAirport; }
  
  FGNavRecord* ILS() const { return _ils; }
  void setILS(FGNavRecord* nav) { _ils = nav; }
  
  FGRunway* reciprocalRunway() const
  { return _reciprocal; }
  void setReciprocalRunway(FGRunway* other);
    
  /**
   * Helper to process property data loaded from an ICAO.threshold.xml file
   */
  void processThreshold(SGPropertyNode* aThreshold);
  
  /**
   * Get SIDs (DPs) associated with this runway
   */
  std::vector<flightgear::SID*> getSIDs() const;
  
  /**
   * Get STARs associared with this runway
   */ 
  std::vector<flightgear::STAR*> getSTARs() const;
  
  
  std::vector<flightgear::Approach*> getApproaches() const;
  
};

#endif // _FG_RUNWAYS_HXX
