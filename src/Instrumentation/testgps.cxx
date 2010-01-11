
#include <Main/fg_init.hxx>
#include <Main/globals.hxx>
#include <Main/fg_props.hxx>

#include <Instrumentation/gps.hxx>
#include <Autopilot/route_mgr.hxx>
#include <Environment/environment_mgr.hxx>

using std::string;

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
  globals = new FGGlobals;
    
  fgInitFGRoot(argc, argv);
  if (!fgInitConfig(argc, argv) ) {
    SG_LOG( SG_GENERAL, SG_ALERT, "Config option parsing failed" );
    exit(-1);
  }
  
  
  fgInitNav();
  
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
  
  
  SGPropertyNode* nd = fgGetNode("/instrumentation/gps", true);
  GPS* gps = new GPS(nd);
  globals->add_subsystem("gps", gps);
  
  
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
  
  
  
  return EXIT_SUCCESS;
}
