/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "plugin.h"
#include "service.h"
#include "traffic.h"

#include <Main/fg_props.hxx>
#include <Main/globals.hxx>
#include <functional>
#include <iostream>
#include <simgear/structure/commands.hxx>

namespace flightgear::swift {

CPlugin::CPlugin()
{
    startServer();
    fgSetBool("/sim/swift/serverRunning", true);
}

CPlugin::~CPlugin()
{
    if (m_dbusConnection) {
        m_dbusConnection->close();
    }
    if (m_dbusThread.joinable()) { m_dbusThread.join(); }
    fgSetBool("/sim/swift/serverRunning", false);
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

void CPlugin::fastLoop()
{
    this->m_dbusDispatcher.runOnce();
    this->m_service->process();
    this->m_traffic->process();
    this->m_traffic->emitSimFrame();
}
} // namespace flightgear::swift
