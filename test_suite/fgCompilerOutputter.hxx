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


#ifndef _FG_COMPILER_OUTPUTTER_HXX
#define _FG_COMPILER_OUTPUTTER_HXX


#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestFailure.h>

#include "fgTestListener.hxx"


// The custom outputter for the FlightGear test suite.
class fgCompilerOutputter : public CppUnit::CompilerOutputter
{
    public:
        // Constructor.
        fgCompilerOutputter(CppUnit::TestResultCollector *result,
            std::vector<TestIOCapt> *capt,
            const clock_t *clock,
            CppUnit::OStream &stream,
            bool ctest = false,
            bool debug = false,
            const std::string &locationFormat = CPPUNIT_COMPILER_LOCATION_FORMAT)
        : CppUnit::CompilerOutputter(result, stream, locationFormat)
        , io_capt(capt)
        , fg_result(result)
        , fg_stream(stream)
        , suite_timer(clock)
        , ctest_output(ctest)
        , debug(debug)
        {
        }

        // Create a new class instance.
        static fgCompilerOutputter *defaultOutputter(CppUnit::TestResultCollector *result, std::vector<TestIOCapt> *capt, const clock_t *clock, CppUnit::OStream &stream);

        // Print a summary after a successful run of the test suite.
        void printSuccess();

        // Detailed printout after a failed run of the test suite.
        void printFailureReport();

        // Printout for each failed test.
        void printFailureDetail(CppUnit::TestFailure *failure);

        // Printout of the test suite stats.
        void printSuiteStats();

        // The captured IO for each failed test.
        std::vector<TestIOCapt> *io_capt;

    private:
        // Store copies of the base class objects.
        CppUnit::TestResultCollector *fg_result;
        CppUnit::OStream &fg_stream;

        // The test suite time, in clock ticks.
        const clock_t *suite_timer;

        // Output control.
        bool ctest_output;
        bool debug;

        // Simgear logstream IO printout.
        void printIOStreamMessages(std::string heading, std::string messages, bool empty);
        void printIOStreamMessages(std::string heading, std::string messages) {printIOStreamMessages(heading, messages, false);}
};


#endif    // _FG_COMPILER_OUTPUTTER_HXX
