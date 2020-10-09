#include "test_flightplan.hxx"

#include <algorithm>

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"

#include <simgear/misc/strutils.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/exception.hxx>

#include <Navaids/FlightPlan.hxx>
#include <Navaids/routePath.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/waypoint.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/airways.hxx>
#include <Navaids/fix.hxx>

#include <Airports/airport.hxx>

using namespace flightgear;

static bool static_haveProcedures = false;


/////////////////////////////////////////////////////////////////////////////

namespace {
class TestFPDelegate : public FlightPlan::Delegate
{
public:
    TestFPDelegate(FlightPlan* plan) :
        _plan(plan)
    {}
    
    virtual ~TestFPDelegate()
    {
    }
    
    void sequence() override
    {
    }
    
    void currentWaypointChanged() override
    {
    }
    
    void waypointsChanged() override
    {
        sawWaypointsChange = true;
    }
    
    void departureChanged() override
    {
        sawDepartureChange = true;

        
        // mimic the default delegate, inserting the SID waypoints
        
        // clear anything existing
        _plan->clearWayptsWithFlag(WPT_DEPARTURE);
        
        // insert waypt for the dpearture runway
        auto dr = new RunwayWaypt(_plan->departureRunway(), _plan);
        dr->setFlag(WPT_DEPARTURE);
        dr->setFlag(WPT_GENERATED);
        _plan->insertWayptAtIndex(dr, 0);
        
        if (_plan->sid()) {
            WayptVec sidRoute;
            bool ok = _plan->sid()->route(_plan->departureRunway(), _plan->sidTransition(), sidRoute);
            if (!ok)
                throw sg_exception("failed to route via SID");
            int insertIndex = 1;
            for (auto w : sidRoute) {
                w->setFlag(WPT_DEPARTURE);
                w->setFlag(WPT_GENERATED);
                _plan->insertWayptAtIndex(w, insertIndex++);
            }
        }
    }
    
    void arrivalChanged() override
    {
        sawArrivalChange = true;

        // mimic the default delegate, inserting the STAR waypoints
        
        // clear anything existing
        _plan->clearWayptsWithFlag(WPT_ARRIVAL);
        
        if (!_plan->destinationAirport()) {
            return;
        }
        
        if (!_plan->destinationRunway()) {
            auto ap = new NavaidWaypoint(_plan->destinationAirport(), _plan);
            ap->setFlag(WPT_ARRIVAL);
            ap->setFlag(WPT_GENERATED);
            _plan->insertWayptAtIndex(ap, -1);
            return;
        }
        
        // insert waypt for the destination runway
        auto dr = new RunwayWaypt(_plan->destinationRunway(), _plan);
        dr->setFlag(WPT_ARRIVAL);
        dr->setFlag(WPT_GENERATED);
        auto leg = _plan->insertWayptAtIndex(dr, -1);
        
        if (_plan->star()) {
            WayptVec starRoute;
            bool ok = _plan->star()->route(_plan->destinationRunway(), _plan->starTransition(), starRoute);
            if (!ok)
                throw sg_exception("failed to route via STAR");
            int insertIndex = leg->index();
            for (auto w : starRoute) {
                w->setFlag(WPT_ARRIVAL);
                w->setFlag(WPT_GENERATED);
                _plan->insertWayptAtIndex(w, insertIndex++);
            }
        }
        
        if (_plan->approach()) {
            WayptVec approachRoute;
            bool ok = _plan->approach()->routeFromVectors(approachRoute);
            if (!ok)
                throw sg_exception("failed to route approach");
            int insertIndex = leg->index();
            for (auto w : approachRoute) {
                w->setFlag(WPT_ARRIVAL);
                w->setFlag(WPT_GENERATED);
                _plan->insertWayptAtIndex(w, insertIndex++);
            }
        }
    }
    
    
    void loaded() override
    {
        didLoad = true;
    }
    
    FlightPlan* _plan = nullptr;
    bool didLoad = false;
    bool sawDepartureChange = false;
    bool sawArrivalChange = false;
    bool sawWaypointsChange = false;
};

class TestFPDelegateFactory : public FlightPlan::DelegateFactory
{
public:
    TestFPDelegateFactory() = default;
    virtual ~TestFPDelegateFactory() = default;

  FlightPlan::Delegate* createFlightPlanDelegate(FlightPlan* fp) override
  {
      auto d = new TestFPDelegate(fp);
      _instances.push_back(d);
      return d;
  }
    
    void destroyFlightPlanDelegate(FlightPlan* fp, FlightPlan::Delegate* d) override
    {
        auto it = std::find_if(_instances.begin(), _instances.end(), [d] (TestFPDelegate* fpd) {
            return fpd == d;
        });
        
        CPPUNIT_ASSERT(it != _instances.end());
        
        _instances.erase(it);
        delete d;
    }
    
    static TestFPDelegate* delegateForPlan(FlightPlan* fp);
    
    std::vector<TestFPDelegate*> _instances;
};

std::shared_ptr<TestFPDelegateFactory> static_factory;

TestFPDelegate* TestFPDelegateFactory::delegateForPlan(FlightPlan* fp)
{
    auto it = std::find_if(static_factory->_instances.begin(),
                           static_factory->_instances.end(),
                           [fp] (TestFPDelegate* del) {
        return del->_plan == fp;
    });
    
    if (it == static_factory->_instances.end())
        return nullptr;
    
    return *it;
}

} // of anonymous namespace

