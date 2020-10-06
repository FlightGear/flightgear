// airport.cxx -- Classes representing airports, seaports and helipads
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

#include "airport.hxx"

#include <algorithm>
#include <cassert>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/sg_inlines.h>
#include <simgear/structure/exception.hxx>

#include <Environment/environment_mgr.hxx>
#include <Environment/environment.hxx>
#include <Main/fg_props.hxx>
#include <Airports/runways.hxx>
#include <Airports/pavement.hxx>
#include <Airports/xmlloader.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/airportdynamicsmanager.hxx>
#include <Navaids/procedure.hxx>
#include <Navaids/waypoint.hxx>
#include <ATC/CommStation.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/positioned.hxx>
#include <Airports/groundnetwork.hxx>
#include <Airports/xmlloader.hxx>

using std::vector;
using std::pair;

using namespace flightgear;

/***************************************************************************
 * FGAirport
 ***************************************************************************/

AirportCache FGAirport::airportCache;

FGAirport::FGAirport( PositionedID aGuid,
                      const std::string &id,
                      const SGGeod& location,
                      const std::string &name,
                      bool has_metar,
                      Type aType ):
    FGPositioned(aGuid, aType, id, location),
    _name(name),
    _has_metar(has_metar),
    mTowerDataLoaded(false),
    mHasTower(false),
    mRunwaysLoaded(false),
    mHelipadsLoaded(false),
    mTaxiwaysLoaded(false),
    mProceduresLoaded(false),
    mThresholdDataLoaded(false),
    mILSDataLoaded(false)
{
    mIsClosed = (name.find("[x]") != std::string::npos);
}


