#include "test_dme.hxx"

#include <cstring>
#include <memory>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Airports/airport.hxx>
#include <Navaids/NavDataCache.hxx>
#include <Navaids/navlist.hxx>
#include <Navaids/navrecord.hxx>

#include <Instrumentation/dme.hxx>

#include <Main/fg_props.hxx>

// Set up function for each test.
void DMEReceiverTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("navradio");
    FGTestApi::setUp::initNavDataCache();
}


// Clean up after each test.
void DMEReceiverTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void DMEReceiverTests::setPositionAndStabilise(DME* r, const SGGeod& g)
{
    FGTestApi::setPosition(g);
    for (int i = 0; i < 60; ++i) {
        r->update(0.1);
    }
}

SGSharedPtr<DME> DMEReceiverTests::setupStandardDME()
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "dmetest");
    configNode->setIntValue("number", 2);

    return new DME(configNode);
}

void DMEReceiverTests::testBasic()
{
    SGSharedPtr<DME> r = setupStandardDME();

    // set a source string pointing at a fictious nav-receiver
    fgSetString("/instrumentation/dmetest[2]/frequencies/source",
                "/instrumentation/nav[0]/frequencies/selected-mhz");

    r->bind();
    r->init();
    globals->get_subsystem_mgr()->add("dme", r.get());

    auto arlanda = fgFindAirportID("ESSA");

    // set the nav frequency
    fgSetDouble("/instrumentation/nav[0]/frequencies/selected-mhz", 113.30);


    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/dmetest[2]");
    node->setBoolValue("serviceable", true);

    fgSetDouble("systems/electrical/outputs/dme", 12.0);

    setPositionAndStabilise(r.get(), arlanda->geod());


    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("in-range"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.4, node->getDoubleValue("indicated-distance-nm"), 0.1);

    // fly towards the station at constant speed

    auto pilot = SGSharedPtr<FGTestApi::TestPilot>(new FGTestApi::TestPilot);
    pilot->setSpeedKts(150);


    FGPositioned::TypeFilter f{FGPositioned::DME};
    FGNavRecordRef arlandaDME = fgpositioned_cast<FGNavRecord>(
        FGPositioned::findClosestWithIdent("ANE", arlanda->geod(), &f));

    const double trueCourseToANE = SGGeodesy::courseDeg(arlanda->geod(), arlandaDME->geod());
    pilot->setCourseTrue(trueCourseToANE);
    FGTestApi::runForTime(30.0);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(150, node->getDoubleValue("indicated-ground-speed-kt"), 0.5);
    // should have travelled (150 / 3600 * 30 ) = 1.25nm
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.15, node->getDoubleValue("indicated-distance-nm"), 0.1);
}

void DMEReceiverTests::testRCFN_04DME()
{
    // disabled pending discussion about the data for this one
    return;
    
    auto rcfn = fgFindAirportID("RCFN");

    auto dmeReceiver = setupStandardDME();

    FGRunwayRef rwy04 = rcfn->getRunwayByIdent("04");

    FGPositioned::TypeFilter filter(FGPositioned::DME);
    auto matches = FGPositioned::findAllWithIdent("IFNN", &filter);
    FGPositioned::sortByRange(matches, rcfn->geod());

    CPPUNIT_ASSERT(!matches.empty());
    // should be size two, really

    auto station = fgpositioned_cast<FGNavRecord>(matches.front());
    CPPUNIT_ASSERT(station);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(110.9, station->get_freq(), 0.01);
}