/////////////////////////////////////////////////////////////////////////////


// Set up function for each test.
void FlightplanTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("flightplan");
    FGTestApi::setUp::initNavDataCache();
    
    SGPath proceduresPath = SGPath::fromEnv("FG_PROCEDURES_PATH");
    if (proceduresPath.exists()) {
        static_haveProcedures = true;
        globals->append_fg_scenery(proceduresPath);
    }
    
    globals->get_subsystem_mgr()->bind();
    globals->get_subsystem_mgr()->init();
    globals->get_subsystem_mgr()->postinit();
}


// Clean up after each test.
void FlightplanTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
    
    if (static_factory) {
        FlightPlan::unregisterDelegateFactory(static_factory);
        static_factory.reset();
    }
}

static FlightPlanRef makeTestFP(const std::string& depICAO, const std::string& depRunway,
                         const std::string& destICAO, const std::string& destRunway,
                         const std::string& waypoints)
{
    FlightPlanRef f = new FlightPlan;
    FGTestApi::setUp::populateFPWithoutNasal(f, depICAO, depRunway, destICAO, destRunway, waypoints);
    return f;
}

void FlightplanTests::testBasic()
{
    FlightPlanRef fp1 = makeTestFP("EGCC", "23L", "EHAM", "24",
                                   "TNT CLN");
    fp1->setIdent("testplan");

    CPPUNIT_ASSERT(fp1->ident() == "testplan");
    CPPUNIT_ASSERT(fp1->departureAirport()->ident() == "EGCC");
    CPPUNIT_ASSERT(fp1->departureRunway()->ident() == "23L");
    CPPUNIT_ASSERT(fp1->destinationAirport()->ident() == "EHAM");
    CPPUNIT_ASSERT(fp1->destinationRunway()->ident() == "24");

    CPPUNIT_ASSERT_EQUAL(5, fp1->numLegs());

    CPPUNIT_ASSERT(fp1->legAtIndex(0)->waypoint()->source()->ident() == "23L");

    CPPUNIT_ASSERT(fp1->legAtIndex(1)->waypoint()->source()->ident() == "TNT");
    CPPUNIT_ASSERT(fp1->legAtIndex(1)->waypoint()->source()->name() == "TRENT VOR-DME");

    CPPUNIT_ASSERT(fp1->legAtIndex(2)->waypoint()->source()->ident() == "CLN");
    CPPUNIT_ASSERT(fp1->legAtIndex(2)->waypoint()->source()->name() == "CLACTON VOR-DME");

    CPPUNIT_ASSERT(fp1->legAtIndex(4)->waypoint()->source()->ident() == "24");

}

void FlightplanTests::testRoutePathBasic()
{
    FlightPlanRef fp1 = makeTestFP("EGHI", "20", "EDDM", "08L",
                                   "SFD LYD BNE CIV ELLX LUX SAA KRH WLD");


    RoutePath rtepath(fp1);
    const int legCount = fp1->numLegs();
    for (int leg = 0; leg < legCount; ++leg) {
        rtepath.trackForIndex(leg);
        rtepath.pathForIndex(leg);
        rtepath.distanceForIndex(leg);
    }

    rtepath.distanceBetweenIndices(2, 5);

    // check some leg parameters

    // BOLOUGNE SUR MER, near LFAY (AMIENS)
    FGNavRecordRef bne = FGNavList::findByFreq(113.8, FGAirport::getByIdent("LFAY")->geod());

    // CHIEVRES
    FGNavRecordRef civ = FGNavList::findByFreq(113.2, FGAirport::getByIdent("EBCI")->geod());

    double distM = SGGeodesy::distanceM(bne->geod(), civ->geod());
    double trackDeg = SGGeodesy::courseDeg(bne->geod(), civ->geod());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(trackDeg, rtepath.trackForIndex(4), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(distM, rtepath.distanceForIndex(4), 2000); // 2km precision, allow for turns

}

// https://sourceforge.net/p/flightgear/codetickets/1703/
// https://sourceforge.net/p/flightgear/codetickets/1939/

void FlightplanTests::testRoutePathSkipped()
{
    FlightPlanRef fp1 = makeTestFP("EHAM", "24", "EDDM", "08L",
                                   "EHEH KBO TAU FFM FFM/100/0.01 FFM/120/0.02 WUR WLD");


    RoutePath rtepath(fp1);

    // skipped point uses inbound track
    CPPUNIT_ASSERT_EQUAL(rtepath.trackForIndex(4), rtepath.trackForIndex(5));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, rtepath.distanceForIndex(5), 1e-9);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, rtepath.distanceForIndex(6), 1e-9);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(101000, rtepath.distanceForIndex(7), 1000);


    // this tests skipping two preceeding points works as it should
    SGGeodVec vec = rtepath.pathForIndex(7);
    CPPUNIT_ASSERT(vec.size() == 9);
}

