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

#ifndef HLAMPAircraft_hxx
#define HLAMPAircraft_hxx

#include <simgear/hla/HLAArrayDataElement.hxx>
#include "HLASceneObject.hxx"

namespace fgai {

class HLAMPAircraftClass;

class HLAMPAircraft : public HLASceneObject {
public:
    HLAMPAircraft(HLAMPAircraftClass* objectClass = 0);
    virtual ~HLAMPAircraft();

    virtual void createAttributeDataElements();

    void setModelPath(const std::string& modelPath)
    { _modelPath.setValue(modelPath); }
    const std::string& getModelPath() const
    { return _modelPath.getValue(); }

    void setModelLivery(const std::string& modelLivery)
    { _modelLivery.setValue(modelLivery); }
    const std::string& getModelLivery() const
    { return _modelLivery.getValue(); }

    /// FIXME feed this by the simtime of the positon?!
    void setSimTime(double simTime)
    { _simTime.setValue(simTime); }
    double getSimTime() const
    { return _simTime.getValue(); }

private:
    // The model stuff
    simgear::HLAStringData _modelPath;
    simgear::HLAStringData _modelLivery;
    
    // The current simTime
    simgear::HLADoubleData _simTime;
};

} // namespace fgai

#endif
