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

#include "HLAPerspectiveCameraClass.hxx"

#include "HLAPerspectiveCamera.hxx"

namespace fgviewer {

HLAPerspectiveCameraClass::HLAPerspectiveCameraClass(const std::string& name, simgear::HLAFederate* federate) :
    HLACameraClass(name, federate)
{
}

HLAPerspectiveCameraClass::~HLAPerspectiveCameraClass()
{
}

simgear::HLAObjectInstance*
HLAPerspectiveCameraClass::createObjectInstance(const std::string& name)
{
    return new HLAPerspectiveCamera(this);
}

void
HLAPerspectiveCameraClass::createAttributeDataElements(simgear::HLAObjectInstance& objectInstance)
{
    if (_leftIndex.empty())
        _leftIndex = getDataElementIndex("projection.left");
    if (_rightIndex.empty())
        _rightIndex = getDataElementIndex("projection.right");
    if (_bottomIndex.empty())
        _bottomIndex = getDataElementIndex("projection.bottom");
    if (_topIndex.empty())
        _topIndex = getDataElementIndex("projection.top");
    if (_distanceIndex.empty())
        _distanceIndex = getDataElementIndex("projection.distance");
    HLACameraClass::createAttributeDataElements(objectInstance);
}

} // namespace fgviewer
