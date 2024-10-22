#include "test_navRadio.hxx"

#include <algorithm>
#include <memory>
#include <cmath>
#include <cstring>

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"

#include <Main/fg_props.hxx>
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

std::string NavRadioTests::formatFrequency(double f)
{
    char buf[16];
    ::snprintf(buf, 16, "%3.2f", f);
    return buf;
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
    CPPUNIT_ASSERT(node->getStringValue("nav-id") == "TLA");
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("in-range"));
}

static const struct  {
  int    nvType; 
  double nvLat, nvLon, nvAlt, nvFreq, nvRnge, nvTwst;
  const std::string nvIden;
  double onRose, atDstNM, atAltFt, atHdg, rSele; 
  bool   vOpnl, vToFlag;
  double vSigNorm, vSigTolr, vHdgDefl, vDeflTolr, vHdgNorm, vDefnTolr, xtkTolr;
  const std::string tDesc;
}  CDITestRoll[] = {  
  //
  //  Test Items: Add test cases here:
  //  nv<=      fields are copied direct from nav dat  => <= Rx pos wrt navaid, Radial =><=        v- Values expected / tested            => <= Line / Desc for Mesg =>  
  //Type Lat      Lon    Alt   Freq   Rnge  Twist   Iden] onRose atNm  atAlt atHdg rSele Op To sigN - Tolr   Defl - Tolr  DNrm -Tolr xtkTolr
  //
  { 3,  53.3,    -2.26,  282, 113.55, 130, -5.0,     "MCT",  25,  10.0,  4000, 200,  25,  1, 0, 1.0,  0.01,   0.0,  9.01,  0.0, 0.01,  50.0, "1: MCT EGCC On Radial"  },
  { 3,  53.3,    -2.26,  282, 113.55, 130, -5.0,     "MCT",  25,  10.0,  4000, 200,  25,  1, 0, 1.0,  0.01,   0.0,  9.01,  0.0, 0.01,  50.0, "2: MCT EGCC On Again "  },
  { 3,  53.3,    -2.26,  282, 113.55, 130, -5.0,     "MCT",  20,  20.0, 12000,  20,  25,  1, 0, 1.0,  0.01,   5.0,  0.1,   0.5, 0.01,  50.0, "3: MCT 5deg Off radial" },
  { 3,  53.3,    -2.26,  282, 113.55, 130, -5.0,     "MCT",  33,  30.0, 16000, 100,  25,  1, 0, 1.0,  0.01,  -8.0,  0.1,  -0.8, 0.01,  50.0, "4: MCT 8deg Off radial" },
  { 3,  53.3,    -2.26,  282, 113.55, 130, -5.0,     "MCT",  38,  40.0, 16000, 280,  25,  1, 0, 1.0,  0.01, -10.0,  0.1,  -1.0, 0.01,  50.0, "5: MCT >10  Off radial" },
  { 3, -31.9,   115.95,   87, 113.70, 130, -2.0,      "PH", 222,  20.0, 12000, 220,  42,  1, 1, 1.0,  0.01,   0.0,  0.01,  0.0, 0.01,  50.0, "6: PH Perth W.Aus On Radial"},
  { 3, -31.9,   115.95,   87, 113.70, 130, -2.0,      "PH", 225,  20.0, 18000, 220,  42,  1, 1, 1.0,  0.01,   3.0,  0.01,  0.3, 0.01,  50.0, "7: PH +3deg Off radial" } 
};   

