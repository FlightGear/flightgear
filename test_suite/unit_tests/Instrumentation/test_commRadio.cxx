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

void CommRadioTests::testBasic()
{
}

void CommRadioTests::testEightPointThree()
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "commtest");
    configNode->setIntValue("number", 2);
    configNode->setBoolValue("eight-point-three", true);
    auto r = Instrumentation::CommRadio::createInstance(configNode);


    fgSetBool("/sim/atis/enabled", false);

    r->bind();
    r->init();

    globals->add_subsystem("comm-radio", r);

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
}
