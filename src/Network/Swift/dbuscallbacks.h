/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <dbus/dbus.h>
#include <functional>

namespace flightgear::swift {

template <typename T>
class DBusAsyncCallbacks
{
public:
    DBusAsyncCallbacks() = default;
    DBusAsyncCallbacks(const std::function<dbus_bool_t(T*)>& add,
                       const std::function<void(T*)>& remove,
                       const std::function<void(T*)>& toggled)
        : m_addHandler(add), m_removeHandler(remove), m_toggledHandler(toggled)
    {
    }

    static dbus_bool_t add(T* watch, void* refcon)
    {
        return static_cast<DBusAsyncCallbacks*>(refcon)->m_addHandler(watch);
    }

    static void remove(T* watch, void* refcon)
    {
        return static_cast<DBusAsyncCallbacks*>(refcon)->m_removeHandler(watch);
    }

    static void toggled(T* watch, void* refcon)
    {
        return static_cast<DBusAsyncCallbacks*>(refcon)->m_toggledHandler(watch);
    }

private:
    std::function<dbus_bool_t(T*)> m_addHandler;
    std::function<void(T*)> m_removeHandler;
    std::function<void(T*)> m_toggledHandler;
};

} // namespace flightgear::swift
