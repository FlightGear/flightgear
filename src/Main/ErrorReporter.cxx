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

#include "config.h"

#include "ErrorReporter.hxx"

#include <algorithm>
#include <ctime> // for strftime, etc
#include <deque>
#include <map>
#include <mutex>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/debug/LogCallback.hxx>
#include <simgear/timing/timestamp.hxx>

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/commands.hxx>

#include <GUI/MessageBox.hxx>
#include <GUI/new_gui.hxx>
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/locale.hxx>
#include <Main/options.hxx>
#include <Main/sentryIntegration.hxx>
#include <Scripting/NasalClipboard.hxx> // clipboard access

using std::string;

namespace {

const double MinimumIntervalBetweenDialogs = 5.0;
const double NoNewErrorsTimeout = 8.0;

// map of context values; we allow a stack of values for
// cases such as sub-sub-model loading where we might process repeated
// nested model XML files
using PerThreadErrorContextStack = std::map<std::string, string_list>;

// context storage. This is per-thread so parallel osgDB threads don't
// confuse each other
static thread_local PerThreadErrorContextStack thread_errorContextStack;
/**
 Define the aggregation points we use for errors, to direct the user towards the likely
 source of problems.
 */
enum class Aggregation {
    MainAircraft,
    HangarAircraft, // handle hangar aircraft differently
    CustomScenery,
    TerraSync,
    AddOn,
    Scenario,
    InputDevice,
    FGData,
    MultiPlayer,
    Unknown,     ///< error coudln't be attributed more specifcially
    OutOfMemory, ///< seperate category to give it a custom message
    Traffic,
    ShadersEffects
};

// these should correspond to simgear::ErrorCode enum
// they map to translateable strings in fgdata/Translations/sys.xml

string_list static_errorIds = {
    "error-missing-shader",
    "error-loading-texture",
    "error-xml-model-load",
    "error-3D-model-load",
    "error-btg-load",
    "error-scenario-load",
    "error-dialog-load",
    "error-audio-fx-load",
    "error-xml-load-command",
    "error-aircraft-systems",
    "error-input-device-config",
    "error-ai-traffic-schedule",
    "error-terrasync"};

string_list static_errorTypeIds = {
    "error-type-unknown",
    "error-type-not-found",
    "error-type-out-of-memory",
    "error-type-bad-header",
    "error-type-bad-data",
    "error-type-misconfigured",
    "error-type-io",
    "error-type-network"};


string_list static_categoryIds = {
    "error-category-aircraft",
    "error-category-aircraft-from-hangar",
    "error-category-custom-scenery",
    "error-category-terrasync",
    "error-category-addon",
    "error-category-scenario",
    "error-category-input-device",
    "error-category-fgdata",
    "error-category-multiplayer",
    "error-category-unknown",
    "error-category-out-of-memory",
    "error-category-traffic",
    "error-category-shaders"};

class RecentLogCallback : public simgear::LogCallback
{
public:
    RecentLogCallback() : simgear::LogCallback(SG_ALL, SG_INFO)
    {
    }

    bool doProcessEntry(const simgear::LogEntry& e) override
    {
        ostringstream os;
        if (e.file != nullptr) {
            os << e.file << ":" << e.line << ":\t";
        }


        os << e.message;

        std::lock_guard<std::mutex> g(_lock); // begin access to shared state
        _recentLogEntries.push_back(os.str());

        while (_recentLogEntries.size() > _preceedingLogMessageCount) {
            _recentLogEntries.pop_front();
        }

        return true;
    }

    string_list copyRecentLogEntries() const
    {
        std::lock_guard<std::mutex> g(_lock); // begin access to shared state

        string_list r(_recentLogEntries.begin(), _recentLogEntries.end());
        return r;
    }

private:
    mutable std::mutex _lock;
    int _preceedingLogMessageCount = 6;

    using LogDeque = std::deque<string>;
    LogDeque _recentLogEntries;
};

} // namespace

namespace flightgear {

class ErrorReporter::ErrorReporterPrivate
{
public:
    bool _reportsDirty = false;
    std::mutex _lock;
    SGTimeStamp _nextShowTimeout;
    bool _haveDonePostInit = false;

    SGPropertyNode_ptr _enabledNode;
    SGPropertyNode_ptr _displayNode;
    SGPropertyNode_ptr _activeErrorNode;
    SGPropertyNode_ptr _mpReportNode;

