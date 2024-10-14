/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dbusconnection.h"
#include "dbusobject.h"

#include <cassert>

namespace flightgear::swift {

CDBusConnection::CDBusConnection()
{
    dbus_threads_init_default();
}

CDBusConnection::CDBusConnection(DBusConnection* connection)
{
    m_connection.reset(connection);
    dbus_connection_ref(connection);
    // Don't exit application, if the connection is disconnected
    dbus_connection_set_exit_on_disconnect(connection, false);
    dbus_connection_add_filter(connection, filterDisconnectedFunction, this, nullptr);
}

CDBusConnection::~CDBusConnection()
{
    close();
    if (m_connection) { dispatch(); } // dispatch is virtual, but safe to call in dtor, as it's declared final
    if (m_dispatcher) { m_dispatcher->remove(this); }
}

bool CDBusConnection::connect(BusType type)
{
    assert(type == SessionBus);
    DBusError error;
    dbus_error_init(&error);

    DBusBusType dbusBusType;
    switch (type) {
    case SessionBus: dbusBusType = DBUS_BUS_SESSION; break;
    }

    m_connection.reset(dbus_bus_get_private(dbusBusType, &error));
    if (dbus_error_is_set(&error)) {
        m_lastError = CDBusError(&error);
        return false;
    }

    // Don't exit application, if the connection is disconnected
    dbus_connection_set_exit_on_disconnect(m_connection.get(), false);
    return true;
}

void CDBusConnection::setDispatcher(CDBusDispatcher* dispatcher)
{
    assert(dispatcher);

    m_dispatcher = dispatcher;

    m_dispatcher->add(this);

    dbus_connection_set_watch_functions(
        m_connection.get(),
        dispatcher->m_watchCallbacks.add,
        dispatcher->m_watchCallbacks.remove,
        dispatcher->m_watchCallbacks.toggled,
        &dispatcher->m_watchCallbacks, nullptr);

    dbus_connection_set_timeout_functions(
        m_connection.get(),
        dispatcher->m_timeoutCallbacks.add,
        dispatcher->m_timeoutCallbacks.remove,
        dispatcher->m_timeoutCallbacks.toggled,
        &dispatcher->m_timeoutCallbacks, nullptr);
}

void CDBusConnection::requestName(const std::string& name)
{
    DBusError error;
    dbus_error_init(&error);
    dbus_bus_request_name(m_connection.get(), name.c_str(), 0, &error);
}

bool CDBusConnection::isConnected() const
{
    return m_connection && dbus_connection_get_is_connected(m_connection.get());
}

void CDBusConnection::registerDisconnectedCallback(CDBusObject* obj, DisconnectedCallback func)
{
    m_disconnectedCallbacks[obj] = func;
}

void CDBusConnection::unregisterDisconnectedCallback(CDBusObject* obj)
{
    auto it = m_disconnectedCallbacks.find(obj);
    if (it == m_disconnectedCallbacks.end()) { return; }
    m_disconnectedCallbacks.erase(it);
}

void CDBusConnection::registerObjectPath(CDBusObject* object, const std::string& interfaceName, const std::string& objectPath, const DBusObjectPathVTable& dbusObjectPathVTable)
{
    (void)interfaceName;
    if (!m_connection) { return; }

    dbus_connection_try_register_object_path(m_connection.get(), objectPath.c_str(), &dbusObjectPathVTable, object, nullptr);
}

void CDBusConnection::sendMessage(const CDBusMessage& message)
{
    if (!isConnected()) { return; }
    dbus_uint32_t serial = message.getSerial();
    dbus_connection_send(m_connection.get(), message.m_message, &serial);
}

void CDBusConnection::close()
{
    if (m_connection) { dbus_connection_close(m_connection.get()); }
}

void CDBusConnection::dispatch()
{
    dbus_connection_ref(m_connection.get());
    if (dbus_connection_get_dispatch_status(m_connection.get()) == DBUS_DISPATCH_DATA_REMAINS) {
        while (dbus_connection_dispatch(m_connection.get()) == DBUS_DISPATCH_DATA_REMAINS)
            ;
    }
    dbus_connection_unref(m_connection.get());
}

void CDBusConnection::setDispatchStatus(DBusConnection* connection, DBusDispatchStatus status)
{
    if (dbus_connection_get_is_connected(connection) == FALSE) { return; }

    switch (status) {
    case DBUS_DISPATCH_DATA_REMAINS:
        //m_dispatcher->add(this);
        break;
    case DBUS_DISPATCH_COMPLETE:
    case DBUS_DISPATCH_NEED_MEMORY:
        break;
    }
}

void CDBusConnection::setDispatchStatus(DBusConnection* connection, DBusDispatchStatus status, void* data)
{
    auto* obj = static_cast<CDBusConnection*>(data);
    obj->setDispatchStatus(connection, status);
}

DBusHandlerResult CDBusConnection::filterDisconnectedFunction(DBusConnection* connection, DBusMessage* message, void* data)
{
    (void)connection; // unused

    auto* obj = static_cast<CDBusConnection*>(data);

    DBusError err;
    dbus_error_init(&err);

    if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected")) {
        for (auto it = obj->m_disconnectedCallbacks.begin(); it != obj->m_disconnectedCallbacks.end(); ++it) {
            it->second();
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

} // namespace flightgear::swift
