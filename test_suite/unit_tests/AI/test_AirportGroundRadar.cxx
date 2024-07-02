/*
 * Copyright (C) 2024 Keith Paterson
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

#include "test_AirportGroundRadar.hxx"

#include <cstring>
#include <memory>

#include "config.h"
#include "test_suite/FGTestApi/testGlobals.hxx"
#include "test_suite/FGTestApi/NavDataCache.hxx"
#include "test_suite/FGTestApi/TestDataLogger.hxx"
#include "test_suite/FGTestApi/TestPilot.hxx"

#include <ATC/AirportGroundRadar.hxx>
#include <AIModel/AIShip.hxx>
#include <Airports/airports_fwd.hxx>
#include <Airports/airport.hxx>


using namespace flightgear;

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void AirportGroundRadarTests::setUp()
{
    FGTestApi::setUp::initTestGlobals("AirportGroundRadar");
    FGTestApi::setUp::initNavDataCache();
    FGAirport::clearAirportsCache();
    FGAirportRef egph = FGAirport::getByIdent("EGPH");
    egph->testSuiteInjectGroundnetXML(SGPath::fromUtf8(FG_TEST_SUITE_DATA) / "EGPH.groundnet.xml");
}

// Clean up after each test.
void AirportGroundRadarTests::tearDown()
{
    FGTestApi::tearDown::shutdownTestGlobals();
}

void AirportGroundRadarTests::testFillingTree()
{
    SGGeod minPos = SGGeod::fromDeg(50,50);
    SGGeod maxPos = SGGeod::fromDeg(60,60);

    AirportGroundRadar testsubject(minPos, maxPos);

    FGTrafficRecord rec;
    auto rect = testsubject.getBox(&rec);
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().x());
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().y());

    for (size_t i = 0; i < 4; i++)
    {
      for (size_t j = 0; j < 4; j++) {
        FGTrafficRecord rec1;
        rec1.setPositionAndHeading((i/10)+50, (i/10)+50, 45, 20, 0);;
        testsubject.add(&rec1);
      }
    }
}

void AirportGroundRadarTests::testFillingTreeSplit()
{
    SGGeod minPos = SGGeod::fromDeg(50,50);
    SGGeod maxPos = SGGeod::fromDeg(60,60);

    AirportGroundRadar testsubject(minPos, maxPos);

    FGTrafficRecord rec;
    auto rect = testsubject.getBox(&rec);
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().x());
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().y());

    for (size_t i = 0; i < 10; i++)
    {
      for (size_t j = 0; j < 10; j++) {
        FGTrafficRecord rec;
        rec.setPositionAndHeading((i/10)+50, (i/10)+50, 45, 20, 0);;
        testsubject.add(&rec);
      }
    }
    CPPUNIT_ASSERT_EQUAL(size_t(100), testsubject.size());
}

void AirportGroundRadarTests::testFillingTreeRemove()
{
    SGGeod minPos = SGGeod::fromDeg(50,50);
    SGGeod maxPos = SGGeod::fromDeg(60,60);

    AirportGroundRadar testsubject(minPos, maxPos);

    FGTrafficRecord rec33;
    auto rect = testsubject.getBox(&rec33);
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().x());
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().y());

    for (size_t i = 0; i < 100; i++)
    {
      FGTrafficRecord rec;
      rec.setPositionAndHeading((i/10)+50, (i/10)+50, 45, 20, 0);;
      testsubject.add(&rec);
      bool removed = testsubject.remove(&rec);
      CPPUNIT_ASSERT_EQUAL(true, removed);
      CPPUNIT_ASSERT_EQUAL(size_t(0), testsubject.size());
    }

    FGTrafficRecord rec1;
    rec1.setPositionAndHeading(50, 50, 45, 20, 0);;
    testsubject.add(&rec1);
    FGTrafficRecord rec2;
    rec2.setPositionAndHeading(50, 50, 45, 20, 0);;
    testsubject.add(&rec2);
    CPPUNIT_ASSERT_EQUAL(size_t(2), testsubject.size());

    testsubject.remove(&rec1);
    testsubject.remove(&rec2);

    CPPUNIT_ASSERT_EQUAL(size_t(0), testsubject.size());

}

void AirportGroundRadarTests::testBlocked()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGTrafficRecord rec1;
  rec1.setId(4);
  rec1.setPositionAndHeading(50, 50, 45, 20, 0);
  testsubject.add(&rec1);
  FGTrafficRecord rec2;
  rec2.setId(6);
  rec2.setPositionAndHeading(50, 50.001, 315, 20, 0);;
  testsubject.add(&rec2);
  CPPUNIT_ASSERT_EQUAL(false, testsubject.isBlocked(&rec1));
  CPPUNIT_ASSERT_EQUAL(false, testsubject.isBlocked(&rec2));
}

void AirportGroundRadarTests::testBlocked1()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGTrafficRecord rec1;
  rec1.setId(1);
  rec1.setPositionAndHeading(50, 50, 315, 20, 0);
  testsubject.add(&rec1);
  FGTrafficRecord rec2;
  rec2.setId(2);
  rec2.setPositionAndHeading(50, 50.001, 45, 20, 0);
  testsubject.add(&rec2);
  CPPUNIT_ASSERT_EQUAL(true, testsubject.isBlocked(&rec1));
  CPPUNIT_ASSERT_EQUAL(false, testsubject.isBlocked(&rec2));
}

void AirportGroundRadarTests::testBlockedBy1()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGTrafficRecord rec1;
  rec1.setId(2);
  rec1.setPositionAndHeading(50, 50, 315, 20, 0);
  testsubject.add(&rec1);
  
  FGTrafficRecord rec2;
  rec2.setId(5);
  rec2.setPositionAndHeading(50, 50.001, 45, 20, 0);
  testsubject.add(&rec2);

  FGTrafficRecord rec3;
  rec3.setId(4);
  rec3.setPositionAndHeading(50, 50.003, 45, 20, 0);
  testsubject.add(&rec3);

  CPPUNIT_ASSERT(testsubject.isBlockedBy(&rec2)==nullptr);
  CPPUNIT_ASSERT_EQUAL(rec2.getId(), testsubject.isBlockedBy(&rec1)->getId());
}

void AirportGroundRadarTests::testBlockedByQueue()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGTrafficRecord rec1;
  rec1.setId(2);
  rec1.setPositionAndHeading(50, 50, 270, 20, 0);
  testsubject.add(&rec1);
  
  FGTrafficRecord rec2;
  rec2.setId(4);
  rec2.setPositionAndHeading(50, 50.001, 270, 20, 0);
  testsubject.add(&rec2);

  FGTrafficRecord rec3;
  rec3.setId(26);
  rec3.setPositionAndHeading(50, 50.002, 270, 20, 0);
  testsubject.add(&rec3);

  // Not near
  FGTrafficRecord boatyMcBoatface4;
  boatyMcBoatface4.setPositionAndHeading(50, 50.005, 270, 20, 0);
  testsubject.add(&boatyMcBoatface4);

  CPPUNIT_ASSERT_MESSAGE("Blocker of rec2", testsubject.isBlockedBy(&rec2)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of rec3", testsubject.isBlockedBy(&rec3)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface4 (None)", testsubject.isBlockedBy(&boatyMcBoatface4)==nullptr);
  CPPUNIT_ASSERT_EQUAL(rec1.getId(), testsubject.isBlockedBy(&rec2)->getId());
  CPPUNIT_ASSERT_EQUAL(rec2.getId(), testsubject.isBlockedBy(&rec3)->getId());
}

void AirportGroundRadarTests::testMove()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGTrafficRecord rec1;
  rec1.setId(2);
  // Will be moved to 50/50
  rec1.setPositionAndHeading(60, 60, 270, 20, 0);
  testsubject.add(&rec1);
  
  FGTrafficRecord rec2;
  rec2.setId(4);
  rec2.setPositionAndHeading(50, 50.001, 270, 20, 0);
  testsubject.add(&rec2);

  FGTrafficRecord rec3;
  rec3.setId(26);
  rec3.setPositionAndHeading(50, 50.002, 270, 20, 0);
  testsubject.add(&rec3);

  testsubject.move(SGRect<double>(50,50), &rec1);
  rec1.setPositionAndHeading(50, 50, 270, 20, 0);

  // Not near
  FGTrafficRecord boatyMcBoatface4;
  boatyMcBoatface4.setId(33);
  boatyMcBoatface4.setPositionAndHeading(50, 50.005, 270, 20, 0);
  testsubject.add(&boatyMcBoatface4);

  CPPUNIT_ASSERT_MESSAGE("Blocker of rec2", testsubject.isBlockedBy(&rec2)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of rec3", testsubject.isBlockedBy(&rec3)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface4 (None)", testsubject.isBlockedBy(&boatyMcBoatface4)==nullptr);
  CPPUNIT_ASSERT_EQUAL(rec1.getId(), testsubject.isBlockedBy(&rec2)->getId());
  CPPUNIT_ASSERT_EQUAL(rec2.getId(), testsubject.isBlockedBy(&rec3)->getId());
}

void AirportGroundRadarTests::testMoveLarge()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGTrafficRecord rec1;
  rec1.setId(2);
  // Will be moved to 50/50
  rec1.setPositionAndHeading(60, 60, 270, 20, 0);
  testsubject.add(&rec1);
  
  FGTrafficRecord rec2;
  rec2.setId(4);
  rec2.setPositionAndHeading(50, 50.001, 270, 20, 0);
  testsubject.add(&rec2);

  FGTrafficRecord rec3;
  rec3.setId(26);
  rec3.setPositionAndHeading(50, 50.002, 270, 20, 0);
  testsubject.add(&rec3);

  // Not near
  FGTrafficRecord boatyMcBoatface4;
  boatyMcBoatface4.setId(33);
  boatyMcBoatface4.setPositionAndHeading(50, 50.005, 270, 20, 0);
  testsubject.add(&boatyMcBoatface4);

  for( int i=100; i < 900; i++) {
    // Not near
    FGTrafficRecord boatyMcBoatface4;
    boatyMcBoatface4.setId(i);
    double fraction = 0.01*i;
    boatyMcBoatface4.setPositionAndHeading((50.5+fraction), (50.5+fraction), 270, 20, 0);
    testsubject.add(&boatyMcBoatface4);
  }

  testsubject.move(SGRect<double>(50,50), &rec1);
  rec1.setPositionAndHeading(50, 50, 270, 20, 0);

  CPPUNIT_ASSERT_MESSAGE("Blocker of rec2", testsubject.isBlockedBy(&rec2)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of rec3", testsubject.isBlockedBy(&rec3)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface4 (None)", testsubject.isBlockedBy(&boatyMcBoatface4)==nullptr);
  CPPUNIT_ASSERT_EQUAL(rec1.getId(), testsubject.isBlockedBy(&rec2)->getId());
  CPPUNIT_ASSERT_EQUAL(rec2.getId(), testsubject.isBlockedBy(&rec3)->getId());
}

void AirportGroundRadarTests::testAirport() {
  FGAirportRef egph = FGAirport::getByIdent("EGPH");
  CPPUNIT_ASSERT_MESSAGE("Airport loaded", egph!=nullptr);
  AirportGroundRadar testsubject(egph);
  FGTrafficRecord rec1;
  rec1.setId(8);
  rec1.setPositionAndHeading(50, 50, 270, 20, 0);
  testsubject.add(&rec1);
  
  FGTrafficRecord rec2;
  rec2.setId(2);
  rec2.setPositionAndHeading(50, 50.001, 270, 20, 0);;
  testsubject.add(&rec2);

  FGTrafficRecord rec3;
  rec3.setId(7);
  rec3.setPositionAndHeading(50, 50.002, 270, 20, 0);;
  testsubject.add(&rec3);

  // Not near
  FGTrafficRecord boatyMcBoatface4;
  boatyMcBoatface4.setId(2);
  boatyMcBoatface4.setPositionAndHeading(50, 50.005, 270, 20, 0);
  testsubject.add(&boatyMcBoatface4);

  CPPUNIT_ASSERT_MESSAGE("Blocker of rec2", testsubject.isBlockedBy(&rec2)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of rec3", testsubject.isBlockedBy(&rec3)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface4 (None)", testsubject.isBlockedBy(&boatyMcBoatface4)==nullptr);
  CPPUNIT_ASSERT_EQUAL(rec1.getId(), testsubject.isBlockedBy(&rec2)->getId());
  CPPUNIT_ASSERT_EQUAL(rec2.getId(), testsubject.isBlockedBy(&rec3)->getId());  
}

