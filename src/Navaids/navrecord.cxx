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

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/props/props.hxx>


#include <Navaids/navrecord.hxx>
#include <Navaids/navdb.hxx>
#include <Airports/runways.hxx>
#include <Airports/airport.hxx>
#include <Airports/xmlloader.hxx>
#include <Main/fg_props.hxx>

FGNavRecord::FGNavRecord(PositionedID aGuid, Type aTy, const std::string& aIdent,
  const std::string& aName, const SGGeod& aPos,
  int aFreq, int aRange, double aMultiuse, PositionedID aRunway) :
  FGPositioned(aGuid, aTy, aIdent, aPos),
  freq(aFreq),
  range(aRange),
  multiuse(aMultiuse),
  mName(aName),
  mRunway(aRunway),
  mColocated(0),
  serviceable(true)
{
}

FGRunwayRef FGNavRecord::runway() const
{
  return loadById<FGRunway>(mRunway);
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

bool FGNavRecord::hasDME() const
{
  return (mColocated > 0);
}



bool FGNavRecord::isVORTAC() const
{
    if (mType != VOR)
        return false;

    return mName.find(" VORTAC") != std::string::npos;
}

void FGNavRecord::setColocatedDME(PositionedID other)
{
    mColocated = other;
}

PositionedID FGNavRecord::colocatedDME() const
{
    return mColocated;
}

void FGNavRecord::updateFromXML(const SGGeod& geod, double heading)
{
    modifyPosition(geod);
    multiuse = heading;
}

double FGNavRecord::glideSlopeAngleDeg() const
{
    if (type() != FGPositioned::GS) {
        SG_LOG(SG_NAVAID, SG_DEV_WARN, "called glideSlopeAngleDeg on non-GS navaid:" << ident());
        return 0.0;
    }
    
    const auto tmp = static_cast<int>(get_multiuse() / 1000.0);
    return static_cast<double>(tmp) / 100.0;
}

//------------------------------------------------------------------------------
FGMobileNavRecord::FGMobileNavRecord( PositionedID aGuid,
                                      Type type,
                                      const std::string& ident,
                                      const std::string& name,
                                      const SGGeod& aPos,
                                      int freq,
                                      int range,
                                      double multiuse,
                                      PositionedID aRunway ):
  FGNavRecord(aGuid, type, ident, name, aPos, freq, range, multiuse, aRunway),
  _initial_elevation_ft(aPos.getElevationFt())
{

}

//------------------------------------------------------------------------------
const SGGeod& FGMobileNavRecord::geod() const
{
  const_cast<FGMobileNavRecord*>(this)->updatePos();
  return FGNavRecord::geod();
}

//------------------------------------------------------------------------------
const SGVec3d& FGMobileNavRecord::cart() const
{
  const_cast<FGMobileNavRecord*>(this)->updatePos();
  return FGNavRecord::cart();
}

//------------------------------------------------------------------------------
void FGMobileNavRecord::clearVehicle()
{
    _vehicle_node.clear();
}

void FGMobileNavRecord::updateVehicle()
{
  _vehicle_node.clear();

  SGPropertyNode* ai_branch = fgGetNode("ai/models");
  if( !ai_branch )
  {
    SG_LOG( SG_NAVAID,
            SG_INFO,
            "Can not update mobile navaid position (no ai/models branch)" );
    return;
  }

  const std::string& nav_name = name();

  // Try any aircraft carriers first
  simgear::PropertyList carrier = ai_branch->getChildren("carrier");
  for(size_t i = 0; i < carrier.size(); ++i)
  {
    const std::string carrier_name = carrier[i]->getStringValue("name");

    if(    carrier_name.empty()
        || nav_name.find(carrier_name) == std::string::npos )
      continue;

    _vehicle_node = carrier[i];
    return;
  }

  // Now the tankers
  const std::string tanker_branches[] = {
    // AI tankers
    "tanker",
    // And finally mp tankers
    "multiplayer"
  };

  for(size_t i = 0; i < sizeof(tanker_branches)/sizeof(tanker_branches[0]); ++i)
  {
    simgear::PropertyList tanker = ai_branch->getChildren(tanker_branches[i]);
    for(size_t j = 0; j < tanker.size(); ++j)
    {
      const std::string callsign = tanker[j]->getStringValue("callsign");

      if(    callsign.empty()
          || nav_name.find(callsign) == std::string::npos )
        continue;

      _vehicle_node = tanker[j];
      return;
    }
  }
}

//------------------------------------------------------------------------------
void FGMobileNavRecord::updatePos()
{
  SGTimeStamp now = SGTimeStamp::now();
  if( (now - _last_vehicle_update).toSecs() > (_vehicle_node.valid() ? 5 : 2) )
  {
    updateVehicle();
    _last_vehicle_update = now;
  }

  if( _vehicle_node.valid() )
    modifyPosition(SGGeod::fromDegFt(
      _vehicle_node->getDoubleValue("position/longitude-deg"),
      _vehicle_node->getDoubleValue("position/latitude-deg"),
      _vehicle_node->getNameString() == "carrier"
      ? _initial_elevation_ft
      : _vehicle_node->getDoubleValue("position/altitude-ft")
    ));
  else
    invalidatePosition();

  serviceable = _vehicle_node.valid();
}

//------------------------------------------------------------------------------
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
