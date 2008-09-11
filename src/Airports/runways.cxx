// runways.cxx -- a simple class to manage airport runway info
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cmath>               // fabs()
#include <cstdio>              // sprintf()
#include <cstdlib>             // atoi()

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <Main/fg_props.hxx>

#include <string>
#include <map>

#include "runways.hxx"

using std::istream;
using std::multimap;
using std::string;

static FGPositioned::Type runwayTypeFromNumber(const std::string& aRwyNo)
{
  return (aRwyNo[0] == 'x') ? FGPositioned::TAXIWAY : FGPositioned::RUNWAY;
}

static std::string cleanRunwayNo(const std::string& aRwyNo)
{
  if (aRwyNo[0] == 'x') {
    return std::string(); // no ident for taxiways
  }
  
  string result(aRwyNo);
  // canonicalise runway ident
  if ((aRwyNo.size() == 1) || !isdigit(aRwyNo[1])) {
	  result = "0" + aRwyNo;
  }

  // trim off trailing garbage
  if (result.size() > 2) {
    char suffix = toupper(result[2]);
    if (suffix == 'X') {
       result = result.substr(0, 2);
    }
  }
  
  return result;
}

FGRunway::FGRunway(FGAirport* aAirport, const string& rwy_no,
                        const double longitude, const double latitude,
                        const double heading, const double length,
                        const double width,
                        const double displ_thresh,
                        const double stopway,
                        const int surface_code,
                        bool reciprocal) :
  FGPositioned(runwayTypeFromNumber(rwy_no), cleanRunwayNo(rwy_no), latitude, longitude, 0.0),
  _airport(aAirport),
  _reciprocal(reciprocal)
{
  _rwy_no = ident();
  
  _lon = longitude;
  _lat = latitude;
  _heading = heading;
  _length = length;
  _width = width;
  _displ_thresh = displ_thresh;
  _stopway = stopway;

  _surface_code = surface_code;
}

string FGRunway::reverseIdent(const string& aRunwayIdent)
{
  // Helipads don't have a seperate number per end
  if (aRunwayIdent.size() && (aRunwayIdent[0] == 'H' || aRunwayIdent[0] == 'h' || aRunwayIdent[0] == 'x')) {
    return aRunwayIdent;
  }

  std::string ident(aRunwayIdent);
    
  char buf[4];
  int rn = atoi(ident.substr(0,2).c_str());
  rn += 18;
  while(rn > 36) {
	  rn -= 36;
  }
  
  sprintf(buf, "%02i", rn);
  if(ident.size() == 3) {
    char suffix = toupper(ident[2]);
    if(suffix == 'L') {
      buf[2] = 'R';
    } else if (suffix == 'R') {
      buf[2] = 'L';
    } else {
      // something else, just copy
      buf[2] = suffix;
    }
    
    buf[3] = 0;
  }
  
  return buf;
}

double FGRunway::score(double aLengthWt, double aWidthWt, double aSurfaceWt) const
{
  int surface = 1;
  if (_surface_code == 12 || _surface_code == 5) // dry lakebed & gravel
    surface = 2;
  else if (_surface_code == 1 || _surface_code == 2) // asphalt & concrete
    surface = 3;
    
  return _length * aLengthWt + _width * aWidthWt + surface * aSurfaceWt + 1e-20;
}

bool FGRunway::isTaxiway() const
{
  return (type() == TAXIWAY);
}

SGGeod FGRunway::threshold() const
{
  return pointOnCenterline(0.0);
}

SGGeod FGRunway::reverseThreshold() const
{
  return pointOnCenterline(lengthM());
}

SGGeod FGRunway::displacedThreshold() const
{
  return pointOnCenterline(_displ_thresh * SG_FEET_TO_METER);
}

SGGeod FGRunway::pointOnCenterline(double aOffset) const
{
  SGGeod result;
  double dummyAz2;
  double halfLengthMetres = lengthM() * 0.5;
  
  SGGeodesy::direct(mPosition, _heading, 
    aOffset - halfLengthMetres,
    result, dummyAz2);
  return result;
}
