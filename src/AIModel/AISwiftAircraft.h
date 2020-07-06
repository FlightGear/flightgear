// AISwiftAircraft.h - Derived AIBase class for swift aircraft
//
// Copyright (C) 2020 - swift Project Community / Contributors (http://swift-project.org/)
// Written by Lars Toenning <dev@ltoenning.de> started on April 2020.
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

#ifndef FLIGHTGEAR_AISWIFTAIRCRAFT_H
#define FLIGHTGEAR_AISWIFTAIRCRAFT_H


#include "AIBase.hxx"
#include <string>

using charPtr = const char*;


class FGAISwiftAircraft : public FGAIBase
{
public:
    FGAISwiftAircraft(const std::string& callsign, const std::string& modelString);
    ~FGAISwiftAircraft() override;
    void updatePosition(SGGeod& position, SGVec3<double>& orientation, double groundspeed, bool initPos);
    void update(double dt) override;
    double getGroundElevation(const SGGeod& pos) const;
    void initProps();
    void setPlaneSurface(double gear, double flaps, double spoilers, double speedBrake, double slats,
                         double wingSweeps, double thrust, double elevator, double rudder, double aileron,
                         bool landingLight, bool taxiLight, bool beaconLight, bool strobeLight, bool navLight,
                         int lightPattern);
    void setPlaneTransponder(int code, bool modeC, bool ident);

    const char* getTypeString() const override { return "swift"; }

private:
    bool m_initPos = false;
    // Property Nodes for transponder and parts
    SGPropertyNode_ptr m_transponderCodeNode;
    SGPropertyNode_ptr m_transponderCModeNode;
    SGPropertyNode_ptr m_transponderIdentNode;

    SGPropertyNode_ptr m_gearNode;
    SGPropertyNode_ptr m_flapsIdentNode;
    SGPropertyNode_ptr m_spoilerNode;
    SGPropertyNode_ptr m_speedBrakeNode;
    //SGPropertyNode_ptr m_slatsNode;
    //SGPropertyNode_ptr m_wingSweepNode;
    //SGPropertyNode_ptr m_thrustNode;
    //SGPropertyNode_ptr m_elevatorNode;
    //SGPropertyNode_ptr m_rudderNode;
    //SGPropertyNode_ptr m_aileronNode;
    SGPropertyNode_ptr m_landLightNode;
    SGPropertyNode_ptr m_taxiLightNode;
    SGPropertyNode_ptr m_beaconLightNode;
    SGPropertyNode_ptr m_strobeLightNode;
    SGPropertyNode_ptr m_navLightNode;
    //SGPropertyNode_ptr m_lightPatternNode;

};


#endif //FLIGHTGEAR_AISWIFTAIRCRAFT_H
