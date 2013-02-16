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

#ifndef HLAAirVehicleClass_hxx
#define HLAAirVehicleClass_hxx

#include "HLASceneObjectClass.hxx"

namespace fgai {

class HLAAirVehicleClass : public HLASceneObjectClass {
public:
    HLAAirVehicleClass(const std::string& name, simgear::HLAFederate* federate);
    virtual ~HLAAirVehicleClass();

    /// Create a new instance of this class.
    virtual simgear::HLAObjectInstance* createObjectInstance(const std::string& name);

    virtual void createAttributeDataElements(simgear::HLAObjectInstance& objectInstance);

    bool setCallSignIndex(const std::string& path)
    { return getDataElementIndex(_callSignIndex, path); }
    const simgear::HLADataElementIndex& getCallSignIndex() const
    { return _callSignIndex; }

    bool setTransponderIndex(const std::string& path)
    { return getDataElementIndex(_transponderIndex, path); }
    const simgear::HLADataElementIndex& getTransponderIndex() const
    { return _transponderIndex; }

private:
    simgear::HLADataElementIndex _callSignIndex;
    simgear::HLADataElementIndex _transponderIndex;
};

} // namespace fgai

#endif
