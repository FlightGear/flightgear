#include "testDigitalFilter.hxx"

#include <strstream>

#include "test_suite/FGTestApi/testGlobals.hxx"


#include <Autopilot/autopilot.hxx>
#include <Autopilot/digitalfilter.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>


#include <simgear/math/sg_random.h>
#include <simgear/props/props_io.hxx>

// Set up function for each test.
void DigitalFilterTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("ap-digitialfilter");
}


// Clean up after each test.
void DigitalFilterTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}


SGPropertyNode_ptr DigitalFilterTests::configFromString(const std::string& s)
{
    SGPropertyNode_ptr config = new SGPropertyNode;

    std::istringstream iss(s);
    readProperties(iss, config);
    return config;
}

void DigitalFilterTests::testNoise()
{
    sg_srandom(999);

    auto config = configFromString(R"(<?xml version="1.0" encoding="UTF-8"?>
                                    <PropertyList>
                                    <filter>
                                        <input>/test/a</input>
                                        <output>/test/b</output>
                                        <type>coherent-noise</type>
                                        <amplitude>3.0</amplitude>
                                        <absolute type="bool">true</absolute>
                                        <discrete-resolution>1024</discrete-resolution>
                                    </filter>
                                    </PropertyList>
                                    )");

    auto ap = new FGXMLAutopilot::Autopilot(globals->get_props(), config);

    globals->add_subsystem("ap", ap, SGSubsystemMgr::FDM);
    ap->bind();
    ap->init();

    //
    //    for (double x=0.0; x < 5.0; x+=0.01) {
    //        fgSetDouble("/test/a", x);
    //        ap->update(0.1);
    //        std::cerr << fgGetDouble("/test/b") << std::endl;
    //    }

    fgSetDouble("/test/a", 0.5);
    ap->update(0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.029, fgGetDouble("/test/b"), 0.001);

    fgSetDouble("/test/a", 0.8);
    ap->update(0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.808, fgGetDouble("/test/b"), 0.001);

    fgSetDouble("/test/a", 0.3);
    ap->update(0.1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.478, fgGetDouble("/test/b"), 0.001);
}
