/*
 * Service module for swift<->FG connection
 * SPDX-FileCopyrightText: (C) 2019-2022 swift Project Community / Contributors (https://swift-project.org/)
 * SPDX-FileCopyrightText: (C) 2019-2022 Lars Toenning <dev@ltoenning.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once


#include "dbusobject.h"
#include <Main/fg_props.hxx>
#include <chrono>
#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/timing/timestamp.hxx>
#include <string>


namespace flightgear::swift {

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

    //! Get x velocity in m/s
    double getVelocityX() const;

    //! Get y velocity in m/s
    double getVelocityY() const;

    //! Get z velocity in m/s
    double getVelocityZ() const;

    //! Get roll rate in rad/sec
    double getRollRate() const;

    //! Get pitch rate in rad/sec
    double getPitchRate() const;

    //! Get yaw rate in rad/sec
    double getYawRate() const;

    double getCom1Volume() const;
    double getCom2Volume() const;

    //! Perform generic processing
    int process();

protected:
    DBusHandlerResult dbusMessageHandler(const CDBusMessage& message) override;

private:
    SGPropertyNode_ptr m_textMessageNode;
    SGPropertyNode_ptr m_aircraftModelPathNode;
    //SGPropertyNode_ptr aircraftLiveryNode;
    //SGPropertyNode_ptr aircraftIcaoCodeNode;
    SGPropertyNode_ptr m_aircraftDescriptionNode;
    SGPropertyNode_ptr m_isPausedNode;
    SGPropertyNode_ptr m_latitudeNode;
    SGPropertyNode_ptr m_longitudeNode;
    SGPropertyNode_ptr m_altitudeMSLNode;
    SGPropertyNode_ptr m_heightAGLNode;
    SGPropertyNode_ptr m_groundSpeedNode;
    SGPropertyNode_ptr m_pitchNode;
    SGPropertyNode_ptr m_rollNode;
    SGPropertyNode_ptr m_trueHeadingNode;
    SGPropertyNode_ptr m_wheelsOnGroundNode;
    SGPropertyNode_ptr m_com1ActiveNode;
    SGPropertyNode_ptr m_com1StandbyNode;
    SGPropertyNode_ptr m_com2ActiveNode;
    SGPropertyNode_ptr m_com2StandbyNode;
    SGPropertyNode_ptr m_transponderCodeNode;
    SGPropertyNode_ptr m_transponderModeNode;
    SGPropertyNode_ptr m_transponderIdentNode;
    SGPropertyNode_ptr m_beaconLightsNode;
    SGPropertyNode_ptr m_landingLightsNode;
    SGPropertyNode_ptr m_navLightsNode;
    SGPropertyNode_ptr m_strobeLightsNode;
    SGPropertyNode_ptr m_taxiLightsNode;
    SGPropertyNode_ptr m_altimeterServiceableNode;
    SGPropertyNode_ptr m_pressAltitudeFtNode;
    SGPropertyNode_ptr m_flapsDeployRatioNode;
    SGPropertyNode_ptr m_gearDeployRatioNode;
    SGPropertyNode_ptr m_speedBrakeDeployRatioNode;
    SGPropertyNode_ptr m_groundElevationNode;
    //SGPropertyNode_ptr numberEnginesNode;
    //SGPropertyNode_ptr engineN1PercentageNode;
    SGPropertyNode_ptr m_aircraftNameNode;
    SGPropertyNode_ptr m_velocityXNode;
    SGPropertyNode_ptr m_velocityYNode;
    SGPropertyNode_ptr m_velocityZNode;
    SGPropertyNode_ptr m_rollRateNode;
    SGPropertyNode_ptr m_pichRateNode;
    SGPropertyNode_ptr m_yawRateNode;
    SGPropertyNode_ptr m_com1VolumeNode;
    SGPropertyNode_ptr m_com2VolumeNode;
};
} // namespace flightgear::swift
