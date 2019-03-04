// dbusserver.h
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

#ifndef BLACKSIM_FGSWIFTBUS_DBUSSERVER_H
#define BLACKSIM_FGSWIFTBUS_DBUSSERVER_H

#include "dbusmessage.h"
#include "dbuserror.h"
#include "dbuscallbacks.h"
#include "dbusdispatcher.h"

#include <event2/event.h>
#include <dbus/dbus.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

namespace FGSwiftBus
{

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
        ~CDBusServer();

        //! Set the dispatcher
        void setDispatcher(CDBusDispatcher *dispatcher);

        //! Connect to bus
        bool listen(const std::string &address);

        //! Is connected?
        bool isConnected() const;

        void dispatch() override {}

        //! Close connection
        void close();

        //! Get the last error
        CDBusError lastError() const { return m_lastError; }

        //! Set the function to be used for handling new connections.
        void setNewConnectionFunc(const NewConnectionFunc &func)
        {
            m_newConnectionFunc = func;
        }

    private:
        void onNewConnection(DBusServer *server, DBusConnection *conn);
        static void onNewConnection(DBusServer *server, DBusConnection *conn, void *data);

        struct DBusServerDeleter
        {
            void operator()(DBusServer *obj) const { dbus_server_unref(obj); }
        };

        CDBusDispatcher *m_dispatcher = nullptr;
        std::unique_ptr<DBusServer, DBusServerDeleter> m_server;
        CDBusError m_lastError;
        NewConnectionFunc m_newConnectionFunc;
    };

}

#endif // guard
