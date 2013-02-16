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

#ifndef HLAPerspectiveCamera_hxx
#define HLAPerspectiveCamera_hxx

#include <simgear/hla/HLABasicDataElement.hxx>
#include "Frustum.hxx"
#include "HLACamera.hxx"

namespace fgviewer {

class HLAPerspectiveCameraClass;

class HLAPerspectiveCamera : public HLACamera {
public:
    HLAPerspectiveCamera(HLAPerspectiveCameraClass* objectClass);
    virtual ~HLAPerspectiveCamera();

    virtual void reflectAttributeValues(const simgear::HLAIndexList& indexList, const simgear::RTIData& tag);
    virtual void reflectAttributeValues(const simgear::HLAIndexList& indexList, const SGTimeStamp& timeStamp, const simgear::RTIData& tag);

    virtual void createAttributeDataElements();

    double getLeft() const
    { return _left.getValue(); }
    void setLeft(double left)
    { _left.setValue(left); }

    double getRight() const
    { return _right.getValue(); }
    void setRight(double right)
    { _right.setValue(right); }

    double getBottom() const
    { return _bottom.getValue(); }
    void setBottom(double bottom)
    { _bottom.setValue(bottom); }

    double getTop() const
    { return _top.getValue(); }
    void setTop(double top)
    { _top.setValue(top); }

    double getDistance() const
    { return _distance.getValue(); }
    void setDistance(double distance)
    { _distance.setValue(distance); }

    Frustum getFrustum() const;
    void setFrustum(const Frustum& frustum);

private:
    simgear::HLADoubleData _left;
    simgear::HLADoubleData _right;
    simgear::HLADoubleData _bottom;
    simgear::HLADoubleData _top;
    simgear::HLADoubleData _distance;
};

} // namespace fgviewer

#endif
