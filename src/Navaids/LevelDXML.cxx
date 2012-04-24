#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "LevelDXML.hxx"

#include <boost/algorithm/string.hpp>

#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <Navaids/waypoint.hxx>
#include <Airports/simple.hxx>

using std::string;
using std::vector;

namespace flightgear
{

NavdataVisitor::NavdataVisitor(FGAirport* aApt, const SGPath& aPath):
  _airport(aApt),
  _path(aPath),
  _sid(NULL),
  _star(NULL),
  _approach(NULL),
  _transition(NULL),
  _procedure(NULL)
{
}

void NavdataVisitor::startXML()
{
}

void NavdataVisitor::endXML()
{
}

void NavdataVisitor::startElement(const char* name, const XMLAttributes &atts)
{
  _text.clear();
  string tag(name);
  if (tag == "Airport") {
    string icao(atts.getValue("ICAOcode"));
    if (_airport->ident() != icao) {
      throw sg_format_exception("Airport and ICAO mismatch", icao, _path.str());
    }
  } else if (tag == "Sid") {
    string ident(atts.getValue("Name"));
    _sid = new SID(ident, _airport);
    _procedure = _sid;
    _waypoints.clear();
    processRunways(_sid, atts);
  } else if (tag == "Star") {
    string ident(atts.getValue("Name"));
    _star = new STAR(ident, _airport);
    _procedure = _star;
    _waypoints.clear();
    processRunways(_star, atts);
  } else if ((tag == "Sid_Waypoint") ||
      (tag == "App_Waypoint") ||
      (tag == "Star_Waypoint") ||
      (tag == "AppTr_Waypoint") ||
      (tag == "SidTr_Waypoint") ||
      (tag == "RwyTr_Waypoint"))
  {
    // reset waypoint data
    _speed = 0.0;
    _altRestrict = RESTRICT_NONE;
    _altitude = 0.0;
  } else if (tag == "Approach") {
    _ident = atts.getValue("Name");
    _waypoints.clear();
    ProcedureType ty = PROCEDURE_APPROACH_RNAV;
    _approach = new Approach(_ident, ty);
    _procedure = _approach;
  } else if ((tag == "Sid_Transition") || 
             (tag == "App_Transition") ||
             (tag == "Star_Transition")) {
    _transIdent = atts.getValue("Name");
    _transition = new Transition(_transIdent, PROCEDURE_TRANSITION, _procedure);
    _transWaypts.clear();
  } else if (tag == "RunwayTransition") {
    _transIdent = atts.getValue("Runway");
    _transition = new Transition(_transIdent, PROCEDURE_RUNWAY_TRANSITION, _procedure);
    _transWaypts.clear();
  } else {
    
  }
}

void NavdataVisitor::processRunways(ArrivalDeparture* aProc, const XMLAttributes &atts)
{
  string v("All");
  if (atts.hasAttribute("Runways")) {
    v = atts.getValue("Runways");
  }
  
  if (v == "All") {
    for (unsigned int r=0; r<_airport->numRunways(); ++r) {
      aProc->addRunway(_airport->getRunwayByIndex(r));
    }
    return;
  }
  
  vector<string> rwys;
  boost::split(rwys, v, boost::is_any_of(" ,"));
  for (unsigned int r=0; r<rwys.size(); ++r) {
    FGRunway* rwy = _airport->getRunwayByIdent(rwys[r]);
    aProc->addRunway(rwy);
  }
}

void NavdataVisitor::endElement(const char* name)
{
  string tag(name);
  if ((tag == "Sid_Waypoint") ||
      (tag == "App_Waypoint") ||
      (tag == "Star_Waypoint"))
  {
    _waypoints.push_back(buildWaypoint(_procedure));
  } else if ((tag == "AppTr_Waypoint") || 
             (tag == "SidTr_Waypoint") ||
             (tag == "RwyTr_Waypoint") ||
             (tag == "StarTr_Waypoint")) 
  {
    _transWaypts.push_back(buildWaypoint(_transition));
  } else if (tag == "Sid_Transition") {
    assert(_sid);
    // SID waypoints are stored backwards, to share code with STARs
    std::reverse(_transWaypts.begin(), _transWaypts.end());
    _transition->setPrimary(_transWaypts);
    _sid->addTransition(_transition);
  } else if (tag == "Star_Transition") {
    assert(_star);
    _transition->setPrimary(_transWaypts);
    _star->addTransition(_transition);
  } else if (tag == "App_Transition") {
    assert(_approach);
    _transition->setPrimary(_transWaypts);
    _approach->addTransition(_transition);
  } else if (tag == "RunwayTransition") {
    ArrivalDeparture* ad;
    if (_sid) {
      // SID waypoints are stored backwards, to share code with STARs
      std::reverse(_transWaypts.begin(), _transWaypts.end());
      ad = _sid;
    } else {
      ad = _star;
    }

    _transition->setPrimary(_transWaypts);
    FGRunwayRef rwy = _airport->getRunwayByIdent(_transIdent);
    ad->addRunwayTransition(rwy, _transition);
  } else if (tag == "Approach") {
    finishApproach();
  } else if (tag == "Sid") {
    finishSid();
  } else if (tag == "Star") {
    finishStar();  
  } else if (tag == "Longitude") {
    _longitude = atof(_text.c_str());
  } else if (tag == "Latitude") {
    _latitude = atof(_text.c_str());
  } else if (tag == "Name") {
    _wayptName = _text;
  } else if (tag == "Type") {
    _wayptType = _text;
  } else if (tag == "Speed") {
    _speed = atoi(_text.c_str());
  } else if (tag == "Altitude") {
    _altitude = atof(_text.c_str());
  } else if (tag == "AltitudeRestriction") {
    if (_text == "at") {
      _altRestrict = RESTRICT_AT;
    } else if (_text == "above") {
      _altRestrict = RESTRICT_ABOVE;
    } else if (_text == "below") {
      _altRestrict = RESTRICT_BELOW;
    } else {
      throw sg_format_exception("Unrecognized altitude restriction", _text);
    }
  } else if (tag == "Hld_Rad_or_Inbd") {
    if (_text == "Inbd") {
      _holdRadial = -1.0;
    }
  } else if (tag == "Hld_Time_or_Dist") {
    _holdDistance = (_text == "Dist");
  } else if (tag == "Hld_Rad_value") {
    _holdRadial = atof(_text.c_str());
  } else if (tag == "Hld_Turn") {
    _holdRighthanded = (_text == "Right");
  } else if (tag == "Hld_td_value") {
    _holdTD = atof(_text.c_str());
  } else if (tag == "Hdg_Crs_value") {
    _course = atof(_text.c_str());
  } else if (tag == "DMEtoIntercept") {
    _dmeDistance = atof(_text.c_str());
  } else if (tag == "RadialtoIntercept") {
    _radial = atof(_text.c_str());
  } else {
    
  }
}

Waypt* NavdataVisitor::buildWaypoint(RouteBase* owner)
{
  Waypt* wp = NULL;
  if (_wayptType == "Normal") {
    // new LatLonWaypoint
    SGGeod pos(SGGeod::fromDeg(_longitude, _latitude));
    wp = new BasicWaypt(pos, _wayptName, owner);
  } else if (_wayptType == "Runway") {
    string ident = _wayptName.substr(2);
    FGRunwayRef rwy = _airport->getRunwayByIdent(ident);
    wp = new RunwayWaypt(rwy, owner);
  } else if (_wayptType == "Hold") {
    SGGeod pos(SGGeod::fromDeg(_longitude, _latitude));
    Hold* h = new Hold(pos, _wayptName, owner);
    wp = h;
    if (_holdRighthanded) {
      h->setRightHanded();
    } else {
      h->setLeftHanded();
    }
    
    if (_holdDistance) {
      h->setHoldDistance(_holdTD);
    } else {
      h->setHoldTime(_holdTD * 60.0);
    }
    
    if (_holdRadial >= 0.0) {
      h->setHoldRadial(_holdRadial);
    }
  } else if (_wayptType == "Vectors") {
    wp = new ATCVectors(owner, _airport);
  } else if ((_wayptType == "Intc") || (_wayptType == "VorRadialIntc")) {
    SGGeod pos(SGGeod::fromDeg(_longitude, _latitude));
    wp = new RadialIntercept(owner, _wayptName, pos, _course, _radial);
  } else if (_wayptType == "DmeIntc") {
    SGGeod pos(SGGeod::fromDeg(_longitude, _latitude));
    wp = new DMEIntercept(owner, _wayptName, pos, _course, _dmeDistance);
  } else if (_wayptType == "ConstHdgtoAlt") {
    wp = new HeadingToAltitude(owner, _wayptName, _course);
  } else if (_wayptType == "PBD") {
    SGGeod pos(SGGeod::fromDeg(_longitude, _latitude)), pos2;
    double az2;
    SGGeodesy::direct(pos, _course, _dmeDistance, pos2, az2);
    wp = new BasicWaypt(pos2, _wayptName, owner);
  } else {
    SG_LOG(SG_GENERAL, SG_ALERT, "implement waypoint type:" << _wayptType);
    throw sg_format_exception("Unrecognized waypt type", _wayptType);
  }
  
  assert(wp);
  if ((_altitude > 0.0) && (_altRestrict != RESTRICT_NONE)) {
    wp->setAltitude(_altitude,_altRestrict);
  }
  
  if (_speed > 0.0) {
    wp->setSpeed(_speed, RESTRICT_AT); // or _BELOW?
  }
  
  return wp;
}

void NavdataVisitor::finishApproach()
{
  WayptVec::iterator it;
  FGRunwayRef rwy;
  
// find the runway node
  for (it = _waypoints.begin(); it != _waypoints.end(); ++it) {
    FGPositionedRef navid = (*it)->source();
    if (!navid) {
      continue;
    }
    
    if (navid->type() == FGPositioned::RUNWAY) {
      rwy = (FGRunway*) navid.get();
      break;
    }
  }
  
  if (!rwy) {
    throw sg_format_exception("Malformed approach, no runway waypt", _ident);
  }
  
  WayptVec primary(_waypoints.begin(), it);
  // erase all points up to and including the runway, to leave only the
  // missed segments
  _waypoints.erase(_waypoints.begin(), ++it);
  
  _approach->setRunway(rwy);
  _approach->setPrimaryAndMissed(primary, _waypoints);
  _airport->addApproach(_approach);
  _approach = NULL;
}

void NavdataVisitor::finishSid()
{
  // reverse order, because that's how we deal with commonality between
  // STARs and SIDs. SID::route undoes  this
  std::reverse(_waypoints.begin(), _waypoints.end());
  _sid->setCommon(_waypoints);
  _airport->addSID(_sid);
  _sid = NULL;
}

void NavdataVisitor::finishStar()
{
  _star->setCommon(_waypoints);
  _airport->addSTAR(_star);
  _star = NULL;
}

void NavdataVisitor::data (const char * s, int len)
{
  _text += string(s, len);
}


void NavdataVisitor::pi (const char * target, const char * data) {
  //cout << "Processing instruction " << target << ' ' << data << endl;
}

void NavdataVisitor::warning (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_WARN, "Warning: " << message << " (" << line << ',' << column << ')');
}

void NavdataVisitor::error (const char * message, int line, int column) {
  SG_LOG(SG_IO, SG_ALERT, "Error: " << message << " (" << line << ',' << column << ')');
}

}