void NavRadioTests::callNavRadioCDI() {
    //
    //2021Ja15 set flag for newnavradio
    //
    fgSetBool("/instrumentation/use-new-navradio", true);
    // setup
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
    //
    int tale = sizeof(CDITestRoll) / sizeof(CDITestRoll[0]);
    for (int i = 0; (i < tale); i++) {
        // prep error message
        const std::string itemDesc = "   navradioCDI Item " + CDITestRoll[i].tDesc + " @ ";
        // Txmitting navaid
        node->setDoubleValue("frequencies/selected-mhz", CDITestRoll[i].nvFreq);
        node->setDoubleValue("radials/selected-deg", CDITestRoll[i].rSele);
        // tbd Filter on type as defined in nav dat
        //FGPositioned::TypeFilter f{FGPositioned::VOR};
        FGPositioned::TypeFilter f{{FGPositioned::VOR, FGPositioned::ILS, FGPositioned::LOC}};
        FGNavRecordRef nav = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent(CDITestRoll[i].nvIden,
                                                                                               SGGeod::fromDeg(CDITestRoll[i].nvLon, CDITestRoll[i].nvLat), &f));
        //
        // For VOR nav dat field 7: 'Twist' == Easterly rotation of Txmitter's 360 wrt True North  c.f for Compass: 'Deviation West Rose is Best'
        // Rx posn is specified according to navaid's radials as printed on chart: True Bng = ( Radial on Rose  +  Twist ( Deviation ))
        //                           ( ftr: Both MCT -5  and PH -2  Are  Negative Twists  )
        SGGeod posWrtRadial = SGGeodesy::direct(nav->geod(), (CDITestRoll[i].onRose + CDITestRoll[i].nvTwst),
                                                (CDITestRoll[i].atDstNM * SG_NM_TO_METER));
        posWrtRadial.setElevationFt(CDITestRoll[i].atAltFt);
        setPositionAndStabilise(r.get(), posWrtRadial);
        // heading-deg property below means bearing to txmitter; calc copied from navradio.cxx  !!!
        double bngToNavaid, az2, s;
        SGGeodesy::inverse(posWrtRadial, (nav->geod()), bngToNavaid, az2, s);
        // calc XTrack error
        double xtkE = sin((CDITestRoll[i].rSele - CDITestRoll[i].onRose) * SG_DEGREES_TO_RADIANS) * (CDITestRoll[i].atDstNM * SG_NM_TO_METER);
        // Verify expected vs Result
        std::string tMesg = itemDesc + "VOR type";
        CPPUNIT_ASSERT_MESSAGE(tMesg, nav->type() == FGPositioned::VOR);
        tMesg = itemDesc + "Operable";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vOpnl, node->getBoolValue("operable"));
        tMesg = itemDesc + "TO Flag";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vToFlag, node->getBoolValue("to-flag"));
        tMesg = itemDesc + "FROM Flag";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vToFlag, !node->getBoolValue("from-flag"));
        //
        tMesg = itemDesc + "nav-id";
        CPPUNIT_ASSERT_MESSAGE(tMesg, node->getStringValue("nav-id") == CDITestRoll[i].nvIden);
        //tbd VOR seems to not set selected-mhz-fmt
        // Converting nvFreq to string results in trailing zeros
        tMesg = itemDesc + "selected-mhz-fmt";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, formatFrequency(CDITestRoll[i].nvFreq), node->getStringValue("frequencies/selected-mhz-fmt"));
        // actual-deg means: bearing seen on intstrument's dial:  actual == onRose
        tMesg = itemDesc + "actual-deg";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, CDITestRoll[i].onRose, node->getDoubleValue("radials/actual-deg"), CDITestRoll[i].vDefnTolr);
        // heading-deg  means true bearing to navaid, not affected by plane's heading
        tMesg = itemDesc + "heading-deg";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, bngToNavaid, node->getDoubleValue("heading-deg"), 1);
        //
        tMesg = itemDesc + "Sig Norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vSigNorm, node->getDoubleValue("signal-quality-norm"), CDITestRoll[i].vSigTolr);
        tMesg = itemDesc + "needle defl";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vHdgDefl, node->getDoubleValue("heading-needle-deflection"), CDITestRoll[i].vDeflTolr);
        tMesg = itemDesc + "defl norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vHdgNorm, node->getDoubleValue("heading-needle-deflection-norm"), CDITestRoll[i].vDefnTolr);
        tMesg = itemDesc + "xTrack error";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xtkE, node->getDoubleValue("crosstrack-error-m"), CDITestRoll[i].xtkTolr);
    }
}

void NavRadioTests::callNewNavRadioCDI()
{
    //
    //2021Ja15 set flag for newnavradio
    //
    fgSetBool("/instrumentation/use-new-navradio", true);
    // setup
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
    //
    int tale = sizeof(CDITestRoll) / sizeof(CDITestRoll[0]);
    for (int i = 0; (i < tale); i++) {
        // prep error message
        const std::string itemDesc = "   navradioCDI Item " + CDITestRoll[i].tDesc + " @ ";
        // Txmitting navaid
        node->setDoubleValue("frequencies/selected-mhz", CDITestRoll[i].nvFreq);
        node->setDoubleValue("radials/selected-deg", CDITestRoll[i].rSele);
        // tbd Filter on type as defined in nav dat
        //FGPositioned::TypeFilter f{FGPositioned::VOR};
        FGPositioned::TypeFilter f{{FGPositioned::VOR, FGPositioned::ILS, FGPositioned::LOC}};
        FGNavRecordRef nav = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent(CDITestRoll[i].nvIden,
                                                                                               SGGeod::fromDeg(CDITestRoll[i].nvLon, CDITestRoll[i].nvLat), &f));
        //
        // For VOR nav dat field 7: 'Twist' == Easterly rotation of Txmitter's 360 wrt True North  c.f for Compass: 'Deviation West Rose is Best'
        // Rx posn is specified according to navaid's radials as printed on chart: True Bng = ( Radial on Rose  +  Twist ( Deviation ))
        //                           ( ftr: Both MCT -5  and PH -2  Are  Negative Twists  )
        SGGeod posWrtRadial = SGGeodesy::direct(nav->geod(), (CDITestRoll[i].onRose + CDITestRoll[i].nvTwst),
                                                (CDITestRoll[i].atDstNM * SG_NM_TO_METER));
        posWrtRadial.setElevationFt(CDITestRoll[i].atAltFt);
        setPositionAndStabilise(r.get(), posWrtRadial);
        // heading-deg property below means bearing to txmitter; calc copied from navradio.cxx  !!!
        double bngToNavaid, az2, s;
        SGGeodesy::inverse(posWrtRadial, (nav->geod()), bngToNavaid, az2, s);
        // calc XTrack error
        double xtkE = sin((CDITestRoll[i].rSele - CDITestRoll[i].onRose) * SG_DEGREES_TO_RADIANS) * (CDITestRoll[i].atDstNM * SG_NM_TO_METER);
        // Verify expected vs Result
        std::string tMesg = itemDesc + "VOR type";
        CPPUNIT_ASSERT_MESSAGE(tMesg, nav->type() == FGPositioned::VOR);
        tMesg = itemDesc + "Operable";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vOpnl, node->getBoolValue("operable"));
        tMesg = itemDesc + "TO Flag";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vToFlag, node->getBoolValue("to-flag"));
        tMesg = itemDesc + "FROM Flag";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vToFlag, !node->getBoolValue("from-flag"));
        //
        tMesg = itemDesc + "nav-id";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, CDITestRoll[i].nvIden, node->getStringValue("nav-id"));

        // Converting nvFreq to string results in trailing zeros
        tMesg = itemDesc + "selected-mhz-fmt";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, formatFrequency(CDITestRoll[i].nvFreq), node->getStringValue("frequencies/selected-mhz-fmt"));

        // actual-deg means: bearing seen on intstrument's dial:  actual == onRose
        tMesg = itemDesc + "actual-deg";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, CDITestRoll[i].onRose, node->getDoubleValue("radials/actual-deg"), CDITestRoll[i].vDefnTolr);
        // heading-deg  means true bearing to navaid, not affected by plane's heading
        tMesg = itemDesc + "heading-deg";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, bngToNavaid, node->getDoubleValue("heading-deg"), 1);
        //
        tMesg = itemDesc + "Sig Norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vSigNorm, node->getDoubleValue("signal-quality-norm"), CDITestRoll[i].vSigTolr);
        tMesg = itemDesc + "needle defl";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vHdgDefl, node->getDoubleValue("heading-needle-deflection"), CDITestRoll[i].vDeflTolr);
        tMesg = itemDesc + "defl norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, CDITestRoll[i].vHdgNorm, node->getDoubleValue("heading-needle-deflection-norm"), CDITestRoll[i].vDefnTolr);
        tMesg = itemDesc + "xTrack error";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xtkE, node->getDoubleValue("crosstrack-error-m"), CDITestRoll[i].xtkTolr);
    }
}

