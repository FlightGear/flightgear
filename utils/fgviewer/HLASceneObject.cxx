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

#include <osg/Transform>

#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/hla/HLAArrayDataElement.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "HLASceneObjectClass.hxx"
#include "HLAViewerFederate.hxx"

namespace fgviewer {

class HLASceneObject::_ModelDataElement : public simgear::HLAProxyDataElement {
public:
    _ModelDataElement(const SGWeakPtr<HLASceneObject>& sceneObject) :
        _sceneObject(sceneObject)
    { }
    virtual ~_ModelDataElement()
    { }
    
    virtual bool decode(simgear::HLADecodeStream& stream)
    {
        if (!HLAProxyDataElement::decode(stream))
            return false;
        
        // No change?
        if (!_name.getDataElement()->getDirty())
            return true;
        _name.getDataElement()->setDirty(false);
        SGSharedPtr<HLASceneObject> sceneObject = _sceneObject.lock();
        if (sceneObject.valid())
            sceneObject->_setModelFileName(_name.getValue());
        return true;
    }
    
protected:
    virtual simgear::HLAStringDataElement* _getDataElement()
    { return _name.getDataElement(); }
    virtual const simgear::HLAStringDataElement* _getDataElement() const
    { return _name.getDataElement(); }
    
private:
    const SGWeakPtr<HLASceneObject> _sceneObject;
    simgear::HLAStringData _name;
};

class HLASceneObject::_Transform : public osg::Transform {
public:
    _Transform(const SGWeakPtr<const HLASceneObject>& sceneObject) :
        _sceneObject(sceneObject)
    {
        setCullingActive(false);
    }
    virtual ~_Transform()
    {
    }

    virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
    {
        if (!nv)
            return false;
        const osg::FrameStamp* frameStamp = nv->getFrameStamp();
        if (!frameStamp)
            return false;
        
        SGLocationd location = _getLocation(SGTimeStamp::fromSec(frameStamp->getSimulationTime()));
        // the position of the view
        matrix.preMultTranslate(toOsg(location.getPosition()));
        // the orientation of the view
        matrix.preMultRotate(toOsg(location.getOrientation()));
    
        return true;
    }
    virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
    {
        if (!nv)
            return false;
        const osg::FrameStamp* frameStamp = nv->getFrameStamp();
        if (!frameStamp)
            return false;
        
        SGLocationd location = _getLocation(SGTimeStamp::fromSec(frameStamp->getSimulationTime()));
        // the position of the view
        matrix.postMultTranslate(toOsg(-location.getPosition()));
        // the orientation of the view
        matrix.postMultRotate(toOsg(inverse(location.getOrientation())));
    
        return true;
    }

    virtual osg::BoundingSphere computeBound() const
    {
        return osg::BoundingSphere();
    }

private:
    SGLocationd _getLocation(const SGTimeStamp& timeStamp) const
    {
        // transform from the simulation typical x-forward/y-right/z-down
        // to the flightgear model system x-back/y-right/z-up
        // SGQuatd simToModel = SGQuatd::fromEulerDeg(0, 180, 0);
        SGQuatd simToModel = SGQuatd::fromRealImag(0, SGVec3d(0, 1, 0));

        SGSharedPtr<const HLASceneObject> sceneObject = _sceneObject.lock();
        if (!sceneObject.valid())
            return SGLocationd(SGVec3d(0, 0, 0), simToModel);

        // Get the cartesian location at the given time stamp from the hla object
        SGLocationd location = sceneObject->getLocation(timeStamp);
        location.setOrientation(location.getOrientation()*simToModel);

        return location;
    }

