/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <simgear/props/propertyObject.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

namespace osg {
class Stats;
}
namespace osgViewer {
class ViewerBase;
};

/*
 * Collect OpenSceneGraph rendering-related statistics and expose them to the
 * property tree.
 */
class FGRenderingStats final : public SGSubsystem {
public:
    static const char* staticSubsystemClassId() { return "rendering-stats"; }

    FGRenderingStats();

    void init() override;
    void shutdown() override;
    void update(double delta_time_sec) override;
private:
    void set_collect_stats(osgViewer::ViewerBase* viewer,
                           bool enabled) const noexcept;
    void copy_stats_to_prop_tree(const osgViewer::ViewerBase* viewer,
                                 SGPropertyNode* node) const noexcept;
    bool get_stat_attribute(const osg::Stats* stats,
                            const std::string& name,
                            double& value) const noexcept;

    bool _enabled{false};

    simgear::PropertyObject<bool> _averaged;
    simgear::PropertyObject<int> _num_allocated_frames;

    SGSharedPtr<SGPropertyNode> _viewer_stat_prop;
    typedef std::pair<osg::Stats*, SGPropertyNode*> StatPropPair;
    std::vector<StatPropPair> _camera_stat_prop_pairs;
};