static const struct {
    int nvType;
    double nvLat, nvLon, nvAlt, nvFreq, nvRnge, nvTwst;
    const std::string nvIden;
    double onRose, atDstNM, atAltFt, atHdg, rSele;
    bool vOpnl, vToFlag;
    double vSigNorm, vSigTolr, vHdgDefl, vDeflTolr, vHdgNorm, vDefnTolr, xtkTolr;
    const std::string tDesc;

} ILSTestRoll[] = {
    //
    //  Ref pilotscafe.com  ILS width: 700ft wide at thrsh. WIthin range, sensed at +-35dg @ 10NM  +-10dg @ 18NM
    //
    //  Test Items: Add test cases here:
    //  nv<=   fields are copied direct from nav dat =>       <= Rx pos wrt navaid, Radial =>   <=       v- Values expected / tested            => <= Item - Desc    =>
    //Typ   Lat      Lon    Alt   Freq  Rnge TruHdng   Iden]   onRose atNm atAlt atHdg  rSele   Op To sigN - Tolr  Defl - Tolr  DNrm -Tolr xtkTol ]
    //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 117.932, 2.5, 2500, 27, 297.932, 1, 1, 1.0, 0.01, 0.0, 0.01, 0.0, 0.01, 50.0, "1: ISFO On LOC"},      //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 116.932, 6.0, 1500, 27, 297.932, 1, 1, 1.0, 0.01, -1.0, 0.10, -0.1, 0.01, 50.0, "2: ISFO -1 Deg"},    //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 118.932, 6.0, 1500, 27, 297.932, 1, 1, 1.0, 0.01, 1.0, 0.01, -0.1, 0.01, 50.0, "3: ISFO +1 Deg"},     //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 113.932, 3.0, 600, 27, 297.932, 1, 1, 1.0, 0.01, -3.0, 0.10, -0.1, 0.01, 50.0, "4: ISFO < MinDefl"},  //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 121.932, 3.0, 600, 27, 297.932, 1, 1, 1.0, 0.01, 3.0, 0.01, -0.1, 0.01, 50.0, "5: ISFO > MzxDefl"},   //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 297.932, 4.0, 1500, 27, 297.932, 1, 1, 1.0, 0.01, 0.0, 0.01, 0.0, 0.01, 50.0, "6: ISFO BC On LOC"},   //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 296.932, 4.0, 1500, 27, 297.932, 1, 1, 1.0, 0.01, 1.0, 0.10, -0.1, 0.01, 50.0, "7: ISFO BC -1 Deg"},  //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 298.932, 4.0, 1500, 27, 297.932, 1, 1, 1.0, 0.01, -1.0, 0.01, -0.1, 0.01, 50.0, "8: ISFO BC +1 Deg"}, //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 293.932, 4.0, 1500, 27, 297.932, 1, 1, 1.0, 0.01, 3.0, 0.10, -0.1, 0.01, 50.0, "9: ISFO BC > MaxD"},  //
    {4, 37.626, -122.394, 8, 109.55, 18, 297.932, "ISFO", 301.932, 4.0, 1500, 27, 297.932, 1, 1, 1.0, 0.01, -3.0, 0.01, -0.1, 0.01, 50.0, "10: ISFO BC < MinD"} //
};

