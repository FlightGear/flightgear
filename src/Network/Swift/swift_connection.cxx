// swift_connection.cxx
//
// Copyright (C) 2019 - swift Project Community / Contributors (http://swift-project.org/)
// Adapted to Flightgear by Lars Toenning <dev@ltoenning.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "plugin.h"
#include "swift_connection.hxx"
#include <Main/fg_props.hxx>
#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/timing/timestamp.hxx>

namespace {
inline std::string fgswiftbusServiceName()
{
    return std::string("org.swift-project.fgswiftbus");
}
} // namespace

bool SwiftConnection::startServer(const SGPropertyNode* arg, SGPropertyNode* root)
{
    SwiftConnection::plug.reset(new FGSwiftBus::CPlugin{});

    serverRunning = true;
    fgSetBool("/sim/swift/serverRunning", true);

    return true;
}

bool SwiftConnection::stopServer(const SGPropertyNode* arg, SGPropertyNode* root)
{
    fgSetBool("/sim/swift/serverRunning", false);
    serverRunning = false;

    SwiftConnection::plug.reset();

    return true;
}

SwiftConnection::SwiftConnection()
{
    init();
}

SwiftConnection::~SwiftConnection()
{
    shutdown();

    if (serverRunning) {
        SwiftConnection::plug.reset();
    }
}

void SwiftConnection::init()
{
    if (!initialized) {
        globals->get_commands()->addCommand("swiftStart", this, &SwiftConnection::startServer);
        globals->get_commands()->addCommand("swiftStop", this, &SwiftConnection::stopServer);

        fgSetBool("/sim/swift/available", true);
        initialized = true;
    }
}

void SwiftConnection::update(double delta_time_sec)
{
    if (serverRunning) {
        SwiftConnection::plug->fastLoop();
    }
}

void SwiftConnection::shutdown()
{
    if (initialized) {
        fgSetBool("/sim/swift/available", false);
        initialized = false;

        globals->get_commands()->removeCommand("swiftStart");
        globals->get_commands()->removeCommand("swiftStop");
    }
}

void SwiftConnection::reinit()
{
    shutdown();
    init();
}

// Register the subsystem.
SGSubsystemMgr::Registrant<SwiftConnection> registrantSwiftConnection(
    SGSubsystemMgr::POST_FDM);
