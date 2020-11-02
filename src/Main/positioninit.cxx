// positioninit.cxx - helpers relating to setting initial aircraft position
//
// Copyright (C) 2012 James Turner  zakalawe@mac.com
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

#include "config.h"

#include "positioninit.hxx"

#include <osgViewer/Viewer>
#include <osg/PagedLOD>

// simgear
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/scene/model/CheckSceneryVisitor.hxx>
#include <simgear/scene/util/OsgMath.hxx>

#include "globals.hxx"
#include "fg_props.hxx"
#include "fg_io.hxx"

#include <Navaids/navlist.hxx>
#include <Airports/runways.hxx>
#include <Airports/airport.hxx>
#include <Airports/dynamics.hxx>
#include <Airports/groundnetwork.hxx>

#include <AIModel/AIManager.hxx>
#include <AIModel/AICarrier.hxx>
#include <AIModel/AIAircraft.hxx>
#include <AIModel/AIFlightPlan.hxx>

#include <Scenery/scenery.hxx>
#include <GUI/MessageBox.hxx>
#include <Viewer/renderer.hxx>

using std::endl;
using std::string;


namespace flightgear
{


    enum InitPosResult {
        ExactPosition,
        VicinityPosition,
        ContinueWaiting,
        Failure
    };

/// to avoid blocking when metar-fetch is enabled, but the network is
/// unresponsive, we need a timeout value. This value is reset on initPosition,
/// and tracked through each call to finalizePosition.
static SGTimeStamp global_finalizeTime;
static bool global_callbackRegistered = false;

void finalizePosition();

namespace { // annonymous namepsace to avoid warnings about inline classes

// Set current tower position lon/lat given an airport id
bool fgSetTowerPosFromAirportID( const string& id)
{
  const FGAirport *a = fgFindAirportID( id);
  if (a) {
    SGGeod tower = a->getTowerLocation();
    fgSetDouble("/sim/tower/longitude-deg",  tower.getLongitudeDeg());
    fgSetDouble("/sim/tower/latitude-deg",  tower.getLatitudeDeg());
    fgSetDouble("/sim/tower/altitude-ft", tower.getElevationFt());
    return true;
  } else {
    return false;
  }
}

class FGTowerLocationListener : public SGPropertyChangeListener {

