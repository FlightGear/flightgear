//
// simple.cxx -- a really simplistic class to manage airport ID,
//               lat, lon of the center of one of it's runways, and
//               elevation in feet.
//
// Written by Curtis Olson, started April 1998.
// Updated by Durk Talsma, started December, 2004.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include "simple.hxx"

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/sg_inlines.h>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>
#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/xmlloader.hxx>

// magic import of a helper which uses FGPositioned internals
extern char** searchAirportNamesAndIdents(const std::string& aFilter);

/***************************************************************************
 * FGAirport
 ***************************************************************************/

FGAirport::FGAirport(const string &id, const SGGeod& location, const SGGeod& tower_location,
        const string &name, bool has_metar, Type aType) :
    FGPositioned(aType, id, location),
    _tower_location(tower_location),
    _name(name),
    _has_metar(has_metar),
    _dynamics(0)
{
}


FGAirport::~FGAirport()
{
    delete _dynamics;
}

bool FGAirport::isAirport() const
{
  return type() == AIRPORT;
}

bool FGAirport::isSeaport() const
{
  return type() == SEAPORT;
}

bool FGAirport::isHeliport() const
{
  return type() == HELIPORT;
}

FGAirportDynamics * FGAirport::getDynamics()
{
    if (_dynamics != 0) {
        return _dynamics;
    } else {
        //cerr << "Trying to load dynamics for " << _id << endl;
        _dynamics = new FGAirportDynamics(this);
        XMLLoader::load(_dynamics);

        FGRunwayPreference rwyPrefs(this);
        XMLLoader::load(&rwyPrefs);
        _dynamics->setRwyUse(rwyPrefs);
   }
    return _dynamics;
}

unsigned int FGAirport::numRunways() const
{
  return mRunways.size();
}

FGRunway* FGAirport::getRunwayByIndex(unsigned int aIndex) const
{
  assert(aIndex >= 0 && aIndex < mRunways.size());
  return mRunways[aIndex];
}

bool FGAirport::hasRunwayWithIdent(const string& aIdent) const
{
  return (getIteratorForRunwayIdent(aIdent) != mRunways.end());
}

FGRunway* FGAirport::getRunwayByIdent(const string& aIdent) const
{
  Runway_iterator it = getIteratorForRunwayIdent(aIdent);
  if (it == mRunways.end()) {
    SG_LOG(SG_GENERAL, SG_ALERT, "no such runway '" << aIdent << "' at airport " << ident());
    throw sg_range_exception("unknown runway " + aIdent + " at airport:" + ident(), "FGAirport::getRunwayByIdent");
  }
  
  return *it;
}

FGAirport::Runway_iterator
FGAirport::getIteratorForRunwayIdent(const string& aIdent) const
{
  string ident(aIdent);
  if ((aIdent.size() == 1) || !isdigit(aIdent[1])) {
    ident = "0" + aIdent;
  }

  Runway_iterator it = mRunways.begin();
  for (; it != mRunways.end(); ++it) {
    if ((*it)->ident() == ident) {
      return it;
    }
  }

  return it; // end()
}

FGRunway* FGAirport::findBestRunwayForHeading(double aHeading) const
{
  Runway_iterator it = mRunways.begin();
  FGRunway* result = NULL;
  double currentBestQuality = 0.0;
  
  SGPropertyNode *param = fgGetNode("/sim/airport/runways/search", true);
  double lengthWeight = param->getDoubleValue("length-weight", 0.01);
  double widthWeight = param->getDoubleValue("width-weight", 0.01);
  double surfaceWeight = param->getDoubleValue("surface-weight", 10);
  double deviationWeight = param->getDoubleValue("deviation-weight", 1);
    
  for (; it != mRunways.end(); ++it) {
    double good = (*it)->score(lengthWeight, widthWeight, surfaceWeight);
    
    double dev = aHeading - (*it)->headingDeg();
    SG_NORMALIZE_RANGE(dev, -180.0, 180.0);
    double bad = fabs(deviationWeight * dev) + 1e-20;
    double quality = good / bad;
    
    if (quality > currentBestQuality) {
      currentBestQuality = quality;
      result = *it;
    }
  }

  return result;
}