void FlightplanTests::testRoutePathTrivialFlightPlan()
{
    FlightPlanRef fp1 = makeTestFP("EGPH", "24", "EGPH", "06",
                                   "");


    RoutePath rtepath(fp1);
    const int legCount = fp1->numLegs();
    for (int leg = 0; leg < legCount; ++leg) {
        rtepath.trackForIndex(leg);
        rtepath.pathForIndex(leg);
        rtepath.distanceForIndex(leg);
    }

    CPPUNIT_ASSERT_DOUBLES_EQUAL(19.68, fp1->totalDistanceNm(), 0.1);
}

void FlightplanTests::testRoutePathVec()
{
    FlightPlanRef fp1 = makeTestFP("KNUQ", "14L", "PHNL", "22R",
                                   "ROKME WOVAB");
    RoutePath rtepath(fp1);

    SGGeodVec vec = rtepath.pathForIndex(0);

    FGAirportRef ksfo = FGAirport::findByIdent("KSFO");
    FGFixRef rokme = fgpositioned_cast<FGFix>(FGPositioned::findClosestWithIdent("ROKME", ksfo->geod()));
    auto depRwy = fp1->departureRunway();

    CPPUNIT_ASSERT_DOUBLES_EQUAL(depRwy->geod().getLongitudeDeg(), vec.front().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(depRwy->geod().getLatitudeDeg(), vec.front().getLatitudeDeg(), 0.01);

    SGGeodVec vec1 = rtepath.pathForIndex(1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLongitudeDeg(), vec1.back().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLatitudeDeg(), vec1.back().getLatitudeDeg(), 0.01);

    SGGeodVec vec2 = rtepath.pathForIndex(2);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLongitudeDeg(), vec2.front().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLatitudeDeg(), vec2.front().getLatitudeDeg(), 0.01);

    //CPPUNIT_ASSERT(vec.front()
}

void FlightplanTests::testRoutePathFinalLegVQPR15()
{
    // test behaviour of RoutePath when the last leg prior to the arrival runway
    // is beyond the runway. This occurs in Paro RNAVZ15 approach.
    
    if (!static_haveProcedures)
        return;
    
    static_factory = std::make_shared<TestFPDelegateFactory>();
    FlightPlan::registerDelegateFactory(static_factory);
    
    FlightPlanRef f = new FlightPlan;
    auto ourDelegate = TestFPDelegateFactory::delegateForPlan(f);
    
    auto vidp = FGAirport::findByIdent("VIDP");
    f->setDeparture(vidp->getRunwayByIdent("09"));
    
    auto vqpr = FGAirport::findByIdent("VQPR");
    f->setDestination(vqpr->getRunwayByIdent("15"));
    f->setApproach(vqpr->findApproachWithIdent("RNVZ15"));
    RoutePath rtepath(f);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1100, rtepath.distanceForIndex(14), 100); // calculated from raw lat lon. 
}

void FlightplanTests::testRoutPathWpt0Midflight()
{
    // test behaviour of RoutePath when WP0 is not a runway
    // happens for the Airbus ND which removes past wpts when sequencing

    FlightPlanRef fp1 = makeTestFP("KNUQ", "14L", "PHNL", "22R",
                                   "ROKME WOVAB");
    // actually delete leg 0 so we start at ROKME
    fp1->deleteIndex(0);

    RoutePath rtepath(fp1);

    SGGeodVec vec = rtepath.pathForIndex(0);

    FGAirportRef ksfo = FGAirport::findByIdent("KSFO");
    FGFixRef rokme = fgpositioned_cast<FGFix>(FGPositioned::findClosestWithIdent("ROKME", ksfo->geod()));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLongitudeDeg(), vec.front().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLatitudeDeg(), vec.front().getLatitudeDeg(), 0.01);

    SGGeodVec vec2 = rtepath.pathForIndex(1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLongitudeDeg(), vec2.front().getLongitudeDeg(), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(rokme->geod().getLatitudeDeg(), vec2.front().getLatitudeDeg(), 0.01);

    //CPPUNIT_ASSERT(vec.front()
}

void FlightplanTests::testBasicAirways()
{
    Airway* awy = Airway::findByIdent("J547", Airway::HighLevel);
    CPPUNIT_ASSERT_EQUAL(awy->ident(), std::string("J547"));

    FGAirportRef kord = FGAirport::findByIdent("KORD");
    FlightPlanRef f = new FlightPlan;
    f->setDeparture(kord);

    CPPUNIT_ASSERT(awy->findEnroute("KITOK"));
    CPPUNIT_ASSERT(awy->findEnroute("LESUB"));

    auto wpt = awy->findEnroute("FNT");
    CPPUNIT_ASSERT(wpt);
    CPPUNIT_ASSERT(wpt->ident() == "FNT");
    CPPUNIT_ASSERT(wpt->source() == FGNavRecord::findClosestWithIdent("FNT", kord->geod()));

    auto wptKUBBS = f->waypointFromString("KUBBS");
    auto wptFNT = f->waypointFromString("FNT");

    CPPUNIT_ASSERT(awy->canVia(wptKUBBS, wptFNT));

    WayptVec path = awy->via(wptKUBBS, wptFNT);
    CPPUNIT_ASSERT_EQUAL(4, static_cast<int>(path.size()));

    CPPUNIT_ASSERT_EQUAL(std::string("PMM"), path.at(0)->ident());
    CPPUNIT_ASSERT_EQUAL(std::string("HASTE"), path.at(1)->ident());
    CPPUNIT_ASSERT_EQUAL(std::string("DEWIT"), path.at(2)->ident());
    CPPUNIT_ASSERT_EQUAL(std::string("FNT"), path.at(3)->ident());
}

