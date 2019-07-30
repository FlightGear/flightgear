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

#include <cstring>
#include <iostream>

#include "dataStore.hxx"
#include "fgTestRunner.hxx"
#include "formatting.hxx"
#include "logging.hxx"

using namespace std;


// Print out a summary of the relax test suite.
void summary(CppUnit::OStream &stream, int system_result, int unit_result, int gui_result, int simgear_result, int fgdata_result)
{
    int synopsis = 0;

    // Title.
    string text = "Summary of the FlightGear test suite";
    printTitle(stream, text);

    // Subtitle.
    text = "Synopsis";
    printSection(stream, text);

    // System/functional test summary.
    if (system_result != -1) {
        text = "System/functional tests";
        printSummaryLine(stream, text, system_result);
        synopsis += system_result;
    }

    // Unit test summary.
    if (unit_result != -1) {
        text = "Unit tests";
        printSummaryLine(stream, text, unit_result);
        synopsis += unit_result;
    }

    // GUI test summary.
    if (gui_result != -1) {
        text = "GUI tests";
        printSummaryLine(stream, text, gui_result);
        synopsis += gui_result;
    }

    // Simgear unit test summary.
    if (simgear_result != -1) {
        text = "Simgear unit tests";
        printSummaryLine(stream, text, simgear_result);
        synopsis += simgear_result;
    }

    // FGData test summary.
    if (fgdata_result != -1) {
        text = "FGData tests";
        printSummaryLine(stream, text, fgdata_result);
        synopsis += fgdata_result;
    }

    // Synopsis.
    text ="Synopsis";
    printSummaryLine(stream, text, synopsis);

    // End.
    stream << endl << endl;
}


int main(int argc, char **argv)
{
    // Declarations.
    int         status_gui=-1, status_simgear=-1, status_system=-1, status_unit=-1, status_fgdata=-1;
    bool        run_system=false, run_unit=false, run_gui=false, run_simgear=false, run_fgdata=false;
    bool        verbose=false, ctest_output=false, debug=false, printSummary=true, help=false;
    char        *subset_system=NULL, *subset_unit=NULL, *subset_gui=NULL, *subset_simgear=NULL, *subset_fgdata=NULL;
    char        firstchar;
    std::string arg, fgRoot;

    // Argument parsing.
    for (int i = 1; i < argc; i++) {
        firstchar = '\0';
        arg = argv[i];

        if (i < argc-1)
            firstchar = argv[i+1][0];

        // System test.
        if (arg == "-s" || arg == "--system-tests") {
            run_system = true;
            if (firstchar != '-')
                subset_system = argv[i+1];

        // Unit test.
        } else if (arg == "-u" || arg == "--unit-tests") {
            run_unit = true;
            if (firstchar != '-')
                subset_unit = argv[i+1];

        // GUI test.
        } else if (arg == "-g" || arg == "--gui-tests") {
            run_gui = true;
            if (firstchar != '-')
                subset_gui = argv[i+1];

        // Simgear test.
        } else if (arg == "-m" || arg == "--simgear-tests") {
            run_simgear = true;
            if (firstchar != '-')
                subset_simgear = argv[i+1];

        // FGData test.
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fgdata-tests") == 0) {
            run_fgdata = true;
            if (firstchar != '-')
                subset_fgdata = argv[i+1];

        // Verbose output.
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;

        // CTest suitable output.
        } else if (arg == "-c" || arg == "--ctest") {
            ctest_output = true;

        // Debug output.
        } else if (arg == "-d" || arg == "--debug") {
            debug = true;

        // No summary output.
        } else if (arg == "--no-summary") {
            printSummary = false;

        // Help.
        } else if (arg == "-h" || arg == "--help") {
            help = true;

        // FGData path.
        } else if (arg == "--fg-root") {
            if (firstchar != '-')
                fgRoot = argv[i+1];
        }
    }

    // Help.
    if (help) {
        std::cout << "Usage: run_test_suite [options]" << std::endl << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -h, --help            show this help message and exit." << std::endl;
        std::cout << std::endl;
        std::cout << "  Test selection options:" << std::endl;
        std::cout << "    -s, --system-tests  execute the system/functional tests." << std::endl;
        std::cout << "    -u, --unit-tests    execute the unit tests." << std::endl;
        std::cout << "    -g, --gui-tests     execute the GUI tests." << std::endl;
        std::cout << "    -m, --simgear-tests execute the simgear tests." << std::endl;
        std::cout << "    -f, --fgdata-tests  execute the FGData tests." << std::endl;
        std::cout << std::endl;
        std::cout << "    The -s, -u, -g, and -m options accept an optional argument to perform a" << std::endl;
        std::cout << "    subset of all tests.  This argument should either be the name of a test" << std::endl;
        std::cout << "    suite or the full name of an individual test." << std::endl;
        std::cout << std::endl;
        std::cout << "    Full test names consist of the test suite name, the separator '::' and then" << std::endl;
        std::cout << "    the individual test name.  The test names can revealed with the verbose" << std::endl;
        std::cout << "    option." << std::endl;
        std::cout << std::endl;
        std::cout << "  Verbosity options:" << std::endl;
        std::cout << "    -v, --verbose       verbose output including names and timings for all" << std::endl;
        std::cout << "                        tests." << std::endl;
        std::cout << "    -c, --ctest         simplified output suitable for running via CTest." << std::endl;
        std::cout << "    -d, --debug         disable IO capture for debugging (super verbose output)." << std::endl;
        std::cout << "    --no-summary        disable the final summary printout." << std::endl;
        std::cout << std::endl;
        std::cout << "  FG options:" << std::endl;
        std::cout << "    --fg-root           the path to FGData" << std::endl;
        std::cout.flush();
        return 0;
    }

    // Turn on all tests if no subset was specified.
    if (!run_system && !run_unit && !run_gui && !run_simgear && !run_fgdata) {
        run_system = true;
        run_unit = true;
        run_gui = true;
        run_simgear = true;
        run_fgdata = true;
    }

    // Set up the data store singleton and FGData path.
    DataStore& data = DataStore::get();
    if (data.findFGRoot(fgRoot, debug) != 0) {
        return 1;
    }

    // Set up logging.
    sglog().setDeveloperMode(true);
    if (debug)
        sglog().setLogLevels(SG_ALL, SG_BULK);
    else
        setupLogging();

    // Execute each of the test suite categories.
    if (run_system)
        status_system = testRunner("System tests", "System / functional tests", subset_system, verbose, ctest_output, debug);
    if (run_unit)
        status_unit = testRunner("Unit tests", "Unit tests", subset_unit, verbose, ctest_output, debug);
    if (run_gui && 0) // Disabled as there are no GUI tests yet.
        status_gui = testRunner("GUI tests", "GUI tests", subset_gui, verbose, ctest_output, debug);
    if (run_simgear)
        status_simgear = testRunner("Simgear unit tests", "Simgear unit tests", subset_simgear, verbose, ctest_output, debug);
    if (run_fgdata)
        status_fgdata = testRunner("FGData tests", "FGData tests", subset_fgdata, verbose, ctest_output, debug);

    // Summary printout.
    if (printSummary && !ctest_output)
        summary(cerr, status_system, status_unit, status_gui, status_simgear, status_fgdata);

    // Deactivate the logging.
    if (!debug)
        stopLogging();

    // Failure.
    if (status_system > 0)
        return 1;
    if (status_unit > 0)
        return 1;
    if (status_gui > 0)
        return 1;
    if (status_simgear > 0)
        return 1;
    if (status_fgdata > 0)
        return 1;

    // Success.
    return 0;
}