FGAirport::~FGAirport()
{
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

//------------------------------------------------------------------------------
unsigned int FGAirport::numRunways() const
{
  loadRunways();
  return mRunways.size();
}

//------------------------------------------------------------------------------
unsigned int FGAirport::numHelipads() const
{
  loadHelipads();
  return mHelipads.size();
}

//------------------------------------------------------------------------------
FGRunwayRef FGAirport::getRunwayByIndex(unsigned int aIndex) const
{
  loadRunways();
  return mRunways.at(aIndex);
}

//------------------------------------------------------------------------------
FGHelipadRef FGAirport::getHelipadByIndex(unsigned int aIndex) const
{
  loadHelipads();
  return loadById<FGHelipad>(mHelipads, aIndex);
}

//------------------------------------------------------------------------------
FGRunwayMap FGAirport::getRunwayMap() const
{
  loadRunways();
  FGRunwayMap map;

  double minLengthFt = fgGetDouble("/sim/navdb/min-runway-length-ft");

  for (auto rwy : mRunways) {
    // ignore unusably short runways
    // TODO other methods don't check this...
    if( rwy->lengthFt() >= minLengthFt )
      map[ rwy->ident() ] = rwy;
  }

  return map;
}

//------------------------------------------------------------------------------
FGHelipadMap FGAirport::getHelipadMap() const
{
  loadHelipads();
  FGHelipadMap map;

  for (auto id : mHelipads) {
    FGHelipad* rwy = loadById<FGHelipad>(id);
    map[ rwy->ident() ] = rwy;
  }

  return map;
}

//------------------------------------------------------------------------------
bool FGAirport::hasRunwayWithIdent(const std::string& aIdent) const
{
  loadRunways();
  for (auto rwy : mRunways) {
    if (rwy->ident() == aIdent) {
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------------------
bool FGAirport::hasHelipadWithIdent(const std::string& aIdent) const
{
  return flightgear::NavDataCache::instance()
    ->airportItemWithIdent(guid(), FGPositioned::HELIPAD, aIdent) != 0;
}

//------------------------------------------------------------------------------
FGRunwayRef FGAirport::getRunwayByIdent(const std::string& aIdent) const
{
    if (aIdent.empty())
        return {};

    loadRunways();
    for (auto rwy : mRunways) {
        if (rwy->ident() == aIdent) {
            return rwy;
        }
  }

  SG_LOG(SG_GENERAL, SG_ALERT, "no such runway '" << aIdent << "' at airport " << ident());
  throw sg_range_exception("unknown runway " + aIdent + " at airport:" + ident(), "FGAirport::getRunwayByIdent");
}

//------------------------------------------------------------------------------
FGHelipadRef FGAirport::getHelipadByIdent(const std::string& aIdent) const
{
  PositionedID id = flightgear::NavDataCache::instance()->airportItemWithIdent(guid(), FGPositioned::HELIPAD, aIdent);
  if (id == 0) {
    SG_LOG(SG_GENERAL, SG_ALERT, "no such helipad '" << aIdent << "' at airport " << ident());
    throw sg_range_exception("unknown helipad " + aIdent + " at airport:" + ident(), "FGAirport::getRunwayByIdent");
  }

  return loadById<FGHelipad>(id);
}

//------------------------------------------------------------------------------
FGRunwayRef FGAirport::findBestRunwayForHeading(double aHeading, struct FindBestRunwayForHeadingParams * parms ) const
{
  loadRunways();

  FGRunway* result = NULL;
  double currentBestQuality = 0.0;

  struct FindBestRunwayForHeadingParams fbrfhp;
  if( NULL != parms ) fbrfhp = *parms;

  SGPropertyNode_ptr searchNode = fgGetNode("/sim/airport/runways/search");
  if( searchNode.valid() ) {
    fbrfhp.lengthWeight = searchNode->getDoubleValue("length-weight", fbrfhp.lengthWeight );
    fbrfhp.widthWeight = searchNode->getDoubleValue("width-weight", fbrfhp.widthWeight );
    fbrfhp.surfaceWeight = searchNode->getDoubleValue("surface-weight", fbrfhp.surfaceWeight );
    fbrfhp.deviationWeight = searchNode->getDoubleValue("deviation-weight", fbrfhp.deviationWeight );
    fbrfhp.ilsWeight = searchNode->getDoubleValue("ils-weight", fbrfhp.ilsWeight );
  }

  for (auto rwy : mRunways) {
    double good = rwy->score( fbrfhp.lengthWeight,  fbrfhp.widthWeight,  fbrfhp.surfaceWeight,  fbrfhp.ilsWeight );
    double dev = aHeading - rwy->headingDeg();
    SG_NORMALIZE_RANGE(dev, -180.0, 180.0);
    double bad = fabs( fbrfhp.deviationWeight * dev) + 1e-20;
    double quality = good / bad;

    if (quality > currentBestQuality) {
      currentBestQuality = quality;
      result = rwy;
    }
  }

  return result;
}

//------------------------------------------------------------------------------
FGRunwayRef FGAirport::findBestRunwayForPos(const SGGeod& aPos) const
{
  loadRunways();

  FGRunway* result = NULL;
  double currentLowestDev = 180.0;

  for (auto rwy : mRunways) {
    double inboundCourse = SGGeodesy::courseDeg(aPos, rwy->end());
    double dev = inboundCourse - rwy->headingDeg();
    SG_NORMALIZE_RANGE(dev, -180.0, 180.0);

    dev = fabs(dev);
    if (dev < currentLowestDev) { // new best match
      currentLowestDev = dev;
      result = rwy;
    }
  } // of runway iteration

  return result;

}

//------------------------------------------------------------------------------
bool FGAirport::hasHardRunwayOfLengthFt(double aLengthFt) const
{
  loadRunways();

  for (auto rwy : mRunways) {
    if (rwy->isHardSurface() && (rwy->lengthFt() >= aLengthFt)) {
      return true; // we're done!
    }
  } // of runways iteration

  return false;
}

FGRunwayRef FGAirport::longestRunway() const
{
    FGRunwayRef r;
    loadRunways();

    for (auto rwy : mRunways) {
        if (!r || (r->lengthFt() < rwy->lengthFt())) {
             r = rwy;
        }
    } // of runways iteration

    return r;
}

//------------------------------------------------------------------------------
FGRunwayList FGAirport::getRunways() const
{
  loadRunways();

  return mRunways;
}

//------------------------------------------------------------------------------
FGRunwayList FGAirport::getRunwaysWithoutReciprocals() const
{
  loadRunways();

  FGRunwayList r;

  for (auto rwy : mRunways) {
    FGRunway* recip = rwy->reciprocalRunway();
    if (recip) {
      FGRunwayList::iterator it = std::find(r.begin(), r.end(), recip);
      if (it != r.end()) {
        continue; // reciprocal already in result set, don't include us
      }
    }

    r.push_back(rwy);
  }

  return r;
}

//------------------------------------------------------------------------------
unsigned int FGAirport::numTaxiways() const
{
  loadTaxiways();
  return mTaxiways.size();
}

//------------------------------------------------------------------------------
FGTaxiwayRef FGAirport::getTaxiwayByIndex(unsigned int aIndex) const
{
  loadTaxiways();
  return loadById<FGTaxiway>(mTaxiways, aIndex);
}

//------------------------------------------------------------------------------
FGTaxiwayList FGAirport::getTaxiways() const
{
  loadTaxiways();
  return loadAllById<FGTaxiway>(mTaxiways);
}

//------------------------------------------------------------------------------
unsigned int FGAirport::numPavements() const
{
  return mPavements.size();
}

//------------------------------------------------------------------------------
FGPavementList FGAirport::getPavements() const
{
  return mPavements;
}

void FGAirport::addPavement(FGPavementRef pavement)
{
  mPavements.push_back(pavement);
}

//------------------------------------------------------------------------------
unsigned int FGAirport::numBoundary() const
{
  return mBoundary.size();
}

//------------------------------------------------------------------------------
FGPavementList FGAirport::getBoundary() const
{
  return mBoundary;
}

void FGAirport::addBoundary(FGPavementRef boundary)
{
  mBoundary.push_back(boundary);
}

//------------------------------------------------------------------------------
unsigned int FGAirport::numLineFeatures() const
{
  return mLineFeatures.size();
}

//------------------------------------------------------------------------------
FGPavementList FGAirport::getLineFeatures() const
{
  return mLineFeatures;
}

void FGAirport::addLineFeature(FGPavementRef linefeature)
{
  mLineFeatures.push_back(linefeature);
}

//------------------------------------------------------------------------------
FGRunwayRef FGAirport::getActiveRunwayForUsage() const
{
  auto envMgr = globals->get_subsystem<FGEnvironmentMgr>();

  // This forces West-facing rwys to be used in no-wind situations
  // which is consistent with Flightgear's initial setup.
  double hdg = 270;

  if (envMgr) {
    FGEnvironment stationWeather(envMgr->getEnvironment(geod()));

    double windSpeed = stationWeather.get_wind_speed_kt();
    if (windSpeed > 0.0) {
      hdg = stationWeather.get_wind_from_heading_deg();
    }
  }

  return findBestRunwayForHeading(hdg);
}

//------------------------------------------------------------------------------
FGAirportRef FGAirport::findClosest( const SGGeod& aPos,
                                     double aCuttofNm,
                                     Filter* filter )
{
  AirportFilter aptFilter;
  if( !filter )
    filter = &aptFilter;

  return static_pointer_cast<FGAirport>
  (
    FGPositioned::findClosest(aPos, aCuttofNm, filter)
  );
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

//------------------------------------------------------------------------------
FGAirport::TypeRunwayFilter::TypeRunwayFilter():
  _type(FGPositioned::AIRPORT),
  _min_runway_length_ft( fgGetDouble("/sim/navdb/min-runway-length-ft", 0.0) )
{

}

//------------------------------------------------------------------------------
bool FGAirport::TypeRunwayFilter::fromTypeString(const std::string& type)
{
  if(      type == "heliport" ) _type = FGPositioned::HELIPORT;
  else if( type == "seaport"  ) _type = FGPositioned::SEAPORT;
  else if( type == "airport"  ) _type = FGPositioned::AIRPORT;
  else                          return false;

  return true;
}

//------------------------------------------------------------------------------
bool FGAirport::TypeRunwayFilter::pass(FGPositioned* pos) const
{
  FGAirport* apt = static_cast<FGAirport*>(pos);
  if(  (apt->type() == FGPositioned::AIRPORT)
    && !apt->hasHardRunwayOfLengthFt(_min_runway_length_ft)
    )
    return false;

  return true;
}

void FGAirport::clearAirportsCache()
{
    airportCache.clear();
}

//------------------------------------------------------------------------------
FGAirportRef FGAirport::findByIdent(const std::string& aIdent)
{
  AirportCache::iterator it = airportCache.find(aIdent);
  if (it != airportCache.end())
   return it->second;

  PortsFilter filter;
  FGAirportRef r = static_pointer_cast<FGAirport>
  (
    FGPositioned::findFirstWithIdent(aIdent, &filter)
  );

  // add airport to the cache (even when it's NULL, so we don't need to search in vain again)
  airportCache[aIdent] = r;

  // we don't warn here when r==NULL, let the caller do that
  return r;
}

//------------------------------------------------------------------------------
FGAirportRef FGAirport::getByIdent(const std::string& aIdent)
{
  FGAirportRef r = findByIdent(aIdent);
  if (!r)
    throw sg_range_exception("No such airport with ident: " + aIdent);
  return r;
}

char** FGAirport::searchNamesAndIdents(const std::string& aFilter)
{
  return NavDataCache::instance()->searchAirportNamesAndIdents(aFilter);
}

// find basic airport location info from airport database
const FGAirport *fgFindAirportID( const std::string& id)
{
    if ( id.empty() ) {
        return NULL;
    }

    return FGAirport::findByIdent(id);
}

PositionedIDVec FGAirport::itemsOfType(FGPositioned::Type ty) const
{
  flightgear::NavDataCache* cache = flightgear::NavDataCache::instance();
  return cache->airportItemsOfType(guid(), ty);
}

void FGAirport::loadRunways() const
{
  if (mRunwaysLoaded) {
    return; // already loaded, great
  }

  loadSceneryDefinitions();

  mRunwaysLoaded = true;
  PositionedIDVec rwys(itemsOfType(FGPositioned::RUNWAY));
  for (auto id : rwys) {
    mRunways.push_back(loadById<FGRunway>(id));
  }
}

void FGAirport::loadHelipads() const
{
  if (mHelipadsLoaded) {
    return; // already loaded, great
  }

  mHelipadsLoaded = true;
  mHelipads = itemsOfType(FGPositioned::HELIPAD);
}

void FGAirport::loadTaxiways() const
{
  if (mTaxiwaysLoaded) {
    return; // already loaded, great
  }

  mTaxiwaysLoaded =  true;
  mTaxiways = itemsOfType(FGPositioned::TAXIWAY);
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

  SG_LOG(SG_GENERAL, SG_INFO, ident() << ": loading procedures from " << path);
  RouteBase::loadAirportProcedures(path, const_cast<FGAirport*>(this));
}

void FGAirport::loadSceneryDefinitions() const
{
  if (mThresholdDataLoaded) {
    return;
  }

  mThresholdDataLoaded = true;

  SGPath path;
  if (!XMLLoader::findAirportData(ident(), "threshold", path)) {
    return; // no XML threshold data
  }

  try {
    SGPropertyNode_ptr rootNode = new SGPropertyNode;
    readProperties(path, rootNode);
    const_cast<FGAirport*>(this)->readThresholdData(rootNode);
  } catch (sg_exception& e) {
    SG_LOG(SG_NAVAID, SG_WARN, ident() << "loading threshold XML failed:" << e.getFormattedMessage());
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
    assert(t1); // too strict? maybe we should finally allow single-ended runways

    processThreshold(t0);
    processThreshold(t1);
  } // of runways iteration
}

void FGAirport::processThreshold(SGPropertyNode* aThreshold)
{
  // first, let's identify the current runway
  std::string rwyIdent(aThreshold->getStringValue("rwy"));
  NavDataCache* cache = NavDataCache::instance();
  PositionedID id = cache->airportItemWithIdent(guid(), FGPositioned::RUNWAY, rwyIdent);

  double lon = aThreshold->getDoubleValue("lon"),
  lat = aThreshold->getDoubleValue("lat");
  SGGeod newThreshold(SGGeod::fromDegM(lon, lat, elevationM()));

  double newHeading = aThreshold->getDoubleValue("hdg-deg");
  double newDisplacedThreshold = aThreshold->getDoubleValue("displ-m");
  double newStopway = aThreshold->getDoubleValue("stopw-m");

  if (id == 0) {
    SG_LOG(SG_GENERAL, SG_DEBUG, "FGAirport::processThreshold: "
           "found runway not defined in the global data:" << ident() << "/" << rwyIdent);
    // enable this code when threshold.xml contains sufficient data to
    // fully specify a new runway, *and* we figure out how to assign runtime
    // Positioned IDs and insert temporary items into the spatial map.
#if 0
    double newLength = 0.0, newWidth = 0.0;
    int surfaceCode = 0;
    FGRunway* rwy = new FGRunway(id, guid(), rwyIdent, newThreshold,
                       newHeading,
                       newLength, newWidth,
                       newDisplacedThreshold, newStopway,
                       surfaceCode);
    // insert into the spatial map too
    mRunways.push_back(rwy);
#endif
  } else {
    FGRunway* rwy = loadById<FGRunway>(id);
    rwy->updateThreshold(newThreshold, newHeading,
                         newDisplacedThreshold, newStopway);

  }
}

SGGeod FGAirport::getTowerLocation() const
{
  validateTowerData();
  return mTowerPosition;
}

void FGAirport::validateTowerData() const
{
  if (mTowerDataLoaded) {
    return;
  }

  mTowerDataLoaded = true;

// first, load data from the cache (apt.dat)
  NavDataCache* cache = NavDataCache::instance();
  PositionedIDVec towers = cache->airportItemsOfType(guid(), FGPositioned::TOWER);
  if (towers.empty()) {
    mHasTower = false;
    mTowerPosition = geod(); // use airport position

    // offset the tower position away from the runway centerline, if
    // airport has a single runway. Offset by eight times the runway width,
    // an entirely guessed figure.
    int runwayCount = numRunways();
    if ((runwayCount > 0) && (runwayCount <= 2)) {
        FGRunway* runway = getRunwayByIndex(0);
        double hdg = runway->headingDeg() + 90;
        mTowerPosition = SGGeodesy::direct(geod(), hdg, runway->widthM() * 8);
    }

    // increase tower elevation by 20 metres above the field elevation
    mTowerPosition.setElevationM(geod().getElevationM() + 20.0);
  } else {
    FGPositionedRef tower = cache->loadById(towers.front());
    mTowerPosition = tower->geod();
    mHasTower = true;
  }

  SGPath path;
  if (!XMLLoader::findAirportData(ident(), "twr", path)) {
    return; // no XML tower data, base position is fine
  }

  try {
    SGPropertyNode_ptr rootNode = new SGPropertyNode;
    readProperties(path, rootNode);
    const_cast<FGAirport*>(this)->readTowerData(rootNode);
    mHasTower = true;
  } catch (sg_exception& e){
    SG_LOG(SG_NAVAID, SG_WARN, ident() << "loading twr XML failed:" << e.getFormattedMessage());
  }
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
  mTowerPosition = SGGeod::fromDegM(lon, lat, fieldElevationM + elevM);
}

void FGAirport::validateILSData()
{
  if (mILSDataLoaded) {
    return;
  }

  // to avoid re-entrancy on this code-path, ensure we set loaded
  // immediately.
  mILSDataLoaded = true;

  SGPath path;
  if (!XMLLoader::findAirportData(ident(), "ils", path)) {
    return; // no XML ILS data
  }

  try {
      SGPropertyNode_ptr rootNode = new SGPropertyNode;
      readProperties(path, rootNode);
      readILSData(rootNode);
  } catch (sg_exception& e){
      SG_LOG(SG_NAVAID, SG_WARN, ident() << "loading ils XML failed:" << e.getFormattedMessage());
  }
}

bool FGAirport::hasTower() const
{
    validateTowerData();
    return mHasTower;
}

void FGAirport::readILSData(SGPropertyNode* aRoot)
{
  NavDataCache* cache = NavDataCache::instance();
  // find the entry matching the runway
  SGPropertyNode* runwayNode, *ilsNode;
  for (int i=0; (runwayNode = aRoot->getChild("runway", i)) != NULL; ++i) {
    for (int j=0; (ilsNode = runwayNode->getChild("ils", j)) != NULL; ++j) {
      // must match on both nav-ident and runway ident, to support the following:
      // - runways with multiple distinct ILS installations (KEWD, for example)
      // - runways where both ends share the same nav ident (LFAT, for example)
      PositionedID ils = cache->findILS(guid(), ilsNode->getStringValue("rwy"),
                                        ilsNode->getStringValue("nav-id"));
      if (ils == 0) {
        SG_LOG(SG_GENERAL, SG_INFO, "reading ILS data for " << ident() <<
               ", couldn't find runway/navaid for:" <<
               ilsNode->getStringValue("rwy") << "/" <<
               ilsNode->getStringValue("nav-id"));
        continue;
      }

      double hdgDeg = ilsNode->getDoubleValue("hdg-deg"),
        lon = ilsNode->getDoubleValue("lon"),
        lat = ilsNode->getDoubleValue("lat"),
        elevM = ilsNode->getDoubleValue("elev-m");

      FGNavRecordRef nav(FGPositioned::loadById<FGNavRecord>(ils));
      assert(nav.valid());
      nav->updateFromXML(SGGeod::fromDegM(lon, lat, elevM), hdgDeg);
    } // of ILS iteration
  } // of runway iteration
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

//------------------------------------------------------------------------------
unsigned int FGAirport::numSIDs() const
{
  loadProcedures();
  return mSIDs.size();
}

//------------------------------------------------------------------------------
flightgear::SID* FGAirport::getSIDByIndex(unsigned int aIndex) const
{
  loadProcedures();
  return mSIDs[aIndex];
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
flightgear::SIDList FGAirport::getSIDs() const
{
  loadProcedures();
  return flightgear::SIDList(mSIDs.begin(), mSIDs.end());
}

//------------------------------------------------------------------------------
unsigned int FGAirport::numSTARs() const
{
  loadProcedures();
  return mSTARs.size();
}

//------------------------------------------------------------------------------
STAR* FGAirport::getSTARByIndex(unsigned int aIndex) const
{
  loadProcedures();
  return mSTARs[aIndex];
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
STARList FGAirport::getSTARs() const
{
  loadProcedures();
  return STARList(mSTARs.begin(), mSTARs.end());
}

unsigned int FGAirport::numApproaches() const
{
  loadProcedures();
  return mApproaches.size();
}

//------------------------------------------------------------------------------
Approach* FGAirport::getApproachByIndex(unsigned int aIndex) const
{
  loadProcedures();
  return mApproaches[aIndex];
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
ApproachList FGAirport::getApproaches(ProcedureType type) const
{
  loadProcedures();
  if( type == PROCEDURE_INVALID )
    return ApproachList(mApproaches.begin(), mApproaches.end());

  ApproachList ret;
  for(size_t i = 0; i < mApproaches.size(); ++i)
  {
    if( mApproaches[i]->type() == type )
      ret.push_back(mApproaches[i]);
  }
  return ret;
}

CommStationList
FGAirport::commStations() const
{
  NavDataCache* cache = NavDataCache::instance();
  CommStationList result;
  for (auto pos : cache->airportItemsOfType(guid(),
                                            FGPositioned::FREQ_GROUND,
                                            FGPositioned::FREQ_UNICOM)) {
    result.push_back( loadById<CommStation>(pos) );
  }

  return result;
}

CommStationList
FGAirport::commStationsOfType(FGPositioned::Type aTy) const
{
  NavDataCache* cache = NavDataCache::instance();
  CommStationList result;
  for (auto pos : cache->airportItemsOfType(guid(), aTy)) {
    result.push_back( loadById<CommStation>(pos) );
  }

  return result;
}

class AirportWithSize
{
public:
    AirportWithSize(FGPositionedRef pos) :
        _pos(pos),
        _sizeMetric(0)
    {
        assert(pos->type() == FGPositioned::AIRPORT);
        FGAirport* apt = static_cast<FGAirport*>(pos.get());
        for (auto rwy : apt->getRunwaysWithoutReciprocals()) {
            _sizeMetric += static_cast<int>(rwy->lengthFt());
        }
    }

    bool operator<(const AirportWithSize& other) const
    {
        return _sizeMetric < other._sizeMetric;
    }

    FGPositionedRef pos() const
    { return _pos; }
private:
    FGPositionedRef _pos;
    unsigned int _sizeMetric;

};

void FGAirport::sortBySize(FGPositionedList& airportList)
{
    std::vector<AirportWithSize> annotated;
    for (auto p : airportList) {
        annotated.push_back(AirportWithSize(p));
    }
    std::sort(annotated.begin(), annotated.end());

    for (unsigned int i=0; i<annotated.size(); ++i) {
        airportList[i] = annotated[i].pos();
    }
}

#if defined(BUILDING_TESTSUITE)

void FGAirport::testSuiteInjectGroundnetXML(const SGPath& path)
{
    _groundNetwork.reset(new FGGroundNetwork(const_cast<FGAirport*>(this)));
    XMLLoader::loadFromPath(_groundNetwork.get(), path);
    _groundNetwork->init();
}

#endif

FGAirportDynamicsRef FGAirport::getDynamics() const
{
    return flightgear::AirportDynamicsManager::find(const_cast<FGAirport*>(this));
}

FGGroundNetwork *FGAirport::groundNetwork() const
{
    if (!_groundNetwork.get()) {
        _groundNetwork.reset(new FGGroundNetwork(const_cast<FGAirport*>(this)));
        XMLLoader::load(_groundNetwork.get());
        _groundNetwork->init();
    }

    return _groundNetwork.get();
}

flightgear::Transition* FGAirport::selectSIDByEnrouteTransition(FGPositioned* enroute) const
{
    loadProcedures();
    for (auto sid : mSIDs) {
        auto trans = sid->findTransitionByEnroute(enroute);
        if (trans) {
            return trans;
        }
    }
    return nullptr;
}

Transition *FGAirport::selectSIDByTransition(const FGRunway* runway,  const string &aIdent) const
{
    loadProcedures();
    for (auto sid : mSIDs) {
        if (runway && !sid->isForRunway(runway))
            continue;

        auto trans = sid->findTransitionByName(aIdent);
        if (trans) {
            return trans;
        }
    }
    return nullptr;
}

flightgear::Transition* FGAirport::selectSTARByEnrouteTransition(FGPositioned* enroute) const
{
    loadProcedures();
    for (auto star : mSTARs) {
        auto trans = star->findTransitionByEnroute(enroute);
        if (trans) {
            return trans;
        }
    }
    return nullptr;
}

Transition *FGAirport::selectSTARByTransition(const FGRunway* runway, const string &aIdent) const
{
    loadProcedures();
    for (auto star : mSTARs) {
        if (runway && !star->isForRunway(runway))
            continue;

        auto trans = star->findTransitionByName(aIdent);
        if (trans) {
            return trans;
        }
    }
    return nullptr;
}

// get airport elevation
double fgGetAirportElev( const std::string& id )
{
    const FGAirport *a=fgFindAirportID( id);
    if (a) {
        return a->getElevation();
    } else {
        return -9999.0;
    }
}


// get airport position
SGGeod fgGetAirportPos( const std::string& id )
{
    const FGAirport *a = fgFindAirportID( id);

    if (a) {
        return SGGeod::fromDegM(a->getLongitude(), a->getLatitude(), a->getElevation());
    } else {
        return SGGeod::fromDegM(0.0, 0.0, -9999.0);
    }
}
