// AircraftMesh.hxx -- Mesh for the computation of the wake induced force and
// moment on an aircraft.
//
// Written by Bertrand Coconnier, started March 2017.
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

#ifndef _FG_AEROMESH_HXX
#define _FG_AEROMESH_HXX

#include "WakeMesh.hxx"

class FGAIAircraft;
class AIWakeGroup;

class AircraftMesh : public WakeMesh {
public:
    AircraftMesh(double _span, double _chord);
    void setPosition(const SGVec3d& pos, const SGQuatd& orient);
    SGVec3d GetForce(const AIWakeGroup& wg, const SGVec3d& vel, double rho);
    const SGVec3d& GetMoment(void) const { return moment; };

#ifndef FG_TESTLIB
private:
#endif
    std::vector<SGVec3d> collPt, midPt;
    SGQuatd Te2b;
    SGVec3d moment;
};

typedef SGSharedPtr<AircraftMesh> AircraftMesh_ptr;

#endif
