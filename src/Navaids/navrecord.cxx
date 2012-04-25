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

FGNavRecord::FGNavRecord(Type aTy, const std::string& aIdent, 
  const std::string& aName, const SGGeod& aPos,
  int aFreq, int aRange, double aMultiuse) :
  FGPositioned(aTy, aIdent, aPos),
  freq(aFreq),
  range(aRange),
  multiuse(aMultiuse),
  _name(aName),
  mRunway(NULL),
  serviceable(true)
{ 
  initAirportRelation();

  // Ranges are included with the latest data format, no need to
  // assign our own defaults, unless the range is not set for some
  // reason.
  if (range < 0.1) {
    SG_LOG(SG_GENERAL, SG_DEBUG, "navaid " << ident() << " has no range set, using defaults");
    switch (type()) {
    case NDB:
    case VOR:
      range = FG_NAV_DEFAULT_RANGE;
      break;
    
    case LOC:
    case ILS:
    case GS:
      range = FG_LOC_DEFAULT_RANGE;
      break;
      
    case DME:
      range = FG_DME_DEFAULT_RANGE;
      break;
    
    default:
      range = FG_LOC_DEFAULT_RANGE;
    }
  }
  
  init(true); // init FGPositioned (now position is adjusted)
}

void FGNavRecord::initAirportRelation()
{
  if ((type() < ILS) || (type() > GS)) {
    return; // not airport-located
  }
  
  mRunway = getRunwayFromName(_name);  
  if (!mRunway) {
    return;
  }
  
  if (type() != GS) {
    SGPropertyNode* ilsData = ilsDataForRunwayAndNavaid(mRunway, ident());
    if (ilsData) {
      processSceneryILS(ilsData);
    }
  }
        
  // fudge elevation to the runway elevation if it's not specified
  if (fabs(elevation()) < 0.01) {
    mPosition.setElevationFt(mRunway->elevation());
  }
  
  if (type() == ILS || type() == LOC) {
    mRunway->setILS(this);
  }
  
  // align localizers with their runway
  if ((type() == ILS) || (type() == LOC)) {
    if (!fgGetBool("/sim/navdb/localizers/auto-align", true)) {
      return;
    }
    
   double threshold 
     = fgGetDouble( "/sim/navdb/localizers/auto-align-threshold-deg", 5.0 );
    alignLocaliserWithRunway(threshold);
  }
}

void FGNavRecord::processSceneryILS(SGPropertyNode* aILSNode)
{
  double hdgDeg = aILSNode->getDoubleValue("hdg-deg"),
    lon = aILSNode->getDoubleValue("lon"),
    lat = aILSNode->getDoubleValue("lat"),
    elevM = aILSNode->getDoubleValue("elev-m");
    
  mPosition = SGGeod::fromDegM(lon, lat, elevM);
  multiuse = hdgDeg;
}

void FGNavRecord::alignLocaliserWithRunway(double aThreshold)
{
// find the distance from the threshold to the localizer
  double dist = SGGeodesy::distanceM(mPosition, mRunway->threshold());

// back project that distance along the runway center line
  SGGeod newPos = mRunway->pointOnCenterline(dist);

  double hdg_diff = get_multiuse() - mRunway->headingDeg();
  SG_NORMALIZE_RANGE(hdg_diff, -180.0, 180.0);

  if ( fabs(hdg_diff) <= aThreshold ) {
    mPosition = SGGeod::fromGeodFt(newPos, mPosition.getElevationFt());
    set_multiuse( mRunway->headingDeg() );
  } else {
    SG_LOG(SG_GENERAL, SG_DEBUG, "localizer:" << ident() << ", aligning with runway " 
      << mRunway->ident() << " exceeded heading threshold");
  }
}

double FGNavRecord::localizerWidth() const
{
  if (!mRunway) {
    return 6.0;
  }
  
  SGVec3d thresholdCart(SGVec3d::fromGeod(mRunway->threshold()));
  double axisLength = dist(cart(), thresholdCart);
  double landingLength = dist(thresholdCart, SGVec3d::fromGeod(mRunway->end()));
  
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
