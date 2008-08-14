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

#include <string>
#include <vector>

class FGRunway
{
public:
  FGRunway();
  
  FGRunway(const std::string& id, const std::string& rwy_no,
            const double longitude, const double latitude,
            const double heading, const double length,
            const double width,
            const double displ_thresh1, const double displ_thresh2,
            const double stopway1, const double stopway2,
            const std::string& lighting_flags, const int surface_code,
            const std::string& shoulder_code, const int marking_code,
            const double smoothness, const bool dist_remaining);
  
  /**
   * given a runway identifier (06, 18L, 31R) compute the identifier for the
   * opposite heading runway (24, 36R, 13L) string.
   */
  static std::string reverseIdent(const std::string& aRunayIdent);
  
  /**
   * score this runway according to the specified weights. Used by
   * FGAirport::findBestRunwayForHeading
   */
  double score(double aLengthWt, double aWidthWt, double aSurfaceWt) const;

  bool isTaxiway() const;
  
    std::string _id;
    std::string _rwy_no;
    std::string _type;                // runway / taxiway

    double _lon;
    double _lat;
    double _heading;
    double _length;
    double _width;
    double _displ_thresh1;
    double _displ_thresh2;
    double _stopway1;
    double _stopway2;

    std::string _lighting_flags;
    int _surface_code;
    std::string _shoulder_code;
    int _marking_code;
    double _smoothness;
    bool   _dist_remaining;
};

typedef std::vector<FGRunway> FGRunwayVector;

#endif // _FG_RUNWAYS_HXX
