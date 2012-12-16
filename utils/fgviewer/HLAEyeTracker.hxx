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

#ifndef HLAEyeTracker_hxx
#define HLAEyeTracker_hxx

#include <simgear/hla/HLAArrayDataElement.hxx>
#include <simgear/hla/HLAObjectClass.hxx>

namespace fgviewer {

class HLAEyeTrackerClass;

class HLAEyeTracker : public simgear::HLAObjectInstance {
public:
    HLAEyeTracker(HLAEyeTrackerClass* objectClass = 0);
    virtual ~HLAEyeTracker();

    virtual void createAttributeDataElements();

    const SGVec3d& getLeftEyeOffset() const
    { return _leftEyeOffset.getValue(); }
    void setLeftEyeOffset(const SGVec3d& leftEyeOffset)
    { _leftEyeOffset.setValue(leftEyeOffset); }

    const SGVec3d& getRightEyeOffset() const
    { return _rightEyeOffset.getValue(); }
    void setRightEyeOffset(const SGVec3d& rightEyeOffset)
    { _rightEyeOffset.setValue(rightEyeOffset); }

private:
    simgear::HLAVec3dData _leftEyeOffset;
    simgear::HLAVec3dData _rightEyeOffset;
};

} // namespace fgviewer

#endif
