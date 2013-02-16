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

#include "HLAPerspectiveCamera.hxx"

#include "HLAPerspectiveCameraClass.hxx"

namespace fgviewer {

HLAPerspectiveCamera::HLAPerspectiveCamera(HLAPerspectiveCameraClass* objectClass) :
    HLACamera(objectClass)
{
}

HLAPerspectiveCamera::~HLAPerspectiveCamera()
{
}

void
HLAPerspectiveCamera::reflectAttributeValues(const simgear::HLAIndexList& indexList, const simgear::RTIData& tag)
{
    HLACamera::reflectAttributeValues(indexList, tag);
    /// Here push data to the osg viewer side
}

void
HLAPerspectiveCamera::reflectAttributeValues(const simgear::HLAIndexList& indexList, const SGTimeStamp& timeStamp, const simgear::RTIData& tag)
{
    HLACamera::reflectAttributeValues(indexList, timeStamp, tag);
    /// Here push data to the osg viewer side
}

void
HLAPerspectiveCamera::createAttributeDataElements()
{
    HLACamera::createAttributeDataElements();

    assert(dynamic_cast<HLAPerspectiveCameraClass*>(getObjectClass().get()));
    HLAPerspectiveCameraClass& objectClass = static_cast<HLAPerspectiveCameraClass&>(*getObjectClass());

    setAttributeDataElement(objectClass.getLeftIndex(), _left.getDataElement());
    setAttributeDataElement(objectClass.getRightIndex(), _right.getDataElement());
    setAttributeDataElement(objectClass.getBottomIndex(), _bottom.getDataElement());
    setAttributeDataElement(objectClass.getTopIndex(), _top.getDataElement());
    setAttributeDataElement(objectClass.getDistanceIndex(), _distance.getDataElement());
}

Frustum
HLAPerspectiveCamera::getFrustum() const
{
    return Frustum(getLeft(), getRight(), getBottom(), getTop(), getDistance());
}

void
HLAPerspectiveCamera::setFrustum(const Frustum& frustum)
{
    setLeft(frustum._left);
    setRight(frustum._right);
    setBottom(frustum._bottom);
    setTop(frustum._top);
    setDistance(frustum._near);
}

} // namespace fgviewer
