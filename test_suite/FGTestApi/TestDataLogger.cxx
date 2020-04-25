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

#include "TestDataLogger.hxx"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

#include <simgear/io/iostreams/sgstream.hxx>

#include "Main/globals.hxx"

namespace FGTestApi {

using DoubleVec = std::vector<double>;

static std::unique_ptr<DataLogger> static_instance;

class DataLogger::DataLoggerPrivate
{
public:
    sg_ofstream _stream;

    struct SampleInfo {
        int column;
        std::string name;
        // range / units info, later
        SGPropertyNode_ptr property;
    };

    double _currentTimeBase;
    std::vector<SampleInfo> _samples;
    DoubleVec _openRow;
    bool _didHeader = false;

    void writeCurrentRow()
    {
        if (!_didHeader) {
            writeHeader();
            _didHeader = true;
        }

        // capture property values into the open row data
        std::for_each(_samples.begin(), _samples.end(), [this](const SampleInfo& info) {
            if (info.property) {
                _openRow[info.column] = info.property->getDoubleValue();
            }
        });

        // write time base
        _stream << globals->get_sim_time_sec() << ",";

        for (const auto v : _openRow) {
            if (std::isnan(v)) {
                _stream << ","; // skip this data point
            } else {
                _stream << v << ",";
            }
        }

        _stream << "\n";

        std::fill(_openRow.begin(), _openRow.end(), std::numeric_limits<double>::quiet_NaN());
    }

    void writeHeader()
    {
        _stream << "sim-time, ";
        std::for_each(_samples.begin(), _samples.end(), [this](const SampleInfo& info) {
            _stream << info.name << ", ";
        });

        _stream << "\n";
    }
};

DataLogger::DataLogger()
{
    d.reset(new DataLoggerPrivate);
}

DataLogger::~DataLogger()
{
    d->_stream.close();
}

bool DataLogger::isActive()
{
    return static_instance != nullptr;
}

DataLogger* DataLogger::instance()
{
    if (!static_instance) {
        static_instance.reset(new DataLogger);
    }

    return static_instance.get();
}

void DataLogger::initTest(const std::string& testName)
{
    d->_stream = sg_ofstream(testName + "_trace.csv");
}

void DataLogger::tearDown()
{
    if (static_instance) {
        static_instance.reset();
    }
}

void DataLogger::writeRecord()
{
    d->writeCurrentRow();
}

void DataLogger::recordProperty(const std::string& name, SGPropertyNode_ptr prop)
{
    int index = static_cast<int>(d->_samples.size());
    DataLoggerPrivate::SampleInfo info{index, name, prop};
    d->_samples.push_back(info);

    if (d->_openRow.size() <= index) {
        d->_openRow.resize(index + 1, std::numeric_limits<double>::quiet_NaN());
    }
}

void DataLogger::recordSamplePoint(const std::string& name, double value)
{
    auto it = std::find_if(d->_samples.begin(), d->_samples.end(), [&name](const DataLoggerPrivate::SampleInfo& sample) {
        return name == sample.name;
    });

    int index = 0;
    if (it == d->_samples.end()) {
        index = static_cast<int>(d->_samples.size());
        DataLoggerPrivate::SampleInfo info{index, name};
        d->_samples.push_back(info);
    } else {
        index = it->column;
    }

    // grow _openRow as required
    if (d->_openRow.size() <= index) {
        d->_openRow.resize(index + 1, std::numeric_limits<double>::quiet_NaN());
    }

    d->_openRow[index] = value;
}

} // namespace FGTestApi
