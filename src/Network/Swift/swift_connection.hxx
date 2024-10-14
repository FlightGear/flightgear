/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <memory>

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "dbusconnection.h"
#include "dbusdispatcher.h"
#include "dbusserver.h"
#include "plugin.h"

namespace flightgear::swift {

class SwiftConnection : public SGSubsystem
{
public:
    SwiftConnection() = default;
    ~SwiftConnection();

    // Subsystem API.
    void init() override;
    void reinit() override;
    void shutdown() override;
    void update(double delta_time_sec) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "swift"; }

private:
    bool startServer(const SGPropertyNode* arg, SGPropertyNode* root);
    bool stopServer(const SGPropertyNode* arg, SGPropertyNode* root);

    // non-virtual shutdown function to avoid calling a virtual function from dtor for proper cleanup
    void shutdownSwift();

    std::unique_ptr<CPlugin> m_plugin;
    bool m_initialized = false;
};
} // namespace flightgear::swift