    using ErrorContext = std::map<std::string, std::string>;
    /**
            strucutre representing a single error which has cocurred
     */
    struct ErrorOcurrence {
        simgear::ErrorCode code;
        simgear::LoadFailure type;
        string detailedInfo;
        sg_location origin;
        time_t when;
        string_list log;
        ErrorContext context;

        bool hasContextKey(const std::string& key) const
        {
            return context.find(key) != context.end();
        }

        std::string getContextValue(const std::string& key) const
        {
            auto it = context.find(key);
            if (it == context.end())
                return {};

            return it->second;
        }
    };

    using OccurrenceVec = std::vector<ErrorOcurrence>;

    std::unique_ptr<RecentLogCallback> _logCallback;
    bool _logCallbackRegistered = false;

    string _terrasyncPathPrefix;
    string _fgdataPathPrefix;
    string _aircraftDirectoryName;

    /**
        @brief hueristic to identify relative paths as origination from the main aircraft as opposed
                to something else. This is based on containing the aircraft directory name.
     */
    bool isMainAircraftPath(const std::string& path) const;

    /**
        structure representing one or more errors, aggregated together
     */
    struct AggregateReport {
        Aggregation type;
        std::string parameter; ///< base on type, the specific point. For example the add-on ID, AI model ident or custom scenery path
        SGTimeStamp lastErrorTime;

        bool haveShownToUser = false;
        OccurrenceVec errors;
        bool isCritical = false;

        bool addOccurence(const ErrorOcurrence& err);
    };


    using AggregateErrors = std::vector<AggregateReport>;
    AggregateErrors _aggregated;
    int _activeReportIndex = -1;
    string_list _significantProperties; ///< properties we want to include in reports, for debugging

    /**
    find the appropriate agrgegate for an error, based on its context
     */
    AggregateErrors::iterator getAggregateForOccurence(const ErrorOcurrence& oc);

    AggregateErrors::iterator getAggregate(Aggregation ag, const std::string& param = {});


    void collectError(simgear::LoadFailure type, simgear::ErrorCode code, const std::string& details, const sg_location& location)
    {
        ErrorOcurrence occurrence{code, type, details, location, time(nullptr)};

        // snapshot the top of the context stacks into our occurence data
        for (const auto& c : thread_errorContextStack) {
            occurrence.context[c.first] = c.second.back();
        }

        occurrence.log = _logCallback->copyRecentLogEntries();

        std::lock_guard<std::mutex> g(_lock); // begin access to shared state
        auto it = getAggregateForOccurence(occurrence);

        // add to the occurence, if it's not a duplicate
        if (!it->addOccurence(occurrence)) {
            return; // duplicate, nothing else to do
        }

        // log it once we know it's not a duplicate
        SG_LOG(SG_GENERAL, SG_WARN, "Error:" << static_errorTypeIds.at(static_cast<int>(type)) << " from " << static_errorIds.at(static_cast<int>(code)) << "::" << details << "\n\t" << location.asString());

        it->lastErrorTime.stamp();
        _reportsDirty = true;

        const auto ty = it->type;
        // decide if it's a critical error or not
        if ((ty == Aggregation::OutOfMemory) || (ty == Aggregation::InputDevice)) {
            it->isCritical = true;
        }

        // aircraft errors are critical if they occur during initial
        // aircraft load, otherwise we just show the warning
        if (!_haveDonePostInit && (ty == Aggregation::MainAircraft)) {
            it->isCritical = true;
        }

        if (code == simgear::ErrorCode::LoadEffectsShaders) {
            it->isCritical = true;
        }
    }

    void collectContext(const std::string& key, const std::string& value)
    {
        if (value == "POP") {
            auto it = thread_errorContextStack.find(key);
            assert(it != thread_errorContextStack.end());
            assert(!it->second.empty());
            it->second.pop_back();
            if (it->second.empty()) {
                thread_errorContextStack.erase(it);
            }
        } else {
            thread_errorContextStack[key].push_back(value);
        }
    }

