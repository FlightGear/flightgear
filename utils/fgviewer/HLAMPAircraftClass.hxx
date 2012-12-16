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

#ifndef HLAMPAircraftClass_hxx
#define HLAMPAircraftClass_hxx

#include "HLASceneObjectClass.hxx"
#include "HLAMPAircraft.hxx"

namespace fgviewer {

class HLAMPAircraftClass : public HLASceneObjectClass {
public:
    HLAMPAircraftClass(const std::string& name, simgear::HLAFederate* federate);
    virtual ~HLAMPAircraftClass();

    virtual HLAMPAircraft* createObjectInstance(const std::string& name);

    virtual void createAttributeDataElements(simgear::HLAObjectInstance& objectInstance);

    bool setSimTimeIndex(const std::string& path)
    { return getDataElementIndex(_simTimeIndex, path); }
    const simgear::HLADataElementIndex& getSimTimeIndex() const
    { return _simTimeIndex; }

    bool setModelPathIndex(const std::string& path)
    { return getDataElementIndex(_modelPathIndex, path); }
    const simgear::HLADataElementIndex& getModelPathIndex() const
    { return _modelPathIndex; }

private:
    simgear::HLADataElementIndex _simTimeIndex;
    simgear::HLADataElementIndex _modelPathIndex;
};

} // namespace fgviewer

#endif
