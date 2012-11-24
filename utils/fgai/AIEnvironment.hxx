// Copyright (C) 2009 - 2012  Mathias Froehlich
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

#ifndef AIEnvironment_hxx
#define AIEnvironment_hxx

#include <simgear/bvh/BVHNode.hxx>
#include <simgear/math/SGGeometry.hxx>
#include "AISubsystem.hxx"

namespace fgai {

class AIPhysics;
class AIBVHPager;

class AIEnvironment : public AISubsystem {
public:
    virtual ~AIEnvironment();

    virtual void update(AIObject& object, const SGTimeStamp& dt);

    // Get these at some point from a weather module and an apropriate
    // hla attribute
    double getDensity() const
    { return 1.29; }
    double getTemperature() const
    { return 15 + 273.15; }
    /// The wind speed in cartesian coorindates in the earth centered frame
    SGVec3d getWindVelocity() const
    { return SGVec3d::zeros(); }
};

} // namespace fgai

#endif
