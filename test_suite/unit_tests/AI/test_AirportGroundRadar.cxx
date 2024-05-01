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

    FGAIShip boatyMcBoatface;
    auto rect = testsubject.getBox(&boatyMcBoatface);
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().x());
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().y());

    for (size_t i = 0; i < 4; i++)
    {
      for (size_t j = 0; j < 4; j++) {
        FGAIShip boatyMcBoatface;
        boatyMcBoatface.setLatitude((i/10)+50);
        boatyMcBoatface.setLongitude((j/10)+50);
        testsubject.add(&boatyMcBoatface);
      }
    }
}

void AirportGroundRadarTests::testFillingTreeSplit()
{
    SGGeod minPos = SGGeod::fromDeg(50,50);
    SGGeod maxPos = SGGeod::fromDeg(60,60);

    AirportGroundRadar testsubject(minPos, maxPos);

    FGAIShip boatyMcBoatface;
    auto rect = testsubject.getBox(&boatyMcBoatface);
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().x());
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().y());

    for (size_t i = 0; i < 10; i++)
    {
      for (size_t j = 0; j < 10; j++) {
        FGAIShip boatyMcBoatface;
        boatyMcBoatface.setLatitude((i/10)+50);
        boatyMcBoatface.setLongitude((j/10)+50);
        testsubject.add(&boatyMcBoatface);
      }
    }
}

void AirportGroundRadarTests::testFillingTreeRemove()
{
    SGGeod minPos = SGGeod::fromDeg(50,50);
    SGGeod maxPos = SGGeod::fromDeg(60,60);

    AirportGroundRadar testsubject(minPos, maxPos);

    FGAIShip boatyMcBoatface;
    auto rect = testsubject.getBox(&boatyMcBoatface);
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().x());
    CPPUNIT_ASSERT_EQUAL(0.0, rect.getMin().y());

    for (size_t i = 0; i < 100; i++)
    {
      FGAIShip boatyMcBoatface;
      boatyMcBoatface.setLatitude((i/10)+50);
      boatyMcBoatface.setLongitude((i/10)+50);
      testsubject.add(&boatyMcBoatface);
      testsubject.remove(&boatyMcBoatface);
      CPPUNIT_ASSERT_EQUAL(size_t(0), testsubject.size());
    }

    FGAIShip boatyMcBoatface1;
    boatyMcBoatface1.setLatitude(50);
    boatyMcBoatface1.setLongitude(50);
    testsubject.add(&boatyMcBoatface1);
    FGAIShip boatyMcBoatface2;
    boatyMcBoatface2.setLatitude(50);
    boatyMcBoatface2.setLongitude(50);
    testsubject.add(&boatyMcBoatface2);
    CPPUNIT_ASSERT_EQUAL(size_t(2), testsubject.size());

    testsubject.remove(&boatyMcBoatface1);
    testsubject.remove(&boatyMcBoatface2);

    CPPUNIT_ASSERT_EQUAL(size_t(0), testsubject.size());

}

void AirportGroundRadarTests::testBlocked()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGAIShip boatyMcBoatface1;
  boatyMcBoatface1.setLatitude(50);
  boatyMcBoatface1.setLongitude(50);
  boatyMcBoatface1.setSpeed(20);
  boatyMcBoatface1.setHeading(45);
  testsubject.add(&boatyMcBoatface1);
  FGAIShip boatyMcBoatface2;
  boatyMcBoatface2.setLatitude(50);
  boatyMcBoatface2.setLongitude(50.001);
  boatyMcBoatface2.setSpeed(20);
  boatyMcBoatface2.setHeading(315);
  testsubject.add(&boatyMcBoatface2);
  CPPUNIT_ASSERT_EQUAL(false, testsubject.isBlocked(&boatyMcBoatface1));
  CPPUNIT_ASSERT_EQUAL(false, testsubject.isBlocked(&boatyMcBoatface2));
}

void AirportGroundRadarTests::testBlocked1()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGAIShip boatyMcBoatface1;
  boatyMcBoatface1.setLatitude(50);
  boatyMcBoatface1.setLongitude(50);
  boatyMcBoatface1.setSpeed(20);
  boatyMcBoatface1.setHeading(315);
  testsubject.add(&boatyMcBoatface1);
  FGAIShip boatyMcBoatface2;
  boatyMcBoatface2.setLatitude(50);
  boatyMcBoatface2.setLongitude(50.001);
  boatyMcBoatface2.setSpeed(20);
  boatyMcBoatface2.setHeading(45);
  testsubject.add(&boatyMcBoatface2);
  CPPUNIT_ASSERT_EQUAL(true, testsubject.isBlocked(&boatyMcBoatface1));
  CPPUNIT_ASSERT_EQUAL(false, testsubject.isBlocked(&boatyMcBoatface2));
}