    void presentErrorToUser(AggregateReport& report)
    {
        const int catId = static_cast<int>(report.type);
        auto catLabel = globals->get_locale()->getLocalizedString(static_categoryIds.at(catId), "sys");

        catLabel = simgear::strutils::replace(catLabel, "%VALUE%", report.parameter);

        _displayNode->setStringValue("category", catLabel);


        auto ns = globals->get_locale()->getLocalizedString("error-next-steps", "sys");
        _displayNode->setStringValue("next-steps", ns);


        // remove any existing error children
        _displayNode->removeChildren("error");

        ostringstream detailsTextStream;

        // add all the discrete errors as child nodes with all their information
        for (const auto& e : report.errors) {
            SGPropertyNode_ptr errNode = _displayNode->addChild("error");
            const auto em = globals->get_locale()->getLocalizedString(static_errorIds.at(static_cast<int>(e.code)), "sys");
            errNode->setStringValue("message", em);
            errNode->setIntValue("code", static_cast<int>(e.code));

            const auto et = globals->get_locale()->getLocalizedString(static_errorTypeIds.at(static_cast<int>(e.type)), "sys");
            errNode->setStringValue("type-message", et);
            errNode->setIntValue("type", static_cast<int>(e.type));
            errNode->setStringValue("details", e.detailedInfo);

            detailsTextStream << em << ": " << et << "\n";
            detailsTextStream << "(" << e.detailedInfo << ")\n";

            if (e.origin.isValid()) {
                errNode->setStringValue("location", e.origin.asString());
                detailsTextStream << "  from:" << e.origin.asString() << "\n";
            }

            detailsTextStream << "\n";
        } // of errors within the report iteration

        _displayNode->setStringValue("details-text", detailsTextStream.str());
        _activeErrorNode->setBoolValue(true);

        report.haveShownToUser = true;

        // compute index; slightly clunky, find the report in _aggregated
        auto it = std::find_if(_aggregated.begin(), _aggregated.end(), [report](const AggregateReport& a) {
            if (a.type != report.type) return false;
            return report.parameter.empty() ? true : report.parameter == a.parameter;
        });
        assert(it != _aggregated.end());
        _activeReportIndex = static_cast<int>(std::distance(_aggregated.begin(), it));
        _displayNode->setBoolValue("index", _activeReportIndex);
        _displayNode->setBoolValue("have-next", _activeReportIndex < (_aggregated.size() - 1));
        _displayNode->setBoolValue("have-previous", _activeReportIndex > 0);
    }

    void sendReportToSentry(AggregateReport& report)
    {
        const int catId = static_cast<int>(report.type);
        flightgear::sentryReportUserError(static_categoryIds.at(catId), 
            _displayNode->getStringValue("details-text"));
    }

    void writeReportToStream(const AggregateReport& report, std::ostream& os) const;
    void writeContextToStream(const ErrorOcurrence& error, std::ostream& os) const;
    void writeLogToStream(const ErrorOcurrence& error, std::ostream& os) const;
    void writeSignificantPropertiesToStream(std::ostream& os) const;

