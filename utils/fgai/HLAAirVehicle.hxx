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

#ifndef HLAAirVehicle_hxx
#define HLAAirVehicle_hxx

#include <simgear/hla/HLAArrayDataElement.hxx>
#include <simgear/hla/HLABasicDataElement.hxx>
#include "HLASceneObject.hxx"

namespace fgai {

class HLAAirVehicleClass;

class HLAAirVehicle : public HLASceneObject {
public:
    HLAAirVehicle(HLAAirVehicleClass* objectClass = 0);
    virtual ~HLAAirVehicle();

    virtual void createAttributeDataElements();

    void setCallSign(const std::string& callSign)
    { _callSign.setValue(callSign); }
    const std::string& getCallSign() const
    { return _callSign.getValue(); }

    /// FIXME encode Mode a/b/c/whatever into a variant and make this backward compatible for all time!!!
    // void setTransponder(unsigned short transponder)
    // { _transponder.setValue(transponder); }
    // unsigned short getTransponder() const
    // { return _transponder.getValue(); }

private:
    simgear::HLAStringData _callSign;
    // simgear::HLAUShortData _transponder;
};

} // namespace fgai

#endif
