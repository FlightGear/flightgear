// WakeMesh.cxx -- Mesh for the computation of a wing wake.
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

#include <vector>
#include <cmath>

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGVec3.hxx>

#include "WakeMesh.hxx"
extern "C" {
#include "../LaRCsim/ls_matrix.h"
}

WakeMesh::WakeMesh(double _span, double _chord)
    : nelm(10), span(_span), chord(_chord)
{
    double y1 = -0.5*span;
    double ds = span / nelm;

    for(int i=0; i<nelm; i++) {
        double y2 = y1 + ds;
        elements.push_back(new AeroElement(SGVec3d(-chord, y1, 0.),
                                           SGVec3d(0., y1, 0.),
                                           SGVec3d(0., y2, 0.),
                                           SGVec3d(-chord, y2, 0.)));
        y1 = y2;
    }

    influenceMtx = nr_matrix(1, nelm, 1, nelm);
    Gamma = nr_matrix(1, nelm, 1, 1);

    for (int i=0; i < nelm; ++i) {
        SGVec3d normal = elements[i]->getNormal();
        SGVec3d collPt = elements[i]->getCollocationPoint();

        for (int j=0; j < nelm; ++j)
            influenceMtx[i+1][j+1] = dot(elements[j]->getInducedVelocity(collPt),
                                         normal);
    }

    // Compute the inverse matrix with the Gauss-Jordan algorithm
    nr_gaussj(influenceMtx, nelm, 0, 0);
}

WakeMesh::~WakeMesh()
{
    nr_free_matrix(influenceMtx, 1, nelm, 1, nelm);
    nr_free_matrix(Gamma, 1, nelm, 1, 1);
}

double WakeMesh::computeAoA(double vel, double rho, double weight)
{
    for (int i=1; i<=nelm; ++i) {
        Gamma[i][1] = 0.0;
        for (int k=1; k<=nelm; ++k)
            Gamma[i][1] -= influenceMtx[i][k];
        Gamma[i][1] *= vel;
    }

    // Compute the lift only. Velocities in the z direction are discarded
    // because they only produce drag. This include the vertical component
    // vel*sin(alpha) and the induced velocities on the bound vortex.
    // This assumption is only valid for small angles.
    SGVec3d f(0.,0.,0.);
    SGVec3d v(-vel, 0.0, 0.0);

    for (int i=0; i<nelm; ++i)
        f += rho*Gamma[i+1][1]*cross(v, elements[i]->getBoundVortex());

    double sinAlpha = -weight/f[2];

    for (int i=1; i<=nelm; ++i)
        Gamma[i][1] *= sinAlpha;

    return asin(sinAlpha);
}

SGVec3d WakeMesh::getInducedVelocityAt(const SGVec3d& at) const
{
    SGVec3d v(0., 0., 0.);

    for (int i=0; i<nelm; ++i)
        v += Gamma[i+1][1] * elements[i]->getInducedVelocity(at);

    return v;
}
