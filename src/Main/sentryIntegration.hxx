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
#include <simgear/misc/strutils.hxx>

namespace flightgear
{
void initSentry();

void shutdownSentry();

void delayedSentryInit();

bool isSentryEnabled();

void addSentryBreadcrumb(const std::string& msg, const std::string& level);

void addSentryTag(const char* tag, const char* value);

void addSentryTag(const std::string& tag, const std::string& value);

void sentryReportNasalError(const std::string& msg, const string_list& stack);

void sentryReportException(const std::string& msg, const std::string& location = {});

void sentryReportFatalError(const std::string& msg, const std::string& more = {});

/**
 * @brief helper to allow per-thread supression of 
 * error reports based on XML parse/include errors. 
 * This is to reduce noise from the launhcer scanning large
 * directories of non-fixlbe aircraft, in a helper thread.
 * 
 * We do it at this level since we don't want to modify
 * the SimGear XML parser, so we set a thread-local flag
 * and use it to avoid reporting the exception when it
 * occurs.
 */
void sentryThreadReportXMLErrors(bool report);

} // of namespace flightgear