void NavRadioTests::callNavRadioILS()
{
    // set flag for newnavradio
    fgSetBool("/instrumentation/use-new-navradio", false);
    // setup
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
    //
    int tale = sizeof(ILSTestRoll) / sizeof(ILSTestRoll[0]);
    for (int i = 0; (i < tale); i++) {
        // prep error message
        const std::string itemDesc = "   navRadioILS Item " + ILSTestRoll[i].tDesc + " @ ";
        // Txmitting navaid
        node->setDoubleValue("frequencies/selected-mhz", ILSTestRoll[i].nvFreq);
        node->setDoubleValue("radials/selected-deg", ILSTestRoll[i].rSele);
        FGPositioned::TypeFilter f{{FGPositioned::VOR, FGPositioned::ILS, FGPositioned::LOC}};
        FGNavRecordRef nav = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent(ILSTestRoll[i].nvIden,
                                                                                               SGGeod::fromDeg(ILSTestRoll[i].nvLon, ILSTestRoll[i].nvLat), &f));
        SGGeod posWrtRadial = SGGeodesy::direct(nav->geod(), (ILSTestRoll[i].onRose), (ILSTestRoll[i].atDstNM * SG_NM_TO_METER));
        posWrtRadial.setElevationFt(ILSTestRoll[i].atAltFt);
        setPositionAndStabilise(r.get(), posWrtRadial);
        // heading-deg property below means bearing to txmitter; calc copied from navradio.cxx  !!!
        double bngToNavaid, az2, s;
        SGGeodesy::inverse(posWrtRadial, (nav->geod()), bngToNavaid, az2, s);
        double xtkE = sin((ILSTestRoll[i].rSele - ILSTestRoll[i].onRose) * SG_DEGREES_TO_RADIANS) * (ILSTestRoll[i].atDstNM * SG_NM_TO_METER);
        //
        const double locWidth = nav->localizerWidth();
        // Expected Defl / Scaling is hokey because ILS width varies ??
        const double deflectionScale = 20.0 / locWidth; // 20 degrees is full VOR swing (-10 to +10 degrees)
        double xpecDefl = (ILSTestRoll[i].vHdgDefl * deflectionScale);
        xpecDefl = (xpecDefl > 10) ? 10 : xpecDefl;
        xpecDefl = (xpecDefl < -10) ? -10 : xpecDefl;
        //
        // Verify expected: Operational and To flags
        std::string tMesg = itemDesc + "ILS type";
        CPPUNIT_ASSERT_MESSAGE(tMesg, nav->type() == FGPositioned::ILS);
        tMesg = itemDesc + "Operable";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].vOpnl, node->getBoolValue("operable"));
        tMesg = itemDesc + "TO Flag";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].vToFlag, node->getBoolValue("to-flag"));
        tMesg = itemDesc + "FROM Flag";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].vToFlag, !node->getBoolValue("from-flag"));
        //
        tMesg = itemDesc + "heading-deg";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, bngToNavaid, node->getDoubleValue("heading-deg"), 1);
        tMesg = itemDesc + "nav-id";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].nvIden, node->getStringValue("nav-id"));
        // Converting nvFreq to string results in trailing zeros
        tMesg = itemDesc + "selected-mhz-fmt";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, formatFrequency(ILSTestRoll[i].nvFreq), node->getStringValue("frequencies/selected-mhz-fmt"));

        // actual-deg means: bearing seen on intstrument's dial:  actual == onRose
        tMesg = itemDesc + "actual-deg";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].onRose, node->getDoubleValue("radials/actual-deg"), ILSTestRoll[i].vDefnTolr);
        tMesg = itemDesc + "Sig Norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].vSigNorm, node->getDoubleValue("signal-quality-norm"), ILSTestRoll[i].vSigTolr);
        tMesg = itemDesc + "needle defl";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xpecDefl, node->getDoubleValue("heading-needle-deflection"), ILSTestRoll[i].vDeflTolr);
        tMesg = itemDesc + "defl norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, (xpecDefl * 0.1), node->getDoubleValue("heading-needle-deflection-norm"), ILSTestRoll[i].vDefnTolr);
        tMesg = itemDesc + "xTrack error";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xtkE, node->getDoubleValue("crosstrack-error-m"), ILSTestRoll[i].xtkTolr);
    }
}

