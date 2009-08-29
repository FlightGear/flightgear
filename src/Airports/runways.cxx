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

#include <cstdio>              // sprintf()
#include <cstdlib>             // atoi()

#include <simgear/compiler.h>

#include <string>

#include "runways.hxx"

using std::string;

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

FGRunway::FGRunway(FGAirport* aAirport, const string& aIdent,
                        const SGGeod& aGeod,
                        const double heading, const double length,
                        const double width,
                        const double displ_thresh,
                        const double stopway,
                        const int surface_code,
                        bool reciprocal) :
  FGRunwayBase(RUNWAY, cleanRunwayNo(aIdent), aGeod, heading, length, width, surface_code, true),
  _airport(aAirport),
  _reciprocal(reciprocal),
  _displ_thresh(displ_thresh),
  _stopway(stopway),
  _ils(NULL)
{
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

SGGeod FGRunway::begin() const
{
  return pointOnCenterline(0.0);
}

SGGeod FGRunway::end() const
{
  return pointOnCenterline(lengthM());
}

SGGeod FGRunway::threshold() const
{
  return pointOnCenterline(_displ_thresh * SG_FEET_TO_METER);
}

void FGRunway::processThreshold(SGPropertyNode* aThreshold)
{
  assert(ident() == aThreshold->getStringValue("rwy"));
}