void FlightplanTests::testAirwayNetworkRoute()
{
    FGAirportRef egph = FGAirport::findByIdent("EGPH");
    FlightPlanRef f = new FlightPlan;
    f->setDeparture(egph);

    auto highLevelNet = Airway::highLevel();

    auto wptTLA = f->waypointFromString("TLA");
    auto wptCNA = f->waypointFromString("CNA");

    WayptVec route;
    bool ok = highLevelNet->route(wptTLA, wptCNA, route);
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(static_cast<int>(route.size()), 18);
}

void FlightplanTests::testParseICAORoute()
{
    FGAirportRef kord = FGAirport::findByIdent("KORD");
    FlightPlanRef f = new FlightPlan;
    f->setDeparture(kord);
    f->setDestination(FGAirport::findByIdent("KSAN"));

    const char* route = "DCT JOT J26 IRK J96 SLN J18 GCK J96 CIM J134 GUP J96 KEYKE J134 DRK J78 LANCY J96 PKE";
  //  const char* route = "DCT KUBBS J547 FNT Q824 HOCKE Q905 SIKBO Q907 MIILS N400A TUDEP NATW GISTI DCT SLANY UL9 DIKAS UL18 GAVGO UL9 KONAN UL607 UBIDU Y863 RUDUS T109 HAREM T104 ANORA STAR";
    bool ok = f->parseICAORouteString(route);
    CPPUNIT_ASSERT(ok);



}

void FlightplanTests::testParseICANLowLevelRoute()
{
    const char* route = "DCT DPA V6 IOW V216 LAA V210 GOSIP V83 ACHES V210 BLOKE V83 ALS V210 RSK V95 INW V12 HOXOL V264 OATES V12 JUWSO V264 PKE";

    FGAirportRef kord = FGAirport::findByIdent("KORD");
    FlightPlanRef f = new FlightPlan;
    f->setDeparture(kord);
    f->setDestination(FGAirport::findByIdent("KSAN"));

    bool ok = f->parseICAORouteString(route);
    CPPUNIT_ASSERT(ok);
}

// https://sourceforge.net/p/flightgear/codetickets/1814/
void FlightplanTests::testBug1814()
{
    const std::string fpXML = R"(<?xml version="1.0" encoding="UTF-8"?>
                    <PropertyList>
                        <version type="int">2</version>
                        <departure>
                            <airport type="string">SAWG</airport>
                            <runway type="string">25</runway>
                        </departure>
                        <destination>
                            <airport type="string">SUMU</airport>
                        </destination>
                        <route>
                              <wp n="6">
                                    <type type="string">navaid</type>
                                    <ident type="string">PUGLI</ident>
                                    <lon type="double">-60.552200</lon>
                                    <lat type="double">-40.490000</lat>
                                  </wp>
                            <wp n="7">
                                <type type="string">navaid</type>
                                <ident type="string">SIGUL</ident>
                                <lon type="double">-59.655800</lon>
                                <lat type="double">-38.312800</lat>
                            </wp>
                            <wp n="8">
                                <type type="string">navaid</type>
                                <ident type="string">MDP</ident>
                                <lon type="double">-57.576400</lon>
                                <lat type="double">-37.929800</lat>
                            </wp>
                        </route>
                    </PropertyList>
  )";

    std::istringstream stream(fpXML);
    FlightPlanRef f = new FlightPlan;
    bool ok = f->load(stream);
    CPPUNIT_ASSERT(ok);

    auto leg = f->legAtIndex(1);
    CPPUNIT_ASSERT_EQUAL(std::string("SIGUL"), leg->waypoint()->ident());

    CPPUNIT_ASSERT_DOUBLES_EQUAL(137, leg->distanceNm(), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(101, f->legAtIndex(2)->distanceNm(), 0.5);
}

void FlightplanTests::testLoadSaveMachRestriction()
{
    const std::string fpXML = R"(<?xml version="1.0" encoding="UTF-8"?>
       <PropertyList>
           <version type="int">2</version>
           <departure>
               <airport type="string">SAWG</airport>
               <runway type="string">25</runway>
           </departure>
           <destination>
               <airport type="string">SUMU</airport>
           </destination>
           <route>
                <wp n="0">
                  <type type="string">navaid</type>
                  <ident type="string">PUGLI</ident>
                  <lon type="double">-60.552200</lon>
                  <lat type="double">-40.490000</lat>
                </wp>
                <wp n="1">
                  <type type="string">basic</type>
                  <alt-restrict type="string">at</alt-restrict>
                  <altitude-ft type="double">36000</altitude-ft>
                  <speed-restrict type="string">mach</speed-restrict>
                  <speed type="double">1.24</speed>
                  <ident type="string">SV002</ident>
                  <lon type="double">-115.50531</lon>
                  <lat type="double">37.89523</lat>
                </wp>
                <wp n="2">
                     <type type="string">navaid</type>
                     <ident type="string">SIGUL</ident>
                     <lon type="double">-60.552200</lon>
                     <lat type="double">-40.490000</lat>
                   </wp>
           </route>
       </PropertyList>
     )";
    
     std::istringstream stream(fpXML);
     FlightPlanRef f = new FlightPlan;
     bool ok = f->load(stream);
     CPPUNIT_ASSERT(ok);

     auto leg = f->legAtIndex(1);
    CPPUNIT_ASSERT_EQUAL(SPEED_RESTRICT_MACH, leg->speedRestriction());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.24, leg->speedMach(), 0.01);
    
    auto firstLeg = f->legAtIndex(0);
    firstLeg->setSpeed(SPEED_RESTRICT_MACH, 1.56);
    
    // upgrade to a hold and set the count
    f->legAtIndex(2)->setHoldCount(8);
    
    // round trip through XML to check :)
    std::ostringstream ss;
    f->save(ss);
    
    std::istringstream iss(ss.str());
    FlightPlanRef f2 = new FlightPlan;
    ok = f2->load(iss);
    CPPUNIT_ASSERT(ok);
    
    auto leg3 = f2->legAtIndex(0);
    CPPUNIT_ASSERT_EQUAL(SPEED_RESTRICT_MACH, leg3->speedRestriction());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.56, leg3->speedMach(), 0.01);
    
    CPPUNIT_ASSERT_EQUAL(8, f2->legAtIndex(2)->holdCount());
}