void AirportGroundRadarTests::testBlockedBy1()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGAIShip boatyMcBoatface1;
  boatyMcBoatface1.setLatitude(50);
  boatyMcBoatface1.setLongitude(50);
  boatyMcBoatface1.setSpeed(20);
  boatyMcBoatface1.setHeading(315);
  testsubject.add(&boatyMcBoatface1);
  
  FGAIShip boatyMcBoatface2;
  boatyMcBoatface2.setLatitude(50);
  boatyMcBoatface2.setLongitude(50.001);
  boatyMcBoatface2.setSpeed(20); 
  boatyMcBoatface2.setHeading(45);
  testsubject.add(&boatyMcBoatface2);

  FGAIShip boatyMcBoatface3;
  boatyMcBoatface3.setLatitude(50);
  boatyMcBoatface3.setLongitude(50.003);
  boatyMcBoatface3.setSpeed(20);
  boatyMcBoatface3.setHeading(45);
  testsubject.add(&boatyMcBoatface3);

  CPPUNIT_ASSERT(testsubject.isBlockedBy(&boatyMcBoatface2)==nullptr);
  CPPUNIT_ASSERT_EQUAL(boatyMcBoatface2.getID(), testsubject.isBlockedBy(&boatyMcBoatface1)->getID());
}

void AirportGroundRadarTests::testBlockedByQueue()
{
  SGGeod minPos = SGGeod::fromDeg(50,50);
  SGGeod maxPos = SGGeod::fromDeg(60,60);

  AirportGroundRadar testsubject(minPos, maxPos);
  FGAIShip boatyMcBoatface1;
  boatyMcBoatface1.setLatitude(50);
  boatyMcBoatface1.setLongitude(50);
  boatyMcBoatface1.setSpeed(20);
  boatyMcBoatface1.setHeading(270);
  testsubject.add(&boatyMcBoatface1);
  
  FGAIShip boatyMcBoatface2;
  boatyMcBoatface2.setLatitude(50);
  boatyMcBoatface2.setLongitude(50.001);
  boatyMcBoatface2.setSpeed(20); 
  boatyMcBoatface2.setHeading(270);
  testsubject.add(&boatyMcBoatface2);

  FGAIShip boatyMcBoatface3;
  boatyMcBoatface3.setLatitude(50);
  boatyMcBoatface3.setLongitude(50.002);
  boatyMcBoatface3.setSpeed(20);
  boatyMcBoatface3.setHeading(270);
  testsubject.add(&boatyMcBoatface3);

  // Not near
  FGAIShip boatyMcBoatface4;
  boatyMcBoatface4.setLatitude(50);
  boatyMcBoatface4.setLongitude(50.005);
  boatyMcBoatface4.setSpeed(20);
  boatyMcBoatface4.setHeading(270);
  testsubject.add(&boatyMcBoatface4);

  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface2", testsubject.isBlockedBy(&boatyMcBoatface2)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface3", testsubject.isBlockedBy(&boatyMcBoatface3)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface4 (None)", testsubject.isBlockedBy(&boatyMcBoatface4)==nullptr);
  CPPUNIT_ASSERT_EQUAL(boatyMcBoatface1.getID(), testsubject.isBlockedBy(&boatyMcBoatface2)->getID());
  CPPUNIT_ASSERT_EQUAL(boatyMcBoatface2.getID(), testsubject.isBlockedBy(&boatyMcBoatface3)->getID());
}

void AirportGroundRadarTests::testAirport() {
  FGAirportRef egph = FGAirport::getByIdent("EGPH");
  CPPUNIT_ASSERT_MESSAGE("Airport loaded", egph!=nullptr);
  AirportGroundRadar testsubject(egph);
  FGAIShip boatyMcBoatface1;
  boatyMcBoatface1.setLatitude(50);
  boatyMcBoatface1.setLongitude(50);
  boatyMcBoatface1.setSpeed(20);
  boatyMcBoatface1.setHeading(270);
  testsubject.add(&boatyMcBoatface1);
  
  FGAIShip boatyMcBoatface2;
  boatyMcBoatface2.setLatitude(50);
  boatyMcBoatface2.setLongitude(50.001);
  boatyMcBoatface2.setSpeed(20); 
  boatyMcBoatface2.setHeading(270);
  testsubject.add(&boatyMcBoatface2);

  FGAIShip boatyMcBoatface3;
  boatyMcBoatface3.setLatitude(50);
  boatyMcBoatface3.setLongitude(50.002);
  boatyMcBoatface3.setSpeed(20);
  boatyMcBoatface3.setHeading(270);
  testsubject.add(&boatyMcBoatface3);

  // Not near
  FGAIShip boatyMcBoatface4;
  boatyMcBoatface4.setLatitude(50);
  boatyMcBoatface4.setLongitude(50.005);
  boatyMcBoatface4.setSpeed(20);
  boatyMcBoatface4.setHeading(270);
  testsubject.add(&boatyMcBoatface4);

  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface2", testsubject.isBlockedBy(&boatyMcBoatface2)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface3", testsubject.isBlockedBy(&boatyMcBoatface3)!=nullptr);
  CPPUNIT_ASSERT_MESSAGE("Blocker of boatyMcBoatface4 (None)", testsubject.isBlockedBy(&boatyMcBoatface4)==nullptr);
  CPPUNIT_ASSERT_EQUAL(boatyMcBoatface1.getID(), testsubject.isBlockedBy(&boatyMcBoatface2)->getID());
  CPPUNIT_ASSERT_EQUAL(boatyMcBoatface2.getID(), testsubject.isBlockedBy(&boatyMcBoatface3)->getID());  
}

