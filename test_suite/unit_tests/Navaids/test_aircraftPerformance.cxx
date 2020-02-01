#include "test_aircraftPerformance.hxx"

#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"

#include <simgear/misc/strutils.hxx>

#include <Main/fg_props.hxx>
#include <Aircraft/AircraftPerformance.hxx>

using namespace flightgear;

// Set up function for each test.
void AircraftPerformanceTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("aircraft-perf");
    FGTestApi::setUp::initNavDataCache();
}


// Clean up after each test.
void AircraftPerformanceTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void AircraftPerformanceTests::testBasic()
{
    fgSetString("/aircraft/performance/icao-category", "C");
    AircraftPerformance ap;

    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.groundSpeedForAltitudeKnots(1000), 152, 1e-3);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.groundSpeedForAltitudeKnots(8000), 224, 1e-3);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.groundSpeedForAltitudeKnots(20000), 491, 1e-3);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.groundSpeedForAltitudeKnots(35000), 461, 1e-3);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.groundSpeedForAltitudeKnots(38000), 458, 1e-3);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.groundSpeedForAltitudeKnots(40000), 458, 1e-3);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.timeBetween(3000, 6000), 100, 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.distanceNmBetween(3000, 6000), 5.430, 1e-3);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.timeBetween(36000, 34000), 100, 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.distanceNmBetween(36000, 34000), 12.805, 1e-1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.timeBetween(15000, 20000), 300, 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.distanceNmBetween(15000, 18000), 14.270, 1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.timeBetween(2000, 25000), 1191.6, 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.distanceNmBetween(2000, 25000), 123.06, 1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.timeBetween(36000, 3000), 1666.6, 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(ap.distanceNmBetween(36000, 3000), 162.02, 1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(251.5, ap.timeToCruise(32.0, 350000), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(503.0, ap.timeToCruise(64.0, 380000), 1);

    CPPUNIT_ASSERT_DOUBLES_EQUAL(1553.7, ap.turnRadiusMForAltitude(4000), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3322.4, ap.turnRadiusMForAltitude(16000), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4602.6, ap.turnRadiusMForAltitude(30000), 1);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4475.5, ap.turnRadiusMForAltitude(38000), 1);
}

void AircraftPerformanceTests::testAltitudeGradient()
{
    fgSetString("/aircraft/performance/icao-category", "E");
    AircraftPerformance ap;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(8332, ap.computePreviousAltitude(10000, 6000), 1);
    
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3260, ap.computeNextAltitude(4000, 2000), 1);


}

