#include "test_commRadio.hxx"

#include <memory>
#include <cstring>

#include "test_suite/FGTestApi/globals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"

#include <Navaids/NavDataCache.hxx>
#include <Airports/airport.hxx>

#include <Instrumentation/commradio.hxx>

using namespace Instrumentation;

// Set up function for each test.
void CommRadioTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("commradio");
    FGTestApi::setUp::initNavDataCache();
}


// Clean up after each test.
void CommRadioTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void CommRadioTests::setPositionAndStabilise(SGSubsystem* r, const SGGeod& g)
{
    FGTestApi::setPosition(g);
    for (int i=0; i<60; ++i) {
        r->update(0.1);
    }
}

void CommRadioTests::testBasic()
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "commtest");
    configNode->setIntValue("number", 2);
    
    auto sub = Instrumentation::CommRadio::createInstance(configNode);
    std::unique_ptr<SGSubsystem> r{sub};

    r->bind();
    r->init();

    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/commtest[2]");
    node->setBoolValue("serviceable", true);
    // needed for the radio to power up
    globals->get_props()->setDoubleValue("systems/electrical/outputs/commtest", 6.0);

    // invalid frequency, of course
    node->setDoubleValue("frequencies/selected-mhz", 140.0);
    
    // let's use BIKF / Keflavik for testing
    auto bikf = FGAirport::getByIdent("BIKF");
    setPositionAndStabilise(r.get(), bikf->geod());

    // should be powered up
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("operable"));
    CPPUNIT_ASSERT_EQUAL(0.0, node->getDoubleValue("signal-quality-norm"));
    std::string id = node->getStringValue("airport-id");
    CPPUNIT_ASSERT(id.empty());

    // published frequencies in our apt.dat:
//    50 12830 ATIS
//    53 12190 GND
//    54 11830 TWR
//    54 13190 AIR GND
//    55 11930 APP
//    56 11930 DEP
    
    node->setDoubleValue("frequencies/selected-mhz", 128.30);
    setPositionAndStabilise(r.get(), bikf->geod());
    
    id = node->getStringValue("airport-id");
    CPPUNIT_ASSERT_EQUAL(std::string{"BIKF"}, id);
    CPPUNIT_ASSERT_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"));
    
    std::string type = node->getStringValue("station-type");
    CPPUNIT_ASSERT_EQUAL(std::string{"atis"}, type);

    
    // and now let's re-tune
    
    node->setDoubleValue("frequencies/selected-mhz", 118.30);
    setPositionAndStabilise(r.get(), bikf->geod());
    
    id = node->getStringValue("airport-id");
    CPPUNIT_ASSERT_EQUAL(std::string{"BIKF"}, id);
    CPPUNIT_ASSERT_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"));
    
    type = node->getStringValue("station-type");
    CPPUNIT_ASSERT_EQUAL(std::string{"tower"}, type);

}

void CommRadioTests::testWith8Point3Mode()
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);
    configNode->setStringValue("name", "commtest");
    configNode->setIntValue("number", 2);
    configNode->setBoolValue("eight-point-three", true);
    auto sub = Instrumentation::CommRadio::createInstance(configNode);
    std::unique_ptr<SGSubsystem> r{sub};
    
    r->bind();
    r->init();
    
    SGPropertyNode_ptr node = globals->get_props()->getNode("instrumentation/commtest[2]");
    node->setBoolValue("serviceable", true);
    // needed for the radio to power up
    globals->get_props()->setDoubleValue("systems/electrical/outputs/commtest", 6.0);
    
    
    // EDDF ATIS is a known problem case
    auto eddf = FGAirport::getByIdent("EDDF");
    setPositionAndStabilise(r.get(), eddf->geod());
    
    // should be powered up
    CPPUNIT_ASSERT_EQUAL(true, node->getBoolValue("operable"));
    
    // published frequencies in our apt.dat:
//    50 11802 ATIS
//    50 11872 ATIS 2
//    52 12190 Delivery
//    53 12180 Ground
//    53 12195 Apron East
//    53 12185 Apron South
//    53 12170 Apron West
//    54 11990 Tower
//    54 12485 Tower
//    54 12732 Tower
//    55 11845 Langen Radar
//    55 12655 Langen Radar
//    55 13612 Langen Radar
//    56 12015 Langen Radar
//    56 12080 Langen Radar
    
    node->setDoubleValue("frequencies/selected-mhz", 118.02);
    setPositionAndStabilise(r.get(), eddf->geod());
    
    std::string id = node->getStringValue("airport-id");
    CPPUNIT_ASSERT_EQUAL(std::string{"EDDF"}, id);
    CPPUNIT_ASSERT_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"));
    
    std::string type = node->getStringValue("station-type");
    CPPUNIT_ASSERT_EQUAL(std::string{"atis"}, type);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"118.020"}, std::string{node->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(4, node->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(118.020, node->getDoubleValue("frequencies/selected-mhz"), 0.001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(118.025, node->getDoubleValue("frequencies/selected-real-frequency-mhz"), 0.001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    
    
// EDME / EGGENFELDEN
    auto edme = FGAirport::getByIdent("EDME");
    node->setDoubleValue("frequencies/selected-mhz", 125.07);
    setPositionAndStabilise(r.get(), edme->geod());
    
    id = node->getStringValue("airport-id");
    CPPUNIT_ASSERT_EQUAL(std::string{"EDME"}, id);
    CPPUNIT_ASSERT_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"));
    
    CPPUNIT_ASSERT_EQUAL(std::string{"125.070"}, std::string{node->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(1132, node->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(125.070, node->getDoubleValue("frequencies/selected-mhz"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(125.075, node->getDoubleValue("frequencies/selected-real-frequency-mhz"), 0.001);
    
// let's try EDDF but passing the real 25ks frequency in instead
    node->setDoubleValue("frequencies/selected-mhz", 118.025);
    setPositionAndStabilise(r.get(), eddf->geod());
    
    id = node->getStringValue("airport-id");
    CPPUNIT_ASSERT_EQUAL(std::string{"EDDF"}, id);
    CPPUNIT_ASSERT_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"));
    
    type = node->getStringValue("station-type");
    CPPUNIT_ASSERT_EQUAL(std::string{"atis"}, type);
    
    CPPUNIT_ASSERT_EQUAL(std::string{"118.020"}, std::string{node->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(4, node->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(118.020, node->getDoubleValue("frequencies/selected-mhz"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(118.025, node->getDoubleValue("frequencies/selected-real-frequency-mhz"), 0.01);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, node->getDoubleValue("signal-quality-norm"), 0.01);
    
// let's try some actual 8.3 spacing channels - note we don't have these in apt.dat right now so
// they won't tune in
    node->setDoubleValue("frequencies/selected-mhz", 125.015);
    setPositionAndStabilise(r.get(), edme->geod());
    
    CPPUNIT_ASSERT_EQUAL(std::string{"125.015"}, std::string{node->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(1123, node->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(125.0150, node->getDoubleValue("frequencies/selected-mhz"), 0.001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(125.0167, node->getDoubleValue("frequencies/selected-real-frequency-mhz"), 0.001);
    
    
    node->setDoubleValue("frequencies/selected-mhz", 120.810);
    setPositionAndStabilise(r.get(), edme->geod());
    
    CPPUNIT_ASSERT_EQUAL(std::string{"120.810"}, std::string{node->getStringValue("frequencies/selected-mhz-fmt")});
    CPPUNIT_ASSERT_EQUAL(450, node->getIntValue("frequencies/selected-channel"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(120.810, node->getDoubleValue("frequencies/selected-mhz"), 0.001);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(120.80833, node->getDoubleValue("frequencies/selected-real-frequency-mhz"), 0.00001);
}