  void valueChanged(SGPropertyNode* node) override
  {
    string id(node->getStringValue());
    if (fgGetBool("/sim/tower/auto-position",true))
    {
      // enforce using closest airport when auto-positioning is enabled
      const char* closest_airport = fgGetString("/sim/airport/closest-airport-id", "");
      if (closest_airport && (id != closest_airport))
      {
        id = closest_airport;
        node->setStringValue(id);
      }
    }
    fgSetTowerPosFromAirportID(id);
  }
};

class FGClosestTowerLocationListener : public SGPropertyChangeListener
{
  void valueChanged(SGPropertyNode* )
  {
    // closest airport has changed
    if (fgGetBool("/sim/tower/auto-position",true))
    {
      // update tower position
      const char* id = fgGetString("/sim/airport/closest-airport-id", "");
      if (id && *id!=0)
        fgSetString("/sim/tower/airport-id", id);
    }
  }
};

} // of anonymous namespace


void initTowerLocationListener() {

  SGPropertyChangeListener* tll = new FGTowerLocationListener();
  globals->addListenerToCleanup(tll);
  fgGetNode("/sim/tower/airport-id",  true)
  ->addChangeListener( tll, true );

  FGClosestTowerLocationListener* ntcl = new FGClosestTowerLocationListener();
  globals->addListenerToCleanup(ntcl);
  fgGetNode("/sim/airport/closest-airport-id", true)
  ->addChangeListener(ntcl , true );
  fgGetNode("/sim/tower/auto-position", true)
  ->addChangeListener(ntcl, true );
}

static void setInitialPosition(const SGGeod& aPos, double aHeadingDeg)
{
    // presets
    fgSetDouble("/sim/presets/longitude-deg", aPos.getLongitudeDeg() );
    fgSetDouble("/sim/presets/latitude-deg", aPos.getLatitudeDeg() );
    fgSetDouble("/sim/presets/heading-deg", aHeadingDeg );

    // other code depends on the actual values being set ...
    fgSetDouble("/position/longitude-deg",  aPos.getLongitudeDeg() );
    fgSetDouble("/position/latitude-deg",  aPos.getLatitudeDeg() );
    fgSetDouble("/orientation/heading-deg", aHeadingDeg );
}

static void fgApplyStartOffset(const SGGeod& aStartPos, double aHeading, double aTargetHeading = HUGE_VAL)
{
  SGGeod startPos(aStartPos);
  if (aTargetHeading == HUGE_VAL) {
    aTargetHeading = aHeading;
  }

  if ( fabs( fgGetDouble("/sim/presets/offset-distance-nm") ) > SG_EPSILON ) {
    double offsetDistance = fgGetDouble("/sim/presets/offset-distance-nm");
    offsetDistance *= SG_NM_TO_METER;
    double offsetAzimuth = aHeading;
    if ( fabs(fgGetDouble("/sim/presets/offset-azimuth-deg")) > SG_EPSILON ) {
      offsetAzimuth = fgGetDouble("/sim/presets/offset-azimuth-deg");
      aHeading = aTargetHeading;
    }

    startPos = SGGeodesy::direct(startPos, offsetAzimuth + 180, offsetDistance);
  }

    setInitialPosition(startPos, aHeading);
}

std::tuple<SGGeod, double> runwayStartPos(FGRunwayRef runway)
{
    fgSetString("/sim/atc/runway", runway->ident().c_str());
    double offsetNm = fgGetDouble("/sim/presets/offset-distance-nm");
    double startOffset = fgGetDouble("/sim/airport/runways/start-offset-m", 5.0);
    SGGeod pos = runway->pointOnCenterline(startOffset);

    const bool overrideHoldShort = fgGetBool("/sim/presets/mp-hold-short-override", false);

    if (!overrideHoldShort && FGIO::isMultiplayerRequested() && (fabs(offsetNm) <0.1)) {
        SG_LOG( SG_GENERAL, SG_MANDATORY_INFO, "Requested to start on " << runway->airport()->ident() << "/" <<
               runway->ident() << ", MP is enabled so computing hold short position to avoid runway incursion");

        FGGroundNetwork* groundNet = runway->airport()->groundNetwork();

        if (groundNet) {
            // add a margin, try to keep the entire aeroplane comfortable off the
            // runway.
            double margin = startOffset + (runway->widthM() * 1.5);
            FGTaxiNodeRef taxiNode = groundNet->findNearestNodeOffRunway(pos, runway, margin);
            if (taxiNode) {
                // set this so multiplayer.nas can inform the user
                fgSetBool("/sim/presets/avoided-mp-runway", true);
                return std::make_tuple(taxiNode->geod(), SGGeodesy::courseDeg(taxiNode->geod(), pos));
            }
            else {
                // if we couldn't find a suitable taxi-node, give up. Guessing a position
                // causes too much pain (starting in the water or similar bad things)
                SG_LOG( SG_GENERAL, SG_ALERT, "Unable to position off runway because groundnet has no taxi node.");
            }
        }
        else {
            SG_LOG( SG_GENERAL, SG_ALERT, "Unable to position off runway because no groundnet.");
        }
    }

    return std::make_tuple(pos, runway->headingDeg());
}

// Set current_options lon/lat given an airport id and heading (degrees)
static bool setPosFromAirportIDandHdg( const string& id, double tgt_hdg ) {
  if ( id.empty() )
    return false;

  // set initial position from runway and heading
  SG_LOG( SG_GENERAL, SG_INFO,
         "Attempting to set starting position from airport code "
         << id << " heading " << tgt_hdg );

  const FGAirport* apt = fgFindAirportID(id);
  if (!apt) return false;

  SGGeod startPos;
  double heading = tgt_hdg;
  if (apt->type() == FGPositioned::HELIPORT) {
    if (apt->numHelipads() > 0) {
      startPos = apt->getHelipadByIndex(0)->geod();
    } else {
      startPos = apt->geod();
    }
  } else {
    FGRunway* r = apt->findBestRunwayForHeading(tgt_hdg);
      std::tie(startPos, heading) = runwayStartPos(r);
  }

    fgApplyStartOffset(startPos, heading);
    return true;
}

static bool airportParkingSetVicinity(const string& id)
{
    const FGAirport* apt = fgFindAirportID(id);
    if (!apt) {
      SG_LOG( SG_GENERAL, SG_ALERT, "Failed to find airport " << id );
      return false;
    }
    
    setInitialPosition(apt->geod(), 0.0);
    return true;
}

// Set current_options lon/lat given an airport id and parkig position name
static bool finalizePositionForParkpos( const string& id, const string& parkpos )
{
    auto aiManager = globals->get_subsystem<FGAIManager>();
    if (!aiManager || !aiManager->getUserAircraft()) {
        SG_LOG( SG_GENERAL, SG_ALERT, "finalizePositionForParkpos: >> failed to find AI manager / user aircraft");
        return false;
    }
    
    auto userAIFP = aiManager->getUserAircraft()->GetFlightPlan();
    if (!userAIFP) {
        SG_LOG( SG_GENERAL, SG_ALERT, "finalizePositionForParkpos: >> failed to find user aircraft AI flight-plan");
        return false;
    }
    
    auto pkr = userAIFP->getParkingGate();
    if (!pkr) {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "Failed to find a parking at airport " << id << ":" << parkpos);
        return false;
    }

