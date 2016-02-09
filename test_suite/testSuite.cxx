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

#include <iostream>

#include "fgTestRunner.hxx"
#include "formatting.hxx"

using namespace std;


// Print out a summary of the relax test suite.
void summary(CppUnit::OStream &stream, int system_result, int unit_result, int gui_result, int simgear_result)
{
    // Title.
    string text = "Summary of the FlightGear test suite";
    printTitle(stream, text);

    // Subtitle.
    text = "Synopsis";
    printSection(stream, text);

    // System/functional test summary.
    text = "System/functional tests";
    printSummaryLine(stream, text, system_result);

    // Unit test summary.
    text = "Unit tests";
    printSummaryLine(stream, text, unit_result);

    // GUI test summary.
    text = "GUI tests";
    printSummaryLine(stream, text, gui_result);

    // Simgear unit test summary.
    text = "Simgear unit tests";
    printSummaryLine(stream, text, simgear_result);

    // Synopsis.
    text ="Synopsis";
    int synopsis = system_result + unit_result + gui_result + simgear_result;
    printSummaryLine(stream, text, synopsis);

    // End.
    stream << endl << endl;
}


int main(void)
{
    // Declarations.
    int status_gui, status_simgear, status_system, status_unit;

    // Execute each of the test suite categories.
    status_system = testRunner("System tests");
    status_unit = testRunner("Unit tests");
    status_gui = testRunner("GUI tests");
    status_simgear = testRunner("Simgear unit tests");

    // Summary printout.
    summary(cerr, status_system, status_unit, status_gui, status_simgear);

    // Failure.
    if (status_system > 0)
        return 1;
    if (status_unit > 0)
        return 1;
    if (status_gui > 0)
        return 1;
    if (status_simgear > 0)
        return 1;

    // Success.
    return 0;
}
