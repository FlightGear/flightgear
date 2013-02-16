// navrecord.cxx -- generic vor/dme/ndb class
//
// Written by Curtis Olson, started May 2004.
//
// Copyright (C) 2004  Curtis L. Olson - http://www.flightgear.org/~curt
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
  #include "config.h"
#endif

#include <istream>

#include <simgear/misc/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/props/props.hxx>


#include <Navaids/navrecord.hxx>
#include <Navaids/navdb.hxx>
#include <Airports/runways.hxx>
#include <Airports/simple.hxx>
#include <Airports/xmlloader.hxx>
#include <Main/fg_props.hxx>
#include <Navaids/NavDataCache.hxx>

FGNavRecord::FGNavRecord(PositionedID aGuid, Type aTy, const std::string& aIdent,
  const std::string& aName, const SGGeod& aPos,
  int aFreq, int aRange, double aMultiuse, PositionedID aRunway) :
  FGPositioned(aGuid, aTy, aIdent, aPos),
  freq(aFreq),
  range(aRange),
  multiuse(aMultiuse),
  mName(aName),
  mRunway(aRunway),
  serviceable(true)
{ 
}

FGRunway* FGNavRecord::runway() const
{
  return (FGRunway*) flightgear::NavDataCache::instance()->loadById(mRunway);
}

double FGNavRecord::localizerWidth() const
{
  if (!mRunway) {
    return 6.0;
  }
  
  FGRunway* rwy = runway();
  SGVec3d thresholdCart(SGVec3d::fromGeod(rwy->threshold()));
  double axisLength = dist(cart(), thresholdCart);
  double landingLength = dist(thresholdCart, SGVec3d::fromGeod(rwy->end()));
  
// Reference: http://dcaa.slv.dk:8000/icaodocs/
// ICAO standard width at threshold is 210 m = 689 feet = approx 700 feet.
// ICAO 3.1.1 half course = DDM = 0.0775
// ICAO 3.1.3.7.1 Sensitivity 0.00145 DDM/m at threshold
//  implies peg-to-peg of 214 m ... we will stick with 210.
// ICAO 3.1.3.7.1 "Course sector angle shall not exceed 6 degrees."
              
// Very short runway:  less than 1200 m (4000 ft) landing length:
  if (landingLength < 1200.0) {
// ICAO fudges localizer sensitivity for very short runways.
// This produces a non-monotonic sensitivity-versus length relation.
    axisLength += 1050.0;
  }

// Example: very short: San Diego   KMYF (Montgomery Field) ILS RWY 28R
// Example: short:      Tom's River KMJX (Robert J. Miller) ILS RWY 6
// Example: very long:  Denver      KDEN (Denver)           ILS RWY 16R
  double raw_width = 210.0 / axisLength * SGD_RADIANS_TO_DEGREES;
  return raw_width < 6.0? raw_width : 6.0;

}

FGTACANRecord::FGTACANRecord(void) :
    channel(""),
    freq(0)
    
{
}

std::istream&
operator >> ( std::istream& in, FGTACANRecord& n )
{
    in >> n.channel >> n.freq ;
    //getline( in, n.name );

    return in;
}
