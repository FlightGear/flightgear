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

#include <cassert>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/sg_inlines.h>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>
#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>
#include <Airports/pavement.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/xmlloader.hxx>
#include <Navaids/procedure.hxx>
#include <Navaids/waypoint.hxx>
#include <ATC/CommStation.hxx>

using std::vector;
using std::pair;

using namespace flightgear;

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
    _dynamics(0),
    mRunwaysLoaded(false),
    mTaxiwaysLoaded(true)
{
  init(true); // init FGPositioned
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

bool FGAirport::isAirportType(FGPositioned* pos)
{
    if (!pos) {
        return false;
    }
    
    return (pos->type() >= AIRPORT) && (pos->type() <= SEAPORT);
}

FGAirportDynamics * FGAirport::getDynamics()
{
    if (_dynamics) {
        return _dynamics;
    }
    
    _dynamics = new FGAirportDynamics(this);
    XMLLoader::load(_dynamics);

    FGRunwayPreference rwyPrefs(this);
    XMLLoader::load(&rwyPrefs);
    _dynamics->setRwyUse(rwyPrefs);
    XMLLoader::load(_dynamics->getSIDs());
    
    return _dynamics;
}

unsigned int FGAirport::numRunways() const
{
  loadRunways();
  return mRunways.size();
}

FGRunway* FGAirport::getRunwayByIndex(unsigned int aIndex) const
{
  loadRunways();
  
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
  if (aIdent.empty())
    return mRunways.end();

  loadRunways();
  
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
  loadRunways();
  
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

FGRunway* FGAirport::findBestRunwayForPos(const SGGeod& aPos) const
{
  loadRunways();
  
  Runway_iterator it = mRunways.begin();
  FGRunway* result = NULL;
  double currentLowestDev = 180.0;
  
  for (; it != mRunways.end(); ++it) {
    double inboundCourse = SGGeodesy::courseDeg(aPos, (*it)->end());
    double dev = inboundCourse - (*it)->headingDeg();
    SG_NORMALIZE_RANGE(dev, -180.0, 180.0);

    dev = fabs(dev);
    if (dev < currentLowestDev) { // new best match
      currentLowestDev = dev;
      result = *it;
    }
  } // of runway iteration
  
  return result;

}

bool FGAirport::hasHardRunwayOfLengthFt(double aLengthFt) const
{
  loadRunways();
  
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
  loadTaxiways();
  return mTaxiways.size();
}

FGTaxiway* FGAirport::getTaxiwayByIndex(unsigned int aIndex) const
{
  loadTaxiways();
  assert(aIndex >= 0 && aIndex < mTaxiways.size());
  return mTaxiways[aIndex];
}

unsigned int FGAirport::numPavements() const
{
  loadTaxiways();
  return mPavements.size();
}

FGPavement* FGAirport::getPavementByIndex(unsigned int aIndex) const
{
  loadTaxiways();
  assert(aIndex >= 0 && aIndex < mPavements.size());
  return mPavements[aIndex];
}

void FGAirport::setRunwaysAndTaxiways(vector<FGRunwayPtr>& rwys,
       vector<FGTaxiwayPtr>& txwys,
       vector<FGPavementPtr>& pvts)
{
  mRunways.swap(rwys);
  Runway_iterator it = mRunways.begin();
  for (; it != mRunways.end(); ++it) {
    (*it)->setAirport(this);
  }

  mTaxiways.swap(txwys);
  mPavements.swap(pvts);
}

FGRunway* FGAirport::getActiveRunwayForUsage() const
{
  static FGEnvironmentMgr* envMgr = NULL;
  if (!envMgr) {
    envMgr = (FGEnvironmentMgr *) globals->get_subsystem("environment");
  }
  
  // This forces West-facing rwys to be used in no-wind situations
  // which is consistent with Flightgear's initial setup.
  double hdg = 270;
  
  if (envMgr) {
    FGEnvironment stationWeather(envMgr->getEnvironment(mPosition));
  
    double windSpeed = stationWeather.get_wind_speed_kt();
    if (windSpeed > 0.0) {
      hdg = stationWeather.get_wind_from_heading_deg();
    }
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
  if (minLengthFt < 0.0) {
    mMinLengthFt = fgGetDouble("/sim/navdb/min-runway-length-ft", 0.0);
  }
}
      
bool FGAirport::HardSurfaceFilter::passAirport(FGAirport* aApt) const
{
  return aApt->hasHardRunwayOfLengthFt(mMinLengthFt);
}

FGAirport* FGAirport::findByIdent(const std::string& aIdent)
{
  FGPositionedRef r;
  PortsFilter filter;
  r = FGPositioned::findNextWithPartialId(r, aIdent, &filter);
  if (!r) {
    return NULL; // we don't warn here, let the caller do that
  }
  return static_cast<FGAirport*>(r.ptr());
}

FGAirport* FGAirport::getByIdent(const std::string& aIdent)
{
  FGPositionedRef r;
  PortsFilter filter;
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

void FGAirport::loadRunways() const
{
  if (mRunwaysLoaded) {
    return; // already loaded, great
  }
  
  mRunwaysLoaded = true;
  loadSceneryDefinitions();
}

void FGAirport::loadTaxiways() const
{
  if (mTaxiwaysLoaded) {
    return; // already loaded, great
  }
}

void FGAirport::loadProcedures() const
{
  if (mProceduresLoaded) {
    return;
  }
  
  mProceduresLoaded = true;
  SGPath path;
  if (!XMLLoader::findAirportData(ident(), "procedures", path)) {
    SG_LOG(SG_GENERAL, SG_INFO, "no procedures data available for " << ident());
    return;
  }
  
  SG_LOG(SG_GENERAL, SG_INFO, ident() << ": loading procedures from " << path.str());
  RouteBase::loadAirportProcedures(path, const_cast<FGAirport*>(this));
}

void FGAirport::loadSceneryDefinitions() const
{  
  SGPath path;
  SGPropertyNode_ptr rootNode = new SGPropertyNode;
  if (XMLLoader::findAirportData(ident(), "threshold", path)) {
    readProperties(path.str(), rootNode);
    const_cast<FGAirport*>(this)->readThresholdData(rootNode);
  }
  
  // repeat for the tower data
  rootNode = new SGPropertyNode;
  if (XMLLoader::findAirportData(ident(), "twr", path)) {
    readProperties(path.str(), rootNode);
    const_cast<FGAirport*>(this)->readTowerData(rootNode);
  }
}

void FGAirport::readThresholdData(SGPropertyNode* aRoot)
{
  SGPropertyNode* runway;
  int runwayIndex = 0;
  for (; (runway = aRoot->getChild("runway", runwayIndex)) != NULL; ++runwayIndex) {
    SGPropertyNode* t0 = runway->getChild("threshold", 0),
      *t1 = runway->getChild("threshold", 1);
    assert(t0);
    assert(t1); // too strict? mayeb we should finally allow single-ended runways
    
    processThreshold(t0);
    processThreshold(t1);
  } // of runways iteration
}

void FGAirport::processThreshold(SGPropertyNode* aThreshold)
{
  // first, let's identify the current runway
  string id(aThreshold->getStringValue("rwy"));
  if (!hasRunwayWithIdent(id)) {
    SG_LOG(SG_GENERAL, SG_DEBUG, "FGAirport::processThreshold: "
      "found runway not defined in the global data:" << ident() << "/" << id);
    return;
  }
  
  FGRunway* rwy = getRunwayByIdent(id);
  rwy->processThreshold(aThreshold);
}

void FGAirport::readTowerData(SGPropertyNode* aRoot)
{
  SGPropertyNode* twrNode = aRoot->getChild("tower")->getChild("twr");
  double lat = twrNode->getDoubleValue("lat"), 
    lon = twrNode->getDoubleValue("lon"), 
    elevM = twrNode->getDoubleValue("elev-m");  
// tower elevation is AGL, not AMSL. Since we don't want to depend on the
// scenery for a precise terrain elevation, we use the field elevation
// (this is also what the apt.dat code does)
  double fieldElevationM = geod().getElevationM();
  
  _tower_location = SGGeod::fromDegM(lon, lat, fieldElevationM + elevM);
}

void FGAirport::addSID(flightgear::SID* aSid)
{
  mSIDs.push_back(aSid);
}

void FGAirport::addSTAR(STAR* aStar)
{
  mSTARs.push_back(aStar);
}

void FGAirport::addApproach(Approach* aApp)
{
  mApproaches.push_back(aApp);
}

unsigned int FGAirport::numSIDs() const
{
  loadProcedures();
  return mSIDs.size();
}

flightgear::SID* FGAirport::getSIDByIndex(unsigned int aIndex) const
{
  loadProcedures();
  return mSIDs[aIndex];
}

flightgear::SID* FGAirport::findSIDWithIdent(const std::string& aIdent) const
{
  loadProcedures();
  for (unsigned int i=0; i<mSIDs.size(); ++i) {
    if (mSIDs[i]->ident() == aIdent) {
      return mSIDs[i];
    }
  }
  
  return NULL;
}

unsigned int FGAirport::numSTARs() const
{
  loadProcedures();
  return mSTARs.size();
}

STAR* FGAirport::getSTARByIndex(unsigned int aIndex) const
{
  loadProcedures();
  return mSTARs[aIndex];
}

STAR* FGAirport::findSTARWithIdent(const std::string& aIdent) const
{
  loadProcedures();
  for (unsigned int i=0; i<mSTARs.size(); ++i) {
    if (mSTARs[i]->ident() == aIdent) {
      return mSTARs[i];
    }
  }
  
  return NULL;
}

unsigned int FGAirport::numApproaches() const
{
  loadProcedures();
  return mApproaches.size();
}

Approach* FGAirport::getApproachByIndex(unsigned int aIndex) const
{
  loadProcedures();
  return mApproaches[aIndex];
}

Approach* FGAirport::findApproachWithIdent(const std::string& aIdent) const
{
  loadProcedures();
  for (unsigned int i=0; i<mApproaches.size(); ++i) {
    if (mApproaches[i]->ident() == aIdent) {
      return mApproaches[i];
    }
  }
  
  return NULL;
}

void FGAirport::setCommStations(CommStationList& comms)
{
    mCommStations.swap(comms);
    for (unsigned int c=0; c<mCommStations.size(); ++c) {
        mCommStations[c]->setAirport(this);
    }
}

CommStationList
FGAirport::commStationsOfType(FGPositioned::Type aTy) const
{
    CommStationList result;
    for (unsigned int c=0; c<mCommStations.size(); ++c) {
        if (mCommStations[c]->type() == aTy) {
            result.push_back(mCommStations[c]);
        }
    }
    return result;
}

// get airport elevation
double fgGetAirportElev( const string& id )
{
    const FGAirport *a=fgFindAirportID( id);
    if (a) {
        return a->getElevation();
    } else {
        return -9999.0;
    }
}


// get airport position
SGGeod fgGetAirportPos( const string& id )
{
    const FGAirport *a = fgFindAirportID( id);

    if (a) {
        return SGGeod::fromDegM(a->getLongitude(), a->getLatitude(), a->getElevation());
    } else {
        return SGGeod::fromDegM(0.0, 0.0, -9999.0);
    }
}
