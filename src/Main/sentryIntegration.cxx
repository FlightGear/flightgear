// sentryIntegration.cxx - Interface with Sentry.io crash reporting
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

#include "config.h"

#include "sentryIntegration.hxx"

#include <simgear/misc/sg_path.hxx>

#include <Include/version.h>
#include <Main/fg_init.hxx>

using namespace std;

// we don't want sentry enabled for the test suite
#if defined(HAVE_SENTRY) && !defined(BUILDING_TESTSUITE)

static bool static_sentryEnabled = false;

#include <sentry.h>

namespace flightgear
{

void initSentry()
{
    sentry_options_t *options = sentry_options_new();
    sentry_options_set_dsn(options, "https://3a3f0bf24d5d482388dd060860c18ffe@sentry.io/5188535");
    
    if (strcmp(FG_BUILD_TYPE, "Dev") == 0) {
        sentry_options_set_release(options, "flightgear-dev@" FLIGHTGEAR_VERSION);
    } else {
        sentry_options_set_release(options, "flightgear@" FLIGHTGEAR_VERSION);
    }
    
    SGPath dataPath = fgHomePath() / "sentry_db";
#if defined(SG_WINDOWS)
    const auto homePathString = dataPath.wstr();
    sentry_options_set_database_pathw(options, homePathString.c_str());
    
    const auto logPath = (fgHomePath() / "fgfs.log").wstr();
    sentry_options_add_attachmentw(options, "log", logPath.c_str());
#else
    const auto homePathString = dataPath.utf8Str();
    sentry_options_set_database_path(options, homePathString.c_str());
    
    const auto logPath = (fgHomePath() / "fgfs.log").utf8Str();
    sentry_options_add_attachment(options, "log", logPath.c_str());
#endif
    
    sentry_init(options);
    static_sentryEnabled = true;
}

void shutdownSentry()
{
    if (static_sentryEnabled) {
        sentry_shutdown();
        static_sentryEnabled = false;
    }
}

bool isSentryEnabled()
{
    return static_sentryEnabled;
}

void addSentryBreadcrumb(const std::string& msg, const std::string& level)
{
    if (!static_sentryEnabled)
        return;

    sentry_value_t crumb = sentry_value_new_breadcrumb("default", msg.c_str());
    sentry_value_set_by_key(crumb, "level", sentry_value_new_string(level.c_str()));
    sentry_add_breadcrumb(crumb);
}

void addSentryTag(const char* tag, const char* value)
{
    if (!static_sentryEnabled)
        return;

    if (!tag || !value)
        return;
    
    sentry_set_tag(tag, value);
}

void sentryReportNasalError(const std::string& msg, const string_list& stack)
{
    if (!static_sentryEnabled)
        return;
#if 0
    sentry_value_t exc = sentry_value_new_object();
    sentry_value_set_by_key(exc, "type", sentry_value_new_string("Exception"));
    sentry_value_set_by_key(exc, "value", sentry_value_new_string(msg.c_str()));

    sentry_value_t stackData = sentry_value_new_list();
    for (const auto& nasalFrame : stack) {
        sentry_value_append(stackData, sentry_value_new_string(nasalFrame.c_str()));
    }
    sentry_value_set_by_key(exc, "stack", stackData);

    
    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(event, "exception", exc);
    
    // add the Nasal stack trace data
    
    // capture the C++ stack-trace. Probably not that useful but can't hurt
    sentry_event_value_add_stacktrace(event, nullptr, 0);
    
    sentry_capture_event(event);
    
#endif
}

} // of namespace

#else

// stubs for non-sentry case

namespace flightgear
{

void initSentry()
{
}

void shutdownSentry()
{
}

bool isSentryEnabled()
{
    return false;
}

void addSentryBreadcrumb(const std::string&, const std::string&)
{
}

void addSentryTag(const char*, const char*)
{
}

void sentryReportNasalError(const std::string&, const string_list&)
{
}

} // of namespace

#endif

// common helpers

namespace flightgear
{

void addSentryTag(const std::string& tag, const std::string& value)
{
    if (tag.empty() || value.empty())
        return;
    
    addSentryTag(tag.c_str(), value.c_str());
}

} // of namespace flightgear
