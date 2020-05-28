#include "test_navRadio.hxx"

#include <memory>
#include <cstring>

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"

#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Navaids/navlist.hxx>

#include <Instrumentation/navradio.hxx>

// Set up function for each test.
void NavRadioTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("navradio");
    FGTestApi::setUp::initNavDataCache();
}


// Clean up after each test.
void NavRadioTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void NavRadioTests::setPositionAndStabilise(FGNavRadio* r, const SGGeod& g)
{
    FGTestApi::setPosition(g);
    for (int i=0; i<60; ++i) {
        r->update(0.1);
    }
}

void NavRadioTests::testBasic()
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "navtest");
    configNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(configNode));
    
    r->bind();
    r->init();
    
    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/navtest[2]");
    node->setBoolValue("serviceable", true);
    // needed for the radio to power up
    globals->get_props()->setDoubleValue("systems/electrical/outputs/nav", 6.0);
    node->setDoubleValue("frequencies/selected-mhz", 113.8);

    SGGeod pos = SGGeod::fromDegFt(-3.352780, 55.499199, 20000);
    setPositionAndStabilise(r.get(), pos);
    
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("operable"));
    CPPUNIT_ASSERT(!strcmp("TLA", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("in-range"));
}

void NavRadioTests::testCDIDeflection()
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "navtest");
    configNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(configNode));
    
    r->bind();
    r->init();
    
    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/navtest[2]");
    node->setBoolValue("serviceable", true);
    // needed for the radio to power up
    globals->get_props()->setDoubleValue("systems/electrical/outputs/nav", 6.0);
    node->setDoubleValue("frequencies/selected-mhz", 113.55);
    
    node->setDoubleValue("radials/selected-deg", 25);
    
    FGPositioned::TypeFilter f{FGPositioned::VOR};
    FGNavRecordRef nav = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent("MCT", SGGeod::fromDeg(-2.26, 53.3), &f));

    // twist of MCT is 5.0, so we use a bearing of 20 here, not 25
    SGGeod posOnRadial = SGGeodesy::direct(nav->geod(), 20.0, 10 * SG_NM_TO_METER);
    posOnRadial.setElevationFt(10000);
    setPositionAndStabilise(r.get(), posOnRadial);

    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("operable"));
    CPPUNIT_ASSERT(!strcmp("MCT", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("crosstrack-error-m"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(!node->getBoolValue("to-flag"));

    
    // move off course
    SGGeod posOffRadial = SGGeodesy::direct(nav->geod(), 15.0, 20 * SG_NM_TO_METER); // 5 degress off
    posOffRadial.setElevationFt(12000);
    setPositionAndStabilise(r.get(), posOffRadial);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.5, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    
    double xtkE = sin(5.0 * SG_DEGREES_TO_RADIANS) * 20 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 50.0);
    CPPUNIT_ASSERT(node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(!node->getBoolValue("to-flag"));
    
    posOffRadial = SGGeodesy::direct(nav->geod(), 28.0, 30 * SG_NM_TO_METER); // 8 degress off
    posOffRadial.setElevationFt(16000);
    setPositionAndStabilise(r.get(), posOffRadial);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-8.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.8, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    
    xtkE = sin(-8.0 * SG_DEGREES_TO_RADIANS) * 30 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 50.0);

    // move more than ten degrees off course
    posOffRadial = SGGeodesy::direct(nav->geod(), 33.0, 40 * SG_NM_TO_METER); // 13 degress off
    posOffRadial.setElevationFt(16000);
    setPositionAndStabilise(r.get(), posOffRadial);
    
    // pegged to full deflection
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-10.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.0, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    
    // cross track error is computed based on true deflection, not clamped
    xtkE = sin(-13.0 * SG_DEGREES_TO_RADIANS) * 40 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 50.0);
    CPPUNIT_ASSERT(node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(!node->getBoolValue("to-flag"));
    
// try on the TO side of the station
    // let's use Perth VOR, but the Australian one to check southern
    // hemisphere operation
    node->setDoubleValue("frequencies/selected-mhz", 113.7);
    node->setDoubleValue("radials/selected-deg", 42.0); // twist is -2.0
    CPPUNIT_ASSERT(!strcmp("113.70", node->getStringValue("frequencies/selected-mhz-fmt")));

    auto perthVOR = fgpositioned_cast<FGNavRecord>(
        FGPositioned::findClosestWithIdent("PH", SGGeod::fromDeg(115.95, -31.9), &f));
    
    SGGeod p = SGGeodesy::direct(perthVOR->geod(), 220.0, 20 * SG_NM_TO_METER); // on the reciprocal radial
    p.setElevationFt(12000);
    setPositionAndStabilise(r.get(), p);
    
    CPPUNIT_ASSERT(!strcmp("PH", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(40.0, node->getDoubleValue("heading-deg"), 0.5);
    
    // actual radial has twist subtracted
    CPPUNIT_ASSERT_DOUBLES_EQUAL(222.0, node->getDoubleValue("radials/actual-deg"), 0.01);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("crosstrack-error-m"), 50.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));
    
// off course on the TO side
    p = SGGeodesy::direct(perthVOR->geod(), 227.0, 100 * SG_NM_TO_METER);
    p.setElevationFt(18000);
    setPositionAndStabilise(r.get(), p);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(47.0, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(229.0, node->getDoubleValue("radials/actual-deg"), 0.01);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(7.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.7, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    
    xtkE = sin(7.0 * SG_DEGREES_TO_RADIANS) * 100 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 50.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));
}

void NavRadioTests::testILSBasic()
{
    // radio setup
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "navtest");
    configNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(configNode));
    r->bind();
    r->init();
    
    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/navtest[2]");
    node->setBoolValue("serviceable", true);
    globals->get_props()->setDoubleValue("systems/electrical/outputs/nav", 6.0);
    
    // test basic ILS: KSFO 28L
    FGPositioned::TypeFilter f{{FGPositioned::VOR, FGPositioned::ILS, FGPositioned::LOC}};
    FGNavRecordRef ils = fgpositioned_cast<FGNavRecord>(
        FGPositioned::findClosestWithIdent("ISFO", SGGeod::fromDeg(-112, 37.6), &f));
    CPPUNIT_ASSERT(ils->type() == FGPositioned::ILS);
    
    node->setDoubleValue("frequencies/selected-mhz", 109.55);
   // node->setDoubleValue("radials/selected-deg", 42.0); // twist is -2.0
    CPPUNIT_ASSERT(!strcmp("109.55", node->getStringValue("frequencies/selected-mhz-fmt")));

    // note we need full precision here, due to ILS sensitivity
    SGGeod p = SGGeodesy::direct(ils->geod(), 117.932, 10 * SG_NM_TO_METER);
    p.setElevationFt(2500);
    setPositionAndStabilise(r.get(), p);
    
    CPPUNIT_ASSERT(!strcmp("ISFO", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(297.9, node->getDoubleValue("radials/target-radial-deg"), 0.1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(297.9, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(117.932, node->getDoubleValue("radials/actual-deg"), 0.1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("heading-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("crosstrack-error-m"), 10.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));
    
    //  1 degree offset
    p = SGGeodesy::direct(ils->geod(), 116.932, 6 * SG_NM_TO_METER);
    p.setElevationFt(1500);
    setPositionAndStabilise(r.get(), p);
    
    const double locWidth = ils->localizerWidth();
    const double deflectionScale = 20.0 / locWidth; // 20 degrees is full VOR swing (-10 to +10 degrees)
    
    CPPUNIT_ASSERT(!strcmp("ISFO", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(297.9, node->getDoubleValue("radials/target-radial-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(296.9, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(116.932, node->getDoubleValue("radials/actual-deg"), 0.1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.0 * deflectionScale, node->getDoubleValue("heading-needle-deflection"), 0.1);
    
    double xtkE = sin(-1.0 * SG_DEGREES_TO_RADIANS) * 6.0 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 1.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));
    
    
    // test pegged (4 degrees off course)
    p = SGGeodesy::direct(ils->geod(), 121.932, 3 * SG_NM_TO_METER);
    p.setElevationFt(600);
    setPositionAndStabilise(r.get(), p);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(301.9, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(121.932, node->getDoubleValue("radials/actual-deg"), 0.1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(10.0, node->getDoubleValue("heading-needle-deflection"), 0.1);
    
    xtkE = sin(4.0 * SG_DEGREES_TO_RADIANS) * 3.0 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 1.0);
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));
    
    
    // also check ILS back course
    // 1 degree offset on the BC
    p = SGGeodesy::direct(ils->geod(), 298.932, 4 * SG_NM_TO_METER);
    p.setElevationFt(1500);
    setPositionAndStabilise(r.get(), p);

    CPPUNIT_ASSERT(!strcmp("ISFO", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(297.9, node->getDoubleValue("radials/target-radial-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(118.9, node->getDoubleValue("heading-deg"), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(298.932, node->getDoubleValue("radials/actual-deg"), 0.1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.0 * deflectionScale, node->getDoubleValue("heading-needle-deflection"), 0.1);
    
    xtkE = sin(-1.0 * SG_DEGREES_TO_RADIANS) * 4.0 * SG_NM_TO_METER;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(xtkE, node->getDoubleValue("crosstrack-error-m"), 1.0);
    
    // these don't change for an ILS
    CPPUNIT_ASSERT(!node->getBoolValue("from-flag"));
    CPPUNIT_ASSERT(node->getBoolValue("to-flag"));
}



void NavRadioTests::testGS()
{
    // radio setup
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "navtest");
    configNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(configNode));
    r->bind();
    r->init();
    
    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/navtest[2]");
    node->setBoolValue("serviceable", true);
    globals->get_props()->setDoubleValue("systems/electrical/outputs/nav", 6.0);
    
    // EDDT 28R
    FGPositioned::TypeFilter f{FGPositioned::GS};
    FGNavRecordRef gs = fgpositioned_cast<FGNavRecord>(
                                                        FGPositioned::findClosestWithIdent("ITLW", SGGeod::fromDeg(13, 52), &f));
    CPPUNIT_ASSERT(gs->type() == FGPositioned::GS);
    node->setDoubleValue("frequencies/selected-mhz", 110.10);
    CPPUNIT_ASSERT(!strcmp("110.10", node->getStringValue("frequencies/selected-mhz-fmt")));
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(gs->glideSlopeAngleDeg(), 3.0, 0.001);
    double gsAngleRad = gs->glideSlopeAngleDeg() * SG_DEGREES_TO_RADIANS;
    
    /////////////
    // derive the GS geometry in cartesian vectors, to match what
    // navradio.cxx does
    SGGeod aboveGS = gs->geod();
    aboveGS.setElevationM(gs->geod().getElevationM() + 100.0);
    SGVec3d gsVerticalAxis = SGVec3d::fromGeod(aboveGS) - gs->cart();
    // intentionally different approach to what navradio uses
    
    gsVerticalAxis *= 0.01; // make it per meter, since we used 100m above
    
    // dervice the baseline
    SGQuatd baseLineRot = SGQuatd::fromLonLat(gs->geod()) * SGQuatd::fromHeadAttBankDeg(80.828, 0, 0);
    SGVec3d gsAltAxis = baseLineRot.backTransform(SGVec3d(1.0, 0.0, 0.0));
  
    const SGVec3d gsCart = gs->cart();
    
   //////////////////
    
    SGVec3d radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 8 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 8 * SG_NM_TO_METER);

    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));
    
    CPPUNIT_ASSERT(!strcmp("ITLW", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.0, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));
    
// 0.5 degree offset above
    gsAngleRad = (gs->glideSlopeAngleDeg() + 0.5) * SG_DEGREES_TO_RADIANS;
    radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 4 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 4 * SG_NM_TO_METER);
    
    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));
    
    CPPUNIT_ASSERT(!strcmp("ITLW", node->getStringValue("nav-id")));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.5, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-2.5, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.714, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));

// 1 degree below (danger!)
    gsAngleRad = (gs->glideSlopeAngleDeg() - 1.0) * SG_DEGREES_TO_RADIANS;
    radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 2 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 2 * SG_NM_TO_METER);
    
    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.5, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));
    
