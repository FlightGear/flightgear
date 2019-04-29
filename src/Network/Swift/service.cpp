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
#include <algorithm>
#include <iostream>
#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/props/props.hxx>
#include <simgear/structure/commands.hxx>
#include <simgear/structure/event_mgr.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/timing/timestamp.hxx>

namespace FGSwiftBus {

CService::CService()
{
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

void CService::addTextMessage(const std::string& text)
{
    if (text.empty()) { return; }
    fgSetString("/sim/messages/copilot", text);
}

std::string CService::getAircraftModelPath() const 
{ 
	return fgGetString("/sim/aircraft-dir"); 
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
	return fgGetString("/sim/description"); 
}

bool CService::isPaused() const 
{ 
	return fgGetBool("/sim/freeze/master"); 
}

double CService::getLatitude() const 
{ 
	return fgGetDouble("/position/latitude-deg"); 
}

double CService::getLongitude() const 
{ 
	return fgGetDouble("/position/longitude-deg"); 
}

double CService::getAltitudeMSL() const 
{ 
	return fgGetDouble("/position/altitude-ft"); 
}

double CService::getHeightAGL() const 
{ 
	return fgGetDouble("/position/altitude-agl-ft");
}

double CService::getGroundSpeed() const 
{ 
	return fgGetDouble("/velocities/groundspeed-kt"); 
}

double CService::getPitch() const 
{ 
	return fgGetDouble("/orientation/pitch-deg"); 
}

double CService::getRoll() const 
{ 
	return fgGetDouble("/orientation/roll-deg"); 
}

double CService::getTrueHeading() const 
{ 
	return fgGetDouble("/orientation/heading-deg"); 
}

bool CService::getAllWheelsOnGround() const 
{ 
	return fgGetBool("/gear/gear/wow"); 
}

int CService::getCom1Active() const 
{ 
	return fgGetDouble("/instrumentation/comm/frequencies/selected-mhz") * 1000; 
}

int CService::getCom1Standby() const 
{ 
	return fgGetDouble("/instrumentation/comm/frequencies/standby-mhz") * 1000; 
}

int CService::getCom2Active() const 
{ 
	return fgGetDouble("/instrumentation/comm[1]/frequencies/selected-mhz") * 1000; 
}

int CService::getCom2Standby() const 
{ 
	return fgGetDouble("/instrumentation/comm[1]/frequencies/standby-mhz") * 1000; 
}

int CService::getTransponderCode() const 
{ 
	return fgGetInt("/instrumentation/transponder/id-code"); 
}

int CService::getTransponderMode() const 
{ 
	return fgGetInt("/instrumentation/transponder/inputs/knob-mode"); 
}

bool CService::getTransponderIdent() const 
{ 
	return fgGetBool("/instrumentation/transponder/ident"); 
}

bool CService::getBeaconLightsOn() const 
{ 
	return fgGetBool("/controls/lighting/beacon"); 
}

bool CService::getLandingLightsOn() const 
{ 
	return fgGetBool("/controls/lighting/landing-lights"); 
}

bool CService::getNavLightsOn() const 
{ 
	return fgGetBool("/controls/lighting/nav-lights"); 
}


bool CService::getStrobeLightsOn() const 
{ 
	return fgGetBool("/controls/lighting/strobe"); 
}

bool CService::getTaxiLightsOn() const 
{ 
	return fgGetBool("/controls/lighting/taxi-light"); 
}

double CService::getPressAlt() const 
{ 
	if (fgGetBool("/instrumentation/altimeter/serviceable")){
		return fgGetDouble("/instrumentation/altimeter/pressure-alt-ft");
	} else {
		return fgGetDouble("/position/altitude-ft");
	}
}

void CService::setCom1Active(int freq) 
{ 
	fgSetDouble("/instrumentation/comm/frequencies/selected-mhz", freq / (double)1000); 
}

void CService::setCom1Standby(int freq) 
{ 
	fgSetDouble("/instrumentation/comm/frequencies/standby-mhz", freq / (double)1000); 
}

void CService::setCom2Active(int freq) 
{
	fgSetDouble("/instrumentation/comm[1]/frequencies/selected-mhz", freq / (double)1000); 
}

void CService::setCom2Standby(int freq) 
{
	fgSetDouble("/instrumentation/comm[1]/frequencies/standby-mhz", freq / (double)1000);
}

void CService::setTransponderCode(int code)
{ 
	fgSetInt("/instrumentation/transponder/id-code", code);
}

void CService::setTransponderMode(int mode) 
{ 
	fgSetInt("/instrumentation/transponder/inputs/knob-mode", mode);
}

double CService::getFlapsDeployRatio() const 
{ 
	return fgGetFloat("/surface-positions/flap-pos-norm");
}

double CService::getGearDeployRatio() const 
{ 
	return fgGetFloat("/gear/gear/position-norm"); 
}

int CService::getNumberOfEngines() const 
{ 
	return 2; 
}

std::vector<double> CService::getEngineN1Percentage() const
{
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
	return fgGetFloat("/surface-positions/speedbrake-pos-norm"); 
}

std::string CService::getAircraftModelFilename() const
{
    std::string modelFileName = fgGetString("/sim/aircraft");
    modelFileName.append("-set.xml");
    return modelFileName;
}

std::string CService::getAircraftModelString() const
{
    std::string modelName   = fgGetString("/sim/aircraft");
    std::string modelString = "FG " + modelName;
    return modelString;
}

std::string CService::getAircraftName() const
{
    return fgGetString("/sim/aircraft");
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