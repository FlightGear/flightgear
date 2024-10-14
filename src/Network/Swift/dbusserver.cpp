/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dbusobject.h"
#include "dbusserver.h"

#include <cassert>
#include <memory>

namespace flightgear::swift {

CDBusServer::CDBusServer()
{
    dbus_threads_init_default();
}

CDBusServer::~CDBusServer()
{
    close();
}

bool CDBusServer::listen(const std::string& address)
{
    DBusError error;
    dbus_error_init(&error);
    m_server.reset(dbus_server_listen(address.c_str(), &error));

    if (!m_server) {
        return false;
    }

    dbus_server_set_new_connection_function(m_server.get(), onNewConnection, this, nullptr);
    return true;
}

bool CDBusServer::isConnected() const
{
    return m_server ? dbus_server_get_is_connected(m_server.get()) : false;
}

void CDBusServer::close()
{
    if (m_server) { dbus_server_disconnect(m_server.get()); }
}

void CDBusServer::setDispatcher(CDBusDispatcher* dispatcher)
{
    assert(dispatcher);
    assert(m_server);

    m_dispatcher = dispatcher;

    dbus_server_set_watch_functions(
        m_server.get(),
        dispatcher->m_watchCallbacks.add,
        dispatcher->m_watchCallbacks.remove,
        dispatcher->m_watchCallbacks.toggled,
        &dispatcher->m_watchCallbacks, nullptr);

    dbus_server_set_timeout_functions(
        m_server.get(),
        dispatcher->m_timeoutCallbacks.add,
        dispatcher->m_timeoutCallbacks.remove,
        dispatcher->m_timeoutCallbacks.toggled,
        &dispatcher->m_timeoutCallbacks, nullptr);
}

void CDBusServer::onNewConnection(DBusServer*, DBusConnection* conn)
{
    auto dbusConnection = std::make_shared<CDBusConnection>(conn);
    m_newConnectionFunc(dbusConnection);
}

void CDBusServer::onNewConnection(DBusServer* server, DBusConnection* conn, void* data)
{
    auto* obj = static_cast<CDBusServer*>(data);
    obj->onNewConnection(server, conn);
}

} // namespace flightgear::swift
