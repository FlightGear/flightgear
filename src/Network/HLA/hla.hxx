//
// Copyright (C) 2009 - 2010  Mathias Fröhlich <Mathias.Froehlich@web.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef _FG_HLA_HXX
#define _FG_HLA_HXX

#include <simgear/compiler.h>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/props/props.hxx>

#include <string>
#include <vector>
#include <Network/protocol.hxx>

namespace simgear {
class HLAFederate;
class HLAObjectClass;
class HLAObjectInstance;
}

class FGHLA : public FGProtocol {
public:
    FGHLA(const std::vector<std::string>& tokens);
    virtual ~FGHLA();

    virtual bool open();
    virtual bool process();
    virtual bool close();

private:
    /// All the utility classes we need currently
    class XMLConfigReader;
    class Federate;

    /// The configuration parameters extracted from the tokens in the constructor
    std::string _objectModelConfig;
    std::string _federation;
    std::string _federate;

    /// The toplevel rti class
    SGSharedPtr<Federate> _hlaFederate;
    /// This class that is used to send register the local instance
    SGSharedPtr<simgear::HLAObjectClass> _localAircraftClass;
    /// The local aircraft instance
    SGSharedPtr<simgear::HLAObjectInstance> _localAircraftInstance;
};

#endif // _FG_HLA_HXX
