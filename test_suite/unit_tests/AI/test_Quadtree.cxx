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

#include "test_Quadtree.hxx"

#include "ATC/QuadTree.hxx"

#include <cstring>
#include <memory>

/////////////////////////////////////////////////////////////////////////////

// Set up function for each test.
void QuadtreeTests::setUp()
{
}

// Clean up after each test.
void QuadtreeTests::tearDown()
{
}

void QuadtreeTests::testAdd()
{
	quadtree::QuadTree<TestObject, decltype(&getBox), decltype(&equal)> index (getBox, equal);
    index.resize(SGRectd(0, 0, 2, 2));
    int id = 0;
    // All 4 Quadrants
    CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 0.5,0.5)));
    CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 1.5,0.5)));
    CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 0.5,1.5)));
    CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 1.5,1.5)));
    // Edges 4 Quadrants
    CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 0,1)));
    CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 2,1)));
    CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 1,0)));
    CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 1,2)));
    // Outside
    CPPUNIT_ASSERT_EQUAL(false, index.add(new TestObject(id++, -1.5, -1.5)));
    CPPUNIT_ASSERT_EQUAL(false, index.add(new TestObject(id++, -1.5, 2.5)));
    CPPUNIT_ASSERT_EQUAL(false, index.add(new TestObject(id++, 2.5, -1.5)));
    CPPUNIT_ASSERT_EQUAL(false, index.add(new TestObject(id++, 2.5, 2.5)));
}

void QuadtreeTests::testAddSplit1()
{
	quadtree::QuadTree<TestObject, decltype(&getBox), decltype(&equal)> index (getBox, equal);
    index.resize(SGRectd(0, 0, 2, 2));
    int id = 0;
    // Add lots
    for( int i= 1; i<=190; ++i) {
        double incr = ((double)1)/i;
        CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 0.1+incr,0.1+incr)));
    }
}

void QuadtreeTests::testAddSplit2()
{
	quadtree::QuadTree<TestObject, decltype(&getBox), decltype(&equal)> index (getBox, equal);
    index.resize(SGRectd(0, 0, 2, 2));
    int id = 0;
    // Add lots
    for( int i= 1; i<=190; ++i) {
        double incr = ((double)1)/i;
        CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 2-incr,0.1+incr)));
    }
}

void QuadtreeTests::testAddSplit3()
{
	quadtree::QuadTree<TestObject, decltype(&getBox), decltype(&equal)> index (getBox, equal);
    index.resize(SGRectd(0, 0, 2, 2));
    int id = 0;
    // Add lots
    for( int i= 1; i<=190; ++i) {
        double incr = ((double)1)/i;
        CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 2-incr, 2-incr)));
    }
}

void QuadtreeTests::testAddSplit4()
{
	quadtree::QuadTree<TestObject, decltype(&getBox), decltype(&equal)> index (getBox, equal);
    index.resize(SGRectd(0, 0, 2, 2));
    int id = 0;
    // Add lots
    for( int i= 1; i<=190; ++i) {
        double incr = ((double)1)/i;
        CPPUNIT_ASSERT_EQUAL(true, index.add(new TestObject(id++, 0.1+incr,2-incr)));
    }
}
