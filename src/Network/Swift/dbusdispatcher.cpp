/*
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dbusdispatcher.h"
#include "dbusconnection.h"
#include <algorithm>

namespace { // anonymous namespace

template <typename T, typename... Args>
std::unique_ptr<T> our_make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} // end of anonymous namespace

namespace flightgear::swift {

//! Functor struct deleteing an event
struct EventDeleter {
    //! Delete functor
    void operator()(event* obj) const
    {
        event_del(obj);
        event_free(obj);
    }
};

//! DBus watch handler
class WatchHandler
{
public:
    //! Constructor
    WatchHandler(event_base* base, DBusWatch* watch)
        : m_base(base), m_watch(watch)
    {
        const unsigned int flags = dbus_watch_get_flags(watch);
        short monitoredEvents = EV_PERSIST;

        if (flags & DBUS_WATCH_READABLE) { monitoredEvents |= EV_READ; }
        if (flags & DBUS_WATCH_WRITABLE) { monitoredEvents |= EV_WRITE; }

        const int fd = dbus_watch_get_unix_fd(watch);
        m_event.reset(event_new(m_base, fd, monitoredEvents, callback, this));
        event_add(m_event.get(), nullptr);
    }

    //! Get DBus watch
    DBusWatch* getWatch() { return m_watch; }

    //! Get DBus watch
    const DBusWatch* getWatch() const { return m_watch; }

private:
    //! Event callback
    static void callback(evutil_socket_t fd, short event, void* data)
    {
        (void)fd; // Not really unused, but GCC/Clang still complain about it.
        auto* watchHandler = static_cast<WatchHandler*>(data);

        unsigned int flags = 0;
        if (event & EV_READ) { flags |= DBUS_WATCH_READABLE; }
        if (event & EV_WRITE) { flags |= DBUS_WATCH_WRITABLE; }
        dbus_watch_handle(watchHandler->m_watch, flags);
    }

    event_base* m_base = nullptr;
    std::unique_ptr<event, EventDeleter> m_event;
    DBusWatch* m_watch = nullptr;
};

//! DBus timeout handler
class TimeoutHandler
{
public:
    //! Constructor
    TimeoutHandler(event_base* base, DBusTimeout* timeout)
        : m_base(base), m_timeout(timeout)
    {
        timeval timer;
        const int interval = dbus_timeout_get_interval(timeout);
        timer.tv_sec = interval / 1000;
        timer.tv_usec = (interval % 1000) * 1000;

        m_event.reset(evtimer_new(m_base, callback, this));
        evtimer_add(m_event.get(), &timer);
    }

    //! Get DBus timeout
    const DBusTimeout* getTimeout() const { return m_timeout; }

private:
    //! Event callback
    static void callback(evutil_socket_t fd, short event, void* data)
    {
        (void)fd;    // unused
        (void)event; // unused
        auto* timeoutHandler = static_cast<TimeoutHandler*>(data);
        dbus_timeout_handle(timeoutHandler->m_timeout);
    }

    event_base* m_base = nullptr;
    std::unique_ptr<event, EventDeleter> m_event;
    DBusTimeout* m_timeout = nullptr;
};

//! Generic Timer
class Timer
{
public:
    Timer() = default;
    //! Constructor
    Timer(event_base* base, const timeval& timeout, const std::function<void()>& func)
        : m_base(base), m_func(func)
    {
        m_event.reset(evtimer_new(m_base, callback, this));
        evtimer_add(m_event.get(), &timeout);
    }

private:
    //! Event callback
    static void callback(evutil_socket_t fd, short event, void* data)
    {
        (void)fd;    // unused
        (void)event; // unused
        auto* timer = static_cast<Timer*>(data);
        timer->m_func();
        delete timer;
    }

    event_base* m_base = nullptr;
    std::unique_ptr<event, EventDeleter> m_event;
    std::function<void()> m_func;
};

CDBusDispatcher::CDBusDispatcher() : m_eventBase(event_base_new())
{
    using namespace std::placeholders;
    m_watchCallbacks = WatchCallbacks(std::bind(&CDBusDispatcher::dbusAddWatch, this, _1),
                                      std::bind(&CDBusDispatcher::dbusRemoveWatch, this, _1),
                                      std::bind(&CDBusDispatcher::dbusWatchToggled, this, _1));

    m_timeoutCallbacks = TimeoutCallbacks(std::bind(&CDBusDispatcher::dbusAddTimeout, this, _1),
                                          std::bind(&CDBusDispatcher::dbusRemoveTimeout, this, _1),
                                          std::bind(&CDBusDispatcher::dbusTimeoutToggled, this, _1));
}

CDBusDispatcher::~CDBusDispatcher()
{
}

void CDBusDispatcher::add(IDispatchable* dispatchable)
{
    m_dispatchList.push_back(dispatchable);
}

void CDBusDispatcher::remove(IDispatchable* dispatchable)
{
    auto it = std::find(m_dispatchList.begin(), m_dispatchList.end(), dispatchable);
    if (it != m_dispatchList.end()) { m_dispatchList.erase(it); }
}

void CDBusDispatcher::waitAndRun()
{
    if (!m_eventBase) { return; }
    event_base_dispatch(m_eventBase.get());
}

void CDBusDispatcher::runOnce()
{
    if (!m_eventBase) { return; }
    event_base_loop(m_eventBase.get(), EVLOOP_NONBLOCK);
    dispatch();
}

void CDBusDispatcher::dispatch()
{
    if (m_dispatchList.empty()) { return; }

    for (IDispatchable* dispatchable : m_dispatchList) {
        dispatchable->dispatch();
    }
}

dbus_bool_t CDBusDispatcher::dbusAddWatch(DBusWatch* watch)
{
    if (dbus_watch_get_enabled(watch) == FALSE) { return true; }

    int fd = dbus_watch_get_unix_fd(watch);
    m_watchers.emplace(fd, our_make_unique<WatchHandler>(m_eventBase.get(), watch));
    return true;
}

void CDBusDispatcher::dbusRemoveWatch(DBusWatch* watch)
{
    for (auto it = m_watchers.begin(); it != m_watchers.end();) {
        if (it->second->getWatch() == watch) {
            it = m_watchers.erase(it);
        } else {
            ++it;
        }
    }
}

void CDBusDispatcher::dbusWatchToggled(DBusWatch* watch)
{
    if (dbus_watch_get_enabled(watch) == TRUE) {
        dbusAddWatch(watch);
    } else {
        dbusRemoveWatch(watch);
    }
}

dbus_bool_t CDBusDispatcher::dbusAddTimeout(DBusTimeout* timeout)
{
    if (dbus_timeout_get_enabled(timeout) == FALSE) { return TRUE; }
    m_timeouts.emplace_back(new TimeoutHandler(m_eventBase.get(), timeout));
    return true;
}

void CDBusDispatcher::dbusRemoveTimeout(DBusTimeout* timeout)
{
    auto predicate = [timeout](const std::unique_ptr<TimeoutHandler>& ptr) {
        return ptr->getTimeout() == timeout;
    };

    m_timeouts.erase(std::remove_if(m_timeouts.begin(), m_timeouts.end(), predicate), m_timeouts.end());
}

void CDBusDispatcher::dbusTimeoutToggled(DBusTimeout* timeout)
{
    if (dbus_timeout_get_enabled(timeout) == TRUE)
        dbusAddTimeout(timeout);
    else
        dbusRemoveTimeout(timeout);
}

} // namespace flightgear::swift
