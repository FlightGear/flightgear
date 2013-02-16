// Copyright (C) 2009 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "HLASceneObject.hxx"

#include <simgear/hla/HLAArrayDataElement.hxx>
#include "HLASceneObjectClass.hxx"
#include "AIPhysics.hxx"

namespace fgai {

HLASceneObject::HLASceneObject(HLASceneObjectClass* objectClass) :
    HLAObjectInstance(objectClass)
{
}

HLASceneObject::~HLASceneObject()
{
}

void
HLASceneObject::createAttributeDataElements()
{
    /// FIXME at some point we should not need that anymore
    HLAObjectInstance::createAttributeDataElements();

    assert(dynamic_cast<HLASceneObjectClass*>(getObjectClass().get()));
    HLASceneObjectClass& objectClass = static_cast<HLASceneObjectClass&>(*getObjectClass());

    simgear::HLACartesianLocation* location = new simgear::HLACartesianLocation;
    setAttributeDataElement(objectClass.getPositionIndex(), location->getPositionDataElement());
    setAttributeDataElement(objectClass.getOrientationIndex(), location->getOrientationDataElement());
    setAttributeDataElement(objectClass.getAngularVelocityIndex(), location->getAngularVelocityDataElement());
    setAttributeDataElement(objectClass.getLinearVelocityIndex(), location->getLinearVelocityDataElement());
    _location = location;
}

void
HLASceneObject::setLocation(const AIPhysics& physics)
{
    if (!_location.valid())
        return;
    /// In the long term do not just copy, but also decide how long to skip updates to the hla attributes ...
    _location->setLocation(physics.getLocation());
    _location->setAngularBodyVelocity(physics.getAngularBodyVelocity());
    _location->setLinearBodyVelocity(physics.getLinearBodyVelocity());
}

} // namespace fgai