  fgSetString("/sim/presets/parkpos", pkr->getName());
  fgApplyStartOffset(pkr->geod(), pkr->getHeading());
  return true;
}


// Set current_options lon/lat given an airport id and runway number
static bool fgSetPosFromAirportIDandRwy( const string& id, const string& rwy, bool rwy_req ) {
  if ( id.empty() )
    return false;

  // set initial position from airport and runway number
  SG_LOG( SG_GENERAL, SG_INFO,
         "Attempting to set starting position for "
         << id << ":" << rwy );

  const FGAirport* apt = fgFindAirportID(id);
  if (!apt) {
    SG_LOG( SG_GENERAL, SG_ALERT, "Failed to find airport:" << id);
    return false;
  }

  if (apt->hasRunwayWithIdent(rwy)) {
      FGRunway* r(apt->getRunwayByIdent(rwy));
      SGGeod startPos;
      double heading;
      std::tie(startPos, heading) = runwayStartPos(r);
      fgApplyStartOffset(startPos, heading);
      return true;
  } else if (apt->hasHelipadWithIdent(rwy)) {
      FGHelipad* h(apt->getHelipadByIdent(rwy));
      fgApplyStartOffset(h->geod(), h->headingDeg());
      return true;
  }

  if (rwy_req) {
      flightgear::modalMessageBox("Runway not available", "Runway/helipad "
                                   + rwy + " not found at airport " + apt->getId()
                                   + " - " + apt->getName() );
  } else {
    SG_LOG( SG_GENERAL, SG_INFO,
           "Failed to find runway/helipad " << rwy <<
           " at airport " << id << ". Using default runway." );
  }
  return false;
}


static void fgSetDistOrAltFromGlideSlope()
{
  string apt_id = fgGetString("/sim/presets/airport-id");
  double gs = SGMiscd::deg2rad(fgGetDouble("/sim/presets/glideslope-deg"));
  double od = fgGetDouble("/sim/presets/offset-distance-nm");
  double alt = fgGetDouble("/sim/presets/altitude-ft");

  double apt_elev = 0.0;
  if ( ! apt_id.empty() ) {
    apt_elev = fgGetAirportElev( apt_id );
    if ( apt_elev < -9990.0 ) {
      apt_elev = 0.0;
    }
  } else {
    apt_elev = 0.0;
  }

  if( fabs(gs) > 0.01 && fabs(od) > 0.1 && alt < -9990 ) {
    // set altitude from glideslope and offset-distance
    od *= SG_NM_TO_METER * SG_METER_TO_FEET;
    alt = fabs(od*tan(gs)) + apt_elev;
    fgSetDouble("/sim/presets/altitude-ft", alt);
    fgSetBool("/sim/presets/onground", false);
    SG_LOG( SG_GENERAL, SG_INFO, "Calculated altitude as: "
           << alt  << " ft" );
  } else if( fabs(gs) > 0.01 && alt > 0 && fabs(od) < 0.1) {
    // set offset-distance from glideslope and altitude
    od  = (alt - apt_elev) / tan(gs);
    od *= -1*SG_FEET_TO_METER * SG_METER_TO_NM;
    fgSetDouble("/sim/presets/offset-distance-nm", od);
    fgSetBool("/sim/presets/onground", false);
    SG_LOG( SG_GENERAL, SG_INFO, "Calculated offset distance as: "
           << od  << " nm" );
  } else if( fabs(gs) > 0.01 ) {
    SG_LOG( SG_GENERAL, SG_ALERT,
           "Glideslope given but not altitude or offset-distance." );
    SG_LOG( SG_GENERAL, SG_ALERT, "Resetting glideslope to zero" );
    fgSetDouble("/sim/presets/glideslope-deg", 0);
    fgSetBool("/sim/presets/onground", true);
  }
}


