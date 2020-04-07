// sentryIntegration.hxx - Interface with Sentry.io crash reporting
//
// Copyright (C) 2020 James Turner  james@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#pragma once

#include <string>

namespace flightgear
{
void initSentry();

void shutdownSentry();

bool isSentryEnabled();

void addSentryBreadcrumb(const std::string& msg, const std::string& level);

void addSentryTag(const char* tag, const char* value);

void addSentryTag(const std::string& tag, const std::string& value);


} // of namespace flightgear

