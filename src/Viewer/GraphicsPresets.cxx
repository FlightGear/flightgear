#include "config.h"

#include <Viewer/GraphicsPresets.hxx>

// std
#include <unordered_set>

// SG
#include <simgear/misc/sg_dir.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/exception.hxx>

// FG
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Main/locale.hxx>
#include <Main/sentryIntegration.hxx>
#include <Scenery/scenery.hxx>

using namespace std;

namespace {

const char* kPresetPropPath = "/sim/rendering/preset";
const char* kPresetNameProp = "/sim/rendering/preset-name";
const char* kPresetDescriptionProp = "/sim/rendering/preset-description";
const char* kPresetActiveProp = "/sim/rendering/preset-active";


const char* kRestartRequiredProp = "/sim/rendering/restart-required";
const char* kSceneryReloadRequiredProp = "/sim/rendering/scenery-reload-required";
const char* kCompositorReloadRequiredProp = "/sim/rendering/compositor-reload-required";

// define the property prefixes which graphics presets are allowed to
// modify. Changes to properties outside these prefixes will be
// blocked

const string_list kWitelistedPrefixes = {
    "/sim/rendering"};


} // namespace

namespace flightgear {

class GraphicsPresets::GraphicsConfigChangeListener : public SGPropertyChangeListener
{
public:
    GraphicsConfigChangeListener()
    {
        _presetProp = fgGetNode(kPresetPropPath, true);
    }

    void registerWithProperty(SGPropertyNode_ptr n)
    {
        auto it = std::find(_watchedProps.begin(), _watchedProps.end(), n);
        if (it != _watchedProps.end()) {
            // this would happen if a preset somehow set the same property more than once
            SG_LOG(SG_GUI, SG_ALERT, "GraphicsPresets: Duplicate registration for:" << n->getPath());
            return;
        }

        _watchedProps.push_back(n);
        n->addChangeListener(this);
    }

    void unregisterFromProperties()
    {
        for (const auto& w : _watchedProps) {
            w->removeChangeListener(this);
        }
        _watchedProps.clear();
    }

    void valueChanged(SGPropertyNode* prop) override
    {
        SG_LOG(SG_GUI, SG_INFO, "GraphicsPreset clearing; setting:" << prop->getPath() << " was modified");
        if (_presetProp->hasValue()) {
            flightgear::addSentryBreadcrumb("clearing graphics preset, config was customised at:" + prop->getPath(), "info");
            _presetProp->clearValue();

            auto gp = globals->get_subsystem<GraphicsPresets>();
            gp->clearPreset();
        }
    }

private:
    SGPropertyNode_ptr _presetProp;

    using PropertyNodeList = std::vector<SGPropertyNode_ptr>;
    PropertyNodeList _watchedProps;
};

/**
    @brief monitor a collection of properties, and set a flag property to true when any of them are
    modified. Used to track the list of 'reload-required' and 'restart-requried' properties defined in
    FG_DATA/Video/graphics-properties.xml
 */
class GraphicsPresets::RequiredPropertyListener : public SGPropertyChangeListener
{
public:
    RequiredPropertyListener(const std::string& requiredProp, SGPropertyNode_ptr props)
    {
        _requiredProp = fgGetNode(requiredProp, true);
        _requiredProp->setBoolValue(false);

        // would happen if graphics-properties.xml was malformed
        if (!props)
            return;

        for (const auto& c : props->getChildren("property")) {
            // tolerate exterior whitespace in the XML
            string path = simgear::strutils::strip(c->getStringValue());
            if (path.empty())
                continue;

            SGPropertyNode_ptr n = fgGetNode(path, true);
            if (n) {
                n->addChangeListener(this);
            }
        } // of properties iteration
    }

    void clearRequiredFlag()
    {
        if (_requiredProp->getBoolValue()) {
            _requiredProp->setBoolValue(false);
        }
    }

