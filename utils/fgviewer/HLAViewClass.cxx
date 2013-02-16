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

#include "HLAViewClass.hxx"

#include "HLAView.hxx"

namespace fgviewer {

HLAViewClass::HLAViewClass(const std::string& name, simgear::HLAFederate* federate) :
    HLAObjectClass(name, federate)
{
}

HLAViewClass::~HLAViewClass()
{
}

simgear::HLAObjectInstance*
HLAViewClass::createObjectInstance(const std::string& name)
{
    return new HLAView(this, getFederate());
}

void
HLAViewClass::createAttributeDataElements(simgear::HLAObjectInstance& objectInstance)
{
    /// FIXME resolve these indices somewhere else!
    if (_nameIndex.empty())
        _nameIndex = getDataElementIndex("name");
    if (_positionIndex.empty())
        _positionIndex = getDataElementIndex("location.position");
    if (_orientationIndex.empty())
        _orientationIndex = getDataElementIndex("location.orientation");
    if (_sceneObjectNameIndex.empty())
        _sceneObjectNameIndex = getDataElementIndex("sceneObjectName");
    HLAObjectClass::createAttributeDataElements(objectInstance);
}

} // namespace fgviewer
