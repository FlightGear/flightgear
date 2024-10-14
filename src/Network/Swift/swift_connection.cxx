/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "plugin.h"
#include "swift_connection.hxx"
#include <Main/fg_props.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

namespace flightgear::swift {

bool SwiftConnection::startServer([[maybe_unused]] const SGPropertyNode*, [[maybe_unused]] SGPropertyNode* root)
{
    m_plugin = std::make_unique<CPlugin>();
    return true;
}

bool SwiftConnection::stopServer([[maybe_unused]] const SGPropertyNode* arg, [[maybe_unused]] SGPropertyNode* root)
{
    m_plugin.reset();
    return true;
}

SwiftConnection::~SwiftConnection()
{
    shutdownSwift();
}

void SwiftConnection::init()
{
    if (!m_initialized) {
        globals->get_commands()->addCommand("swiftStart", this, &SwiftConnection::startServer);
        globals->get_commands()->addCommand("swiftStop", this, &SwiftConnection::stopServer);

        fgSetBool("/sim/swift/available", true);
        m_initialized = true;
    }
}

void SwiftConnection::update(double delta_time_sec)
{
    if (m_plugin) {
        m_plugin->fastLoop();
    }
}

void SwiftConnection::shutdown()
{
    shutdownSwift();
}

void SwiftConnection::shutdownSwift()
{
    if (m_initialized) {
        fgSetBool("/sim/swift/available", false);
        m_initialized = false;

        globals->get_commands()->removeCommand("swiftStart");
        globals->get_commands()->removeCommand("swiftStop");
    }
}

void SwiftConnection::reinit()
{
    shutdown();
    init();
}

} // namespace flightgear::swift

// Register the subsystem.
SGSubsystemMgr::Registrant<flightgear::swift::SwiftConnection> registrantSwiftConnection(
    SGSubsystemMgr::POST_FDM);
