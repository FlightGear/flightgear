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

#include <algorithm>
#include <cstring>
#include <iostream>

#include <simgear/debug/logstream.hxx>

#include "dataStore.hxx"
#include "fgTestRunner.hxx"
#include "formatting.hxx"
#include "logging.hxx"

using namespace std;


// The help message.
void helpPrintout(std::ostream &stream) {
    stream << "Usage: run_test_suite [options]" << std::endl << std::endl;
    stream << "Options:" << std::endl;
    stream << "  -h, --help            show this help message and exit." << std::endl;
    stream << std::endl;
    stream << "  Test selection options:" << std::endl;
    stream << "    -s, --system-tests  execute the system/functional tests." << std::endl;
    stream << "    -u, --unit-tests    execute the unit tests." << std::endl;
    stream << "    -g, --gui-tests     execute the GUI tests." << std::endl;
    stream << "    -m, --simgear-tests execute the simgear tests." << std::endl;
    stream << "    -f, --fgdata-tests  execute the FGData tests." << std::endl;
    stream << std::endl;
    stream << "    The -s, -u, -g, and -m options accept an optional argument to perform a" << std::endl;
    stream << "    subset of all tests.  This argument should either be the name of a test" << std::endl;
    stream << "    suite or the full name of an individual test." << std::endl;
    stream << std::endl;
    stream << "    Full test names consist of the test suite name, the separator '::' and then" << std::endl;
    stream << "    the individual test name.  The test names can revealed with the verbose" << std::endl;
    stream << "    option." << std::endl;
    stream << std::endl;
    stream << "  Logging options:" << std::endl;
    stream << "    --log-level={bulk,debug,info,warn,alert,popup,dev_warn,dev_alert}" << std::endl;
    stream << "                        specify the minimum logging level to output" << std::endl;
    stream << "    --log-class=[none, terrain, astro, flight, input, gl, view, cockpit," << std::endl;
    stream << "                 general, math, event, aircraft, autopilot, io, clipper," << std::endl;
    stream << "                 network, atc, nasal, instrumentation, systems, ai, environment," << std::endl;
    stream << "                 sound, navaid, gui, terrasync, particles, headless, osg," << std::endl;
    stream << "                 undefined, all]" << std::endl;
    stream << "                        select the logging class(es) to output." << std::endl;
    stream << "    --log-split         output the different non-interleaved log streams" << std::endl;
    stream << "                        sequentially." << std::endl;
    stream << std::endl;
    stream << "  Verbosity options:" << std::endl;
    stream << "    -t, --timings       verbose output including names and timings for all" << std::endl;
    stream << "                        tests." << std::endl;
    stream << "    -c, --ctest         simplified output suitable for running via CTest." << std::endl;
    stream << "    -d, --debug         disable IO capture for debugging (super verbose output)." << std::endl;
    stream << "    --no-summary        disable the final summary printout." << std::endl;
    stream << std::endl;
    stream << "  FG options:" << std::endl;
    stream << "    --fg-root           the path to FGData." << std::endl;
    stream << std::endl;
    stream << "Environmental variables:" << std::endl;
    stream << "  FG_TEST_LOG_LEVEL     equivalent to the --log-level option." << std::endl;
    stream << "  FG_TEST_LOG_CLASS     equivalent to the --log-class option." << std::endl;
    stream << "  FG_TEST_LOG_SPLIT     equivalent to the --log-split option." << std::endl;
    stream << "  FG_TEST_TIMINGS       equivalent to the -t or --timings option." << std::endl;
    stream << "  FG_TEST_DEBUG         equivalent to the -d or --debug option." << std::endl;
    stream << "  FG_ROOT               the path to FGData.  The order of precedence is" << std::endl;
    stream << "                         --fg-root, the FG_DATA_DIR CMake option, FG_ROOT," << std::endl;
    stream << "                        '../fgdata/', and '../data/'." << std::endl;
    stream.flush();
}


