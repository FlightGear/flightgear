// AISwiftAircraft.h - Derived AIBase class for swift aircrafts
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

    const char* getTypeString() const override { return "swift"; }

private:
    bool m_initPos = false;
};


#endif //FLIGHTGEAR_AISWIFTAIRCRAFT_H
