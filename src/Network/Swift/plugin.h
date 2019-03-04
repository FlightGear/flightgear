// plugin.h
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

#ifndef BLACKSIM_FGSWIFTBUS_PLUGIN_H
#define BLACKSIM_FGSWIFTBUS_PLUGIN_H

//! \file

/*!
 * \namespace FGSwiftBus
 * Plugin loaded by Flightgear which publishes a DBus service
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "dbusconnection.h"
#include "dbusdispatcher.h"
#include "dbusserver.h"
#include "config.h"
#include <memory>
#include <thread>

namespace FGSwiftBus
{
    class CService;
    class CTraffic;
    class CWeather;

    /*!
     * Main plugin class
     */
    class CPlugin
    {
    public:
        //! Constructor
        CPlugin();
        void startServer();
        //! Destructor
        ~CPlugin();
        static float startServerDeferred(float, float, int, void* refcon);
        void         fastLoop();
		
    private:
        CDBusDispatcher m_dbusDispatcher;
        std::unique_ptr<CDBusServer> m_dbusP2PServer;
        std::shared_ptr<CDBusConnection> m_dbusConnection;
        std::unique_ptr<CService> m_service;
        std::unique_ptr<CTraffic> m_traffic;

        std::thread m_dbusThread;
        bool m_isRunning = false;
        bool m_shouldStop = false;
    };
}

#endif // guard
