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

#ifndef HLAViewer_hxx
#define HLAViewer_hxx

#include <simgear/hla/HLAArrayDataElement.hxx>
#include <simgear/hla/HLAObjectClass.hxx>

namespace fgviewer {

class HLAViewerClass;

class HLAViewer : public simgear::HLAObjectInstance {
public:
    HLAViewer(HLAViewerClass* objectClass = 0);
    virtual ~HLAViewer();

    virtual void createAttributeDataElements();

private:
    simgear::HLAStringData _name;
};

} // namespace fgviewer

#endif
