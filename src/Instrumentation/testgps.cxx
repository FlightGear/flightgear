
#include <fstream>

#include <simgear/misc/sg_path.hxx>
#include <simgear/structure/exception.hxx>

#include <Main/fg_init.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include <Instrumentation/gps.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Environment/environment_mgr.hxx>
#include <Navaids/airways.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/procedure.hxx>

using std::string;
using namespace flightgear;

char *homedir = ::getenv( "HOME" );
char *hostname = ::getenv( "HOSTNAME" );
bool free_hostname = false;

void testSetPosition(const SGGeod& aPos)
{
  fgSetDouble("/position/longitude-deg", aPos.getLongitudeDeg());
  fgSetDouble("/position/latitude-deg", aPos.getLatitudeDeg());
  fgSetDouble("/position/altitude-ft", aPos.getElevationFt());
}

void printScratch(SGPropertyNode* scratch)
{
  if (!scratch->getBoolValue("valid", false)) {
    SG_LOG(SG_GENERAL, SG_ALERT, "Scratch is invalid."); 
    return;
  }

  SG_LOG(SG_GENERAL, SG_ALERT, "Scratch:" <<
    scratch->getStringValue("ident") << "/" << scratch->getStringValue("name"));
  
  SG_LOG(SG_GENERAL, SG_ALERT, "\t" << scratch->getDoubleValue("longitude-deg")
    << " " << scratch->getDoubleValue("latitude-deg") << " @ " << scratch->getDoubleValue("altitude-ft"));
    
  SG_LOG(SG_GENERAL, SG_ALERT, "\t" << scratch->getDoubleValue("true-bearing-deg") <<
    " (" << scratch->getDoubleValue("mag-bearing-deg") << " magnetic) " << scratch->getDoubleValue("distance-nm"));
  
  if (scratch->hasChild("result-index")) {
    SG_LOG(SG_GENERAL, SG_ALERT, "\tresult-index:" << scratch->getIntValue("result-index"));
  } 
  
  if (scratch->hasChild("route-index")) {
    SG_LOG(SG_GENERAL, SG_ALERT, "\troute-index:" << scratch->getIntValue("route-index"));
  }
}

void printRoute(const WayptVec& aRoute)
{
  SG_LOG(SG_GENERAL, SG_INFO, "route size=" << aRoute.size());
  for (unsigned int r=0; r<aRoute.size();++r) {
    Waypt* w = aRoute[r];
    SG_LOG(SG_GENERAL, SG_ALERT, "\t" << r << ": " << w->ident() << " "
      << w->owner()->ident());
  }
}

void createDummyRoute(FGRouteMgr* rm)
{
  SGPropertyNode* rmInput = fgGetNode("/autopilot/route-manager/input", true);
  rmInput->setStringValue("UW");
  rmInput->setStringValue("TLA/347/13");
  rmInput->setStringValue("TLA");
  rmInput->setStringValue("HAVEN");
  rmInput->setStringValue("NEW/305/29");
  rmInput->setStringValue("NEW");
  rmInput->setStringValue("OTR");
}