// false course above, reversed
    gsAngleRad = (gs->glideSlopeAngleDeg() + 3.0) * SG_DEGREES_TO_RADIANS;
    radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 5 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 5 * SG_NM_TO_METER);
    
    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(6.0, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));
    
    // false course above, reversed, 0.35 offset below
    gsAngleRad = (gs->glideSlopeAngleDeg() + 2.65) * SG_DEGREES_TO_RADIANS;
    radioPos = gsCart;
    radioPos += (gsVerticalAxis * tan(gsAngleRad) * 3 * SG_NM_TO_METER);
    radioPos += (gsAltAxis * 3 * SG_NM_TO_METER);
    
    setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.65, node->getDoubleValue("gs-direct-deg"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1.75, node->getDoubleValue("gs-needle-deflection"), 0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-0.5, node->getDoubleValue("gs-needle-deflection-norm"), 0.01);
    CPPUNIT_ASSERT(node->getBoolValue("gs-in-range"));
}

void NavRadioTests::testILSFalseCourse()
{
    
    // also GS false lobes
}

void NavRadioTests::testILSPaired()
{
    // EGPH and countless more
}

void NavRadioTests::testILSAdjacentPaired()
{
    // eg KJFK
}

void NavRadioTests::testGlideslopeLongDistance()
{
    // radio setup
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "navtest");
    configNode->setIntValue("number", 2);
    std::unique_ptr<FGNavRadio> r(new FGNavRadio(configNode));
    r->bind();
    r->init();

    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/navtest[2]");
    node->setBoolValue("serviceable", true);
    globals->get_props()->setDoubleValue("systems/electrical/outputs/nav", 6.0);

    // EGLL 27L
    FGPositioned::TypeFilter f{FGPositioned::GS};
    FGNavRecordRef gs = fgpositioned_cast<FGNavRecord>(
        FGPositioned::findClosestWithIdent("ILL", SGGeod::fromDeg(0, 51), &f));
    CPPUNIT_ASSERT(gs->type() == FGPositioned::GS);
    node->setDoubleValue("frequencies/selected-mhz", 109.50);
    CPPUNIT_ASSERT(!strcmp("109.50", node->getStringValue("frequencies/selected-mhz-fmt")));

    CPPUNIT_ASSERT_DOUBLES_EQUAL(gs->glideSlopeAngleDeg(), 3.0, 0.001);
    double gsAngleRad = gs->glideSlopeAngleDeg() * SG_DEGREES_TO_RADIANS;

    // standard approach (per charts)
    SGGeod p = SGGeodesy::direct(gs->geod(), 90, 7.5 * SG_NM_TO_METER);
    p.setElevationFt(2500);
    setPositionAndStabilise(r.get(), p);
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("gs-in-range"));

    // normal approach
    p = SGGeodesy::direct(gs->geod(), 90, 9 * SG_NM_TO_METER);
    p.setElevationFt(3000);
    setPositionAndStabilise(r.get(), p);
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("gs-in-range"));

    // in our current nav data, the GS range is defined as 10nm, so the gs-in-range
    // is false for these

    // 4000 feet intercept
    p = SGGeodesy::direct(gs->geod(), 90, 12 * SG_NM_TO_METER);
    p.setElevationFt(4000);
    setPositionAndStabilise(r.get(), p);
    CPPUNIT_ASSERT_EQUAL(false, node->getBoolValue("gs-in-range"));
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("in-range"));

    // further back
    p = SGGeodesy::direct(gs->geod(), 90, 17.5 * SG_NM_TO_METER);
    p.setElevationFt(4000);
    setPositionAndStabilise(r.get(), p);
    CPPUNIT_ASSERT_EQUAL(false, node->getBoolValue("gs-in-range"));
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("in-range"));

    // really pushing it
    p = SGGeodesy::direct(gs->geod(), 90, 25 * SG_NM_TO_METER);
    p.setElevationFt(4000);
    setPositionAndStabilise(r.get(), p);
    CPPUNIT_ASSERT_EQUAL(false, node->getBoolValue("gs-in-range"));
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("in-range"));
}
