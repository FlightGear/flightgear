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


int main(void)
{
    // Declarations.
    int status_gui, status_simgear, status_system, status_unit;

    // Execute each of the test suite categories.
    status_system = testRunner("System tests");
    status_unit = testRunner("Unit tests");
    status_gui = testRunner("GUI tests");
    status_simgear = testRunner("Simgear unit tests");

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
