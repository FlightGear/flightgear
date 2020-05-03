/*
 * Copyright (C) 2016 Edward d'Auvergne
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


#include "test_props.hxx"

#include <simgear/props/props_io.hxx>

// Set up function for each test.
void SimgearPropsTests::setUp()
{
    // Create a property tree.
    tree = new SGPropertyNode;
}


// Clean up after each test.
void SimgearPropsTests::tearDown()
{
    // Delete the tree (avoiding the memory leak).
    delete tree;
}


// Test property aliasing, to catch possible memory leaks.
void SimgearPropsTests::testAliasLeak()
{
    // Declarations.
    SGPropertyNode *alias;

    // Create a new node.
    tree->getNode("test-node", true);

    // Aliased node.
    alias = tree->getNode("test-alias", true);
    alias->alias("test-node");
}

void SimgearPropsTests::testPropsCopyIf()
{
// dummy property tree structure
    tree->setIntValue("a/a/a", 42);
    tree->setStringValue("a/a/b", "foo");
    tree->setIntValue("a/a/c[2]", 99);
    tree->setIntValue("a/a/d", 1);
    tree->setIntValue("a/b/a[0]", 50);
    tree->setIntValue("a/b/a[1]", 100);
    tree->setStringValue("a/b/b", "foo");

    SGPropertyNode_ptr destA(new SGPropertyNode);

    copyPropertiesIf(tree, destA, [](const SGPropertyNode* src) {
        // always copy non-leaf nodes
        if (src->nChildren() > 0) 
            return true;

        return (src->getType() == simgear::props::INT) &&
            src->getIntValue() > 50;
    });

    CPPUNIT_ASSERT_EQUAL(1, destA->getNode("a/a")->nChildren()); // only 99
    CPPUNIT_ASSERT_EQUAL(99, destA->getIntValue("a/a/c[2]"));
    CPPUNIT_ASSERT_EQUAL(1, destA->getNode("a/b")->nChildren()); // only 100
    CPPUNIT_ASSERT_EQUAL(100, destA->getIntValue("a/b/a[1]"));
}
