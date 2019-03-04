// dbusobject.cpp 
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

#include "dbusobject.h"
#include <cassert>

namespace FGSwiftBus
{
    CDBusObject::CDBusObject()
    { }

    CDBusObject::~CDBusObject()
    {
        if (m_dbusConnection) { m_dbusConnection->unregisterDisconnectedCallback(this); }
    };

    void CDBusObject::setDBusConnection(const std::shared_ptr<CDBusConnection> &dbusConnection)
    {
        m_dbusConnection = dbusConnection;
        dbusConnectedHandler();
        CDBusConnection::DisconnectedCallback disconnectedHandler = std::bind(&CDBusObject::dbusDisconnectedHandler, this);
        m_dbusConnection->registerDisconnectedCallback(this, disconnectedHandler);
    }

    void CDBusObject::registerDBusObjectPath(const std::string &interfaceName, const std::string &objectPath)
    {
        assert(m_dbusConnection);
        m_interfaceName = interfaceName;
        m_objectPath = objectPath;
        m_dbusConnection->registerObjectPath(this, interfaceName, objectPath, m_dbusObjectPathVTable);
    }

    void CDBusObject::sendDBusSignal(const std::string &name)
    {
        if (! m_dbusConnection) { return; }
        CDBusMessage signal = CDBusMessage::createSignal(m_objectPath, m_interfaceName, name);
        m_dbusConnection->sendMessage(signal);
    }

    void CDBusObject::sendDBusMessage(const CDBusMessage &message)
    {
        if (! m_dbusConnection) { return; }
        m_dbusConnection->sendMessage(message);
    }

    void CDBusObject::maybeSendEmptyDBusReply(bool wantsReply, const std::string &destination, dbus_uint32_t serial)
    {
        if (wantsReply)
        {
            CDBusMessage reply = CDBusMessage::createReply(destination, serial);
            m_dbusConnection->sendMessage(reply);
        }
    }

    void CDBusObject::queueDBusCall(const std::function<void ()> &func)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_qeuedDBusCalls.push_back(func);
    }

    void CDBusObject::invokeQueuedDBusCalls()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (m_qeuedDBusCalls.size() > 0)
        {
            m_qeuedDBusCalls.front()();
            m_qeuedDBusCalls.pop_front();
        }
    }

    void CDBusObject::dbusObjectPathUnregisterFunction(DBusConnection *connection, void *data)
    {
        (void)connection; // unused
        (void)data; // unused
    }

    DBusHandlerResult CDBusObject::dbusObjectPathMessageFunction(DBusConnection *connection, DBusMessage *message, void *data)
    {
        (void)connection; // unused

        auto *obj = static_cast<CDBusObject *>(data);

        DBusError err;
        dbus_error_init(&err);

        CDBusMessage dbusMessage(message);
        return obj->dbusMessageHandler(dbusMessage);
    }

}
