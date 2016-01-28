/*
 * Copyright (C) 2016-2018 Edward d'Auvergne
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


#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>


// Execute all test suites for the given test category.
int testRunner(const std::string& title)
{
    // Declarations.
    CppUnit::TextTestRunner runner;

    // Get all tests.
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry(title).makeTest());

    // Set the test suite output IO stream.
    runner.setOutputter(CppUnit::CompilerOutputter::defaultOutputter(&runner.result(), std::cerr));

    // Execute the tests.
    runner.run("", false, true, false);

    // Return the status of the tests.
    CppUnit::TestResultCollector &status = runner.result();
    return status.testFailures();
}