// Convert the log class comma separated string list into a simgear debug class value.
sgDebugClass processLogClass(std::string classList, bool &failure) {
    // Declarations.
    std::string logClassItem, val;
    unsigned int logClass=0;

    // Convert the command line value into a string array.
    std::replace(classList.begin(), classList.end(), ',', ' ');
    std::vector<std::string> logClasses;
    stringstream temp(classList);
    while (temp >> val)
        logClasses.push_back(val);

    // Build up the value.
    for (auto const& logClassItem: logClasses)
        if (logClassItem == "none")
            logClass |= SG_NONE;
        else if (logClassItem == "terrain")
            logClass |= SG_TERRAIN;
        else if (logClassItem == "astro")
            logClass |= SG_ASTRO;
        else if (logClassItem == "flight")
            logClass |= SG_FLIGHT;
        else if (logClassItem == "input")
            logClass |= SG_INPUT;
        else if (logClassItem == "gl")
            logClass |= SG_GL;
        else if (logClassItem == "view")
            logClass |= SG_VIEW;
        else if (logClassItem == "cockpit")
            logClass |= SG_COCKPIT;
        else if (logClassItem == "general")
            logClass |= SG_GENERAL;
        else if (logClassItem == "math")
            logClass |= SG_MATH;
        else if (logClassItem == "event")
            logClass |= SG_EVENT;
        else if (logClassItem == "aircraft")
            logClass |= SG_AIRCRAFT;
        else if (logClassItem == "autopilot")
            logClass |= SG_AUTOPILOT;
        else if (logClassItem == "io")
            logClass |= SG_IO;
        else if (logClassItem == "clipper")
            logClass |= SG_CLIPPER;
        else if (logClassItem == "network")
            logClass |= SG_NETWORK;
        else if (logClassItem == "atc")
            logClass |= SG_ATC;
        else if (logClassItem == "nasal")
            logClass |= SG_NASAL;
        else if (logClassItem == "instrumentation")
            logClass |= SG_INSTR;
        else if (logClassItem == "systems")
            logClass |= SG_SYSTEMS;
        else if (logClassItem == "ai")
            logClass |= SG_AI;
        else if (logClassItem == "environment")
            logClass |= SG_ENVIRONMENT;
        else if (logClassItem == "sound")
            logClass |= SG_SOUND;
        else if (logClassItem == "navaid")
            logClass |= SG_NAVAID;
        else if (logClassItem == "gui")
            logClass |= SG_GUI;
        else if (logClassItem == "terrasync")
            logClass |= SG_TERRASYNC;
        else if (logClassItem == "particles")
            logClass |= SG_PARTICLES;
        else if (logClassItem == "headless")
            logClass |= SG_HEADLESS;
        else if (logClassItem == "osg")
            logClass |= SG_OSG;
        else if (logClassItem == "undefined")
            logClass |= SG_UNDEFD;
        else if (logClassItem == "all")
            logClass |= SG_ALL;
        else {
            std::cout << "The log class \"" << logClassItem << "\" must be one of:" << std::endl;
            std::cout << "    {none, terrain, astro, flight, input, gl, view, cockpit, general, math," << std::endl;
            std::cout << "    event, aircraft, autopilot, io, clipper, network, atc, nasal," << std::endl;
            std::cout << "    instrumentation, systems, ai, environment, sound, navaid, gui, terrasync," << std::endl;
            std::cout << "    particles, headless, osg, undefined, all}" << std::endl << std::endl;
            std::cout.flush();
            failure = true;
        }

    // Return a simgear debug class.
    return sgDebugClass(logClass);
}