// Set current_options lon/lat given a Nav ID or GUID
static bool fgSetPosFromNAV( const string& id,
                             const double& freq,
                             FGPositioned::Type type,
                             PositionedID guid)
{
    FGNavRecordRef nav;
    if (guid != 0) {
        nav = FGPositioned::loadById<FGNavRecord>(guid);
        if (!nav)
            return false;
    } else {
        FGNavList::TypeFilter filter(type);
        const nav_list_type navlist = FGNavList::findByIdentAndFreq( id.c_str(), freq, &filter );

        if (navlist.empty()) {
          SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate NAV = "
                 << id << ":" << freq );
          return false;
        }

        if( navlist.size() > 1 ) {
          std::ostringstream buf;
          buf << "Ambigous NAV-ID: '" << id << "'. Specify id and frequency. Available stations:" << endl;
          for( const auto& nav : navlist ) {
            // NDB stored in kHz, VOR stored in MHz * 100 :-P
            double factor = nav->type() == FGPositioned::NDB ? 1.0 : 1/100.0;
            string unit = nav->type() == FGPositioned::NDB ? "kHz" : "MHz";
            buf << nav->ident() << " "
                << std::setprecision(5) << static_cast<double>(nav->get_freq() * factor) << " "
                << nav->get_lat() << "/" << nav->get_lon()
                << endl;
          }

          SG_LOG( SG_GENERAL, SG_ALERT, buf.str() );
          return false;
        }

        // nav list must be of length 1
        nav = navlist[0];
    }

    fgApplyStartOffset(nav->geod(), fgGetDouble("/sim/presets/heading-deg"));
    return true;
}

static InitPosResult setInitialPosFromCarrier( const string& carrier )
{
  const auto initialPos = FGAICarrier::initialPositionForCarrier(carrier);
  if (initialPos.first) {
    // set these so scenery system has a vicinity to work with, and
    // so our PagedLOD is loaded
    fgSetDouble("/sim/presets/longitude-deg",  initialPos.second.getLongitudeDeg());
    fgSetDouble("/sim/presets/latitude-deg",  initialPos.second.getLatitudeDeg());
    SG_LOG( SG_GENERAL, SG_DEBUG, "Initial carrier pos = " << initialPos.second );
    return VicinityPosition;
  }

  SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate aircraft carrier = " << carrier );
  return Failure;
}

static InitPosResult checkCarrierSceneryLoaded(const SGSharedPtr<FGAICarrier> carrierRef)
{
    SGVec3d cartPos = carrierRef->getCartPos();
    auto framestamp = globals->get_renderer()->getViewer()->getFrameStamp();
    simgear::CheckSceneryVisitor csnv(globals->get_scenery()->getPager(),
                                      toOsg(cartPos),
                                      100.0 /* range in metres */,
                                      framestamp);

    // currently the PagedLODs will not be loaded by the DatabasePager
    // while the splashscreen is there, so CheckSceneryVisitor force-loads
    // missing objects in the main thread
    carrierRef->getSceneBranch()->accept(csnv);
    if (!csnv.isLoaded()) {
      return ContinueWaiting;
    }

    // and then wait for the load to actually be synced to the main thread
    if (carrierRef->getSceneBranch()->getNumChildren() < 1) {
      return ContinueWaiting;
    }

    return VicinityPosition;
}

