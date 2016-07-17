// runwaybase.cxx -- a base class for runways and taxiways
//
// Written by James Turner, started December 2008.
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

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>

#include "runwaybase.hxx"

using std::string;

/*
 * surface codes
 * 1 - asphalt
 * 2 - concrete
 * 3 - turf
 * 4 - dirt
 * 5 - gravel
 * 6 - asphalt helipad
 * 7 - concrete helipad
 * 8 - turf helipad
 * 9 - dirt helipad
 * 12 -  lakebed
 */
const char * FGRunwayBase::surfaceName( int surface_code )
{
  switch( surface_code ) {
    case 1: return "asphalt";
    case 2: return "concrete";
    case 3: return "turf";
    case 4: return "dirt";
    case 5: return "gravel";
    case 6: return "asphalt helipad";
    case 7: return "concrete helipad";
    case 8: return "turf helipad";
    case 9: return "dirt helipad";
    case 12: return "lakebed";
    default: return "unknown";
  }
}

FGRunwayBase::FGRunwayBase(PositionedID aGuid, Type aTy, const string& aIdent,
                        const SGGeod& aGeod,
                        const double heading, const double length,
                        const double width,
                        const int surface_code) :
  FGPositioned(aGuid, aTy, aIdent, aGeod)
{
  _heading = heading;
  _length = length;
  _width = width;
  _surface_code = surface_code;
}

SGGeod FGRunwayBase::pointOnCenterline(double aOffset) const
{
  SGGeod result = SGGeodesy::direct(geod(), _heading, aOffset);
  result.setElevationM(geod().getElevationM());
    
  return result;
}

SGGeod FGRunwayBase::pointOffCenterline(double aOffset, double lateralOffset) const
{
    SGGeod result = pointOnCenterline(aOffset);
    return SGGeodesy::direct(result, SGMiscd::normalizePeriodic(0, 360,_heading+90.0), lateralOffset);
}


bool FGRunwayBase::isHardSurface() const
{
  return ((_surface_code == 1) || (_surface_code == 2));
}

FGTaxiway::FGTaxiway(PositionedID aGuid,
                     const string& aIdent,
                        const SGGeod& aGeod,
                        const double heading, const double length,
                        const double width,
                        const int surface_code) :
  FGRunwayBase(aGuid, TAXIWAY, aIdent, aGeod, heading, length, width, surface_code)
{
}
