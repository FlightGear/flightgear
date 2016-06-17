// initialstate.cxx -- setup initial state of the aircraft
//
// Written by James Turner
//
// Copyright (C) 2016 James Turner <zakalawe@mac.com>
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
// $Id$

#include "config.h"

#include "initialstate.hxx"

#include <algorithm>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>

#include <Main/fg_props.hxx>
#include <GUI/MessageBox.hxx>

using namespace simgear;

namespace {

    class NodeValue
    {
    public:
        NodeValue(const std::string& s) : v(s) {}
        bool operator()(const SGPropertyNode_ptr n) const
        {
            return (v == n->getStringValue());
        }
    private:
        std::string v;
    };

    SGPropertyNode_ptr nodeForState(const std::string& nm)
    {
        SGPropertyNode_ptr sim = fgGetNode("/sim");
        const PropertyList& states = sim->getChildren("state");
        PropertyList::const_iterator it;
        for (it = states.begin(); it != states.end(); ++it) {
            const PropertyList& names = (*it)->getChildren("name");
            if (std::find_if(names.begin(), names.end(), NodeValue(nm)) != names.end()) {
                return *it;
            }
        }

        return SGPropertyNode_ptr();
    }

} // of anonymous namespace

namespace flightgear
{

bool isInitialStateName(const std::string& name)
{
    SGPropertyNode_ptr n = nodeForState(name);
    return n.valid();
}

void applyInitialState()
{
    std::string nm = fgGetString("/sim/aircraft-state");
    if (nm.empty()) {
        return;
    }

    SGPropertyNode_ptr stateNode = nodeForState(nm);
    if (!stateNode) {
        SG_LOG(SG_AIRCRAFT, SG_WARN, "missing state node for:" << nm);
        std::string aircraft = fgGetString("/sim/aircraft");
        modalMessageBox("Unknown aircraft state",
                                    "The selected aircraft (" + aircraft + ") does not have a stage '" + nm + "')");

        return;
    }

    SG_LOG(SG_AIRCRAFT, SG_INFO, "Applying aircraft state:" << nm);

    // copy all overlay properties to the tree
    copyProperties(stateNode->getChild("overlay"), globals->get_props());
}

} // of namespace flightgear