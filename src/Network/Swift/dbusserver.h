/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "dbuscallbacks.h"
#include "dbusdispatcher.h"
#include "dbuserror.h"
#include "dbusmessage.h"

#include <dbus/dbus.h>
#include <event2/event.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace flightgear::swift {

class CDBusObject;

//! DBus connection
class CDBusServer : public IDispatchable
{
public:
    //! New connection handler function
    using NewConnectionFunc = std::function<void(std::shared_ptr<CDBusConnection>)>;

    //! Constructor
    CDBusServer();

    //! Destructor
    ~CDBusServer() override;

    //! Set the dispatcher
    void setDispatcher(CDBusDispatcher* dispatcher);

    //! Connect to bus
    bool listen(const std::string& address);

    //! Is connected?
    bool isConnected() const;

    void dispatch() override {}

    //! Close connection
    void close();

    //! Get the last error
    CDBusError lastError() const { return m_lastError; }

    //! Set the function to be used for handling new connections.
    void setNewConnectionFunc(const NewConnectionFunc& func)
    {
        m_newConnectionFunc = func;
    }

private:
    void onNewConnection(DBusServer* server, DBusConnection* conn);
    static void onNewConnection(DBusServer* server, DBusConnection* conn, void* data);

    struct DBusServerDeleter {
        void operator()(DBusServer* obj) const { dbus_server_unref(obj); }
    };

    CDBusDispatcher* m_dispatcher = nullptr;
    std::unique_ptr<DBusServer, DBusServerDeleter> m_server;
    CDBusError m_lastError;
    NewConnectionFunc m_newConnectionFunc;
};

} // namespace flightgear::swift
