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
#include <simgear/props/props_io.hxx>

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
  serviceable(true),
  trans_ident(aIdent)
{ 
  initAirportRelation();

  // Ranges are included with the latest data format, no need to
  // assign our own defaults, unless the range is not set for some
  // reason.
  if (range < 0.1) {
    SG_LOG(SG_GENERAL, SG_WARN, "navaid " << ident() << " has no range set, using defaults");
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
  
}

void FGNavRecord::initAirportRelation()
{
  if ((type() < ILS) || (type() > GS)) {
    return; // not airport-located
  }
  
  mRunway = getRunwayFromName(_name);  
  
  if (type() != GS) {
    readAirportSceneryData();
  }
        
  // fudge elevation to the runway elevation if it's not specified
  if (fabs(elevation()) < 0.01) {
    mPosition.setElevationFt(mRunway->elevation());
  }
  
  if (type() == ILS) {
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

void FGNavRecord::readAirportSceneryData()
{
  // allow users to disable the scenery data in the short-term
  // longer term, this option can probably disappear
  if (!fgGetBool("/sim/use-scenery-airport-data")) {
    return; 
  }
  
  SGPath path;
  SGPropertyNode_ptr rootNode = new SGPropertyNode;
  if (!XMLLoader::findAirportData(mRunway->airport()->ident(), "ils", path)) {
    return;
  }
  
  readProperties(path.str(), rootNode);
  SGPropertyNode* runwayNode, *ilsNode;
  for (int i=0; (runwayNode = rootNode->getChild("runway", i)) != NULL; ++i) {
    for (int j=0; (ilsNode = runwayNode->getChild("ils", j)) != NULL; ++j) {
      // must match on both nav-ident and runway ident, to support the following:
      // - runways with multiple distinct ILS installations (KEWD, for example)
      // - runways where both ends share the same nav ident (LFAT, for example)
      if ((ilsNode->getStringValue("nav-id") == ident()) &&
          (ilsNode->getStringValue("rwy") == mRunway->ident())) {
        processSceneryILS(ilsNode);
        return;
      }
    } // of ILS iteration
  } // of runway iteration
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
  SGGeod runwayThreshold(mRunway->threshold());
  double dist, az1, az2;
  SGGeodesy::inverse(mPosition, runwayThreshold, az1, az2, dist);

// back project that distance along the runway center line
  SGGeod newPos = mRunway->pointOnCenterline(dist);

  double hdg_diff = get_multiuse() - mRunway->headingDeg();
  SG_NORMALIZE_RANGE(hdg_diff, -180.0, 180.0);

  if ( fabs(hdg_diff) <= aThreshold ) {
    mPosition = newPos;
    set_multiuse( mRunway->headingDeg() );
  } else {
    SG_LOG(SG_GENERAL, SG_WARN, "localizer:" << ident() << ", aligning with runway " 
      << mRunway->ident() << " exceeded heading threshold");
  }
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