// Set current_options lon/lat given an aircraft carrier id
static InitPosResult setFinalPosFromCarrier( const string& carrier, const string& posid )
{

  SGSharedPtr<FGAICarrier> carrierRef = FGAICarrier::findCarrierByNameOrPennant(carrier);
  if (!carrierRef) {
    SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate aircraft carrier = "
           << carrier );
    return Failure;
  }

  auto res = checkCarrierSceneryLoaded(carrierRef);
  if (res != VicinityPosition) {
      return res; // either failrue or keep waiting for scenery load
  }

  SGGeod geodPos;
  double heading;
  SGVec3d uvw;
  if (carrierRef->getParkPosition(posid, geodPos, heading, uvw)) {

    ////////
    double lon = geodPos.getLongitudeDeg();
    double lat = geodPos.getLatitudeDeg();
    double alt = geodPos.getElevationFt() + 2.0;

    SG_LOG( SG_GENERAL, SG_INFO, "Attempting to set starting position for "
           << carrier << " at lat = " << lat << ", lon = " << lon
           << ", alt = " << alt << ", heading = " << heading);

    fgSetDouble("/sim/presets/longitude-deg",  lon);
    fgSetDouble("/sim/presets/latitude-deg",  lat);
    fgSetDouble("/sim/presets/altitude-ft", alt);
    fgSetDouble("/sim/presets/heading-deg", heading);
    fgSetDouble("/position/longitude-deg",  lon);
    fgSetDouble("/position/latitude-deg",  lat);
    fgSetDouble("/position/altitude-ft", alt);
    fgSetDouble("/orientation/heading-deg", heading);

    fgSetString("/sim/presets/speed-set", "UVW");
    fgSetDouble("/velocities/uBody-fps", uvw(0));
    fgSetDouble("/velocities/vBody-fps", uvw(1));
    fgSetDouble("/velocities/wBody-fps", uvw(2));
    fgSetDouble("/sim/presets/uBody-fps", uvw(0));
    fgSetDouble("/sim/presets/vBody-fps", uvw(1));
    fgSetDouble("/sim/presets/wBody-fps", uvw(2));

    fgSetBool("/sim/presets/onground", true);

    /////////
    return ExactPosition;
  }
  SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate aircraft carrier = " << carrier );
  return Failure;
}

static InitPosResult setFinalPosFromCarrierFLOLS(const string& carrier, bool abeam)
{
    SGSharedPtr<FGAICarrier> carrierRef = FGAICarrier::findCarrierByNameOrPennant(carrier);
    if (!carrierRef) {
      SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate aircraft carrier = "
             << carrier );
      return Failure;
    }

    auto res = checkCarrierSceneryLoaded(carrierRef);
    if (res != VicinityPosition) {
        SG_LOG(SG_GENERAL, SG_DEBUG, "carrier scenery not yet loaded");
        return res; // either failure or keep waiting for scenery load
    }

    SGGeod flolsPosition;
    double headingToFLOLS;
    if (!carrierRef->getFLOLSPositionHeading(flolsPosition, headingToFLOLS)) {
        SG_LOG( SG_GENERAL, SG_ALERT, "Unable to compiute FLOLS position for carrier = "
               << carrier );
        return Failure;
    }

    const auto flolsElevationFt = flolsPosition.getElevationFt();
    double gs = SGMiscd::deg2rad(carrierRef->getFLOLFSGlidepathAngleDeg());
    const double od = fgGetDouble("/sim/presets/offset-distance-nm");

    // start position
    SGGeod startPos;
    if (abeam) {
      // If we're starting from the abeam position, we are opposite the FLOLS, downwind on a left hand circuit
      startPos = SGGeodesy::direct(flolsPosition, headingToFLOLS - 90, od * SG_NM_TO_METER);
    } else {
      startPos = SGGeodesy::direct(flolsPosition, headingToFLOLS + 180, od * SG_NM_TO_METER);
    }

    double alt = fgGetDouble("/sim/presets/altitude-ft");

    if (alt < 0.0f) {
      // No altitude set, so base on glideslope
      const double offsetFt = od * SG_NM_TO_METER * SG_METER_TO_FEET;
      startPos.setElevationFt(fabs(offsetFt*tan(gs)) + flolsElevationFt);
    } else {
      startPos.setElevationFt(alt);
    }

    fgSetDouble("/sim/presets/longitude-deg",  startPos.getLongitudeDeg());
    fgSetDouble("/sim/presets/latitude-deg",  startPos.getLatitudeDeg());
    fgSetDouble("/sim/presets/altitude-ft", startPos.getElevationFt());
    fgSetDouble("/sim/presets/heading-deg", abeam ? (headingToFLOLS - 180) : headingToFLOLS);
    fgSetDouble("/position/longitude-deg",  startPos.getLongitudeDeg());
    fgSetDouble("/position/latitude-deg",  startPos.getLatitudeDeg());
    fgSetDouble("/position/altitude-ft", startPos.getElevationFt());
    fgSetDouble("/orientation/heading-deg", headingToFLOLS);
    fgSetBool("/sim/presets/onground", false);

    return ExactPosition;
}

