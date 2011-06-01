// runwaybase.cxx -- a base class for runways and taxiways
//
// Written by James Turber, started December 2008.
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

FGRunwayBase::FGRunwayBase(Type aTy, const string& aIdent,
                        const SGGeod& aGeod,
                        const double heading, const double length,
                        const double width,
                        const int surface_code,
                        bool index) :
  FGPositioned(aTy, aIdent, aGeod)
{
  _heading = heading;
  _length = length;
  _width = width;
  _surface_code = surface_code;
  
  init(index);
}

SGGeod FGRunwayBase::pointOnCenterline(double aOffset) const
{
  SGGeod result;
  double dummyAz2;
  double halfLengthMetres = lengthM() * 0.5;
  
  SGGeodesy::direct(mPosition, _heading, 
    aOffset - halfLengthMetres,
    result, dummyAz2);
  return result;
}



SGGeod FGRunwayBase::pointOffCenterline(double aOffset, double lateralOffset) const
{
  SGGeod result;
  SGGeod temp;
  double dummyAz2;
  double halfLengthMetres = lengthM() * 0.5;

  SGGeodesy::direct(mPosition, _heading, 
    aOffset - halfLengthMetres,
    temp, dummyAz2);

  SGGeodesy::direct(temp, (_heading+90.0), 
    lateralOffset,
    result, dummyAz2);

  return result;
}


bool FGRunwayBase::isHardSurface() const
{
  return ((_surface_code == 1) || (_surface_code == 2));
}

FGTaxiway::FGTaxiway(const string& aIdent,
                        const SGGeod& aGeod,
                        const double heading, const double length,
                        const double width,
                        const int surface_code) :
  FGRunwayBase(TAXIWAY, aIdent, aGeod, heading, length, width, surface_code, false)
{
}