    SGWeakPtr<const HLASceneObject> _sceneObject;
};

HLASceneObject::HLASceneObject(HLASceneObjectClass* objectClass, const SGWeakPtr<simgear::HLAFederate>& federate) :
    HLAObjectInstance(objectClass),
    _sceneObject(federate)
{
    _pagedLOD = new osg::PagedLOD;
    _pagedLOD->setRange(0, 0, 80000);
        
    _transform = new _Transform(this);
    _transform->addChild(_pagedLOD.get());
}

HLASceneObject::~HLASceneObject()
{
    _removeFromSceneGraph();
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

    setAttributeDataElement(objectClass.getSceneObjectNameIndex(), _sceneObject.getDataElement());
}

void
HLASceneObject::discoverInstance(const simgear::RTIData& tag)
{
    HLAObjectInstance::discoverInstance(tag);
    _addToSceneGraph();
}

void
HLASceneObject::removeInstance(const simgear::RTIData& tag)
{
    _removeFromSceneGraph();
    HLAObjectInstance::removeInstance(tag);
}

void
HLASceneObject::registerInstance(simgear::HLAObjectClass* objectClass)
{
    HLAObjectInstance::registerInstance(objectClass);
    _addToSceneGraph();
}

void
HLASceneObject::deleteInstance(const simgear::RTIData& tag)
{
    _removeFromSceneGraph();
    HLAObjectInstance::deleteInstance(tag);
}

const HLASceneObject*
HLASceneObject::getSceneObject() const
{
    return _sceneObject.getObject();
}

HLASceneObject*
HLASceneObject::getSceneObject()
{
    return _sceneObject.getObject();
}

void
HLASceneObject::setSceneObject(HLASceneObject* sceneObject)
{
    _sceneObject.setObject(sceneObject);
}

SGLocationd
HLASceneObject::getLocation(const SGTimeStamp& timeStamp) const
{
    const HLASceneObject* sceneObject = getSceneObject();
    if (!sceneObject)
        return _getRelativeLocation(timeStamp);
    return sceneObject->getLocation(timeStamp).getAbsoluteLocation(_getRelativeLocation(timeStamp));
}

simgear::HLADataElement*
HLASceneObject::getModelDataElement()
{
    return new _ModelDataElement(this);
}

osg::Node*
HLASceneObject::getNode()
{
    return _transform.get();
}

SGLocationd
HLASceneObject::_getRelativeLocation(const SGTimeStamp& timeStamp) const
{
    if (!_location.valid())
        return SGLocationd(SGVec3d::zeros(), SGQuatd::unit());
    return _location->getLocation(timeStamp);
}

SGVec3d
HLASceneObject::_getRelativeAngularVelocity() const
{
    if (!_location.valid())
        return SGVec3d(0, 0, 0);
    return _location->getAngularBodyVelocity();
}

SGVec3d
HLASceneObject::_getRelativeLinearVelocity() const
{
    if (!_location.valid())
        return SGVec3d(0, 0, 0);
    return _location->getLinearBodyVelocity();
}

void
HLASceneObject::_setModelFileName(const std::string& fileName)
{
    if (0 < _pagedLOD->getNumFileNames() && fileName == _pagedLOD->getFileName(0))
        return;

    SGSharedPtr<simgear::HLAFederate> federate = getFederate().lock();
    if (federate.valid()) {
        assert(dynamic_cast<HLAViewerFederate*>(federate.get()));
        HLAViewerFederate* viewerFederate = static_cast<HLAViewerFederate*>(federate.get());

        osg::ref_ptr<osgDB::Options> localOptions;
        localOptions = static_cast<osgDB::Options*>(viewerFederate->getReaderWriterOptions()->clone(osg::CopyOp()));
        // FIXME: particle systems have some issues. Do not use them currently.
        localOptions->setPluginStringData("SimGear::PARTICLESYSTEM", "OFF");
        _pagedLOD->setDatabaseOptions(localOptions.get());
    }

    // Finally set the file name
    if (unsigned numChildren = _pagedLOD->getNumChildren())
        _pagedLOD->removeChildren(0, numChildren);
    _pagedLOD->setFileName(0, fileName);
}

void
HLASceneObject::_addToSceneGraph()
{
    SGSharedPtr<simgear::HLAFederate> federate = getFederate().lock();
    if (!federate.valid())
        return;
    assert(dynamic_cast<HLAViewerFederate*>(federate.get()));
    HLAViewerFederate* viewerFederate = static_cast<HLAViewerFederate*>(federate.get());

    /// Add ourselves to the dynamic scene graph
    viewerFederate->addDynamicModel(getNode());
}

void
HLASceneObject::_removeFromSceneGraph()
{
    /// FIXME O(#SceneObjects), pool them and reuse in create
    unsigned i = _transform->getNumParents();
    while (i--) {
        osg::Group* parent = _transform->getParent(i);
        parent->removeChild(_transform.get());
    }
}

} // namespace fgviewer