void FlightplanTests::testBasicDiscontinuity()
{
    FlightPlanRef fp1 = makeTestFP("LIRF", "34L", "LEBL", "25R",
                                   "ESINO GITRI BALEN MUREN TOSNU");

    const auto tdBefore = fp1->totalDistanceNm();
    
    
    const SGGeod balenPos = fp1->legAtIndex(3)->waypoint()->position();
    const SGGeod murenPos = fp1->legAtIndex(4)->waypoint()->position();
    const auto crs = SGGeodesy::courseDeg(balenPos, murenPos);
    
    // total distance should not change
    fp1->insertWayptAtIndex(new Discontinuity(fp1), 4); // betwee BALEN and MUREN
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(tdBefore, fp1->totalDistanceNm(), 1.0);

    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(4)->courseDeg(),
                                 SGGeodesy::courseDeg(balenPos, murenPos), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(4)->distanceNm(), 0.0, 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(5)->courseDeg(),
                                 SGGeodesy::courseDeg(balenPos, murenPos), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(5)->distanceNm(),
                                 SGGeodesy::distanceNm(balenPos, murenPos), 0.1);
    
    // ensure that pointAlongRoute works correctly going into the DISCON
    
    
    const auto pos1 = fp1->pointAlongRoute(3, 20.0);
    const auto validP1 = SGGeodesy::direct(balenPos, crs, 20.0 * SG_NM_TO_METER);
    
    CPPUNIT_ASSERT(SGGeodesy::distanceM(pos1, validP1) < 500.0);
    
    const auto pos2 = fp1->pointAlongRoute(5, -10.0);
    const auto crs2 = SGGeodesy::courseDeg(murenPos, balenPos);
    const auto validP2 = SGGeodesy::direct(murenPos, crs2, 10.0 * SG_NM_TO_METER);
    
    CPPUNIT_ASSERT(SGGeodesy::distanceM(pos2, validP2) < 500.0);
    
    // remove the discontinuity
    fp1->deleteIndex(4);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(4)->courseDeg(),
                                 SGGeodesy::courseDeg(balenPos, murenPos), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(fp1->legAtIndex(4)->distanceNm(),
                                 SGGeodesy::distanceNm(balenPos, murenPos), 0.1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(tdBefore, fp1->totalDistanceNm(), 1.0);

}

void FlightplanTests::testOnlyDiscontinuityRoute()
{
    FlightPlanRef f = new FlightPlan;
    FGAirportRef depApt = FGAirport::getByIdent("LFBD");
    f->setDeparture(depApt);


    FGAirportRef destApt = FGAirport::getByIdent("EHAM");
    f->setDestination(destApt);

    f->insertWayptAtIndex(new Discontinuity(f), 0);
    
    RoutePath rp1(f);
    
    // discontinuity should act like a straight segment between preceeding and following
    const double d = SGGeodesy::distanceNm(depApt->geod(), destApt->geod());
   // CPPUNIT_ASSERT_DOUBLES_EQUAL(rp1.distanceForIndex(0), d, 0.5);
    
    // start inserting waypoints ahead of the DISCON, Boeing FPL style
    WayptRef wpt = f->waypointFromString("LMG");
    f->insertWayptAtIndex(wpt, 0);
    
    wpt = f->waypointFromString("KUKOR");
    f->insertWayptAtIndex(wpt, 1);
    
    wpt = f->waypointFromString("EPL");
    f->insertWayptAtIndex(wpt, 2);
}

void FlightplanTests::testLeadingWPDynamic()
{
    FlightPlanRef f = new FlightPlan;
    // plan has no departure, so this discon is floating
    f->insertWayptAtIndex(new Discontinuity(f), 0);
    
    auto ha = new HeadingToAltitude(f, "TO_3000", 90);
    ha->setAltitude(3000, RESTRICT_AT);
    f->insertWayptAtIndex(ha, 1);
    
    RoutePath rp1(f);
    // distance will be invalid, but shouldn;t assert or crash :)
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, f->totalDistanceNm(), 0.001);
}

