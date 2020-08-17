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


#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "fgCompilerOutputter.hxx"
#include "fgTestListener.hxx"
#include "formatting.hxx"


// Execute all test suites for the given test category.
int testRunner(const std::string& type, const std::string& title, char *subset, bool timings, bool ctest_output, bool debug)
{
    // Declarations.
    CppUnit::TextTestRunner runner;

    // Print out a title.
    if (!ctest_output)
        printTitle(std::cerr, title);

    // Get all tests.
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry(type).makeTest());

    // Set up the test listener.
    fgTestListener *testListener;
    testListener = new fgTestListener;
    runner.eventManager().addListener(testListener);
    testListener->timings = timings;
    testListener->ctest_output = ctest_output;
    testListener->debug = debug;

    // Set the test suite output IO stream.
    runner.setOutputter(new fgCompilerOutputter(&runner.result(), &testListener->io_capt, &testListener->sum_time, std::cerr, ctest_output, debug));

    // Execute the tests.
    if (subset == NULL)
        runner.run("", false, true, false);
    else {
        std::stringstream testStream(subset);
        std::string testName;
        while( testStream.good() ) {
            getline( testStream, testName, ',' );
            runner.run(testName, false, true, false);
        }
    }

    // Clean up.
    delete testListener;

    // Return the status of the tests.
    CppUnit::TestResultCollector &status = runner.result();
    return status.testFailuresTotal();
}
