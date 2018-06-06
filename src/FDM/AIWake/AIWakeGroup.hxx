// AIWakeGroup.hxx -- Group of AI wake meshes for the computation of the induced
// wake.
//
// Written by Bertrand Coconnier, started April 2017.
//
// Copyright (C) 2017  Bertrand Coconnier  - bcoconni@users.sf.net
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// $Id$

#ifndef _FG_AIWAKEGROUP_HXX
#define _FG_AIWAKEGROUP_HXX

#include <simgear/props/propsfwd.hxx>

#include "FDM/AIWake/WakeMesh.hxx"

namespace FGTestApi { namespace PrivateAccessor { namespace FDM { class Accessor; } } }
class FGAIAircraft;

class AIWakeGroup {
    friend class FGTestApi::PrivateAccessor::FDM::Accessor;

    struct AIWakeData {
        explicit AIWakeData(WakeMesh* m = nullptr) : mesh(m) {}

        SGVec3d position {SGVec3d::zeros()};
        SGQuatd Te2b {SGQuatd::unit()};
        bool visited {false};
        WakeMesh_ptr mesh;
    };

    std::map<int, AIWakeData> _aiWakeData;
    SGPropertyNode_ptr _density_slugft;

public:
    AIWakeGroup(void);
    void AddAI(FGAIAircraft* ai);
    SGVec3d getInducedVelocityAt(const SGVec3d& pt) const;
    // Garbage collection
    void gc(void);
};

#endif
