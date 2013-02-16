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

#include "HLAView.hxx"

#include <simgear/hla/HLAArrayDataElement.hxx>
#include "HLAViewClass.hxx"
#include "HLAViewerFederate.hxx"

namespace fgviewer {

HLAView::HLAView(HLAViewClass* objectClass, const SGWeakPtr<simgear::HLAFederate>& federate) :
    HLAObjectInstance(objectClass),
    _sceneObject(federate)
{
}

HLAView::~HLAView()
{
}

void
HLAView::createAttributeDataElements()
{
    /// FIXME at some point we should not need that anymore
    HLAObjectInstance::createAttributeDataElements();

    assert(dynamic_cast<HLAViewClass*>(getObjectClass().get()));
    HLAViewClass& objectClass = static_cast<HLAViewClass&>(*getObjectClass());

    setAttributeDataElement(objectClass.getNameIndex(), _name.getDataElement());
    setAttributeDataElement(objectClass.getPositionIndex(), _position.getDataElement());
    setAttributeDataElement(objectClass.getOrientationIndex(), _orientation.getDataElement());
    setAttributeDataElement(objectClass.getSceneObjectNameIndex(), _sceneObject.getDataElement());
}

void
HLAView::discoverInstance(const simgear::RTIData& tag)
{
    HLAObjectInstance::discoverInstance(tag);

    SGSharedPtr<simgear::HLAFederate> federate = getFederate().lock();
    if (federate.valid()) {
        assert(dynamic_cast<HLAViewerFederate*>(federate.get()));
        static_cast<HLAViewerFederate*>(federate.get())->insertView(this);
    }
}

void
HLAView::removeInstance(const simgear::RTIData& tag)
{
    HLAObjectInstance::removeInstance(tag);

    SGSharedPtr<simgear::HLAFederate> federate = getFederate().lock();
    if (federate.valid()) {
        assert(dynamic_cast<HLAViewerFederate*>(federate.get()));
        static_cast<HLAViewerFederate*>(federate.get())->eraseView(this);
    }
}

void
HLAView::registerInstance(simgear::HLAObjectClass* objectClass)
{
    HLAObjectInstance::registerInstance(objectClass);

    SGSharedPtr<simgear::HLAFederate> federate = getFederate().lock();
    if (federate.valid()) {
        assert(dynamic_cast<HLAViewerFederate*>(federate.get()));
        static_cast<HLAViewerFederate*>(federate.get())->insertView(this);
    }
}

void
HLAView::deleteInstance(const simgear::RTIData& tag)
{
    SGSharedPtr<simgear::HLAFederate> federate = getFederate().lock();
    if (federate.valid()) {
        assert(dynamic_cast<HLAViewerFederate*>(federate.get()));
        static_cast<HLAViewerFederate*>(federate.get())->eraseView(this);
    }

    HLAObjectInstance::deleteInstance(tag);
}

const HLASceneObject*
HLAView::getSceneObject() const
{
    /// FIXME!!! The federate should track unresolved object references
    _sceneObject.recheckObject();
    return _sceneObject.getObject();
}

HLASceneObject*
HLAView::getSceneObject()
{
    /// FIXME!!! The federate should track unresolved object references
    _sceneObject.recheckObject();
    return _sceneObject.getObject();
}

void
HLAView::setSceneObject(HLASceneObject* sceneObject)
{
    _sceneObject.setObject(sceneObject);
}

SGLocationd
HLAView::getLocation(const SGTimeStamp& timeStamp) const
{
    const HLASceneObject* sceneObject = getSceneObject();
    SGLocationd location(getPosition(), getOrientation());
    if (!sceneObject)
        return location;
    return sceneObject->getLocation(timeStamp).getAbsoluteLocation(location);
}

} // namespace fgviewer
