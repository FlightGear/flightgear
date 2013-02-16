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

#ifndef HLAWindowDrawable_hxx
#define HLAWindowDrawable_hxx

#include <simgear/hla/HLABasicDataElement.hxx>
#include "HLADrawable.hxx"

namespace fgviewer {

class HLAWindowDrawableClass;

class HLAWindowDrawable : public HLADrawable {
public:
    HLAWindowDrawable(HLAWindowDrawableClass* objectClass = 0);
    virtual ~HLAWindowDrawable();

    virtual void createAttributeDataElements();

    bool getFullscreen() const
    { return _fullscreen.getValue(); }
    void setFullscreen(bool fullscreen)
    { _fullscreen.setValue(fullscreen); }

    const SGVec2i& getPosition() const
    { return _position.getValue(); }
    void setPosition(const SGVec2i& position)
    { _position.setValue(position); }

    const SGVec2i& getSize() const
    { return _size.getValue(); }
    void setSize(const SGVec2i& size)
    { _size.setValue(size); }

private:
    simgear::HLABoolData _fullscreen;
    simgear::HLAVec2iData _position;
    simgear::HLAVec2iData _size;
};

} // namespace fgviewer

#endif
