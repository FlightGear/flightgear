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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "AIPhysics.hxx"

#include <simgear/math/SGGeometry.hxx>
#include "AIEnvironment.hxx"

namespace fgai {

AIPhysics::AIPhysics(const AIPhysics& physics) :
    _location(physics._location),
    _linearBodyVelocity(physics._linearBodyVelocity),
    _angularBodyVelocity(physics._angularBodyVelocity),
    _geodPosition(physics._geodPosition),
    _horizontalLocalOrientation(physics._horizontalLocalOrientation)
{
}

AIPhysics::AIPhysics(const SGLocationd& location, const SGVec3d& linearBodyVelocity,
                           const SGVec3d& angularBodyVelocity) :
    _location(location),
    _linearBodyVelocity(linearBodyVelocity),
    _angularBodyVelocity(angularBodyVelocity)
{
    _geodPosition = SGGeod::fromCart(_location.getPosition());
    _horizontalLocalOrientation = SGQuatd::fromLonLat(_geodPosition);
}

AIPhysics::~AIPhysics()
{
}

void
AIPhysics::update(AIObject& object, const SGTimeStamp& dt)
{
    advanceByBodyVelocity(dt.toSecs(), _linearBodyVelocity, _angularBodyVelocity);
}

void
AIPhysics::advanceByBodyAcceleration(const double& dt,
                                        const SGVec3d& linearAcceleration,
                                        const SGVec3d& angularAcceleration)
{
    // The current linear and angular velocity
    SGVec3d linearVelocity = getLinearBodyVelocity();
    SGVec3d angularVelocity = getAngularBodyVelocity();
    
    // an euler step for the velocities, the positions get upgraded below
    linearVelocity += dt*linearAcceleration;
    angularVelocity += dt*angularAcceleration;
    
    advanceByBodyVelocity(dt, linearVelocity, angularVelocity);
}

void
AIPhysics::advanceByBodyVelocity(const double& dt,
                                    const SGVec3d& linearVelocity,
                                    const SGVec3d& angularVelocity)
{
    // Do an euler step with the derivatives at mSimTime
    _location.eulerStepBodyVelocities(dt, _linearBodyVelocity, _angularBodyVelocity);
    _geodPosition = SGGeod::fromCart(_location.getPosition());
    _horizontalLocalOrientation = SGQuatd::fromLonLat(_geodPosition);
    
    // Store the new velocities for the next interval at mSimTim + dt
    _linearBodyVelocity = linearVelocity;
    _angularBodyVelocity = angularVelocity;
}

void
AIPhysics::advanceToLocation(const double& dt, const SGLocationd& location)
{
    // At first we need to move along with the announced velocities, so:
    // Do an euler step with the derivatives at mSimTime.
    _location.eulerStepBodyVelocities(dt, _linearBodyVelocity, _angularBodyVelocity);
    _geodPosition = SGGeod::fromCart(_location.getPosition());
    _horizontalLocalOrientation = SGQuatd::fromLonLat(_geodPosition);
    
    // Now compute velocities that will move to the desired position, orientation if the next
    // advance method is called with the same dt value
    SGVec3d positionDifference = location.getPosition() - _location.getPosition();
    _linearBodyVelocity = (1/dt)*_location.getOrientation().transform(positionDifference);
    _angularBodyVelocity = SGQuatd::forwardDifferenceVelocity(_location.getOrientation(), location.getOrientation(), dt);
}

} // namespace fgai