void FlightplanTests::testRadialIntercept()
{
    // replicate AJO1R departure
    FlightPlanRef f = makeTestFP("LFKC", "36", "LIRF", "25", "BUNAX BEBEV AJO");
    
    // set BUNAX as overflight
    f->legAtIndex(1)->waypoint()->setFlag(WPT_OVERFLIGHT);
    f->insertWayptAtIndex(new BasicWaypt(SGGeod::fromDeg(8.78333, 42.566), "KC502", f), 1);

    SGGeod pos = SGGeod::fromDeg(8.445556,42.216944);
    auto intc = new RadialIntercept(f, "INTC", pos, 230, 5);
    f->insertWayptAtIndex(intc, 3); // between BUNAX and BEBEV
    
    RoutePath rtepath(f);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(232, f->legAtIndex(3)->courseDeg(), 1.0);
}

void FlightplanTests::loadFGFPWithoutDepartureArrival()
{
    static_factory = std::make_shared<TestFPDelegateFactory>();
    FlightPlan::registerDelegateFactory(static_factory);

    FlightPlanRef f = new FlightPlan;
    
    SGPath fgfpPath = simgear::Dir::current().path() / "test_fgfp_without_dep_arr.fgfp";
    {
        sg_ofstream s(fgfpPath);
        s << R"(<?xml version="1.0" encoding="UTF-8"?>
            <PropertyList>
              <version type="int">2</version>
              <departure>
                <airport type="string">EDDM</airport>
                <runway type="string">08R</runway>
              </departure>
              <destination>
                <airport type="string">EDDF</airport>
              </destination>
              <route>
                <wp n="1">
                  <type type="string">navaid</type>
                  <ident type="string">GIVMI</ident>
                  <lon type="double">11.364700</lon>
                  <lat type="double">48.701100</lat>
                </wp>
                <wp n="2">
                  <type type="string">navaid</type>
                  <ident type="string">ERNAS</ident>
                  <lon type="double">11.219400</lon>
                  <lat type="double">48.844700</lat>
                </wp>
                <wp n="3">
                  <type type="string">navaid</type>
                  <ident type="string">TALAL</ident>
                  <lon type="double">11.085300</lon>
                  <lat type="double">49.108300</lat>
                </wp>
                <wp n="4">
                  <type type="string">navaid</type>
                  <ident type="string">ERMEL</ident>
                  <lon type="double">11.044700</lon>
                  <lat type="double">49.187800</lat>
                </wp>
                <wp n="5">
                  <type type="string">navaid</type>
                  <ident type="string">PSA</ident>
                  <lon type="double">9.348300</lon>
                  <lat type="double">49.862200</lat>
                </wp>
              </route>
            </PropertyList>
        )";
    }
    
    auto ourDelegate = TestFPDelegateFactory::delegateForPlan(f);
    CPPUNIT_ASSERT(!ourDelegate->didLoad);
    
    CPPUNIT_ASSERT(f->load(fgfpPath));
    
    CPPUNIT_ASSERT(ourDelegate->didLoad);
    CPPUNIT_ASSERT(ourDelegate->sawArrivalChange);
    CPPUNIT_ASSERT(ourDelegate->sawDepartureChange);
}

void FlightplanTests::loadFGFPWithEmbeddedProcedures()
{
    static_factory = std::make_shared<TestFPDelegateFactory>();
    FlightPlan::registerDelegateFactory(static_factory);

    FlightPlanRef f = new FlightPlan;
    
    SGPath fgfpPath = simgear::Dir::current().path() / "test_fgfp_with_dep_arr.fgfp";
    {
        sg_ofstream s(fgfpPath);
        s << R"(<?xml version="1.0" encoding="UTF-8"?>
            <PropertyList>
              <version type="int">2</version>
              <departure>
                <airport type="string">EDDM</airport>
                <runway type="string">08R</runway>
              </departure>
              <destination>
                <airport type="string">EDDF</airport>
              </destination>
              <route>
                    <wp>
                      <type type="string">runway</type>
                      <departure type="bool">true</departure>
                      <ident type="string">08R</ident>
                      <icao type="string">EDDM</icao>
                    </wp>
                <wp n="1">
                  <type type="string">navaid</type>
                  <ident type="string">GIVMI</ident>
                  <lon type="double">11.364700</lon>
                  <lat type="double">48.701100</lat>
                </wp>
                <wp n="2">
                  <type type="string">navaid</type>
                  <ident type="string">ERNAS</ident>
                  <lon type="double">11.219400</lon>
                  <lat type="double">48.844700</lat>
                </wp>
                <wp n="3">
                  <type type="string">navaid</type>
                  <ident type="string">TALAL</ident>
                  <lon type="double">11.085300</lon>
                  <lat type="double">49.108300</lat>
                </wp>
                <wp n="4">
                  <type type="string">navaid</type>
                  <ident type="string">ERMEL</ident>
                  <lon type="double">11.044700</lon>
                  <lat type="double">49.187800</lat>
                </wp>
                <wp n="5">
                  <type type="string">navaid</type>
                  <ident type="string">PSA</ident>
                  <lon type="double">9.348300</lon>
                  <lat type="double">49.862200</lat>
                </wp>
        
                    <wp>
                      <type type="string">runway</type>
                      <arrival type="bool">true</arrival>
                      <ident type="string">36</ident>
                      <icao type="string">EDDF</icao>
                    </wp>
              </route>
            </PropertyList>
        )";
    }
    
    auto ourDelegate = TestFPDelegateFactory::delegateForPlan(f);
    CPPUNIT_ASSERT(!ourDelegate->didLoad);
    
    CPPUNIT_ASSERT(f->load(fgfpPath));
    
    CPPUNIT_ASSERT(ourDelegate->didLoad);
    CPPUNIT_ASSERT(!ourDelegate->sawArrivalChange);
    CPPUNIT_ASSERT(!ourDelegate->sawDepartureChange);
}

