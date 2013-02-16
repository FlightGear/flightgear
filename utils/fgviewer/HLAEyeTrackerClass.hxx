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

#ifndef HLAEyeTrackerClass_hxx
#define HLAEyeTrackerClass_hxx

#include <string>
#include <simgear/hla/HLAObjectClass.hxx>

namespace fgviewer {

class HLAEyeTrackerClass : public simgear::HLAObjectClass {
public:
    HLAEyeTrackerClass(const std::string& name, simgear::HLAFederate* federate);
    virtual ~HLAEyeTrackerClass();

    /// Create a new instance of this class.
    virtual simgear::HLAObjectInstance* createObjectInstance(const std::string& name);

    virtual void createAttributeDataElements(simgear::HLAObjectInstance& objectInstance);

    bool setLeftEyeOffsetIndex(const std::string& path)
    { return getDataElementIndex(_leftEyeOffsetIndex, path); }
    const simgear::HLADataElementIndex& getLeftEyeOffsetIndex() const
    { return _leftEyeOffsetIndex; }

    bool setRightEyeOffsetIndex(const std::string& path)
    { return getDataElementIndex(_rightEyeOffsetIndex, path); }
    const simgear::HLADataElementIndex& getRightEyeOffsetIndex() const
    { return _rightEyeOffsetIndex; }

private:
    simgear::HLADataElementIndex _leftEyeOffsetIndex;
    simgear::HLADataElementIndex _rightEyeOffsetIndex;
};

} // namespace fgviewer

#endif