    void valueChanged(SGPropertyNode* prop) override
    {
        if (!_requiredProp->getBoolValue()) {
            SG_LOG(SG_GUI, SG_INFO, "GraphicsPreset: saw modification of:" << prop->getPath() << ", setting:" << _requiredProp->getPath() << " to true");
            _requiredProp->setBoolValue(true);
        }
    }

private:
    SGPropertyNode_ptr _requiredProp;
};

static bool do_apply_preset(const SGPropertyNode* arg, SGPropertyNode* root)
{
    auto gp = globals->get_subsystem<GraphicsPresets>();

    bool result = false;
    if (arg->hasChild("path")) {
        SGPath p = SGPath::fromUtf8(arg->getStringValue("path"));
        if (!p.exists()) {
            SG_LOG(SG_IO, SG_ALERT, "apply-graphics-preset: no file at:" << p);
            return false;
        }

        result = gp->applyCustomPreset(p);
    } else if (arg->hasChild("preset-name")) {
        // helper for PUI UI: PUI ComboBox gives us the name, not the ID.
        // so allow specify a preset by (localized) name.
        gp->applyPresetByName(arg->getStringValue("preset-name"));
    } else if (arg->hasChild("preset")) {
        fgSetString(kPresetPropPath, arg->getStringValue("preset"));
        result = gp->applyCurrentPreset();
    } else {
        // just apply the current selected one
        result = gp->applyCurrentPreset();
    }

    if (arg->getBoolValue("reload-scenery")) {
        if (fgGetBool(kSceneryReloadRequiredProp)) {
            SG_LOG(SG_GUI, SG_MANDATORY_INFO, "apply-graphics-preset: triggering scenery reload");
            globals->get_scenery()->reinit(); // this will set
        }
    }

    return result;
}

static bool do_save_preset(const SGPropertyNode* arg, SGPropertyNode* root)
{
    // TODO implement me
    if (!arg->hasChild("path")) {
        SG_LOG(SG_GUI, SG_ALERT, "do_save_preset: no out path argument provided");
        return false;
    }

    const SGPath path = SGPath::fromUtf8(arg->getStringValue("path"));
    const string name = arg->getStringValue("name");
    auto gp = globals->get_subsystem<GraphicsPresets>();

    return gp->saveToXML(path, name);
}

static bool do_list_standard_presets(const SGPropertyNode* arg, SGPropertyNode* root)
{
    auto gp = globals->get_subsystem<GraphicsPresets>();
    if (!arg->hasValue("destination-path")) {
        SG_LOG(SG_GUI, SG_ALERT, "list-graphics-preset: no destination path supplied");
        return false;
    }

    SGPropertyNode_ptr destRoot = fgGetNode(arg->getStringValue("destination-path"), true /* create */);

    if (arg->hasValue("clear-destination")) {
        destRoot->removeAllChildren();
    }

    // format the way PUI combo-box (actualy, fgValueList) like it
    if (arg->getBoolValue("as-combobox-values")) {
        for (const auto& preset : gp->listPresets()) {
            SGPropertyNode_ptr v = destRoot->addChild("value");
            v->setStringValue(preset.name);
        }
    } else {
        for (const auto& preset : gp->listPresets()) {
            SGPropertyNode_ptr pn = destRoot->addChild("preset");
            pn->setStringValue("name", preset.name);
            pn->setStringValue("id", preset.id);
            pn->setStringValue("description", preset.description);
        }
    }

    return true;
}


GraphicsPresets::GraphicsPresets()
{
    // needs to be done early so that applyInitialPreset
    // can setup the registration
    _listener.reset(new GraphicsConfigChangeListener);
}

GraphicsPresets::~GraphicsPresets()
{
}

void GraphicsPresets::applyInitialPreset()
{
    const string currentPreset = fgGetString(kPresetPropPath);
    fgSetBool(kPresetActiveProp, false);
    if (!currentPreset.empty()) {
        SG_LOG(SG_GUI, SG_INFO, "Applying graphics preset:" << currentPreset);
        addSentryBreadcrumb("Startup selection of preset:" + currentPreset, "info");
        applyCurrentPreset();
    }
}

void GraphicsPresets::init()
{
    // create the change listeners. Because we do the initial application before this, we won't
    // see any changes caused by any initial preset load (or autosave.xml load), only
    // future changes made by the user via settings UI.
    SGPropertyNode_ptr graphicsPropsXML(new SGPropertyNode);
    try {
        readProperties(globals->findDataPath("Video/graphics-properties.xml"), graphicsPropsXML.get());

        _restartListener.reset(new RequiredPropertyListener{kRestartRequiredProp, graphicsPropsXML->getChild("restart-required")});
        _sceneryReloadListener.reset(new RequiredPropertyListener{kSceneryReloadRequiredProp, graphicsPropsXML->getChild("scenery-reload-required")});
        _compositorReloadListener.reset(new RequiredPropertyListener{kCompositorReloadRequiredProp, graphicsPropsXML->getChild("compositor-reload-required")});

        SGPropertyNode_ptr toSave = graphicsPropsXML->getChild("save-to-file");
        if (toSave) {
            for (const auto& p : toSave->getChildren("property")) {
                string t = simgear::strutils::strip(p->getStringValue());
                _propertiesToSave.push_back(t);
            }
        }
    } catch (sg_exception& e) {
        SG_LOG(SG_GUI, SG_ALERT, "Failed to read graphics-properties.xml");
    }

    globals->get_commands()->addCommand("apply-graphics-preset", do_apply_preset);
    globals->get_commands()->addCommand("save-graphics-preset", do_save_preset);
    globals->get_commands()->addCommand("list-graphics-presets", do_list_standard_presets);
}

void GraphicsPresets::update(double delta_time_sec)
{
    SG_UNUSED(delta_time_sec);
}

void GraphicsPresets::shutdown()
{
    globals->get_commands()->removeCommand("apply-graphics-preset");
    globals->get_commands()->removeCommand("save-graphics-preset");
    globals->get_commands()->removeCommand("list-graphics-presets");

    _listener->unregisterFromProperties();
    _listener.reset();
}

bool GraphicsPresets::applyCurrentPreset()
{
    _listener->unregisterFromProperties();

    const string presetId = fgGetString(kPresetPropPath);
    GraphicsPresetInfo info;
    const auto ok = loadStandardPreset(presetId, info);
    if (!ok) {
        return false;
    }

    addSentryBreadcrumb("loading graphics preset:" + presetId, "info");

    return innerApplyPreset(info, true);
}

bool GraphicsPresets::loadStandardPreset(const std::string& id, GraphicsPresetInfo& info)
{
    const auto path = globals->findDataPath("Video/" + id + "-preset.xml");
    if (!path.exists()) {
        SG_LOG(SG_GUI, SG_ALERT, "No such graphics preset '" << id << "' found");
        return false;
    }

    return loadPresetXML(path, info);
}


auto GraphicsPresets::listPresets() -> GraphicsPresetVec
{
    GraphicsPresetVec result;

    auto videoPaths = globals->get_data_paths("Video");
    for (const auto& vp : videoPaths) {
        simgear::Dir videoDir(vp);
        for (const auto& presetFile : videoDir.children(simgear::Dir::TYPE_FILE, "-preset.xml")) {
            GraphicsPresetInfo info;
            loadPresetXML(presetFile, info);
            result.push_back(info);
        }
    } // of Video/ data dirs iteration

    // Sort the resulting list by the order number or alphabetically if some
    // presets have the same order number
    sort(result.begin(), result.end(), [](auto &a, auto &b) {
        if (a.orderNum != b.orderNum)
            return a.orderNum < b.orderNum;
        return a.name < b.name;
    });

    return result;
}

bool GraphicsPresets::applyCustomPreset(const SGPath& path)
{
    GraphicsPresetInfo info;
    const auto ok = loadPresetXML(path, info);
    if (!ok) {
        return false;
    }

    addSentryBreadcrumb("loading graphics preset from:" + path.utf8Str(), "info");
    return innerApplyPreset(info, true);
}

bool GraphicsPresets::applyPresetByName(const std::string& name)
{
    const auto presets = listPresets();
    auto it = std::find_if(presets.begin(), presets.end(), [name](const GraphicsPresetInfo& pi) { return simgear::strutils::iequals(name, pi.name); });
    if (it == presets.end()) {
        SG_LOG(SG_GUI, SG_ALERT, "Couldn't find graphics preset with name:" << name);
        return false;
    }

    fgSetString(kPresetPropPath, it->id);
    return applyCurrentPreset();
}

bool GraphicsPresets::innerApplyPreset(const GraphicsPresetInfo& info, bool overwriteAutosaved)
{
    fgSetString(kPresetNameProp, info.name);
    fgSetString(kPresetDescriptionProp, info.description);

    if (info.id == "custom") {
        fgSetBool(kPresetActiveProp, false);
    }

    std::unordered_set<string> leafProps;

    copyPropertiesIf(info.properties, globals->get_props(), [overwriteAutosaved, &leafProps](const SGPropertyNode* src) {
        if (src->getParent() == nullptr)
            return true; // root node passes

        // due to the slightly odd way SGPropertyNode::getPath works, we
        // don't need to omit settings here; it will be dropped
        // automatically.
        const auto path = src->getPath(true);

        auto it = std::find_if(kWitelistedPrefixes.begin(), kWitelistedPrefixes.end(), [&path](const string& p) {
            // if we're high up in the tree, eg looking at /sim, then we
            // want to check if at least one prefix includes that path
            if (path.length() < p.length()) {
                return simgear::strutils::starts_with(p, path);
            }

            // if the prefix is longer (more specific) than our path, we
            // want to consider the full prefix
            return simgear::strutils::starts_with(path, p);
        });

        if (it == kWitelistedPrefixes.end()) {
            return false; // skip entirely
        }

        // find the corresponding destination node
        auto dstNode = globals->get_props()->getNode(path);

        // if destination exists, and we're not over-writing, check its
        // ARCHIVE flag.
        if (!overwriteAutosaved && dstNode && (dstNode->getAttribute(SGPropertyNode::ARCHIVE) == false)) {
            return false;
        }

        // only watch the leaf properties
        const bool isLeaf = src->nChildren() == 0;
        if (isLeaf) {
            leafProps.insert(path);
        }
        return true; // easy, just copy it
    });

    _listener->unregisterFromProperties();
    for (const auto& p : leafProps) {
        _listener->registerWithProperty(fgGetNode(p));
    }

    fgSetBool(kPresetActiveProp, true);
    return true;
}

void GraphicsPresets::clearPreset()
{
    fgSetString(kPresetNameProp, "");
    fgSetString(kPresetDescriptionProp, "");
    fgSetBool(kPresetActiveProp, false);
    _listener->unregisterFromProperties();
}

bool GraphicsPresets::loadPresetXML(const SGPath& p, GraphicsPresetInfo& info)
{
    SGPropertyNode_ptr props(new SGPropertyNode);

    try {
        readProperties(p, props.get());
    } catch (sg_exception& e) {
        SG_LOG(SG_IO, SG_ALERT, "XML errors loading " << p.str() << "\n\t" << e.getFormattedMessage());
        return false;
    }

    const string id = props->getStringValue("id");
    const string rawName = props->getStringValue("name");
    const string rawDesc = props->getStringValue("description");
    int orderNum = props->getIntValue("order-num", 99);

    if (id.empty() || rawName.empty() || rawDesc.empty()) {
        SG_LOG(SG_IO, SG_ALERT, "Missing preset info loading: " << p.str());
        return false;
    }

    info.id = id;
    info.name = globals->get_locale()->getLocalizedString(rawName, "graphics-presets");
    info.description = globals->get_locale()->getLocalizedString(rawDesc, "graphics-presets");
    info.orderNum = orderNum;

    if (info.name.empty())
        info.name = rawName; // no translation defined
    if (info.description.empty())
        info.description = rawDesc;

    info.properties = props->getChild("settings");
    if (!info.properties) {
        SG_LOG(SG_IO, SG_ALERT, "Missing settings loading: " << p.str());
        return false;
    }

    // read devices list
    info.devices.clear();
    SGPropertyNode_ptr devices = props->getChild("devices");
    if (devices) {
        for (auto d : devices->getChildren("device")) {
            const auto t = simgear::strutils::strip(d->getStringValue());
            info.devices.push_back(t);
        }
    }

    return true;
}

bool GraphicsPresets::saveToXML(const SGPath& path, const std::string& name)
{
    SGPropertyNode_ptr presetXML(new SGPropertyNode);
    presetXML->setStringValue("id", path.file_base()); // without .xml
    presetXML->setStringValue("name", name);
    // no description

    auto settingsNode = presetXML->getChild("settings", true);

    copyPropertiesIf(globals->get_props(), settingsNode, [this](const SGPropertyNode* nd) {
        // TODO
        return true;
    });

    return true;
}

} // namespace flightgear
