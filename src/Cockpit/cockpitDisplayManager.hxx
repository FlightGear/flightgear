// cockpitDisplayManager.hxx -- manage cockpit displays, typically
// rendered using a sub-camera or render-texture
//
// Copyright (C) 2012 James Turner  zakalawe@mac.com
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

#ifndef COCKPIT_DISPLAY_MGR_HXX
#define COCKPIT_DISPLAY_MGR_HXX 1

#include <simgear/compiler.h>

#include <vector>
#include <simgear/structure/subsystem_mgr.hxx>

class SGPropertyNode;

namespace flightgear
{

/**
 * Manage aircraft displays.
 */
class CockpitDisplayManager : public SGSubsystemGroup
{
public:
    CockpitDisplayManager ();
    virtual ~CockpitDisplayManager ();

    // Subsystem API.
    void init() override;
    InitStatus incrementalInit() override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "cockpit-displays"; }

private:
    bool build (SGPropertyNode* config_props);

    std::vector<std::string> _displays;
};

} // of namespace lfightgear

#endif // COCKPIT_DISPLAY_MGR_HXX