void FlightplanTests::loadFGFPWithOldProcedures()
{
    if (!static_haveProcedures)
        return;
    
    FlightPlanRef f = new FlightPlan;
    
    SGPath fgfpPath = simgear::Dir::current().path() / "test_fgfp_old_procedure_idents.fgfp";
    {
        sg_ofstream s(fgfpPath);
        s << R"(<?xml version="1.0" encoding="UTF-8"?>
            <PropertyList>
              <version type="int">2</version>
              <departure>
                <airport type="string">KJFK</airport>
                <runway type="string">13L</runway>
                <sid type="string">CANDR</sid>
              </departure>
              <destination>
                <airport type="string">EHAM</airport>
                <star type="string">BEDUM</star>
              </destination>
              <route>
                    <wp>
                      <type type="string">runway</type>
                      <arrival type="bool">true</arrival>
                      <ident type="string">36</ident>
                      <icao type="string">EDDF</icao>
                    </wp>
              </route>
            </PropertyList>
        )";
    }
    
    auto kjfk = FGAirport::findByIdent("KJFK");
    auto eham = FGAirport::findByIdent("EHAM");
    CPPUNIT_ASSERT(f->load(fgfpPath));
    
    CPPUNIT_ASSERT(f->sid() == nullptr);
    CPPUNIT_ASSERT(f->star() == nullptr);
}

void FlightplanTests::loadFGFPWithProcedureIdents()
{
    if (!static_haveProcedures)
        return;
    
    FlightPlanRef f = new FlightPlan;
    
    SGPath fgfpPath = simgear::Dir::current().path() / "test_fgfp_procedure_idents.fgfp";
    {
        sg_ofstream s(fgfpPath);
        s << R"(<?xml version="1.0" encoding="UTF-8"?>
            <PropertyList>
              <version type="int">2</version>
              <departure>
                <airport type="string">KJFK</airport>
                <runway type="string">13L</runway>
                <sid type="string">DEEZZ5.13L</sid>
                <sid_trans type="string">CANDR</sid_trans>
              </departure>
              <destination>
                <airport type="string">EHAM</airport>
                <star type="string">EEL1A</star>
                <star_trans type="string">KUBAT</star_trans>
              </destination>
              <route>
                    <wp>
                      <type type="string">runway</type>
                      <arrival type="bool">true</arrival>
                      <ident type="string">36</ident>
                      <icao type="string">EDDF</icao>
                    </wp>
              </route>
            </PropertyList>
        )";
    }
    
    auto kjfk = FGAirport::findByIdent("KJFK");
    auto eham = FGAirport::findByIdent("EHAM");
    CPPUNIT_ASSERT(f->load(fgfpPath));
    
    CPPUNIT_ASSERT_EQUAL(f->sid()->ident(), string{"DEEZZ5.13L"});
    CPPUNIT_ASSERT_EQUAL(f->sidTransition()->ident(), string{"CANDR"});
    
    CPPUNIT_ASSERT_EQUAL(f->star()->ident(), string{"EEL1A"});
    CPPUNIT_ASSERT_EQUAL(f->starTransition()->ident(), string{"KUBAT"});
}

void FlightplanTests::testCloningBasic()
{
    FlightPlanRef fp1 = makeTestFP("EGCC", "23L", "EHAM", "24",
                                   "TNT CLN");
    fp1->setIdent("testplan");

    fp1->setCruiseAltitudeFt(24000);
    fp1->setCruiseSpeedKnots(448);

    auto fp2 = fp1->clone("testplan2");

    CPPUNIT_ASSERT(fp2->ident() == "testplan2");
    CPPUNIT_ASSERT(fp2->departureAirport()->ident() == "EGCC");
    CPPUNIT_ASSERT(fp2->departureRunway()->ident() == "23L");
    CPPUNIT_ASSERT(fp2->destinationAirport()->ident() == "EHAM");
    CPPUNIT_ASSERT(fp2->destinationRunway()->ident() == "24");
    CPPUNIT_ASSERT_EQUAL(24000, fp2->cruiseAltitudeFt());
    CPPUNIT_ASSERT_EQUAL(448, fp2->cruiseSpeedKnots());

    CPPUNIT_ASSERT_EQUAL(5, fp2->numLegs());

    CPPUNIT_ASSERT(fp2->legAtIndex(0)->waypoint()->source()->ident() == "23L");
    CPPUNIT_ASSERT(fp2->legAtIndex(1)->waypoint()->source()->ident() == "TNT");
    CPPUNIT_ASSERT(fp2->legAtIndex(2)->waypoint()->source()->ident() == "CLN");

}