void NavRadioTests::callNewNavRadioILS()
{
    // set flag for newnavradio
    fgSetBool("/instrumentation/use-new-navradio", true);
    // setup
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
    //
    int tale = sizeof(ILSTestRoll) / sizeof(ILSTestRoll[0]);
    for (int i = 0; (i < tale); i++) {
      // prep error message
      const std::string itemDesc = "newNavRadioILS Item " + ILSTestRoll[i].tDesc + " @ ";
      // Txmitting navaid
      node->setDoubleValue("frequencies/selected-mhz", ILSTestRoll[i].nvFreq);
      node->setDoubleValue("radials/selected-deg",     ILSTestRoll[i].rSele);
      FGPositioned::TypeFilter f{{FGPositioned::VOR, FGPositioned::ILS, FGPositioned::LOC}};
      FGNavRecordRef nav = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent(ILSTestRoll[i].nvIden, \
                                                          SGGeod::fromDeg( ILSTestRoll[i].nvLon, ILSTestRoll[i].nvLat), &f));
      SGGeod posWrtRadial = SGGeodesy::direct(nav->geod(), (ILSTestRoll[i].onRose ), (ILSTestRoll[i].atDstNM * SG_NM_TO_METER));
      posWrtRadial.setElevationFt(ILSTestRoll[i].atAltFt);
      setPositionAndStabilise(r.get(), posWrtRadial);
      // heading-deg property below means bearing to txmitter; calc copied from navradio.cxx  !!!
      double bngToNavaid, az2, s; 
      SGGeodesy::inverse(posWrtRadial, (nav->geod()), bngToNavaid, az2, s);
      double xtkE = sin(  (ILSTestRoll[i].rSele - ILSTestRoll[i].onRose) * SG_DEGREES_TO_RADIANS) \
                        * ( ILSTestRoll[i].atDstNM * SG_NM_TO_METER ) ;
      //                
      const double locWidth = nav->localizerWidth();
      // Expected Defl / Scaling is hokey because ILS width varies ?? 
      const double deflectionScale = 20.0 / locWidth; // 20 degrees is full VOR swing (-10 to +10 degrees)
      double xpecDefl = (ILSTestRoll[i].vHdgDefl * deflectionScale);
             xpecDefl = ( xpecDefl >  10 ) ?  10 : xpecDefl;
             xpecDefl = ( xpecDefl < -10 ) ? -10 : xpecDefl;
      //                
      // Verify expected: Operational and To flags
             std::string tMesg = itemDesc + "ILS type";
             CPPUNIT_ASSERT_MESSAGE(tMesg, nav->type() == FGPositioned::ILS);
             tMesg = itemDesc + "Operable";
             CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].vOpnl, node->getBoolValue("operable"));
             tMesg = itemDesc + "TO Flag";
             CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].vToFlag, node->getBoolValue("to-flag"));
             tMesg = itemDesc + "FROM Flag";
             CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].vToFlag, !node->getBoolValue("from-flag"));
             //
             tMesg = itemDesc + "heading-deg";
             CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, bngToNavaid, node->getDoubleValue("heading-deg"), 1);
             tMesg = itemDesc + "nav-id";
             CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].nvIden, node->getStringValue("nav-id"));
             // Converting nvFreq to string results in trailing zeros
             tMesg = itemDesc + "selected-mhz-fmt";
             CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, formatFrequency(ILSTestRoll[i].nvFreq), node->getStringValue("frequencies/selected-mhz-fmt"));
             // actual-deg means: bearing seen on intstrument's dial:  actual == onRose
             tMesg = itemDesc + "actual-deg";
             CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].onRose, node->getDoubleValue("radials/actual-deg"), ILSTestRoll[i].vDefnTolr);
             tMesg = itemDesc + "Sig Norm";
             CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, ILSTestRoll[i].vSigNorm, node->getDoubleValue("signal-quality-norm"), ILSTestRoll[i].vSigTolr);
             tMesg = itemDesc + "needle defl";
             CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xpecDefl, node->getDoubleValue("heading-needle-deflection"), ILSTestRoll[i].vDeflTolr);
             tMesg = itemDesc + "defl norm";
             CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, (xpecDefl * 0.1), node->getDoubleValue("heading-needle-deflection-norm"), ILSTestRoll[i].vDefnTolr);
             tMesg = itemDesc + "xTrack error";
             CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xtkE, node->getDoubleValue("crosstrack-error-m"), ILSTestRoll[i].xtkTolr);
    }
  }    

static const struct  {
  int    nvType; 
  double nvLat, nvLon, nvAlt, nvFreq, nvRnge, nvAzim;
  const std::string nvIden;
  double onRose, atDstNM, atAltFt, plusDeg, atHdg, rTruDeg; 
  bool   vInRnge, vFalse;
  double vSigNorm, vSigTolr, vGSDefl, vDeflTolr, vGSDefn, vDefnTolr;
  const std::string tDesc;
} GSTestRoll[] = {  
  //  
  //  Test Items: Add test cases here:
  //  nv<=   fields are copied direct from nav dat =>       <= Rx pos wrt navaid, Radial   =>  <=       v- Values expected / tested            => <= Item - Desc    =>  
  //Typ   Lat      Lon    Alt  Freq  Rnge   GSAzim  Iden]   onRose atNm atAlt or Deg    atHdg  TruDeg  Rng Fls SgN - Tolr GSDefl-Tolr GSDefn-Tolr ]
  // 
  { 4, 52.563,   13.305, 101, 110.10, 10,   3.000, "ITLW", 117.932, 8.0,    0,     0,  80.828, 260.857,  1, 0, 1.0,  0.01,  0.0,  0.1,  0.0, 0.01, "1: EDDT 26R +0 deg"  }, //
  { 4, 52.563,   13.305, 101, 110.10, 10,   3.000, "ITLW", 117.932, 4.0,    0,  0.50,  80.828, 260.857,  1, 0, 1.0,  0.01,  0.0,  0.1,  0.0, 0.01, "2: EDDT 26R +0.5 d"  }, //
  { 4, 52.563,   13.305, 101, 110.10, 10,   3.000, "ITLW", 117.932, 2.0,    0, -1.00,  80.828, 260.857,  1, 0, 1.0,  0.01,  0.0,  0.1,  0.0, 0.01, "3: EDDT 26R -1 deg"  }, //
  { 4, 52.563,   13.305, 101, 110.10, 10,   3.000, "ITLW", 117.932, 5.0,    0,  3.00,  80.828, 260.857,  1, 1, 1.0,  0.01,  0.0,  0.1,  0.0, 0.01, "4: EDDT 26R +3.0 Fls"}, //
  { 4, 52.563,   13.305, 101, 110.10, 10,   3.000, "ITLW", 117.932, 3.0,    0, +2.65,  80.828, 260.857,  1, 1, 1.0,  0.01,-1.75,  0.1, -0.5, 0.01, "5: EDDT 26R +3.5 Fls"}, //
//  { 4, 51.464,   -0.439,  50, 109.50, 10,   3.000,  "ILL",  89.690, 7.5, 2500,     0,  80.828, 269.690,  1, 0, 1.0,  0.01,  0.0,  0.1,  0.0, 0.01, "6: EGLL 27L  2K5 7M5"}, // Fail: Rx finds IBB
//  { 4, 51.464,   -0.439,  50, 109.50, 10,   3.000,  "ILL",  89.690, 9.0, 3000,     0,  80.828, 269.690,  1, 0, 1.0,  0.01,  0.0,  0.1,  0.0, 0.01, "7: EGLL 27L  3K0 9M0"}, //
//  { 4, 51.464,   -0.439,  50, 109.50, 10,   3.000,  "ILL",  89.690,17.5, 4000,     0,  80.828, 269.690,  1, 0, 1.0,  0.01,  0.0,  0.1,  0.0, 0.01, "8: EGLL 27L  4K 17M5"}, //
 // { 4, 51.464,   -0.439,  50, 109.50, 10,   3.000,  "ILL",  89.690,25.0, 4000,     0,  80.828, 269.690,  1, 0, 1.0,  0.01,  0.0,  0.1,  0.0, 0.01, "9: EGLL 27L  4K 25M0"}  //
};   