// Set current_options lon/lat given a fix ident and GUID
static bool fgSetPosFromFix( const string& id, PositionedID guid )
{
    FGPositionedRef fix;
    if (guid != 0) {
        fix = FGPositioned::loadById<FGPositioned>(guid);
    } else {
        FGPositioned::TypeFilter fixFilter(FGPositioned::FIX);
        fix = FGPositioned::findFirstWithIdent(id, &fixFilter);
    }

  if (!fix) {
    SG_LOG( SG_GENERAL, SG_ALERT, "Failed to locate fix = " << id );
    return false;
  }

  fgApplyStartOffset(fix->geod(), fgGetDouble("/sim/presets/heading-deg"));
  return true;
}

// Set the initial position based on presets (or defaults)
bool initPosition()
{
  global_finalizeTime = SGTimeStamp(); // reset to invalid
  if (!global_callbackRegistered) {
    globals->get_event_mgr()->addTask("finalizePosition", &finalizePosition, 0.1);
    global_callbackRegistered = true;
  }

  double gs = SGMiscd::deg2rad(fgGetDouble("/sim/presets/glideslope-deg"));
  double od = fgGetDouble("/sim/presets/offset-distance-nm");
  double alt = fgGetDouble("/sim/presets/altitude-ft");

  bool set_pos = false;

  // clear this value, so we don't preserve an old value and confuse
  // the ATC manager. We will set it again if it's valid
  fgSetString("/sim/atc/runway", "");

  // If glideslope is specified, then calculate offset-distance or
  // altitude relative to glide slope if either of those was not
  // specified.
  if ( fabs( gs ) > 0.01 ) {
    fgSetDistOrAltFromGlideSlope();
  }


  // If we have an explicit, in-range lon/lat, don't change it, just use it.
  // If not, check for an airport-id and use that.
  // If not, default to the middle of the KSFO field.
  // The default values for lon/lat are deliberately out of range
  // so that the airport-id can take effect; valid lon/lat will
  // override airport-id, however.
  double lon_deg = fgGetDouble("/sim/presets/longitude-deg");
  double lat_deg = fgGetDouble("/sim/presets/latitude-deg");
  if ( lon_deg >= -180.0 && lon_deg <= 180.0
      && lat_deg >= -90.0 && lat_deg <= 90.0 )
  {
    set_pos = true;
  }

  string apt = fgGetString("/sim/presets/airport-id");
  const bool apt_req = fgGetBool("/sim/presets/airport-requested");
  string rwy_no = fgGetString("/sim/presets/runway");
  bool rwy_req = fgGetBool("/sim/presets/runway-requested");
  string vor = fgGetString("/sim/presets/vor-id");
  double vor_freq = fgGetDouble("/sim/presets/vor-freq");
  string ndb = fgGetString("/sim/presets/ndb-id");
  double ndb_freq = fgGetDouble("/sim/presets/ndb-freq");
  string carrier = fgGetString("/sim/presets/carrier");
  string parkpos = fgGetString("/sim/presets/parkpos");
  string fix = fgGetString("/sim/presets/fix");

  // the launcher sets this to precisely identify a navaid
  PositionedID navaidId = fgGetInt("/sim/presets/navaid-id");

  SGPropertyNode *hdg_preset = fgGetNode("/sim/presets/heading-deg", true);
  double hdg = hdg_preset->getDoubleValue();
  double original_hdg = hdg_preset->getDoubleValue();

  // save some start parameters, so that we can later say what the
  // user really requested. TODO generalize that and move it to options.cxx
  static bool start_options_saved = false;
  if (!start_options_saved) {
    start_options_saved = true;
    SGPropertyNode *opt = fgGetNode("/sim/startup/options", true);

    opt->setDoubleValue("latitude-deg", lat_deg);
    opt->setDoubleValue("longitude-deg", lon_deg);
    opt->setDoubleValue("heading-deg", hdg);
    opt->setStringValue("airport", apt.c_str());
    opt->setStringValue("runway", rwy_no.c_str());
  }

  if (hdg > 9990.0) {
    hdg = fgGetDouble("/environment/config/boundary/entry/wind-from-heading-deg", 270);
  }

  if ( !set_pos && !carrier.empty() ) {
    // an aircraft carrier is requested
    const auto result = setInitialPosFromCarrier( carrier );
    if (result != Failure) {
      // we at least found the carrier
      set_pos = true;
    }
  }

  if (apt_req && !rwy_req) {
    // ensure that if the users asks for a specific airport, but not a runway,
    // presumably because they want automatic selection, we do not look
    // for the default runway (from $FGDATA/location-preset.xml) which is
    // likely missing.
    rwy_no.clear();
  }

  if ( !set_pos && !apt.empty() && !parkpos.empty() ) {
    // An airport + parking position is requested
    // since this depends on parking, which is part of dynamics, and hence
    // also depends on ATC (the ground controller), we need to defer this
    // until position finalisation
    // the rest of the work happens in finalizePosFromParkpos
    if ( airportParkingSetVicinity( apt ) ) {
      set_pos = true;
    }
  }

  if ( !set_pos && !apt.empty() && !rwy_no.empty() ) {
    // An airport + runway is requested
    if ( fgSetPosFromAirportIDandRwy( apt, rwy_no, rwy_req ) ) {
      set_pos = true;
    }
  }

  if ( !set_pos && !apt.empty() ) {
    // An airport is requested (find runway closest to hdg)
    if ( setPosFromAirportIDandHdg( apt, hdg ) ) {
      set_pos = true;
    }
  }

  // if an airport ID was requested, set closest-airport-id
  // and tower based upon it.
  if (!apt.empty() && set_pos) {
      // set tower position
      fgSetString("/sim/airport/closest-airport-id", apt.c_str());
      fgSetString("/sim/tower/airport-id", apt.c_str());
  }

  if (original_hdg < 9990.0) {
    // The user-set heading may be overridden by the setPosFromAirportID above.
    SG_LOG(SG_GENERAL, SG_ALERT, "Setting heading preset to  : " << original_hdg);
    hdg_preset->setDoubleValue(original_hdg);
  }

  if ( !set_pos && !vor.empty() ) {
    // a VOR is requested
    if ( fgSetPosFromNAV( vor, vor_freq, FGPositioned::VOR, navaidId ) ) {
      set_pos = true;
    }
  }

  if ( !set_pos && !ndb.empty() ) {
    // an NDB is requested
    if ( fgSetPosFromNAV( ndb, ndb_freq, FGPositioned::NDB, navaidId ) ) {
      set_pos = true;
    }
  }

  if ( !set_pos && !fix.empty() ) {
    // a Fix is requested
    if ( fgSetPosFromFix( fix, navaidId ) ) {
      set_pos = true;
    }
  }

  if ( !set_pos ) {
    const std::string defaultAirportId = fgGetString("/sim/presets/airport-id");
    const FGAirport* airport = fgFindAirportID(defaultAirportId);
    if( airport ) {
      const SGGeod & airportGeod = airport->geod();
      fgSetDouble("/sim/presets/longitude-deg", airportGeod.getLongitudeDeg());
      fgSetDouble("/sim/presets/latitude-deg", airportGeod.getLatitudeDeg());
    } else {
      // So, the default airport is unknown? We are in serious trouble.
      // Let's hope KSFO still exists somehow
      fgSetDouble("/sim/presets/longitude-deg", -122.374843);
      fgSetDouble("/sim/presets/latitude-deg", 37.619002);
      SG_LOG(SG_GENERAL, SG_ALERT, "Sorry, the default airport ('" << defaultAirportId
             << "') seems to be unknown.");
    }
  }

  fgSetDouble( "/position/longitude-deg",
              fgGetDouble("/sim/presets/longitude-deg") );
  fgSetDouble( "/position/latitude-deg",
              fgGetDouble("/sim/presets/latitude-deg") );
  fgSetDouble( "/orientation/heading-deg", hdg_preset->getDoubleValue());

  // determine if this should be an on-ground or in-air start
  if ((fabs(gs) > 0.01 || fabs(od) > 0.1 || alt > 0.1) && carrier.empty()) {
    fgSetBool("/sim/presets/onground", false);
  } else {
    fgSetBool("/sim/presets/onground", true);
  }

  fgSetBool("/sim/position-finalized", false);

  // Initialize the longitude, latitude and altitude to the initial position
  fgSetDouble("/position/altitude-ft", fgGetDouble("/sim/presets/altitude-ft"));
  fgSetDouble("/position/longitude-deg", fgGetDouble("/sim/presets/longitude-deg"));
  fgSetDouble("/position/latitude-deg", fgGetDouble("/sim/presets/latitude-deg"));

  return true;
}

