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

#include <cstring> // for strcmp

#include <simgear/debug/LogCallback.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>

#include <Main/fg_init.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include <flightgearBuildId.h>

using namespace std;

// we don't want sentry enabled for the test suite
#if defined(HAVE_SENTRY) && !defined(BUILDING_TESTSUITE)

static bool static_sentryEnabled = false;

#include <sentry.h>

namespace {

class SentryLogCallback : public simgear::LogCallback
{
public:
    SentryLogCallback() : simgear::LogCallback(SG_ALL, SG_WARN)
    {
    }

    bool doProcessEntry(const simgear::LogEntry& e) override
    {
        // we need original priority here, so we don't record MANDATORY_INFO
        // or DEV_ messages, which would get noisy.
        const auto op = e.originalPriority;
        if ((op != SG_WARN) && (op != SG_ALERT)) {
            return true;
        }

        flightgear::addSentryBreadcrumb(e.message, (op == SG_WARN) ? "warning" : "error");
        return true;
    }
};

} // namespace

namespace flightgear
{

bool sentryReportCommand(const SGPropertyNode* args, SGPropertyNode* root)
{
    if (!static_sentryEnabled) {
        SG_LOG(SG_GENERAL, SG_WARN, "Sentry.io not enabled at startup");
        return false;
    }

    sentry_value_t exc = sentry_value_new_object();
    sentry_value_set_by_key(exc, "type", sentry_value_new_string("Report"));

    const string message = args->getStringValue("message");
    sentry_value_set_by_key(exc, "value", sentry_value_new_string(message.c_str()));

    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(event, "exception", exc);
    // capture the C++ stack-trace. Probably not that useful but can't hurt
    sentry_event_value_add_stacktrace(event, nullptr, 0);

    sentry_capture_event(event);

    return true;
}

void initSentry()
{
    sentry_options_t *options = sentry_options_new();
    // API key is defined in config.h, set in an environment variable prior
    // to running CMake, so it can be customised. Env var at build time is:
    // FLIGHTGEAR_SENTRY_API_KEY
    sentry_options_set_dsn(options, SENTRY_API_KEY);
    
    if (strcmp(FG_BUILD_TYPE, "Dev") == 0) {
        sentry_options_set_release(options, "flightgear-dev@" FLIGHTGEAR_VERSION);
    } else {
        sentry_options_set_release(options, "flightgear@" FLIGHTGEAR_VERSION);
    }
    
    const auto buildString = std::to_string(JENKINS_BUILD_NUMBER);
    sentry_options_set_dist(options, buildString.c_str());
    
    SGPath dataPath = fgHomePath() / "sentry_db";
#if defined(SG_WINDOWS)
    const auto homePathString = dataPath.wstr();
    sentry_options_set_database_pathw(options, homePathString.c_str());
    
    const auto logPath = (fgHomePath() / "fgfs.log").wstr();
    sentry_options_add_attachmentw(options, logPath.c_str());
#else
    const auto homePathString = dataPath.utf8Str();
    sentry_options_set_database_path(options, homePathString.c_str());
    
    const auto logPath = (fgHomePath() / "fgfs.log").utf8Str();
    sentry_options_add_attachment(options, logPath.c_str());
#endif
    
    sentry_init(options);
    static_sentryEnabled = true;

    sglog().addCallback(new SentryLogCallback);
}

void delayedSentryInit()
{
    if (!static_sentryEnabled)
        return;

    // allow the user to opt-out of sentry.io features
    if (!fgGetBool("/sim/startup/sentry-crash-reporting-enabled", true)) {
        SG_LOG(SG_GENERAL, SG_INFO, "Disabling Sentry.io reporting");
        sentry_shutdown();
        static_sentryEnabled = false;
        return;
    }

    globals->get_commands()->addCommand("sentry-report", &sentryReportCommand);
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

void sentryReportException(const std::string& msg, const std::string& location)
{
    if (!static_sentryEnabled)
        return;

    sentry_value_t exc = sentry_value_new_object();
    sentry_value_set_by_key(exc, "type", sentry_value_new_string("Exception"));
    sentry_value_set_by_key(exc, "value", sentry_value_new_string(msg.c_str()));

    if (!location.empty()) {
        sentry_value_set_by_key(exc, "location", sentry_value_new_string(location.c_str()));
    }

    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(event, "exception", exc);
    
    // capture the C++ stack-trace. Probably not that useful but can't hurt
    sentry_event_value_add_stacktrace(event, nullptr, 0);
    sentry_capture_event(event);
}

void  sentryReportFatalError(const std::string& msg, const std::string& more)
{
    if (!static_sentryEnabled)
        return;

    sentry_value_t sentryMsg = sentry_value_new_object();
    sentry_value_set_by_key(sentryMsg, "type", sentry_value_new_string("Fatal Error"));
    sentry_value_set_by_key(sentryMsg, "formatted", sentry_value_new_string(msg.c_str()));

    if (!more.empty()) {
        sentry_value_set_by_key(sentryMsg, "more", sentry_value_new_string(more.c_str()));
    }

    sentry_value_t event = sentry_value_new_event();
    sentry_value_set_by_key(event, "message", sentryMsg);
    
    sentry_event_value_add_stacktrace(event, nullptr, 0);
    sentry_capture_event(event);
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

void delayedSentryInit()
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

void sentryReportException(const std::string&, const  std::string&)
{
}

void sentryReportFatalError(const std::string&, const std::string&)
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
