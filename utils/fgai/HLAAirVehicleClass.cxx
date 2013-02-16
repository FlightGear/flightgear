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

#include "HLAAirVehicleClass.hxx"

#include "HLAAirVehicle.hxx"

namespace fgai {

HLAAirVehicleClass::HLAAirVehicleClass(const std::string& name, simgear::HLAFederate* federate) :
    HLASceneObjectClass(name, federate)
{
}

HLAAirVehicleClass::~HLAAirVehicleClass()
{
}

simgear::HLAObjectInstance*
HLAAirVehicleClass::createObjectInstance(const std::string& name)
{
    return new HLAAirVehicle(this);
}

void
HLAAirVehicleClass::createAttributeDataElements(simgear::HLAObjectInstance& objectInstance)
{
    /// FIXME resolve these indices somewhere else!
    if (_callSignIndex.empty())
        _callSignIndex = getDataElementIndex("callSign");
    if (_transponderIndex.empty())
        _transponderIndex = getDataElementIndex("transponder");
    HLASceneObjectClass::createAttributeDataElements(objectInstance);
}

} // namespace fgai