bool finalizeMetar()
{
  double hdg = fgGetDouble( "/environment/metar/base-wind-dir-deg", 9999.0 );
  string apt = fgGetString("/sim/presets/airport-id");
  double strthdg = fgGetDouble( "/sim/startup/options/heading-deg", 9999.0 );
  string parkpos = fgGetString( "/sim/presets/parkpos" );
  bool onground = fgGetBool( "/sim/presets/onground", false );
  const bool rwy_req = fgGetBool("/sim/presets/runway-requested");
  // this logic is taken from former startup.nas
  bool needMetar = (hdg < 360.0) &&
    !apt.empty() &&
    (strthdg > 360.0) &&
    !rwy_req &&
    onground &&
    parkpos.empty();

  if (needMetar) {
    // timeout so we don't spin forever if the network is down
    if (global_finalizeTime.elapsedMSec() > fgGetInt("/sim/startup/metar-fetch-timeout-msec", 6000)) {
      SG_LOG(SG_GENERAL, SG_WARN, "finalizePosition: timed out waiting for METAR fetch");
      return true;
    }

    if (fgGetBool( "/environment/metar/failure" )) {
      SG_LOG(SG_ENVIRONMENT, SG_INFO, "metar download failed, not waiting");
      return true;
    }

    if (!fgGetBool( "/environment/metar/valid" )) {
      return false;
    }

    SG_LOG(SG_ENVIRONMENT, SG_INFO,
           "Using METAR for runway selection: '" << fgGetString("/environment/metar/data") << "'" );
    setPosFromAirportIDandHdg( apt, hdg );
    // fall through to return true
  } // of need-metar case

  return true;
}

