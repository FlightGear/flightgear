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

class HLASceneObject::Location : public simgear::HLAAbstractLocation {
public:
    Location() :
        _position(SGVec3d::zeros()),
        _imag(SGVec3d::zeros()),
        _angularVelocity(SGVec3d::zeros()),
        _linearVelocity(SGVec3d::zeros())
    { }

    virtual SGLocationd getLocation() const
    { return SGLocationd(_position, SGQuatd::fromPositiveRealImag(_imag)); }
    virtual void setLocation(const SGLocationd& location)
    {
        _position = location.getPosition();
        _imag = location.getOrientation().getPositiveRealImag();
    }

    virtual SGVec3d getCartPosition() const
    { return _position; }
    virtual void setCartPosition(const SGVec3d& position)
    { _position = position; }

    virtual SGQuatd getCartOrientation() const
    { return SGQuatd::fromPositiveRealImag(_imag); }
    virtual void setCartOrientation(const SGQuatd& orientation)
    { _imag = orientation.getPositiveRealImag(); }

    virtual SGVec3d getAngularBodyVelocity() const
    { return _angularVelocity; }
    virtual void setAngularBodyVelocity(const SGVec3d& angularVelocity)
    { _angularVelocity = angularVelocity; }

    virtual SGVec3d getLinearBodyVelocity() const
    { return _linearVelocity; }
    virtual void setLinearBodyVelocity(const SGVec3d& linearVelocity)
    { _linearVelocity = linearVelocity; }

    virtual double getTimeDifference(const SGTimeStamp& timeStamp) const
    { return _position.getDataElement()->getTimeDifference(timeStamp); }

    simgear::HLADataElement* getPositionDataElement()
    { return _position.getDataElement(); }
    simgear::HLADataElement* getOrientationDataElement()
    { return _imag.getDataElement(); }

    simgear::HLADataElement* getAngularVelocityDataElement()
    { return _angularVelocity.getDataElement(); }
    simgear::HLADataElement* getLinearVelocityDataElement()
    { return _linearVelocity.getDataElement(); }

private:
    simgear::HLAVec3dData _position;
    simgear::HLAVec3dData _imag;

    simgear::HLAVec3dData _angularVelocity;
    simgear::HLAVec3dData _linearVelocity;
};


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

    Location* location = new Location;
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