void NavRadioTests::callNavRadioGS() {
    //set flag for newnavradio
    fgSetBool("/instrumentation/use-new-navradio", false);
    // setup
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
    //
    // GS beam depth +-0.7deg; needle deflection -+3.5; gs-direct: Rxvr elevation from GS Txmitter level
    //
    const double halfBeam = 0.700;
    const double deflFact = 3.500;
    //
    int tale = sizeof(GSTestRoll) / sizeof(GSTestRoll[0]);
    for (int i = 0; (i < tale); i++) {
        // prep error message
        const std::string itemDesc = "   navRadioGS Item " + GSTestRoll[i].tDesc + " @ ";
        // Txmitting navaid
        node->setDoubleValue("frequencies/selected-mhz", GSTestRoll[i].nvFreq);
        node->setDoubleValue("radials/selected-deg", GSTestRoll[i].rTruDeg);
        FGPositioned::TypeFilter f{{FGPositioned::VOR, FGPositioned::GS, FGPositioned::LOC}};
        FGNavRecordRef nav = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent(GSTestRoll[i].nvIden,
                                                                                               SGGeod::fromDeg(GSTestRoll[i].nvLon, GSTestRoll[i].nvLat), &f));

        // Check for proper nav type befor doing GS things
        std::string tMesg = itemDesc + "GS Type ?";
        CPPUNIT_ASSERT_MESSAGE(tMesg, nav->type() == FGPositioned::GS);
        /////////////
        // derive the GS geometry in cartesian vectors, to match what navradio.cxx does
        SGGeod aboveGS = nav->geod();
        aboveGS.setElevationM(nav->geod().getElevationM() + 100);
        SGVec3d gsVerticalAxis = SGVec3d::fromGeod(aboveGS) - nav->cart();
        // intentionally different approach to what navradio uses
        gsVerticalAxis *= 0.01; // make it per meter, since we used 100m above
        // derive the baseline
        SGQuatd baseLineRot = SGQuatd::fromLonLat(nav->geod()) * SGQuatd::fromHeadAttBankDeg(GSTestRoll[i].atHdg, 0, 0);
        SGVec3d gsAltAxis = baseLineRot.backTransform(SGVec3d(1.0, 0.0, 0.0));
        const SGVec3d gsCart = nav->cart();
        //////////////////
        // expected deflection is calculated here if atAltFt is non-zero
        double xpecAzim, xpecDefl, xpecDefn;
        double bngToNavaid, az2, s;
        if (GSTestRoll[i].atAltFt == 0) {
            // Line item atAlt is zero so use degrees off GlideSlope for Rx position
            double gsAngleRad = (nav->glideSlopeAngleDeg() + GSTestRoll[i].plusDeg) * SG_DEGREES_TO_RADIANS;
            SGVec3d radioPos = gsCart;
            radioPos += (gsVerticalAxis * tan(gsAngleRad) * GSTestRoll[i].atDstNM * SG_NM_TO_METER);
            radioPos += (gsAltAxis * GSTestRoll[i].atDstNM * SG_NM_TO_METER);
            setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));
            xpecAzim = (GSTestRoll[i].nvAzim + GSTestRoll[i].plusDeg);
        } else {
            // Line item atAlt is non zero so use altitude  for Rx position
            SGGeod p = SGGeodesy::direct(nav->geod(), GSTestRoll[i].atAltFt, GSTestRoll[i].atDstNM * SG_NM_TO_METER);
            p.setElevationFt(GSTestRoll[i].atAltFt);
            setPositionAndStabilise(r.get(), p);
            //tbd calc Rx Azim from Tx for altitude case
            SGGeodesy::inverse(p, nav->geod(), bngToNavaid, az2, s);
            xpecAzim = SG_RADIANS_TO_DEGREES * (atan((GSTestRoll[i].atAltFt * SG_FEET_TO_METER) / s));
        }
        //
        if (GSTestRoll[i].vFalse) {
            // rxFlse indicates false signal, use deflections manually entered in Item
            xpecDefl = GSTestRoll[i].vGSDefl;
            xpecDefn = GSTestRoll[i].vGSDefn;
        } else {
            if (GSTestRoll[i].atAltFt == 0) {
                // not rxFlse  and atAltFt is  zero : calculate needle deflections from Rx posn degrees  wrt beam
                xpecDefn = 0 - GSTestRoll[i].plusDeg / halfBeam; // Plane high: needle below
            } else {
                // not rxFlse  and atAltFt non zero : calculate needle deflections from Rx posn azimuth  wrt beam
                xpecDefn = (GSTestRoll[i].nvAzim - az2) / halfBeam; // Plane high: needle below;
            }
            xpecDefn = (xpecDefn > 1) ? 1 : xpecDefn;
            xpecDefn = (xpecDefn < -1) ? -1 : xpecDefn;
            xpecDefl = xpecDefn * deflFact;
        }
        //
        // Verify expected:
        tMesg = itemDesc + "Operable";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, GSTestRoll[i].vInRnge, node->getBoolValue("operable"));
        tMesg = itemDesc + "nav-id";
        auto dddbug = node->getStringValue("nav-id");
        CPPUNIT_ASSERT_MESSAGE(tMesg, node->getStringValue("nav-id") == GSTestRoll[i].nvIden);
        tMesg = itemDesc + "Sig Norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, GSTestRoll[i].vSigNorm, node->getDoubleValue("signal-quality-norm"), GSTestRoll[i].vSigTolr);
        tMesg = itemDesc + "gs-direct";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xpecAzim, node->getDoubleValue("gs-direct-deg"), 1);
        tMesg = itemDesc + "needle defl";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xpecDefl, node->getDoubleValue("gs-needle-deflection"), GSTestRoll[i].vDeflTolr);
        tMesg = itemDesc + "defl norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xpecDefn, node->getDoubleValue("gs-needle-deflection-norm"), GSTestRoll[i].vDefnTolr);
        //
    }
}

