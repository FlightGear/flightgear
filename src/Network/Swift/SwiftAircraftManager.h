// SwiftAircraftManager.h - Manger class for aircrafts generated for swift 
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

#include "SwiftAircraft.h"
#include <vector>

#ifndef FGSWIFTAIRCRAFTMANAGER_H
#define FGSWIFTAIRCRAFTMANAGER_H

class FGSwiftAircraftManager
{
public:
    FGSwiftAircraftManager();
    ~FGSwiftAircraftManager();
    std::map<std::string, FGSwiftAircraft*> aircraftByCallsign;
    bool                          addPlane(const std::string& callsign, std::string modelString);
    void                          updatePlanes(std::vector<std::string> callsigns, std::vector<SGGeod> positions, std::vector<SGVec3d> orientations, std::vector<double> groundspeeds, std::vector<bool> onGrounds);
    void                          getRemoteAircraftData(std::vector<std::string>& callsigns, std::vector<double>& latitudesDeg, std::vector<double>& longitudesDeg,
                                                        std::vector<double>& elevationsM, std::vector<double>& verticalOffsets) const;
    void                          removePlane(const std::string& callsign);
	void removeAllPlanes();
};
#endif