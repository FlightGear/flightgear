// dbuscallbacks.h 
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

#ifndef BLACKSIM_FGSWIFTBUS_DBUSASYNCCALLBACKS_H
#define BLACKSIM_FGSWIFTBUS_DBUSASYNCCALLBACKS_H

#include <dbus/dbus.h>
#include <functional>

namespace FGSwiftBus
{
    //! \cond PRIVATE
    template <typename T>
    class DBusAsyncCallbacks
    {
    public:
        DBusAsyncCallbacks() = default;
        DBusAsyncCallbacks(const std::function<dbus_bool_t(T *)> &add,
                           const std::function<void(T *)> &remove,
                           const std::function<void(T *)> &toggled)
            : m_addHandler(add), m_removeHandler(remove), m_toggledHandler(toggled)
        { }

        static dbus_bool_t add(T *watch, void *refcon)
        {
            return static_cast<DBusAsyncCallbacks *>(refcon)->m_addHandler(watch);
        }

        static void remove(T *watch, void *refcon)
        {
            return static_cast<DBusAsyncCallbacks *>(refcon)->m_removeHandler(watch);
        }

        static void toggled(T *watch, void *refcon)
        {
            return static_cast<DBusAsyncCallbacks *>(refcon)->m_toggledHandler(watch);
        }

    private:
        std::function<dbus_bool_t(T *)> m_addHandler;
        std::function<void(T *)> m_removeHandler;
        std::function<void(T *)> m_toggledHandler;
    };
    //! \endcond

}

#endif // guard
