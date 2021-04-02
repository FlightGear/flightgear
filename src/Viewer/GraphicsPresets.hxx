#pragma once

#include <string>
#include <vector>

#include <simgear/misc/strutils.hxx>
#include <simgear/props/propsfwd.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

// forward decls
class SGPath;

namespace flightgear {

class GraphicsPresets : public SGSubsystem
{
public:
    GraphicsPresets();
    ~GraphicsPresets();

    static const char* staticSubsystemClassId() { return "graphics-presets"; }

    struct GraphicsPresetInfo {
        std::string id;
        std::string name;        // localized
        std::string description; // localized
        int orderNum;
        string_list devices;

        SGPropertyNode_ptr properties;
    };

    using GraphicsPresetVec = std::vector<GraphicsPresetInfo>;

    // implement SGSubsystem intergace
    void init() override;
    void shutdown() override;

    void update(double delta_time_sec) override;

    /**
        @brief init() is called too late (after fgOSInit), so we call this method early,
                to load the initial preset if set, at that time.
     */
    void applyInitialPreset();

    /**
     * @brief Apply the settings defined in the current graphics preset,
     * to the property tree
     *
     */
    bool applyCurrentPreset();

    bool applyCustomPreset(const SGPath& path);

    /**
        @brief apply a preset identified by its (localized) name. This is helpful
        for the rnedering dialog since PUI combo-boxes only record the name of
        items and no other data.
     */
    bool applyPresetByName(const std::string& name);

    /**
        @brief retrieve all standard presets which are defined
     */
    GraphicsPresetVec listPresets();

    bool saveToXML(const SGPath& path, const std::string& name, const std::string& desc);

private:
    void clearPreset();

    bool loadStandardPreset(const std::string& id, GraphicsPresetInfo& info);

    bool innerApplyPreset(const GraphicsPresetInfo& info, bool overwriteAutosaved);

    bool loadPresetXML(const SGPath& p, GraphicsPresetInfo& info);


    class RequiredPropertyListener;
    class GraphicsConfigChangeListener;

    std::unique_ptr<GraphicsConfigChangeListener> _listener;
    std::unique_ptr<RequiredPropertyListener> _restartListener;
    std::unique_ptr<RequiredPropertyListener> _sceneryReloadListener;
    std::unique_ptr<RequiredPropertyListener> _compositorReloadListener;

    string_list _propertiesToSave;
};

} // namespace flightgear