bool FGAirport::hasHardRunwayOfLengthFt(double aLengthFt) const
{
  unsigned int numRunways(mRunways.size());
  for (unsigned int r=0; r<numRunways; ++r) {
    FGRunway* rwy = mRunways[r];
    if (rwy->isReciprocal()) {
      continue; // we only care about lengths, so don't do work twice
    }

    if (rwy->isHardSurface() && (rwy->lengthFt() >= aLengthFt)) {
      return true; // we're done!
    }
  } // of runways iteration

  return false;
}

unsigned int FGAirport::numTaxiways() const
{
  return mTaxiways.size();
}

FGRunway* FGAirport::getTaxiwayByIndex(unsigned int aIndex) const
{
  assert(aIndex >= 0 && aIndex < mTaxiways.size());
  return mTaxiways[aIndex];
}

void FGAirport::addRunway(FGRunway* aRunway)
{
  aRunway->setAirport(this);
  
  if (aRunway->isTaxiway()) {
    mTaxiways.push_back(aRunway);
  } else {
    mRunways.push_back(aRunway);
  }
}

FGRunway* FGAirport::getActiveRunwayForUsage() const
{
  static FGEnvironmentMgr* envMgr = NULL;
  if (!envMgr) {
    envMgr = (FGEnvironmentMgr *) globals->get_subsystem("environment");
  }
  
  FGEnvironment stationWeather(envMgr->getEnvironment(mPosition));
  
  double windSpeed = stationWeather.get_wind_speed_kt();
  double hdg = stationWeather.get_wind_from_heading_deg();
  if (windSpeed <= 0.0) {
    hdg = 270;	// This forces West-facing rwys to be used in no-wind situations
    // which is consistent with Flightgear's initial setup.
  }
  
  return findBestRunwayForHeading(hdg);
}

FGAirport* FGAirport::findClosest(const SGGeod& aPos, double aCuttofNm, Filter* filter)
{
  AirportFilter aptFilter;
  if (filter == NULL) {
    filter = &aptFilter;
  }
  
  FGPositionedRef r = FGPositioned::findClosest(aPos, aCuttofNm, filter);
  if (!r) {
    return NULL;
  }
  
  return static_cast<FGAirport*>(r.ptr());
}

FGAirport::HardSurfaceFilter::HardSurfaceFilter(double minLengthFt) :
  mMinLengthFt(minLengthFt)
{
}
      
bool FGAirport::HardSurfaceFilter::pass(FGPositioned* aPos) const
{
  if (aPos->type() != AIRPORT) {
    return false; // exclude seaports and heliports as well, we need a runways
  }
   
  return static_cast<FGAirport*>(aPos)->hasHardRunwayOfLengthFt(mMinLengthFt);
}

FGAirport* FGAirport::findByIdent(const std::string& aIdent)
{
  FGPositionedRef r;
  AirportFilter filter;
  r = FGPositioned::findNextWithPartialId(r, aIdent, &filter);
  if (!r) {
    return NULL; // we don't warn here, let the caller do that
  }
  return static_cast<FGAirport*>(r.ptr());
}

FGAirport* FGAirport::getByIdent(const std::string& aIdent)
{
  FGPositionedRef r;
  AirportFilter filter;
  r = FGPositioned::findNextWithPartialId(r, aIdent, &filter);
  if (!r) {
    throw sg_range_exception("No such airport with ident: " + aIdent);
  }
  return static_cast<FGAirport*>(r.ptr());
}

char** FGAirport::searchNamesAndIdents(const std::string& aFilter)
{
  // we delegate all the work to a horrible helper in FGPositioned, which can
  // access the (private) index data.
  return searchAirportNamesAndIdents(aFilter);
}

// find basic airport location info from airport database
const FGAirport *fgFindAirportID( const string& id)
{
    if ( id.empty() ) {
        return NULL;
    }
    
    return FGAirport::findByIdent(id);
}


// get airport elevation
double fgGetAirportElev( const string& id )
{
    SG_LOG( SG_GENERAL, SG_BULK,
            "Finding elevation for airport: " << id );

    const FGAirport *a=fgFindAirportID( id);
    if (a) {
        return a->getElevation();
    } else {
        return -9999.0;
    }
}


// get airport position
Point3D fgGetAirportPos( const string& id )
{
    SG_LOG( SG_ATC, SG_BULK,
            "Finding position for airport: " << id );

    const FGAirport *a = fgFindAirportID( id);

    if (a) {
        return Point3D(a->getLongitude(), a->getLatitude(), a->getElevation());
    } else {
        return Point3D(0.0, 0.0, -9999.0);
    }
}
