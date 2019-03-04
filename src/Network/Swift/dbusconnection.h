// dbusconnection.h
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

#ifndef BLACKSIM_FGSWIFTBUS_DBUSCONNECTION_H
#define BLACKSIM_FGSWIFTBUS_DBUSCONNECTION_H

#include "dbusmessage.h"
#include "dbuserror.h"
#include "dbuscallbacks.h"
#include "dbusdispatcher.h"

#include <event2/event.h>
#include <dbus/dbus.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace FGSwiftBus
{

    class CDBusObject;

    //! DBus connection
    class CDBusConnection : public IDispatchable
    {
    public:
        //! Bus type
        enum BusType { SessionBus };

        //! Disconnect Callback
        using DisconnectedCallback = std::function<void()>;

        //! Default constructor
        CDBusConnection();

        //! Constructor
        CDBusConnection(DBusConnection *connection);

        //! Destructor
        ~CDBusConnection() override;

        // The ones below are not implemented yet.
        // If you need them, make sure that connection reference count is correct
        CDBusConnection(const CDBusConnection &) = delete;
        CDBusConnection &operator=(const CDBusConnection &) = delete;

        //! Connect to bus
        bool connect(BusType type);

        //! Set dispatcher
        void setDispatcher(CDBusDispatcher *dispatcher);

        //! Request name to the bus
        void requestName(const std::string &name);

        //! Is connected?
        bool isConnected() const;

        //! Register a disconnected callback
        void registerDisconnectedCallback(CDBusObject *obj, DisconnectedCallback func);

        //! Register a disconnected callback
        void unregisterDisconnectedCallback(CDBusObject *obj);

        //! Register DBus object with interfaceName and objectPath.
        //! \param object
        //! \param interfaceName
        //! \param objectPath
        //! \param dbusObjectPathVTable Virtual table handling DBus messages
        void registerObjectPath(CDBusObject *object, const std::string &interfaceName, const std::string &objectPath, const DBusObjectPathVTable &dbusObjectPathVTable);

        //! Send message to bus
        void sendMessage(const CDBusMessage &message);

        //! Close connection
        void close();

        //! Get the last error
        CDBusError lastError() const { return m_lastError; }

    protected:
        // cppcheck-suppress virtualCallInConstructor
        virtual void dispatch() override final;

    private:
        void setDispatchStatus(DBusConnection *connection, DBusDispatchStatus status);
        static void setDispatchStatus(DBusConnection *connection, DBusDispatchStatus status, void *data);
        static DBusHandlerResult filterDisconnectedFunction(DBusConnection *connection, DBusMessage *message, void *data);

        struct DBusConnectionDeleter
        {
            void operator()(DBusConnection *obj) const { dbus_connection_unref(obj); }
        };

        CDBusDispatcher *m_dispatcher = nullptr;
        std::unique_ptr<DBusConnection, DBusConnectionDeleter> m_connection;
        CDBusError m_lastError;
        std::unordered_map<CDBusObject *, DisconnectedCallback> m_disconnectedCallbacks;
    };

}

#endif // guard
