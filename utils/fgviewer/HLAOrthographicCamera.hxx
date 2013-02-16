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

#ifndef HLAOrthographicCamera_hxx
#define HLAOrthographicCamera_hxx

#include <simgear/hla/HLABasicDataElement.hxx>
#include "HLACamera.hxx"

namespace fgviewer {

class HLAOrthographicCameraClass;

class HLAOrthographicCamera : public HLACamera {
public:
    HLAOrthographicCamera(HLAOrthographicCameraClass* objectClass);
    virtual ~HLAOrthographicCamera();

    virtual void createAttributeDataElements();

    double getLeft() const
    { return _left.getValue(); }
    void setLeft(double left)
    { _left.setValue(left); }

    double getRight() const
    { return _right.getValue(); }
    void setRight(double right)
    { _right.setValue(right); }

    double getBottom() const
    { return _bottom.getValue(); }
    void setBottom(double bottom)
    { _bottom.setValue(bottom); }

    double getTop() const
    { return _top.getValue(); }
    void setTop(double top)
    { _top.setValue(top); }

private:
    simgear::HLADoubleData _left;
    simgear::HLADoubleData _right;
    simgear::HLADoubleData _bottom;
    simgear::HLADoubleData _top;
};

} // namespace fgviewer

#endif
