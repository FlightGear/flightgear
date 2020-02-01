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


#include <algorithm>
#include <iomanip>

#include <cppunit/SourceLine.h>
#include <cppunit/TestResultCollector.h>

#include "fgCompilerOutputter.hxx"
#include "formatting.hxx"
#include "logging.hxx"

using namespace std;


// Create a new class instance.
fgCompilerOutputter * fgCompilerOutputter::defaultOutputter(CppUnit::TestResultCollector *result, vector<TestIOCapt> *capt, const clock_t *clock, CppUnit::OStream &stream)
{
    return new fgCompilerOutputter(result, capt, clock, stream);
}


// Printout for each failed test.
void fgCompilerOutputter::printFailureDetail(CppUnit::TestFailure *failure)
{
    // Declarations.
    TestIOCapt test_io;
    vector<TestIOCapt>::iterator test_iter;

    // Initial separator.
#ifdef _WIN32
    fg_stream << endl;
#endif
    fg_stream << string(WIDTH_DIVIDER, '=') << endl;

    // Test info.
    fg_stream << (failure->isError() ? "ERROR: " : "FAIL: ") << failure->failedTestName() << endl;
    fg_stream << string(WIDTH_DIVIDER, '-') << endl;
    fg_stream << (failure->isError() ? "Error" : "Assertion") << ": ";
    printFailureLocation(failure->sourceLine());
    printFailureMessage(failure);
    fg_stream.flush();

    if (debug)
        return;

    // The captured IO for this test.
    test_iter = find_if(io_capt->begin(), io_capt->end(), matchTestName(failure->failedTestName()));
    if (test_iter != io_capt->end())
        test_io = *test_iter;

    // SG_LOG IO streams.
    if (!test_io.sg_interleaved.empty())
        fgCompilerOutputter::printIOStreamMessages("SG_LOG, "+test_io.log_class+" class, "+test_io.log_priority+" priority", test_io.sg_interleaved, true);
    if (!test_io.sg_bulk_only.empty())
        fgCompilerOutputter::printIOStreamMessages("SG_LOG, "+test_io.log_class+" class, SG_BULK only priority", test_io.sg_bulk_only);
    if (!test_io.sg_debug_only.empty())
        fgCompilerOutputter::printIOStreamMessages("SG_LOG, "+test_io.log_class+" class, SG_DEBUG only priority", test_io.sg_debug_only);
    if (!test_io.sg_info_only.empty())
        fgCompilerOutputter::printIOStreamMessages("SG_LOG, "+test_io.log_class+" class, SG_INFO only priority", test_io.sg_info_only);
    if (!test_io.sg_warn_only.empty())
        fgCompilerOutputter::printIOStreamMessages("SG_LOG, "+test_io.log_class+" class, SG_WARN only priority", test_io.sg_warn_only);
    if (!test_io.sg_alert_only.empty())
        fgCompilerOutputter::printIOStreamMessages("SG_LOG, "+test_io.log_class+" class, SG_ALERT only priority", test_io.sg_alert_only);

    // Default IO streams.
    fgCompilerOutputter::printIOStreamMessages("STDOUT and STDERR", test_io.stdio);
}


// Detailed printout after a failed run of the test suite.
void fgCompilerOutputter::printFailureReport()
{
    // Custom printouts for each failed test.
    printFailuresList();

    // CTest output (nothing).
    if (ctest_output)
        return;

    // A summary with timing info.
    printSuiteStats();

    // Final summary.
    fg_stream << endl << "[ FAILED ]" << endl << endl;
    fg_stream.flush();
}


void fgCompilerOutputter::printIOStreamMessages(string heading, string messages, bool empty)
{
    // Silence.
    if (!empty && messages.size() == 0)
        return;

    // Divider.
    fg_stream << string(WIDTH_DIVIDER, '-') << endl;

    // Heading.
    fg_stream << "# " << heading << endl << endl;

    // Nothing to do
    if (messages.size() == 0)
        fg_stream << "(empty)" << endl << endl;

    // The IO stream contents.
    else
        fg_stream << messages << endl;
}


// Printout of the test suite stats.
void fgCompilerOutputter::printSuiteStats()
{
    // A divider.
#ifdef _WIN32
    fg_stream << endl;
#endif
    fg_stream << string(WIDTH_DIVIDER, '-') << endl;

    // Timing and test count line.
    fg_stream << "Ran " << fg_result->runTests() << " tests";
    streamsize prec = fg_stream.precision();
    fg_stream << setprecision(3);
    fg_stream << " in " << ((double)*suite_timer)/CLOCKS_PER_SEC << " seconds." << endl;
    fg_stream << setprecision(prec);

    // Failure line.
    if (!fg_result->wasSuccessful()) {
        fg_stream << endl << "Failures = " << fg_result->testFailures() << endl;
        fg_stream << "Errors   = " << fg_result->testErrors() << endl;
    }
}


// Print a summary after a successful run of the test suite.
void fgCompilerOutputter::printSuccess()
{
    // CTest output (nothing).
    if (ctest_output)
        return;

    // A summary with timing info.
    printSuiteStats();

    // Final summary.
    fg_stream << endl << "[ OK ]" << endl << endl;
    fg_stream.flush();
}
