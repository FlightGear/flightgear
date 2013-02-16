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

#include "HLACamera.hxx"

#include <simgear/hla/HLAArrayDataElement.hxx>
#include "HLACameraClass.hxx"

namespace fgviewer {

HLACamera::HLACamera(HLACameraClass* objectClass) :
    HLAObjectInstance(objectClass)
{
}

HLACamera::~HLACamera()
{
}

void
HLACamera::createAttributeDataElements()
{
    /// FIXME at some point we should not need that anymore
    HLAObjectInstance::createAttributeDataElements();

    assert(dynamic_cast<HLACameraClass*>(getObjectClass().get()));
    HLACameraClass& objectClass = static_cast<HLACameraClass&>(*getObjectClass());

    setAttributeDataElement(objectClass.getNameIndex(), _name.getDataElement());
    setAttributeDataElement(objectClass.getViewerIndex(), _viewer.getDataElement());
    setAttributeDataElement(objectClass.getDrawableIndex(), _drawable.getDataElement());
    setAttributeDataElement(objectClass.getViewportIndex(), _viewport.getDataElement());
    setAttributeDataElement(objectClass.getPositionIndex(), _position.getDataElement());
    setAttributeDataElement(objectClass.getOrientationIndex(), _orientation.getDataElement());
}

} // namespace fgviewer
