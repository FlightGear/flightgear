/*
 * Copyright (C) 2020 James Turner
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

#pragma once

#include <memory>
#include <string>

#include <simgear/props/props.hxx>

namespace FGTestApi {

class DataLogger
{
public:
    ~DataLogger();

    static DataLogger* instance();
    static bool isActive();

    void initTest(const std::string& testName);

    void recordProperty(const std::string& name, SGPropertyNode_ptr prop);

    void setUp();

    void tearDown();

    void recordSamplePoint(const std::string& name, double value);

    void writeRecord();

private:
    DataLogger();

    class DataLoggerPrivate;
    std::unique_ptr<DataLoggerPrivate> d;
};


} // namespace FGTestApi