int main(int argc, char* argv[])
{

try{
  globals = new FGGlobals;
    
  fgInitFGRoot(argc, argv);
  if (!fgInitConfig(argc, argv) ) {
    SG_LOG( SG_GENERAL, SG_ALERT, "Config option parsing failed" );
    exit(-1);
  }
  
  
  fgInitNav();
  fgSetDouble("/environment/magnetic-variation-deg", 0.0);
  
  Airway::load();
  
  SG_LOG(SG_GENERAL, SG_ALERT, "hello world!");
  
  const FGAirport* egph = fgFindAirportID("EGPH");
  SG_LOG(SG_GENERAL, SG_ALERT, "egph: cart location:" << egph->cart());
  
  FGAirport::AirportFilter af;
  FGPositioned::List l = FGPositioned::findClosestN(egph->geod(), 20, 2000.0, &af);
  for (unsigned int i=0; i<l.size(); ++i) {
    SG_LOG(SG_GENERAL, SG_ALERT, "\t" << l[i]->ident() << "/" << l[i]->name());
  }
  
  //l = FGPositioned::findWithinRange(egph->geod(), 500.0, &af);
  //for (unsigned int i=0; i<l.size(); ++i) {
  //  SG_LOG(SG_GENERAL, SG_ALERT, "\t" << l[i]->ident() << "/" << l[i]->name());
  //}
  

  FGRouteMgr* rm = new FGRouteMgr;
  globals->add_subsystem( "route-manager", rm );
  
 // FGEnvironmentMgr* envMgr = new FGEnvironmentMgr;
 // globals->add_subsystem("environment", envMgr);
 // envMgr->init();
  
  fgSetBool("/sim/realism/simple-gps", true);
  
  // _realismSimpleGps
  
  SGPropertyNode* nd = fgGetNode("/instrumentation/gps", true);
  GPS* gps = new GPS(nd);
  globals->add_subsystem("gps", gps);

  const FGAirport* egph = fgFindAirportID("EGPH");
  testSetPosition(egph->geod());
  
  // startup the route manager
  rm->init();
  
  nd->setBoolValue("serviceable", true);
  fgSetBool("/systems/electrical/outputs/gps", true);
  
  gps->init();
  SGPropertyNode* scratch  = nd->getChild("scratch", 0, true);
  SGPropertyNode* wp = nd->getChild("wp", 0, true);
  SGPropertyNode* wp1 = wp->getChild("wp", 1, true);
  
  // update a few times
  gps->update(0.05);
  gps->update(0.05);
  gps->update(0.05);
  
  scratch->setStringValue("query", "TL");
  scratch->setStringValue("type", "Vor");
  scratch->setBoolValue("exact", false);
  nd->setStringValue("command", "search");
  printScratch(scratch);
  
  nd->setStringValue("command", "next");
  printScratch(scratch);
  
  nd->setStringValue("command", "next");
  printScratch(scratch);
  
// alphanumeric sort, partial matching
  nd->setDoubleValue("config/min-runway-length-ft", 5000.0);
  scratch->setBoolValue("exact", false);
  scratch->setBoolValue("order-by-distance", false);
  scratch->setStringValue("query", "KS");
  scratch->setStringValue("type", "apt");
  
  nd->setStringValue("command", "search");
  printScratch(scratch);
  
  nd->setStringValue("command", "next");
  printScratch(scratch);
  
  nd->setStringValue("command", "next");
  printScratch(scratch);
  
// alphanumeric sort, explicit matching
  scratch->setBoolValue("exact", true);
  scratch->setBoolValue("order-by-distance", true);
  scratch->setStringValue("type", "vor");
  scratch->setStringValue("query", "DCS");
  
  nd->setStringValue("command", "search");
  printScratch(scratch);
  
  nd->setStringValue("command", "next");
  printScratch(scratch);
  
// search on totally missing
  scratch->setBoolValue("exact", true);
  scratch->setBoolValue("order-by-distance", true);
  scratch->setStringValue("query", "FOFOFOFOF");
  nd->setStringValue("command", "search");
  printScratch(scratch);
  
// nearest
  scratch->setStringValue("type", "apt");
  scratch->setIntValue("max-results", 10);
  nd->setStringValue("command", "nearest");
  printScratch(scratch);
  
  nd->setStringValue("command", "next");
  printScratch(scratch);
  
  nd->setStringValue("command", "next");
  printScratch(scratch);
  
// direct to
  nd->setStringValue("command", "direct");
  SG_LOG(SG_GENERAL, SG_ALERT, "mode:" << nd->getStringValue("mode") << "\n\t"
    << wp1->getStringValue("ID") << " " << wp1->getDoubleValue("longitude-deg") 
    << " " << wp1->getDoubleValue("latitude-deg"));

// OBS mode
  scratch->setStringValue("query", "UW");
  scratch->setBoolValue("order-by-distance", true);
  nd->setStringValue("command", "search");
  printScratch(scratch);
  
  nd->setStringValue("command", "obs");
  SG_LOG(SG_GENERAL, SG_ALERT, "mode:" << nd->getStringValue("mode") << "\n\t"
    << wp1->getStringValue("ID") << " " << wp1->getDoubleValue("longitude-deg") 
    << " " << wp1->getDoubleValue("latitude-deg"));
  
// load route waypoints
  createDummyRoute(rm);

  scratch->setIntValue("route-index", 5);
  nd->setStringValue("command", "load-route-wpt");
  printScratch(scratch);
  
  nd->setStringValue("command", "next");
  printScratch(scratch);
  
  nd->setStringValue("command", "next");
  printScratch(scratch);
  
  scratch->setIntValue("route-index", 2);
  nd->setStringValue("command", "load-route-wpt");
  nd->setStringValue("command", "direct");
  SG_LOG(SG_GENERAL, SG_ALERT, "mode:" << nd->getStringValue("mode") << "\n\t"
    << wp1->getStringValue("ID") << " " << wp1->getDoubleValue("longitude-deg") 
    << " " << wp1->getDoubleValue("latitude-deg"));
  
// route editing
  SGGeod pos = egph->geod();
  scratch->setStringValue("ident", "FOOBAR");
  scratch->setDoubleValue("longitude-deg", pos.getLongitudeDeg());
  scratch->setDoubleValue("latitude-deg", pos.getLatitudeDeg());
  nd->setStringValue("command", "define-user-wpt");
  printScratch(scratch);
  
// airways
  FGPositioned::TypeFilter vorFilt(FGPositioned::VOR);
  FGPositionedRef tla = FGPositioned::findClosestWithIdent("TLA", pos, &vorFilt);
  FGPositionedRef big = FGPositioned::findClosestWithIdent("BIG", pos, &vorFilt); 
  FGPositionedRef pol = FGPositioned::findClosestWithIdent("POL", pos, &vorFilt);
   
  const FGAirport* eddm = fgFindAirportID("EDDM");
  FGPositionedRef mun = FGPositioned::findClosestWithIdent("MUN", 
    eddm->geod(), &vorFilt);
  
  const FGAirport* ksfo = fgFindAirportID("KSFO");
  FGPositionedRef sfo = FGPositioned::findClosestWithIdent("SFO", 
    ksfo->geod(), &vorFilt);


  WayptRef awy1 = new NavaidWaypoint(tla, NULL);
  WayptRef awy2 = new NavaidWaypoint(big, NULL);
  WayptRef awy3 = new NavaidWaypoint(pol, NULL);
  WayptRef awy4 = new NavaidWaypoint(mun, NULL);
  WayptRef awy5 = new NavaidWaypoint(sfo, NULL);
  
  WayptRef awy6 = new NavaidWaypoint(
    (FGPositioned*) fgFindAirportID("KJFK"), NULL);
  
  SGPath p("/Users/jmt/Desktop/airways.kml");
  std::fstream f;
  f.open(p.str().c_str(), fstream::out | fstream::trunc);
  
// pre-amble
  f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
    "<Document>\n";

  WayptVec route;
  Airway::highLevel()->route(awy1, awy3, route);
  Route::dumpRouteToLineString("egph-egcc", route, f);
  
  Airway::lowLevel()->route(awy1, awy2, route);
  Route::dumpRouteToLineString("egph-big", route, f);
  
  Airway::lowLevel()->route(awy2, awy4, route);
  Route::dumpRouteToLineString("big-mun", route, f);
  
  Airway::highLevel()->route(awy4, awy5, route);
  Route::dumpRouteToLineString("mun-sfo", route, f);
  
  Airway::lowLevel()->route(awy5, awy6, route);
  Route::dumpRouteToLineString("sfo-jfk", route, f);
  
  // post-amble
  f << "</Document>\n" 
    "</kml>" << endl;
  f.close();
  
// procedures
  SGPath op("/Users/jmt/Desktop/procedures.kml");
  f.open(op.str().c_str(), fstream::out | fstream::trunc);
  
  FGAirport* eham = (FGAirport*) fgFindAirportID("EHAM");
  FGPositioned::TypeFilter fixFilt(FGPositioned::FIX);
  
  WayptVec approach;
  FGPositionedRef redfa = FGPositioned::findClosestWithIdent("REDFA", 
    eham->geod(), &fixFilt);
  bool ok = eham->buildApproach(new NavaidWaypoint(redfa, NULL), 
    eham->getRunwayByIdent("18R"), approach);
  if (!ok ) {
    SG_LOG(SG_GENERAL, SG_INFO, "failed to build approach");
  }
  
  
  FGAirport* egll = (FGAirport*) fgFindAirportID("EGLL");  
  WayptVec approach2;
  ok = egll->buildApproach(new NavaidWaypoint(big, NULL), 
    egll->getRunwayByIdent("27R"), approach2);
  if (!ok ) {
    SG_LOG(SG_GENERAL, SG_INFO, "failed to build approach");
  }
  
// pre-amble
  f << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\n"
    "<Document>\n";
    
  Route::dumpRouteToLineString("REDFA 18R", approach, f);
  Route::dumpRouteToLineString("EGLL 27R", approach2, f);
  
  // post-amble
  f << "</Document>\n" 
    "</kml>" << endl;
  f.close();  
  
             
  return EXIT_SUCCESS;


  
} catch (sg_exception& ex) {
  SG_LOG(SG_GENERAL, SG_ALERT, "exception:" << ex.getFormattedMessage());
}
  return EXIT_FAILURE;
}
