/*
 * Copyright (C) 2021 James Turner
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

#include <simgear/structure/subsystem_mgr.hxx>

namespace flightgear {

class ErrorReporter : public SGSubsystem
{
public:
    ErrorReporter();
    ~ErrorReporter();

    void preinit();

    void init() override;
    void bind() override;
    void unbind() override;

    void update(double dt) override;

    void shutdown() override;

    static const char* staticSubsystemClassId() { return "error-reporting"; }

    static std::string threadSpecificContextValue(const std::string& key);

private:
    class ErrorReporterPrivate;

    std::unique_ptr<ErrorReporterPrivate> d;
};

} // namespace flightgear
