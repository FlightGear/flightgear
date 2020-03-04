// service.h - Service module for swift<->FG connection 
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

#ifndef BLACKSIM_FGSWIFTBUS_SERVICE_H
#define BLACKSIM_FGSWIFTBUS_SERVICE_H

//! \file

#ifndef NOMINMAX
#define NOMINMAX
#endif


#include "dbusobject.h"
#include <chrono>
#include <string>
#include <Main/fg_props.hxx>
#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/timing/timestamp.hxx>


//! \cond PRIVATE
#define FGSWIFTBUS_SERVICE_INTERFACENAME "org.swift_project.fgswiftbus.service"
#define FGSWIFTBUS_SERVICE_OBJECTPATH "/fgswiftbus/service"
//! \endcond

namespace FGSwiftBus {

	/*!
     * FGSwiftBus service object which is accessible through DBus
     */
class CService : public CDBusObject
{
public:
    //! Constructor
    CService();

    //! DBus interface name
    static const std::string& InterfaceName();

    //! DBus object path
    static const std::string& ObjectPath();

    //! Getting flightgear version
    static int getVersionNumber();

    ////! Add a text message to the on-screen display, with RGB components in the range [0,1]
    void addTextMessage(const std::string& text);

    ////! Get full path to current aircraft model
    std::string getAircraftModelPath() const;

    ////! Get base filename of current aircraft model
    std::string getAircraftModelFilename() const;

    ////! Get canonical swift model string of current aircraft model
    std::string getAircraftModelString() const;

    ////! Get name of current aircraft model
    std::string getAircraftName() const;

    ////! Get path to current aircraft livery
    std::string getAircraftLivery() const;

    //! Get the ICAO code of the current aircraft model
    std::string getAircraftIcaoCode() const;

    ////! Get the description of the current aircraft model
    std::string getAircraftDescription() const;

    //! True if sim is paused
    bool isPaused() const;

    //! Get aircraft latitude in degrees
    double getLatitude() const;

    //! Get aircraft longitude in degrees
    double getLongitude() const;

    //! Get aircraft altitude in feet
    double getAltitudeMSL() const;

    //! Get aircraft height in feet
    double getHeightAGL() const;

    //! Get aircraft groundspeed in knots
    double getGroundSpeed() const;

    //! Get aircraft pitch in degrees above horizon
    double getPitch() const;

    //! Get aircraft roll in degrees
    double getRoll() const;

    //! Get aircraft true heading in degrees
    double getTrueHeading() const;

    //! Get whether all wheels are on the ground
    bool getAllWheelsOnGround() const;

    //! Get the current COM1 active frequency in kHz
    int getCom1Active() const;

    //! Get the current COM1 standby frequency in kHz
    int getCom1Standby() const;

    //! Get the current COM2 active frequency in kHz
    int getCom2Active() const;

    //! Get the current COM2 standby frequency in kHz
    int getCom2Standby() const;

    //! Get the current transponder code in decimal
    int getTransponderCode() const;

    //! Get the current transponder mode (depends on the aircraft, 0-2 usually mean standby, >2 active)
    int getTransponderMode() const;

    //! Get whether we are currently squawking ident
    bool getTransponderIdent() const;

    //! Get whether beacon lights are on
    bool getBeaconLightsOn() const;

    //! Get whether landing lights are on
    bool getLandingLightsOn() const;

    //! Get whether nav lights are on
    bool getNavLightsOn() const;

    //! Get whether strobe lights are on
    bool getStrobeLightsOn() const;

    //! Get whether taxi lights are on
    bool getTaxiLightsOn() const;

    //! Get pressure altitude in ft
    double getPressAlt() const;

    //! Set the current COM1 active frequency in kHz
    void setCom1Active(int freq);

    //! Set the current COM1 standby frequency in kHz
    void setCom1Standby(int freq);

    //! Set the current COM2 active frequency in kHz
    void setCom2Active(int freq);

    //! Set the current COM2 standby frequency in kHz
    void setCom2Standby(int freq);

    ////! Set the current transponder code in decimal
    void setTransponderCode(int code);

    ////! Set the current transponder mode (depends on the aircraft, 0 and 1 usually mean standby, >1 active)
    void setTransponderMode(int mode);

    //! Get flaps deploy ratio, where 0.0 is flaps fully retracted, and 1.0 is flaps fully extended.
    double getFlapsDeployRatio() const;

    //! Get gear deploy ratio, where 0 is up and 1 is down
    double getGearDeployRatio() const;

    //! Get the number of engines of current aircraft
    int getNumberOfEngines() const;

    //! Get the N1 speed as percent of max (per engine)
    std::vector<double> getEngineN1Percentage() const;

    //! Get the ratio how much the speedbrakes surfaces are extended (0.0 is fully retracted, and 1.0 is fully extended)
    double getSpeedBrakeRatio() const;

    //! Get ground elevation at aircraft current position
    double getGroundElevation() const;

    //! Perform generic processing
    int process();

protected:
    DBusHandlerResult dbusMessageHandler(const CDBusMessage& message) override;

private:
    SGPropertyNode_ptr textMessageNode;
    SGPropertyNode_ptr aircraftModelPathNode;
    //SGPropertyNode_ptr aircraftLiveryNode;
    //SGPropertyNode_ptr aircraftIcaoCodeNode;
    SGPropertyNode_ptr aircraftDescriptionNode;
    SGPropertyNode_ptr isPausedNode;
    SGPropertyNode_ptr latitudeNode;
    SGPropertyNode_ptr longitudeNode;
    SGPropertyNode_ptr altitudeMSLNode;
    SGPropertyNode_ptr heightAGLNode;
    SGPropertyNode_ptr groundSpeedNode;
    SGPropertyNode_ptr pitchNode;
    SGPropertyNode_ptr rollNode;
    SGPropertyNode_ptr trueHeadingNode;
    SGPropertyNode_ptr wheelsOnGroundNode;
    SGPropertyNode_ptr com1ActiveNode;
    SGPropertyNode_ptr com1StandbyNode;
    SGPropertyNode_ptr com2ActiveNode;
    SGPropertyNode_ptr com2StandbyNode;
    SGPropertyNode_ptr transponderCodeNode;
    SGPropertyNode_ptr transponderModeNode;
    SGPropertyNode_ptr transponderIdentNode;
    SGPropertyNode_ptr beaconLightsNode;
    SGPropertyNode_ptr landingLightsNode;
    SGPropertyNode_ptr navLightsNode;
    SGPropertyNode_ptr strobeLightsNode;
    SGPropertyNode_ptr taxiLightsNode;
    SGPropertyNode_ptr altimeterServiceableNode;
    SGPropertyNode_ptr pressAltitudeFtNode;
    SGPropertyNode_ptr flapsDeployRatioNode;
    SGPropertyNode_ptr gearDeployRatioNode;
    SGPropertyNode_ptr speedBrakeDeployRatioNode;
    SGPropertyNode_ptr groundElevation;
    //SGPropertyNode_ptr numberEnginesNode;
    //SGPropertyNode_ptr engineN1PercentageNode;
    SGPropertyNode_ptr aircraftNameNode;

};
} // namespace FGSwiftBus

#endif // guard