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

#ifndef HLAOrthographicCameraClass_hxx
#define HLAOrthographicCameraClass_hxx

#include "HLACameraClass.hxx"

namespace fgviewer {

class HLAOrthographicCameraClass : public HLACameraClass {
public:
    HLAOrthographicCameraClass(const std::string& name, simgear::HLAFederate* federate);
    virtual ~HLAOrthographicCameraClass();

    /// Create a new instance of this class.
    virtual simgear::HLAObjectInstance* createObjectInstance(const std::string& name);

    virtual void createAttributeDataElements(simgear::HLAObjectInstance& objectInstance);

    bool setLeftIndex(const std::string& path)
    { return getDataElementIndex(_leftIndex, path); }
    const simgear::HLADataElementIndex& getLeftIndex() const
    { return _leftIndex; }

    bool setRightIndex(const std::string& path)
    { return getDataElementIndex(_rightIndex, path); }
    const simgear::HLADataElementIndex& getRightIndex() const
    { return _rightIndex; }

    bool setBottomIndex(const std::string& path)
    { return getDataElementIndex(_bottomIndex, path); }
    const simgear::HLADataElementIndex& getBottomIndex() const
    { return _bottomIndex; }

    bool setTopIndex(const std::string& path)
    { return getDataElementIndex(_topIndex, path); }
    const simgear::HLADataElementIndex& getTopIndex() const
    { return _topIndex; }

private:
    simgear::HLADataElementIndex _leftIndex;
    simgear::HLADataElementIndex _rightIndex;
    simgear::HLADataElementIndex _bottomIndex;
    simgear::HLADataElementIndex _topIndex;
};

} // namespace fgviewer

#endif
