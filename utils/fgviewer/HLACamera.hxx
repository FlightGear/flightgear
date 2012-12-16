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

#ifndef HLACamera_hxx
#define HLACamera_hxx

#include <simgear/hla/HLAArrayDataElement.hxx>
#include <simgear/hla/HLAObjectClass.hxx>

namespace fgviewer {

class HLACameraClass;

class HLACamera : public simgear::HLAObjectInstance {
public:
    HLACamera(HLACameraClass* objectClass = 0);
    virtual ~HLACamera();

    virtual void createAttributeDataElements();

    const std::string& getName() const
    { return _name.getValue(); }
    void setName(const std::string& name)
    { _name.setValue(name); }

    const std::string& getViewer() const
    { return _viewer.getValue(); }
    void setViewer(const std::string& viewer)
    { _viewer.setValue(viewer); }

    const std::string& getDrawable() const
    { return _drawable.getValue(); }
    void setDrawable(const std::string& drawable)
    { _drawable.setValue(drawable); }

    const SGVec4i& getViewport() const
    { return _viewport.getValue(); }
    void setViewport(const SGVec4i& viewport)
    { _viewport.setValue(viewport); }

    const SGVec3d& getPosition() const
    { return _position.getValue(); }
    void setPosition(const SGVec3d& position)
    { _position.setValue(position); }

    const SGQuatd& getOrientation() const
    { return _orientation.getValue(); }
    void setOrientation(const SGQuatd& orientation)
    { _orientation.setValue(orientation); }

    SGLocationd getLocation() const
    { return SGLocationd(getPosition(), getOrientation()); }
    void setLocation(const SGLocationd& location)
    { setPosition(location.getPosition()); setOrientation(location.getOrientation()); }

private:
    simgear::HLAStringData _name;
    simgear::HLAStringData _viewer;
    simgear::HLAStringData _drawable;
    simgear::HLAVec4iData _viewport;
    simgear::HLAVec3dData _position;
    simgear::HLAQuat3dData _orientation;
};

} // namespace fgviewer

#endif
