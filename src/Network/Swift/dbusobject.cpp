/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dbusobject.h"

#include <algorithm>
#include <cassert>

namespace flightgear::swift {

CDBusObject::~CDBusObject()
{
    if (m_dbusConnection) { m_dbusConnection->unregisterDisconnectedCallback(this); }
};

void CDBusObject::setDBusConnection(const std::shared_ptr<CDBusConnection>& dbusConnection)
{
    m_dbusConnection = dbusConnection;
    dbusConnectedHandler();
    CDBusConnection::DisconnectedCallback disconnectedHandler = std::bind(&CDBusObject::dbusDisconnectedHandler, this);
    m_dbusConnection->registerDisconnectedCallback(this, disconnectedHandler);
}

void CDBusObject::registerDBusObjectPath(const std::string& interfaceName, const std::string& objectPath)
{
    assert(m_dbusConnection);
    m_interfaceName = interfaceName;
    m_objectPath = objectPath;
    m_dbusConnection->registerObjectPath(this, interfaceName, objectPath, m_dbusObjectPathVTable);
}

void CDBusObject::sendDBusSignal(const std::string& name)
{
    if (!m_dbusConnection) { return; }
    CDBusMessage signal = CDBusMessage::createSignal(m_objectPath, m_interfaceName, name);
    m_dbusConnection->sendMessage(signal);
}

void CDBusObject::sendDBusMessage(const CDBusMessage& message)
{
    if (!m_dbusConnection) { return; }
    m_dbusConnection->sendMessage(message);
}

void CDBusObject::maybeSendEmptyDBusReply(bool wantsReply, const std::string& destination, dbus_uint32_t serial)
{
    if (wantsReply) {
        CDBusMessage reply = CDBusMessage::createReply(destination, serial);
        m_dbusConnection->sendMessage(reply);
    }
}

void CDBusObject::queueDBusCall(const std::function<void()>& func)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queuedDBusCalls.push_back(func);
}

void CDBusObject::invokeQueuedDBusCalls()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::for_each(m_queuedDBusCalls.begin(), m_queuedDBusCalls.end(),
                  [](const auto& dbusCall) { dbusCall(); });
    m_queuedDBusCalls.clear();
}

void CDBusObject::dbusObjectPathUnregisterFunction(DBusConnection* connection, void* data)
{
    (void)connection; // unused
    (void)data;       // unused
}

DBusHandlerResult CDBusObject::dbusObjectPathMessageFunction(DBusConnection* connection, DBusMessage* message, void* data)
{
    (void)connection; // unused

    auto* obj = static_cast<CDBusObject*>(data);

    DBusError err;
    dbus_error_init(&err);

    CDBusMessage dbusMessage(message);
    return obj->dbusMessageHandler(dbusMessage);
}

} // namespace flightgear::swift
