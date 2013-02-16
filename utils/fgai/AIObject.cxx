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

#include "AIObject.hxx"

#include <simgear/bvh/BVHLineSegmentVisitor.hxx>
#include <simgear/bvh/BVHNode.hxx>
#include <simgear/math/SGGeometry.hxx>

#include "AIManager.hxx"

namespace fgai {

AIObject::AIObject() :
    _environment(new AIEnvironment),
    _subsystemGroup(new AISubsystemGroup)
{
}

AIObject::~AIObject()
{
}

void
AIObject::init(AIManager& manager)
{
    _simTime = manager.getSimTime();
}

void
AIObject::update(AIManager& manager, const SGTimeStamp& simTime)
{
    _simTime = simTime;
}

void
AIObject::shutdown(AIManager& manager)
{
    _simTime = SGTimeStamp();
}

void
AIObject::setGroundCache(const AIPhysics& physics, AIBVHPager& pager, const SGTimeStamp& dt)
{
    SGVec3d point = physics.getLocation().getPosition();
    double linearVelocity = norm(physics.getLinearBodyVelocity());
    // The 2 is a security factor for accelerations, but at least 100 meters
    double radius = std::max(100.0, 2*dt.toSecs()*linearVelocity);
    SGSphered requiredSphere(point, radius);
    /// Are we already good enough?
    if (requiredSphere.inside(_querySphere))
        return;
    // Now query something somehow bigger to avoid querying again in the next frame
    SGSphered sphere(point, 4*radius);
    _node = pager.getBoundingVolumes(sphere);
    if (!_node.valid())
        return;
    _querySphere = sphere;
}

bool
AIObject::getGroundIntersection(SGVec3d& point, SGVec3d& normal, const SGLineSegmentd& lineSegment) const
{
    if (!_node.valid())
        return false;
    simgear::BVHLineSegmentVisitor lineSegmentVisitor(lineSegment);
    _node->accept(lineSegmentVisitor);
    if (lineSegmentVisitor.empty())
        return false;
    normal = lineSegmentVisitor.getNormal();
    point = lineSegmentVisitor.getPoint();
    return true;
}

bool
AIObject::getGroundIntersection(SGPlaned& plane, const SGLineSegmentd& lineSegment) const
{
    SGVec3d point;
    SGVec3d normal;
    if (!getGroundIntersection(point, normal, lineSegment))
        return false;
    plane = SGPlaned(normal, point);
    return true;
}

} // namespace fgai
