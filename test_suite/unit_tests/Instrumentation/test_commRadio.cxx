#include "test_commRadio.hxx"

#include <cstring>
#include <memory>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Airports/airport.hxx>
#include <Navaids/NavDataCache.hxx>

#include <Instrumentation/commradio.hxx>
#include <Main/fg_props.hxx>
#include <Main/locale.hxx>

// Set up function for each test.
void CommRadioTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("commradio");
    FGTestApi::setUp::initNavDataCache();

    // otherwise ATCSPeech will call locale functions and assert
    globals->get_locale()->selectLanguage({});
}


// Clean up after each test.
void CommRadioTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


// std::string NavRadioTests::formatFrequency(double f)
// {
//     char buf[16];
//     ::snprintf(buf, 16, "%3.2f", f);
//     return buf;
// }

SGSubsystemRef CommRadioTests::setupStandardRadio(const std::string& name, int index, bool enable833)
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", name);
    configNode->setIntValue("number", index);
    configNode->setBoolValue("eight-point-three", enable833);
    auto r = Instrumentation::CommRadio::createInstance(configNode);

    fgSetBool("/sim/atis/enabled", false);

    r->bind();
    r->init();

    globals->add_subsystem("comm-radio", r);

    return r;
}

void CommRadioTests::testBasic()
{
    auto r = setupStandardRadio("commtest", 2, false);

    FGAirportRef apt = FGAirport::getByIdent("EDDM");
    FGTestApi::setPositionAndStabilise(apt->geod());

    SGPropertyNode_ptr n = globals->get_props()->getNode("instrumentation/commtest[2]");
    // EDDM ATIS
    n->setDoubleValue("frequencies/selected-mhz", 123.125);
    r->update(1.0);

    //   CPPUNIT_ASSERT_DOUBLES_EQUAL(25, n->getDoubleValue("frequencies/selected-channel-width-khz"), 1e-3);
    CPPUNIT_ASSERT_EQUAL("123.12"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});

    CPPUNIT_ASSERT_EQUAL("EDDM"s, string{n->getStringValue("airport-id")});
    CPPUNIT_ASSERT_EQUAL("ATIS"s, string{n->getStringValue("station-name")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, n->getDoubleValue("slant-distance-m"), 1e-6);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, n->getDoubleValue("signal-quality-norm"), 1e-6);


    n->setDoubleValue("frequencies/selected-mhz", 121.72);
    r->update(1.0);

    CPPUNIT_ASSERT_EQUAL("121.72"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});

    CPPUNIT_ASSERT_EQUAL("EDDM"s, string{n->getStringValue("airport-id")});
    CPPUNIT_ASSERT_EQUAL("CLNC DEL"s, string{n->getStringValue("station-name")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, n->getDoubleValue("slant-distance-m"), 1e-6);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, n->getDoubleValue("signal-quality-norm"), 1e-6);
}

