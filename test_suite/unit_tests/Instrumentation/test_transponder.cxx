#include "test_transponder.hxx"

#include <cstring>
#include <memory>

#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/testGlobals.hxx"

#include <Airports/airport.hxx>
#include <Navaids/NavDataCache.hxx>

#include <Instrumentation/transponder.hxx>
#include <Main/fg_props.hxx>
#include <Main/locale.hxx>

// Set up function for each test.
void TransponderTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("transponder");
    FGTestApi::setUp::initNavDataCache();
}


// Clean up after each test.
void TransponderTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

SGSubsystemRef TransponderTests::setupStandardTransponder(const std::string& name, int index)
{
    SGPropertyNode_ptr configNode(new SGPropertyNode);

    globals->get_props()->setDoubleValue("systems/electrical/outputs/transponder", 10.0);

    configNode->setStringValue("name", name);
    configNode->setIntValue("number", index);
    auto r = new Transponder(configNode);

    r->bind();
    r->init();

    globals->add_subsystem("transponder", r);

    return r;
}

void TransponderTests::testBasic()
{
    SGPropertyNode* altitudeSource = fgGetNode("/instrumentation/altimeter", true);

    auto t = setupStandardTransponder("transponder", 0);
    altitudeSource->setDoubleValue("mode-s-alt-ft", 1234.0);


    SGPropertyNode* xpdrNode = fgGetNode("/instrumentation/transponder[0]");
    xpdrNode->setIntValue("inputs/knob-mode", 5); // KNOB_ALT
    xpdrNode->setIntValue("inputs/mode", 2);      // MODE S

    xpdrNode->setIntValue("id-code", 4621);

    t->update(1.0);

    CPPUNIT_ASSERT_EQUAL(4, xpdrNode->getIntValue("inputs/digit[3]"));
    CPPUNIT_ASSERT_EQUAL(6, xpdrNode->getIntValue("inputs/digit[2]"));
    CPPUNIT_ASSERT_EQUAL(2, xpdrNode->getIntValue("inputs/digit[1]"));
    CPPUNIT_ASSERT_EQUAL(1, xpdrNode->getIntValue("inputs/digit[0]"));

    CPPUNIT_ASSERT_EQUAL(true, xpdrNode->getBoolValue("altitude-valid"));
    CPPUNIT_ASSERT_EQUAL(1234, xpdrNode->getIntValue("altitude"));

    xpdrNode->setIntValue("inputs/digit[2]", 2);
    CPPUNIT_ASSERT_EQUAL(4221, xpdrNode->getIntValue("id-code"));

    t->update(1.0);

    CPPUNIT_ASSERT_EQUAL(4221, xpdrNode->getIntValue("transmitted-id"));

    xpdrNode->setBoolValue("inputs/ident-btn", true);
    CPPUNIT_ASSERT_EQUAL(true, xpdrNode->getBoolValue("ident"));
    xpdrNode->setBoolValue("inputs/ident-btn", false);

    // remain on for now
    CPPUNIT_ASSERT_EQUAL(true, xpdrNode->getBoolValue("ident"));

    FGTestApi::runForTime(20.0);
    CPPUNIT_ASSERT_EQUAL(false, xpdrNode->getBoolValue("ident"));

    xpdrNode->setIntValue("inputs/knob-mode", 1); // KNOB_STANDBY
}

void TransponderTests::testStandby()
{
    SGPropertyNode* altitudeSource = fgGetNode("/instrumentation/altimeter", true);
    auto t = setupStandardTransponder("transponder", 0);
    altitudeSource->setDoubleValue("mode-s-alt-ft", 1234.0);


    SGPropertyNode* xpdrNode = fgGetNode("/instrumentation/transponder[0]");
    xpdrNode->setIntValue("inputs/knob-mode", 1); // KNOB_STANDBY
    xpdrNode->setIntValue("id-code", 4621);

    t->update(1.0);

    CPPUNIT_ASSERT_EQUAL(4621, xpdrNode->getIntValue("id-code"));
    CPPUNIT_ASSERT_EQUAL(-9999, xpdrNode->getIntValue("transmitted-id"));
}
