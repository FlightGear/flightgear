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

#ifndef HLACameraClass_hxx
#define HLACameraClass_hxx

#include <string>
#include <simgear/hla/HLAObjectClass.hxx>

namespace fgviewer {

class HLACameraClass : public simgear::HLAObjectClass {
public:
    HLACameraClass(const std::string& name, simgear::HLAFederate* federate);
    virtual ~HLACameraClass();

    /// Create a new instance of this class.
    virtual simgear::HLAObjectInstance* createObjectInstance(const std::string& name);

    virtual void createAttributeDataElements(simgear::HLAObjectInstance& objectInstance);

    bool setNameIndex(const std::string& path)
    { return getDataElementIndex(_nameIndex, path); }
    const simgear::HLADataElementIndex& getNameIndex() const
    { return _nameIndex; }

    bool setViewerIndex(const std::string& path)
    { return getDataElementIndex(_viewerIndex, path); }
    const simgear::HLADataElementIndex& getViewerIndex() const
    { return _viewerIndex; }

    bool setDrawableIndex(const std::string& path)
    { return getDataElementIndex(_drawableIndex, path); }
    const simgear::HLADataElementIndex& getDrawableIndex() const
    { return _drawableIndex; }

    bool setViewportIndex(const std::string& path)
    { return getDataElementIndex(_viewportIndex, path); }
    const simgear::HLADataElementIndex& getViewportIndex() const
    { return _viewportIndex; }

    bool setPositionIndex(const std::string& path)
    { return getDataElementIndex(_positionIndex, path); }
    const simgear::HLADataElementIndex& getPositionIndex() const
    { return _positionIndex; }

    bool setOrientationIndex(const std::string& path)
    { return getDataElementIndex(_orientationIndex, path); }
    const simgear::HLADataElementIndex& getOrientationIndex() const
    { return _orientationIndex; }

private:
    simgear::HLADataElementIndex _nameIndex;
    simgear::HLADataElementIndex _viewerIndex;
    simgear::HLADataElementIndex _drawableIndex;
    simgear::HLADataElementIndex _viewportIndex;
    simgear::HLADataElementIndex _positionIndex;
    simgear::HLADataElementIndex _orientationIndex;
};

} // namespace fgviewer

#endif
