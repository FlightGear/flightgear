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

#ifndef HLASceneObject_hxx
#define HLASceneObject_hxx

#include <osg/Node>
#include <osg/PagedLOD>
#include <osg/ref_ptr>

#include <simgear/hla/HLAObjectClass.hxx>
#include <simgear/hla/HLALocation.hxx>

#include "HLAObjectReferenceData.hxx"

namespace fgviewer {

class HLASceneObjectClass;

class HLASceneObject : public simgear::HLAObjectInstance {
public:
    HLASceneObject(HLASceneObjectClass* objectClass, const SGWeakPtr<simgear::HLAFederate>& federate);
    virtual ~HLASceneObject();

    virtual void createAttributeDataElements();

    virtual void discoverInstance(const simgear::RTIData& tag);
    virtual void removeInstance(const simgear::RTIData& tag);

    virtual void registerInstance(simgear::HLAObjectClass* objectClass);
    virtual void deleteInstance(const simgear::RTIData& tag);

    const HLASceneObject* getSceneObject() const;
    HLASceneObject* getSceneObject();
    void setSceneObject(HLASceneObject* sceneObject);

    virtual SGLocationd getLocation(const SGTimeStamp& timeStamp) const;

    simgear::HLADataElement* getModelDataElement();

    osg::Node* getNode();

protected:
    SGLocationd _getRelativeLocation(const SGTimeStamp& timeStamp) const;
    // Return the velocities of this current object without taking the referenced velocities into account
    SGVec3d _getRelativeAngularVelocity() const;
    SGVec3d _getRelativeLinearVelocity() const;

    void _setModelFileName(const std::string& fileName);

private:
    class _ModelDataElement;
    class _Transform;

    // Add the transform node to the scenegraph
    void _addToSceneGraph();
    // Remove the transform node from the scenegraph
    void _removeFromSceneGraph();

    // The location of this object
    SGSharedPtr<simgear::HLAAbstractLocation> _location;
    // If this object is living relative to an other one, this one is non zero
    simgear::HLAObjectReferenceData<HLASceneObject> _sceneObject;

    // The top level transform node for this scene object
    osg::ref_ptr<_Transform> _transform;
    osg::ref_ptr<osg::PagedLOD> _pagedLOD;
};

} // namespace fgviewer

#endif