void FlightplanTests::testCloningFGFP()
{
    static_factory = std::make_shared<TestFPDelegateFactory>();
    FlightPlan::registerDelegateFactory(static_factory);

    FlightPlanRef fp1 = new FlightPlan;
    
    SGPath fgfpPath = simgear::Dir::current().path() / "test_fgfp_cloning.fgfp";
    {
        sg_ofstream s(fgfpPath);
        s << R"(<?xml version="1.0" encoding="UTF-8"?>
            <PropertyList>
              <version type="int">2</version>
              <departure>
                <airport type="string">EDDM</airport>
                <runway type="string">08R</runway>
              </departure>
              <destination>
                <airport type="string">EDDF</airport>
              </destination>
              <route>
                <wp n="1">
                  <type type="string">navaid</type>
                  <ident type="string">GIVMI</ident>
                  <lon type="double">11.364700</lon>
                  <lat type="double">48.701100</lat>
                </wp>
                <wp n="2">
                  <type type="string">navaid</type>
                  <ident type="string">ERNAS</ident>
                  <lon type="double">11.219400</lon>
                  <lat type="double">48.844700</lat>
                </wp>
                <wp n="3">
                  <type type="string">navaid</type>
                  <ident type="string">TALAL</ident>
                  <lon type="double">11.085300</lon>
                  <lat type="double">49.108300</lat>
                </wp>
                <wp n="4">
                  <type type="string">navaid</type>
                  <ident type="string">ERMEL</ident>
                  <lon type="double">11.044700</lon>
                  <lat type="double">49.187800</lat>
                </wp>
                <wp n="5">
                  <type type="string">navaid</type>
                  <ident type="string">PSA</ident>
                  <lon type="double">9.348300</lon>
                  <lat type="double">49.862200</lat>
                </wp>
              </route>
            </PropertyList>
        )";
    }
    
    auto ourDelegate = TestFPDelegateFactory::delegateForPlan(fp1);
    CPPUNIT_ASSERT(!ourDelegate->didLoad);
    
    CPPUNIT_ASSERT(fp1->load(fgfpPath));
    
    CPPUNIT_ASSERT(ourDelegate->didLoad);
    CPPUNIT_ASSERT(ourDelegate->sawArrivalChange);
    CPPUNIT_ASSERT(ourDelegate->sawDepartureChange);
    
    auto fp2 = fp1->clone();
    auto secondDelegate = TestFPDelegateFactory::delegateForPlan(fp2);

    CPPUNIT_ASSERT(secondDelegate->didLoad);
    CPPUNIT_ASSERT(secondDelegate->sawWaypointsChange);
    CPPUNIT_ASSERT(!secondDelegate->sawArrivalChange);
    CPPUNIT_ASSERT(!secondDelegate->sawDepartureChange);
    
    CPPUNIT_ASSERT(fp2->departureAirport()->ident() == "EDDM");
    CPPUNIT_ASSERT(fp2->departureRunway()->ident() == "08R");
    CPPUNIT_ASSERT(fp2->destinationAirport()->ident() == "EDDF");

    CPPUNIT_ASSERT_EQUAL(fp2->legAtIndex(0)->waypoint()->source()->ident(), string{"08R"});
    CPPUNIT_ASSERT_EQUAL(fp2->legAtIndex(1)->waypoint()->source()->ident(), string{"GIVMI"});
    CPPUNIT_ASSERT_EQUAL(fp2->legAtIndex(5)->waypoint()->source()->ident(), string{"PSA"});
    CPPUNIT_ASSERT_EQUAL(fp2->legAtIndex(6)->waypoint()->source()->ident(), string{"EDDF"});
    CPPUNIT_ASSERT_EQUAL(7, fp2->numLegs());
    
}

void FlightplanTests::testCloningProcedures() {
    // procedures not loaded, abandon test
    if (!static_haveProcedures)
        return;
    
    static_factory = std::make_shared<TestFPDelegateFactory>();
    FlightPlan::registerDelegateFactory(static_factory);
    
    auto egkk = FGAirport::findByIdent("EGKK");
    auto sid = egkk->findSIDWithIdent("SAM3P");
    
    FlightPlanRef fp1 = makeTestFP("EGKK", "08R", "EHAM", "18R",
                                   "");
    auto ourDelegate = TestFPDelegateFactory::delegateForPlan(fp1);
    
    fp1->setSID(sid);
    auto eham = FGAirport::findByIdent("EHAM");
    auto eel1A = eham->findSTARWithIdent("EEL1A");
    fp1->setSTAR(eel1A, "BEDUM");
    
    auto fp2 = fp1->clone();
    CPPUNIT_ASSERT(fp2->departureAirport()->ident() == "EGKK");
    CPPUNIT_ASSERT(fp2->departureRunway()->ident() == "08R");
    CPPUNIT_ASSERT(fp2->destinationAirport()->ident() == "EHAM");
    CPPUNIT_ASSERT(fp2->destinationRunway()->ident() == "18R");

    CPPUNIT_ASSERT(fp2->legAtIndex(0)->waypoint()->source()->ident() == "08R");
    CPPUNIT_ASSERT_EQUAL(fp2->sid()->ident(), string{"SAM3P"});
    
    CPPUNIT_ASSERT_EQUAL(fp2->star()->ident(), string{"EEL1A"});
    CPPUNIT_ASSERT_EQUAL(fp2->starTransition()->ident(), string{"BEDUM"});
}