    bool dismissReportCommand(const SGPropertyNode* args, SGPropertyNode*);
    bool saveReportCommand(const SGPropertyNode* args, SGPropertyNode*);
    bool showErrorReportCommand(const SGPropertyNode* args, SGPropertyNode*);
};

auto ErrorReporter::ErrorReporterPrivate::getAggregateForOccurence(const ErrorReporter::ErrorReporterPrivate::ErrorOcurrence& oc)
    -> AggregateErrors::iterator
{
    // all OOM errors go to a dedicated category. This is so we don't blame
    // out of memory on the aircraft/scenery/etc, when it's not the underlying
    // cause.
    if (oc.type == simgear::LoadFailure::OutOfMemory) {
        return getAggregate(Aggregation::OutOfMemory, {});
    }

    if (oc.hasContextKey("primary-aircraft")) {
        const auto fullId = fgGetString("/sim/aircraft-id");
        if (fullId != fgGetString("/sim/aircraft")) {
            return getAggregate(Aggregation::MainAircraft, fullId);
        }

        return getAggregate(Aggregation::HangarAircraft, fullId);
    }

    if (oc.hasContextKey("multiplayer")) {
        return getAggregate(Aggregation::MultiPlayer, {});
    }

    // traffic cases: need to handle errors in the traffic files (schedule, rwyuse)
    // but also errors loading aircraft models associated with traffic
    if (oc.code == simgear::ErrorCode::AITrafficSchedule) {
        return getAggregate(Aggregation::Traffic, {});
    }

    if (oc.hasContextKey("traffic-aircraft-callsign")) {
        return getAggregate(Aggregation::Traffic, {});
    }

    // all TerraSync coded errors go there: this is errors for the
    // actual download process (eg, failed to write to disk)
    if (oc.code == simgear::ErrorCode::TerraSync) {
        return getAggregate(Aggregation::TerraSync, {});
    }

    if (oc.hasContextKey("terrain-stg") || oc.hasContextKey("btg")) {
        // determine if it's custom scenery, TerraSync or FGData

        // bucket is no use here, we need to check the BTG/XML/STG path etc.
        // STG is probably the best bet. This ensures if a custom scenery
        // STG references a model, XML or texture in FGData or TerraSync
        // incorrectly, we attribute the error to the scenery, which is
        // likely what we want/expect
        auto path = oc.hasContextKey("terrain-stg") ? oc.getContextValue("terrain-stg") : oc.getContextValue("btg");
        if (simgear::strutils::starts_with(path, _fgdataPathPrefix)) {
            return getAggregate(Aggregation::FGData, {});
        } else if (simgear::strutils::starts_with(path, _terrasyncPathPrefix)) {
            return getAggregate(Aggregation::TerraSync, {});
        }

        // custom scenery, find out the prefix
        for (const auto& sceneryPath : globals->get_fg_scenery()) {
            const auto pathStr = sceneryPath.utf8Str();
            if (simgear::strutils::starts_with(path, pathStr)) {
                return getAggregate(Aggregation::CustomScenery, pathStr);
            }
        }

        // shouldn't ever happen
        return getAggregate(Aggregation::CustomScenery, {});
    }

    if (oc.hasContextKey("scenario-path")) {
        const auto scenarioPath = oc.getContextValue("scenario-path");
        return getAggregate(Aggregation::Scenario, scenarioPath);
    }

    if (oc.hasContextKey("input-device")) {
        return getAggregate(Aggregation::InputDevice, oc.getContextValue("input-device"));
    }

    // start guessing :)
    // from this point on we're using less reliable inferences about where the
    // error came from, trying to avoid 'unknown'
    if (isMainAircraftPath(oc.origin.asString())) {
        const auto fullId = fgGetString("/sim/aircraft-id");
        if (fullId != fgGetString("/sim/aircraft")) {
            return getAggregate(Aggregation::MainAircraft, fullId);
        }

        return getAggregate(Aggregation::HangarAircraft, fullId);
    }

    // GUI dialog errors often have no context
    if (oc.code == simgear::ErrorCode::GUIDialog) {
        // check if it's an add-on and use that
        return getAggregate(Aggregation::FGData);
    }

    // becuase we report shader errors from the main thread, they don't
    // get attributed. Collect them into their own category, which also
    // means we can display a more specific message
    if (oc.code == simgear::ErrorCode::LoadEffectsShaders) {
        return getAggregate(Aggregation::ShadersEffects);
    }

    return getAggregate(Aggregation::Unknown);
}

auto ErrorReporter::ErrorReporterPrivate::getAggregate(Aggregation ag, const std::string& param)
    -> AggregateErrors::iterator
{
    auto it = std::find_if(_aggregated.begin(), _aggregated.end(), [ag, &param](const AggregateReport& a) {
        if (a.type != ag) return false;
        return param.empty() ? true : param == a.parameter;
    });

    if (it == _aggregated.end()) {
        AggregateReport r;
        r.type = ag;
        r.parameter = param;
        _aggregated.push_back(r);
        it = _aggregated.end() - 1;
    }

    return it;
}

void ErrorReporter::ErrorReporterPrivate::writeReportToStream(const AggregateReport& report, std::ostream& os) const
{
    os << "FlightGear " << VERSION << " error report, created at ";
    {
        char buf[64];
        time_t now = time(nullptr);
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
        os << buf << endl;
    }

    os << "Category:" << static_categoryIds.at(static_cast<int>(report.type)) << endl;
    if (!report.parameter.empty()) {
        os << "\tParameter:" << report.parameter << endl;
    }

    os << endl; // insert a blank line after header data

    int index = 1;
    char whenBuf[64];

    for (const auto& err : report.errors) {
        os << "Error " << index++ << std::endl;
        os << "\tcode:" << static_errorIds.at(static_cast<int>(err.code)) << endl;
        os << "\ttype:" << static_errorTypeIds.at(static_cast<int>(err.type)) << endl;

        strftime(whenBuf, sizeof(whenBuf), "%H:%M:%S GMT", gmtime(&err.when));
        os << "\twhen:" << whenBuf << endl;

        os << "\t" << err.detailedInfo << std::endl;
        os << "\tlocation:" << err.origin.asString() << endl;
        writeContextToStream(err, os);
        writeLogToStream(err, os);
        os << std::endl; // trailing blank line
    }

    os << "Command line / launcher / fgfsrc options" << endl;
    for (auto o : Options::sharedInstance()->extractOptions()) {
        os << "\t" << o << "\n";
    }
    os << endl;

    writeSignificantPropertiesToStream(os);
}

void ErrorReporter::ErrorReporterPrivate::writeSignificantPropertiesToStream(std::ostream& os) const
{
    os << "Properties:" << endl;
    for (const auto& ps : _significantProperties) {
        auto node = fgGetNode(ps);
        if (!node) {
            os << "\t" << ps << ": not defined\n";
        } else {
            os << "\t" << ps << ": " << node->getStringValue() << "\n";
        }
    }
    os << endl;
}


bool ErrorReporter::ErrorReporterPrivate::dismissReportCommand(const SGPropertyNode* args, SGPropertyNode*)
{
    std::lock_guard<std::mutex> g(_lock);
    _activeErrorNode->setBoolValue(false);

    if (args->getBoolValue("dont-show")) {
        // TODO implement dont-show behaviour
    }

    // clear any values underneath displayNode?

    _nextShowTimeout.stamp();
    _reportsDirty = true; // set this so we check for another report to present
    _activeReportIndex = -1;

    return true;
}

bool ErrorReporter::ErrorReporterPrivate::showErrorReportCommand(const SGPropertyNode* args, SGPropertyNode*)
{
    std::lock_guard<std::mutex> g(_lock);

    if (_aggregated.empty()) {
        return false;
    }

    const auto numAggregates = static_cast<int>(_aggregated.size());
    if (args->getBoolValue("next")) {
        _activeReportIndex++;
        if (_activeReportIndex >= numAggregates) {
            return false;
        }
    } else if (args->getBoolValue("previous")) {
        if (_activeReportIndex < 1) {
            return false;
        }
        _activeReportIndex--;
    } else if (args->hasChild("index")) {
        _activeReportIndex = args->getIntValue("index");
        if ((_activeReportIndex < 0) || (_activeReportIndex >= numAggregates)) {
            return false;
        }
    } else {
        _activeReportIndex = 0;
    }

    auto& report = _aggregated.at(_activeReportIndex);
    presentErrorToUser(report);

    auto gui = globals->get_subsystem<NewGUI>();
    if (!gui->getDialog("error-report")) {
        gui->showDialog("error-report");
    }

    return true;
}

bool ErrorReporter::ErrorReporterPrivate::saveReportCommand(const SGPropertyNode* args, SGPropertyNode*)
{
    if (_activeReportIndex < 0) {
        return false;
    }

    const auto& report = _aggregated.at(_activeReportIndex);

    const string where = args->getStringValue("where");

    string when;
    {
        char buf[64];
        time_t now = time(nullptr);
        strftime(buf, sizeof(buf), "%Y%m%d", gmtime(&now));
        when = buf;
    }

    if (where.empty() || (where == "!desktop")) {
        SGPath p = SGPath::desktop() / ("flightgear_error_" + when + ".txt");
        int uniqueCount = 2;
        while (p.exists()) {
            p = SGPath::desktop() / ("flightgear_error_" + when + "_" + std::to_string(uniqueCount++) + ".txt");
        }

        sg_ofstream f(p, std::ios_base::out);
        writeReportToStream(report, f);
    } else if (where == "!clipboard") {
        std::ostringstream os;
        writeReportToStream(report, os);
        NasalClipboard::getInstance()->setText(os.str());
    }

    return true;
}


void ErrorReporter::ErrorReporterPrivate::writeContextToStream(const ErrorOcurrence& error, std::ostream& os) const
{
    os << "\tcontext:\n";
    for (const auto& c : error.context) {
        os << "\t\t" << c.first << " = " << c.second << "\n";
    }
}

void ErrorReporter::ErrorReporterPrivate::writeLogToStream(const ErrorOcurrence& error, std::ostream& os) const
{
    os << "\tpreceeding log:\n";
    for (const auto& c : error.log) {
        os << "\t\t" << c << "\n";
    }
}

bool ErrorReporter::ErrorReporterPrivate::AggregateReport::addOccurence(const ErrorOcurrence& err)
{
    // hacky reduction in noise for 2020.3: special case this error.
    // file was removed but update-in-place means it will persist
    // this is the removed RNLAF traffic which has many errors
    if (type == Aggregation::Traffic) {
        const auto originPath = err.origin.asString();
        if (simgear::strutils::ends_with(originPath, "Traffic/R/RNAF.xml")) {
            return false; // don't add
        }
    }

    auto it = std::find_if(errors.begin(), errors.end(), [err](const ErrorOcurrence& ext) {
        // check if the two occurences match for the purposes of
        // de-duplication.
        return (ext.code == err.code) &&
               (ext.type == err.type) &&
               (ext.detailedInfo == err.detailedInfo) &&
               (ext.origin.asString() == err.origin.asString());
    });

    if (it != errors.end()) {
        return false; // duplicate, don't add
    }

    errors.push_back(err);
    lastErrorTime.stamp();
    return true;
}

bool ErrorReporter::ErrorReporterPrivate::isMainAircraftPath(const std::string& path) const
{
    const auto pos = path.find(_aircraftDirectoryName);
    return pos != std::string::npos;
}


////////////////////////////////////////////

ErrorReporter::ErrorReporter() : d(new ErrorReporterPrivate)
{
    d->_logCallback.reset(new RecentLogCallback);

    // define significant properties
    d->_significantProperties = {
        "/sim/aircraft-id",
        "/sim/aircraft-dir",
        "/sim/rendering/gl-version",
        "/sim/rendering/gl-renderer",
        "/sim/rendering/gl-shading-language-version",
        "/sim/rendering/max-texture-size",
        "/sim/rendering/max-texture-units",
        "/sim/rendering/shaders/skydome",
        "/sim/rendering/shaders/water",
        "/sim/rendering/shaders/model",
        "/sim/rendering/shaders/landmass",
        "/sim/rendering/shaders/vegetation-effects",
        "/sim/rendering/shaders/transition",
        "/sim/rendering/max-paged-lod",
        "/sim/rendering/photoscenery/enabled",
        "/sim/rendering/preset-description",
        "/sim/rendering/multithreading-mode",
        "/sim/rendering/multi-sample-buffers",
        "/sim/rendering/multi-samples",
        "/scenery/use-vpb"};
}

ErrorReporter::~ErrorReporter()
{
    // if we are deleted withut being shutdown(), ensure we clean
    // up our logging callback
    if (d->_logCallbackRegistered) {
        sglog().removeCallback(d->_logCallback.get());
    }
}

void ErrorReporter::bind()
{
    SGPropertyNode_ptr n = fgGetNode("/sim/error-report", true);

    d->_enabledNode = n->getNode("enabled", true);
    d->_displayNode = n->getNode("display", true);
    d->_activeErrorNode = n->getNode("active", true);
    d->_mpReportNode = n->getNode("mp-report-enabled", true);
}

void ErrorReporter::unbind()
{
    d->_enabledNode.clear();
    d->_displayNode.clear();
    d->_activeErrorNode.clear();
}

void ErrorReporter::preinit()
{
    ErrorReporterPrivate* p = d.get();
    simgear::setFailureCallback([p](simgear::LoadFailure type, simgear::ErrorCode code, const std::string& details, const sg_location& location) {
        p->collectError(type, code, details, location);
    });

    simgear::setErrorContextCallback([p](const std::string& key, const std::string& value) {
        p->collectContext(key, value);
    });

    sglog().addCallback(d->_logCallback.get());
    d->_logCallbackRegistered = true;
}

void ErrorReporter::init()
{
    // we want to disable errors in developer mode, but since self-compiled
    // builds default to developer-mode=true, need an override so people
    // can see errors if they want
    const auto developerMode = fgGetBool("sim/developer-mode");
    const auto disableInDeveloperMode = !d->_enabledNode->getParent()->getBoolValue("enable-in-developer-mode");
    const auto dd = developerMode && disableInDeveloperMode;

    if (dd || !d->_enabledNode) {
        SG_LOG(SG_GENERAL, SG_INFO, "Error reporting disabled");
        simgear::setFailureCallback(simgear::FailureCallback());
        simgear::setErrorContextCallback(simgear::ContextCallback());
        if (d->_logCallbackRegistered) {
            sglog().removeCallback(d->_logCallback.get());
            d->_logCallbackRegistered = false;
        }
        return;
    }

    globals->get_commands()->addCommand("dismiss-error-report", d.get(), &ErrorReporterPrivate::dismissReportCommand);
    globals->get_commands()->addCommand("save-error-report-data", d.get(), &ErrorReporterPrivate::saveReportCommand);
    globals->get_commands()->addCommand("show-error-report", d.get(), &ErrorReporterPrivate::showErrorReportCommand);

    // cache these values here
    d->_fgdataPathPrefix = globals->get_fg_root().utf8Str();
    d->_terrasyncPathPrefix = globals->get_terrasync_dir().utf8Str();

    const auto aircraftPath = SGPath::fromUtf8(fgGetString("/sim/aircraft-dir"));
    d->_aircraftDirectoryName = aircraftPath.file();
}

void ErrorReporter::update(double dt)
{
    bool showDialog = false;
    bool showPopup = false;
    bool havePendingReports = false;

    // beginning of locked section
    {
        std::lock_guard<std::mutex> g(d->_lock);
        if (!d->_enabledNode->getBoolValue()) {
            return;
        }

        // we are into the update phase (postinit has ocurred). We treat errors
        // after this point with lower severity, to avoid popups into a flight
        d->_haveDonePostInit = true;

        SGTimeStamp n = SGTimeStamp::now();

        // ensure we pause between successive error dialogs
        const auto timeSinceLastDialog = (n - d->_nextShowTimeout).toSecs();
        if (timeSinceLastDialog < MinimumIntervalBetweenDialogs) {
            return;
        }

        if (!d->_reportsDirty) {
            return;
        }

        if (d->_activeReportIndex >= 0) {
            return; // already showing a report
        }

        // check if any reports are due

        // check if an error is current active
        for (auto& report : d->_aggregated) {
            if (report.type == Aggregation::MultiPlayer) {
                if (!d->_mpReportNode->getBoolValue()) {
                    // mark it as shown, to supress it
                    report.haveShownToUser = true;
                }
            }

            if (report.haveShownToUser) {
                // unless we ever re-show?
                continue;
            }

            const auto ageSec = (n - report.lastErrorTime).toSecs();
            if (ageSec > NoNewErrorsTimeout) {
                d->presentErrorToUser(report);
                if (report.isCritical) {
                    showDialog = true;
                } else {
                    showPopup = true;
                }

                d->sendReportToSentry(report);

                // if we show one report, don't consider any others for now
                break;
            } else {
                havePendingReports = true;
            }
        } // of active aggregates iteration

        if (!havePendingReports) {
            d->_reportsDirty = false;
        }
    } // end of locked section

    if (flightgear::isHeadlessMode()) {
        showDialog = false;
        showPopup = false;
    }

// don't show any error reporting UI on stable, for the moment.
    showDialog = false;
    showPopup = false;


    // do not call into another subsystem with our lock held,
    // as this can trigger deadlocks
    if (showDialog) {
        auto gui = globals->get_subsystem<NewGUI>();
        gui->showDialog("error-report");
        // this needs a bit more thought, disabling for the now
#if 0
        // pause the sim when showing the popup
        SGPropertyNode_ptr pauseArgs(new SGPropertyNode);
        pauseArgs->setBoolValue("force-pause", true);
        globals->get_commands()->execute("do_pause", pauseArgs);
#endif
    } else if (showPopup) {
        SGPropertyNode_ptr popupArgs(new SGPropertyNode);
        popupArgs->setIntValue("index", d->_activeReportIndex);
        globals->get_commands()->execute("show-error-notification-popup", popupArgs, nullptr);
    }
}

void ErrorReporter::shutdown()
{
    if (d->_enabledNode) {
        globals->get_commands()->removeCommand("dismiss-error-report");
        globals->get_commands()->removeCommand("save-error-report-data");
        globals->get_commands()->removeCommand("show-error-report");

// during a reset we don't want to touch the log callback; it was added in
// preinit, which does not get repeated on a reset
        const bool inReset = fgGetBool("/sim/signals/reinit", false);
        if (!inReset) {
            sglog().removeCallback(d->_logCallback.get());
            d->_logCallbackRegistered = false;
        }
    }
}

std::string ErrorReporter::threadSpecificContextValue(const std::string& key)
{
    auto it = thread_errorContextStack.find(key);
    if (it == thread_errorContextStack.end())
        return {};

    return it->second.back();
}


} // namespace flightgear
