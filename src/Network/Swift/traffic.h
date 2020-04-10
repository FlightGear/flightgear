// traffic.h - Traffic module for swift<->FG connection
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

#ifndef BLACKSIM_FGSWIFTBUS_TRAFFIC_H
#define BLACKSIM_FGSWIFTBUS_TRAFFIC_H

//! \file

#include "SwiftAircraftManager.h"
#include "dbusobject.h"
#include <functional>
#include <utility>

//! \cond PRIVATE
#define FGSWIFTBUS_TRAFFIC_INTERFACENAME "org.swift_project.fgswiftbus.traffic"
#define FGSWIFTBUS_TRAFFIC_OBJECTPATH "/fgswiftbus/traffic"
//! \endcond

namespace FGSwiftBus {
/*!
     * FGSwiftBus service object for traffic aircraft which is accessible through DBus
     */
class CTraffic : public CDBusObject
{
public:
    //! Constructor
    CTraffic();

    //! Destructor
    ~CTraffic() override;

    //! DBus interface name
    static const std::string& InterfaceName();
    
	//! DBus object path
    static const std::string& ObjectPath();

    //! Initialize the multiplayer planes rendering and return true if successful
    bool initialize();

    //! Remove all traffic aircraft
    void removeAllPlanes();

    //! Perform generic processing
    int process();

    void emitSimFrame();

protected:
    virtual void dbusDisconnectedHandler() override;

    DBusHandlerResult dbusMessageHandler(const CDBusMessage& message) override;

private:
    bool m_initialized = false;
    bool m_enabled     = false;

    void emitPlaneAdded(const std::string& callsign);
    void emitPlaneAddingFailed(const std::string& callsign);
    void cleanup();

    struct Plane {
        void*                                 id = nullptr;
        std::string                           callsign;
        std::string                           aircraftIcao;
        std::string                           airlineIcao;
        std::string                           livery;
        std::string                           modelName;
        bool                                  hasSurfaces = false;
        bool                                  hasXpdr     = false;
        char                                  label[32]{};
        float                                 targetGearPosition = 0;
        std::chrono::system_clock::time_point prevSurfacesLerpTime;
    };

    std::unordered_map<std::string, Plane*> m_planesByCallsign;
    std::unordered_map<void*, Plane*>       m_planesById;
    std::vector<std::string>                m_followPlaneViewSequence;
    std::chrono::system_clock::time_point   m_timestampLastSimFrame = std::chrono::system_clock::now();

    std::string m_followPlaneViewCallsign;

    bool m_emitSimFrame = true;


    std::unique_ptr<FGSwiftAircraftManager> acm;

    static void planeLoaded(void* id, bool succeeded, void* self);
};
} // namespace FGSwiftBus

#endif // guard
