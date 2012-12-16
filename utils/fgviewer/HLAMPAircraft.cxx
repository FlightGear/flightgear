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

#include "HLAMPAircraft.hxx"

#include "HLAMPAircraftClass.hxx"

namespace fgviewer {

HLAMPAircraft::HLAMPAircraft(HLASceneObjectClass* objectClass, const SGWeakPtr<simgear::HLAFederate>& federate) :
    HLASceneObject(objectClass, federate),
    _simTimeOffset(SGLimitsd::max())
{
}

HLAMPAircraft::~HLAMPAircraft()
{
}

void
HLAMPAircraft::reflectAttributeValues(const simgear::HLAIndexList& indexList, const simgear::RTIData& tag)
{
    HLASceneObject::reflectAttributeValues(indexList, tag);
}

void
HLAMPAircraft::reflectAttributeValues(const simgear::HLAIndexList& indexList, const SGTimeStamp& timeStamp, const simgear::RTIData& tag)
{
    HLASceneObject::reflectAttributeValues(indexList, timeStamp, tag);
}

void
HLAMPAircraft::createAttributeDataElements()
{
    /// FIXME at some point we should not need that anymore
    HLASceneObject::createAttributeDataElements();

    assert(dynamic_cast<HLAMPAircraftClass*>(getObjectClass().get()));
    HLAMPAircraftClass& objectClass = static_cast<HLAMPAircraftClass&>(*getObjectClass());

    setAttributeDataElement(objectClass.getSimTimeIndex(), _simTime.getDataElement());
    setAttributeDataElement(objectClass.getModelPathIndex(), getModelDataElement());
}

SGLocationd
HLAMPAircraft::getLocation(const SGTimeStamp& timeStamp) const
{
    // FIXME: puh, is frame rate dependent. Think about something more robust ...
    if (_lastTimeStamp != timeStamp) {
        double simTimeOffset = timeStamp.toSecs() - _simTime.getValue();
        if (2 < fabs(simTimeOffset - _simTimeOffset))
            _simTimeOffset = simTimeOffset;
        else
            _simTimeOffset = 0.999*_simTimeOffset + 0.001*simTimeOffset;
        _lastTimeStamp = timeStamp;
    }
    double dt = timeStamp.toSecs() - _simTime.getValue() - _simTimeOffset;
    
    SGLocationd location = HLASceneObject::getLocation(timeStamp);
    location.eulerStepBodyVelocitiesMidOrientation(dt, _getRelativeLinearVelocity(), _getRelativeAngularVelocity());
    return location;
}

} // namespace fgviewer
