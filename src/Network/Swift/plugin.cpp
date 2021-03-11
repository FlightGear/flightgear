// plugin.cpp
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

#include "plugin.h"
#include "service.h"
#include "traffic.h"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <cmath>
#include <functional>
#include <iostream>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <thread>

namespace {
inline std::string fgswiftbusServiceName()
{
    return std::string("org.swift-project.fgswiftbus");
}
} // namespace

namespace FGSwiftBus {
CPlugin::CPlugin()
{
    startServer();
}

CPlugin::~CPlugin()
{
    if (m_dbusConnection) {
        m_dbusConnection->close();
    }
    m_shouldStop = true;
    if (m_dbusThread.joinable()) { m_dbusThread.join(); }
}

void CPlugin::startServer()
{
    m_service.reset(new CService());
    m_traffic.reset(new CTraffic());
    m_dbusP2PServer.reset(new CDBusServer());

    std::string ip = fgGetString("/sim/swift/address", "127.0.0.1");
    std::string port = fgGetString("/sim/swift/port", "45003");
    std::string listenAddress = "tcp:host=" + ip + ",port=" + port;
    if (!m_dbusP2PServer->listen(listenAddress)) {
        m_service->addTextMessage("FGSwiftBus startup failed!");
        return;
    }

    m_dbusP2PServer->setDispatcher(&m_dbusDispatcher);
    m_dbusP2PServer->setNewConnectionFunc([this](const std::shared_ptr<CDBusConnection>& conn) {
        m_dbusConnection = conn;
        m_dbusConnection->setDispatcher(&m_dbusDispatcher);
        m_service->setDBusConnection(m_dbusConnection);
        m_service->registerDBusObjectPath(m_service->InterfaceName(), m_service->ObjectPath());
        m_traffic->setDBusConnection(m_dbusConnection);
        m_traffic->registerDBusObjectPath(m_traffic->InterfaceName(), m_traffic->ObjectPath());
    });

    SG_LOG(SG_NETWORK, SG_INFO, "FGSwiftBus started");
}

float CPlugin::startServerDeferred(float, float, int, void* refcon)
{
    auto plugin = static_cast<CPlugin*>(refcon);
    if (!plugin->m_isRunning) {
        plugin->startServer();
        plugin->m_isRunning = true;
    }

    return 0;
}

void CPlugin::fastLoop()
{
    this->m_dbusDispatcher.runOnce();
    this->m_service->process();
    this->m_traffic->process();
    this->m_traffic->emitSimFrame();
}
} // namespace FGSwiftBus
