// service.cpp - Service module for swift<->FG connection 
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

#include "service.h"
#include <Main/fg_props.hxx>
#include <iostream>
#include <simgear/debug/logstream.hxx>
#include <simgear/structure/commands.hxx>

#define FGSWIFTBUS_API_VERSION 2;

namespace FGSwiftBus {

CService::CService()
{
    // Initialize node pointers
    m_textMessageNode           = fgGetNode("/sim/messages/copilot", true);
    m_aircraftModelPathNode     = fgGetNode("/sim/aircraft-dir", true);
    m_aircraftDescriptionNode   = fgGetNode("/sim/description", true);
    m_isPausedNode              = fgGetNode("/sim/freeze/master", true);
    m_latitudeNode              = fgGetNode("/position/latitude-deg", true);
    m_longitudeNode             = fgGetNode("/position/longitude-deg", true);
    m_altitudeMSLNode           = fgGetNode("/position/altitude-ft", true);
    m_heightAGLNode             = fgGetNode("/position/altitude-agl-ft", true);
    m_groundSpeedNode           = fgGetNode("/velocities/groundspeed-kt", true);
    m_pitchNode                 = fgGetNode("/orientation/pitch-deg", true);
    m_rollNode                  = fgGetNode("/orientation/roll-deg", true);
    m_trueHeadingNode           = fgGetNode("/orientation/heading-deg", true);
    m_wheelsOnGroundNode        = fgGetNode("/gear/gear/wow", true);
    m_com1ActiveNode            = fgGetNode("/instrumentation/comm/frequencies/selected-mhz", true);
    m_com1StandbyNode           = fgGetNode("/instrumentation/comm/frequencies/standby-mhz", true);
    m_com2ActiveNode            = fgGetNode("/instrumentation/comm[1]/frequencies/selected-mhz", true);
    m_com2StandbyNode           = fgGetNode("/instrumentation/comm[1]/frequencies/standby-mhz", true);
    m_transponderCodeNode       = fgGetNode("/instrumentation/transponder/id-code", true);
    m_transponderModeNode       = fgGetNode("/instrumentation/transponder/inputs/knob-mode", true);
    m_transponderIdentNode      = fgGetNode("/instrumentation/transponder/ident", true);
    m_beaconLightsNode          = fgGetNode("/controls/lighting/beacon", true);
    m_landingLightsNode         = fgGetNode("/controls/lighting/landing-lights", true);
    m_navLightsNode             = fgGetNode("/controls/lighting/nav-lights", true);
    m_strobeLightsNode          = fgGetNode("/controls/lighting/strobe", true);
    m_taxiLightsNode            = fgGetNode("/controls/lighting/taxi-light", true);
    m_altimeterServiceableNode  = fgGetNode("/instrumentation/altimeter/serviceable", true);
    m_pressAltitudeFtNode       = fgGetNode("/instrumentation/altimeter/pressure-alt-ft", true);
    m_flapsDeployRatioNode      = fgGetNode("/surface-positions/flap-pos-norm", true);
    m_gearDeployRatioNode       = fgGetNode("/gear/gear/position-norm", true);
    m_speedBrakeDeployRatioNode = fgGetNode("/surface-positions/speedbrake-pos-norm", true);
    m_aircraftNameNode        = fgGetNode("/sim/aircraft", true);
    m_groundElevationNode     = fgGetNode("/position/ground-elev-m", true);

    SG_LOG(SG_NETWORK, SG_INFO, "FGSwiftBus Service initialized");
}

const std::string& CService::InterfaceName() {
    static const std::string s(FGSWIFTBUS_SERVICE_INTERFACENAME);
    return s;
}

const std::string& CService::ObjectPath()
{
    static const std::string s(FGSWIFTBUS_SERVICE_OBJECTPATH);
    return s;
}

// Static method
int CService::getVersionNumber()
{
    return FGSWIFTBUS_API_VERSION;
}

void CService::addTextMessage(const std::string& text)
{
    if (text.empty()) { return; }
    m_textMessageNode->setStringValue(text);
}

std::string CService::getAircraftModelPath() const 
{ 
	return m_aircraftModelPathNode->getStringValue();
}

std::string CService::getAircraftLivery() const 
{ 
	return ""; 
}

std::string CService::getAircraftIcaoCode() const 
{ 
	return ""; 
}

std::string CService::getAircraftDescription() const 
{ 
	return m_aircraftDescriptionNode->getStringValue();
}

bool CService::isPaused() const 
{ 
	return m_isPausedNode->getBoolValue();
}

double CService::getLatitude() const 
{ 
	return m_latitudeNode->getDoubleValue();
}

double CService::getLongitude() const 
{ 
	return m_longitudeNode->getDoubleValue();
}

double CService::getAltitudeMSL() const 
{ 
	return m_altitudeMSLNode->getDoubleValue();
}

double CService::getHeightAGL() const 
{ 
	return m_heightAGLNode->getDoubleValue();
}

double CService::getGroundSpeed() const 
{ 
	return m_groundSpeedNode->getDoubleValue();
}

double CService::getPitch() const 
{ 
	return m_pitchNode->getDoubleValue();
}

double CService::getRoll() const 
{ 
	return m_rollNode->getDoubleValue();
}

double CService::getTrueHeading() const 
{ 
	return m_trueHeadingNode->getDoubleValue();
}

bool CService::getAllWheelsOnGround() const 
{ 
	return m_wheelsOnGroundNode->getBoolValue();
}

int CService::getCom1Active() const 
{
	return (int)(m_com1ActiveNode->getDoubleValue() * 1000);
}

int CService::getCom1Standby() const 
{ 
	return (int)(m_com1StandbyNode->getDoubleValue() * 1000);
}

int CService::getCom2Active() const 
{ 
	return (int)(m_com2ActiveNode->getDoubleValue() * 1000);
}

int CService::getCom2Standby() const 
{ 
	return (int)(m_com2StandbyNode->getDoubleValue() * 1000);
}

int CService::getTransponderCode() const 
{ 
	return m_transponderCodeNode->getIntValue();
}

int CService::getTransponderMode() const 
{ 
	return m_transponderModeNode->getIntValue();
}

bool CService::getTransponderIdent() const 
{ 
	return m_transponderIdentNode->getBoolValue();
}

bool CService::getBeaconLightsOn() const 
{ 
	return m_beaconLightsNode->getBoolValue();
}

bool CService::getLandingLightsOn() const 
{ 
	return m_landingLightsNode->getBoolValue();
}

bool CService::getNavLightsOn() const 
{ 
	return m_navLightsNode->getBoolValue();
}


bool CService::getStrobeLightsOn() const 
{ 
	return m_strobeLightsNode->getBoolValue();
}

bool CService::getTaxiLightsOn() const 
{ 
	return m_taxiLightsNode->getBoolValue();
}

double CService::getPressAlt() const 
{ 
	if (m_altimeterServiceableNode->getBoolValue()){
		return m_pressAltitudeFtNode->getDoubleValue();
	} else {
		return m_altitudeMSLNode->getDoubleValue();
	}
}

void CService::setCom1Active(int freq) 
{
    m_com1ActiveNode->setDoubleValue(freq /(double)1000);
}

void CService::setCom1Standby(int freq) 
{
    m_com1StandbyNode->setDoubleValue(freq /(double)1000);
}

void CService::setCom2Active(int freq) 
{
    m_com2ActiveNode->setDoubleValue(freq /(double)1000);
}

void CService::setCom2Standby(int freq) 
{
    m_com2StandbyNode->setDoubleValue(freq /(double)1000);
}

void CService::setTransponderCode(int code)
{
    m_transponderCodeNode->setIntValue(code);
}

void CService::setTransponderMode(int mode) 
{
    m_transponderModeNode->setIntValue(mode);
}

double CService::getFlapsDeployRatio() const 
{ 
	return m_flapsDeployRatioNode->getFloatValue();
}

double CService::getGearDeployRatio() const 
{ 
	return m_gearDeployRatioNode->getFloatValue();
}

int CService::getNumberOfEngines() const 
{
    // TODO Use correct property
	return 2; 
}

std::vector<double> CService::getEngineN1Percentage() const
{
    // TODO use correct engine numbers
    std::vector<double> list;
    const auto          number = static_cast<unsigned int>(getNumberOfEngines());
    list.reserve(number);
    for (unsigned int engineNumber = 0; engineNumber < number; ++engineNumber) {
        list.push_back(fgGetDouble("/engine/engine/n1"));
    }
    return list;
}

double CService::getSpeedBrakeRatio() const 
{ 
	return m_speedBrakeDeployRatioNode->getFloatValue();
}

double CService::getGroundElevation() const
{
    return m_groundElevationNode->getDoubleValue();
}

std::string CService::getAircraftModelFilename() const
{
    std::string modelFileName = getAircraftName();
    modelFileName.append("-set.xml");
    return modelFileName;
}

std::string CService::getAircraftModelString() const
{
    std::string modelName   = getAircraftName();
    std::string modelString = "FG " + modelName;
    return modelString;
}

std::string CService::getAircraftName() const
{
    return m_aircraftNameNode->getStringValue();
}

static const char* introspection_service = DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE;

DBusHandlerResult CService::dbusMessageHandler(const CDBusMessage& message_)
{
    CDBusMessage        message(message_);
    const std::string   sender     = message.getSender();
    const dbus_uint32_t serial     = message.getSerial();
    const bool          wantsReply = message.wantsReply();

    if (message.getInterfaceName() == DBUS_INTERFACE_INTROSPECTABLE) {
        if (message.getMethodName() == "Introspect") {
            sendDBusReply(sender, serial, introspection_service);
        }
    } else if (message.getInterfaceName() == FGSWIFTBUS_SERVICE_INTERFACENAME) {
        if (message.getMethodName() == "addTextMessage") {
            maybeSendEmptyDBusReply(wantsReply, sender, serial);
            std::string text;
            message.beginArgumentRead();
            message.getArgument(text);

            queueDBusCall([=]() {
                addTextMessage(text);
            });
        } else if (message.getMethodName() == "getOwnAircraftSituationData") {
            queueDBusCall([=]() {
                double       lat         = getLatitude();
                double       lon         = getLongitude();
                double       alt         = getAltitudeMSL();
                double       gs          = getGroundSpeed();
                double       pitch       = getPitch();
                double       roll        = getRoll();
                double       trueHeading = getTrueHeading();
                double       pressAlt    = getPressAlt();
                CDBusMessage reply       = CDBusMessage::createReply(sender, serial);
                reply.beginArgumentWrite();
                reply.appendArgument(lat);
                reply.appendArgument(lon);
                reply.appendArgument(alt);
                reply.appendArgument(gs);
                reply.appendArgument(pitch);
                reply.appendArgument(roll);
                reply.appendArgument(trueHeading);
                reply.appendArgument(pressAlt);
                sendDBusMessage(reply);
            });
        } else if (message.getMethodName() == "getVersionNumber") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getVersionNumber());
            });
        } else if (message.getMethodName() == "getAircraftModelPath") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getAircraftModelPath());
            });
        } else if (message.getMethodName() == "getAircraftModelFilename") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getAircraftModelFilename());
            });
        } else if (message.getMethodName() == "getAircraftModelString") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getAircraftModelString());
            });
        } else if (message.getMethodName() == "getAircraftName") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getAircraftName());
            });
        } else if (message.getMethodName() == "getAircraftLivery") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getAircraftLivery());
            });
        } else if (message.getMethodName() == "getAircraftIcaoCode") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getAircraftIcaoCode());
            });
        } else if (message.getMethodName() == "getAircraftDescription") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getAircraftDescription());
            });
        } else if (message.getMethodName() == "isPaused") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, isPaused());
            });
        } else if (message.getMethodName() == "getLatitudeDeg") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getLatitude());
            });
        } else if (message.getMethodName() == "getLongitudeDeg") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getLongitude());
            });
        } else if (message.getMethodName() == "getAltitudeMslFt") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getAltitudeMSL());
            });
        } else if (message.getMethodName() == "getHeightAglFt") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getHeightAGL());
            });
        } else if (message.getMethodName() == "getGroundSpeedKts") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getGroundSpeed());
            });
        } else if (message.getMethodName() == "getPitchDeg") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getPitch());
            });
        } else if (message.getMethodName() == "getRollDeg") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getRoll());
            });
        } else if (message.getMethodName() == "getAllWheelsOnGround") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getAllWheelsOnGround());
            });
        } else if (message.getMethodName() == "getCom1ActiveKhz") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getCom1Active());
            });
        } else if (message.getMethodName() == "getCom1StandbyKhz") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getCom1Standby());
            });
        } else if (message.getMethodName() == "getCom2ActiveKhz") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getCom2Active());
            });
        } else if (message.getMethodName() == "getCom2StandbyKhz") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getCom2Standby());
            });
        } else if (message.getMethodName() == "getTransponderCode") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getTransponderCode());
            });
        } else if (message.getMethodName() == "getTransponderMode") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getTransponderMode());
            });
        } else if (message.getMethodName() == "getTransponderIdent") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getTransponderIdent());
            });
        } else if (message.getMethodName() == "getBeaconLightsOn") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getBeaconLightsOn());
            });
        } else if (message.getMethodName() == "getLandingLightsOn") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getLandingLightsOn());
            });
        } else if (message.getMethodName() == "getNavLightsOn") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getNavLightsOn());
            });
        } else if (message.getMethodName() == "getStrobeLightsOn") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getStrobeLightsOn());
            });
        } else if (message.getMethodName() == "getTaxiLightsOn") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getTaxiLightsOn());
            });
        } else if (message.getMethodName() == "getPressAlt") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getPressAlt());
            });
        } else if (message.getMethodName() == "getGroundElevation") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getGroundElevation());
            });
        } else if (message.getMethodName() == "setCom1ActiveKhz") {
            maybeSendEmptyDBusReply(wantsReply, sender, serial);
            int frequency = 0;
            message.beginArgumentRead();
            message.getArgument(frequency);
            queueDBusCall([=]() {
                setCom1Active(frequency);
            });
        } else if (message.getMethodName() == "setCom1StandbyKhz") {
            maybeSendEmptyDBusReply(wantsReply, sender, serial);
            int frequency = 0;
            message.beginArgumentRead();
            message.getArgument(frequency);
            queueDBusCall([=]() {
                setCom1Standby(frequency);
            });
        } else if (message.getMethodName() == "setCom2ActiveKhz") {
            maybeSendEmptyDBusReply(wantsReply, sender, serial);
            int frequency = 0;
            message.beginArgumentRead();
            message.getArgument(frequency);
            queueDBusCall([=]() {
                setCom2Active(frequency);
            });
        } else if (message.getMethodName() == "setCom2StandbyKhz") {
            maybeSendEmptyDBusReply(wantsReply, sender, serial);
            int frequency = 0;
            message.beginArgumentRead();
            message.getArgument(frequency);
            queueDBusCall([=]() {
                setCom2Standby(frequency);
            });
        } else if (message.getMethodName() == "setTransponderCode") {
            maybeSendEmptyDBusReply(wantsReply, sender, serial);
            int code = 0;
            message.beginArgumentRead();
            message.getArgument(code);
            queueDBusCall([=]() {
                setTransponderCode(code);
            });
        } else if (message.getMethodName() == "setTransponderMode") {
            maybeSendEmptyDBusReply(wantsReply, sender, serial);
            int mode = 0;
            message.beginArgumentRead();
            message.getArgument(mode);
            queueDBusCall([=]() {
                setTransponderMode(mode);
            });
        } else if (message.getMethodName() == "getFlapsDeployRatio") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getFlapsDeployRatio());
            });
        } else if (message.getMethodName() == "getGearDeployRatio") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getGearDeployRatio());
            });
        } else if (message.getMethodName() == "getEngineN1Percentage") {
            queueDBusCall([=]() {
                std::vector<double> array = getEngineN1Percentage();
                sendDBusReply(sender, serial, array);
            });
        } else if (message.getMethodName() == "getSpeedBrakeRatio") {
            queueDBusCall([=]() {
                sendDBusReply(sender, serial, getSpeedBrakeRatio());
            });
        } else {
            // Unknown message. Tell DBus that we cannot handle it
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
    }
    return DBUS_HANDLER_RESULT_HANDLED;
}


int CService::process()
{
    invokeQueuedDBusCalls();

    return 1;
}

} // namespace FGSwiftBus