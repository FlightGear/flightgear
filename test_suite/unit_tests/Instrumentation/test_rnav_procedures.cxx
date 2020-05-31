/*
 * Copyright (C) 2019 James Turner
 *
 * This file is part of the program FlightGear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "test_rnav_procedures.hxx"

#include <memory>
#include <cstring>

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"

#include <simgear/structure/exception.hxx>

#include <Airports/airport.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/FlightPlan.hxx>

#include <Instrumentation/gps.hxx>

#include <Autopilot/route_mgr.hxx>

using namespace flightgear;

/////////////////////////////////////////////////////////////////////////////

namespace {
class TestFPDelegate : public FlightPlan::Delegate
{
public:
    FlightPlanRef thePlan;
    int sequenceCount = 0;

    virtual ~TestFPDelegate()
    {
    }
    
    void sequence() override
    {
        
        ++sequenceCount;
        int newIndex = thePlan->currentIndex() + 1;
        if (newIndex >= thePlan->numLegs()) {
            thePlan->finish();
            return;
        }
        
        thePlan->setCurrentIndex(newIndex);
    }
    
    void currentWaypointChanged() override
    {
    }
    
    void departureChanged() override
    {
        // mimic the default delegate, inserting the SID waypoints
        
        // clear anything existing
        thePlan->clearWayptsWithFlag(WPT_DEPARTURE);
        
        // insert waypt for the dpearture runway
        auto dr = new RunwayWaypt(thePlan->departureRunway(), thePlan);
        dr->setFlag(WPT_DEPARTURE);
        dr->setFlag(WPT_GENERATED);
        thePlan->insertWayptAtIndex(dr, 0);
        
        if (thePlan->sid()) {
            WayptVec sidRoute;
            bool ok = thePlan->sid()->route(thePlan->departureRunway(), thePlan->sidTransition(), sidRoute);
            if (!ok)
                throw sg_exception("failed to route via SID");
            int insertIndex = 1;
            for (auto w : sidRoute) {
                w->setFlag(WPT_DEPARTURE);
                w->setFlag(WPT_GENERATED);
                thePlan->insertWayptAtIndex(w, insertIndex++);
            }
        }
    }
    
    void arrivalChanged() override
    {
        // mimic the default delegate, inserting the STAR waypoints
        
        // clear anything existing
        thePlan->clearWayptsWithFlag(WPT_ARRIVAL);
        
        // insert waypt for the destination runway
        auto dr = new RunwayWaypt(thePlan->destinationRunway(), thePlan);
        dr->setFlag(WPT_ARRIVAL);
        dr->setFlag(WPT_GENERATED);
        auto leg = thePlan->insertWayptAtIndex(dr, -1);
        
        if (thePlan->star()) {
            WayptVec starRoute;
            bool ok = thePlan->star()->route(thePlan->destinationRunway(), thePlan->starTransition(), starRoute);
            if (!ok)
                throw sg_exception("failed to route via STAR");
            int insertIndex = leg->index();
            for (auto w : starRoute) {
                w->setFlag(WPT_ARRIVAL);
                w->setFlag(WPT_GENERATED);
                thePlan->insertWayptAtIndex(w, insertIndex++);
            }
        }
    }
};

} // of anonymous namespace

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void RNAVProcedureTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("rnav-procedures");
    FGTestApi::setUp::initNavDataCache();
    
    SGPath proceduresPath = SGPath::fromEnv("FG_PROCEDURES_PATH");
    if (proceduresPath.exists()) {
        globals->append_fg_scenery(proceduresPath);
    }
    
    setupRouteManager();
}

// Clean up after each test.
void RNAVProcedureTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

GPS* RNAVProcedureTests::setupStandardGPS(SGPropertyNode_ptr config,
                                const std::string name, const int index)
{
    SGPropertyNode_ptr configNode(config.valid() ? config
                                                 : SGPropertyNode_ptr{new SGPropertyNode});
    configNode->setStringValue("name", name);
    configNode->setIntValue("number", index);
    
    GPS* gps(new GPS(configNode));
    m_gps = gps;
    
    m_gpsNode = globals->get_props()->getNode("instrumentation", true)->getChild(name, index, true);
    m_gpsNode->setBoolValue("serviceable", true);
    globals->get_props()->setDoubleValue("systems/electrical/outputs/gps", 6.0);
    
    gps->bind();
    gps->init();
    
    globals->add_subsystem("gps", gps, SGSubsystemMgr::POST_FDM);
    return gps;
}

void RNAVProcedureTests::setupRouteManager()
{
    auto rm = globals->add_new_subsystem<FGRouteMgr>();
    rm->bind();
    rm->init();
    rm->postinit();
}

void RNAVProcedureTests::setPositionAndStabilise(const SGGeod& g)
{
    FGTestApi::setPosition(g);
    for (int i=0; i<60; ++i) {
        m_gps->update(0.015);
    }
}

/////////////////////////////////////////////////////////////////////////////

#if 0
void RNAVProcedureTests::testBasic()
{
    setupStandardGPS();
    
    FGPositioned::TypeFilter f{FGPositioned::VOR};
    auto bodrumVOR = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("BDR", SGGeod::fromDeg(27.6, 37), &f));
    SGGeod p1 = SGGeodesy::direct(bodrumVOR->geod(), 45.0, 5.0 * SG_NM_TO_METER);
    
    FGTestApi::setPositionAndStabilise(p1);
    

}
#endif

void RNAVProcedureTests::testHeadingToAlt()
{
    auto vhhh = FGAirport::findByIdent("VHHH");
//    FGTestApi::setUp::logPositionToKML("heading_to_alt");

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    fp->addDelegate(testDelegate);

    rm->setFlightPlan(fp);
    
    // we don't have Nasal, but our delegate does the same work
    FGTestApi::setUp::populateFPWithNasal(fp, "VHHH", "25R", "EGLL", "27R", "HAZEL");
    
    auto wp = new HeadingToAltitude(fp, "TO_4000", 270);
    wp->setAltitude(4000, RESTRICT_ABOVE);
    fp->insertWayptAtIndex(wp, 1); // between the runway WP and HAZEL

  //  FGTestApi::writeFlightPlanToKML(fp);

    auto depRwy = fp->departureRunway();
    
    setupStandardGPS();
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));

    fp->activate();
    m_gpsNode->setStringValue("command", "leg");

    CPPUNIT_ASSERT_EQUAL(fp->currentIndex(), 0);

    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(depRwy->pointOnCenterline(0.0));
    pilot->setCourseTrue(depRwy->headingDeg()); 
    pilot->setSpeedKts(200);
    pilot->flyGPSCourse(m_gps);
    
    CPPUNIT_ASSERT_EQUAL(fp->currentIndex(), 0);

    // check we sequence to the heading-to-alt wp
    bool ok = FGTestApi::runForTimeWithCheck(300.0, [fp] () {
      if (fp->currentIndex() == 1) {
          return true;
      }
      return false;
    });
    CPPUNIT_ASSERT(ok);

    // leisurely climb out
    pilot->setVerticalFPM(1800);
    pilot->setTargetAltitudeFtMSL(8000);

    CPPUNIT_ASSERT_EQUAL(std::string{"VHHH-25R"}, std::string{m_gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"TO_4000"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(270.0, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(270.0, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // fly until we're turned to to heading
    ok = FGTestApi::runForTimeWithCheck(20, [pilot] () {
        return pilot->isOnHeading(270.);
    });
    CPPUNIT_ASSERT(ok);
    
    // capture the position now
    SGGeod posAtHdgAltStart = globals->get_aircraft_position();
    FGTestApi::runForTime(40.0);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(270.0, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(270.0, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    const double crs = SGGeodesy::courseDeg(posAtHdgAltStart, globals->get_aircraft_position());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(270.0, crs, 1.0);
    
    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        return (fp->currentIndex() == 2);
    });
    CPPUNIT_ASSERT(ok);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4000.0, globals->get_aircraft_position().getElevationFt(), 100.0);

     FGTestApi::runForTime(40.0);
}

// ugly version: the heading to hold will be very mis-aligned with
// the course when the leg is sequenced. (more than 90 degrees turn(
void RNAVProcedureTests::testUglyHeadingToAlt()
{
    auto vhhh = FGAirport::findByIdent("VHHH");
  //  FGTestApi::setUp::logPositionToKML("heading_to_alt_ugly");

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    fp->addDelegate(testDelegate);

    rm->setFlightPlan(fp);
    
    // we don't have Nasal, but our delegate does the same work
    FGTestApi::setUp::populateFPWithNasal(fp, "VHHH", "07L", "EGLL", "27R", "HAZEL");
    
    auto wp = new HeadingToAltitude(fp, "TO_4000", 210);
    wp->setAltitude(4000, RESTRICT_ABOVE);
    fp->insertWayptAtIndex(wp, 1); // between the runway WP and HAZEL

  //  FGTestApi::writeFlightPlanToKML(fp);

    auto depRwy = fp->departureRunway();
    
    setupStandardGPS();
    setPositionAndStabilise(fp->departureRunway()->pointOnCenterline(0.0));

    fp->activate();
    m_gpsNode->setStringValue("command", "leg");

    CPPUNIT_ASSERT_EQUAL(fp->currentIndex(), 0);

    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(depRwy->pointOnCenterline(0.0));
    pilot->setCourseTrue(depRwy->headingDeg());
    pilot->setSpeedKts(200);
    pilot->flyGPSCourse(m_gps);
    
    // check we sequence to the heading-to-alt wp
    bool ok = FGTestApi::runForTimeWithCheck(240.0, [fp] () {
      if (fp->currentIndex() == 1) {
          return true;
      }
      return false;
    });
    CPPUNIT_ASSERT(ok);

    // leisurely climb out
    pilot->setVerticalFPM(1800);
    pilot->setTargetAltitudeFtMSL(8000);

    CPPUNIT_ASSERT_EQUAL(std::string{"VHHH-07L"}, std::string{m_gpsNode->getStringValue("wp/wp[0]/ID")});
    CPPUNIT_ASSERT_EQUAL(std::string{"TO_4000"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(210.0, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(210.0, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    // fly until we're turned to to heading
    ok = FGTestApi::runForTimeWithCheck(120, [pilot] () {
        return pilot->isOnHeading(210.0);
    });
    CPPUNIT_ASSERT(ok);
    
    // capture the position now
    SGGeod posAtHdgAltStart = globals->get_aircraft_position();
    FGTestApi::runForTime(40.0);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(210.0, m_gpsNode->getDoubleValue("wp/wp[1]/bearing-true-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(210.0, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-deviation-deg"), 0.5);
    
    const double crs = SGGeodesy::courseDeg(posAtHdgAltStart, globals->get_aircraft_position());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(210.0, crs, 1.0);
    
    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        return (fp->currentIndex() == 2);
    });
    CPPUNIT_ASSERT(ok);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4000.0, globals->get_aircraft_position().getElevationFt(), 100.0);

     FGTestApi::runForTime(40.0);
}


void RNAVProcedureTests::testEGPH_TLA6C()
{
    
    auto egph = FGAirport::findByIdent("EGPH");
    
    auto sid = egph->findSIDWithIdent("TLA6C");
    // procedures not loaded, abandon test
    if (!sid)
        return;
    
   // FGTestApi::setUp::logPositionToKML("procedure_egph_tla6c");

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    fp->addDelegate(testDelegate);
    
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithNasal(fp, "EGPH", "24", "EGLL", "27R", "DCS POL DTY");
    
    fp->setSID(sid);
    
    FGRunwayRef departureRunway = fp->departureRunway();
    CPPUNIT_ASSERT_EQUAL(std::string{"24"}, fp->legAtIndex(0)->waypoint()->source()->name());
    
    CPPUNIT_ASSERT_EQUAL(std::string{"UW"}, fp->legAtIndex(1)->waypoint()->ident());
    
    auto d242Wpt =  fp->legAtIndex(2)->waypoint();
    CPPUNIT_ASSERT_EQUAL(std::string{"D242H"}, d242Wpt->ident());
    CPPUNIT_ASSERT_EQUAL(true, d242Wpt->flag(WPT_OVERFLIGHT));
    
    const auto wp3Ident = fp->legAtIndex(3)->waypoint()->ident();
    
    // depeding which versino fo the procedures we loaded, we can find
    // one ID or the other
    CPPUNIT_ASSERT((wp3Ident == "D346T") || (wp3Ident == "D345T"));
    
  //  FGTestApi::writeFlightPlanToKML(fp);

    CPPUNIT_ASSERT(rm->activate());
   
    
    setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(departureRunway->threshold());
    m_gpsNode->setStringValue("command", "leg");

    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(globals->get_aircraft_position());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(departureRunway->headingDeg(), m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    pilot->setCourseTrue(m_gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    pilot->setSpeedKts(220);
    pilot->flyGPSCourse(m_gps);
    
    FGTestApi::runForTime(20.0);
    // check we're somewhere along the runway, on the centerline
    // and still on waypoint zero
    
    bool ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 1) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    // check what we sequenced to
    double elapsed = globals->get_sim_time_sec();
    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 2) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);

    elapsed = globals->get_sim_time_sec();
    
    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 3) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);

    elapsed = globals->get_sim_time_sec();
    
    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 4) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, m_gpsNode->getDoubleValue("wp/wp[1]/course-error-nm"), 0.05);

    ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 5) {
            return true;
        }
        return false;
    });
    
    CPPUNIT_ASSERT(ok);

    CPPUNIT_ASSERT_EQUAL(std::string{"TLA"}, fp->legAtIndex(5)->waypoint()->ident());
    CPPUNIT_ASSERT_EQUAL(std::string{"TLA"}, std::string{m_gpsNode->getStringValue("wp/wp[1]/ID")});
}

void RNAVProcedureTests::testLFKC_AJO1R()
{
    auto lfkc = FGAirport::findByIdent("LFKC");
    auto sid = lfkc->findSIDWithIdent("AJO1R");
    // procedures not loaded, abandon test
    if (!sid)
        return;
    
  //  FGTestApi::setUp::logPositionToKML("procedure_LFKC_AJO1R");
    
    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    fp->addDelegate(testDelegate);
    
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithNasal(fp, "LFKC", "36", "EGLL", "27R", "");
    
    fp->setSID(sid);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"BEBEV"}, fp->legAtIndex(4)->waypoint()->ident());
    CPPUNIT_ASSERT_EQUAL(std::string{"AJO"}, fp->legAtIndex(5)->waypoint()->ident());
    double d = fp->legAtIndex(5)->distanceAlongRoute();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(72, d, 1.0); // ensure the route didn't blow up to 0,0
    
    FGRunwayRef departureRunway = fp->departureRunway();
   // FGTestApi::writeFlightPlanToKML(fp);
    
    CPPUNIT_ASSERT(rm->activate());
    
    setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(departureRunway->threshold());
    m_gpsNode->setStringValue("command", "leg");
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(globals->get_aircraft_position());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(departureRunway->headingDeg(), m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    pilot->setCourseTrue(m_gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    pilot->setSpeedKts(220);
    pilot->flyGPSCourse(m_gps);
    
    FGTestApi::runForTime(20.0);
    // check we're somewhere along the runway, on the centerline
    // and still on waypoint zero
    
    bool ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 1) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
}

void RNAVProcedureTests::testTransitionsSID()
{
    auto kjfk = FGAirport::findByIdent("kjfk");
    auto runway = kjfk->getRunwayByIdent("13L");
    

    auto sid = kjfk->selectSIDByTransition(runway, "CANDR");
    // procedures not loaded, abandon test
    if (!sid)
        return;

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    fp->addDelegate(testDelegate);
    
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithNasal(fp, "KJFK", "13L", "KCLE", "24R", "");
    
    fp->setSID(sid);
    CPPUNIT_ASSERT_EQUAL(8, fp->numLegs());
    auto wp = fp->legAtIndex(6);
    CPPUNIT_ASSERT_EQUAL(std::string{"CANDR"}, wp->waypoint()->ident());
    CPPUNIT_ASSERT(rm->activate());
}

void RNAVProcedureTests::testTransitionsSTAR()
{
    auto kjfk = FGAirport::findByIdent("kjfk");
    auto runway = kjfk->getRunwayByIdent("22L");
    auto star = kjfk->selectSTARByTransition(runway, "SEY");
    // procedures not loaded, abandon test
    if (!star)
        return;

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    fp->addDelegate(testDelegate);
    
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithNasal(fp, "KBOS", "22R", "KJFK", "22L", "");
    
    fp->setSTAR(star);
    CPPUNIT_ASSERT_EQUAL(9, fp->numLegs());
    auto wp = fp->legAtIndex(1);
    CPPUNIT_ASSERT_EQUAL(std::string{"SEY"}, wp->waypoint()->ident());
    CPPUNIT_ASSERT(rm->activate());
}

void RNAVProcedureTests::testLEBL_LARP2F()
{
    auto lebl = FGAirport::findByIdent("LEBL");
    auto sid = lebl->findSIDWithIdent("LARP1F.25L");
    // procedures not loaded, abandon test
    if (!sid)
        return;

    FGTestApi::setUp::logPositionToKML("procedure_LEBL-LARP2F");

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    fp->addDelegate(testDelegate);
    
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithNasal(fp, "LEBL", "25L", "LEIB", "06", "");
    
    fp->setSID(sid);
    // I don't know if this should pass or not! 
    // If its a bug, wrap next line in CPPUNIT_ASSERT_ASSERTION_FAIL
    CPPUNIT_ASSERT_EQUAL(fp->legAtIndex(1)->waypoint()->position(), SGGeod::fromDeg(0, 0)); 
    CPPUNIT_ASSERT_EQUAL(std::string{"LARPA"}, fp->legAtIndex(8)->waypoint()->ident());
    double d = fp->legAtIndex(8)->distanceAlongRoute();
    CPPUNIT_ASSERT_DOUBLES_EQUAL(46.6, d, 1.0); // ensure the route didn't blow up
    
    FGTestApi::writeFlightPlanToKML(fp);

    CPPUNIT_ASSERT(rm->activate());
    
    FGRunwayRef departureRunway = fp->departureRunway();
    
    setupStandardGPS();
    
    FGTestApi::setPositionAndStabilise(departureRunway->threshold());
    m_gpsNode->setStringValue("command", "leg");
    
    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->resetAtPosition(globals->get_aircraft_position());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(departureRunway->headingDeg(), m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
    pilot->setCourseTrue(m_gpsNode->getDoubleValue("wp/leg-true-course-deg"));
    pilot->setSpeedKts(220);
    pilot->flyGPSCourse(m_gps);
    pilot->setTargetAltitudeFtMSL(8000);
    pilot->setVerticalFPM(1800);
    FGTestApi::runForTime(20.0);
    bool ok = FGTestApi::runForTimeWithCheck(180.0, [fp] () {
        if (fp->currentIndex() == 1) {
            return true;
        }
        return false;
    });
    CPPUNIT_ASSERT(ok);
    
    FGTestApi::runForTime(180.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(199, m_gpsNode->getDoubleValue("wp/leg-true-course-deg"), 0.5);
}

// This could probably be in a better place but this allows it to access TestDelegate.
// Also it does relate to procedures as its a bug that only occurs with procedures
void RNAVProcedureTests::testIndexOf()
{
	auto egkk = FGAirport::findByIdent("EGKK");
    auto sid = egkk->findSIDWithIdent("SAM3P");
    // procedures not loaded, abandon test
    if (!sid)
        return;

    auto rm = globals->get_subsystem<FGRouteMgr>();
    auto fp = new FlightPlan;
    
    auto testDelegate = new TestFPDelegate;
    testDelegate->thePlan = fp;
    fp->addDelegate(testDelegate);
    
    rm->setFlightPlan(fp);
    FGTestApi::setUp::populateFPWithNasal(fp, "EGKK", "08R", "EGJJ", "27", "LELNA");
    
    fp->setSID(sid);
	FGPositioned::TypeFilter f{FGPositioned::VOR};
    auto southamptonVOR = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("SAM", SGGeod::fromDeg(-1.25, 51.0), &f));
	auto SAM = fp->legAtIndex(6)->waypoint();
	CPPUNIT_ASSERT_EQUAL(southamptonVOR->ident(), SAM->ident());
	CPPUNIT_ASSERT_EQUAL(6, fp->findWayptIndex(southamptonVOR));
}
