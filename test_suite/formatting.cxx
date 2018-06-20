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


#include <string>

#include "formatting.hxx"

using namespace std;


// Printout of a section label.
void printSection(CppUnit::OStream &stream, const string &text)
{
    // Declarations.
    string::size_type width = text.size();

    // Format the text.
    stream << endl;
    stream << text << endl;
    stream << string(width, '=') << endl << endl;
}


// Print a summary line.
void printSummaryLine(CppUnit::OStream &stream, const string &name, int passed)
{
    // Declarations.
    int n;
    string dots, state;

    // Passed.
    if (!passed)
        state = "OK";

    // Skipped.
    else if (passed == -1)
        state = "Skipped";

    // Failed.
    else
        state = "Failed";

    // Dots.
    n = WIDTH_DIVIDER - name.size() - state.size() - 6;
    dots = string(n, '.');

    // Write out the line.
    stream << name;
    stream << " " << dots << " ";
    stream << "[ " << state << " ]" << endl;
}


// Printout of a title label.
void printTitle(CppUnit::OStream &stream, const string &text)
{
    // Declarations.
    string::size_type width = text.size() + 4;

    // Format the text.
    stream << endl << endl;
    stream << string(width, '=') << endl;
    stream << "= " << text << " =" << endl;
    stream << string(width, '=') << endl << endl;
}