void NavRadioTests::callNewNavRadioGS()
{
    //set flag for newnavradio
    fgSetBool("/instrumentation/use-new-navradio", true);
    // setup
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
    //
    // GS beam depth +-0.7deg; needle deflection -+3.5; gs-direct: Rxvr elevation from GS Txmitter level
    //
    const double halfBeam = 0.700;
    const double deflFact = 3.500;
    //
    int tale = sizeof(GSTestRoll) / sizeof(GSTestRoll[0]);
    for (int i = 0; (i < tale); i++) {
        // prep error message
        const std::string itemDesc = "newnavRadioGS Item " + GSTestRoll[i].tDesc + " @ ";
        // Txmitting navaid
        node->setDoubleValue("frequencies/selected-mhz", GSTestRoll[i].nvFreq);
        node->setDoubleValue("radials/selected-deg", GSTestRoll[i].rTruDeg);
        FGPositioned::TypeFilter f{{FGPositioned::VOR, FGPositioned::GS, FGPositioned::LOC}};
        FGNavRecordRef nav = fgpositioned_cast<FGNavRecord>(FGPositioned::findClosestWithIdent(GSTestRoll[i].nvIden,
                                                                                               SGGeod::fromDeg(GSTestRoll[i].nvLon, GSTestRoll[i].nvLat), &f));

        // Check for proper nav type befor doing GS things
        std::string tMesg = itemDesc + "GS Type ?";
        CPPUNIT_ASSERT_MESSAGE(tMesg, nav->type() == FGPositioned::GS);
        /////////////
        // derive the GS geometry in cartesian vectors, to match what navradio.cxx does
        SGGeod aboveGS = nav->geod();
        aboveGS.setElevationM(nav->geod().getElevationM() + 100);
        SGVec3d gsVerticalAxis = SGVec3d::fromGeod(aboveGS) - nav->cart();
        // intentionally different approach to what navradio uses
        gsVerticalAxis *= 0.01; // make it per meter, since we used 100m above
        // derive the baseline
        SGQuatd baseLineRot = SGQuatd::fromLonLat(nav->geod()) * SGQuatd::fromHeadAttBankDeg(GSTestRoll[i].atHdg, 0, 0);
        SGVec3d gsAltAxis = baseLineRot.backTransform(SGVec3d(1.0, 0.0, 0.0));
        const SGVec3d gsCart = nav->cart();
        //////////////////
        // expected deflection is calculated here if atAltFt is non-zero
        double xpecAzim, xpecDefl, xpecDefn;
        double bngToNavaid, az2, s;
        if (GSTestRoll[i].atAltFt == 0) {
            // Line item atAlt is zero so use degrees off GlideSlope for Rx position
            double gsAngleRad = (nav->glideSlopeAngleDeg() + GSTestRoll[i].plusDeg) * SG_DEGREES_TO_RADIANS;
            SGVec3d radioPos = gsCart;
            radioPos += (gsVerticalAxis * tan(gsAngleRad) * GSTestRoll[i].atDstNM * SG_NM_TO_METER);
            radioPos += (gsAltAxis * GSTestRoll[i].atDstNM * SG_NM_TO_METER);
            setPositionAndStabilise(r.get(), SGGeod::fromCart(radioPos));
            xpecAzim = (GSTestRoll[i].nvAzim + GSTestRoll[i].plusDeg);
        } else {
            // Line item atAlt is non zero so use altitude  for Rx position
            SGGeod p = SGGeodesy::direct(nav->geod(), GSTestRoll[i].atAltFt, GSTestRoll[i].atDstNM * SG_NM_TO_METER);
            p.setElevationFt(GSTestRoll[i].atAltFt);
            setPositionAndStabilise(r.get(), p);
            //tbd calc Rx Azim from Tx for altitude case
            SGGeodesy::inverse(p, nav->geod(), bngToNavaid, az2, s);
            xpecAzim = SG_RADIANS_TO_DEGREES * (atan((GSTestRoll[i].atAltFt * SG_FEET_TO_METER) / s));
        }
        //
        if (GSTestRoll[i].vFalse) {
            // rxFlse indicates false signal, use deflections manually entered in Item
            xpecDefl = GSTestRoll[i].vGSDefl;
            xpecDefn = GSTestRoll[i].vGSDefn;
        } else {
            if (GSTestRoll[i].atAltFt == 0) {
                // not rxFlse  and atAltFt is  zero : calculate needle deflections from Rx posn degrees  wrt beam
                xpecDefn = 0 - GSTestRoll[i].plusDeg / halfBeam; // Plane high: needle below
            } else {
                // not rxFlse  and atAltFt non zero : calculate needle deflections from Rx posn azimuth  wrt beam
                xpecDefn = (GSTestRoll[i].nvAzim - az2) / halfBeam; // Plane high: needle below;
            }
            xpecDefn = (xpecDefn > 1) ? 1 : xpecDefn;
            xpecDefn = (xpecDefn < -1) ? -1 : xpecDefn;
            xpecDefl = xpecDefn * deflFact;
        }
        //
        // Verify expected:
        tMesg = itemDesc + "Operable";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(tMesg, GSTestRoll[i].vInRnge, node->getBoolValue("operable"));
        tMesg = itemDesc + "nav-id";
        auto dddbug = node->getStringValue("nav-id");
        CPPUNIT_ASSERT_MESSAGE(tMesg, node->getStringValue("nav-id") == GSTestRoll[i].nvIden);
        tMesg = itemDesc + "Sig Norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, GSTestRoll[i].vSigNorm, node->getDoubleValue("signal-quality-norm"), GSTestRoll[i].vSigTolr);
        tMesg = itemDesc + "gs-direct";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xpecAzim, node->getDoubleValue("gs-direct-deg"), 1);
        tMesg = itemDesc + "needle defl";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xpecDefl, node->getDoubleValue("gs-needle-deflection"), GSTestRoll[i].vDeflTolr);
        tMesg = itemDesc + "defl norm";
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(tMesg, xpecDefn, node->getDoubleValue("gs-needle-deflection-norm"), GSTestRoll[i].vDefnTolr);
        //
    }
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
    CPPUNIT_ASSERT(node->getStringValue("frequencies/selected-mhz-fmt") == "110.10");
    
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
    
    CPPUNIT_ASSERT(node->getStringValue("nav-id") == "ITLW");
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
    
    CPPUNIT_ASSERT(node->getStringValue("nav-id") == "ITLW");
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
    CPPUNIT_ASSERT(node->getStringValue("frequencies/selected-mhz-fmt") == "109.50");

    CPPUNIT_ASSERT_DOUBLES_EQUAL(gs->glideSlopeAngleDeg(), 3.0, 0.001);

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

