// swift_connection.hxx
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

#ifndef SWIFT_CONNECTION_H
#define SWIFT_CONNECTION_H
#include "dbusconnection.h"
#include "dbusdispatcher.h"
#include <Main/fg_props.hxx>
#include <simgear/compiler.h>
#include <simgear/io/raw_socket.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include "plugin.h"
#include "dbusserver.h"
#include <memory>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX
#endif

class SwiftConnection : public SGSubsystem
{
public:
    SwiftConnection();
    ~SwiftConnection();

    // Subsystem API.
    void init() override;
    void reinit() override;
    void shutdown() override;
    void update(double delta_time_sec) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "swift"; }

    bool startServer(const SGPropertyNode* arg, SGPropertyNode* root);
    bool stopServer(const SGPropertyNode* arg, SGPropertyNode* root);
    FGSwiftBus::CPlugin* plug{};

private:
    bool serverRunning = false;
    bool initialized   = false;
};

#endif