void finalizePosition()
{
  // first call to finalize after an initPosition call
  if (global_finalizeTime.get_usec() == 0) {
    global_finalizeTime = SGTimeStamp::now();
  }

  bool done = true;

    /* Scenarios require Nasal, so FGAIManager loads the scenarios,
     * including its models such as a/c carriers, in its 'postinit',
     * which is the very last thing we do.
     * flightgear::initPosition is called very early in main.cxx/fgIdleFunction,
     * one of the first things we do, long before scenarios/carriers are
     * loaded. => When requested "initial preset position" relates to a
     * carrier, recalculate the 'initial' position here
     */
    const std::string carrier = fgGetString("/sim/presets/carrier");
    const std::string carrierpos = fgGetString("/sim/presets/carrier-position");
    const std::string parkpos = fgGetString("/sim/presets/parkpos");
    const std::string runway = fgGetString("/sim/presets/runway");
    const std::string apt = fgGetString("/sim/presets/airport-id");

    if (!carrier.empty())
    {
        /* /sim/presets/carrier-position can take a number of different values
           - name of a parking/cataputt
           - FLOLS - to position on final approach
           - abeam - on an abeam position from the FLOLS, heading downwind
           - <empty> indicating a start position of a catapult
        */

        // Convert to lower case to simplify comparison
        std::string cpos = simgear::strutils::lowercase(carrierpos);

        const bool    inair = (cpos == "flols") || (cpos == "abeam");
        const bool    abeam = (cpos == "abeam");
        InitPosResult carrierResult;
        if (inair) {
            carrierResult = setFinalPosFromCarrierFLOLS(carrier, abeam);
        } else {
            // We don't simply use cpos as it is lower case, and the
            // search against parking/catapult positions is case-sensitive.
            carrierResult = setFinalPosFromCarrier(carrier, carrierpos);
        }
        if (carrierResult == ExactPosition) {
            done = true;
        } else if (carrierResult == Failure) {
            SG_LOG(SG_GENERAL, SG_ALERT, "secondary carrier init failed");
            done = true;
        } else {
            done = false;
            // 60 second timeout on waiting for the carrier to load
            if (global_finalizeTime.elapsedMSec() > 60000) {
                SG_LOG(SG_GENERAL, SG_ALERT, "Timeout waiting for carrier scenery to load, will start on the water.");
                done = true;
            }
        }

    } else if (!apt.empty() && !parkpos.empty()) {
        // parking position depends on ATC / dynamics code to assign spaces,
        // so we wait until this point to initialise
        bool ok = finalizePositionForParkpos(apt, parkpos);
        if (!ok) {
            SG_LOG(SG_GENERAL, SG_WARN, "finalizePositionForParkPos failed, reverting to best runway");
            
            // clear this so finalizeMetar works as expected
            fgSetString("/sim/presets/parkpos", "");
            finalizeMetar();
        }
    } else {
        done = finalizeMetar();
    }

    fgSetBool("/sim/position-finalized", done);
    if (done) {
        globals->get_event_mgr()->removeTask("finalizePosition");
        global_callbackRegistered = false;
    }
}

} // of namespace flightgear