// Convert the log priority string into a simgear debug priority value.
sgDebugPriority processLogPriority(std::string logLevel, bool &failure) {
    // Declarations.
    sgDebugPriority logPriority=SG_INFO;

    // Conversion.
    if (logLevel == "bulk")
        logPriority = SG_BULK;
    else if (logLevel == "debug")
        logPriority = SG_DEBUG;
    else if (logLevel == "info")
        logPriority = SG_INFO;
    else if (logLevel == "warn")
        logPriority = SG_WARN;
    else if (logLevel == "alert")
        logPriority = SG_ALERT;
    else if (logLevel == "popup")
        logPriority = SG_POPUP;
    else if (logLevel == "dev_warn")
        logPriority = SG_DEV_WARN;
    else if (logLevel == "dev_alert")
        logPriority = SG_DEV_ALERT;
    else {
        std::cout << "The log level setting of \"" << logLevel << "\" must be one of {bulk,debug,info,warn,alert,popup,dev_warn,dev_alert}.\n\n";
        std::cout.flush();
        failure = true;
    }

    // Return the simgear debug priority.
    return logPriority;
}


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
    bool        logSplit=false;
    bool        timings=false, ctest_output=false, debug=false, printSummary=true, help=false;
    char        *subset_system=NULL, *subset_unit=NULL, *subset_gui=NULL, *subset_simgear=NULL, *subset_fgdata=NULL;
    bool        failure=false;
    char        firstchar;
    std::string arg, delim, fgRoot, logClassVal, logLevel;
    size_t      delimPos;

    // The default logging class and priority to show.
    sgDebugClass    logClass=SG_ALL;
    sgDebugPriority logPriority=SG_INFO;

    // Process environmental variables before the command line options.
    if (getenv("FG_TEST_LOG_LEVEL"))
        logPriority = processLogPriority(getenv("FG_TEST_LOG_LEVEL"), failure);
    if (getenv("FG_TEST_LOG_CLASS"))
        logClass = processLogClass(getenv("FG_TEST_LOG_CLASS"), failure);
    if (getenv("FG_TEST_LOG_SPLIT"))
        logSplit = true;
    if (getenv("FG_TEST_TIMINGS"))
        timings = true;
    if (getenv("FG_TEST_DEBUG"))
        debug = true;
    if (failure)
        return 1;

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

        // Log class.
        } else if (arg.find( "--log-class" ) == 0) {
            // Process the command line.
            logClass = SG_NONE;
            delimPos = arg.find('=');
            logClassVal = arg.substr(delimPos + 1);
            logClass = processLogClass(logClassVal, failure);
            if (failure)
                return 1;

        // Log level.
        } else if (arg.find( "--log-level" ) == 0) {
            // Process the command line level.
            delimPos = arg.find('=');
            logLevel = arg.substr(delimPos + 1);
            logPriority = processLogPriority(logLevel, failure);
            if (failure)
                return 1;

        // Log splitting.
        } else if (arg == "--log-split") {
            logSplit = true;

        // Timing output.
        } else if (arg == "-t" || arg == "--timings") {
            timings = true;

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
        helpPrintout(std::cout);
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
    if (data.validateFGRoot() != 0) {
        return 1;
    }

    // Set up logging.
    sglog().setDeveloperMode(true);
    if (debug)
        sglog().setLogLevels(sgDebugClass(logClass), logPriority);
    else
        setupLogging(sgDebugClass(logClass), logPriority, logSplit);

    // Execute each of the test suite categories.
    if (run_system)
        status_system = testRunner("System tests", "System / functional tests", subset_system, timings, ctest_output, debug);
    if (run_unit)
        status_unit = testRunner("Unit tests", "Unit tests", subset_unit, timings, ctest_output, debug);
    if (run_gui && 0) // Disabled as there are no GUI tests yet.
        status_gui = testRunner("GUI tests", "GUI tests", subset_gui, timings, ctest_output, debug);
    if (run_simgear)
        status_simgear = testRunner("Simgear unit tests", "Simgear unit tests", subset_simgear, timings, ctest_output, debug);
    if (run_fgdata)
        status_fgdata = testRunner("FGData tests", "FGData tests", subset_fgdata, timings, ctest_output, debug);

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
