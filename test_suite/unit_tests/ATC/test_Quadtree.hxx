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

#pragma once

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <memory>

#include <simgear/props/props.hxx>
#include <simgear/math/SGRect.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

class SGGeod;

class TestObject : public SGReferenced {
  public:
    int id;
    SGRectd pos;
    TestObject(int i, double x, double y) {
        id = i;
        pos.set(x,y,0,0);
    }
};

// The unit tests for the QuadTree.
class QuadtreeTests : public CppUnit::TestFixture
{
    // Set up the test suite.
    CPPUNIT_TEST_SUITE(QuadtreeTests);
    CPPUNIT_TEST(testAdd);
    CPPUNIT_TEST(testAddSplit1);
    CPPUNIT_TEST(testAddSplit2);
    CPPUNIT_TEST(testAddSplit3);
    CPPUNIT_TEST(testAddSplit4);
    CPPUNIT_TEST(testMove);
    CPPUNIT_TEST(testMove2);
    CPPUNIT_TEST_SUITE_END();

	/**Function implementing calculation of dimension for Quadtree*/
    static SGRectd getBox(SGSharedPtr<TestObject> aiObject) {
		return SGRectd((*aiObject).pos.getMin(),
		(*aiObject).pos.getMax());
	};
	/**Function implementing equals for Quadtree*/
    static bool equal(SGSharedPtr<TestObject> o, SGSharedPtr<TestObject> o2) {
		return (*o).id == (*o2).id;
	};

public:
    // Set up function for each test.
    void setUp();

    // Clean up after each test.
    void tearDown();

    // The tests.
    /**Filling of the Quadtree without split*/
    void testAdd();
    /**Filling of the Quadtree with split*/
    void testAddSplit1();
    /**Filling of the Quadtree with split*/
    void testAddSplit2();
    /**Filling of the Quadtree with split*/
    void testAddSplit3();
    /**Filling of the Quadtree with split*/
    void testAddSplit4();
    /**Filling and move*/
    void testMove();
    /**Filling and move*/
    void testMove2();
};
