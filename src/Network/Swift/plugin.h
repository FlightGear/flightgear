/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once



#include "config.h"
#include "dbusconnection.h"
#include "dbusdispatcher.h"
#include "dbusserver.h"
#include <memory>
#include <thread>

namespace flightgear::swift {
class CService;
class CTraffic;

/*!
 * Plugin loaded by Flightgear which publishes a DBus service
 */
class CPlugin
{
public:
    //! Constructor
    CPlugin();
    void startServer();
    //! Destructor
    ~CPlugin();
    void fastLoop();

private:
    CDBusDispatcher m_dbusDispatcher;
    std::unique_ptr<CDBusServer> m_dbusP2PServer;
    std::shared_ptr<CDBusConnection> m_dbusConnection;
    std::unique_ptr<CService> m_service;
    std::unique_ptr<CTraffic> m_traffic;

    std::thread m_dbusThread;
};
} // namespace flightgear::swift
