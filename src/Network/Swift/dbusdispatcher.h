// dbusdispatcher.h 
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

#ifndef BLACKSIM_FGSWIFTBUS_DBUSDISPATCHER_H
#define BLACKSIM_FGSWIFTBUS_DBUSDISPATCHER_H

#include "dbuscallbacks.h"

#include <event2/event.h>
#include <dbus/dbus.h>

#include <unordered_map>
#include <vector>
#include <memory>

namespace FGSwiftBus
{

    class WatchHandler;
    class TimeoutHandler;
    class CDBusConnection;
    class CDBusDispatcher;

    //! Dispatchable Interface
    class IDispatchable
    {
    public:
        //! Default constructor
        IDispatchable() = default;

        //! Default destructor
        virtual ~IDispatchable() = default;

        //! Dispatch execution method
        virtual void dispatch() = 0;

    private:
        friend CDBusDispatcher;
    };

    //! DBus Dispatcher
    class CDBusDispatcher
    {
    public:
        //! Constructor
        CDBusDispatcher();

        //! Destructor
        virtual ~CDBusDispatcher();

        //! Add dispatchable object
        void add(IDispatchable *dispatchable);

        //! Remove dispatchable object
        void remove(IDispatchable *dispatchable);

        //! Waits for events to be dispatched and handles them
        void waitAndRun();

        //! Dispatches ready handlers and returns without waiting
        void runOnce();

    private:
        friend class WatchHandler;
        friend class TimeoutHandler;
        friend class Timer;
        friend class CDBusConnection;
        friend class CDBusServer;

        struct EventBaseDeleter
        {
            void operator()(event_base *obj) const { event_base_free(obj); }
        };

        using WatchCallbacks = DBusAsyncCallbacks<DBusWatch>;
        using TimeoutCallbacks = DBusAsyncCallbacks<DBusTimeout>;

        void dispatch();

        dbus_bool_t dbusAddWatch(DBusWatch *watch);
        void dbusRemoveWatch(DBusWatch *watch);
        void dbusWatchToggled(DBusWatch *watch);

        dbus_bool_t dbusAddTimeout(DBusTimeout *timeout);
        void dbusRemoveTimeout(DBusTimeout *timeout);
        void dbusTimeoutToggled(DBusTimeout *timeout);

        WatchCallbacks m_watchCallbacks;
        TimeoutCallbacks m_timeoutCallbacks;
        std::unordered_multimap<evutil_socket_t, std::unique_ptr<WatchHandler>> m_watchers;
        std::vector<std::unique_ptr<TimeoutHandler>> m_timeouts;
        std::unique_ptr<event_base, EventBaseDeleter> m_eventBase;

        std::vector<IDispatchable*> m_dispatchList;
    };
}

#endif
