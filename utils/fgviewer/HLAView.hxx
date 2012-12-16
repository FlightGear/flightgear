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

#ifndef HLAView_hxx
#define HLAView_hxx

#include <simgear/hla/HLAArrayDataElement.hxx>
#include <simgear/hla/HLAObjectInstance.hxx>

#include "HLAObjectReferenceData.hxx"
#include "HLASceneObject.hxx"

namespace fgviewer {

class HLAViewClass;

class HLAView : public simgear::HLAObjectInstance {
public:
    HLAView(HLAViewClass* objectClass, const SGWeakPtr<simgear::HLAFederate>& federate);
    virtual ~HLAView();

    virtual void createAttributeDataElements();

    virtual void discoverInstance(const simgear::RTIData& tag);
    virtual void removeInstance(const simgear::RTIData& tag);

    virtual void registerInstance(simgear::HLAObjectClass* objectClass);
    virtual void deleteInstance(const simgear::RTIData& tag);

    const std::string& getNameAttribute() const
    { return _name.getValue(); }
    void setNameAttribute(const std::string& name)
    { _name.setValue(name); }

    const SGVec3d& getPosition() const
    { return _position.getValue(); }
    void setPosition(const SGVec3d& position)
    { _position.setValue(position); }

    const SGQuatd& getOrientation() const
    { return _orientation.getValue(); }
    void setOrientation(const SGQuatd& orientation)
    { _orientation.setValue(orientation); }

    const HLASceneObject* getSceneObject() const;
    HLASceneObject* getSceneObject();
    void setSceneObject(HLASceneObject* sceneObject);

    virtual SGLocationd getLocation(const SGTimeStamp& simTime) const;

private:
    simgear::HLAStringData _name;

    simgear::HLAVec3dData _position;
    simgear::HLAQuat3dData _orientation;

    simgear::HLAObjectReferenceData<HLASceneObject> _sceneObject;
};

} // namespace fgviewer

#endif
