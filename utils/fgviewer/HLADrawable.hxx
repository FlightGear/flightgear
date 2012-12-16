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

#ifndef HLADrawable_hxx
#define HLADrawable_hxx

#include <simgear/hla/HLAArrayDataElement.hxx>
#include <simgear/hla/HLAObjectClass.hxx>

namespace fgviewer {

class HLADrawableClass;

class HLADrawable : public simgear::HLAObjectInstance {
public:
    HLADrawable(HLADrawableClass* objectClass = 0);
    virtual ~HLADrawable();

    virtual void createAttributeDataElements();

    const std::string& getName() const
    { return _name.getValue(); }
    void setName(const std::string& name)
    { _name.setValue(name); }

    const std::string& getRenderer() const
    { return _renderer.getValue(); }
    void setRenderer(const std::string& renderer)
    { _renderer.setValue(renderer); }

    const std::string& getDisplay() const
    { return _display.getValue(); }
    void setDisplay(const std::string& display)
    { _display.setValue(display); }

private:
    simgear::HLAStringData _name;
    // This is normally this object reference, but here it should be sufficient to work with the name
    // simgear::HLAObjectReferenceData<HLARenderer> _renderer;
    simgear::HLAStringData _renderer;
    simgear::HLAStringData _display;
};

} // namespace fgviewer

#endif