void CommRadioTests::testEightPointThree()
{
    auto r = setupStandardRadio("commtest", 2, true);


    FGAirportRef apt = FGAirport::getByIdent("EGKK");
    FGTestApi::setPositionAndStabilise(apt->geod());

    SGPropertyNode_ptr n = globals->get_props()->getNode("instrumentation/commtest[2]");

    // EGKK ATIS
    n->setDoubleValue("frequencies/selected-mhz", 136.525);

    r->update(1.0);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(25, n->getDoubleValue("frequencies/selected-channel-width-khz"), 1e-3);
    CPPUNIT_ASSERT_EQUAL("136.525"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});

    // random 8.3Khz station

    n->setDoubleValue("frequencies/selected-mhz", 120.11);
    r->update(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.33, n->getDoubleValue("frequencies/selected-channel-width-khz"), 1e-3);
    CPPUNIT_ASSERT_EQUAL("120.110"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(338, n->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(120.10833, n->getDoubleValue("frequencies/selected-real-frequency-mhz"), 1e-6);

    // select station by channel, on 8.3khz boundary
    n->setIntValue("frequencies/selected-channel", 2561);
    r->update(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.33, n->getDoubleValue("frequencies/selected-channel-width-khz"), 1e-3);
    CPPUNIT_ASSERT_EQUAL("134.005"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(2561, n->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(134.000, n->getDoubleValue("frequencies/selected-real-frequency-mhz"), 1e-6);

    // select station by channel, on 25Khz boundary
    n->setIntValue("frequencies/selected-channel", 2560);
    r->update(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(25, n->getDoubleValue("frequencies/selected-channel-width-khz"), 1e-3);
    CPPUNIT_ASSERT_EQUAL("134.000"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(2560, n->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(134.000, n->getDoubleValue("frequencies/selected-real-frequency-mhz"), 1e-6);

    // select by frequency
    n->setDoubleValue("frequencies/selected-mhz", 120.035);
    r->update(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.33, n->getDoubleValue("frequencies/selected-channel-width-khz"), 1e-3);
    CPPUNIT_ASSERT_EQUAL("120.035"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(326, n->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(120.03333, n->getDoubleValue("frequencies/selected-real-frequency-mhz"), 1e-6);

    // under-run the permitted frequency range
    n->setDoubleValue("frequencies/selected-mhz", 117.99);
    r->update(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(25.0, n->getDoubleValue("frequencies/selected-channel-width-khz"), 1e-3);
    CPPUNIT_ASSERT_EQUAL(0, n->getIntValue("frequencies/selected-channel"));

    n->setDoubleValue("frequencies/selected-mhz", 118.705);
    r->update(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.33, n->getDoubleValue("frequencies/selected-channel-width-khz"), 1e-3);
    CPPUNIT_ASSERT_EQUAL("118.705"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(113, n->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(118.700, n->getDoubleValue("frequencies/selected-real-frequency-mhz"), 1e-6);

    // over-run the frequency range
    n->setDoubleValue("frequencies/selected-mhz", 137.000);
    r->update(1.0);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8.33, n->getDoubleValue("frequencies/selected-channel-width-khz"), 1e-3);
    CPPUNIT_ASSERT_EQUAL("136.990"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(3039, n->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(136.99166, n->getDoubleValue("frequencies/selected-real-frequency-mhz"), 1e-6);
}

void CommRadioTests::testEPLLTuning833()
{
    // this test is disabled until data entry for EPLL is fixed
    return;

    auto r = setupStandardRadio("commtest", 2, true);

    FGAirportRef apt = FGAirport::getByIdent("EPLL");
    FGTestApi::setPositionAndStabilise(apt->geod());

    SGPropertyNode_ptr n = globals->get_props()->getNode("instrumentation/commtest[2]");
    // should be EPLL TWR
    n->setDoubleValue("frequencies/selected-mhz", 124.225);
    r->update(1.0);

    CPPUNIT_ASSERT_EQUAL("EPLL"s, string{n->getStringValue("airport-id")});
    CPPUNIT_ASSERT_EQUAL("Lodz TOWER"s, string{n->getStringValue("station-name")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, n->getDoubleValue("slant-distance-m"), 1e-6);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, n->getDoubleValue("signal-quality-norm"), 1e-6);
}

void CommRadioTests::testEPLLTuning25()
{
    auto r = setupStandardRadio("commtest", 2, false);

    FGAirportRef apt = FGAirport::getByIdent("EPLL");
    FGTestApi::setPositionAndStabilise(apt->geod());

    SGPropertyNode_ptr n = globals->get_props()->getNode("instrumentation/commtest[2]");
    // should be EPLL TWR
    n->setDoubleValue("frequencies/selected-mhz", 124.23);
    r->update(1.0);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(124.23, n->getDoubleValue("frequencies/selected-mhz"), 1e-6);
    CPPUNIT_ASSERT_EQUAL("124.22"s, string{n->getStringValue("frequencies/selected-mhz-fmt")});

    // fail for now
#if 0
    CPPUNIT_ASSERT_EQUAL("EPLL"s, string{n->getStringValue("airport-id")});
    CPPUNIT_ASSERT_EQUAL("Lodz TOWER"s, string{n->getStringValue("station-name")});
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0, n->getDoubleValue("slant-distance-m"), 1e-6);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, n->getDoubleValue("signal-quality-norm"), 1e-6);
#endif
}
