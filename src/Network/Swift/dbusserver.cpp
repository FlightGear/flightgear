#include <utility>

// dbusserver.cpp 
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

#include "dbusserver.h"
#include "dbusobject.h"

#include <algorithm>
#include <cassert>
#include <memory>

namespace FGSwiftBus
{

    CDBusServer::CDBusServer()
    {
        dbus_threads_init_default();
    }

    CDBusServer::~CDBusServer()
    {
        close();
    }

    bool CDBusServer::listen(const std::string &address)
    {
        DBusError error;
        dbus_error_init(&error);
        m_server.reset(dbus_server_listen(address.c_str(), &error));

        if (! m_server)
        {
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

    void CDBusServer::setDispatcher(CDBusDispatcher *dispatcher)
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

    void CDBusServer::onNewConnection(DBusServer *, DBusConnection *conn)
    {
        auto dbusConnection = std::make_shared<CDBusConnection>(conn);
        m_newConnectionFunc(dbusConnection);
    }

    void CDBusServer::onNewConnection(DBusServer *server, DBusConnection *conn, void *data)
    {
        auto *obj = static_cast<CDBusServer *>(data);
        obj->onNewConnection(server, conn);
    }

}
