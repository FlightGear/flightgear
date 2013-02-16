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

#ifndef AIObject_hxx
#define AIObject_hxx

#include <list>
#include <string>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/structure/SGWeakReferenced.hxx>
#include <simgear/timing/timestamp.hxx>

#include "AIEnvironment.hxx"
#include "AIManager.hxx"
#include "AIPhysics.hxx"

namespace fgai {

class AIBVHPager;

class AIObject : public SGWeakReferenced {
public:
    AIObject();
    virtual ~AIObject();

    // also register the required hla objects here
    virtual void init(AIManager& manager);
    virtual void update(AIManager& manager, const SGTimeStamp& simTime);
    virtual void shutdown(AIManager& manager);

    void setGroundCache(const AIPhysics& physics, AIBVHPager& pager, const SGTimeStamp& dt);
    bool getGroundIntersection(SGVec3d& point, SGVec3d& normal, const SGLineSegmentd& lineSegment) const;
    bool getGroundIntersection(SGPlaned& plane, const SGLineSegmentd& lineSegment) const;

    const SGTimeStamp& getSimTime() const
    { return _simTime; }

    const AIEnvironment& getEnvironment() const
    { return *_environment; }
    AIEnvironment& getEnvironment()
    { return *_environment; }

    const AISubsystemGroup& getSubsystemGroup() const
    { return *_subsystemGroup; }
    AISubsystemGroup& getSubsystemGroup()
    { return *_subsystemGroup; }

    const AIPhysics& getPhysics() const
    { return *_physics; }
    AIPhysics& getPhysics()
    { return *_physics; }

protected:
    void setPhysics(AIPhysics* physics)
    { _physics = physics; }

private:
    friend class AIManager;

    // The simulation time
    SGTimeStamp _simTime;

    // The iterator to our own list entry in the manager class
    AIManager::ObjectList::iterator _objectListIterator;

    // The components we have for an ai object
    SGSharedPtr<AIEnvironment> _environment;
    SGSharedPtr<AISubsystemGroup> _subsystemGroup;
    SGSharedPtr<AIPhysics> _physics;

    /// The equivalent of the ground cache for FGInterface.
    SGSharedPtr<simgear::BVHNode> _node;
    /// The sphere we really queried for the ground cache
    SGSphered _querySphere;
};

} // namespace fgai

#endif
