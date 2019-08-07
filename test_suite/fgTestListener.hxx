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


#ifndef _FG_TEST_LISTENER_HXX
#define _FG_TEST_LISTENER_HXX


#include <time.h>
#include <vector>

#include "formatting.hxx"


// Data structure for holding the captured IO for a failed test.
struct TestIOCapt {
    std::string name;
    std::string log_class;
    std::string log_priority;
    std::string stdio;
    std::string sg_interleaved;
    std::string sg_bulk_only;
    std::string sg_debug_only;
    std::string sg_info_only;
    std::string sg_warn_only;
    std::string sg_alert_only;
};


// Match the test by name for std:find using a vector<TestIOCapt>.
class matchTestName
{
    std::string _name;

public:
    matchTestName(const std::string &name) : _name(name) {}

    bool operator()(const TestIOCapt &item) const {
        return item.name == _name;
    }
};


// The custom test runner for the FlightGear test suite.
class fgTestListener : public CppUnit::TestListener
{
protected:
    // Failure state.
    bool m_failure, m_error;

    public:
        // Constructor.
        fgTestListener(): m_failure(false), m_error(false), sum_time(0) { };

        // Override the base class function to capture IO. streams
        void startTest(CppUnit::Test *test);

        // Override the base class function to restore IO streams.
        void endTest(CppUnit::Test *test);

        // Handle failures.
        void addFailure(const CppUnit::TestFailure &failure);

        // Test suite timing.
        clock_t sum_time;

        // IO capture for all failed tests.
        std::vector<TestIOCapt> io_capt;

        // Output settings.
        bool timings;
        bool ctest_output;
        bool debug;

    protected:
        // The original IO streams.
        std::streambuf *orig_cerr, *orig_cout;

        // Captured IO streams.
        std::stringstream capt;

        // Test timings.
        clock_t m_time;
};


#endif  // _FG_TEST_LISTENER_HXX
