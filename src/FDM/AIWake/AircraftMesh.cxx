// AircraftMesh.cxx -- Mesh for the computation of the wake induced force and
// moment on an aircraft.

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

#include <vector>
#include <cmath>

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGVec3.hxx>
#include <simgear/math/SGGeoc.hxx>
#include <simgear/math/SGGeod.hxx>
#include <simgear/math/SGQuat.hxx>
#include "AircraftMesh.hxx"
#include <FDM/flight.hxx>
#include "AIWakeGroup.hxx"
#include "AIModel/AIAircraft.hxx"

AircraftMesh::AircraftMesh(double _span, double _chord, const std::string& name)
    : WakeMesh(_span, _chord, name)
{
    collPt.resize(nelm, SGVec3d::zeros());
    midPt.resize(nelm, SGVec3d::zeros());
}

void AircraftMesh::setPosition(const SGVec3d& _pos, const SGQuatd& orient)
{
    SGVec3d pos = _pos * SG_METER_TO_FEET;
    SGGeoc geoc = SGGeoc::fromCart(pos);
    SGQuatd Te2l = SGQuatd::fromLonLatRad(geoc.getLongitudeRad(),
                                          geoc.getLatitudeRad());
    Te2b = Te2l * orient;

    for (int i=0; i<nelm; ++i) {
        SGVec3d pt = elements[i]->getCollocationPoint();
        collPt[i] = pos + Te2b.backTransform(pt);
        pt = elements[i]->getBoundVortexMidPoint();
        midPt[i] = pos + Te2b.backTransform(pt);
    }
}

SGVec3d AircraftMesh::GetForce(const AIWakeGroup& wg, const SGVec3d& vel,
                               double rho)
{
    std::vector<double> rhs;
    rhs.resize(nelm, 0.0);

    for (int i=0; i<nelm; ++i)
        rhs[i] = dot(elements[i]->getNormal(),
                     Te2b.transform(wg.getInducedVelocityAt(collPt[i])));

    for (int i=1; i<=nelm; ++i) {
        Gamma[i][1] = 0.0;
        for (int k=1; k<=nelm; ++k)
            Gamma[i][1] += influenceMtx[i][k]*rhs[k-1];
    }

    SGVec3d f(0.,0.,0.);
    moment = SGVec3d::zeros();

    for (int i=0; i<nelm; ++i) {
        SGVec3d mp = elements[i]->getBoundVortexMidPoint();
        SGVec3d v = Te2b.transform(wg.getInducedVelocityAt(midPt[i]));
        v += getInducedVelocityAt(mp);

        // The minus sign before vel to transform the aircraft velocity from the
        // body frame to wind frame.
        SGVec3d Fi = rho*Gamma[i+1][1]*cross(v-vel,
                                             elements[i]->getBoundVortex());
        f += Fi;
        moment += cross(mp, Fi);
    }

    return f;
}
