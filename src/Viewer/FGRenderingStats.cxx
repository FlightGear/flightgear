/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "FGRenderingStats.hxx"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <Viewer/renderer.hxx>

namespace {

const unsigned int k_max_allocated_frames{300};
// We want milliseconds, but OSG gives us seconds
const double k_time_multiplier{1000.0};

const char* k_stats_enabled_prop{"/sim/rendering/stats/enabled"};
const char* k_stats_averaged_prop{"/sim/rendering/stats/averaged"};
const char* k_stats_allocated_frames_prop{"/sim/rendering/stats/allocated-frames"};
const char* k_stats_timings_prop{"/sim/rendering/stats/timings"};

const std::vector<std::pair<std::string, std::string>> k_viewer_stat_name_map = {
    {"Framerate",                   "framerate"},
    {"Event traversal time taken",  "event-traversal"},
    {"Event traversal begin time",  "event-traversal-begin"},
    {"Event traversal end time",    "event-traversal-end"},
    {"Update traversal time taken", "update-traversal"},
    {"Update traversal begin time", "update-traversal-begin"},
    {"Update traversal end time",   "update-traversal-end"},
};
const std::vector<std::pair<std::string, std::string>> k_camera_stat_name_map = {
    {"Cull traversal time taken",   "cull-traversal"},
    {"Cull traversal begin time",   "cull-traversal-begin"},
    {"Cull traversal end time",     "cull-traversal-end"},
    {"Draw traversal time taken",   "draw-traversal"},
    {"Draw traversal begin time",   "draw-traversal-begin"},
    {"Draw traversal end time",     "draw-traversal-end"},
    {"GPU draw time taken",         "gpu-draw"},
    {"GPU draw begin time",         "gpu-draw-begin"},
    {"GPU draw end time",           "gpu-draw-end"},
};

} // anonymous namespace

FGRenderingStats::FGRenderingStats() :
    _averaged(k_stats_averaged_prop),
    _num_allocated_frames(k_stats_allocated_frames_prop)
{

}

void FGRenderingStats::init()
{

}

void FGRenderingStats::shutdown()
{
}

void FGRenderingStats::update(double delta_time_sec)
{
    SG_UNUSED(delta_time_sec);

    osgViewer::ViewerBase* viewer = globals->get_renderer()->getViewerBase();
    if (!viewer) {
        return;
    }

    // Enable or disable stat collecting if needed.
    // We could use a listener here, but no idea how long OSG takes to actually
    // enable stat collecting. We do it in the main loop just to be safe.
    bool should_collect = fgGetBool(k_stats_enabled_prop, false);
    if (should_collect != _enabled) {
        set_collect_stats(viewer, should_collect);
        _enabled = should_collect;
    }

    // Copy the stats to the property tree
    if (_enabled) {
        SGPropertyNode* timings_node = fgGetNode(k_stats_timings_prop, true);
        copy_stats_to_prop_tree(viewer, timings_node);
    }
}

void FGRenderingStats::set_collect_stats(osgViewer::ViewerBase* viewer,
                                         bool enabled) const noexcept
{
    // Viewer stats
    osg::Stats* viewer_stats = viewer->getViewerStats();
    if (!viewer_stats) {
        return;
    }
    if (viewer_stats) {
        viewer_stats->collectStats("frame_rate", enabled);
        viewer_stats->collectStats("event",      enabled);
        viewer_stats->collectStats("update",     enabled);
        // viewer_stats->collectStats("scene",      enabled);
    }

    // Per-camera stats
    osgViewer::ViewerBase::Cameras cameras;
    viewer->getCameras(cameras);

    for (osg::Camera* camera : cameras) {
        osg::Stats* camera_stats = camera->getStats();
        if (camera_stats) {
            camera_stats->collectStats("rendering", enabled);
            camera_stats->collectStats("gpu",       enabled);
            // camera_stats->collectStats("scene",     enabled);
        }
    }
}

void FGRenderingStats::copy_stats_to_prop_tree(const osgViewer::ViewerBase* viewer,
                                               SGPropertyNode* node) const noexcept
{

    // Copy the viewer stats
    SGPropertyNode* viewer_prop = node->getChild("viewer", 0, true);
    const osg::Stats* viewer_stats = viewer->getViewerStats();
    if (!viewer_stats) {
        return;
    }
    for (const auto& name_pair : k_viewer_stat_name_map) {
        double value;
        get_stat_attribute(viewer_stats, name_pair.first, value);
        viewer_prop->setDoubleValue(name_pair.second, value);
    }

    // Now the camera stats
    osgViewer::ViewerBase::Cameras cameras;
    const_cast<osgViewer::ViewerBase*>(viewer)->getCameras(cameras);

    int camera_index = 0;
    for (const osg::Camera* camera : cameras) {
        SGPropertyNode* camera_prop = node->getChild("camera", camera_index, true);
        const osg::Stats* camera_stats = camera->getStats();
        if (!camera_stats) {
            continue;
        }

        camera_prop->setStringValue("name", camera->getName());
        camera_prop->setIntValue("render-order", camera->getRenderOrder());
        camera_prop->setIntValue("render-order-num", camera->getRenderOrderNum());

        for (const auto& name_pair : k_camera_stat_name_map) {
            double value;
            get_stat_attribute(camera_stats, name_pair.first, value);
            camera_prop->setDoubleValue(name_pair.second, value);
        }
        ++camera_index;
    }

    // XXX: Remove unnecessary camera nodes
}

bool FGRenderingStats::get_stat_attribute(const osg::Stats* stats,
                                          const std::string& name,
                                          double& value) const noexcept
{
    if (!stats) {
        return false;
    }
    bool ret = false;
    if (_averaged) {
        ret = stats->getAveragedAttribute(name, value);
    } else {
        ret = stats->getAttribute(stats->getLatestFrameNumber(), name, value);
    }
    value *= k_time_multiplier;
    return ret;
}

// Register the subsystem
SGSubsystemMgr::Registrant<FGRenderingStats> registrantFGRenderingStats(
    SGSubsystemMgr::DISPLAY,
    {
      {"viewer", SGSubsystemMgr::Dependency::HARD},
    });
