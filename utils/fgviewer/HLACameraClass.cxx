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

#include "HLACameraClass.hxx"

#include "HLACamera.hxx"

namespace fgviewer {

HLACameraClass::HLACameraClass(const std::string& name, simgear::HLAFederate* federate) :
    HLAObjectClass(name, federate)
{
}

HLACameraClass::~HLACameraClass()
{
}

simgear::HLAObjectInstance*
HLACameraClass::createObjectInstance(const std::string& name)
{
    return new HLACamera(this);
}

void
HLACameraClass::createAttributeDataElements(simgear::HLAObjectInstance& objectInstance)
{
    /// FIXME resolve these indices somewhere else!
    if (_nameIndex.empty())
        _nameIndex = getDataElementIndex("name");
    if (_viewerIndex.empty())
        _viewerIndex = getDataElementIndex("viewer");
    if (_drawableIndex.empty())
        _drawableIndex = getDataElementIndex("drawable");
    if (_viewportIndex.empty())
        _viewportIndex = getDataElementIndex("viewport");
    if (_positionIndex.empty())
        _positionIndex = getDataElementIndex("location.position");
    if (_orientationIndex.empty())
        _orientationIndex = getDataElementIndex("location.orientation");
    HLAObjectClass::createAttributeDataElements(objectInstance);
}

} // namespace fgviewer
