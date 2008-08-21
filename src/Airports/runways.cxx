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

FGRunway::FGRunway()
{
}

FGRunway::FGRunway(const string& rwy_no,
                        const double longitude, const double latitude,
                        const double heading, const double length,
                        const double width,
                        const double displ_thresh1, const double displ_thresh2,
                        const double stopway1, const double stopway2,
                        const int surface_code)
{
  _rwy_no = rwy_no;
  if (rwy_no[0] == 'x') {
    _type = "taxiway";
  } else {
    _type = "runway";
    
  // canonicalise runway ident
    if ((rwy_no.size() == 1) || !isdigit(rwy_no[1])) {
	  _rwy_no = "0" + rwy_no;
    }

    if ((_rwy_no.size() > 2) && (_rwy_no[2] == 'x')) {
      _rwy_no = _rwy_no.substr(0, 2);
    }
  }

  _lon = longitude;
  _lat = latitude;
  _heading = heading;
  _length = length;
  _width = width;
  _displ_thresh1 = displ_thresh1;
  _displ_thresh2 = displ_thresh2;
  _stopway1 = stopway1;
  _stopway2 = stopway2;

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
    if(ident.substr(2,1) == "L") {
        buf[2] = 'R';
        buf[3] = '\0';
    } else if (ident.substr(2,1) == "R") {
        buf[2] = 'L';
        buf[3] = '\0';
    } else if (ident.substr(2,1) == "C") {
        buf[2] = 'C';
        buf[3] = '\0';
    } else if (ident.substr(2,1) == "T") {
        buf[2] = 'T';
        buf[3] = '\0';
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "Unknown runway code "
        << aRunwayIdent << " passed to FGRunway::reverseIdent");
    }
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
  return (_rwy_no[0] == 'x');
}