void NavRadioTests::testVORSignalQuality()
{
    std::unique_ptr<FGNavRadio> r = testVORSignalQuality_setup("navtest", 2);
    SGPropertyNode* node = fgGetNode("/instrumentation/navtest[2]");

    SGGeod pos = SGGeod::fromDegFt(-3.35277800, 55.499167, 20000);
    setPositionAndStabilise(r.get(), pos);
    // The signal quality should be good here
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0,
                                 node->getDoubleValue("signal-quality-norm"),
                                 0.01);

    // 2786 feet is the navaid altitude above sea level, according to nav.dat.
    // I believe 150000 is high enough, however I'm not knowledgeable in this
    // field.
    pos = SGGeod::fromDegFt(-3.35277800, 55.499167, 2786 + 150000);
    setPositionAndStabilise(r.get(), pos);
    // Too high for a good signal here
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0,
                                 node->getDoubleValue("signal-quality-norm"),
                                 0.01);
    r.reset();

    // Test that changing the standby frequency doesn't affect the rate of
    // change of signal quality (it has no reason to disable the low-pass
    // filter).
    double rate1 = testVORSignalQuality_maxRateOfChange(false);
    double rate2 = testVORSignalQuality_maxRateOfChange(true);
    // At the time of this writing, rate1 == rate2, however randomness, signal
    // quality affected by weather... might be added in the future.
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, rate2 / rate1, 0.01);
}

std::unique_ptr<FGNavRadio>
NavRadioTests::testVORSignalQuality_setup(const std::string& name, int index)
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", name);
    configNode->setIntValue("number", index);
    auto r = std::make_unique<FGNavRadio>(configNode);
    r->bind();
    r->init();

    SGPropertyNode* node = fgGetNode("/instrumentation/" + name, index);
    node->setBoolValue("serviceable", true);
    // Needed for the radio to power up
    fgSetDouble("/systems/electrical/outputs/nav", 6.0);
    node->setDoubleValue("frequencies/selected-mhz", 113.8);
    node->setDoubleValue("frequencies/standby-mhz", 112.4); // whatever

    return r;
}

// Rise above the selected VOR station and measure how quickly the signal
// quality diminishes. Return the largest variation seen between two
// consecutive positions.
//
// When the low-pass filter is disabled, the return value is larger (by a
// factor of approximately four with the FG code from November 2022).
double
NavRadioTests::testVORSignalQuality_maxRateOfChange(bool changeStandbyFreq)
{
    std::unique_ptr<FGNavRadio> r = testVORSignalQuality_setup("navtest", 2);
    SGPropertyNode* node = fgGetNode("/instrumentation/navtest[2]");

    double currAlt = 2786 + 65000; // in feet, above MSL
    double maxChange = 0.0;
    double signalQuality, lastSignalQuality = 1.0;

    for (int i = 0; i < 1000; i++) {
        currAlt += 20;
        SGGeod pos = SGGeod::fromDegFt(-3.35277800, 55.499167, currAlt);
        FGTestApi::setPosition(pos);

        if (changeStandbyFreq) {
            node->setDoubleValue("frequencies/standby-mhz",
                                           (i % 2) ? 112.2 : 112.5);
        }
        r->update(0.01);

        signalQuality = node->getDoubleValue("signal-quality-norm");
        maxChange = std::max(maxChange,
                             std::abs(signalQuality - lastSignalQuality));
        lastSignalQuality = signalQuality;
    }

    return maxChange;
}
