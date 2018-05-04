// airportdynamicsmanager.hxx - manager for dynamic (changeable)
// part of airport state
//
// Written by James Turner, started December 2015
//
// Copyright (C) 2015 James Turner <zakalawe@mac.com>
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

#ifndef AIRPORTDYNAMICSMANAGER_H
#define AIRPORTDYNAMICSMANAGER_H

#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include <map>

#include "airports_fwd.hxx"

namespace flightgear
{

class AirportDynamicsManager : public SGSubsystem
{
public:
    AirportDynamicsManager();
    virtual ~AirportDynamicsManager();

    // Subsystem API.
    void init() override;
    void reinit() override;
    void shutdown() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "airport-dynamics"; }

    static FGAirportDynamicsRef find(const std::string& icao);

    static FGAirportDynamicsRef find(const FGAirportRef& apt);

    FGAirportDynamicsRef dynamicsForICAO(const std::string& icao);

private:
    typedef std::map<std::string, FGAirportDynamicsRef> ICAODynamicsDict;
    ICAODynamicsDict m_dynamics;
};

} // of namespace

#endif // AIRPORTDYNAMICSMANAGER_H
